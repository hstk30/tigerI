#include "env.h"

E_enventry E_VarEntry(Ty_ty ty) {
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_varEntry;
    entry->u.var.ty = ty;
    return entry;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty results) {
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_funEntry;
    entry->u.fun.formals = formals;
    entry->u.fun.results = results;
    return entry;
}

S_table E_base_tenv() {
    S_table tenv = S_empty();
    S_enter(tenv, S_Symbol("int"), Ty_Int());
    S_enter(tenv, S_Symbol("string"), Ty_String());
    S_enter(tenv, S_Symbol("nil"), Ty_Nil());
    return tenv;
}

