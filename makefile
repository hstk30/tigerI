CC=cc
CFLAGS=-Wno-pointer-to-int-cast -Wno-int-to-pointer-cast

LEX_OBJECTS = lex.yy.o errormsg.o util.o 
ABSYN_OBJECTS = $(LEX_OBJECTS) y.tab.o parse.o absyn.o table.o symbol.o 
TYPE_CHECK_OBJECTS = $(ABSYN_OBJECTS) env.o types.o semant.o

lextest: lextest.o $(LEX_OBJECTS)
	$(CC) -o $@ lextest.o $(LEX_OBJECTS)

prabsyn: prabsyn.o $(ABSYN_OBJECTS)
	$(CC) -o $@ prabsyn.o $(ABSYN_OBJECTS)

typecheck: typecheck.o $(TYPE_CHECK_OBJECTS)
	$(CC) -o $@ typecheck.o $(TYPE_CHECK_OBJECTS)

y.tab.o: y.tab.c 
errormsg.o: errormsg.h util.h
util.o: util.h
absyn.o: absyn.h util.h symbol.h
symbol.o: symbol.h util.h table.h
table.o: table.h util.h
parse.o: parse.h util.h errormsg.h symbol.h absyn.h
env.o: env.h util.h symbol.h types.h 
types.o: types.h utils.h symbol.h
semant.o: semant.h

lextest.o: lextest.c absyn.h symbol.h y.tab.h errormsg.h util.h
prabsyn.o: prabsyn.h util.h absyn.h
typecheck.o: type_check.c

y.tab.c: tiger.y
	yacc -dv tiger.y

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

lex.yy.o: y.tab.h errormsg.h util.h
lex.yy.c: tiger.lex
	lex tiger.lex

clean: 
	rm -f a.out *.o y.tab.c y.tab.h lex.yy.c y.output lextest prabsyn typecheck

