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

struct expty tranVar(S_table venv, S_table tenv, A_var v);
struct expty tranExp(S_table venv, S_table tenv, A_exp a);
void transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty transTy(S_table tenv, A_ty a);



struct expty tranVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
    }

}

struct expty tranExp(S_table venv, S_table tenv, A_exp a) {
    switch (a->kind) {
        case A_opExp: {
              A_oper oper = a->u.op.oper;
              struct expty left = tranExp(venv, tenv, a->u.op.left);
              struct expty right = tranExp(venv, tenv, a->u.op.right);
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


