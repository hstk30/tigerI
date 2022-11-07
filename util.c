/*
 * util.c - commonly used utility functions.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"


void *Malloc(size_t len)
{
    void *p = malloc(len);
    if (!p) {
        fprintf(stderr,"\nRan out of memory!\n");
        exit(1);
    }
    return p;
}

void *Realloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr,"\nRan out of memory!\n");
        exit(1);
    }
    return p;

}

void *Calloc(size_t cnt, size_t size)
{
    void *p = calloc(cnt, size);
    if (!p) {
        fprintf(stderr,"\nRan out of memory!\n");
        exit(1);
    }
    return p;
}

void Free(void *ptr)
{
    free(ptr);
}

string String(char *s)
{
    string p = Malloc(strlen(s)+1);
    strcpy(p,s);
    return p;
}

U_boolList U_BoolList(bool head, U_boolList tail)
{
    U_boolList list = Malloc(sizeof(*list));
    list->head = head;
    list->tail = tail;
    return list;
}
