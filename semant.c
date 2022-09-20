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

static struct expty
transCallExp(S_table venv, S_table tenv, A_exp call_exp) {
    /* 1. call return value
     * 2. args type check
     */
    return expTy(NULL, Ty_Void());
}

static struct expty
transRecodeExp(S_table venv, S_table tenv, A_exp record_exp) {
    /* 1. look up record type get `A_fieldList record` 
     * 2. check `A_field.name`  with `A_efield.name`
     * 3. check `A_field.typ` with `A_efield.exp`
     */
    return expTy(NULL, Ty_Void());
}

static struct expty
transArrayExp(S_table venv, S_table tenv, A_exp record_exp) {
    /* 1. look up array type 
     * 2. check `size` is `int` and `init` type is right
     */
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

static struct expty
transLetExp(S_table venv, S_table tenv, A_exp let_exp) {
    /* 1. enter dec to the venv and tenv
     * 2. trans body
     */
    return expTy(NULL, Ty_Void());
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

void transDec(S_table venv, S_table tenv, A_dec d) {

}

Ty_ty transTy(S_table tenv, A_ty a) {

}

void SEM_transProg(A_exp exp) {
    S_table venv = E_base_venv(), tenv = E_base_tenv();
    transExp(venv, tenv, exp);
}

