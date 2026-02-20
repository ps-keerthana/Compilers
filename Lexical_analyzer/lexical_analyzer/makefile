CC = gcc
CFLAGS = -w

all: parser

parser: lex.yy.c y.tab.c 
	$(CC) $(CFLAGS) lex.yy.c y.tab.c  -o parser

lex.yy.c: cl_lexer.l
	flex cl_lexer.l

y.tab.c y.tab.h: cl_parser.y
	yacc -d cl_parser.y

clean:
	rm -f lex.yy.c y.tab.c y.tab.h parser
	