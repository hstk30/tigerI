#include "frame.h"
#include "temp.h"
#include "util.h"


const int F_wordSize = 8;

struct F_frame_ {
    Temp_label name;
    /* formals and local variable */
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

static Temp_temp FP = NULL;
Temp_temp F_FP() {
    if (FP == NULL) {
        FP = Temp_newtemp();
    }
    return FP;
}

static Temp_temp RV = NULL;
Temp_temp F_RV() {
    if (RV == NULL) {
        RV = Temp_newtemp();
    }
    return RV;
}

T_exp F_externalCall(string s, T_expList args) {
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}

T_exp F_Exp(F_access access, T_exp frame_ptr) {
    if (access->kind == inFrame) {
        return T_Mem(T_Binop(T_plus, frame_ptr, T_Const(access->u.offset)));
    } else {
        return T_Mem(T_Temp(access->u.reg));
    }
}

F_frag F_StringFrag(Temp_label label, string str) {
    F_frag p = checked_malloc(sizeof(*p));

    p->kind = F_stringFrag;
    p->u.stringg.label = label;
    p->u.stringg.str = str;

    return p;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
    F_frag p = checked_malloc(sizeof(*p));

    p->kind = F_procFrag;
    p->u.proc.body = body;
    p->u.proc.frame = frame;

    return p;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
    F_fragList p = checked_malloc(sizeof(*p));

    p->head = head;
    p->tail = tail;

    return p;
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
    /* TODO: add function name label here temporarily */
    return T_Seq(T_Label(frame->name), stm);
}

/* debug info */
void F_print(F_frame f) {
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
    printf("Frame size: %d\n\n", f->offset);
    
    return ;
}

