#include <stdio.h>

#include "translate.h"
#include "frame.h"

static Tr_level OUTERMOST_LEVEL = NULL;

struct Tr_level_ {
    Tr_level parent;
    Tr_accessList s_formals;  /* static link + formals */
    F_frame frame;
};

struct Tr_access_ {
    Tr_level level;
    F_access access;
};

static Tr_access Tr_Access(Tr_level level, F_access access) {
    Tr_access p = checked_malloc(sizeof(*p));

    p->level = level;
    p->access = access;

    return p;
}

Tr_accessList 
Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList p = checked_malloc(sizeof(*p));

    p->head = head;
    p->tail = tail;

    return p;
}

static Tr_accessList 
makeSformals(Tr_level level, F_accessList formals) {
    if (formals == NULL) {
        return NULL;
    }

    return Tr_AccessList(
            Tr_Access(level, formals->head),
            makeSformals(level, formals->tail));
}

Tr_level Tr_outermost(void) {
    if (OUTERMOST_LEVEL == NULL) {
        Tr_level level = Tr_newLevel(NULL, Temp_namedlabel("tiger_global"), NULL);
        /* let no `NULL` */
        level->parent = level;
        OUTERMOST_LEVEL = level;
    } 

    return OUTERMOST_LEVEL;
}

Tr_level 
Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level level = checked_malloc(sizeof(*level));

    U_boolList static_link_formals = U_BoolList(TRUE, formals);
    level->parent = parent;
    level->frame = F_newFrame(name, static_link_formals);
    level->s_formals = makeSformals(level, F_formals(level->frame));

    return level;
}

Tr_access Tr_static_link(Tr_level level) {
    return level->s_formals->head;
}

Tr_accessList
Tr_formals(Tr_level level) {
    return level->s_formals->tail;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    F_access fa = F_allocLocal(level->frame, escape); 

    return Tr_Access(level, fa);
}

typedef struct patchList_ *patchList;
struct patchList_ {
    Temp_label *head;
    patchList tail;
};
static patchList PatchList(Temp_label *head, patchList tail);

struct Cx {
    patchList trues;
    patchList falses;
    T_stm stm;
};

struct Tr_exp_ {
    enum {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    } u;
};
static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

/* debug function */
void Tr_print(Tr_level level) {
    F_print(level->frame);
}


