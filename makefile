LEX_OBJECTS = lex.yy.o errormsg.o util.o 
ABSYN_OBJECTS = $(LEX_OBJECTS) y.tab.o parse.o absyn.o table.o symbol.o 

lextest: lextest.o $(LEX_OBJECTS)
	cc -o $@ lextest.o $(LEX_OBJECTS)

prabsyn: prabsyn.o $(ABSYN_OBJECTS)
	cc -o $@ prabsyn.o $(ABSYN_OBJECTS)

errormsg.o: errormsg.c errormsg.h util.h
util.o: util.c util.h
absyn.o: absyn.c absyn.h util.h symbol.h
prabsyn.o: prabsyn.c prabsyn.h util.h absyn.h
symbol.o: symbol.c symbol.h util.h table.h
table.o: table.c table.h util.h
parse.o: parse.c parse.h util.h errormsg.h symbol.h absyn.h
y.tab.o: y.tab.c 

y.tab.c: tiger.y
	yacc -dv tiger.y

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

lextest.o: lextest.c absyn.h symbol.h y.tab.h errormsg.h util.h
lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h
lex.yy.c: tiger.lex
	lex tiger.lex

clean: 
	rm -f a.out *.o
