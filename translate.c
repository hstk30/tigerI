/*
 * Cause this file have multi-chatper code,
 * so layout by chatper, not coding rule.
 * TODO: when finish this project, then rearranage it.
 */
#include <stdio.h>

#include "translate.h"
#include "frame.h"


/*** stack frame or activation record rel ***/
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

/* debug function */
void Tr_print(Tr_level level) {
    F_print(level->frame);
}

/*** stack frame end ***/

/*** Intermediate representation rel***/
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

static void 
doPatch(patchList t, Temp_label label) {
    for (; t; t = t->tail) {
        *(t->head) = label;
    }
    return ;
}

static patchList 
joinPatch(patchList first, patchList second) {
    if (!first) {
        return second;
    }

    for (; first->tail; first = first->tail) 
        ;
    first->tail = second;

    return first;
}

static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_ex;
    p->u.ex = ex;

    return p;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_nx;
    p->u.nx = nx;

    return p;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp p = checked_malloc(sizeof(*p));

    p->kind = Tr_cx;
    p->u.cx = (struct Cx){trues, falses, stm};

    return p;
}


static T_exp 
unEx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex: return e->u.ex;
        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);

            return T_Eseq(
                    T_Move(T_Temp(r), T_Const(1)),
                    T_Eseq(
                        e->u.cx.stm,
                        T_Eseq(
                            T_Label(f),
                            T_Eseq(
                                T_Move(T_Temp(r), T_Const(0)),
                                T_Eseq(
                                    T_Label(t), 
                                    T_Temp(r))))));
        }
        case Tr_nx: return T_Eseq(e->u.nx, T_Const(0));
        default: assert(0);
    }
}

static T_stm
unNx(Tr_exp e) {

}

static struct Cx 
unCx(Tr_exp e) { 

}

