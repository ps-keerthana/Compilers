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
    UNSIGNED = 265,                /* UNSIGNED  */
    SIGNED = 266,                  /* SIGNED  */
    IF = 267,                      /* IF  */
    ELSE = 268,                    /* ELSE  */
    FOR = 269,                     /* FOR  */
    WHILE = 270,                   /* WHILE  */
    DO = 271,                      /* DO  */
    SWITCH = 272,                  /* SWITCH  */
    CASE = 273,                    /* CASE  */
    DEFAULT = 274,                 /* DEFAULT  */
    BREAK = 275,                   /* BREAK  */
    CONTINUE = 276,                /* CONTINUE  */
    RETURN = 277,                  /* RETURN  */
    STRUCT = 278,                  /* STRUCT  */
    TYPEDEF = 279,                 /* TYPEDEF  */
    ENUM = 280,                    /* ENUM  */
    UNION = 281,                   /* UNION  */
    SIZEOF = 282,                  /* SIZEOF  */
    AUTO = 283,                    /* AUTO  */
    CONST = 284,                   /* CONST  */
    VOLATILE = 285,                /* VOLATILE  */
    STATIC = 286,                  /* STATIC  */
    EXTERN = 287,                  /* EXTERN  */
    REGISTER = 288,                /* REGISTER  */
    INLINE = 289,                  /* INLINE  */
    IDENTIFIER = 290,              /* IDENTIFIER  */
    INT_CONST = 291,               /* INT_CONST  */
    FLOAT_CONST = 292,             /* FLOAT_CONST  */
    CHAR_CONST = 293,              /* CHAR_CONST  */
    STRING_LITERAL = 294,          /* STRING_LITERAL  */
    ASSIGN_OP = 295,               /* ASSIGN_OP  */
    ELLIPSIS = 296,                /* ELLIPSIS  */
    ARROW = 297,                   /* ARROW  */
    INC_OP = 298,                  /* INC_OP  */
    DEC_OP = 299,                  /* DEC_OP  */
    LEFT_SHIFT = 300,              /* LEFT_SHIFT  */
    RIGHT_SHIFT = 301,             /* RIGHT_SHIFT  */
    LE_OP = 302,                   /* LE_OP  */
    GE_OP = 303,                   /* GE_OP  */
    EQ_OP = 304,                   /* EQ_OP  */
    NE_OP = 305,                   /* NE_OP  */
    AND_OP = 306,                  /* AND_OP  */
    OR_OP = 307,                   /* OR_OP  */
    UMINUS = 308,                  /* UMINUS  */
    UPLUS = 309,                   /* UPLUS  */
    LOWER_THAN_ELSE = 310          /* LOWER_THAN_ELSE  */
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
#define UNSIGNED 265
#define SIGNED 266
#define IF 267
#define ELSE 268
#define FOR 269
#define WHILE 270
#define DO 271
#define SWITCH 272
#define CASE 273
#define DEFAULT 274
#define BREAK 275
#define CONTINUE 276
#define RETURN 277
#define STRUCT 278
#define TYPEDEF 279
#define ENUM 280
#define UNION 281
#define SIZEOF 282
#define AUTO 283
#define CONST 284
#define VOLATILE 285
#define STATIC 286
#define EXTERN 287
#define REGISTER 288
#define INLINE 289
#define IDENTIFIER 290
#define INT_CONST 291
#define FLOAT_CONST 292
#define CHAR_CONST 293
#define STRING_LITERAL 294
#define ASSIGN_OP 295
#define ELLIPSIS 296
#define ARROW 297
#define INC_OP 298
#define DEC_OP 299
#define LEFT_SHIFT 300
#define RIGHT_SHIFT 301
#define LE_OP 302
#define GE_OP 303
#define EQ_OP 304
#define NE_OP 305
#define AND_OP 306
#define OR_OP 307
#define UMINUS 308
#define UPLUS 309
#define LOWER_THAN_ELSE 310

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 714 "cl_parser.y"

    char  *str_val;   /* literal text for terminals */
    int    node_idx;  /* tree node index for non-terminals */

#line 182 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
