#include "escape.h"
#include "symbol.h"
#include "env.h"

static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);


void Esc_findEscape(A_exp ast_root) {
    S_table eenv = E_base_eenv();
    int init_deep = 0;

    traverseExp(eenv, init_deep, ast_root);

    return ;
}

static void traverseExp(S_table eenv, int depth, A_exp e) {
    switch (e->kind) {
        case A_varExp: {

        }
        case A_nilExp: return ;
        case A_intExp: return ;
        case A_stringExp: return ;
        case A_callExp: return ;
        case A_opExp: return ;
        case A_recordExp: return ;
        case A_arrayExp: return ;
        case A_seqExp: return ;
        case A_assignExp: return ;
        case A_ifExp: return ;
        case A_whileExp: return ;
        case A_forExp: return ;
        case A_breakExp: return ;  
        case A_letExp: return ;
        default: assert(0);
    }
}

static void traverseDec(S_table eenv, int depth, A_dec d) {
    switch (d->kind) {
        case A_varDec: return ;
        case A_functionDec: {
            A_fundecList fl = NULL;
            A_fieldList params = NULL;

            for (fl = d->u.function; fl; fl = fl->tail) {
                S_beginScope(eenv);

                for (params = fl->head->params; params; params = params->tail) {
                    if (!params->head->escape) {
                        params->head->escape = FALSE;
                        S_enter(eenv, params->head->name, E_EscEntry(nest_depth, &(params->head->escape)));
                    }
                }
                S_endScope(eenv);
            }

            
        }
        case A_typeDec: return ;
        default: assert(0);
    }
}

static void traverseVar(S_table eenv, int depth, A_var v) {
    switch (v->kind) {
        case A_simpleVar: return ;
        case A_fieldVar: return ;
        case A_subscriptVar: return ;
        default: assert(0);
    }
}
