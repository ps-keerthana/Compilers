%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"

extern int yylineno;

/* Enable token printing  */
#define PRINT_TOKEN(type) \
    printf("Line %-4d  %-18s  %s\n", yylineno, type, yytext);

%}

%option noyywrap
%option yylineno

%x BLOCK_COMMENT

 /* Pattern Definitions */

DIGIT        [0-9]
NONZERO      [1-9]
LETTER       [_a-zA-Z]
ALNUM        [_a-zA-Z0-9]

HEX_CONST    0[xX][0-9a-fA-F]+
INT_CONST    (0|{NONZERO}{DIGIT}*)

FLOAT_CONST ({DIGIT}+\.{DIGIT}*([eE][+-]?{DIGIT}+)?|{DIGIT}*\.{DIGIT}+([eE][+-]?{DIGIT}+)?|{DIGIT}+[eE][+-]?{DIGIT}+)

ID           {LETTER}{ALNUM}*
ESC          (\\[nrtbfav\\\'\"\?0])

%%

 /* Whitespace */
[ \t\r\n]+        ;

 /* Single-line comment */
"//"[^\n]*        ;

 /*  Multi-line comment  */
"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>.|\n     ;
<BLOCK_COMMENT><<EOF>>  {
    printf("***lexing terminated*** [lexer error]: invalid token\n");
    exit(1);
}

 /*  Preprocessors NOT allowed  */
"#".* {
    printf("***lexing terminated*** [lexer error]: invalid token\n");
    exit(1);
}

 /*  Keywords  */
"int"       { PRINT_TOKEN("KEYWORD"); return INT; }
"float"     { PRINT_TOKEN("KEYWORD"); return FLOAT; }
"char"      { PRINT_TOKEN("KEYWORD"); return CHAR; }
"void"      { PRINT_TOKEN("KEYWORD"); return VOID; }
"double"    { PRINT_TOKEN("KEYWORD"); return DOUBLE; }
"short"     { PRINT_TOKEN("KEYWORD"); return SHORT; }
"long"      { PRINT_TOKEN("KEYWORD"); return LONG; }

"if"        { PRINT_TOKEN("KEYWORD"); return IF; }
"else"      { PRINT_TOKEN("KEYWORD"); return ELSE; }
"for"       { PRINT_TOKEN("KEYWORD"); return FOR; }
"while"     { PRINT_TOKEN("KEYWORD"); return WHILE; }
"do"        { PRINT_TOKEN("KEYWORD"); return DO; }

"switch"    { PRINT_TOKEN("KEYWORD"); return SWITCH; }
"case"      { PRINT_TOKEN("KEYWORD"); return CASE; }
"default"   { PRINT_TOKEN("KEYWORD"); return DEFAULT; }

"break"     { PRINT_TOKEN("KEYWORD"); return BREAK; }
"continue"  { PRINT_TOKEN("KEYWORD"); return CONTINUE; }
"return"    { PRINT_TOKEN("KEYWORD"); return RETURN; }

"struct"    { PRINT_TOKEN("KEYWORD"); return STRUCT; }
"typedef"   { PRINT_TOKEN("KEYWORD"); return TYPEDEF; }
"enum"      { PRINT_TOKEN("KEYWORD"); return ENUM; }
"union"     { PRINT_TOKEN("KEYWORD"); return UNION; }

"sizeof"    { PRINT_TOKEN("KEYWORD"); return SIZEOF; }

"auto"      { PRINT_TOKEN("KEYWORD"); return AUTO; }
"const"     { PRINT_TOKEN("KEYWORD"); return CONST; }
"static"    { PRINT_TOKEN("KEYWORD"); return STATIC; }
"extern"    { PRINT_TOKEN("KEYWORD"); return EXTERN; }
"register"  { PRINT_TOKEN("KEYWORD"); return REGISTER; }

 /*  Identifiers  */
{ID} {
    PRINT_TOKEN("IDENTIFIER");
    return IDENTIFIER;
}

 /*  Numeric Literals (ORDER MATTERS)  */

 /* Hex must come before INT_CONST */
{HEX_CONST} {
    PRINT_TOKEN("INT_CONST");
    return INT_CONST;
}

{FLOAT_CONST} {
    PRINT_TOKEN("FLOAT_CONST");
    return FLOAT_CONST;
}

{INT_CONST} {
    PRINT_TOKEN("INT_CONST");
    return INT_CONST;
}

 /*  Character Literal  */
\'({ESC}|[^\'\\])\' {
    PRINT_TOKEN("CHAR_CONST");
    return CHAR_CONST;
}

 /*  String Literal  */
\"([^\"\\]|{ESC})*\" {
    PRINT_TOKEN("STRING_LITERAL");
    return STRING_LITERAL;
}

 /*  Multi-character Operators (LONGEST FIRST)  */

"<<="   { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
">>="   { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"..."   { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

"=="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"!="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"<="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
">="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

"&&"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"||"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

"++"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"--"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

"+="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"-="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"*="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"/="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"%="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

"&="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"|="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
"^="    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

"<<"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }
">>"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }


"->"    { PRINT_TOKEN("OPERATOR"); return OPERATOR; }

 /*  Single-character Operators  */
[+\-*/%&|^~!<>=] {
    PRINT_TOKEN("OPERATOR");
    return OPERATOR;
}

 /*  Punctuation  */
[(){}\[\];,.:?] {
    PRINT_TOKEN("PUNCTUATION");
    return yytext[0];
}

 /*  Invalid Token  */
. {
    printf("***lexing terminated*** [lexer error]: invalid token\n");
    exit(1);
}

%%