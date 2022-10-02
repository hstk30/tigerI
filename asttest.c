#include <stdio.h>

#include "absyn.h" 
#include "prabsyn.h" 
#include "parse.h" 


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr,"usage: a.out filename\n"); 
        return(1);
    }
    A_exp absyn_tree_root = parse(argv[1]);
    if (absyn_tree_root) {
        pr_exp(stdout, absyn_tree_root, 0);
    }
    else {
        fprintf(stderr, "parsing failed!\n");
        return 1;
    }
    return 0;
}

