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
 * test16.tig
 * skip all `Ty_name` type
 */ 
static Ty_ty
actual_ty(Ty_ty ty) {
    Ty_ty actual = ty;
    while (actual->kind != Ty_name) {
        actual = actual->u.name.ty;
    }
    return actual;
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
        case A_divideOp: {
            /* TODO: divide zero error */
            if (left_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.left->pos, "Opexp `+ - * /` clause integer required");
            if (right_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.right->pos, "Opexp `+ - * /` clause integer required");
            return ret_expty;
         }
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp: {
            if (left_expty.ty->kind != Ty_int || right_expty.ty->kind != Ty_int) {
                EM_error(opexp->pos, "Opexp `< <= > >=`clause integer required");
            }
            return ret_expty;
        }
        case A_eqOp:
        case A_neqOp: {
            /* TODO: eq, neq can apply to `record` `array` `string`, also `nil` 
             * Does `void` need special handle? */
            if ((left_expty.ty->kind == Ty_nil && right_expty.ty->kind == Ty_record) ||
                    (left_expty.ty->kind == Ty_record && right_expty.ty->kind== Ty_nil)) {
                return ret_expty;
            }
            if (left_expty.ty->kind != right_expty.ty->kind) {
                EM_error(opexp->pos, "Opexp `<> =` clause type dismatch");
            }
            return ret_expty;
        }
        default: assert(0);
    }
}

static void
checkArgs(S_table venv, S_table tenv,  A_expList args, Ty_tyList formals, int pos) {
    if ((args == NULL && formals != NULL) ||
            (args != NULL && formals == NULL)) {
        EM_error(pos, "Function call args inconsistent with declare");
        return ;
    }
    if (args == NULL && formals == NULL) {
        return ;
    }

    struct expty arg_expty = transExp(venv, tenv, args->head);
    if (arg_expty.ty->kind != formals->head->kind) {
        EM_error(args->head->pos, "Function call args type dismatch");
    }
    return checkArgs(venv, tenv, args->tail, formals->tail, pos);
}

/* 
 * 1. args type check
 * 2. call return value
 */
static struct expty
transCallExp(S_table venv, S_table tenv, A_exp call_exp) {
    E_enventry func = S_look(venv, call_exp->u.call.func);

    if (func == NULL) {
        EM_error(call_exp->pos, "Function %s use before declare", S_name(call_exp->u.call.func));
        return expTy(NULL, Ty_Void());
    }
    checkArgs(venv, tenv, call_exp->u.call.args, func->u.fun.formals, call_exp->pos);

    if (func->u.fun.results == NULL) {
        return expTy(NULL, Ty_Void());
    } else {
        return expTy(NULL, actual_ty(func->u.fun.results));
    }
}

/* 
 * 1. look up record type get `A_fieldList record` 
 * 2. check `A_field.name`  with `A_efield.name`
 * 3. check `A_field.typ` with `A_efield.exp`
 */
static void 
checkRecordExp(S_table venv, S_table tenv, A_efieldList kv, Ty_fieldList kt, int pos) {
    if ((kv == NULL && kt != NULL) ||
            (kv != NULL && kt == NULL)) {
        EM_error(pos, "Record create inconsistent with declare");
        return ;
    }
    if (kv == NULL && kt == NULL) {
        return ;
    }

    if (kv->head->name != kt->head->name) {
        EM_error(pos, "Record key %s inconsistent with declare %s", S_name(kv->head->name), S_name(kt->head->name));
    }

    struct expty val_expty = transExp(venv, tenv, kv->head->exp);
    if (val_expty.ty->kind != actual_ty(kt->head->ty)->kind) {

    }


}

static struct expty
transRecodeExp(S_table venv, S_table tenv, A_exp record_exp) {
    Ty_ty ty_env = S_look(tenv, record_exp->u.record.typ);
    A_efieldList 

    if (ty_env == NULL || actual_ty(ty_env)->kind != Ty_record) {
        EM_error(record_exp->pos, "Record type %s use before declare", S_name(record_exp->u.record.typ));
        return expTy(NULL, Ty_Nil());
    }





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
            EM_error(if_exp->pos, "IF THEN ELSE clause THEN ELSE type dismatch");
            return expTy(NULL, Ty_Void());
        } else {
            return expTy(NULL, then_expty.ty);
        }
    } else {
        if (then_expty.ty->kind != Ty_void) {
            EM_error(if_exp->pos, "IF THEN clause THEN must return void");
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


static struct expty
transSimpleVar(S_table venv, S_table tenv, A_var simple_var) {
    E_enventry x = S_look(venv, simple_var->u.simple);

    if (x && x->kind == E_varEntry) {
        return expTy(NULL, actual_ty(x->u.var.ty));
    } else {
        EM_error(simple_var->pos, "Variable %s use before declare", S_name(simple_var->u.simple));
        /* default int ? */
        return expTy(NULL, Ty_Int());
    }
}

/*
 * var type has the `sym` field?
 */
static struct expty
transFieldVar(S_table venv, S_table tenv, A_var field_var) {
    struct expty record_expty = transVar(venv, tenv, field_var->u.field.var);

}

/*
 * index `exp` must be `int`
 */
static struct expty
transSubscriptVar(S_table venv, S_table tenv, A_var subscript_var) {
    struct expty array_expty = transVar(venv, tenv, subscript_var->u.subscript.var);
    if (array_expty.ty->kind != Ty_array) {
        EM_error(subscript_var->pos, "Array subscript array type required");
        return expTy(NULL, Ty_Int());
    }

    struct expty idx_expty = transExp(venv, tenv, subscript_var->u.subscript.exp);
    if (idx_expty.ty->kind != Ty_int) {
        EM_error(subscript_var->pos, "Array index int type required");
        return expTy(NULL, Ty_Int());
    }
    return expTy(NULL, array_expty.ty->u.array);
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: return transSimpleVar(venv, tenv, v);
        case A_fieldVar: return transFieldVar(venv, tenv, v);
        case A_subscriptVar: return transSubscriptVar(venv, tenv, v);
        default: assert(0);
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
            struct expty last_expty;
            for (el = a->u.seq; el; el = el->tail) {
                last_expty = transExp(venv, tenv, el->head);
            }
            return expTy(NULL, last_expty.ty);
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
 * `A_fieldList` -> `Ty_tyList`
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
    A_fundec f;
    A_fundecList fl;
    A_fieldList params = NULL;
    Ty_tyList formal_tys = NULL;
    Ty_ty result_ty = NULL;
    E_enventry func_entry = NULL;
    struct expty body_expty;

    /* first time collect function head info */
    for (fl = func_dec->u.function; fl; fl = fl->tail) {
        f = fl->head;
        formal_tys = makeFormalTyList(tenv, f->params);
        if (f->result) {
            result_ty = S_look(tenv, f->result);
            if (result_ty == NULL) {
                EM_error(f->pos, "Function return type %s use before declare", S_name(f->result));
            }
        }
        S_enter(venv, f->name, E_FunEntry(formal_tys, result_ty));
    }

    /* second time handle function body */
    for (fl = func_dec->u.function; fl; fl = fl->tail) {
        f = fl->head;
        func_entry = S_look(venv, f->name);
        S_beginScope(venv);
        params = f->params;
        formal_tys= func_entry->u.fun.formals;
        if (params && formal_tys) {
            for (; params; params = params->tail, formal_tys = formal_tys->tail) {
                S_enter(venv, params->head->name, E_VarEntry(formal_tys->head));
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
    A_nametyList iter;
    Ty_ty t1, t2;

    /* first time just add name to `tenv` */
    for (iter = ty_dec->u.type; iter; iter = ty_dec->u.type->tail) {
        t1 = S_look(tenv, iter->head->name);
        if (t1 && t1->kind != Ty_name) {
            /* In fact, temp_ty just will be int string or nil */
            S_enter(tenv, iter->head->name, Ty_Name(iter->head->name, t1));
        } else {
            S_enter(tenv, iter->head->name, Ty_Name(iter->head->name, NULL));
        }
    }

    /* second time set right type to `Ty_name` type */
    for (iter = ty_dec->u.type; iter; iter = ty_dec->u.type->tail) {
        t1 = S_look(tenv, iter->head->name);
        if (t1 && t1->kind == Ty_name && t1->u.name.ty == NULL) {
            t2 = transTy(tenv, iter->head->ty);
            t1->u.name.ty = t2;
        }
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

static Ty_ty
transNameTy(S_table tenv, A_ty name_ty) {
    Ty_ty t = S_look(tenv, name_ty->u.name);
    if (t) {
        if (t->kind != Ty_name || t->u.name.ty) {
            return Ty_Name(name_ty->u.name, t);
        } else {
            /* test16.tig */
            EM_error(name_ty->pos, "Type %s circle declare", S_name(name_ty->u.name));
            return Ty_Name(name_ty->u.name, Ty_Int());
        }
    } else {
        EM_error(name_ty->pos, "Type %s use before declare", S_name(name_ty->u.name));
        return Ty_Name(name_ty->u.name, Ty_Int());
    }
}

/* 
 * `A_fieldList` -> `Ty_fieldList`
 */
static Ty_fieldList 
makeTyFieldList(S_table tenv, A_fieldList field_list) {
    if (field_list == NULL)
        return NULL;
    Ty_ty field_ty = S_look(tenv, field_list->head->typ);
    if (field_ty == NULL) {
        EM_error(field_list->head->pos, "Type %s use before declare", S_name(field_list->head->typ));
        field_ty = Ty_Int();
    } 
    return Ty_FieldList(
        Ty_Field(field_list->head->name, field_ty), 
        makeTyFieldList(tenv, field_list->tail)
    );
}

/*
 * NOTE: type-id {} -> Ty_Record(NULL)
 */
static Ty_ty
transRecordTy(S_table tenv, A_ty record_ty) {
    Ty_fieldList t = makeTyFieldList(tenv, record_ty->u.record);
    return Ty_Record(t);
}

static Ty_ty
transArrayTy(S_table tenv, A_ty array_ty) {
    Ty_ty t = S_look(tenv, array_ty->u.array);
    if (t == NULL) {
        EM_error(array_ty->pos, "Type %s use before declare", S_name(array_ty->u.array));
        /* TODO: return what when it happen? */
        return Ty_Array(Ty_Int());
    }
    return Ty_Array(t);
}

Ty_ty transTy(S_table tenv, A_ty t) {
    switch (t->kind) {
        case A_nameTy: return transNameTy(tenv, t);
        case A_recordTy: return transRecordTy(tenv, t);
        case A_arrayTy: return transArrayTy(tenv, t);
        default: assert(0);
    }
}

void SEM_transProg(A_exp exp) {
    S_table venv = E_base_venv(), tenv = E_base_tenv();
    transExp(venv, tenv, exp);
}

