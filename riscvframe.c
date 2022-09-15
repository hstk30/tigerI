#include "frame.h"
#include "util.h"

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
