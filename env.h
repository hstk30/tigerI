#ifndef ENV_H__
#define ENV_H__

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "translate.h"

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ {
    enum {
        E_varEntry, 
        E_funEntry,
        E_escEntry
    } kind;
    union {
        struct {
            Tr_access access; 
            Ty_ty ty;
        } var;
        struct {
            Tr_level level; 
            Temp_label label; 
            Ty_tyList formals; 
            Ty_ty results;
        } fun;
        struct {
            int depth; 
            bool *escape;
        } escape;
    } u;
};

/* constructor for value environment */
E_enventry E_VarEntry(Tr_access access, Ty_ty ty);
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty results);
E_enventry E_EscEntry(int depth, bool *escape);

S_table E_base_tenv();  /* Ty_ty  environment */
S_table E_base_venv();  /* E_enventry environment */
S_table E_base_eenv();  /* escape env */

#endif
