%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "y.tab.h"

extern int yylineno;

/*
 * last_token: stores the text of the most recently lexed token.
 * Used by yyerror() in the parser to report which token caused
 * the syntax error.
 */
char last_token[256];

/* Helper: copy yytext into last_token safely */
#define SAVE_TOKEN strncpy(last_token, yytext, 255); last_token[255] = '\0';

%}

%option noyywrap
%option yylineno

%x BLOCK_COMMENT

 /* ─────────────────────────────────────────
    Pattern Definitions
    ───────────────────────────────────────── */

DIGIT        [0-9]
NONZERO      [1-9]
LETTER       [_a-zA-Z]
ALNUM        [_a-zA-Z0-9]

HEX_CONST    0[xX][0-9a-fA-F]+
INT_CONST    (0|{NONZERO}{DIGIT}*)

FLOAT_CONST  ({DIGIT}+\.{DIGIT}*([eE][+-]?{DIGIT}+)?|{DIGIT}*\.{DIGIT}+([eE][+-]?{DIGIT}+)?|{DIGIT}+[eE][+-]?{DIGIT}+)

ID           {LETTER}{ALNUM}*
ESC          (\\[nrtbfav\\\'\"\?0])

%%

 /* ─── Whitespace ────────────────────────── */
[ \t\r\n]+        ;

 /* ─── Single-line comment ───────────────── */
"//"[^\n]*        ;

 /* ─── Multi-line comment ────────────────── */
"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>.|\n     ;        /* BUG FIX: was .|\"\n\" (string literal) — must be .\|\n */
<BLOCK_COMMENT><<EOF>>  {
    fprintf(stderr, "***lexing terminated*** [lexer error]: unterminated block comment\n");
    exit(1);
}

 /* ─── Preprocessors NOT allowed ─────────── */
"#".* {
    fprintf(stderr, "***lexing terminated*** [lexer error]: preprocessor directives not allowed (line %d)\n", yylineno);
    exit(1);
}

 /* ─── Keywords ──────────────────────────── */
"int"       { SAVE_TOKEN; return INT; }
"float"     { SAVE_TOKEN; return FLOAT; }
"char"      { SAVE_TOKEN; return CHAR; }
"void"      { SAVE_TOKEN; return VOID; }
"double"    { SAVE_TOKEN; return DOUBLE; }
"short"     { SAVE_TOKEN; return SHORT; }
"long"      { SAVE_TOKEN; return LONG; }
"unsigned"  { SAVE_TOKEN; return UNSIGNED; }
"signed"    { SAVE_TOKEN; return SIGNED; }

"if"        { SAVE_TOKEN; return IF; }
"else"      { SAVE_TOKEN; return ELSE; }
"for"       { SAVE_TOKEN; return FOR; }
"while"     { SAVE_TOKEN; return WHILE; }
"do"        { SAVE_TOKEN; return DO; }

"switch"    { SAVE_TOKEN; return SWITCH; }
"case"      { SAVE_TOKEN; return CASE; }
"default"   { SAVE_TOKEN; return DEFAULT; }

"break"     { SAVE_TOKEN; return BREAK; }
"continue"  { SAVE_TOKEN; return CONTINUE; }
"return"    { SAVE_TOKEN; return RETURN; }

"struct"    { SAVE_TOKEN; return STRUCT; }
"typedef"   { SAVE_TOKEN; return TYPEDEF; }
"enum"      { SAVE_TOKEN; return ENUM; }
"union"     { SAVE_TOKEN; return UNION; }

"sizeof"    { SAVE_TOKEN; return SIZEOF; }

"auto"      { SAVE_TOKEN; return AUTO; }
"const"     { SAVE_TOKEN; return CONST; }
"volatile"  { SAVE_TOKEN; return VOLATILE; }
"static"    { SAVE_TOKEN; return STATIC; }
"extern"    { SAVE_TOKEN; return EXTERN; }
"register"  { SAVE_TOKEN; return REGISTER; }
"inline"    { SAVE_TOKEN; return INLINE; }

 /* ─── Identifiers ───────────────────────── */
{ID} {
    SAVE_TOKEN;
    yylval.str_val = strdup(yytext);
    return IDENTIFIER;
}

 /* ─── Numeric Literals (order matters!) ─── */

 /* Hex must come before plain INT_CONST */
{HEX_CONST} {
    SAVE_TOKEN;
    yylval.str_val = strdup(yytext);
    return INT_CONST;
}

{FLOAT_CONST} {
    SAVE_TOKEN;
    yylval.str_val = strdup(yytext);
    return FLOAT_CONST;
}

{INT_CONST} {
    SAVE_TOKEN;
    yylval.str_val = strdup(yytext);
    return INT_CONST;
}

 /* ─── Character Literal ─────────────────── */
\'({ESC}|[^\'\\])\' {
    SAVE_TOKEN;
    yylval.str_val = strdup(yytext);
    return CHAR_CONST;
}

 /* ─── String Literal ────────────────────── */
\"([^\"\\]|{ESC})*\" {
    SAVE_TOKEN;
    yylval.str_val = strdup(yytext);
    return STRING_LITERAL;
}

 /* ─── Multi-character Operators (LONGEST FIRST) ─ */
"<<="   { SAVE_TOKEN; return ASSIGN_OP; }
">>="   { SAVE_TOKEN; return ASSIGN_OP; }
"+="    { SAVE_TOKEN; return ASSIGN_OP; }
"-="    { SAVE_TOKEN; return ASSIGN_OP; }
"*="    { SAVE_TOKEN; return ASSIGN_OP; }
"/="    { SAVE_TOKEN; return ASSIGN_OP; }
"%="    { SAVE_TOKEN; return ASSIGN_OP; }
"&="    { SAVE_TOKEN; return ASSIGN_OP; }
"|="    { SAVE_TOKEN; return ASSIGN_OP; }
"^="    { SAVE_TOKEN; return ASSIGN_OP; }

"..."   { SAVE_TOKEN; return ELLIPSIS; }
"->"    { SAVE_TOKEN; return ARROW; }
"++"    { SAVE_TOKEN; return INC_OP; }
"--"    { SAVE_TOKEN; return DEC_OP; }
"<<"    { SAVE_TOKEN; return LEFT_SHIFT; }
">>"    { SAVE_TOKEN; return RIGHT_SHIFT; }
"<="    { SAVE_TOKEN; return LE_OP; }
">="    { SAVE_TOKEN; return GE_OP; }
"=="    { SAVE_TOKEN; return EQ_OP; }
"!="    { SAVE_TOKEN; return NE_OP; }
"&&"    { SAVE_TOKEN; return AND_OP; }
"||"    { SAVE_TOKEN; return OR_OP; }

 /* ─── Single-character Operators ─────────── */
[+\-*/%&|^~!<>=] {
    SAVE_TOKEN;
    return yytext[0];
}

 /* ─── Punctuation ────────────────────────── */
[(){}[\];,.:?] {
    SAVE_TOKEN;
    return yytext[0];
}

 /* ─── Invalid Token ──────────────────────── */
. {
    fprintf(stderr, "***lexing terminated*** [lexer error]: invalid token '%s' at line %d\n",
            yytext, yylineno);
    exit(1);
}

%%
