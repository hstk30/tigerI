/*
 * absyn.h - Abstract Syntax Header (Chapter 4)
 *
 * All types and functions declared in this header file begin with "A_"
 * Linked list types end with "..list"
 */

/* Type Definitions */

#ifndef ABSYN_H_
#define ABSYN_H_

#include "symbol.h"
#include "util.h"

typedef int A_pos;

typedef struct A_var_ *A_var;
typedef struct A_exp_ *A_exp;
typedef struct A_dec_ *A_dec;
typedef struct A_ty_ *A_ty;

typedef struct A_decList_ *A_decList;
typedef struct A_expList_ *A_expList;
typedef struct A_field_ *A_field;
typedef struct A_fieldList_ *A_fieldList;
typedef struct A_fundec_ *A_fundec;
typedef struct A_fundecList_ *A_fundecList;
typedef struct A_namety_ *A_namety;
typedef struct A_nametyList_ *A_nametyList;
typedef struct A_efield_ *A_efield;
typedef struct A_efieldList_ *A_efieldList;

typedef enum {
    A_plusOp, A_minusOp, A_timesOp, A_divideOp,
    A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp
} A_oper;

/*
 * lvalue   -> id
 *          -> lvalue.id
 *          -> lvalue[exp]
 */
struct A_var_ {
    enum {
        A_simpleVar, 
        A_fieldVar, 
        A_subscriptVar
    } kind;
    A_pos pos;
    union {
        S_symbol simple;    
        struct {
            A_var var;   
            S_symbol sym;
        } field;  
        struct {
            A_var var;
            A_exp exp;
        } subscript; 
    } u;
};

/*
 * exp  -> lvalue
 *      -> nil
 *      -> (expseq)
 *      -> int_literal
 *      -> string_literal
 *      -> - exp
 *      -> id(args)
 *
 *      -> exp arith_op exp
 *      -> exp cmp_op exp
 *      -> exp bool_op exp
 *
 *      -> type-id\{record_init\}
 *      -> type-id\[exp\] of exp
 *      -> lvalue := exp
 *
 *      -> if exp then exp else exp
 *      -> if exp then exp
 *      -> while exp do exp
 *      -> for id := exp to exp do exp
 *      -> break
 *      -> let decs in expseq end
 *
 * args     -> 𝜖
 *          -> args_no_empty
 * args_no_empty    -> exp
 *                  -> args_no_empty, exp
 *
 * expseq   -> 𝜖
 *          -> expseq_no_empty
 * expseq_no_empty  -> exp
 *                  -> expseq_no_empty; exp
 */

struct A_exp_ {
    enum {
        A_varExp, A_nilExp, A_intExp, A_stringExp, A_callExp,
        A_opExp, A_recordExp, A_arrayExp, A_seqExp, A_assignExp, 
        A_ifExp, A_whileExp, A_forExp, A_breakExp, A_letExp 
    } kind;
    A_pos pos;
    union {
        A_var var;    /* a */
        /* nil; - needs only the pos and kind */
        int intt;   /* 1 */
        string stringg; /* "123" */ 
        struct {S_symbol func; A_expList args;} call; /* func(args) */
        struct {A_oper oper; A_exp left; A_exp right;} op;    /* a + b */
        struct {S_symbol typ; A_efieldList fields;} record;   /* tree = {key: int, value: int} */
        struct {S_symbol typ; A_exp size, init;} array;   /* type[size] of init */
        A_expList seq;    /* (exp; ...) */
        struct {A_var var; A_exp exp;} assign;    /* var a := exp */
        /* if test then thenn else elsee */
        struct {A_exp test, then, elsee;} iff; /* elsee is optional */
        struct {A_exp test, body;} whilee;    /* while test do body */
        struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr;   /* for var := lo to hi do body */
        /* breakk; - need only the pos */
        struct {A_decList decs; A_exp body;} let; /* let decs in body end */
    } u;
};

/* 
 * decs -> 𝜖
 *      -> decs dec
 * dec  -> tydec
 *      -> vardec
 *      -> fundec
 *
 * tydec    -> type type-id = ty
 * ty       -> type-id
 *          -> \{tyfields\}
 *          -> array of type-id
 * tyfields -> 𝜖
 *          -> tyfields_no_empty
 * tyfields_no_empty    -> id: type-id
 *                      -> tyfields_no_empty, id: type-id
 *
 * vardec   -> var id := exp
 *          -> var id: type-id := exp
 *
 * fundec   -> function id(tyfields) = exp
 *          -> function id(tyfields): type-id = exp
 */

struct A_dec_ {
    enum {
        A_functionDec, 
        A_varDec, 
        A_typeDec
    } kind;
    A_pos pos;
    union {
        A_fundecList function;
        /* escape may change after the initial declaration */
        struct {S_symbol var; S_symbol typ; A_exp init; bool escape;} var; 
        A_nametyList type;
    } u;
};

struct A_ty_ {
    enum {
        A_nameTy, 
        A_recordTy, 
        A_arrayTy
    } kind;
    A_pos pos;
    union {
        S_symbol name;  /* type type_id = ty */
        A_fieldList record; /* { tyfields } */
        S_symbol array; /* array of type_id */
    } u;
};

struct A_field_ {
    S_symbol name, typ; 
    A_pos pos; 
    bool escape;
};
struct A_fieldList_ {
    A_field head; 
    A_fieldList tail;
};
struct A_expList_ {
    A_exp head; 
    A_expList tail;
};
struct A_fundec_ {
    A_pos pos;
    S_symbol name; 
    A_fieldList params; 
    S_symbol result; 
    A_exp body;
};

struct A_fundecList_ {A_fundec head; A_fundecList tail;};
struct A_decList_ {A_dec head; A_decList tail;};
struct A_namety_ {S_symbol name; A_ty ty;};
struct A_nametyList_ {A_namety head; A_nametyList tail;};
struct A_efield_ {S_symbol name; A_exp exp;};
struct A_efieldList_ {A_efield head; A_efieldList tail;};


/* Function Prototypes */
A_var A_SimpleVar(A_pos pos, S_symbol sym);
A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym);
A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp);

A_exp A_VarExp(A_pos pos, A_var var);
A_exp A_NilExp(A_pos pos);
A_exp A_IntExp(A_pos pos, int i);
A_exp A_StringExp(A_pos pos, string s);
A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args);
A_exp A_OpExp(A_pos pos, A_oper oper, A_exp left, A_exp right);
A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields);
A_exp A_SeqExp(A_pos pos, A_expList seq);
A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp);
A_exp A_IfExp(A_pos pos, A_exp test, A_exp then, A_exp elsee);
A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body);
A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body);
A_exp A_BreakExp(A_pos pos);
A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body);
A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init);

A_dec A_FunctionDec(A_pos pos, A_fundecList function);
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init);
A_dec A_TypeDec(A_pos pos, A_nametyList type);
A_decList A_DecList(A_dec head, A_decList tail);

A_ty A_NameTy(A_pos pos, S_symbol name);
A_ty A_RecordTy(A_pos pos, A_fieldList record);
A_ty A_ArrayTy(A_pos pos, S_symbol array);

A_field A_Field(A_pos pos, S_symbol name, S_symbol typ);
A_fieldList A_FieldList(A_field head, A_fieldList tail);

A_expList A_ExpList(A_exp head, A_expList tail);

A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result,
		  A_exp body);
A_fundecList A_FundecList(A_fundec head, A_fundecList tail);

A_namety A_Namety(S_symbol name, A_ty ty);
A_nametyList A_NametyList(A_namety head, A_nametyList tail);

A_efield A_Efield(S_symbol name, A_exp exp);
A_efieldList A_EfieldList(A_efield head, A_efieldList tail);

#endif
