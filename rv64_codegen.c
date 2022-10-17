#include "codegen.h"
#include "frame.h"

#define L(h, t) Temp_TempList(h, t)

static AS_instrList iList = NULL, last = NULL;

static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);
static void emit(AS_instr inst);

static void
munchMoveStm(T_stm mov) {
    T_exp dst = mov->u.MOVE.dst;
    T_exp src = mov->u.MOVE.src;
    char buf[100];

    if (dst->kind == T_MEM) {
        if (dst->u.MEM->kind == T_BINOP
                && dst->u.MEM->u.BINOP.op == T_plus
                && dst->u.MEM->u.BINOP.right->kind == T_CONST) {
            /* MOVE( MEM( BINOP(PLUS, TEMP t100, CONST 8)), CONST 8) */
            Temp_temp d0 = munchExp(dst->u.MEM->u.BINOP.left);
            Temp_temp s0 = munchExp(src);
            sprintf(buf, "sd `s0, %d(`d0`)`\n", dst->u.MEM->u.BINOP.right->u.CONST);
            emit(AS_Oper(String(buf), L(d0, NULL), L(s0, NULL), NULL));
        } else if (dst->u.MEM->kind == T_BINOP 
                    && dst->u.MEM->u.BINOP.op == T_plus
                    && dst->u.MEM->u.BINOP.left->kind == T_CONST) {
            Temp_temp d0 = munchExp(dst->u.MEM->u.BINOP.right);
            Temp_temp s0 = munchExp(src);
            sprintf(buf, "sd `s0, %d(`d0`)`\n", dst->u.MEM->u.BINOP.left->u.CONST);
            emit(AS_Oper(String(buf), L(d0, NULL), L(s0, NULL), NULL));
        } else if (src->kind == T_MEM) {
            /* MOVE( MEM(e1), MEM(e2)) */
            Temp_temp s0 = munchExp(src->u.MEM);
            Temp_temp d0 = munchExp(dst->u.MEM);
            /* Normally, not allow to `move` from `MEM` to `MEM` */
            Temp_temp temp = Temp_newtemp();
            sprintf(buf, "sd `s0, `d1\n");
            emit(AS_Oper(String(buf), L(temp, NULL), L(s0, NULL), NULL));
            sprintf(buf, "sd `d1, `d0\n");
            emit(AS_Oper(String(buf), L(d0, NULL), L(temp, NULL), NULL));
        } else {
            Temp_temp s0 = munchExp(src);
            Temp_temp d0 = munchExp(dst->u.MEM);
            sprintf(buf, "sd `s0, 0(`d0)\n");
            emit(AS_Oper(String(buf), L(d0, NULL), L(s0, NULL), NULL));
        }
    } else if (dst->kind == T_TEMP) {
            Temp_temp s0 = munchExp(src);
            sprintf(buf, "mv `d0, `s0\n");
            emit(AS_Oper(String(buf), L(dst->u.TEMP, NULL), L(s0, NULL), NULL));
    } else {
        assert(0);
    }

    return ;
}

static void 
munchStm(T_stm s) {
    switch (s->kind) {
        case T_MOVE: munchMoveStm(s);

        default: assert(0);
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

