#include <stdio.h>

#include "env.h"
#include "temp.h"

E_enventry E_VarEntry(Tr_access access, Ty_ty ty) 
{
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_varEntry;
    entry->u.var.access = access;
    entry->u.var.ty = ty;
    return entry;
}

E_enventry 
E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty results) 
{
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_funEntry;
    entry->u.fun.level = level;
    entry->u.fun.label = label;
    entry->u.fun.formals = formals;
    entry->u.fun.results = results;
    return entry;
}

E_enventry E_EscEntry(int depth, bool *escape) 
{
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_escEntry;
    entry->u.escape.depth = depth;
    entry->u.escape.escape = escape;
    return entry;
}

S_table E_base_tenv() 
{
    S_table tenv = S_empty();
    S_enter(tenv, S_Symbol("int"), Ty_Int());
    S_enter(tenv, S_Symbol("string"), Ty_String());
    S_enter(tenv, S_Symbol("nil"), Ty_Nil());
    return tenv;
}

S_table E_base_venv() 
{
    S_table venv = S_empty();

    S_enter(venv, S_Symbol("print"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_print"), 
                Ty_TyList(Ty_String(), NULL), Ty_Void()));
    S_enter(venv, S_Symbol("flush"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_flush"), 
                NULL, Ty_Void()));
    S_enter(venv, S_Symbol("getchar"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_getchar"),
                NULL, Ty_String()));
    S_enter(venv, S_Symbol("ord"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_ord"),
                Ty_TyList(Ty_String(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("chr"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_chr"),
                Ty_TyList(Ty_Int(), NULL), Ty_String()));
    S_enter(venv, S_Symbol("size"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_size"),
                Ty_TyList(Ty_String(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("substring"), 
            E_FunEntry(
                Tr_outermost(),
                Temp_namedlabel("tiger_substring"),
                Ty_TyList(Ty_String(), 
                    Ty_TyList(Ty_Int(), 
                        Ty_TyList(Ty_Int(), NULL))), 
                Ty_String()));
    S_enter(venv, S_Symbol("concat"), 
            E_FunEntry(
                Tr_outermost(),
                Temp_namedlabel("tiger_concat"),
                Ty_TyList(Ty_String(), 
                    Ty_TyList(Ty_String(), NULL)), 
                Ty_String()));
    S_enter(venv, S_Symbol("not"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_not"), 
                Ty_TyList(Ty_Int(), NULL), Ty_Int()));
    S_enter(venv, S_Symbol("exit"), 
            E_FunEntry(Tr_outermost(), Temp_namedlabel("tiger_exit"), 
                Ty_TyList(Ty_Int(), NULL), Ty_Void()));
    return venv;
}

S_table E_base_eenv() 
{
    S_table eenv = S_empty();
    return eenv;
}

