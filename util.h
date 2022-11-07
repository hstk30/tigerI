#ifndef UTIL_H_
#define UTIL_H_

#include <assert.h>

typedef char *string;
typedef char bool;

#define TRUE 1
#define FALSE 0

void *Malloc(size_t);
#define checked_malloc Malloc
void *Calloc(size_t, size_t);
void *Realloc(void *ptr, size_t size);
void Free(void *ptr);

string String(char *);

typedef struct U_boolList_ *U_boolList;
struct U_boolList_ {
    bool head; 
    U_boolList tail;
};
U_boolList U_BoolList(bool head, U_boolList tail);

#endif
