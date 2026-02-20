%{
#include <stdio.h>
#include <stdlib.h>

extern int yylineno;
extern FILE *yyin;

void yyerror(const char *s)
{
    printf("***parsing terminated*** [syntax error]\n");
    exit(1);
}
%}

%token INT FLOAT CHAR VOID DOUBLE SHORT LONG
%token IF ELSE FOR WHILE DO SWITCH CASE DEFAULT
%token BREAK CONTINUE RETURN STRUCT TYPEDEF ENUM UNION
%token SIZEOF AUTO CONST STATIC EXTERN REGISTER
%token IDENTIFIER
%token INT_CONST FLOAT_CONST CHAR_CONST STRING_LITERAL
%token OPERATOR

%%

program:
    token_list
    ;

token_list:
      token_list token
    | /* empty */
    ;

token:
      INT
    | FLOAT
    | CHAR
    | VOID
    | DOUBLE
    | SHORT
    | LONG
    | IF
    | ELSE
    | FOR
    | WHILE
    | DO
    | SWITCH
    | CASE
    | DEFAULT
    | BREAK
    | CONTINUE
    | RETURN
    | STRUCT
    | TYPEDEF
    | ENUM
    | UNION
    | SIZEOF
    | AUTO
    | CONST
    | STATIC
    | EXTERN
    | REGISTER
    | IDENTIFIER
    | INT_CONST
    | FLOAT_CONST
    | CHAR_CONST
    | STRING_LITERAL
    | OPERATOR
    | '(' | ')' | '{' | '}' | '[' | ']'
    | ';' | ',' | '.' | ':' | '?'
    ;


%%

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("***process terminated*** [input error]: invalid number of command-line arguments\n");
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin)
    {
        printf("***process terminated*** [input error]: no such file exists\n");
        return 1;
    }

    yyparse();

    printf("***parsing successful***\n");

    fclose(yyin);
    return 0;
}