#include <stdio.h>
#include <stdlib.h>

#include "semant.h"
#include "types.h"
#include "env.h"
#include "absyn.h"
#include "errormsg.h"
#include "escape.h"
#include "translate.h"


struct LoopLabel {
    bool is_nested;
    Temp_labelList done_labels; 
};
static struct LoopLabel LOOP_LABELS = {FALSE, NULL};

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
struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v);
struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a);
void transDec(Tr_level level, S_table venv, S_table tenv, A_dec d);
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
transOpExp(Tr_level level, S_table venv, S_table tenv, A_exp opexp) {
    A_oper oper = opexp->u.op.oper;
    struct expty left_expty = transExp(level, venv, tenv, opexp->u.op.left);
    struct expty right_expty = transExp(level, venv, tenv, opexp->u.op.right);

    switch (oper) {
        case A_plusOp: 
        case A_minusOp:
        case A_timesOp:
        case A_divideOp: {
            /* TODO: divide zero error */
            if (left_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.left->pos, 
                        "Opexp `+ - * /` clause <type: int> required");
            if (right_expty.ty->kind != Ty_int) 
                EM_error(opexp->u.op.right->pos, 
                        "Opexp `+ - * /` clause integer required");
            return expTy(
                    Tr_arithExp(oper, left_expty.exp, right_expty.exp), 
                    Ty_Int());
         }
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp: {
            if (left_expty.ty->kind != Ty_int || right_expty.ty->kind != Ty_int) {
                EM_error(opexp->pos, "Opexp `< <= > >=`clause <type: int> required");
            }
            return expTy(
                    Tr_cmpExp(oper, left_expty.exp, right_expty.exp), 
                    Ty_Int());
        }
        case A_eqOp:
        case A_neqOp: {
             /* Does `void` need special handle? */
            if (!type_equal(left_expty.ty, right_expty.ty)) {
                EM_error(opexp->pos, "Opexp `<> =` clause type dismatch");
            }
            return expTy(
                    Tr_cmpExp(oper, left_expty.exp, right_expty.exp), 
                    Ty_Int());
        }
        default: assert(0);
    }
}

static Tr_expList
makeArgs(Tr_level level, S_table venv, S_table tenv,  
        A_expList args, Ty_tyList formals, int pos) {
    if ((args == NULL && formals != NULL) ||
            (args != NULL && formals == NULL)) {
        EM_error(pos, "Function call args inconsistent with declare");
        exit(1);
    }
    if (args == NULL && formals == NULL) {
        return NULL;
    }

    struct expty arg_expty = transExp(level, venv, tenv, args->head);
    if (!type_equal(arg_expty.ty, formals->head)) {
        EM_error(args->head->pos, "Function call args type dismatch");
        exit(1);
    }
    return Tr_ExpList(
            arg_expty.exp,
            makeArgs(level, venv, tenv, args->tail, formals->tail, pos));
}

/* 
 * 1. args type check
 * 2. call return value
 */
static struct expty
transCallExp(Tr_level level, S_table venv, S_table tenv, A_exp call_exp) {
    E_enventry func = S_look(venv, call_exp->u.call.func);

    if (func == NULL) {
        EM_error(call_exp->pos, "Function %s use before declare", 
                S_name(call_exp->u.call.func));
        return expTy(NULL, Ty_Void());
    }
    Tr_expList args = makeArgs(level, venv, tenv, 
            call_exp->u.call.args, func->u.fun.formals, call_exp->pos);

    if (func->u.fun.results == NULL) {
        return expTy(
                Tr_callExp(), 
                Ty_Void());
    } else {
        return expTy(
                Tr_callExp(), 
                actual_ty(func->u.fun.results));
    }
}

/* 
 * 1. look up record type get `A_fieldList record` 
 * 2. check `A_field.name`  with `A_efield.name`
 * 3. check `A_field.typ` with `A_efield.exp`
 */
static Tr_expList 
makeRecordVals(Tr_level level, S_table venv, S_table tenv, 
        A_efieldList kv, Ty_fieldList kt, int pos) {
    if ((kv == NULL && kt != NULL) ||
            (kv != NULL && kt == NULL)) {
        EM_error(pos, "Record create length inconsistent with declare");
        exit(1);
    }
    if (kv == NULL && kt == NULL) {
        return NULL;
    }

    if (kv->head->name != kt->head->name) {
        EM_error(pos, "Record key name %s inconsistent with declare %s", 
            S_name(kv->head->name), S_name(kt->head->name));
        exit(1);
    }

    struct expty val_expty = transExp(level, venv, tenv, kv->head->exp);
    if (!type_equal(val_expty.ty, kt->head->ty)) {
        EM_error(kv->head->exp->pos, 
                "Record value type inconsistent with declare");
        exit(1);
    }
    return Tr_ExpList(
            val_expty.exp,
            makeRecordVals(level, venv, tenv, kv->tail, kt->tail, pos));
}

static struct expty
transRecodeExp(Tr_level level, S_table venv, S_table tenv, A_exp record_exp) {
    Ty_ty deced_ty = S_look(tenv, record_exp->u.record.typ), record_ty;

    if (deced_ty == NULL) {
        EM_error(record_exp->pos, "Record type %s use before declare", 
                S_name(record_exp->u.record.typ));
        return expTy(NULL, Ty_Nil());
    }
    record_ty = actual_ty(deced_ty);
    if (record_ty->kind != Ty_record) {
        EM_error(record_exp->pos, "Expect record type before `{...}`");
        return expTy(NULL, Ty_Nil());
    }
    Tr_expList val_exps = makeRecordVals(level, venv, tenv, 
            record_exp->u.record.fields, record_ty->u.record, record_exp->pos);

    return expTy(
            Tr_recordExp(val_exps), 
            actual_ty(record_ty));
}

/* 
 * 1. look up array type 
 * 2. check `size` is `int` and `init` type is right
 */
static struct expty
transArrayExp(Tr_level level, S_table venv, S_table tenv, A_exp array_exp) {
    Ty_ty deced_ty = S_look(tenv, array_exp->u.array.typ), array_ty;

    if (deced_ty == NULL) {
        EM_error(array_exp->pos, "Array type %s use before declare", 
                S_name(array_exp->u.array.typ));
        exit(1);
    }

    array_ty = actual_ty(deced_ty);
    if (array_ty->kind != Ty_array) {
        EM_error(array_exp->pos, "Expect array type before `[exp] of exp`");
        exit(1);
    }

    struct expty size_expty = transExp(level, venv, tenv, array_exp->u.array.size);
    if (size_expty.ty->kind != Ty_int) {
        EM_error(array_exp->pos, "Array size clause must be int");
        exit(1);
    }

    struct expty init_expty = transExp(level, venv, tenv, array_exp->u.array.init);
    /* NOTE: array items type compare `array_ty->u.array` instead of `array_ty` */
    if (!type_equal(init_expty.ty, array_ty->u.array)) {
        EM_error(array_exp->pos, "Array init clause type dismatch");
        exit(1);
    }

    return expTy(
            Tr_arrayExp(size_expty.exp, init_expty.exp), 
            actual_ty(array_ty));
}

/*
 * `()` `(a <> nul)` `(s1; s2; s3)` all be `seqexp`
 */
static struct expty
transSeqExp(Tr_level level, S_table venv, S_table tenv, A_exp seq_exp) {
    A_expList el;
    struct expty last_expty;
    
    if (seq_exp->u.seq == NULL) {
        return expTy(NULL, Ty_Void());
    } else {
        for (el = seq_exp->u.seq; el; el = el->tail) {
            last_expty = transExp(level, venv, tenv, el->head);
        }
        return expTy(NULL, actual_ty(last_expty.ty));
    }
}

static struct expty
transAssignExp(Tr_level level, S_table venv, S_table tenv, A_exp assign_exp) {
    struct expty var_expty = transVar(level, venv, tenv, assign_exp->u.assign.var); 
    struct expty val_expty = transExp(level, venv, tenv, assign_exp->u.assign.exp); 

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
transIfExp(Tr_level level, S_table venv, S_table tenv, A_exp if_exp) {
    struct expty test_expty = transExp(level, venv, tenv, if_exp->u.iff.test);
    if (test_expty.ty->kind != Ty_int) {
        EM_error(if_exp->pos, "If test clause must be int");
        return expTy(NULL, Ty_Void());
    }

    struct expty then_expty = transExp(level, venv, tenv, if_exp->u.iff.then);
    if (if_exp->u.iff.elsee) {
        struct expty else_expty = transExp(level, venv, tenv, if_exp->u.iff.elsee);
        if (!type_equal(then_expty.ty, else_expty.ty)) {
            EM_error(if_exp->pos, "IF THEN ELSE clause THEN ELSE type dismatch");
            return expTy(NULL, Ty_Void());
        } else {
            return expTy(
                    Tr_ifExp(test_expty.exp, then_expty.exp, else_expty.exp), 
                    actual_ty(then_expty.ty));
        }
    } else {
        if (then_expty.ty->kind != Ty_void) {
            EM_error(if_exp->pos, "IF THEN clause THEN must return void");
            return expTy(NULL, Ty_Void());
        } else {
            return expTy(
                    Tr_ifExp(test_expty.exp, then_expty.exp, Tr_nop()), 
                    Ty_Void());
        }
    }
}

static struct expty
transWhileExp(Tr_level level, S_table venv, S_table tenv, A_exp while_exp) {
    struct expty test_expty = transExp(level, venv, tenv, while_exp->u.whilee.test);
    if (test_expty.ty->kind != Ty_int) {
        /* Not return, just go through */
        EM_error(while_exp->pos, "While test clause must be int");
        exit(1);
    }

    bool temp = LOOP_LABELS.is_nested;
    LOOP_LABELS.is_nested = TRUE;
    Temp_label done = Temp_newlabel();
    LOOP_LABELS.done_labels = Temp_LabelList(done, LOOP_LABELS.done_labels);

    struct expty body_expty = transExp(level, venv, tenv, while_exp->u.whilee.body);
    if (body_expty.ty->kind != Ty_void) {
        EM_error(while_exp->pos, "WHILE body clause must be void");
        exit(1);
    }

    LOOP_LABELS.done_labels = LOOP_LABELS.done_labels->tail;
    LOOP_LABELS.is_nested = temp;

    return expTy(
            Tr_whileExp(test_expty.exp, body_expty.exp, done), 
            Ty_Void());
}

static struct expty
transForExp(Tr_level level, S_table venv, S_table tenv, A_exp for_exp) {
    struct expty lo_expty = transExp(level, venv, tenv, for_exp->u.forr.lo);
    struct expty hi_expty = transExp(level, venv, tenv, for_exp->u.forr.hi);
    
    if (lo_expty.ty->kind != Ty_int || hi_expty.ty->kind != Ty_int) {
        EM_error(for_exp->pos, "For lo, hi clause must be int");
        exit(1);
    }

    S_beginScope(venv);
    bool temp = LOOP_LABELS.is_nested;
    LOOP_LABELS.is_nested = TRUE;
    Temp_label done = Temp_newlabel();
    LOOP_LABELS.done_labels = Temp_LabelList(done, LOOP_LABELS.done_labels);

    Tr_access access = Tr_allocLocal(level, for_exp->u.forr.escape);
    S_enter(venv, for_exp->u.forr.var, E_VarEntry(access, Ty_Int()));
    /* TODO: for iter var should not be assign 
     * So, maybe add an keyword `const`, and an attribute `const` to `A_var`
     */ 
    struct expty body_expty = transExp(level, venv, tenv, for_exp->u.forr.body);

    LOOP_LABELS.done_labels = LOOP_LABELS.done_labels->tail;
    LOOP_LABELS.is_nested = temp;
    S_endScope(venv);

    if (body_expty.ty->kind != Ty_void) {
        EM_error(for_exp->pos, "For body clause must return void");
        exit(1);
    }
    return expTy(
            Tr_forExp(lo_expty.exp, hi_expty.exp, body_expty.exp, done), 
            Ty_Void());
}

static struct expty
transBreakExp(Tr_level level, S_table venv, S_table tenv, A_exp brk_exp) {
    if (!LOOP_LABELS.is_nested) {
        EM_error(brk_exp->pos, "Break clause outside loop");
        exit(1);
    }

    Temp_label done = LOOP_LABELS.done_labels->head;
    return expTy(
            Tr_breakExp(done), 
            Ty_Void());
}

/* 
 * 1. enter dec to the venv and tenv
 * 2. trans body
 */
static struct expty
transLetExp(Tr_level level, S_table venv, S_table tenv, A_exp let_exp) {
    struct expty body_expty;
    A_decList dl;

    S_beginScope(venv);
    S_beginScope(tenv);
    for (dl = let_exp->u.let.decs; dl; dl = dl->tail) {
        transDec(level, venv, tenv, dl->head);
    }
    body_expty = transExp(level, venv, tenv, let_exp->u.let.body);
    S_endScope(tenv);
    S_endScope(venv);
    return body_expty;
}

static struct expty
transSimpleVar(Tr_level level, S_table venv, S_table tenv, A_var simple_var) {
    E_enventry x = S_look(venv, simple_var->u.simple);

    if (x && x->kind == E_varEntry) {
        return expTy(
                Tr_simpleVar(x->u.var.access, level), 
                actual_ty(x->u.var.ty));
    } else {
        EM_error(simple_var->pos, "Variable %s use before declare", 
                S_name(simple_var->u.simple));
        /* default int ? */
        return expTy(NULL, Ty_Int());
    }
}

/*
 * var type has the `sym` field?
 */
static struct expty
transFieldVar(Tr_level level, S_table venv, S_table tenv, A_var field_var) {
    struct expty record_expty = transVar(level, venv, tenv, field_var->u.field.var);

    if (record_expty.ty->kind != Ty_record) {
        EM_error(field_var->pos, "Record field operate <type: record> required");
        return expTy(NULL, Ty_Int());
    }

    Ty_fieldList iter;
    int nth = 0;
    for (iter = record_expty.ty->u.record; iter; iter = iter->tail) {
        nth ++;
        if (iter->head->name == field_var->u.field.sym) {
            return expTy(
                    Tr_fieldVar(record_expty.exp, nth), 
                    actual_ty(iter->head->ty));
        }
    }

    EM_error(field_var->pos, "Record not have the key: %s", 
            S_name(field_var->u.field.sym));
    return expTy(NULL, Ty_Int());
}

/*
 * index `exp` must be `int`
 */
static struct expty
transSubscriptVar(Tr_level level, S_table venv, S_table tenv, A_var subscript_var) {
    struct expty array_expty = transVar(level, venv, tenv, 
                                        subscript_var->u.subscript.var);

    if (array_expty.ty->kind != Ty_array) {
        EM_error(subscript_var->pos, 
                "Array subscript operate <type: array> required");
        return expTy(NULL, Ty_Int());
    }

    struct expty idx_expty = transExp(level, venv, tenv, 
                                      subscript_var->u.subscript.exp);
    if (idx_expty.ty->kind != Ty_int) {
        EM_error(subscript_var->pos, "Array index <type: int> required");
        return expTy(NULL, Ty_Int());
    }
    /* TODO: check out of index error */
    return expTy(
            Tr_subscriptVar(array_expty.exp, idx_expty.exp), 
            actual_ty(array_expty.ty->u.array));
}

static void
transVarDec(Tr_level level, S_table venv, S_table tenv, A_dec var_dec) {
    Ty_ty deced_ty;
    struct expty init_expty = transExp(level, venv, tenv, var_dec->u.var.init);

    if (var_dec->u.var.typ) {
        deced_ty = S_look(tenv, var_dec->u.var.typ);
        if (deced_ty == NULL) {
            EM_error(var_dec->pos, "Type %s use before declare", 
                    S_name(var_dec->u.var.typ));
        }
        if (!type_equal(deced_ty, init_expty.ty)) {
            EM_error(var_dec->pos, "Variable init type dismatch");
        }
    } else {
        /* test45.tig */
        if (init_expty.ty->kind == Ty_nil) {
            EM_error(var_dec->pos, 
                    "Variable init `nil` can only used in declared record type");
        }
    }

    Tr_access access = Tr_allocLocal(level, var_dec->u.var.escape);
    S_enter(venv, var_dec->u.var.var, E_VarEntry(access, init_expty.ty));

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
        EM_error(params->head->pos, "Type %s use before declare", 
                S_name(params->head->typ));
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
transFuncDec(Tr_level level, S_table venv, S_table tenv, A_dec func_dec) {
    A_fundec f;
    A_fundecList fl;
    A_fieldList params = NULL;
    Ty_tyList formal_tys = NULL;
    Ty_ty result_ty = NULL;
    E_enventry func_entry = NULL;
    Tr_level new_level;
    Tr_accessList al;
    U_boolList escape_params;
    struct expty body_expty;

    /* first time collect function head info */
    for (fl = func_dec->u.function; fl; fl = fl->tail) {
        f = fl->head;
        escape_params = makeBoolList(f->params);
        new_level = Tr_newLevel(level, Temp_newlabel(), escape_params);
        formal_tys = makeFormalTyList(tenv, f->params);
        if (f->result) {
            result_ty = S_look(tenv, f->result);
            if (result_ty == NULL) {
                EM_error(f->pos, "Function return type %s use before declare", 
                        S_name(f->result));
            }
        } else {
            result_ty = Ty_Void();
        }
        S_enter(venv, f->name, 
                E_FunEntry(new_level, Temp_newlabel(), formal_tys, result_ty));
    }

    /* second time handle function body */
    for (fl = func_dec->u.function; fl; fl = fl->tail) {
        f = fl->head;
        params = f->params;
        func_entry = S_look(venv, f->name);
        new_level = func_entry->u.fun.level;
        formal_tys = func_entry->u.fun.formals;
        al = Tr_formals(func_entry->u.fun.level);

        /* test_break.tig */
        S_beginScope(venv);
        bool temp = LOOP_LABELS.is_nested;
        LOOP_LABELS.is_nested = FALSE;

        for (; params && formal_tys && al; 
                params = params->tail, formal_tys = formal_tys->tail, al = al->tail) {
            S_enter(venv, params->head->name, 
                    E_VarEntry(al->head, formal_tys->head));
        }
        body_expty = transExp(new_level, venv, tenv, f->body);
        if (!type_equal(body_expty.ty, func_entry->u.fun.results)) {
            EM_error(func_dec->pos, "Function declared return type dimatch with body");
        }

        LOOP_LABELS.is_nested = temp;
        S_endScope(venv);
        /* Tr_print(new_level); */
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
            EM_error(name_ty->pos, "Type %s circle declare", 
                    S_name(name_ty->u.name));
            return Ty_Name(name_ty->u.name, Ty_Int());
        }
    } else {
        EM_error(name_ty->pos, "Type %s use before declare", 
                S_name(name_ty->u.name));
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
        EM_error(field_list->head->pos, "Type %s use before declare", 
                S_name(field_list->head->typ));
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
        EM_error(array_ty->pos, "Type %s use before declare", 
                S_name(array_ty->u.array));
        return Ty_Array(Ty_Int());
    }
    return Ty_Array(t);
}

struct expty transExp(Tr_level level, S_table venv, S_table tenv, A_exp a) {
    switch (a->kind) {
        case A_varExp: return transVar(level, venv, tenv, a->u.var);
        case A_nilExp: return expTy(Tr_nilExp(), Ty_Nil());
        case A_intExp: return expTy(Tr_intExp(a->u.intt), Ty_Int());
        case A_stringExp: return expTy(
                                  Tr_stringExp(a->u.stringg), 
                                  Ty_String());
        case A_callExp: return transCallExp(level, venv, tenv, a);
        case A_opExp: return transOpExp(level, venv, tenv, a);
        case A_recordExp: return transRecodeExp(level, venv, tenv, a);
        case A_arrayExp: return transArrayExp(level, venv, tenv, a);
        case A_seqExp: return transSeqExp(level, venv, tenv, a);
        case A_assignExp: return transAssignExp(level, venv, tenv, a);
        case A_ifExp: return transIfExp(level, venv, tenv, a);
        case A_whileExp: return transWhileExp(level, venv, tenv, a);
        case A_forExp: return transForExp(level, venv, tenv, a);
        case A_breakExp: return transBreakExp(level, venv, tenv, a);  /* check used by `while` `for` */
        case A_letExp: return transLetExp(level, venv, tenv, a);
        default: assert(0);
    }
}

struct expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: return transSimpleVar(level, venv, tenv, v);
        case A_fieldVar: return transFieldVar(level, venv, tenv, v);
        case A_subscriptVar: return transSubscriptVar(level, venv, tenv, v);
        default: assert(0);
    }
}

void transDec(Tr_level level, S_table venv, S_table tenv, A_dec d) {
    switch (d->kind) {
        case A_varDec: return transVarDec(level, venv, tenv, d);
        case A_functionDec: return transFuncDec(level, venv, tenv, d);
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
    Tr_level outermost = Tr_outermost();
    Tr_level tiger_main = Tr_newLevel( outermost, Temp_namedlabel("tiger_main"), NULL);
    transExp(tiger_main, venv, tenv, exp);

    /* Tr_print(tiger_main); */
    /* Tr_print(outermost); */

    return ;
}

