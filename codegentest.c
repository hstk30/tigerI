#include <stdio.h>
#include <stdlib.h>

#include "frame.h"
#include "printtree.h"
#include "temp.h"
#include "tree.h"
#include "util.h"
#include "errormsg.h"
#include "parse.h"
#include "absyn.h"
#include "symbol.h"
#include "escape.h"
#include "semant.h"
#include "canon.h"
#include "codegen.h"
#include "assem.h"


static void
gen_as_proc(FILE *out, F_frag proc) {
    T_stmList sl = C_linearize(proc->u.proc.body);
    struct C_block b = C_basicBlocks(sl);
    T_stmList canon_stm = C_traceSchedule(b);
    printStmList(stdout, canon_stm);

    AS_instrList instr_codes = F_codegen(proc->u.proc.frame, canon_stm);

    /* two layer `temp map`, first search in `F_tempMap` for register, then temp */
    AS_printInstrList(out, instr_codes, Temp_layerMap(F_tempMap, Temp_name()));
}

static void 
gen_as_strs(FILE *out, F_fragList str_frags) {
    F_fragList iter;

    for (iter = str_frags; iter; iter = iter->tail) {
        fprintf(out, "%s:\n", Temp_labelstring(iter->head->u.stringg.label));
        fprintf(out, "\t.asciz \"%s\"\n\n", iter->head->u.stringg.str);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr,"usage: a.out filename\n"); 
        exit(1); 
    }

    F_initMap();
    A_exp absyn_tree_root = parse(argv[1]);
    if (absyn_tree_root) {
        Esc_findEscape(absyn_tree_root);
        F_fragList procs = SEM_transProg(absyn_tree_root);
        F_fragList iter;
        for (iter = procs; iter; iter = iter->tail) {
            if (iter->head->kind == F_procFrag) {
                gen_as_proc(stdout, iter->head);
            } else {
                break;
            }
        }
        gen_as_strs(stdout, iter);
    } else {
        fprintf(stderr, "parsing failed!\n");
        return 1;
    }
    return 0;
}
