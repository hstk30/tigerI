/*
 * Frame interface, should be irrelevant with machine
 * implement certain machine relevant frame in xxxframe.c
 */

#ifndef FRAME_H_
#define FRAME_H_

typedef struct F_frame *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;
struct F_accessList_{
    F_access head;
    F_accessList tail;
};
F_accessList_ F_accessList(F_access head, F_accessList tail);


typedef struct F_frag_ *F_frag;

#endif
