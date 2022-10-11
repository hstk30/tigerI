#ifndef TRANSLATE_H_
#define TRANSLATE_H_

#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "frame.h"

typedef struct Tr_level_ *Tr_level;
typedef struct Tr_access_ *Tr_access;
typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_ {
    Tr_access head;
    Tr_accessList tail;
};
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost();
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
Tr_access Tr_static_link(Tr_level level);
Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_expList_ *Tr_expList;
struct Tr_expList_ {
    Tr_exp head;
    Tr_expList tail;
};
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

void Tr_init();

Tr_exp Tr_nop();
Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);
Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp idx);
Tr_exp Tr_fieldVar(Tr_exp base, int nth);
Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_cmpExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp if_exp, Tr_exp then_exp, Tr_exp else_exp);
Tr_exp Tr_nilExp();
Tr_exp Tr_intExp(int n);
Tr_exp Tr_stringExp(string str);
Tr_exp Tr_recordExp(Tr_expList el, int nvals);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);
Tr_exp Tr_whileExp(Tr_exp test_exp, Tr_exp body_exp, Temp_label done);
Tr_exp Tr_forExp(Tr_exp lo_exp, Tr_exp hi_exp, Tr_exp body_exp, Temp_label done);
Tr_exp Tr_breakExp(Temp_label done);
Tr_exp Tr_callExp(Temp_label func_name, Tr_expList args);
Tr_exp Tr_assignExp(Tr_exp var_exp, Tr_exp val_exp);
Tr_exp Tr_seqExp(Tr_expList rev_stm_seq);

void Tr_procEntryExit(Tr_level level, Tr_exp proc_exp);
F_fragList Tr_getResult(void);

void Tr_printLevel(Tr_level level);
void Tr_printTree(Tr_exp proc_exp);

#endif
