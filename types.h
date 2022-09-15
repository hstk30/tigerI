/*
 * types.h - 
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

/*
 * 类型环境中的绑定值，根据抽象语法转换为此
 */

#ifndef TYPES_H_
#define TYPES_H_

#include "symbol.h"

typedef struct Ty_ty_ *Ty_ty;
typedef struct Ty_tyList_ *Ty_tyList;
typedef struct Ty_field_ *Ty_field;
typedef struct Ty_fieldList_ *Ty_fieldList;

struct Ty_ty_ {
    enum {
        Ty_int, Ty_string, Ty_nil, Ty_void, /* 内置 */
        Ty_record, Ty_array, Ty_name
    } kind;
    union {
        Ty_fieldList record;
        Ty_ty array;
        struct {
            S_symbol sym; 
            Ty_ty ty;
        } name; /* 递归类型声明时用于占位 */
    } u;
};

struct Ty_tyList_ {
    Ty_ty head; 
    Ty_tyList tail;
};

struct Ty_field_ {
    S_symbol name; 
    Ty_ty ty;
};

struct Ty_fieldList_ {
    Ty_field head; 
    Ty_fieldList tail;
};

Ty_ty Ty_Nil(void);
Ty_ty Ty_Int(void);
Ty_ty Ty_String(void);
Ty_ty Ty_Void(void);

Ty_ty Ty_Record(Ty_fieldList fields);   /* {id: type-id, ...} */
Ty_ty Ty_Array(Ty_ty ty);   /* array of type-id */
Ty_ty Ty_Name(S_symbol sym, Ty_ty ty);  /* 递归类型定义，作为占位符 */

Ty_tyList Ty_TyList(Ty_ty head, Ty_tyList tail);
Ty_field Ty_Field(S_symbol name, Ty_ty ty);
Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail);

void Ty_print(Ty_ty t);
void TyList_print(Ty_tyList list);

#endif
