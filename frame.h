/*
 * Frame interface, should be irrelevant with machine
 * implement certain machine relevant frame in xxxframe.c
 */

#ifndef FRAME_H_
#define FRAME_H_

#include "util.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"

/* set in xxframe.c */
extern const int F_wordSize;

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;
struct F_accessList_{
    F_access head;
    F_accessList tail;
};
F_accessList F_AccessList(F_access head, F_accessList tail);

F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);

string F_getlabel(F_frame frame);   /* ?? */

/*** translate interface ***/
T_exp F_externalCall(string s, T_expList args);
T_exp F_Exp(F_access access, T_exp frame_ptr);

/*** end ***/

typedef struct F_frag_ *F_frag;
struct F_frag_ {
    enum {
        F_stringFrag, F_procFrag
    } kind;
    union {
        struct {Temp_label label; string str;} stringg;
        struct {T_stm body; F_frame frame;} proc;
    } u;
};
F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

F_frag F_string(Temp_label lab, string str);    /* ?? */
F_frag F_newProcFrag(T_stm body, F_frame frame);    /* ?? */

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {
    F_frag head;
    F_fragList tail;
};
F_fragList F_FragList(F_frag head, F_fragList tail);

Temp_map F_tempMap();
Temp_tempList F_registers(void);

Temp_temp F_FP();
Temp_temp F_SP();
Temp_temp F_ZERO();
Temp_temp F_RA();
Temp_temp F_RV();

T_stm        F_procEntryExit1(F_frame frame, T_stm stm);
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc      F_procEntryExit3(F_frame frame, AS_instrList body);

AS_instrList F_codegen(F_frame f, T_stmList stmList);


void F_print(F_frame);

#endif
