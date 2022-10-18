#include "codegen.h"
#include "frame.h"

#define TTL(h, t) Temp_TempList(h, t)
#define TLL(h, t) Temp_LabelList(h, t)

static AS_instrList iList = NULL, last = NULL;

static void munchStm(T_stm s);
static Temp_temp munchExp(T_exp e);
static void emit(AS_instr inst);

static void
munchMoveStm(T_stm mov) {
    T_exp dst = mov->u.MOVE.dst;
    T_exp src = mov->u.MOVE.src;
    char buf[100];

    /* OPT: Use `match` mentioned in book, code will have too many ident */
    if (dst->kind == T_MEM) {
        if (dst->u.MEM->kind == T_BINOP
                && dst->u.MEM->u.BINOP.op == T_plus
                && dst->u.MEM->u.BINOP.right->kind == T_CONST) {
            /* MOVE( MEM( BINOP(PLUS, TEMP t100, CONST 8)), CONST 8) */
            Temp_temp d0 = munchExp(dst->u.MEM->u.BINOP.left);
            Temp_temp s0 = munchExp(src);
            sprintf(buf, "sd `s0, %d(`d0`)`\n", dst->u.MEM->u.BINOP.right->u.CONST);
            emit(AS_Move(String(buf), TTL(d0, NULL), TTL(s0, NULL)));
        } else if (dst->u.MEM->kind == T_BINOP 
                    && dst->u.MEM->u.BINOP.op == T_plus
                    && dst->u.MEM->u.BINOP.left->kind == T_CONST) {
            Temp_temp d0 = munchExp(dst->u.MEM->u.BINOP.right);
            Temp_temp s0 = munchExp(src);
            sprintf(buf, "sd `s0, %d(`d0`)`\n", dst->u.MEM->u.BINOP.left->u.CONST);
            emit(AS_Move(String(buf), TTL(d0, NULL), TTL(s0, NULL)));
        } else if (src->kind == T_MEM) {
            /* MOVE( MEM(e1), MEM(e2)) */
            Temp_temp s0 = munchExp(src->u.MEM);
            Temp_temp d0 = munchExp(dst->u.MEM);
            /* Normally, not allow to `move` from `MEM` to `MEM` */
            Temp_temp temp = Temp_newtemp();
            emit(AS_Move(String("sd `s0, `d1\n"), TTL(temp, NULL), TTL(s0, NULL)));
            emit(AS_Move(String("sd `d1, `d0\n"), TTL(d0, NULL), TTL(temp, NULL)));
        } else {
            Temp_temp s0 = munchExp(src);
            Temp_temp d0 = munchExp(dst->u.MEM);
            emit(AS_Move(String("sd `s0, 0(`d0)\n"), TTL(d0, NULL), TTL(s0, NULL)));
        }
    } else if (dst->kind == T_TEMP) {
            Temp_temp s0 = munchExp(src);
            emit(AS_Move(String("mv `d0, `s0\n"), TTL(dst->u.TEMP, NULL), TTL(s0, NULL)));
    } else {
        assert(0);
    }

    return ;
}

static void
munchCjumpStm(T_stm cjump) {
    Temp_temp s0 = munchExp(cjump->u.CJUMP.left);
    Temp_temp s1 = munchExp(cjump->u.CJUMP.right);

    /* `canon` let all `CJUMP` follow by it's `false` */
    switch (cjump->u.CJUMP.op) {
        case T_eq:
            return emit(AS_Oper(String("beq `s0, `s1, `j0\n"), 
                        NULL, TTL(s0, TTL(s1, NULL)), 
                        AS_Targets(TLL(cjump->u.CJUMP.l_true, NULL))));
        case T_ne: 
            return emit(AS_Oper(String("bne `s0, `s1, `j0\n"), 
                        NULL, TTL(s0, TTL(s1, NULL)), 
                        AS_Targets(TLL(cjump->u.CJUMP.l_true, NULL))));
        case T_lt:
            return emit(AS_Oper(String("blt `s0, `s1, `j0\n"), 
                        NULL, TTL(s0, TTL(s1, NULL)), 
                        AS_Targets(TLL(cjump->u.CJUMP.l_true, NULL))));
        case T_le:
            return emit(AS_Oper(String("bge `s1, `s0, `j0\n"), 
                        NULL, TTL(s1, TTL(s0, NULL)), 
                        AS_Targets(TLL(cjump->u.CJUMP.l_true, NULL))));
        case T_gt:
            return emit(AS_Oper(String("blt `s1, `s0, `j0\n"), 
                        NULL, TTL(s1, TTL(s0, NULL)), 
                        AS_Targets(TLL(cjump->u.CJUMP.l_true, NULL))));
        case T_ge:
            return emit(AS_Oper(String("bge `s0, `s1, `j0\n"), 
                        NULL, TTL(s0, TTL(s1, NULL)), 
                        AS_Targets(TLL(cjump->u.CJUMP.l_true, NULL))));
        default: assert(0);
    }
}

static void 
munchStm(T_stm s) {
    switch (s->kind) {
        case T_SEQ: {
            munchStm(s->u.SEQ.left);
            munchStm(s->u.SEQ.right);
            return ;
        }
        case T_LABEL: {
            emit(AS_Label(Temp_labelstring(s->u.LABEL), s->u.LABEL));
            return ;
        }
        case T_JUMP: {
            /*
             * Normally, we only have 
             * `T_Jump(T_Name(l), Temp_LabelList(l, NULL))`
             */
            emit(AS_Oper(String("j `j0\n"), 
                        NULL, NULL, 
                        AS_Targets(s->u.JUMP.jumps)));
            return ;
        }
        case T_CJUMP: return munchCjumpStm(s);
        case T_MOVE: return munchMoveStm(s);
        case T_EXP: {
            munchExp(s->u.EXP);
            return ;
        }
        default: assert(0);
    }
}

static Temp_temp
munchBinopExp(T_exp e) {
    char buf[100];
    T_exp left = e->u.BINOP.left;
    T_exp right = e->u.BINOP.right;
    Temp_temp d0 = Temp_newtemp();

    if (e->u.BINOP.op == T_plus) {
        if (left->kind == T_CONST) {
            sprintf(buf, "addi `d0, `s0, %d\n", left->u.CONST);
            emit(AS_Oper(String(buf), 
                        TTL(d0, NULL), TTL(munchExp(right), NULL), NULL)); 
        } else if (right->kind == T_CONST) {
            sprintf(buf, "addi `d0, `s0, %d\n", right->u.CONST);
            emit(AS_Oper(String(buf), 
                        TTL(d0, NULL), TTL(munchExp(left), NULL), NULL)); 
        }
        return d0;
    }

    Temp_temp s0 = munchExp(left);
    Temp_temp s1 = munchExp(right);
    switch (e->u.BINOP.op) {
        case T_plus:
            emit(AS_Oper(String("add `d0, `s0, `s1\n"), 
                        TTL(d0, NULL), TTL(s0, TTL(s1, NULL)), 
                        NULL));
            return d0;
        case T_minus:
            emit(AS_Oper(String("sub `d0, `s0, `s1\n"), 
                        TTL(d0, NULL), TTL(s0, TTL(s1, NULL)), 
                        NULL));
            return d0;
        case T_mul:
            emit(AS_Oper(String("mul `d0, `s0, `s1\n"), 
                        TTL(d0, NULL), TTL(s0, TTL(s1, NULL)), 
                        NULL));
            return d0;
        case T_div:
            emit(AS_Oper(String("div`d0, `s0, `s1\n"), 
                        TTL(d0, NULL), TTL(s0, TTL(s1, NULL)), 
                        NULL));
            return d0;
        default: assert(0);
    }

}

static Temp_temp 
munchMemExp(T_exp e) {
    Temp_temp d0 = Temp_newtemp();
    Temp_temp s0 = munchExp(e->u.MEM);
    emit(AS_Oper(String("ld `d0, 0(`s0`)\n"), 
                TTL(d0, NULL), TTL(s0, NULL), 
                NULL));

    return d0;
}

static Temp_temp munchExp(T_exp e) {
    switch (e->kind) {
        case T_BINOP: return munchBinopExp(e);
        case T_MEM: return munchMemExp(e);
        case T_TEMP: break;
        case T_NAME: break;
        case T_CONST: break;
        case T_CALL: break;

        /* After `canon` will not have `ESEQ` */
        case T_ESEQ:
        default: assert(0);
    }
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
