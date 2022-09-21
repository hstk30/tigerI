#include <stdio.h>
#include "semant.h"
#include "types.h"
#include "env.h"
#include "absyn.h"
#include "errormsg.h"

typedef void *Tr_exp;

struct expty {
    Tr_exp exp;
    Ty_ty ty;
};
struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty transTy(S_table tenv, A_ty a);

/*
 * skip all `Ty_name` type
 */ 
static Ty_ty
actual_ty(Ty_ty ty) {

}


static struct expty 
transOpExp(S_table venv, S_table tenv, A_exp opexp) {
    A_oper oper = opexp->u.op.oper;
    struct expty left_expty = transExp(venv, tenv, opexp->u.op.left);
    struct expty right_expty = transExp(venv, tenv, opexp->u.op.right);
    struct expty ret_expty = expTy(NULL, Ty_Int());
    switch (oper) {
        case A_plusOp: 
        case A_minusOp:
        case A_timesOp:
        case A_divideOp:
            /* TODO: divide zero error */
            if (left_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.left->pos, "Opexp clause integer required");
            if (right_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.right->pos, "Opexp clause integer required");
            return ret_expty;
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp:
            if (left_expty.ty->kind != Ty_int || right_expty.ty->kind != Ty_int) {
                EM_error(opexp->pos, "Opexp clause integer required");
            }
            return ret_expty;
        case A_eqOp:
        case A_neqOp: {
            /* TODO: eq, neq can apply to `record` `array` `string`, also `nil` */
            return ret_expty;
        }
        default: assert(0);
    }
}

/* 
 * 1. call return value
 * 2. args type check
 */
static struct expty
transCallExp(S_table venv, S_table tenv, A_exp call_exp) {
    return expTy(NULL, Ty_Void());
}

/* 
 * 1. look up record type get `A_fieldList record` 
 * 2. check `A_field.name`  with `A_efield.name`
 * 3. check `A_field.typ` with `A_efield.exp`
 */
static struct expty
transRecodeExp(S_table venv, S_table tenv, A_exp record_exp) {
    return expTy(NULL, Ty_Void());
}

/* 
 * 1. look up array type 
 * 2. check `size` is `int` and `init` type is right
 */
static struct expty
transArrayExp(S_table venv, S_table tenv, A_exp record_exp) {
    return expTy(NULL, Ty_Void());
}

static struct expty
transIfExp(S_table venv, S_table tenv, A_exp if_exp) {
    struct expty test_expty = transExp(venv, tenv, if_exp->u.iff.test);
    if (test_expty.ty->kind != Ty_int) {
        EM_error(if_exp->pos, "If clause test must be int");
        return expTy(NULL, Ty_Void());
    }
    struct expty then_expty = transExp(venv, tenv, if_exp->u.iff.then);
    if (if_exp->u.iff.elsee) {
        struct expty else_expty = transExp(venv, tenv, if_exp->u.iff.elsee);
        if (then_expty.ty->kind != else_expty.ty->kind) {
            EM_error(if_exp->pos, "IF THEN ELSE clause then else type dismatch");
            return expTy(NULL, Ty_Void());
        } else {
            return expTy(NULL, then_expty.ty);
        }
    } else {
        if (then_expty.ty->kind != Ty_void) {
            EM_error(if_exp->pos, "IF THEN clause then must return void");
        }
        return expTy(NULL, Ty_Void());
    }
}

static struct expty
transWhileExp(S_table venv, S_table tenv, A_exp while_exp) {
    struct expty test_expty = transExp(venv, tenv, while_exp->u.whilee.test);
    if (test_expty.ty->kind != Ty_int) {
        EM_error(while_exp->pos, "While clause test must be int");
        return expTy(NULL, Ty_Void());
    }
    struct expty body_expty = transExp(venv, tenv, while_exp->u.whilee.body);
    if (body_expty.ty->kind != Ty_void) {
        EM_error(while_exp->pos, "WHILE clause body must return void");
    }
    return expTy(NULL, Ty_Void());
}

static struct expty
transForExp(S_table venv, S_table tenv, A_exp for_exp) {
    struct expty lo_expty = transExp(venv, tenv, for_exp->u.forr.lo);
    struct expty hi_expty = transExp(venv, tenv, for_exp->u.forr.hi);
    struct expty body_expty;
    struct expty ret_expty = expTy(NULL, Ty_Void());
    
    if (lo_expty.ty->kind != Ty_int || hi_expty.ty->kind != Ty_int) {
        EM_error(for_exp->pos, "For clause lo, hi must be int");
    }

    S_beginScope(venv);
    S_enter(venv, for_exp->u.forr.var, E_VarEntry(Ty_Int()));
    body_expty = transExp(venv, tenv, for_exp->u.forr.body);
    S_endScope(venv);

    if (body_expty.ty->kind != Ty_void) {
        EM_error(for_exp->pos, "For clause body must return void");
    }
    return ret_expty;
}

/* 
 * 1. enter dec to the venv and tenv
 * 2. trans body
 */
static struct expty
transLetExp(S_table venv, S_table tenv, A_exp let_exp) {
    struct expty body_expty;
    A_decList d;

    S_beginScope(venv);
    S_beginScope(tenv);
    for (d = let_exp->u.let.decs; d; d = d->tail) {
        transDec(venv, tenv, d->head);
    }
    body_expty = transExp(venv, tenv, let_exp->u.let.body);
    S_endScope(tenv);
    S_endScope(venv);

    return body_expty;
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    switch (a->kind) {
        case A_varExp: return transVar(venv, tenv, a->u.var);
        case A_nilExp: return expTy(NULL, Ty_Nil());
        case A_intExp: return expTy(NULL, Ty_Int());
        case A_stringExp: return expTy(NULL, Ty_String());
        case A_callExp: return transCallExp(venv, tenv, a);
        case A_opExp: return transOpExp(venv, tenv, a);
        case A_recordExp: return transRecodeExp(venv, tenv, a);
        case A_arrayExp: return transArrayExp(venv, tenv, a);
        case A_seqExp: {
            A_expList el;
            for (el = a->u.seq; el; el = el->tail) {
                transExp(venv, tenv, el->head);
            }
            return expTy(NULL, Ty_Void());
        }
        case A_assignExp: {
            struct expty var_expty = transVar(venv, tenv, a->u.assign.var); 
            struct expty val_expty = transExp(venv, tenv, a->u.assign.exp); 
            if (var_expty.ty != val_expty.ty) {
                EM_error(a->pos, "Assign clause type dismatch");
            }
            return expTy(NULL, Ty_Void());
        }
        case A_ifExp: return transIfExp(venv, tenv, a);
        case A_whileExp: return transWhileExp(venv, tenv, a);
        case A_forExp: return transForExp(venv, tenv, a);
        case A_breakExp: return expTy(NULL, Ty_Void());  /* check used by `while` `for` */
        case A_letExp: return transLetExp(venv, tenv, a);
        default: assert(0);
    }
}

static void
transVarDec(S_table venv, S_table tenv, A_dec var_dec) {
    struct expty init_expty = transExp(venv, tenv, var_dec->u.var.init);
    return ;
}

/*
 * iter `params` 
 * `A_field.typ` -> `Ty_ty`
 */
static Ty_tyList
makeFormalTyList(S_table tenv, A_fieldList params) {
    if (params == NULL) {
        return NULL;
    }
    Ty_ty param_ty = S_look(tenv, params->head->typ);
    if (param_ty == NULL) {
        EM_error(params->head->pos, "Type %s use before declare", params->head->typ);
    }
    return Ty_TyList(param_ty, makeFormalTyList(tenv, params->tail));
}

/* 
 * test6.tig test7.tig
 * iter func_dec->u.function
 * `A_fundec->params` -> `E_enventry->u.fun.formals`
 * `A_fundec->result` -> `E_enventry->u.fun.results`
 * enter param to function scope
 * transExp(body)
 * TODO: recursion function dec
 */
static void
transFuncDec(S_table venv, S_table tenv, A_dec func_dec) {
    A_fundecList fl;
    A_fundec f;
    Ty_ty result_ty = NULL;
    Ty_tyList formal_tys = NULL;
    struct expty body_expty;

    for (fl = func_dec->u.function; fl; fl = fl->tail) {
        f = fl->head;
        formal_tys = makeFormalTyList(tenv, f->params);
        if (f->result) {
            result_ty = S_look(tenv, f->result);
        }
        S_enter(venv, f->name, E_FunEntry(formal_tys, result_ty));

        S_beginScope(venv);
        if (f->params && formal_tys) {
            A_fieldList l;
            Ty_tyList t;
            for (l = f->params, t = formal_tys; l; l = l->tail, t = t->tail) {
                S_enter(venv, l->head->name, E_VarEntry(t->head));
            }
        }
        body_expty = transExp(venv, tenv, f->body);
        if (f->result && body_expty.ty->kind != result_ty->kind) {
            EM_error(func_dec->pos, "Function declared return type dimatch with body");
        }
        S_endScope(venv);
    }

    return ;
}

/* 
 * test5.tig
 * iter `A_dec.u.type` twice to handle recursion
 */
static void
transTyDec(S_table venv, S_table tenv, A_dec ty_dec) {
    A_nametyList tl;
    Ty_ty temp_ty;

    for (tl = ty_dec->u.type; tl; tl = ty_dec->u.type->tail) {
        temp_ty = transTy(tenv, tl->head->ty);
        S_enter(tenv, tl->head->name, temp_ty);
    }
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case A_varDec: return transVarDec(venv, tenv, d);
        case A_functionDec: return transFuncDec(venv, tenv, d);
        case A_typeDec: return transTyDec(venv, tenv, d);
        default: assert(0);
    }
}

Ty_ty transTy(S_table tenv, A_ty t) {
    switch (t->kind) {
        case A_nameTy: {
            Ty_ty name_ty = S_look(tenv, t->u.name);
            if (name_ty == NULL) {
                return Ty_Name(t->u.name, NULL);
            } else {
                return name_ty;
            }
        }
        case A_recordTy: {

        }
        case A_arrayTy:
        default: assert(0);
    }
}

void SEM_transProg(A_exp exp) {
    S_table venv = E_base_venv(), tenv = E_base_tenv();
    transExp(venv, tenv, exp);
}

