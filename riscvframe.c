#include "frame.h"
#include "temp.h"
#include "util.h"

struct F_frame_ {
    Temp_label name;
    /* formals and local variable */;
    F_accessList formals, locals, locals_tail;   
    int offset;
};

struct F_access_ {
    enum {inFrame, inReg} kind;
    union {
        int offset;
        Temp_temp reg;
    } u;
};

static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);

static F_access InFrame(int offset) {
    F_access acc = checked_malloc(sizeof(*acc));
    acc->kind = inFrame;
    acc->u.offset = offset;
    return acc;
}

static F_access InReg(Temp_temp reg) {
    F_access acc = checked_malloc(sizeof(*acc));
    acc->kind = inReg;
    acc->u.reg = reg;
    return acc;
}

static F_access
makeAccess(F_frame f, bool escape) {
    F_access access;

    if (escape) {
        access = InFrame(f->offset);
        f->offset += F_wordSize;
    } else {
        access = InReg(Temp_newtemp());
    }

    return access;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

static F_accessList
makeFormals(F_frame f, U_boolList formals) {
    if (formals == NULL) {
        return NULL;
    }

    F_access access;
    if (formals->head) {
        access = InFrame(f->offset);
        f->offset += F_wordSize;
    } else {
        access = InReg(Temp_newtemp());
    }
    return F_AccessList(access, makeFormals(f, formals->tail));
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame p = checked_malloc(sizeof(*p));
    p->name = name;
    p->locals = NULL;
    p->locals_tail = NULL;
    p->offset = 0;
    p->formals = makeFormals(p, formals);

    return p;
}

Temp_label F_name(F_frame f) {
    return f->name;
}

F_accessList F_formals(F_frame f) {
    return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
    F_access access = makeAccess(f, escape);
    F_accessList tail_node = F_AccessList(access, NULL);

    if (f->locals == NULL) {
        f->locals = tail_node;
        f->locals_tail = tail_node;
    } else {
        f->locals_tail->tail = tail_node;
        f->locals_tail = tail_node;
    }

    return access;
}

/* debug info */
void FP_frame(F_frame f) {
    F_accessList iter;
    printf("Frame name: %s\n", S_name(f->name));

    printf("\tformals: \n");
    for (iter = f->formals; iter; iter = iter->tail) {
        if (iter->head->kind == inFrame) {
            printf("\t\tInFrame(%d)\n", iter->head->u.offset);
        } else {

            printf("\t\tInReg\n");
        }
    }

    printf("\tlocals: \n");
    for (iter = f->locals; iter; iter = iter->tail) {
        if (iter->head->kind == inFrame) {
            printf("\t\tInFrame(%d)\n", iter->head->u.offset);
        } else {

            printf("\t\tInReg\n");
        }
    }
    printf("Frame size: %d\n", f->offset);
    
    return ;
}

