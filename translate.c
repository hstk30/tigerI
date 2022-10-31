/*
 * Cause this file have multi-chatper code,
 * so layout by chatper, not coding rule.
 * TODO: when finish this project, then rearranage it.
 */
#include <stdio.h>

#include "translate.h"
#include "frame.h"
#include "printtree.h"
#include "canon.h"


static Tr_level OUTERMOST_LEVEL = NULL;
static F_fragList PROC_FRAG_HEAD = NULL;
static F_fragList PROC_FRAG_TAIL = NULL;
static F_fragList STR_FRAG_HEAD = NULL;
static F_fragList STR_FRAG_TAIL = NULL;

static void Tr_insert_proc(F_frag proc);
static void Tr_insert_str(F_frag str);

/*** stack frame or activation record rel ***/
struct Tr_level_ {
    Tr_level parent;
    Tr_accessList s_formals;  /* static link + formals */
    F_frame frame;
};

struct Tr_access_ {
    Tr_level level;
    F_access f_access;
};

static Tr_access Tr_Access(Tr_level level, F_access f_access) 
{
    Tr_access p = checked_malloc(sizeof(*p));

    p->level = level;
    p->f_access = f_access;

    return p;
}

Tr_accessList 
Tr_AccessList(Tr_access head, Tr_accessList tail) 
{
    Tr_accessList p = checked_malloc(sizeof(*p));

    p->head = head;
    p->tail = tail;

    return p;
}

static Tr_accessList 
makeSformals(Tr_level level, F_accessList formals) 
{
    if (formals == NULL)
        return NULL;

    return Tr_AccessList(
            Tr_Access(level, formals->head),
            makeSformals(level, formals->tail));
}

Tr_level Tr_outermost(void) 
{
    if (OUTERMOST_LEVEL == NULL) {
        Tr_level level = Tr_newLevel(NULL, Temp_namedlabel("tiger_global"), NULL);
        /* let no `NULL` */
        level->parent = level;
        OUTERMOST_LEVEL = level;
    } 

    return OUTERMOST_LEVEL;
}

Tr_level 
Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) 
{
    Tr_level level = checked_malloc(sizeof(*level));
    U_boolList static_link_formals = U_BoolList(TRUE, formals);
    level->parent = parent;
    level->frame = F_newFrame(name, static_link_formals);
    level->s_formals = makeSformals(level, F_formals(level->frame));

    return level;
}

Tr_access Tr_static_link(Tr_level level) 
{
    return level->s_formals->head;
}

Tr_accessList
Tr_formals(Tr_level level) 
{
    return level->s_formals->tail;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) 
{
    F_access fa = F_allocLocal(level->frame, escape); 

    return Tr_Access(level, fa);
}

/*** stack frame end ***/

/*** Intermediate representation rel***/
typedef struct patchList_ *patchList;
struct patchList_ {
    Temp_label *head;
    patchList tail;
};
static patchList PatchList(Temp_label *head, patchList tail);

struct Cx {
    patchList trues;
    patchList falses;
    T_stm stm;
};

struct Tr_exp_ {
    enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    } u;
};
static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

static patchList 
PatchList(Temp_label *head, patchList tail) 
{
    patchList p = checked_malloc(sizeof(*p));   

    p->head = head;
    p->tail = tail;

    return p;
}

static void 
doPatch(patchList t, Temp_label label) 
{
    for (; t; t = t->tail) {
        *(t->head) = label;
    }
    return ;
}

static patchList 
joinPatch(patchList first, patchList second) 
{
    if (!first)
        return second;

    for (; first->tail; first = first->tail) 
        ;
    first->tail = second;

    return first;
}

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) 
{
    Tr_expList p = checked_malloc(sizeof(*p));

    p->head = head;
    p->tail = tail;

    return p;
}

static Tr_exp Tr_Ex(T_exp ex) 
{
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_ex;
    p->u.ex = ex;

    return p;
}

static Tr_exp Tr_Nx(T_stm nx) 
{
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_nx;
    p->u.nx = nx;

    return p;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) 
{
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_cx;
    p->u.cx = (struct Cx) {
        .trues = trues, 
        .falses = falses, 
        .stm = stm
    };

    return p;
}


static T_exp 
unEx(Tr_exp e) 
{
    switch (e->kind) {
        case Tr_ex: return e->u.ex;
        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);

            return T_Eseq(
                    T_Move(T_Temp(r), T_Const(1)),
                    T_Eseq(
                        e->u.cx.stm,
                        T_Eseq(
                            T_Label(f),
                            T_Eseq(
                                T_Move(T_Temp(r), T_Const(0)),
                                T_Eseq(
                                    T_Label(t), 
                                    T_Temp(r))))));
        }
        case Tr_nx: return T_Eseq(e->u.nx, T_Const(0));
        default: assert(0);
    }
}

static T_stm
unNx(Tr_exp e) 
{
    switch (e->kind) {
        case Tr_ex: return T_Exp(e->u.ex);
        case Tr_cx: {
            /* test46.tig
             * Can't just like `return e->u.cx.stm;` 
             * For `CJUMP` must set `l_true` and `l_false` */
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return e->u.cx.stm;
        }
        case Tr_nx: return e->u.nx;
        default: assert(0);
    }
}

static struct Cx 
unCx(Tr_exp e) 
{ 
    switch (e->kind) {
        case Tr_ex: {
            T_stm stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
            patchList trues = PatchList(&stm->u.CJUMP.l_true, NULL);
            patchList falses = PatchList(&stm->u.CJUMP.l_false, NULL);
            return (struct Cx) {
                .trues = trues, 
                .falses = falses, 
                .stm = stm
            };
        }
        case Tr_cx: return e->u.cx;
        case Tr_nx:     /* Can't unCx a nx exp */
        default: assert(0);
    }
}

static Tr_exp
makeStaticLink(Tr_level from, Tr_level to) 
{
    T_exp fp = T_Temp(F_FP());

    Tr_level iter = from;
    while (to != iter) {
        fp = F_Exp(Tr_static_link(iter)->f_access, fp);
        iter = iter->parent;
    }

    return Tr_Ex(fp);
}

Tr_exp Tr_nop() 
{
    return Tr_Nx(T_Exp(T_Const(0)));
}

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level) 
{
    Tr_exp loc_fp = makeStaticLink(level, access->level);
    return Tr_Ex(F_Exp(access->f_access, unEx(loc_fp)));
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp idx) 
{
    return Tr_Ex(
            T_Mem(T_Binop(T_plus, 
                    unEx(base), 
                    T_Binop(T_mul, 
                        unEx(idx), 
                        T_Const(F_wordSize)))));
}

Tr_exp Tr_fieldVar(Tr_exp base, int nth) 
{
    return Tr_Ex(
            T_Mem(T_Binop(T_plus, 
                    unEx(base), 
                    T_Const(nth * F_wordSize))));
}

Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right) 
{
    T_binOp t_op;
    /* OPT: use r/lshift xor to optimize mul div */
    switch (op) {
        case A_plusOp: t_op = T_plus; break;
        case A_minusOp: t_op = T_minus; break;
        case A_timesOp: t_op = T_mul; break;
        case A_divideOp: t_op = T_div; break;
        default: assert(0);
    }

    return Tr_Ex(T_Binop(t_op, 
                unEx(left), 
                unEx(right)));
}

Tr_exp Tr_cmpExp(A_oper op, Tr_exp left, Tr_exp right) 
{
    T_relOp t_op;
    switch (op) {
        case A_ltOp: t_op = T_lt; break;
        case A_leOp: t_op = T_le; break;
        case A_gtOp: t_op = T_gt; break;
        case A_geOp: t_op = T_ge; break;
        case A_eqOp: t_op = T_eq; break;
        case A_neqOp: t_op = T_ne; break;
        default: assert(0);
    }
    T_stm stm = T_Cjump(t_op, unEx(left), unEx(right), NULL, NULL);
    patchList trues = PatchList(&(stm->u.CJUMP.l_true), NULL);
    patchList falses = PatchList(&(stm->u.CJUMP.l_false), NULL);

    return Tr_Cx(trues, falses, stm);
}

Tr_exp Tr_ifExp(Tr_exp test_exp, Tr_exp then_exp, Tr_exp else_exp) 
{
    struct Cx cx_test = unCx(test_exp);
    Temp_label t = Temp_newlabel(), f = Temp_newlabel(), done = Temp_newlabel();
    T_exp r = T_Temp(Temp_newtemp());

    if (else_exp == NULL) {
        doPatch(cx_test.trues, t);
        doPatch(cx_test.falses, done);
        return Tr_Nx(
                T_Seq(
                    cx_test.stm,
                    T_Seq(
                        T_Label(t),
                        T_Seq(
                            unNx(then_exp),
                            T_Label(done)))));
    }
    
    /* nx -- nx, no return value */
    if (then_exp->kind == Tr_nx && else_exp->kind == Tr_nx) {
        doPatch(cx_test.trues, t);
        doPatch(cx_test.falses, f);

        return Tr_Nx(
                T_Seq(
                    cx_test.stm,
                    T_Seq(
                        T_Label(t),
                        T_Seq(
                            unNx(then_exp),
                            T_Seq(
                                T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
                                T_Seq(
                                    T_Label(f),
                                    T_Seq(unNx(else_exp),
                                        T_Label(done))))))));
    }

    /* 
     * cx -- ex or ex -- cx
     * Bool exp. Like `if (x < 5 & a > b) then ... else ...` 
     * `x < 5 & a > b` => `if x < 5 then a > b else 0`
     * cx -- cx
     * `if x < 5 then a > b else a <= b` (Damn!)
     */
    Temp_label z = Temp_newlabel();
    struct Cx cx_temp;
    Tr_exp ex_cx;;
    if (then_exp->kind == Tr_cx || else_exp->kind == Tr_cx) {
        if (then_exp->kind == Tr_cx) {
            cx_temp = unCx(then_exp);
            ex_cx = else_exp;
            doPatch(cx_test.trues, z);
            doPatch(cx_test.falses, f);
        } else {
            cx_temp = unCx(else_exp);
            ex_cx = then_exp;
            doPatch(cx_test.trues, t);
            doPatch(cx_test.falses, z);
        }
        doPatch(cx_temp.trues, t);
        doPatch(cx_temp.falses, f);

        return Tr_Ex(
                T_Eseq(
                    cx_test.stm,
                    T_Eseq(
                        T_Label(z),
                        T_Eseq(
                            cx_temp.stm,
                            T_Eseq(
                                T_Label(t),
                                T_Eseq(
                                    T_Move(r, T_Const(1)),
                                    T_Eseq(
                                        T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
                                        T_Eseq(
                                            T_Label(f), 
                                            T_Eseq(
                                                T_Move(r, unEx(ex_cx)), 
                                                T_Eseq(
                                                    T_Label(done),
                                                    r))))))))));
    }

    /* ex -- ex, ex -- nx, ex -- cx, has return value 
     * like `if exp then func() else stm`  
     * func() has no return value, but is a `Tr_Ex`
     * so return type `Void` will cast to `Int`
     */
    if (then_exp->kind == Tr_ex || else_exp->kind == Tr_ex) {
        doPatch(cx_test.trues, t);
        doPatch(cx_test.falses, f);

        return Tr_Ex(
                T_Eseq(
                    cx_test.stm,
                    T_Eseq(
                        T_Label(t),
                        T_Eseq(
                            T_Move(r, unEx(then_exp)),
                            T_Eseq(
                                T_Jump(T_Name(done), Temp_LabelList(done, NULL)),
                                T_Eseq(
                                    T_Label(f),
                                    T_Eseq(
                                        T_Move(r, unEx(else_exp)),
                                        T_Eseq(
                                            T_Label(done), 
                                            r))))))));
    }
    /* other `kind` pair should be blocked by type check */
    assert(0);
}

/* 
 * TODO: `nil` is a constant pointer 
 */
static Temp_temp nil = NULL;
Tr_exp Tr_nilExp() 
{
    if (nil == NULL)
        nil = Temp_newtemp();

    return Tr_Ex(T_Temp(nil));
}

Tr_exp Tr_intExp(int n) 
{
    return Tr_Ex(T_Const(n));
}

Tr_exp Tr_stringExp(string str) 
{
    Temp_label l = Temp_newlabel();

    F_frag f_str = F_StringFrag(l, str);
    Tr_insert_str(f_str);

    return Tr_Ex(T_Name(l));
}

Tr_exp Tr_recordExp(Tr_expList el, int nvals) 
{
    Tr_expList iter;
    T_exp r = T_Temp(Temp_newtemp());

    T_stm init_stm = T_Move(r, 
                            F_externalCall("allocRecord", 
                                T_ExpList(T_Const(nvals * F_wordSize), NULL)));
    /* 
     * `init_stm` like `((creat_stm, ...), MOVE), MOVE)`,
     * will be linear later 
     */
    int i;
    for (iter = el, i = 0; iter; iter = iter->tail, i++) {
        init_stm = T_Seq(
                        init_stm, 
                        T_Move(
                            T_Mem(T_Binop(T_plus, r, T_Const(i * F_wordSize))), 
                            unEx(iter->head))); 
    }
    return Tr_Ex(T_Eseq(init_stm, r));
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) 
{
    T_exp r = T_Temp(Temp_newtemp());
    return Tr_Ex(
            T_Eseq(
                T_Move(
                    r, 
                    F_externalCall("initArray", 
                        T_ExpList(
                            unEx(size), 
                            T_ExpList(
                                unEx(init), 
                                NULL)))),
                r)); 
}

Tr_exp Tr_whileExp(Tr_exp test_exp, Tr_exp body_exp, Temp_label done) 
{
    Temp_label test = Temp_newlabel();
    Temp_label body = Temp_newlabel();
    struct Cx cx_test = unCx(test_exp);

    doPatch(cx_test.trues, body);
    doPatch(cx_test.falses, done);

    return Tr_Nx(
            T_Seq(
                T_Label(test), 
                T_Seq(
                    cx_test.stm, 
                    T_Seq(
                        T_Label(body), 
                        T_Seq(
                            unNx(body_exp),
                            T_Seq(
                                T_Jump(T_Name(test), Temp_LabelList(test, NULL)),
                                T_Label(done)))))));
}


Tr_exp Tr_forExp(Tr_exp lo_exp, Tr_exp hi_exp, Tr_exp body_exp, Temp_label done) 
{
    Temp_label test = Temp_newlabel();
    Temp_label body = Temp_newlabel();
    T_exp idx = T_Temp(Temp_newtemp());
    T_exp limit = T_Temp(Temp_newtemp());

    return Tr_Nx(
            T_Seq(
                T_Move(idx, unEx(lo_exp)),
                T_Seq(
                    T_Move(limit, unEx(hi_exp)),
                    T_Seq(
                        T_Label(test), 
                        T_Seq(
                            T_Cjump(T_le, idx, limit, body, done),
                            T_Seq(
                                T_Label(body),
                                T_Seq(
                                    unNx(body_exp),
                                    T_Seq(
                                        T_Move(idx, T_Binop(T_plus, idx, T_Const(1))),
                                        T_Seq(
                                            T_Jump(T_Name(test), Temp_LabelList(test, NULL)), 
                                            T_Label(done))))))))));

}

Tr_exp Tr_breakExp(Temp_label done) 
{
    return Tr_Nx(T_Jump(T_Name(done), Temp_LabelList(done, NULL)));
}

/* TODO: not sure */
Tr_exp Tr_callExp(Temp_label func_name, Tr_expList args) 
{
    T_expList t_args = NULL;
    Tr_expList iter;

    for (iter = args; iter; iter = iter->tail) {
        t_args = T_ExpList(unEx(iter->head), t_args);
    }

    /* `t_args` is reverse */
    return Tr_Ex(
            T_Call(
                T_Name(func_name), 
                T_ExpList(T_Temp(F_FP()), t_args)));
}


Tr_exp Tr_assignExp(Tr_exp var_exp, Tr_exp val_exp) 
{
    return Tr_Nx(
            T_Move(
                unEx(var_exp), 
                unEx(val_exp)));
}

Tr_exp Tr_seqExp(Tr_expList rev_exps) 
{
    if (rev_exps == NULL)
        return Tr_nop();

    Tr_expList iter;
    /* last exp is the `seq's` return val */
    T_exp seq_exps = unEx(rev_exps->head);

    for (iter = rev_exps->tail; iter; iter = iter->tail) {
        seq_exps = T_Eseq(unNx(iter->head), seq_exps);
    }

    return Tr_Ex(seq_exps);
}

void Tr_procEntryExit(Tr_level level, Tr_exp proc_body) 
{
    T_stm with_ret = T_Move(T_Temp(F_RV()), unEx(proc_body));
    T_stm body_stm = F_procEntryExit1(level->frame, with_ret);
    F_frag f_proc = F_ProcFrag(body_stm, level->frame);

    Tr_insert_proc(f_proc);
}

static void 
Tr_insert_proc(F_frag proc) 
{
    if (PROC_FRAG_TAIL != NULL) {
        PROC_FRAG_TAIL->tail = F_FragList(proc, NULL);
        PROC_FRAG_TAIL = PROC_FRAG_TAIL->tail;
    } else {
        PROC_FRAG_HEAD= F_FragList(proc, NULL);
        PROC_FRAG_TAIL = PROC_FRAG_HEAD;
    }
}

static void 
Tr_insert_str(F_frag str) 
{
    if (STR_FRAG_TAIL != NULL) {
        STR_FRAG_TAIL->tail = F_FragList(str, NULL);
        STR_FRAG_TAIL = STR_FRAG_TAIL->tail;
    } else {
        STR_FRAG_HEAD= F_FragList(str, NULL);
        STR_FRAG_TAIL = STR_FRAG_HEAD;
    }
}

F_fragList Tr_getResult(void) 
{
    PROC_FRAG_TAIL->tail = STR_FRAG_HEAD;
    return PROC_FRAG_HEAD;
}

/* debug function */
void Tr_printLevel(Tr_level level) 
{
    F_print(level->frame);
}

