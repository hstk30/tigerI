LEX_OBJECTS = lex.yy.o errormsg.o util.o

lextest.o: lextest.c tokens.h errormsg.h util.h
lex.yy.o: lex.yy.c tokens.h errormsg.h util.h
errormsg.o: errormsg.c errormsg.h util.h
util.o: util.c util.h

parsetest.o: parsetest.c errormsg.h util.h
	cc -g -c parsetest.c

y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.grm
	yacc -dv tiger.grm

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

lextest: lextest.o $(LEX_OBJECTS)
	cc -o $@ lextest.o $(LEX_OBJECTS)

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h
	cc -g -c lex.yy.c

lex.yy.c: tiger.lex
	lex tiger.lex

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f a.out util.o parsetest.o lex.yy.o errormsg.o y.tab.c y.tab.h y.tab.o
