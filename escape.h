/* 
 * 如果一个变量是传地址实参，或者它被取了地址，
 * 或者内层的嵌套函数对其进行了访问，
 * 则称该变量是*逃逸的*(escape)
 * *逃逸的* 的变量一样要分配在栈帧中
 */

#ifndef ESCAPE_H_
#define ESCAPE_H_

#include "absyn.h"

void Esc_findEscape(A_exp exp);

#endif
