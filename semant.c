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

/*
 * return actual type which without `Ty_Name`
 * NOTE: returned by `S_look` from `tenv` need process by `actual_ty()`
 */ 
struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty transTy(S_table tenv, A_ty a);

/*
 * test16.tig
 * skip all `Ty_name` type
 * cause `type a = b` is the same type, so just compare type reference
 */ 
static Ty_ty
actual_ty(Ty_ty ty) {
    Ty_ty actual = ty;
    while (actual->kind == Ty_name) {
        actual = actual->u.name.ty;
    }
    return actual;
}

static bool
type_equal(Ty_ty t1, Ty_ty t2) {
    Ty_ty a1 = actual_ty(t1);
    Ty_ty a2 = actual_ty(t2);

    if ((a1->kind == Ty_record && a2->kind == Ty_nil) ||
            (a1->kind == Ty_nil && a2->kind == Ty_record)) {
        return TRUE;
    }
    return (a1 == a2);
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
                EM_error(opexp->u.op.left->pos, "Opexp `+ - * /` clause <type: int> required");
            if (right_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.right->pos, "Opexp `+ - * /` clause integer required");
            return ret_expty;
         }
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp: {
            if (left_expty.ty->kind != Ty_int || right_expty.ty->kind != Ty_int) {
                EM_error(opexp->pos, "Opexp `< <= > >=`clause <type: int> required");
            }
            return ret_expty;
        }
        case A_eqOp:
        case A_neqOp: {
             /* Does `void` need special handle? */
            if (!type_equal(left_expty.ty, right_expty.ty)) {
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
    if (!type_equal(arg_expty.ty, formals->head)) {
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
        EM_error(pos, "Record create length inconsistent with declare");
        return ;
    }
    if (kv == NULL && kt == NULL) {
        return ;
    }

    /* TODO: test_record_exp.tig */
    if (kv->head->name != kt->head->name) {
        EM_error(pos, "Record key name %s inconsistent with declare %s", 
            S_name(kv->head->name), S_name(kt->head->name));
    }

    struct expty val_expty = transExp(venv, tenv, kv->head->exp);
    if (!type_equal(val_expty.ty, kt->head->ty)) {
        EM_error(kv->head->exp->pos, "Record value type inconsistent with declare");
    }
    return checkRecordExp(venv, tenv, kv->tail, kt->tail, pos);
}

static struct expty
transRecodeExp(S_table venv, S_table tenv, A_exp record_exp) {
    Ty_ty deced_ty = S_look(tenv, record_exp->u.record.typ), record_ty;

    if (deced_ty == NULL) {
        EM_error(record_exp->pos, "Record type %s use before declare", S_name(record_exp->u.record.typ));
        return expTy(NULL, Ty_Nil());
    }
    record_ty = actual_ty(deced_ty);
    if (record_ty->kind != Ty_record) {
        EM_error(record_exp->pos, "Expect record type before `{...}`");
        return expTy(NULL, Ty_Nil());
    }
    checkRecordExp(venv, tenv, record_exp->u.record.fields, record_ty->u.record, record_exp->pos);

    return expTy(NULL, actual_ty(record_ty));
}

/* 
 * 1. look up array type 
 * 2. check `size` is `int` and `init` type is right
 */
static struct expty
transArrayExp(S_table venv, S_table tenv, A_exp array_exp) {
    Ty_ty deced_ty = S_look(tenv, array_exp->u.array.typ), array_ty;

    if (deced_ty == NULL) {
        EM_error(array_exp->pos, "Array type %s use before declare", S_name(array_exp->u.array.typ));
        return expTy(NULL, Ty_Array(Ty_Int()));
    }

    array_ty = actual_ty(deced_ty);
    if (array_ty->kind != Ty_array) {
        EM_error(array_exp->pos, "Expect array type before `[exp] of exp`");
        return expTy(NULL, Ty_Array(Ty_Int()));
    }

    struct expty size_expty = transExp(venv, tenv, array_exp->u.array.size);
    if (size_expty.ty->kind != Ty_int) {
        EM_error(array_exp->pos, "Array size clause must be int");
        return expTy(NULL, Ty_Array(Ty_Int()));
    }

    struct expty init_expty = transExp(venv, tenv, array_exp->u.array.init);
    /* NOTE: array items type compare `array_ty->u.array` instead of `array_ty` */
    if (!type_equal(init_expty.ty, array_ty->u.array)) {
        EM_error(array_exp->pos, "Array init clause type dismatch");
        return expTy(NULL, Ty_Array(Ty_Int()));
    }

    return expTy(NULL, actual_ty(array_ty));
}

static struct expty
transAssignExp(S_table venv, S_table tenv, A_exp assign_exp) {
    struct expty var_expty = transVar(venv, tenv, assign_exp->u.assign.var); 
    struct expty val_expty = transExp(venv, tenv, assign_exp->u.assign.exp); 

    /* var a : my_record := nil */
    /* consider: 
     * a := my_record{a=1, b=2} 
     * not compare `kind` but `ty`
     */
    if (!type_equal(var_expty.ty, val_expty.ty)) {
        EM_error(assign_exp->pos, "Assign clause type dismatch");
    }

    return expTy(NULL, Ty_Void());
}

static struct expty
transIfExp(S_table venv, S_table tenv, A_exp if_exp) {
    struct expty test_expty = transExp(venv, tenv, if_exp->u.iff.test);
    if (test_expty.ty->kind != Ty_int) {
        EM_error(if_exp->pos, "If test clause must be int");
        return expTy(NULL, Ty_Void());
    }
    struct expty then_expty = transExp(venv, tenv, if_exp->u.iff.then);
    if (if_exp->u.iff.elsee) {
        struct expty else_expty = transExp(venv, tenv, if_exp->u.iff.elsee);
        if (!type_equal(then_expty.ty, else_expty.ty)) {
            EM_error(if_exp->pos, "IF THEN ELSE clause THEN ELSE type dismatch");
            return expTy(NULL, Ty_Void());
        } else {
            return expTy(NULL, actual_ty(then_expty.ty));
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
        /* Not return, just go through */
        EM_error(while_exp->pos, "While test clause must be int");
    }
    struct expty body_expty = transExp(venv, tenv, while_exp->u.whilee.body);
    if (body_expty.ty->kind != Ty_void) {
        EM_error(while_exp->pos, "WHILE body clause must be void");
    }
    return expTy(NULL, Ty_Void());
}

static struct expty
transForExp(S_table venv, S_table tenv, A_exp for_exp) {
    struct expty lo_expty = transExp(venv, tenv, for_exp->u.forr.lo);
    struct expty hi_expty = transExp(venv, tenv, for_exp->u.forr.hi);
    struct expty ret_expty = expTy(NULL, Ty_Void());
    
    if (lo_expty.ty->kind != Ty_int || hi_expty.ty->kind != Ty_int) {
        /* Not return, just go through */
        EM_error(for_exp->pos, "For lo, hi clause must be int");
    }

    S_beginScope(venv);
    S_enter(venv, for_exp->u.forr.var, E_VarEntry(Ty_Int()));
    /* TODO: for iter var should not be assign 
     * So, maybe add an keyword `const`, and an attribute `const` to `A_var`
     */ 
    struct expty body_expty = transExp(venv, tenv, for_exp->u.forr.body);
    S_endScope(venv);

    if (body_expty.ty->kind != Ty_void) {
        EM_error(for_exp->pos, "For body clause must return void");
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

    if (record_expty.ty->kind != Ty_record) {
        EM_error(field_var->pos, "Record field operate <type: record> required");
        return expTy(NULL, Ty_Int());
    }

    Ty_fieldList iter;
    for (iter = record_expty.ty->u.record; iter; iter = iter->tail) {
        if (iter->head->name == field_var->u.field.sym) {
            return expTy(NULL, actual_ty(iter->head->ty));
        }
    }

    EM_error(field_var->pos, "Record not have the key: %s", S_name(field_var->u.field.sym));
    return expTy(NULL, Ty_Int());
}

/*
 * index `exp` must be `int`
 */
static struct expty
transSubscriptVar(S_table venv, S_table tenv, A_var subscript_var) {
    struct expty array_expty = transVar(venv, tenv, subscript_var->u.subscript.var);

    if (array_expty.ty->kind != Ty_array) {
        EM_error(subscript_var->pos, "Array subscript operate <type: array> required");
        return expTy(NULL, Ty_Int());
    }

    struct expty idx_expty = transExp(venv, tenv, subscript_var->u.subscript.exp);
    if (idx_expty.ty->kind != Ty_int) {
        EM_error(subscript_var->pos, "Array index <type: int> required");
        return expTy(NULL, Ty_Int());
    }
    return expTy(NULL, actual_ty(array_expty.ty->u.array));
}

static void
transVarDec(S_table venv, S_table tenv, A_dec var_dec) {
    Ty_ty deced_ty;
    struct expty init_expty = transExp(venv, tenv, var_dec->u.var.init);

    if (var_dec->u.var.typ) {
        deced_ty = S_look(tenv, var_dec->u.var.typ);
        if (deced_ty == NULL) {
            EM_error(var_dec->pos, "Type %s use before declare", S_name(var_dec->u.var.typ));
        }
        if (!type_equal(deced_ty, init_expty.ty)) {
            EM_error(var_dec->pos, "Variable init type dismatch");
        }
    } 

    S_enter(venv, var_dec->u.var.var, E_VarEntry(init_expty.ty));

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
        EM_error(params->head->pos, "Type %s use before declare", S_name(params->head->typ));
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
        } else {
            result_ty = Ty_Void();
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
        if (!type_equal(body_expty.ty, func_entry->u.fun.results)) {
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
    for (iter = ty_dec->u.type; iter; iter = iter->tail) {
        t1 = S_look(tenv, iter->head->name);
        if (t1 && t1->kind != Ty_name) {
            /* In fact, temp_ty just will be int string or nil */
            S_enter(tenv, iter->head->name, Ty_Name(iter->head->name, t1));
        } else {
            S_enter(tenv, iter->head->name, Ty_Name(iter->head->name, NULL));
        }
    }

    /* second time set right type to `Ty_name` type */
    for (iter = ty_dec->u.type; iter; iter = iter->tail) {
        t1 = S_look(tenv, iter->head->name);
        if (t1 && t1->kind == Ty_name && t1->u.name.ty == NULL) {
            t2 = transTy(tenv, iter->head->ty);
            t1->u.name.ty = t2;
        }
    }
}

static Ty_ty
transNameTy(S_table tenv, A_ty name_ty) {
    Ty_ty t = S_look(tenv, name_ty->u.name);
    if (t) {
        if (t->kind != Ty_name || t->u.name.ty) {
            /* type my_record = {a: int, b: int}
             * type my_record2 = my_record
             * type_equal(my_record, my_record2) == 1
             */
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

/*
 * `()` `(a <> nul)` `(s1; s2; s3)` all be `seqexp`
 */
static struct expty
transSeqExp(S_table venv, S_table tenv, A_exp seq_exp) {
    A_expList el;
    struct expty last_expty;
    
    if (seq_exp->u.seq == NULL) {
        return expTy(NULL, Ty_Void());
    } else {
        for (el = seq_exp->u.seq; el; el = el->tail) {
            last_expty = transExp(venv, tenv, el->head);
        }
        return expTy(NULL, actual_ty(last_expty.ty));
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
        case A_seqExp: return transSeqExp(venv, tenv, a);
        case A_assignExp: return transAssignExp(venv, tenv, a);
        case A_ifExp: return transIfExp(venv, tenv, a);
        case A_whileExp: return transWhileExp(venv, tenv, a);
        case A_forExp: return transForExp(venv, tenv, a);
        case A_breakExp: return expTy(NULL, Ty_Void());  /* check used by `while` `for` */
        case A_letExp: return transLetExp(venv, tenv, a);
        default: assert(0);
    }
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: return transSimpleVar(venv, tenv, v);
        case A_fieldVar: return transFieldVar(venv, tenv, v);
        case A_subscriptVar: return transSubscriptVar(venv, tenv, v);
        default: assert(0);
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

