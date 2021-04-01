all:parser

parser:main.c syntax.tab.c lex.yy.c
	gcc main.c syntax.tab.c -lfl -ly -o parser

syntax.tab.c:syntax.y
	bison -d syntax.y

scanner:lex.yy.c main.c
	gcc main.c lex.yy.c -lfl -o scanner
	
lex.yy.c:lexical.l
	flex lexical.l

.phony:clean

clean:
	rm parser scanner lex.yy.c syntax.tab.c syntax.tab.h