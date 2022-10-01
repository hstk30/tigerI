#include <stdio.h>

#include "escape.h"
#include "symbol.h"
#include "env.h"


static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);


U_boolList makeBoolList(A_fieldList params) {
    if (params == NULL) {
        return NULL;
    }

    return U_BoolList(
            params->head->escape, 
            makeBoolList(params->tail));
}

static void traverseExp(S_table eenv, int depth, A_exp e) {
    switch (e->kind) {
        case A_varExp: return traverseVar(eenv, depth, e->u.var);
        case A_nilExp: return ;
        case A_intExp: return ;
        case A_stringExp: return ;
        case A_callExp: {
            A_expList el;
            for (el = e->u.call.args; el; el = el->tail) {
                traverseExp(eenv, depth, el->head);
            }
            return ;
        }
        case A_opExp: {
            traverseExp(eenv, depth, e->u.op.left);
            traverseExp(eenv, depth, e->u.op.right);
            return ;
        }
        case A_recordExp: {
            A_efieldList kv;
            for (kv = e->u.record.fields; kv; kv = kv->tail) {
                traverseExp(eenv, depth, kv->head->exp);
            }
            return ;
        }
        case A_arrayExp: {
            traverseExp(eenv, depth, e->u.array.size);
            traverseExp(eenv, depth, e->u.array.init);
            return ;
        }
        case A_seqExp: {
            A_expList el;
            for (el = e->u.seq; el; el = el->tail) {
                traverseExp(eenv, depth, el->head);
            }
            return ;
        }
        case A_assignExp: {
            traverseExp(eenv, depth, e->u.assign.exp);
            traverseVar(eenv, depth, e->u.assign.var);
            return ;
        }
        case A_ifExp: {
            traverseExp(eenv, depth, e->u.iff.test);
            traverseExp(eenv, depth, e->u.iff.then);
            if (e->u.iff.elsee) {
                traverseExp(eenv, depth, e->u.iff.elsee);
            }
            return ;
        }
        case A_whileExp: {
            traverseExp(eenv, depth, e->u.whilee.test);
            traverseExp(eenv, depth, e->u.whilee.body);
            return ;
        }
        case A_forExp: {
            S_beginScope(eenv);
            e->u.forr.escape = FALSE;
            S_enter(eenv, e->u.forr.var, E_EscEntry(depth, &(e->u.forr.escape)));
            S_endScope(eenv);
            return ;
        }
        case A_breakExp: return ;  
        case A_letExp: {
            A_decList dl;
            S_beginScope(eenv);
            for (dl = e->u.let.decs; dl; dl = dl->tail) {
                traverseDec(eenv, depth, dl->head);
            }
            traverseExp(eenv, depth, e->u.let.body);
            S_endScope(eenv);
            return ;
        } 
        default: assert(0);
    }
}

static void traverseDec(S_table eenv, int depth, A_dec d) {
    switch (d->kind) {
        case A_varDec: {
            /* first init then var */
            traverseExp(eenv, depth, d->u.var.init);
            d->u.var.escape = FALSE;
            S_enter(eenv, d->u.var.var, E_EscEntry(depth, &(d->u.var.escape)));
            return ;
        }
        case A_functionDec: {
            A_fundecList fl = NULL;
            A_fieldList params = NULL;

            for (fl = d->u.function; fl; fl = fl->tail) {
                S_beginScope(eenv);

                for (params = fl->head->params; params; params = params->tail) {
                    params->head->escape = FALSE;
                    S_enter(eenv, params->head->name, 
                            E_EscEntry(depth + 1, &(params->head->escape)));
                }
                traverseExp(eenv, depth + 1, fl->head->body);

                S_endScope(eenv);
            }
        }
        case A_typeDec: return ;
        default: assert(0);
    }
}

static void traverseVar(S_table eenv, int depth, A_var v) {
    switch (v->kind) {
        case A_simpleVar: {
            E_enventry x = S_look(eenv, v->u.simple);
            if (x->u.escape.depth < depth) {
                *(x->u.escape.escape) = TRUE;
            }
            return ;
        }
        case A_fieldVar: {
            traverseVar(eenv, depth, v->u.field.var);
            return ;
        }
        case A_subscriptVar: {
            traverseVar(eenv, depth, v->u.subscript.var);
            traverseExp(eenv, depth, v->u.subscript.exp);
            return ;
        }
        default: assert(0);
    }
}

void Esc_findEscape(A_exp ast_root) {
    S_table eenv = E_base_eenv();
    int init_deep = 0;

    traverseExp(eenv, init_deep, ast_root);

    return ;
}
