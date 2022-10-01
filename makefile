CC=cc
CFLAGS=-g -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast

LEX_OBJECTS = lex.yy.o errormsg.o util.o 
ABSYN_OBJECTS = $(LEX_OBJECTS) y.tab.o parse.o absyn.o table.o symbol.o 
TYPE_CHECK_OBJECTS = $(ABSYN_OBJECTS) env.o types.o escape.o \
					 temp.o translate.o riscvframe.o semant.o 

lextest: $(LEX_OBJECTS) lextest.o 
	$(CC) -o $@ lextest.o $(LEX_OBJECTS)

asttest: $(ABSYN_OBJECTS) prabsyn.o asttest.o
	$(CC) -o $@ prabsyn.o asttest.o $(ABSYN_OBJECTS)

typetest: $(TYPE_CHECK_OBJECTS) typetest.o 
	$(CC) -o $@ typetest.o $(TYPE_CHECK_OBJECTS) 

y.tab.o: y.tab.c 
errormsg.o: errormsg.h util.h
util.o: util.h
absyn.o: absyn.h util.h symbol.h
symbol.o: symbol.h util.h table.h
table.o: table.h util.h
parse.o: parse.h util.h errormsg.h symbol.h absyn.h
env.o: env.h util.h symbol.h types.h 
types.o: types.h util.h symbol.h
semant.o: semant.h
escape.o: escape.h
temp.o: temp.h
translate.o: translate.h

riscvframe.o: frame.h riscvframe.c

lextest.o: lextest.c 
prabsyn.o: prabsyn.h 
asttest.o: asttest.c
typetest.o: typetest.c 

y.tab.c: tiger.y
	yacc -dv tiger.y

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

lex.yy.o: y.tab.h errormsg.h util.h
lex.yy.c: tiger.lex
	lex tiger.lex

.PHONY: all clean

all: lextest asttest typetest 

clean: 
	rm -f a.out *.o y.tab.c y.tab.h lex.yy.c y.output \
		lextest asttest typetest 

