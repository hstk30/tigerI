#include "frame.h"
#include "temp.h"
#include "util.h"

struct F_frame_ {
    Temp_label name;
    F_accessList formals, locals;   /* formals and local variable */;
    int size;
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

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_accessList al = NULL;
    U_boolList bl;
    int offset = 0;
    F_frame frame = checked_malloc(sizeof(*frame));

    frame->name = name;

    for (bl = formals; bl; bl = bl->tail) {

    }
}

Temp_label F_name(F_frame f) {
    return f->name;
}

F_accessList F_formals(F_frame f) {
    return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {

}

