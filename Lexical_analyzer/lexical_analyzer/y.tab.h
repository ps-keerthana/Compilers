/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INT = 258,                     /* INT  */
    FLOAT = 259,                   /* FLOAT  */
    CHAR = 260,                    /* CHAR  */
    VOID = 261,                    /* VOID  */
    DOUBLE = 262,                  /* DOUBLE  */
    SHORT = 263,                   /* SHORT  */
    LONG = 264,                    /* LONG  */
    IF = 265,                      /* IF  */
    ELSE = 266,                    /* ELSE  */
    FOR = 267,                     /* FOR  */
    WHILE = 268,                   /* WHILE  */
    DO = 269,                      /* DO  */
    SWITCH = 270,                  /* SWITCH  */
    CASE = 271,                    /* CASE  */
    DEFAULT = 272,                 /* DEFAULT  */
    BREAK = 273,                   /* BREAK  */
    CONTINUE = 274,                /* CONTINUE  */
    RETURN = 275,                  /* RETURN  */
    STRUCT = 276,                  /* STRUCT  */
    TYPEDEF = 277,                 /* TYPEDEF  */
    ENUM = 278,                    /* ENUM  */
    UNION = 279,                   /* UNION  */
    SIZEOF = 280,                  /* SIZEOF  */
    AUTO = 281,                    /* AUTO  */
    CONST = 282,                   /* CONST  */
    STATIC = 283,                  /* STATIC  */
    EXTERN = 284,                  /* EXTERN  */
    REGISTER = 285,                /* REGISTER  */
    IDENTIFIER = 286,              /* IDENTIFIER  */
    INT_CONST = 287,               /* INT_CONST  */
    FLOAT_CONST = 288,             /* FLOAT_CONST  */
    CHAR_CONST = 289,              /* CHAR_CONST  */
    STRING_LITERAL = 290,          /* STRING_LITERAL  */
    OPERATOR = 291                 /* OPERATOR  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define INT 258
#define FLOAT 259
#define CHAR 260
#define VOID 261
#define DOUBLE 262
#define SHORT 263
#define LONG 264
#define IF 265
#define ELSE 266
#define FOR 267
#define WHILE 268
#define DO 269
#define SWITCH 270
#define CASE 271
#define DEFAULT 272
#define BREAK 273
#define CONTINUE 274
#define RETURN 275
#define STRUCT 276
#define TYPEDEF 277
#define ENUM 278
#define UNION 279
#define SIZEOF 280
#define AUTO 281
#define CONST 282
#define STATIC 283
#define EXTERN 284
#define REGISTER 285
#define IDENTIFIER 286
#define INT_CONST 287
#define FLOAT_CONST 288
#define CHAR_CONST 289
#define STRING_LITERAL 290
#define OPERATOR 291

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
