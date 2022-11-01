CC=cc
CFLAGS=-g -c -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast 

ifeq ($(TG_DEBUG), TRUE)
	CFLAGS+=-D TG_DEBUG
endif

LEX_OBJECTS = lex.yy.o errormsg.o util.o 
ABSYN_OBJECTS = $(LEX_OBJECTS) y.tab.o parse.o absyn.o table.o symbol.o 
SEMANT_OBJECTS = $(ABSYN_OBJECTS) env.o types.o escape.o \
					 temp.o tree.o translate.o frame.o semant.o canon.o
CODEGEN_OBJECTS = $(SEMANT_OBJECTS) assem.o codegen.o 
					 
lextest: $(LEX_OBJECTS) lextest.o 
	$(CC) -o $@ lextest.o $(LEX_OBJECTS)

asttest: $(ABSYN_OBJECTS) prabsyn.o asttest.o
	$(CC) -o $@ prabsyn.o asttest.o $(ABSYN_OBJECTS)

semanttest: $(SEMANT_OBJECTS) semanttest.o  assem.o printtree.o
	$(CC) -o $@ semanttest.o printtree.o assem.o $(SEMANT_OBJECTS) 

codegentest: $(CODEGEN_OBJECTS) codegentest.o printtree.o
	$(CC) -o $@ codegentest.o printtree.o $(CODEGEN_OBJECTS) 

ISA=AARCH64
FRAME=aarch64_frame.c
CODEGEN=aarch64_codegen.c

ifeq ($(ISA), RV64)
	FRAME=rv64_frame.c
	CODEGEN=rv64_codegen.c
endif 

frame.o: $(FRAME)
	$(CC) $(CFLAGS) $(FRAME) -o $@ 

codegen.o: $(CODEGEN)
	$(CC) $(CFLAGS) $(CODEGEN) -o $@

lextest.o: lextest.c 
prabsyn.o: prabsyn.h 
asttest.o: asttest.c
semanttest.o: semanttest.c 
codegentest.o: codegentest.c

y.tab.c: tiger.y
	yacc -dv tiger.y

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

lex.yy.o: y.tab.h errormsg.h util.h
lex.yy.c: tiger.lex
	lex tiger.lex

.PHONY: all clean

all: lextest asttest semanttest codegentest

clean: 
	rm -f a.out *.o y.tab.c y.tab.h lex.yy.c y.output \
		lextest asttest semanttest codegentest

