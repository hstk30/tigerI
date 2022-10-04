/*
 * Cause this file have multi-chatper code,
 * so layout by chatper, not coding rule.
 * TODO: when finish this project, then rearranage it.
 */
#include <stdio.h>

#include "translate.h"
#include "frame.h"


/*** stack frame or activation record rel ***/
static Tr_level OUTERMOST_LEVEL = NULL;

struct Tr_level_ {
    Tr_level parent;
    Tr_accessList s_formals;  /* static link + formals */
    F_frame frame;
};

struct Tr_access_ {
    Tr_level level;
    F_access f_access;
};

static Tr_access Tr_Access(Tr_level level, F_access f_access) {
    Tr_access p = checked_malloc(sizeof(*p));

    p->level = level;
    p->f_access = f_access;

    return p;
}

Tr_accessList 
Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList p = checked_malloc(sizeof(*p));

    p->head = head;
    p->tail = tail;

    return p;
}

static Tr_accessList 
makeSformals(Tr_level level, F_accessList formals) {
    if (formals == NULL) {
        return NULL;
    }

    return Tr_AccessList(
            Tr_Access(level, formals->head),
            makeSformals(level, formals->tail));
}

Tr_level Tr_outermost(void) {
    if (OUTERMOST_LEVEL == NULL) {
        Tr_level level = Tr_newLevel(NULL, Temp_namedlabel("tiger_global"), NULL);
        /* let no `NULL` */
        level->parent = level;
        OUTERMOST_LEVEL = level;
    } 

    return OUTERMOST_LEVEL;
}

Tr_level 
Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level level = checked_malloc(sizeof(*level));

    U_boolList static_link_formals = U_BoolList(TRUE, formals);
    level->parent = parent;
    level->frame = F_newFrame(name, static_link_formals);
    level->s_formals = makeSformals(level, F_formals(level->frame));

    return level;
}

Tr_access Tr_static_link(Tr_level level) {
    return level->s_formals->head;
}

Tr_accessList
Tr_formals(Tr_level level) {
    return level->s_formals->tail;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    F_access fa = F_allocLocal(level->frame, escape); 

    return Tr_Access(level, fa);
}

/* debug function */
void Tr_print(Tr_level level) {
    F_print(level->frame);
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

static void 
doPatch(patchList t, Temp_label label) {
    for (; t; t = t->tail) {
        *(t->head) = label;
    }
    return ;
}

static patchList 
joinPatch(patchList first, patchList second) {
    if (!first) {
        return second;
    }

    for (; first->tail; first = first->tail) 
        ;
    first->tail = second;

    return first;
}

static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_ex;
    p->u.ex = ex;

    return p;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_nx;
    p->u.nx = nx;

    return p;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
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
unEx(Tr_exp e) {
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
unNx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex: return T_Exp(e->u.ex);
        case Tr_cx: return e->u.cx.stm; /* TODO */
        case Tr_nx: return e->u.nx;
        default: assert(0);
    }
}

static struct Cx 
unCx(Tr_exp e) { 
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
        case Tr_nx:
        default: assert(0);
    }
}

static Tr_exp
makeStaticLink(Tr_level from, Tr_level to) {
    T_exp fp = T_Temp(F_FP());

    Tr_level iter = from;
    while (to != iter) {
        fp = F_Exp(Tr_static_link(iter)->f_access, fp);
        iter = iter->parent;
    }

    return Tr_Ex(fp);
}

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level) {
    Tr_exp loc_fp = makeStaticLink(level, access->level);
    return Tr_Ex(F_Exp(access->f_access, unEx(loc_fp)));
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp idx) {
    return Tr_Ex(
            T_Mem(T_Binop(T_plus, 
                    unEx(base), 
                    T_Binop(T_mul, 
                        unEx(idx), 
                        T_Const(F_wordSize)))));
}

Tr_exp Tr_fieldVar(Tr_exp base, int nth) {
    return Tr_Ex(
            T_Mem(T_Binop(T_plus, 
                    unEx(base), 
                    T_Const(nth * F_wordSize))));
}

Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right) {
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

Tr_exp Tr_cmpExp(A_oper op, Tr_exp left, Tr_exp right) {
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
    patchList trues = PatchList(&stm->u.CJUMP.l_true, NULL);
    patchList falses = PatchList(&stm->u.CJUMP.l_false, NULL);

    return Tr_Cx(trues, falses, stm);
}

Tr_exp Tr_ifExp(Tr_exp if_exp, Tr_exp then_exp, Tr_exp else_exp) {

}

