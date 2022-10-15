#include "codegen.h"
#include "frame.h"

#define L(h, t) Temp_TempList(h, t)

static AS_instrList iList = NULL, last = NULL;

static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);

static void
munchMoveStm(T_stm mov) {
    T_exp dst = mov->u.MOVE.dst;
    T_exp src = mov->u.MOVE.src;
    T_exp mem, binop;

    if (dst->kind == T_MEM) {
        mem = dst->u.MEM;
        switch (mem->kind) {
            case T_BINOP: {
                if (mem->u.BINOP.op == T_plus) {
                    if (mem->u.BINOP.right->kind == T_CONST) {

                    }
                }
                

            }
        }

    } else if (dst->kind == T_TEMP) {

    } else {
        assert(0);
    }
}

static void 
munchStm(T_stm s) {
    switch (s->kind) {
        case T_MOVE: {
            T_exp dst = s->u.MOVE.dst;
            T_exp src = s->u.MOVE.src;
            switch (dst->kind) {
                case T_MEM: {
                

                }
                case T_TEMP:
                default: assert(0);
            }
        }
    }

}

static Temp_temp munchExp(T_exp e) {

}


static void emit(AS_instr inst) {
    if (last != NULL) {
        last->tail = AS_InstrList(inst, NULL);
        last = last->tail;
    } else {
        iList = AS_InstrList(inst, NULL);
        last = iList;
    }
}


AS_instrList F_codegen(F_frame f, T_stmList stmLists) {
    AS_instrList list;
    T_stmList sl;

    for (sl = stmLists; sl; sl = sl->tail) {
        munchStm(sl->head);
    }

    list = iList;
    iList = last = NULL;

    return list;
}

