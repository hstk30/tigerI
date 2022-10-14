#ifndef CODEGEN_H__
#define CODEGEN_H__

#include "assem.h"
#include "frame.h"
#include "temp.h"
#include "tree.h"

AS_instrList F_codegen(F_frame f, T_stmList stmList);

#endif 
