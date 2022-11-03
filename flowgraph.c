#include "flowgraph.h"
#include "assem.h"
#include "graph.h"
#include "temp.h"
#include <assert.h>


Temp_tempList FG_def(G_node n)
{
    AS_instr instr = G_nodeInfo(n);
    switch (instr->kind) {
        case I_MOVE:
            return instr->u.MOVE.dst;
        case I_OPER:
            return instr->u.OPER.dst;
        case I_LABEL:
            return NULL;
        default:
            assert(0);
    }
}

Temp_tempList FG_use(G_node n)
{
    AS_instr instr = G_nodeInfo(n);
    switch (instr->kind) {
        case I_MOVE:
            return instr->u.MOVE.src;
        case I_OPER:
            return instr->u.OPER.src;
        case I_LABEL:
            return NULL;
        default:
            assert(0);
    }
}

bool FG_isMove(G_node n)
{
    AS_instr instr = G_nodeInfo(n);
    return instr->kind == I_MOVE;
}

G_graph FG_AssemFlowGraph(AS_instrList instr_list)
{

}

