#include <stdio.h>
#include "semant.h"
#include "types.h"
#include "errormsg.h"

typedef void *Tr_exp;

struct expty {
    Tr_exp exp;
    Ty_ty ty;
};
struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty transTy(S_table tenv, A_ty a);



/* 
 * 遇到变量的时候检查变量是否已经声明
 */
struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
    }

}

/* 
 * 遇到表达式的时候检查表达式的类型是否一致
 */
struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    switch (a->kind) {
        case A_opExp: {
              A_oper oper = a->u.op.oper;
              struct expty left = transExp(venv, tenv, a->u.op.left);
              struct expty right = transExp(venv, tenv, a->u.op.right);
              if (oper == A_plusOp) {
                  if (left.ty->kind != Ty_int) 
                      EM_error(a->u.op.left->pos, "integer required");
                  if (right.ty->kind != Ty_int) 
                      EM_error(a->u.op.right->pos, "integer required");
                  return expTy(NULL, Ty_Int());
              }
          }
        /* TODO */
    }

    assert(0);
}

/* 
 * 遇到声明的时候扩充值环境和类型环境
 */
void transDec(S_table venv, S_table tenv, A_dec d) {

}

/* 
 * 遇到类型声明的时候将抽象语法中的类型表达式A_ty 
 * 转换为类型描述Ty_ty，放入类型环境中
 */

Ty_ty transTy(S_table tenv, A_ty a) {

}

