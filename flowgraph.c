#include <assert.h>
#include <stdio.h>

#include "assem.h"
#include "graph.h"
#include "flowgraph.h"
#include "temp.h"
#include "table.h"


typedef struct TAB_table_ *LN_table;
static LN_table LN_empty(void)
{
    return TAB_empty();
}

static void LN_enter(LN_table t, Temp_label k, G_node v)
{
    TAB_enter(t, k, v);
}

static G_node LN_look(LN_table t, Temp_label k)
{
    return TAB_look(t, k);
}

static void show_cfg_info(FILE *out, void *info)
{
    AS_instr instr = (AS_instr) info;
    switch (instr->kind) {
        case I_OPER:
            fprintf(out, "%s -> ", instr->u.OPER.assem);
            break;
        case I_LABEL:
            fprintf(out, "%s -> ", instr->u.LABEL.assem);
            break;
        case I_MOVE:
            fprintf(out, "%s -> ", instr->u.MOVE.assem);
            break;
        default:
            assert(0);
    }
}

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
    AS_instrList cur_il;
    G_graph cfg = G_Graph();
    LN_table label2node = LN_empty();
    AS_instr info;
    G_node cur_node, prev_node, jump_dst;
    G_nodeList cur_nl;
    Temp_labelList jumps;

    for (cur_il = instr_list; cur_il; cur_il = cur_il->tail) {
        info = cur_il->head;
        cur_node = G_Node(cfg, info);
        if (info->kind == I_LABEL)
            LN_enter(label2node, info->u.LABEL.label, cur_node);
    }

    for (cur_nl = G_nodes(cfg); cur_nl; cur_nl = cur_nl->tail) {
        cur_node = cur_nl->head;
        G_addEdge(prev_node, cur_node);
        info = G_nodeInfo(cur_node);
        if (info->kind == I_OPER && info->u.OPER.jumps) {
            for (jumps = info->u.OPER.jumps->labels; jumps; jumps = jumps->tail) {
                jump_dst = LN_look(label2node, jumps->head);
                G_addEdge(cur_node, jump_dst);
            }
        }
        prev_node = cur_node;
    }

#ifdef TG_DEBUG
    G_show(stdout, G_nodes(cfg), show_cfg_info);
#endif
    return cfg;
}

