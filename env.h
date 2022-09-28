#ifndef ENV_H__
#define ENV_H__

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
    enum {
        E_varEntry, 
        E_funEntry,
        E_escEntry
    } kind;
    union {
        struct {Ty_ty ty;} var;
        struct {Ty_tyList formals; Ty_ty results;} fun;
        struct {int depth; bool *escape;} escape;
    } u;

};

/* constructor for value environment */
E_enventry E_VarEntry(Ty_ty ty);
E_enventry E_FunEntry(Ty_tyList formals, Ty_ty results);
E_enventry E_EscEntry(int depth, bool *escape);

S_table E_base_tenv();  /* Ty_ty  environment */
S_table E_base_venv();  /* E_enventry environment */
S_table E_base_eenv();  /* escape env */

#endif
