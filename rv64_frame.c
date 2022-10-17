#include "frame.h"
#include "temp.h"
#include "util.h"


/*                  RISC-V X regs
 *
 *  Register    ABI Name    Desc                Saver
 *  x0          zero        —-                  --        
 *  x1          ra          Return address      Caller
 *  x2          sp          Stack pointer       Callee
 *  x3          gp          Global pointer      —-
 *  x4          tp          Thread pointer      --
 *  x5-7        t0-2        Temporaries         Caller
 *  x8          s0/fp       Saved/frame pointer Callee
 *  x9          s1          Saved register      Callee
 *  x10-11      a0-1        Fn args/return val  Caller
 *  x12-17      a2-7        Fn args             Caller
 *  x18-27      s2-11       Saved registers     Callee
 *  x28-31      t3-6        Temporaries         Caller
 */

const int F_wordSize = 8;
Temp_map F_tempMap = NULL;

static Temp_tempList returnSink = NULL;

static const int F_XREG_NUM = 32;
static Temp_temp XREGS[F_XREG_NUM];
static Temp_tempList ARG_REGS = NULL;
static Temp_tempList CALLEE_SAVES = NULL;
static Temp_tempList CALLER_SAVES = NULL;
static char XREG_NAMES[][5] = {
    "x0", "ra", "sp", "gp", "tp", 
    "t0", "t1", "t2",
    "s0", "s1",
    "a0", "a1",
    "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"
};

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
    if (formals == NULL)
        return NULL;

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

void F_initMap() {
    if (F_tempMap != NULL)
        return ;

    int i;
    F_tempMap = Temp_empty();
    for (i = 0; i < F_XREG_NUM; i++) {
        XREGS[i] = Temp_newtemp();
        Temp_enter(F_tempMap, XREGS[i], String(XREG_NAMES[i]));
    }
#ifdef TG_DEBUG
    Temp_dumpMap(stdout, F_tempMap);
#endif
}

Temp_temp F_FP() {
    return XREGS[8];
}

Temp_temp F_SP() {
    return XREGS[2];
}

Temp_temp F_ZERO() {
    return XREGS[0];
}

Temp_temp F_RA() {
    return XREGS[1];
}

/* RV just use one reg */
Temp_temp F_RV() {
    return XREGS[10];
}

Temp_tempList makeRegList(int *idxs, int len) {
    if (len == 0)
        return NULL;

    return Temp_TempList(
            XREGS[idxs[0]], 
            makeRegList(idxs + 1, len - 1));
}

Temp_tempList F_argRegs() {
    if (ARG_REGS != NULL)
        return ARG_REGS;

    int idxs[8] = {10, 11, 12, 13, 14, 15, 16, 17};
    ARG_REGS = makeRegList(idxs, 8);
    return ARG_REGS;
}

Temp_tempList F_calleeSaves() {
    if (CALLEE_SAVES != NULL)
        return CALLEE_SAVES;

    int idxs[12] = {
        8, 9, 
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27 
    };
    CALLEE_SAVES = makeRegList(idxs, 12);
    return CALLEE_SAVES;
}

Temp_tempList F_callerSaves() {
    if (CALLER_SAVES != NULL)
        return CALLER_SAVES;

    int idxs[15] = {
        5, 6, 7, 
        10, 11, 12, 13, 14, 15, 16, 17, 
        28, 29, 30, 31
    };
    CALLER_SAVES = makeRegList(idxs, 8);
    return CALLER_SAVES;
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
    return stm;
}

/* TODO: */
AS_instrList F_procEntryExit2(AS_instrList body) {
    if (!returnSink)
        returnSink = Temp_TempList(NULL, NULL);

    return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
    char buf[100];
    sprintf(buf, "PROCEDURE %s\n", S_name(frame->name));
    return AS_Proc(String(buf), body, "END\n");
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

