/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "cl_parser.y"

/*
 * cl_parser.y  --  CS327 Assignment 4: Intermediate Code Generation
 *
 * QUADRUPLE FORMAT:  op | arg1 | arg2 | result
 *
 * CORRECT CONTROL-FLOW ORDER:
 *
 *   if (cond) S:                       if (cond) S1 else S2:
 *     [cond quads]                       [cond quads]
 *     jf  cond _ Lend                    jf  cond _ Lelse
 *     [S quads]                          [S1 quads]
 *     label _ _ Lend                     goto _ _ Lend
 *                                        label _ _ Lelse
 *                                        [S2 quads]
 *                                        label _ _ Lend
 *
 *   while (cond) S:                    for (init; cond; step) S:
 *     label _ _ Lstart                   [init quads]
 *     [cond quads]                       label _ _ Lstart
 *     jf cond _ Lend                     [cond quads]
 *     [S quads]                          jf cond _ Lend
 *     goto _ _ Lstart                    [S quads]
 *     label _ _ Lend                     [step quads]
 *                                        goto _ _ Lstart
 *                                        label _ _ Lend
 *
 *   a && b  (short-circuit):           a || b  (short-circuit):
 *     [a quads]                          [a quads]
 *     jf  a _ Lfalse                     jt  a _ Ltrue
 *     [b quads]                          [b quads]
 *     jf  b _ Lfalse                     jt  b _ Ltrue
 *     t = 1                              t = 0
 *     goto _ _ Lend                      goto _ _ Lend
 *     label _ _ Lfalse                   label _ _ Ltrue
 *     t = 0                              t = 1
 *     label _ _ Lend                     label _ _ Lend
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

extern int   yylineno;
extern FILE *yyin;
extern char  last_token[256];
extern char *yytext;
extern int   yychar;
int yylex(void);

/* ================================================================
   IR ENGINE
   ================================================================ */

#define IR_MAXQ   8000
#define IR_POOL  16000
#define IR_PLEN   64

typedef struct {
    char op    [24];
    char arg1  [64];
    char arg2  [64];
    char result[64];
    int  is_marker;
} Quad;

static Quad ir_q[IR_MAXQ];
static int  ir_n    = 0;
static int  ir_tmpn = 0;
static int  ir_lbln = 0;

static char ir_pool[IR_POOL][IR_PLEN];
static int  ir_pool_n = 0;

static const char *ir_intern(const char *s) {
    if (!s || !s[0]) return "";
    for (int i = 0; i < ir_pool_n; i++)
        if (!strcmp(ir_pool[i], s)) return ir_pool[i];
    if (ir_pool_n >= IR_POOL) return "??";
    strncpy(ir_pool[ir_pool_n], s, IR_PLEN-1);
    ir_pool[ir_pool_n][IR_PLEN-1] = '\0';
    return ir_pool[ir_pool_n++];
}

static const char *ir_newtemp(void) {
    char buf[IR_PLEN]; snprintf(buf, IR_PLEN, "t%d", ++ir_tmpn);
    return ir_intern(buf);
}

static const char *ir_newlabel(void) {
    char buf[IR_PLEN]; snprintf(buf, IR_PLEN, "L%d", ir_lbln++);
    return ir_intern(buf);
}

static int ir_emit(const char *op, const char *a1, const char *a2, const char *res) {
    if (ir_n >= IR_MAXQ) { fprintf(stderr,"[IR] overflow\n"); return -1; }
    int i = ir_n++;
    strncpy(ir_q[i].op,    op  ?op  :"",23);
    strncpy(ir_q[i].arg1,  a1  ?a1  :"",63);
    strncpy(ir_q[i].arg2,  a2  ?a2  :"",63);
    strncpy(ir_q[i].result,res ?res :"",63);
    ir_q[i].is_marker = 0;
    return i;
}

static int ir_mark(void) {
    if (ir_n >= IR_MAXQ) return -1;
    int i = ir_n++;
    strcpy(ir_q[i].op,    "---");
    strcpy(ir_q[i].arg1,  "");
    strcpy(ir_q[i].arg2,  "");
    strcpy(ir_q[i].result,"");
    ir_q[i].is_marker = 1;
    return i;
}

static int ir_insert_before(int pos, const char *op, const char *a1,
                            const char *a2, const char *res) {
    if (ir_n >= IR_MAXQ) { fprintf(stderr,"[IR] overflow\n"); return -1; }
    if (pos < 0 || pos > ir_n) pos = ir_n;
    for (int i = ir_n; i > pos; i--) ir_q[i] = ir_q[i-1];
    ir_n++;
    strncpy(ir_q[pos].op,    op  ?op  :"",23);
    strncpy(ir_q[pos].arg1,  a1  ?a1  :"",63);
    strncpy(ir_q[pos].arg2,  a2  ?a2  :"",63);
    strncpy(ir_q[pos].result,res ?res :"",63);
    ir_q[pos].is_marker = 0;
    return pos;
}

static void ir_delete(int pos) {
    if (pos < 0 || pos >= ir_n) return;
    for (int i = pos; i < ir_n-1; i++) ir_q[i] = ir_q[i+1];
    ir_n--;
}

static void ir_delete_markers(void) {
    int i = 0;
    while (i < ir_n) {
        if (ir_q[i].is_marker) ir_delete(i);
        else i++;
    }
}

/* ================================================================
   FIX BUG 3: Dead temporary elimination pass.
   After IR generation, remove any quad whose result is a temporary
   (t1, t2, ...) that is never referenced as arg1 or arg2 in any
   subsequent quad. This handles the post-increment-as-statement case
   where  = p _ tN  saves the old value into tN that is never read.
   The pass is run once before printing.
   ================================================================ */
static void ir_eliminate_dead_temps(void) {
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < ir_n; i++) {
            /* Only consider quads that assign to a temporary */
            const char *res = ir_q[i].result;
            if (!res[0]) continue;
            /* Must start with 't' followed by digits */
            if (res[0] != 't') continue;
            int all_digits = 1;
            for (int k = 1; res[k]; k++) if (res[k] < '0' || res[k] > '9') { all_digits = 0; break; }
            if (!all_digits) continue;
            /* Skip call quads — their result may be ignored intentionally but
               we must keep the call for its side effects */
            if (!strcmp(ir_q[i].op, "call")) continue;
            /* Check if this temp is used anywhere in the entire quad list */
            int used = 0;
            for (int j = 0; j < ir_n && !used; j++) {
                if (j == i) continue;
                if (!strcmp(ir_q[j].arg1, res)) used = 1;
                if (!strcmp(ir_q[j].arg2, res)) used = 1;
                if (!strcmp(ir_q[j].result, res) && j != i) {
                    /* Another quad writes the same temp — original is dead */
                }
            }
            if (!used) {
                /* Dead temporary assignment — remove it */
                /* But only if the operation itself has no side-effects.
                   Safe to remove: plain copy (=), arithmetic, logical, shift.
                   Do NOT remove: call, store, param, addr, deref, elemaddr
                   (those have side effects or are needed for addressing). */
                const char *op = ir_q[i].op;
                int safe_to_remove = 0;
                if (!strcmp(op,"=")    || !strcmp(op,"+")    || !strcmp(op,"-")   ||
                    !strcmp(op,"*")    || !strcmp(op,"/")    || !strcmp(op,"%")   ||
                    !strcmp(op,"minus")|| !strcmp(op,"!")    || !strcmp(op,"~")   ||
                    !strcmp(op,"&")    || !strcmp(op,"|")    || !strcmp(op,"^")   ||
                    !strcmp(op,"<<")   || !strcmp(op,">>")   ||
                    !strcmp(op,"<")    || !strcmp(op,">")    || !strcmp(op,"<=")  ||
                    !strcmp(op,">=")   || !strcmp(op,"==")   || !strcmp(op,"!="))
                    safe_to_remove = 1;
                if (safe_to_remove) {
                    ir_delete(i);
                    changed = 1;
                    break; /* restart scan after mutation */
                }
            }
        }
    }
}

/* Side-channels */
#define PT_SIZE 24000
static const char *ir_place[PT_SIZE];
static const char *ir_place_addr[PT_SIZE];
static int ir_argcount[PT_SIZE];
static int ir_place_init = 0;

static void ir_init(void) {
    if (!ir_place_init) {
        memset(ir_place,0,sizeof ir_place);
        memset(ir_place_addr,0,sizeof ir_place_addr);
        memset(ir_argcount,0,sizeof ir_argcount);
        ir_place_init=1;
    }
}
static void ir_setplace(int n, const char *p) { if(n>=0&&n<PT_SIZE&&p) ir_place[n]=p; }
static const char *ir_getplace(int n) { if(n>=0&&n<PT_SIZE&&ir_place[n])return ir_place[n];return ""; }
static void ir_setplace_addr(int n, const char *a) { if(n>=0&&n<PT_SIZE&&a) ir_place_addr[n]=ir_intern(a); }
static const char *ir_getplace_addr(int n) { if(n>=0&&n<PT_SIZE&&ir_place_addr[n]) return ir_place_addr[n]; return ""; }
static void ir_set_argcount(int n, int c) { if(n>=0&&n<PT_SIZE) ir_argcount[n]=c; }
static int ir_get_argcount(int n) { if(n>=0&&n<PT_SIZE) return ir_argcount[n]; return 0; }

#define MK_SIZE 24000
static int ir_marker[MK_SIZE];
static int ir_marker_init = 0;
static void ir_set_marker(int n, int m) {
    if (!ir_marker_init) { memset(ir_marker,-1,sizeof ir_marker); ir_marker_init=1; }
    if (n>=0&&n<MK_SIZE) ir_marker[n]=m;
}
static int ir_get_marker(int n) {
    if (!ir_marker_init) return -1;
    if (n>=0&&n<MK_SIZE) return ir_marker[n];
    return -1;
}

static char ir_cur_func[128] = "";

/* ================================================================
   FIX BUG 1: Deferred param buffer for function calls.
   Problem: arg_list emits "param" instructions immediately as each
   argument expression is reduced. For nested calls like
   add(square(2), square(3)), the param for add's first argument
   (t5, result of square(2)) is emitted immediately after call square,
   but BEFORE the second nested call square(3). This interleaves params
   from different call frames, producing incorrect IR.

   Solution: Buffer all param quads for a call site. The arg_list rule
   records (arg_place, arg_index) in a deferred buffer instead of
   emitting param immediately. When the outer call is reduced
   (postfix_expr '(' arg_list ')'), we flush the deferred params for
   that call level immediately before the call quad.

   A stack of call levels handles arbitrary nesting depth.
   ================================================================ */
#define PARAM_BUF_DEPTH  64   /* max nesting depth of calls          */
#define PARAM_BUF_ARGS   256  /* max args per single call site        */

typedef struct {
    char places[PARAM_BUF_ARGS][64]; /* arg places in left-to-right order */
    int  count;
} ParamLevel;

static ParamLevel param_stack[PARAM_BUF_DEPTH];
static int        param_depth = -1;  /* -1 means no call in progress     */

/* Called when we enter a postfix_expr '(' — push a new call level */
static void param_push_level(void) {
    param_depth++;
    if (param_depth >= PARAM_BUF_DEPTH) {
        fprintf(stderr, "[IR] param stack overflow\n");
        param_depth = PARAM_BUF_DEPTH - 1;
    }
    param_stack[param_depth].count = 0;
}

/* Record one argument at the current call level (called from arg_list) */
static void param_record(const char *place) {
    if (param_depth < 0) {
        /* Fallback: no active call frame — emit directly (shouldn't happen) */
        ir_emit("param", place ? place : "", "", "");
        return;
    }
    ParamLevel *lv = &param_stack[param_depth];
    if (lv->count < PARAM_BUF_ARGS) {
        strncpy(lv->places[lv->count], place ? place : "", 63);
        lv->places[lv->count][63] = '\0';
        lv->count++;
    }
}

/* Flush all buffered params for the current level, then pop */
static void param_flush_and_pop(void) {
    if (param_depth < 0) return;
    ParamLevel *lv = &param_stack[param_depth];
    for (int i = 0; i < lv->count; i++)
        ir_emit("param", lv->places[i], "", "");
    param_depth--;
}

/* ================================================================
   SEMANTIC ANALYSIS
   Lightweight semantic checks during IR generation:
   - undeclared identifiers / functions
   - invalid dereference / address-of
   - basic assignment and operator type checking
   - function call arity checks
   - break/continue outside loop or switch
   ================================================================ */

enum {
    SEM_UNKNOWN = 0,
    SEM_INT,
    SEM_FLOAT,
    SEM_CHAR,
    SEM_VOID,
    SEM_STRUCT
};

typedef struct {
    char name[128];
    int base;
    int ptr_depth;
    int is_function;
    int param_count;
    int is_defined;
    int is_local;
} SemSymbol;

#define SEM_MAX_SYMBOLS 2048
static SemSymbol sem_symbols[SEM_MAX_SYMBOLS];
static int sem_symbol_n = 0;

static int sem_base_node[PT_SIZE];
static int sem_ptr_node[PT_SIZE];
static int sem_lvalue_node[PT_SIZE];
static int sem_func_node[PT_SIZE];
static int sem_paramcount_node[PT_SIZE];
static int sem_node_init = 0;

static int sem_decl_base = SEM_UNKNOWN;
static int sem_decl_ptr = 0;
static int sem_current_return_base = SEM_UNKNOWN;
static int sem_current_return_ptr = 0;
static int sem_current_function = -1;
static int sem_in_function = 0;
static int switch_depth = 0;

static void sem_reset_node_info(void) {
    if (sem_node_init) return;
    for (int i = 0; i < PT_SIZE; i++) {
        sem_base_node[i] = SEM_UNKNOWN;
        sem_ptr_node[i] = 0;
        sem_lvalue_node[i] = 0;
        sem_func_node[i] = 0;
        sem_paramcount_node[i] = -1;
    }
    sem_node_init = 1;
}

static const char *sem_base_name(int base) {
    switch (base) {
        case SEM_INT: return "int";
        case SEM_FLOAT: return "float";
        case SEM_CHAR: return "char";
        case SEM_VOID: return "void";
        case SEM_STRUCT: return "struct";
        default: return "unknown";
    }
}

static void sem_type_to_string(int base, int ptr_depth, char *buf, size_t bufsz) {
    snprintf(buf, bufsz, "%s", sem_base_name(base));
    size_t len = strlen(buf);
    while (ptr_depth-- > 0 && len + 1 < bufsz) buf[len++] = '*';
    buf[len] = '\0';
}

static void sem_errorf(const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "\n+--------------------------------------------------------------+\n");
    fprintf(stderr, "|                    SEMANTIC ERROR                            |\n");
    fprintf(stderr, "+--------------------------------------------------------------+\n");
    fprintf(stderr, "  Line         : %d\n", yylineno);
    fprintf(stderr, "  Near token   : '%s'\n", last_token[0] ? last_token : "(unknown)");
    fprintf(stderr, "  In function  : %s\n", ir_cur_func[0] ? ir_cur_func : "(global)");
    fprintf(stderr, "  Message      : ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n\n");
    exit(1);
}

static void sem_init(void) {
    sem_reset_node_info();
}

static void sem_set_node(int n, int base, int ptr_depth, int is_lvalue) {
    if (n < 0 || n >= PT_SIZE) return;
    sem_base_node[n] = base;
    sem_ptr_node[n] = ptr_depth;
    sem_lvalue_node[n] = is_lvalue;
    sem_func_node[n] = 0;
    sem_paramcount_node[n] = -1;
}

static void sem_copy_node(int dst, int src) {
    if (dst < 0 || dst >= PT_SIZE || src < 0 || src >= PT_SIZE) return;
    sem_base_node[dst] = sem_base_node[src];
    sem_ptr_node[dst] = sem_ptr_node[src];
    sem_lvalue_node[dst] = sem_lvalue_node[src];
    sem_func_node[dst] = sem_func_node[src];
    sem_paramcount_node[dst] = sem_paramcount_node[src];
}

static int sem_is_numeric(int base, int ptr_depth) {
    return ptr_depth == 0 && (base == SEM_INT || base == SEM_FLOAT || base == SEM_CHAR);
}

static int sem_is_integer_like(int base, int ptr_depth) {
    return ptr_depth == 0 && (base == SEM_INT || base == SEM_CHAR);
}

static int sem_is_scalar(int base, int ptr_depth) {
    return ptr_depth > 0 || sem_is_numeric(base, ptr_depth);
}

static int sem_types_match_exact(int b1, int p1, int b2, int p2) {
    return b1 == b2 && p1 == p2;
}

static int sem_is_null_literal(int node_idx) {
    const char *p = ir_getplace(node_idx);
    return sem_base_node[node_idx] == SEM_INT && sem_ptr_node[node_idx] == 0 && p && !strcmp(p, "0");
}

static int sem_assignment_compatible(int lhs_node, int rhs_node) {
    int lb = sem_base_node[lhs_node], lp = sem_ptr_node[lhs_node];
    int rb = sem_base_node[rhs_node], rp = sem_ptr_node[rhs_node];
    if (sem_types_match_exact(lb, lp, rb, rp)) return 1;
    if (sem_is_numeric(lb, lp) && sem_is_numeric(rb, rp)) return 1;
    if (lp > 0 && sem_is_null_literal(rhs_node)) return 1;
    return 0;
}

static int sem_find_symbol(const char *name, int locals_only) {
    if (!name || !name[0]) return -1;
    for (int i = sem_symbol_n - 1; i >= 0; i--) {
        if (strcmp(sem_symbols[i].name, name)) continue;
        if (locals_only && !sem_symbols[i].is_local) continue;
        return i;
    }
    return -1;
}

static int sem_lookup_symbol(const char *name) {
    int idx = sem_find_symbol(name, 1);
    if (idx >= 0) return idx;
    for (int i = sem_symbol_n - 1; i >= 0; i--) {
        if (!strcmp(sem_symbols[i].name, name) && !sem_symbols[i].is_local) return i;
    }
    return -1;
}

static void sem_clear_locals(void) {
    while (sem_symbol_n > 0 && sem_symbols[sem_symbol_n - 1].is_local) sem_symbol_n--;
}

static int sem_add_symbol(const char *name, int base, int ptr_depth, int is_function,
                          int param_count, int is_defined, int is_local) {
    if (sem_symbol_n >= SEM_MAX_SYMBOLS) sem_errorf("symbol table overflow");
    strncpy(sem_symbols[sem_symbol_n].name, name ? name : "?", 127);
    sem_symbols[sem_symbol_n].name[127] = '\0';
    sem_symbols[sem_symbol_n].base = base;
    sem_symbols[sem_symbol_n].ptr_depth = ptr_depth;
    sem_symbols[sem_symbol_n].is_function = is_function;
    sem_symbols[sem_symbol_n].param_count = param_count;
    sem_symbols[sem_symbol_n].is_defined = is_defined;
    sem_symbols[sem_symbol_n].is_local = is_local;
    return sem_symbol_n++;
}

static void sem_declare_object(const char *name, int base, int ptr_depth) {
    int existing = sem_find_symbol(name, sem_in_function ? 1 : 0);
    if (existing >= 0) sem_errorf("redeclaration of '%s'", name);
    sem_add_symbol(name, base, ptr_depth, 0, -1, 1, sem_in_function);
}

static void sem_declare_function(const char *name, int base, int ptr_depth,
                                 int param_count, int is_definition) {
    int idx = sem_lookup_symbol(name);
    if (idx >= 0) {
        if (!sem_symbols[idx].is_function)
            sem_errorf("'%s' was previously declared as a variable", name);
        if (!sem_types_match_exact(sem_symbols[idx].base, sem_symbols[idx].ptr_depth, base, ptr_depth))
            sem_errorf("conflicting return type for function '%s'", name);
        if (param_count >= 0 && sem_symbols[idx].param_count >= 0 &&
            sem_symbols[idx].param_count != param_count)
            sem_errorf("conflicting parameter count for function '%s'", name);
        if (param_count >= 0) sem_symbols[idx].param_count = param_count;
        if (is_definition && sem_symbols[idx].is_defined)
            sem_errorf("redefinition of function '%s'", name);
        if (is_definition) sem_symbols[idx].is_defined = 1;
        return;
    }
    sem_add_symbol(name, base, ptr_depth, 1, param_count, is_definition, 0);
}

static void sem_begin_decl(int base, int ptr_depth) {
    sem_decl_base = base;
    sem_decl_ptr = ptr_depth;
}

static void sem_begin_function(const char *name, int ret_base, int ret_ptr) {
    sem_clear_locals();
    sem_in_function = 1;
    sem_current_return_base = ret_base;
    sem_current_return_ptr = ret_ptr;
    strncpy(ir_cur_func, name ? name : "?", 127);
    ir_cur_func[127] = '\0';
    sem_declare_function(name, ret_base, ret_ptr, -1, 1);
    sem_current_function = sem_lookup_symbol(name);
}

static void sem_finish_function_header(int param_count) {
    if (sem_current_function >= 0 && param_count >= 0)
        sem_symbols[sem_current_function].param_count = param_count;
}

static void sem_end_function(void) {
    sem_clear_locals();
    sem_in_function = 0;
    sem_current_function = -1;
    sem_current_return_base = SEM_UNKNOWN;
    sem_current_return_ptr = 0;
}

static void sem_require_numeric_binary(const char *op, int lhs, int rhs) {
    if (!sem_is_numeric(sem_base_node[lhs], sem_ptr_node[lhs]) ||
        !sem_is_numeric(sem_base_node[rhs], sem_ptr_node[rhs])) {
        sem_errorf("operator '%s' requires numeric operands", op);
    }
}

static void sem_require_integer_binary(const char *op, int lhs, int rhs) {
    if (!sem_is_integer_like(sem_base_node[lhs], sem_ptr_node[lhs]) ||
        !sem_is_integer_like(sem_base_node[rhs], sem_ptr_node[rhs])) {
        sem_errorf("operator '%s' requires integer operands", op);
    }
}

static void sem_set_binary_result_numeric(int dst, int lhs, int rhs) {
    int base = (sem_base_node[lhs] == SEM_FLOAT || sem_base_node[rhs] == SEM_FLOAT) ? SEM_FLOAT : SEM_INT;
    sem_set_node(dst, base, 0, 0);
}

/* ----------------------------------------------------------------
   Short-circuit label stacks for && and ||
   ---------------------------------------------------------------- */
#define SC_DEPTH 64
static const char *sc_and_lfalse[SC_DEPTH];
static int sc_and_depth = 0;
static const char *sc_or_ltrue[SC_DEPTH];
static int sc_or_depth  = 0;

/* ----------------------------------------------------------------
   Switch dispatch infrastructure.
   BUG 1 FIX: The original switch generated no comparison/jump IR —
   case bodies ran unconditionally and break was never patched.

   Design: a switch context stack.  When SWITCH '(' expr ')' '{' is
   reduced, we push:
     - the place of the switch expression
     - a fresh Lend label  (break → goto Lend)
     - an array of (case_value_string, case_label) pairs built as
       each labeled_stmt is reduced

   After the entire case_list is parsed, we retroactively INSERT the
   dispatch quads (== + jt pairs) before the first case body, and
   patch all raw break quads to goto Lend.

   A "default" case is tracked separately; it always goes last in the
   dispatch chain (fall-through to default if no case matched).
   ---------------------------------------------------------------- */
#define SW_MAX_CASES  256
#define SW_DEPTH       32

typedef struct {
    char  sw_expr[64];          /* place of the switch expression       */
    char  sw_lend[32];          /* label for end of switch / break      */
    /* Per-case info filled in as labeled_stmt rules reduce              */
    char  case_vals[SW_MAX_CASES][64];  /* "1", "2", ... or "" for default */
    char  case_lbls[SW_MAX_CASES][32];  /* Lcase0, Lcase1, ...             */
    int   case_is_default[SW_MAX_CASES];
    int   n_cases;
    int   has_default;
    int   default_idx;
    int   body_start_quad;      /* ir_n at the moment of switch open    */
} SwContext;

static SwContext sw_stack[SW_DEPTH];
static int sw_depth = 0;

static void sw_push(const char *expr_place, const char *lend) {
    if (sw_depth >= SW_DEPTH) return;
    SwContext *sc = &sw_stack[sw_depth++];
    strncpy(sc->sw_expr, expr_place ? expr_place : "", 63);
    sc->sw_expr[63] = '\0';
    strncpy(sc->sw_lend, lend ? lend : "", 31);
    sc->sw_lend[31] = '\0';
    sc->n_cases = 0;
    sc->has_default = 0;
    sc->default_idx = -1;
    sc->body_start_quad = ir_n;
}

static void sw_pop(void) { if (sw_depth > 0) sw_depth--; }
static SwContext *sw_cur(void) { return sw_depth > 0 ? &sw_stack[sw_depth-1] : NULL; }

/* Called from labeled_stmt when a CASE expr ':' is reduced.
   Allocates a fresh label for this case body and records it. */
static const char *sw_add_case(const char *val_place) {
    SwContext *sc = sw_cur();
    if (!sc || sc->n_cases >= SW_MAX_CASES) return ir_newlabel();
    const char *lbl = ir_newlabel();
    strncpy(sc->case_vals[sc->n_cases], val_place ? val_place : "", 63);
    sc->case_vals[sc->n_cases][63] = '\0';
    strncpy(sc->case_lbls[sc->n_cases], lbl, 31);
    sc->case_lbls[sc->n_cases][31] = '\0';
    sc->case_is_default[sc->n_cases] = 0;
    sc->n_cases++;
    return lbl;
}

/* Called from labeled_stmt when DEFAULT ':' is reduced. */
static const char *sw_add_default(void) {
    SwContext *sc = sw_cur();
    if (!sc || sc->n_cases >= SW_MAX_CASES) return ir_newlabel();
    const char *lbl = ir_newlabel();
    strncpy(sc->case_vals[sc->n_cases], "", 63);
    strncpy(sc->case_lbls[sc->n_cases], lbl, 31);
    sc->case_is_default[sc->n_cases] = 1;
    sc->has_default = 1;
    sc->default_idx = sc->n_cases;
    sc->n_cases++;
    return lbl;
}

/* ----------------------------------------------------------------
   Loop-label stack for break/continue translation.
   ---------------------------------------------------------------- */
#define LOOP_DEPTH 64
static const char *loop_lstart[LOOP_DEPTH];
static const char *loop_lend[LOOP_DEPTH];
static int loop_depth = 0;

static void loop_push(const char *ls, const char *le) {
    if (loop_depth < LOOP_DEPTH) {
        loop_lstart[loop_depth] = ls;
        loop_lend[loop_depth]   = le;
        loop_depth++;
    }
}
static void loop_pop(void) { if (loop_depth > 0) loop_depth--; }
static const char *loop_cur_lend(void)   { return loop_depth>0 ? loop_lend[loop_depth-1]   : NULL; }
static const char *loop_cur_lstart(void) { return loop_depth>0 ? loop_lstart[loop_depth-1] : NULL; }

/* ----------------------------------------------------------------
   Print source + IR
   ---------------------------------------------------------------- */
static void ir_print_source(const char *path) {
    printf("\n");
    printf("+============================================================+\n");
    printf("| SOURCE PROGRAM                                             |\n");
    printf("+============================================================+\n");
    FILE *f = fopen(path,"r"); if(!f){printf("  (cannot open)\n");return;}
    char line[512]; int ln=1;
    while(fgets(line,sizeof line,f)) printf("  %3d | %s",ln++,line);
    fclose(f);
    printf("\n");
}

/* ================================================================
   ir_quad_to_stmt: produce a human-readable statement string for a quad.
   Used as the extra "stmt" column in the 5-column IR table.
   ================================================================ */
static void ir_quad_to_stmt(const Quad *q, char *buf, size_t bufsz) {
    const char *a1 = q->arg1[0] ? q->arg1 : "_";
    const char *a2 = q->arg2[0] ? q->arg2 : "_";
    const char *rs = q->result[0] ? q->result : "_";
    const char *op = q->op;

    if (!strcmp(op,"label")) {
        snprintf(buf,bufsz,"label %s",rs);
    } else if (!strcmp(op,"jf") || !strcmp(op,"jt")) {
        snprintf(buf,bufsz,"%s %s, %s",op,a1,rs);
    } else if (!strcmp(op,"goto")) {
        snprintf(buf,bufsz,"goto %s",rs);
    } else if (!strcmp(op,"call")) {
        snprintf(buf,bufsz,"%s = call %s, %s",rs,a1,a2);
    } else if (!strcmp(op,"param")) {
        snprintf(buf,bufsz,"param %s",a1);
    } else if (!strcmp(op,"return")) {
        if (q->arg1[0]) snprintf(buf,bufsz,"return %s",a1);
        else            snprintf(buf,bufsz,"return");
    } else if (!strcmp(op,"begin_func")) {
        snprintf(buf,bufsz,"begin_func %s",a1);
    } else if (!strcmp(op,"end_func")) {
        snprintf(buf,bufsz,"end_func");
    } else if (!strcmp(op,"deref")) {
        snprintf(buf,bufsz,"%s = deref %s",rs,a1);
    } else if (!strcmp(op,"addr")) {
        snprintf(buf,bufsz,"%s = & %s",rs,a1);
    } else if (!strcmp(op,"elemaddr")) {
        snprintf(buf,bufsz,"%s = elemaddr %s[%s]",rs,a1,a2);
    } else if (!strcmp(op,"store")) {
        snprintf(buf,bufsz,"store %s -> [%s]",a1,rs);
    } else if (!strcmp(op,"=")) {
        snprintf(buf,bufsz,"%s = %s",rs,a1);
    } else if (!strcmp(op,"minus") || !strcmp(op,"!") || !strcmp(op,"~") || !strcmp(op,"sizeof")) {
        snprintf(buf,bufsz,"%s = %s %s",rs,op,a1);
    } else if (q->arg2[0] && strcmp(q->arg2,"_")) {
        snprintf(buf,bufsz,"%s = %s %s %s",rs,a1,op,a2);
    } else {
        snprintf(buf,bufsz,"%s = %s %s",rs,op,a1);
    }
}

static void ir_print(void) {
    /* Run dead-temp elimination before printing */
    ir_eliminate_dead_temps();

    ir_delete_markers();

    printf("\n");
    printf("+============================================================+\n");
    printf("| INTERMEDIATE CODE  (Three Address Code -- Quadruples)      |\n");
    printf("+============================================================+\n");
    if (ir_n==0){printf("  (no IR generated)\n\n");return;}

    /* Column widths: stmt, op, arg1, arg2, result */
    int ws=4, w0=4, w1=5, w2=5, w3=6;
    for(int i=0;i<ir_n;i++){
        char sbuf[256]; ir_quad_to_stmt(&ir_q[i],sbuf,sizeof sbuf);
        int l;
        if((l=(int)strlen(sbuf             ))>ws)ws=l;
        if((l=(int)strlen(ir_q[i].op      ))>w0)w0=l;
        if((l=(int)strlen(ir_q[i].arg1    ))>w1)w1=l;
        if((l=(int)strlen(ir_q[i].arg2    ))>w2)w2=l;
        if((l=(int)strlen(ir_q[i].result  ))>w3)w3=l;
    }
    ws++;w0++;w1++;w2++;w3++;

    /* Build separator */
    char sep[768];int sp=0;
    #define SEP(n) do{for(int _=0;_<(n);_++)sep[sp++]='-';}while(0)
    sep[sp++]='+';SEP(5);sep[sp++]='+';
    SEP(ws+2);sep[sp++]='+';
    SEP(w0+2);sep[sp++]='+';SEP(w1+2);sep[sp++]='+';
    SEP(w2+2);sep[sp++]='+';SEP(w3+2);sep[sp++]='+';sep[sp]='\0';

    printf("%s\n",sep);
    printf("| %-3s | %-*s | %-*s | %-*s | %-*s | %-*s |\n",
           "#",ws,"stmt",w0,"op",w1,"arg1",w2,"arg2",w3,"result");
    printf("%s\n",sep);
    for(int i=0;i<ir_n;i++){
        char sbuf[256]; ir_quad_to_stmt(&ir_q[i],sbuf,sizeof sbuf);
        const char *a1=ir_q[i].arg1[0]?ir_q[i].arg1:"_";
        const char *a2=ir_q[i].arg2[0]?ir_q[i].arg2:"_";
        const char *rs=ir_q[i].result[0]?ir_q[i].result:"_";
        printf("| %-3d | %-*s | %-*s | %-*s | %-*s | %-*s |\n",
               i,ws,sbuf,w0,ir_q[i].op,w1,a1,w2,a2,w3,rs);
    }
    printf("%s\n",sep);
    /*
     * BUG C FIX: ir_tmpn is the allocation high-watermark and includes
     * temps removed by dead-temp elimination, causing gaps in the footer
     * (e.g. "t1..t6" when t3 was eliminated).  Instead, scan the live IR
     * table for the actual highest temp number still present, and list
     * only the temps that genuinely exist in the output.
     */
    int max_tmp = 0;
    int max_lbl = -1;
    for(int i=0;i<ir_n;i++){
        /* scan all three fields for tN references */
        const char *fields[3];
        fields[0]=ir_q[i].arg1; fields[1]=ir_q[i].arg2; fields[2]=ir_q[i].result;
        for(int f=0;f<3;f++){
            const char *s=fields[f];
            if(s[0]=='t'){
                int ok=1; for(int k=1;s[k];k++) if(s[k]<'0'||s[k]>'9'){ok=0;break;}
                if(ok){ int n=atoi(s+1); if(n>max_tmp)max_tmp=n; }
            }
            if(s[0]=='L'){ int n=atoi(s+1); if(n>max_lbl)max_lbl=n; }
        }
    }
    if(max_tmp==0 && max_lbl<0)
        printf("  Total quads: %d    Temporaries: (none)    Labels: (none)\n\n",ir_n);
    else if(max_lbl<0)
        printf("  Total quads: %d    Temporaries: t1..t%d    Labels: (none)\n\n",ir_n,max_tmp);
    else if(max_tmp==0)
        printf("  Total quads: %d    Temporaries: (none)    Labels: L0..L%d\n\n",ir_n,max_lbl);
    else
        printf("  Total quads: %d    Temporaries: t1..t%d    Labels: L0..L%d\n\n",ir_n,max_tmp,max_lbl);
}


/* ================================================================
   ASSIGNMENT 3 Part 3 -- Derivation Tree
   ================================================================ */
#define MAX_TREE_NODES 20000
typedef struct{char rule[512];char lhs[128];int parent,depth,children[32],nchildren,line;}TreeNode;
static TreeNode tnodes[MAX_TREE_NODES];
static int tsize=0,tred=0,tstack[MAX_TREE_NODES],ttop=0;
static int cnt_global_decls=0,cnt_func_defs=0;
static int cnt_if_without_else=0,cnt_if_with_else=0;
static int cnt_for_loops=0,cnt_while_loops=0,cnt_do_while_loops=0;
static int cnt_switch_stmts=0,cnt_return_stmts=0;
static int cnt_ptr_decls=0,cnt_int_consts=0;
static int cnt_func_calls=0,cnt_assignments=0,cnt_struct_decls=0;
static int ifd_cur=0,ifd_max=0;

static void xlhs(const char*r,char*d,int dl){
    const char*a=strstr(r,"\xe2\x86\x92");if(!a){strncpy(d,r,dl-1);d[dl-1]='\0';return;}
    int n=(int)(a-r);if(n>=dl)n=dl-1;strncpy(d,r,n);d[n]='\0';
    while(n>0&&d[n-1]==' ')d[--n]='\0';
}
static int xrhs(const char*rule){
    static const char*nt[]={"additive_expr","and_expr","arg_list","assignment_expr",
        "block_item","block_item_list","case_body_list","case_list",
        "compound_stmt","conditional_expr","declaration","declarator",
        "declarator_list","enum_decl","enumerator","enumerator_list",
        "equality_expr","exclusive_or_expr","expr","expr_stmt",
        "external_decl","function_def","function_header",
        "inclusive_or_expr","initializer","initializer_list",
        "iteration_stmt","jump_stmt","labeled_stmt",
        "logical_and_expr","logical_or_expr",
        "member_decl","member_decl_list","multiplicative_expr",
        "non_case_stmt","param_decl","param_list",
        "postfix_expr","primary_expr","program",
        "relational_expr","selection_stmt","shift_expr","stmt",
        "storage_class_list","storage_class_spec","struct_decl",
        "translation_unit","type_qualifier","type_spec","unary_expr",NULL};
    const char*a=strstr(rule,"\xe2\x86\x92");if(!a)return 0;
    const char*p=a+3;while(*p==' ')p++;if(!*p)return 0;
    int c=0;char w[128];int wl=0;
    while(1){if(*p==' '||*p=='\t'||!*p){if(wl){w[wl]='\0';for(int i=0;nt[i];i++)if(!strcmp(w,nt[i])){c++;break;}wl=0;}if(!*p)break;}else if(wl<127)w[wl++]=*p;p++;}
    return c;
}
int new_node(const char*rule){
    if(tsize>=MAX_TREE_NODES){fprintf(stderr,"[tree overflow]\n");return -1;}
    int idx=tsize++;TreeNode*n=&tnodes[idx];
    strncpy(n->rule,rule,511);n->rule[511]='\0';xlhs(rule,n->lhs,sizeof n->lhs);
    n->parent=-1;n->depth=0;n->nchildren=0;n->line=yylineno;tred++;
    int rhs=xrhs(rule);if(rhs>ttop)rhs=ttop;
    int cs=ttop-rhs;
    for(int i=cs;i<ttop;i++){int ch=tstack[i];if(n->nchildren<32)n->children[n->nchildren++]=ch;tnodes[ch].parent=idx;}
    ttop-=rhs;tstack[ttop++]=idx;return idx;
}
static void ptree(int idx,int d){
    if(idx<0||idx>=tsize)return;TreeNode*n=&tnodes[idx];
    for(int i=0;i<d;i++)printf("  ");printf("%s\n",n->rule);
    for(int i=0;i<n->nchildren;i++)ptree(n->children[i],d+1);
}
void print_tree_reverse(void){
    printf("\n+--------------------------------------------------------------+\n");
    printf("|   DERIVATION TREE  (indented, top-down structure)            |\n");
    printf("+--------------------------------------------------------------+\n");
    printf("  Total reductions: %d\n\n",tred);
    if(ttop>0)ptree(tstack[ttop-1],0);
    else for(int i=0;i<tsize;i++)if(tnodes[i].parent==-1){ptree(i,0);break;}
    printf("\n");
}


/* ================================================================
   ASSIGNMENT 3 Part 4 -- Parsing Table
   ================================================================ */
#define MAX_PT_STATES  600
#define MAX_PT_SYMBOLS 200
#define PT_SYM_LEN      48
#define PT_CELL_LEN     16
typedef struct{char sym[PT_SYM_LEN];int is_terminal;}PtSym;
typedef struct{char action[PT_CELL_LEN];}PtCell;
static PtSym ptsy[MAX_PT_SYMBOLS];static int nptsy=0;
static PtCell pttb[MAX_PT_STATES][MAX_PT_SYMBOLS];static int nptst=0;
static int ptfoa(const char*s,int t){for(int i=0;i<nptsy;i++)if(!strcmp(ptsy[i].sym,s))return i;if(nptsy>=MAX_PT_SYMBOLS)return -1;strncpy(ptsy[nptsy].sym,s,PT_SYM_LEN-1);ptsy[nptsy].is_terminal=t;return nptsy++;}
static void ptset(int st,const char*s,int t,const char*v){int c=ptfoa(s,t);if(c<0||st<0||st>=MAX_PT_STATES)return;if(!pttb[st][c].action[0])strncpy(pttb[st][c].action,v,PT_CELL_LEN-1);}
static void squo(char*t){int l=(int)strlen(t);if(l>=2&&t[0]=='\''&&t[l-1]=='\''){memmove(t,t+1,l-1);t[l-1]='\0';}}
static void parse_yout(const char*path){
    FILE*f=fopen(path,"r");if(!f)return;char buf[512];int cur=-1;
    while(fgets(buf,sizeof buf,f)){
        if(!strncmp(buf,"State ",6)){int s=atoi(buf+6);cur=s;if(s+1>nptst)nptst=s+1;continue;}
        if(cur<0)continue;char*p=buf;while(*p==' '||*p=='\t')p++;if(!*p||*p=='\n')continue;
        if(!strncmp(p,"$end  accept",12)||!strncmp(p,"$default  accept",16)){ptset(cur,"$end",1,"acc");continue;}
        char sy[64],tg[32];
        if(sscanf(p,"%63s shift, and go to state %31s",sy,tg)==2){squo(sy);char c2[PT_CELL_LEN];snprintf(c2,PT_CELL_LEN,"s%s",tg);ptset(cur,sy,1,c2);continue;}
        if(sscanf(p,"%63s go to state %31s",sy,tg)==2){squo(sy);ptset(cur,sy,0,tg);continue;}
        char rest[256];
        if(sscanf(p,"%63s reduce using rule %255[^\n]",sy,rest)==2){squo(sy);int rn=atoi(rest);char c2[PT_CELL_LEN];snprintf(c2,PT_CELL_LEN,"r%d",rn);ptset(cur,sy,1,c2);continue;}
        if(sscanf(p,"$default reduce using rule %255[^\n]",rest)==1){int rn=atoi(rest);char c2[PT_CELL_LEN];snprintf(c2,PT_CELL_LEN,"r%d",rn);for(int c=0;c<nptsy;c++)if(ptsy[c].is_terminal&&!pttb[cur][c].action[0])strncpy(pttb[cur][c].action,c2,PT_CELL_LEN-1);}
    }fclose(f);
}
static void export_csv(void){
    mkdir("output",0755);FILE*o=fopen("output/parsing_table.csv","w");if(!o)return;
    fprintf(o,"State");for(int c=0;c<nptsy;c++)fprintf(o,",%s",ptsy[c].sym);fprintf(o,"\n");
    for(int s=0;s<nptst;s++){int h=0;for(int c=0;c<nptsy;c++)if(pttb[s][c].action[0]){h=1;break;}if(!h)continue;
        fprintf(o,"%d",s);for(int c=0;c<nptsy;c++)fprintf(o,",%s",pttb[s][c].action[0]?pttb[s][c].action:".");fprintf(o,"\n");}
    fclose(o);
}
void print_parsing_table(const char*yp){
    parse_yout(yp);
    int tc[MAX_PT_SYMBOLS],tnc=0,gc[MAX_PT_SYMBOLS],gnc=0;
    for(int i=0;i<nptsy;i++){int h=0;for(int s=0;s<nptst&&!h;s++)if(pttb[s][i].action[0])h=1;if(!h)continue;if(ptsy[i].is_terminal)tc[tnc++]=i;else gc[gnc++]=i;if(tnc+gnc>=MAX_PT_SYMBOLS)break;}
    int show[64];int ns=0;
    #define ADD_SHOW(x) do{int _x=(x),_d=0;for(int _i=0;_i<ns;_i++)if(show[_i]==_x){_d=1;break;}if(!_d&&ns<64)show[ns++]=_x;}while(0)
    ADD_SHOW(0);{FILE*f=fopen(yp,"r");if(f){char b[256];while(fgets(b,sizeof b,f))if(strstr(b,"conflicts:")){int s;if(sscanf(b,"State %d",&s)==1)ADD_SHOW(s);}fclose(f);}}
    for(int c=0;c<nptsy;c++)if(!strcmp(ptsy[c].sym,"$end"))for(int s=0;s<nptst;s++)if(!strcmp(pttb[s][c].action,"acc")){ADD_SHOW(s);break;}
    for(int s=1,a=0;s<nptst&&a<4;s++){int h=0;for(int i=0;i<tnc&&!h;i++)if(pttb[s][tc[i]].action[0])h=1;if(h){ADD_SHOW(s);a++;}}
    for(int i=0;i<ns-1;i++)for(int j=i+1;j<ns;j++)if(show[i]>show[j]){int t=show[i];show[i]=show[j];show[j]=t;}
    const int CW=7,SW=5;
    printf("\n+--------------------------------------------------------------+\n");
    printf("|   LALR(1) PARSING TABLE  (selected states shown)             |\n");
    printf("|   Full table -> output/parsing_table.csv                     |\n");
    printf("+--------------------------------------------------------------+\n");
    printf("  sN=shift  rN=reduce  N=goto  acc=accept  .=error\n\n");
    printf("%*s |",SW,"State");printf(" %-*s|",(tnc*CW)-1," ACTION");printf(" GOTO\n");
    printf("%*s |",SW,"");for(int i=0;i<tnc;i++)printf(" %-*.*s",CW-1,CW-1,ptsy[tc[i]].sym);printf("|");for(int i=0;i<gnc;i++)printf(" %-*.*s",CW-1,CW-1,ptsy[gc[i]].sym);printf("\n");
    printf("%*s-+",SW,"-----");for(int i=0;i<tnc+gnc;i++)printf("-------+");printf("\n");
    for(int si=0;si<ns;si++){int s=show[si];if(s<0||s>=nptst)continue;printf("%*d |",SW,s);for(int i=0;i<tnc;i++){const char*v=pttb[s][tc[i]].action;printf(" %-*.*s",CW-1,CW-1,v[0]?v:".");}printf("|");for(int i=0;i<gnc;i++){const char*v=pttb[s][gc[i]].action;printf(" %-*.*s",CW-1,CW-1,v[0]?v:".");}printf("\n");}
    printf("\n  Showing %d / %d states.  Full: output/parsing_table.csv\n\n",ns,nptst);
    export_csv();
    printf("  [Part 4] Full LALR(1) table (%d states x %d symbols) saved.\n\n",nptst,nptsy);
}


/* ================================================================
   ASSIGNMENT 3 Part 5 -- Error Diagnostics
   ================================================================ */
static const char*const tok_names[]={"int","float","char","void","double","short","long","unsigned","signed","if","else","for","while","do","switch","case","default","break","continue","return","struct","typedef","enum","union","sizeof","auto","const","volatile","static","extern","register","inline","IDENTIFIER","INT_CONST","FLOAT_CONST","CHAR_CONST","STRING_LITERAL","ASSIGN_OP(+=etc)","...","->","++","--","<<",">>","<=",">=","==","!=","&&","||","UMINUS","UPLUS","LOWER_THAN_ELSE"};
static const int tok_names_count=(int)(sizeof tok_names/sizeof tok_names[0]);
static const char*tok_name_safe(int tok){if(!tok)return"EOF";if(tok==256)return"<e>";if(tok<256){static char b[8];if(tok>=32&&tok<127)snprintf(b,8,"'%c'",tok);else snprintf(b,8,"0x%02x",tok);return b;}int i=tok-258;return(i>=0&&i<tok_names_count)?tok_names[i]:"?";}
void yyerror(const char *s){
    fprintf(stderr,"\n+--------------------------------------------------------------+\n");
    fprintf(stderr,"|                     PARSE ERROR                              |\n");
    fprintf(stderr,"+--------------------------------------------------------------+\n");
    fprintf(stderr,"  Line         : %d\n",yylineno);
    fprintf(stderr,"  Near token   : '%s'\n",last_token[0]?last_token:"(unknown)");
    fprintf(stderr,"  Lookahead    : %s (token id %d)\n",tok_name_safe(yychar),yychar);
    fprintf(stderr,"  Message      : %s\n",s);
    fprintf(stderr,"\n  Hints:\n");
    fprintf(stderr,"    - Missing ';' at end of statement?\n");
    fprintf(stderr,"    - Unmatched '{', '}', '(', or ')'?\n");
    fprintf(stderr,"    - Invalid or unsupported construct near line %d?\n",yylineno);
    fprintf(stderr,"    - Declaration after statement (C89 mode)?\n\n");
    exit(1);
}


/* ================================================================
   ASSIGNMENT 3 Part 6 -- Grammar Statistics
   ================================================================ */
static void print_grammar_summary(void){
    FILE*f=fopen("y.output","r");int rc=0,sc=0,sr=0,rr=0;
    if(f){char buf[512];while(fgets(buf,sizeof buf,f)){
        if(buf[0]==' '&&buf[1]==' '&&buf[2]==' '&&buf[3]>='0'&&buf[3]<='9')rc++;
        if(!strncmp(buf,"State ",6)){int s=atoi(buf+6);if(s+1>sc)sc=s+1;}
        if(strstr(buf,"shift/reduce")){int n=0;sscanf(buf,"State %*d conflicts: %d",&n);sr+=(n>0)?n:1;}
        if(strstr(buf,"reduce/reduce")){int n=0;sscanf(buf,"State %*d conflicts: %d",&n);rr+=(n>0)?n:1;}
    }fclose(f);}
    printf("\n+--------------------------------------------------------------+\n");
    printf("|              PART 6 -- ADDITIONAL INFORMATION (A3)           |\n");
    printf("+--------------------------------------------------------------+\n\n");
    printf("  -- Program Analysis --\n");
    printf("  #global_declarations         = %d\n",cnt_global_decls);
    printf("  #function_definitions        = %d\n",cnt_func_defs);
    printf("  #if_without_else             = %d\n",cnt_if_without_else);
    printf("  #if_with_else                = %d\n",cnt_if_with_else);
    printf("  if_else_max_nesting_depth    = %d\n",ifd_max);
    printf("  #for_loops                   = %d\n",cnt_for_loops);
    printf("  #while_loops                 = %d\n",cnt_while_loops);
    printf("  #do_while_loops              = %d\n",cnt_do_while_loops);
    printf("  #switch_statements           = %d\n",cnt_switch_stmts);
    printf("  #return_statements           = %d\n",cnt_return_stmts);
    printf("  #pointer_declarations        = %d\n",cnt_ptr_decls);
    printf("  #integer_constants           = %d\n",cnt_int_consts);
    printf("  #function_calls              = %d\n",cnt_func_calls);
    printf("  #assignments                 = %d\n",cnt_assignments);
    printf("  #struct_union_declarations   = %d\n",cnt_struct_decls);
    printf("  #total_reductions            = %d\n",tred);
    printf("  #total_quadruples            = %d\n",ir_n);
    printf("\n  -- Grammar Statistics --\n");
    if(rc>0)printf("  Productions (grammar rules)  : %d\n",rc);
    if(sc>0)printf("  LALR(1) states               : %d\n",sc);
    printf("  Shift/reduce conflicts       : %d  (target: 0)\n",sr);
    printf("  Reduce/reduce conflicts      : %d  (target: 0)\n",rr);
    printf("\n  -- Conflict Resolution Summary --\n");
    printf("  1. Dangling-else (S/R)  -> %%prec LOWER_THAN_ELSE\n");
    printf("  2. Operator precedence  -> 15-level %%left/%%right table\n");
    printf("  3. Unary vs binary +/-  -> %%prec UMINUS/UPLUS\n");
    printf("  4. TYPEDEF redundancy   -> Removed redundant rule (State 62)\n");
    printf("  5. case_body_list S/R   -> non_case_stmt + %%expect 38\n\n");
}


#line 1083 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
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
#line 1013 "cl_parser.y"

    char  *str_val;
    int    node_idx;

#line 1251 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_FLOAT = 4,                      /* FLOAT  */
  YYSYMBOL_CHAR = 5,                       /* CHAR  */
  YYSYMBOL_VOID = 6,                       /* VOID  */
  YYSYMBOL_DOUBLE = 7,                     /* DOUBLE  */
  YYSYMBOL_SHORT = 8,                      /* SHORT  */
  YYSYMBOL_LONG = 9,                       /* LONG  */
  YYSYMBOL_UNSIGNED = 10,                  /* UNSIGNED  */
  YYSYMBOL_SIGNED = 11,                    /* SIGNED  */
  YYSYMBOL_IF = 12,                        /* IF  */
  YYSYMBOL_ELSE = 13,                      /* ELSE  */
  YYSYMBOL_FOR = 14,                       /* FOR  */
  YYSYMBOL_WHILE = 15,                     /* WHILE  */
  YYSYMBOL_DO = 16,                        /* DO  */
  YYSYMBOL_SWITCH = 17,                    /* SWITCH  */
  YYSYMBOL_CASE = 18,                      /* CASE  */
  YYSYMBOL_DEFAULT = 19,                   /* DEFAULT  */
  YYSYMBOL_BREAK = 20,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 21,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 22,                    /* RETURN  */
  YYSYMBOL_STRUCT = 23,                    /* STRUCT  */
  YYSYMBOL_TYPEDEF = 24,                   /* TYPEDEF  */
  YYSYMBOL_ENUM = 25,                      /* ENUM  */
  YYSYMBOL_UNION = 26,                     /* UNION  */
  YYSYMBOL_SIZEOF = 27,                    /* SIZEOF  */
  YYSYMBOL_AUTO = 28,                      /* AUTO  */
  YYSYMBOL_CONST = 29,                     /* CONST  */
  YYSYMBOL_VOLATILE = 30,                  /* VOLATILE  */
  YYSYMBOL_STATIC = 31,                    /* STATIC  */
  YYSYMBOL_EXTERN = 32,                    /* EXTERN  */
  YYSYMBOL_REGISTER = 33,                  /* REGISTER  */
  YYSYMBOL_INLINE = 34,                    /* INLINE  */
  YYSYMBOL_IDENTIFIER = 35,                /* IDENTIFIER  */
  YYSYMBOL_INT_CONST = 36,                 /* INT_CONST  */
  YYSYMBOL_FLOAT_CONST = 37,               /* FLOAT_CONST  */
  YYSYMBOL_CHAR_CONST = 38,                /* CHAR_CONST  */
  YYSYMBOL_STRING_LITERAL = 39,            /* STRING_LITERAL  */
  YYSYMBOL_ASSIGN_OP = 40,                 /* ASSIGN_OP  */
  YYSYMBOL_ELLIPSIS = 41,                  /* ELLIPSIS  */
  YYSYMBOL_ARROW = 42,                     /* ARROW  */
  YYSYMBOL_INC_OP = 43,                    /* INC_OP  */
  YYSYMBOL_DEC_OP = 44,                    /* DEC_OP  */
  YYSYMBOL_LEFT_SHIFT = 45,                /* LEFT_SHIFT  */
  YYSYMBOL_RIGHT_SHIFT = 46,               /* RIGHT_SHIFT  */
  YYSYMBOL_LE_OP = 47,                     /* LE_OP  */
  YYSYMBOL_GE_OP = 48,                     /* GE_OP  */
  YYSYMBOL_EQ_OP = 49,                     /* EQ_OP  */
  YYSYMBOL_NE_OP = 50,                     /* NE_OP  */
  YYSYMBOL_AND_OP = 51,                    /* AND_OP  */
  YYSYMBOL_OR_OP = 52,                     /* OR_OP  */
  YYSYMBOL_53_ = 53,                       /* '='  */
  YYSYMBOL_54_ = 54,                       /* '?'  */
  YYSYMBOL_55_ = 55,                       /* ':'  */
  YYSYMBOL_56_ = 56,                       /* '|'  */
  YYSYMBOL_57_ = 57,                       /* '^'  */
  YYSYMBOL_58_ = 58,                       /* '&'  */
  YYSYMBOL_59_ = 59,                       /* '<'  */
  YYSYMBOL_60_ = 60,                       /* '>'  */
  YYSYMBOL_61_ = 61,                       /* '+'  */
  YYSYMBOL_62_ = 62,                       /* '-'  */
  YYSYMBOL_63_ = 63,                       /* '*'  */
  YYSYMBOL_64_ = 64,                       /* '/'  */
  YYSYMBOL_65_ = 65,                       /* '%'  */
  YYSYMBOL_UMINUS = 66,                    /* UMINUS  */
  YYSYMBOL_UPLUS = 67,                     /* UPLUS  */
  YYSYMBOL_68_ = 68,                       /* '!'  */
  YYSYMBOL_69_ = 69,                       /* '~'  */
  YYSYMBOL_70_ = 70,                       /* '.'  */
  YYSYMBOL_LOWER_THAN_ELSE = 71,           /* LOWER_THAN_ELSE  */
  YYSYMBOL_72_ = 72,                       /* '('  */
  YYSYMBOL_73_ = 73,                       /* ')'  */
  YYSYMBOL_74_ = 74,                       /* ','  */
  YYSYMBOL_75_ = 75,                       /* '['  */
  YYSYMBOL_76_ = 76,                       /* ']'  */
  YYSYMBOL_77_ = 77,                       /* ';'  */
  YYSYMBOL_78_ = 78,                       /* '{'  */
  YYSYMBOL_79_ = 79,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 80,                  /* $accept  */
  YYSYMBOL_program = 81,                   /* program  */
  YYSYMBOL_translation_unit = 82,          /* translation_unit  */
  YYSYMBOL_external_decl = 83,             /* external_decl  */
  YYSYMBOL_function_def = 84,              /* function_def  */
  YYSYMBOL_function_header = 85,           /* function_header  */
  YYSYMBOL_86_1 = 86,                      /* $@1  */
  YYSYMBOL_87_2 = 87,                      /* $@2  */
  YYSYMBOL_88_3 = 88,                      /* $@3  */
  YYSYMBOL_89_4 = 89,                      /* $@4  */
  YYSYMBOL_param_list = 90,                /* param_list  */
  YYSYMBOL_param_decl = 91,                /* param_decl  */
  YYSYMBOL_declaration = 92,               /* declaration  */
  YYSYMBOL_93_5 = 93,                      /* $@5  */
  YYSYMBOL_94_6 = 94,                      /* $@6  */
  YYSYMBOL_95_7 = 95,                      /* $@7  */
  YYSYMBOL_declarator_list = 96,           /* declarator_list  */
  YYSYMBOL_declarator = 97,                /* declarator  */
  YYSYMBOL_initializer = 98,               /* initializer  */
  YYSYMBOL_initializer_list = 99,          /* initializer_list  */
  YYSYMBOL_type_qualifier = 100,           /* type_qualifier  */
  YYSYMBOL_type_spec = 101,                /* type_spec  */
  YYSYMBOL_storage_class_spec = 102,       /* storage_class_spec  */
  YYSYMBOL_storage_class_list = 103,       /* storage_class_list  */
  YYSYMBOL_struct_decl = 104,              /* struct_decl  */
  YYSYMBOL_member_decl_list = 105,         /* member_decl_list  */
  YYSYMBOL_member_decl = 106,              /* member_decl  */
  YYSYMBOL_enum_decl = 107,                /* enum_decl  */
  YYSYMBOL_enumerator_list = 108,          /* enumerator_list  */
  YYSYMBOL_enumerator = 109,               /* enumerator  */
  YYSYMBOL_compound_stmt = 110,            /* compound_stmt  */
  YYSYMBOL_block_item_list = 111,          /* block_item_list  */
  YYSYMBOL_block_item = 112,               /* block_item  */
  YYSYMBOL_stmt = 113,                     /* stmt  */
  YYSYMBOL_expr_stmt = 114,                /* expr_stmt  */
  YYSYMBOL_selection_stmt = 115,           /* selection_stmt  */
  YYSYMBOL_116_8 = 116,                    /* $@8  */
  YYSYMBOL_case_list = 117,                /* case_list  */
  YYSYMBOL_case_body_list = 118,           /* case_body_list  */
  YYSYMBOL_labeled_stmt = 119,             /* labeled_stmt  */
  YYSYMBOL_non_case_stmt = 120,            /* non_case_stmt  */
  YYSYMBOL_iteration_stmt = 121,           /* iteration_stmt  */
  YYSYMBOL_122_9 = 122,                    /* @9  */
  YYSYMBOL_123_10 = 123,                   /* $@10  */
  YYSYMBOL_124_11 = 124,                   /* @11  */
  YYSYMBOL_jump_stmt = 125,                /* jump_stmt  */
  YYSYMBOL_expr = 126,                     /* expr  */
  YYSYMBOL_assignment_expr = 127,          /* assignment_expr  */
  YYSYMBOL_conditional_expr = 128,         /* conditional_expr  */
  YYSYMBOL_logical_or_expr = 129,          /* logical_or_expr  */
  YYSYMBOL_130_12 = 130,                   /* @12  */
  YYSYMBOL_logical_and_expr = 131,         /* logical_and_expr  */
  YYSYMBOL_132_13 = 132,                   /* @13  */
  YYSYMBOL_inclusive_or_expr = 133,        /* inclusive_or_expr  */
  YYSYMBOL_exclusive_or_expr = 134,        /* exclusive_or_expr  */
  YYSYMBOL_and_expr = 135,                 /* and_expr  */
  YYSYMBOL_equality_expr = 136,            /* equality_expr  */
  YYSYMBOL_relational_expr = 137,          /* relational_expr  */
  YYSYMBOL_shift_expr = 138,               /* shift_expr  */
  YYSYMBOL_additive_expr = 139,            /* additive_expr  */
  YYSYMBOL_multiplicative_expr = 140,      /* multiplicative_expr  */
  YYSYMBOL_unary_expr = 141,               /* unary_expr  */
  YYSYMBOL_postfix_expr = 142,             /* postfix_expr  */
  YYSYMBOL_143_14 = 143,                   /* $@14  */
  YYSYMBOL_144_15 = 144,                   /* $@15  */
  YYSYMBOL_primary_expr = 145,             /* primary_expr  */
  YYSYMBOL_arg_list = 146                  /* arg_list  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1121

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  80
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  67
/* YYNRULES -- Number of rules.  */
#define YYNRULES  206
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  358

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   310


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    68,     2,     2,     2,    65,    58,     2,
      72,    73,    63,    61,    74,    62,    70,    64,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,    77,
      59,    53,    60,    54,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    75,     2,    76,    57,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    78,    56,    79,    69,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    66,    67,
      71
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,  1068,  1068,  1071,  1072,  1075,  1076,  1080,  1087,  1086,
    1093,  1092,  1099,  1098,  1105,  1104,  1113,  1115,  1117,  1119,
    1123,  1126,  1130,  1133,  1136,  1142,  1141,  1145,  1144,  1147,
    1148,  1150,  1149,  1152,  1153,  1156,  1157,  1160,  1163,  1176,
    1179,  1192,  1195,  1198,  1201,  1204,  1207,  1212,  1216,  1217,
    1220,  1221,  1225,  1226,  1229,  1230,  1231,  1232,  1233,  1234,
    1235,  1236,  1237,  1238,  1239,  1240,  1241,  1242,  1243,  1244,
    1245,  1246,  1247,  1248,  1249,  1252,  1253,  1254,  1255,  1256,
    1259,  1260,  1264,  1265,  1266,  1267,  1268,  1269,  1272,  1273,
    1276,  1277,  1280,  1281,  1282,  1285,  1286,  1289,  1290,  1294,
    1297,  1302,  1305,  1310,  1313,  1318,  1319,  1320,  1321,  1322,
    1323,  1326,  1328,  1340,  1364,  1396,  1395,  1462,  1464,  1469,
    1470,  1474,  1494,  1507,  1508,  1509,  1510,  1511,  1520,  1528,
    1519,  1556,  1555,  1623,  1658,  1707,  1742,  1792,  1798,  1812,
    1816,  1827,  1831,  1838,  1842,  1878,  1901,  1905,  1925,  1931,
    1930,  1960,  1966,  1965,  1995,  1999,  2007,  2011,  2019,  2023,
    2031,  2035,  2043,  2053,  2057,  2063,  2069,  2075,  2083,  2087,
    2093,  2101,  2105,  2111,  2119,  2123,  2129,  2137,  2147,  2151,
    2158,  2165,  2172,  2178,  2185,  2192,  2205,  2212,  2217,  2240,
    2244,  2262,  2261,  2289,  2288,  2318,  2323,  2328,  2347,  2363,
    2372,  2376,  2380,  2384,  2388,  2402,  2410
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT", "CHAR",
  "VOID", "DOUBLE", "SHORT", "LONG", "UNSIGNED", "SIGNED", "IF", "ELSE",
  "FOR", "WHILE", "DO", "SWITCH", "CASE", "DEFAULT", "BREAK", "CONTINUE",
  "RETURN", "STRUCT", "TYPEDEF", "ENUM", "UNION", "SIZEOF", "AUTO",
  "CONST", "VOLATILE", "STATIC", "EXTERN", "REGISTER", "INLINE",
  "IDENTIFIER", "INT_CONST", "FLOAT_CONST", "CHAR_CONST", "STRING_LITERAL",
  "ASSIGN_OP", "ELLIPSIS", "ARROW", "INC_OP", "DEC_OP", "LEFT_SHIFT",
  "RIGHT_SHIFT", "LE_OP", "GE_OP", "EQ_OP", "NE_OP", "AND_OP", "OR_OP",
  "'='", "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'", "'+'", "'-'",
  "'*'", "'/'", "'%'", "UMINUS", "UPLUS", "'!'", "'~'", "'.'",
  "LOWER_THAN_ELSE", "'('", "')'", "','", "'['", "']'", "';'", "'{'",
  "'}'", "$accept", "program", "translation_unit", "external_decl",
  "function_def", "function_header", "$@1", "$@2", "$@3", "$@4",
  "param_list", "param_decl", "declaration", "$@5", "$@6", "$@7",
  "declarator_list", "declarator", "initializer", "initializer_list",
  "type_qualifier", "type_spec", "storage_class_spec",
  "storage_class_list", "struct_decl", "member_decl_list", "member_decl",
  "enum_decl", "enumerator_list", "enumerator", "compound_stmt",
  "block_item_list", "block_item", "stmt", "expr_stmt", "selection_stmt",
  "$@8", "case_list", "case_body_list", "labeled_stmt", "non_case_stmt",
  "iteration_stmt", "@9", "$@10", "@11", "jump_stmt", "expr",
  "assignment_expr", "conditional_expr", "logical_or_expr", "@12",
  "logical_and_expr", "@13", "inclusive_or_expr", "exclusive_or_expr",
  "and_expr", "equality_expr", "relational_expr", "shift_expr",
  "additive_expr", "multiplicative_expr", "unary_expr", "postfix_expr",
  "$@14", "$@15", "primary_expr", "arg_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-297)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-192)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1027,  -297,  -297,  -297,  -297,  -297,    29,    34,   145,    50,
     -16,  1091,   -15,    -4,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,    57,  1027,  -297,  -297,    22,  -297,  1091,    -9,  -297,
    1059,    49,    66,  -297,  -297,  -297,   146,  -297,  -297,  -297,
    -297,    81,  1091,  -297,  -297,  -297,   121,    99,   130,  1091,
    -297,  -297,   316,  -297,  -297,   147,  -297,    72,    10,  -297,
    -297,  -297,  -297,  1091,   -12,   220,  -297,    72,    99,   182,
       9,  -297,  1091,   277,   150,   165,   172,  -297,   175,   916,
     205,   198,   202,   808,   929,  -297,  -297,  -297,  -297,  -297,
     916,   916,   916,   916,   916,   916,   916,   916,   916,  -297,
    -297,  -297,   241,  1059,  -297,   393,  -297,  -297,  -297,  -297,
    -297,  -297,  -297,    83,  -297,  -297,   115,   310,   214,   215,
     304,    -3,   154,    69,   178,   169,    78,    96,  -297,   290,
      -5,   329,   302,   102,  -297,   294,  -297,    72,   470,  -297,
     103,  -297,  -297,   104,    51,   331,    99,  -297,   479,  -297,
     916,   507,  -297,   690,   916,    55,  -297,  -297,  -297,  -297,
     129,   631,  -297,  -297,  -297,  -297,  -297,  -297,  -297,  -297,
    -297,   168,   291,  -297,  -297,   916,  -297,  -297,   916,  -297,
     916,   916,   916,   916,   916,   916,   916,   916,   916,   916,
     916,   916,   916,   916,   916,   916,   916,   916,   334,  -297,
    -297,   335,   298,   916,   248,   299,    84,   582,    14,   320,
     340,    72,  -297,   303,   135,  -297,  -297,  -297,  -297,  -297,
    -297,  -297,   189,   821,   821,   916,   365,   191,  -297,   749,
    -297,   308,  -297,  -297,   916,    58,   916,   215,  -297,   304,
      -3,   154,   154,    69,    69,    69,    69,   178,   178,   169,
     169,  -297,  -297,  -297,  -297,  -297,  -297,  -297,   309,   916,
     141,  -297,   193,  -297,    19,  -297,    84,  -297,  -297,  -297,
     195,   307,  -297,    84,   313,  -297,   248,   314,  -297,   690,
     864,   877,   217,   317,  -297,   749,  -297,  -297,  -297,  -297,
    -297,  -297,  -297,   310,   916,   214,  -297,  -297,   219,  -297,
    -297,   999,   315,   356,  -297,    62,  -297,  -297,  -297,   361,
     221,  -297,   379,   690,   224,   690,   231,  -297,   916,   328,
    -297,  -297,   916,  -297,  -297,    25,  -297,   762,  -297,   591,
    -297,   690,  -297,   690,  -297,   690,   690,   235,   292,  -297,
     358,  -297,  -297,  -297,  -297,   239,  -297,  -297,  -297,  -297,
     362,     6,  -297,  -297,  -297,  -297,  -297,  -297
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    54,    55,    56,    57,    58,    59,    60,    61,    62,
       0,     0,     0,     0,    75,    52,    53,    77,    78,    76,
      79,     0,     2,     3,     5,     0,     6,     0,    25,    80,
       0,    73,    74,    64,    65,    66,    71,    67,    69,    68,
      70,    84,     0,    31,    73,    74,    94,     0,    87,     0,
       1,     4,     0,     7,    63,     0,    29,     0,    27,    81,
      33,    34,    72,     0,     0,     0,    88,     0,     0,    97,
       0,    95,     0,     0,     0,     0,     0,   131,     0,     0,
       0,     0,     0,     0,     0,   199,   200,   201,   202,   203,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   111,
      99,   103,    25,     0,   105,     0,   101,   104,   106,   107,
     110,   108,   109,     0,   141,   143,   146,   148,   151,   154,
     156,   158,   160,   163,   168,   171,   174,   178,   189,     8,
      37,     0,     0,     0,    35,     0,    30,     0,     0,    91,
       0,    83,    89,     0,     0,     0,     0,    93,     0,    86,
       0,     0,   128,     0,     0,     0,   119,   139,   140,   137,
       0,     0,   187,   179,   180,   186,   182,   181,   185,   183,
     184,     0,    27,   100,   102,     0,   112,   149,     0,   152,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   197,
     198,     0,   193,     0,     0,     0,     0,     0,     0,    39,
       0,     0,    26,    12,     0,    82,    90,    32,    92,    98,
      96,    85,     0,     0,     0,     0,     0,     0,   119,   122,
     138,     0,   204,   142,     0,     0,     0,   155,   174,   157,
     159,   161,   162,   166,   167,   164,   165,   169,   170,   172,
     173,   175,   176,   177,   145,   144,   196,   195,     0,     0,
       0,    18,     0,    16,    24,    11,     0,    38,    47,    44,
       0,     0,    42,     0,     0,    36,     0,     0,    28,     0,
       0,     0,     0,     0,   115,   121,   123,   124,   125,   120,
     126,   127,   188,   150,     0,   153,   192,   205,     0,   190,
       9,     0,    20,     0,    50,     0,    43,    41,    40,     0,
       0,    15,   113,     0,     0,     0,     0,   129,     0,     0,
     147,   194,     0,    19,    17,     0,    21,     0,    48,     0,
      13,     0,   135,     0,   133,     0,     0,     0,     0,   206,
       0,    22,    49,    51,    46,     0,   114,   136,   134,   130,
       0,     0,   117,    23,    45,   132,   116,   118
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -297,  -297,  -297,   413,  -297,  -297,  -297,  -297,  -297,  -297,
    -205,   137,   -47,  -297,  -297,  -297,   -31,   229,  -258,  -297,
    -297,     0,   -26,    56,     7,   -28,   -59,    12,   373,   296,
     -24,  -297,   338,  -115,  -148,  -219,  -297,  -297,   216,  -296,
    -297,  -213,  -297,  -297,  -297,  -212,   -70,  -157,   151,  -297,
    -297,   212,  -297,   211,   268,   269,   267,   131,   -13,   127,
     166,     2,  -297,  -297,  -297,  -297,  -297
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    21,    22,    23,    24,    25,   204,   205,   276,   277,
     262,   263,    26,    57,   137,    67,   133,   134,   267,   305,
      27,    64,    29,   103,    44,    65,    66,    45,    70,    71,
     104,   105,   106,   107,   108,   109,   319,   351,   229,   110,
     289,   111,   225,   336,   153,   112,   113,   114,   115,   116,
     234,   117,   236,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   258,   259,   128,   298
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      28,    53,   270,   224,    59,   101,   142,    31,   304,   155,
     288,    43,    32,   160,   142,   308,   290,   291,   233,    41,
      46,    73,    28,   130,    79,    80,    55,    54,   171,    31,
      58,    48,    33,   140,    32,   138,   143,    34,   226,   254,
     255,    35,   352,    36,   148,   135,   183,   184,   206,   268,
     271,   131,   102,    40,   302,   357,    30,    50,   101,    31,
     132,   340,    42,    47,    32,   139,   288,   207,    56,   343,
     208,   310,   290,   291,    49,   280,   281,    59,    30,   142,
     222,   287,   303,   146,   227,   356,   162,   136,   147,   142,
     272,   171,   163,   164,   165,   166,   167,   168,   169,   170,
      52,   341,   297,   172,   223,   102,   214,   130,   235,   268,
     228,    84,    31,   294,   189,   190,   268,    32,   196,    85,
      86,    87,    88,    89,   345,   146,    60,    90,    91,   175,
     218,   197,   175,   260,    69,   131,   327,   287,   198,   199,
     200,   328,    92,    61,   132,    93,    94,    95,    37,    62,
      38,   102,    96,    97,    39,   282,    98,   175,    31,    63,
     176,   231,   266,    32,   312,   339,   201,   177,   202,   178,
     268,   203,   243,   244,   245,   246,   211,   211,   211,   212,
     216,   217,   238,   238,   238,   238,   238,   238,   238,   238,
     238,   238,   238,   238,   238,   251,   252,   253,   332,    68,
     334,   185,   186,   175,   264,   286,   230,   264,    72,   211,
     314,   316,   278,   187,   188,   175,   346,   299,   347,   129,
     348,   349,   150,     1,     2,     3,     4,     5,     6,     7,
       8,     9,   193,   194,   195,   145,   238,   151,   238,   191,
     192,   232,   175,    10,   152,    12,    13,   154,   337,    15,
      16,     1,     2,     3,     4,     5,     6,     7,     8,     9,
     156,   286,   279,   175,   284,   175,   300,   301,   306,   301,
     180,    10,   181,    12,    13,   157,   264,    15,    16,   158,
       1,     2,     3,     4,     5,     6,     7,     8,     9,   261,
     317,   175,   321,   322,   330,   301,   238,   333,   175,   141,
      10,   264,    12,    13,   335,   175,    15,    16,   350,   175,
      79,    80,   354,   301,   241,   242,   247,   248,    56,     1,
       2,     3,     4,     5,     6,     7,     8,     9,    74,   264,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    10,
      11,    12,    13,    84,    14,    15,    16,    17,    18,    19,
      20,    85,    86,    87,    88,    89,   149,   249,   250,    90,
      91,   179,   182,   -10,   209,   210,   213,   219,   136,   256,
     257,  -191,   265,   273,    92,   274,   -14,    93,    94,    95,
     283,   292,   296,   307,    96,    97,   309,   311,    98,   318,
     325,   326,   331,    99,    52,   100,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    74,   338,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    10,    11,    12,    13,
      84,    14,    15,    16,    17,    18,    19,    20,    85,    86,
      87,    88,    89,   329,   353,    51,    90,    91,   324,   355,
     275,   144,   220,   174,   285,   320,   293,   295,   237,   240,
     239,    92,     0,     0,    93,    94,    95,     0,     0,     0,
       0,    96,    97,     0,     0,    98,     0,     0,     0,     0,
      99,    52,   173,     1,     2,     3,     4,     5,     6,     7,
       8,     9,     1,     2,     3,     4,     5,     6,     7,     8,
       9,     0,     0,    10,     0,    12,    13,     0,     0,    15,
      16,     0,    10,     0,    12,    13,     0,     0,    15,    16,
       1,     2,     3,     4,     5,     6,     7,     8,     9,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      10,    11,    12,    13,    84,    14,    15,    16,    17,    18,
      19,    20,    85,    86,    87,    88,    89,     0,     0,   215,
      90,    91,     0,     0,     0,     0,     0,     0,   221,     0,
       0,     0,     0,     0,     0,    92,     0,     0,    93,    94,
      95,     0,     0,     0,     0,    96,    97,     0,     0,    98,
       0,     0,     0,     0,    99,     1,     2,     3,     4,     5,
       6,     7,     8,     9,     1,     2,     3,     4,     5,     6,
       7,     8,     9,     0,     0,    10,     0,    12,    13,     0,
       0,    15,    16,     0,    10,     0,    12,    13,     0,     0,
      15,    16,     0,   261,     0,     0,     0,     0,     0,     0,
       0,     0,   261,     0,     1,     2,     3,     4,     5,     6,
       7,     8,     9,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    10,   269,    12,    13,    84,     0,
      15,    16,     0,     0,   344,     0,    85,    86,    87,    88,
      89,     0,     0,     0,    90,    91,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    92,
       0,     0,    93,    94,    95,     0,     0,     0,     0,    96,
      97,     0,    74,    98,    75,    76,    77,    78,    79,    80,
      81,    82,    83,     0,     0,     0,     0,    84,     0,     0,
       0,     0,     0,     0,     0,    85,    86,    87,    88,    89,
       0,     0,     0,    90,    91,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    92,     0,
       0,    93,    94,    95,     0,     0,     0,     0,    96,    97,
       0,    74,    98,    75,    76,    77,    78,    99,    52,    81,
      82,    83,     0,     0,     0,     0,    84,     0,     0,     0,
       0,     0,     0,     0,    85,    86,    87,    88,    89,    84,
       0,     0,    90,    91,     0,     0,     0,    85,    86,    87,
      88,    89,     0,     0,     0,    90,    91,    92,     0,     0,
      93,    94,    95,     0,     0,     0,     0,    96,    97,     0,
      92,    98,     0,    93,    94,    95,    99,    52,     0,     0,
      96,    97,     0,     0,    98,    84,     0,     0,     0,     0,
     266,   342,     0,    85,    86,    87,    88,    89,    84,     0,
       0,    90,    91,     0,     0,     0,    85,    86,    87,    88,
      89,     0,     0,     0,    90,    91,    92,     0,     0,    93,
      94,    95,     0,     0,     0,     0,    96,    97,     0,    92,
      98,     0,    93,    94,    95,   159,     0,     0,     0,    96,
      97,    84,     0,    98,     0,     0,     0,     0,    99,    85,
      86,    87,    88,    89,    84,     0,     0,    90,    91,     0,
       0,     0,    85,    86,    87,    88,    89,     0,     0,     0,
      90,    91,    92,     0,     0,    93,    94,    95,     0,     0,
       0,     0,    96,    97,     0,    92,    98,   313,    93,    94,
      95,     0,     0,    84,     0,    96,    97,     0,     0,    98,
     315,    85,    86,    87,    88,    89,    84,     0,     0,    90,
      91,     0,     0,     0,    85,    86,    87,    88,    89,     0,
       0,     0,    90,    91,    92,     0,     0,    93,    94,    95,
       0,     0,     0,     0,    96,    97,     0,    92,    98,     0,
      93,    94,    95,     0,     0,     0,     0,    96,    97,     0,
       0,   161,     1,     2,     3,     4,     5,     6,     7,     8,
       9,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    10,     0,    12,    13,     0,     0,    15,    16,
       1,     2,     3,     4,     5,     6,     7,     8,     9,     0,
     323,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      10,    11,    12,    13,     0,    14,    15,    16,    17,    18,
      19,    20,     1,     2,     3,     4,     5,     6,     7,     8,
       9,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    10,     0,    12,    13,     0,    14,    15,    16,
      17,    18,    19,    20,     1,     2,     3,     4,     5,     6,
       7,     8,     9,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    10,     0,    12,    13,     0,     0,
      15,    16
};

static const yytype_int16 yycheck[] =
{
       0,    25,   207,   151,    30,    52,    65,     0,   266,    79,
     229,    11,     0,    83,    73,   273,   229,   229,   175,    35,
      35,    49,    22,    35,    18,    19,    35,    27,    98,    22,
      30,    35,     3,    64,    22,    63,    67,     3,   153,   196,
     197,     7,   338,     9,    72,    35,    49,    50,    53,   206,
      36,    63,    52,     3,    35,   351,     0,     0,   105,    52,
      72,    36,    78,    78,    52,    77,   285,    72,    77,   327,
      75,   276,   285,   285,    78,   223,   224,   103,    22,   138,
     150,   229,    63,    74,   154,    79,    84,    77,    79,   148,
      76,   161,    90,    91,    92,    93,    94,    95,    96,    97,
      78,    76,   259,   103,   151,   105,   137,    35,   178,   266,
      55,    27,   105,    55,    45,    46,   273,   105,    40,    35,
      36,    37,    38,    39,   329,    74,    77,    43,    44,    74,
      79,    53,    74,   203,    35,    63,    74,   285,    42,    43,
      44,    79,    58,    77,    72,    61,    62,    63,     3,     3,
       5,   151,    68,    69,     9,   225,    72,    74,   151,    78,
      77,   161,    78,   151,   279,   322,    70,    52,    72,    54,
     327,    75,   185,   186,   187,   188,    74,    74,    74,    77,
      77,    77,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   313,    78,
     315,    47,    48,    74,   204,   229,    77,   207,    78,    74,
     280,   281,    77,    59,    60,    74,   331,    76,   333,    72,
     335,   336,    72,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    63,    64,    65,    53,   234,    72,   236,    61,
      62,    73,    74,    23,    72,    25,    26,    72,   318,    29,
      30,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      55,   285,    73,    74,    73,    74,    73,    74,    73,    74,
      56,    23,    57,    25,    26,    77,   276,    29,    30,    77,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    41,
      73,    74,    73,    74,    73,    74,   294,    73,    74,    79,
      23,   301,    25,    26,    73,    74,    29,    30,    73,    74,
      18,    19,    73,    74,   183,   184,   189,   190,    77,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,   329,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    79,   191,   192,    43,
      44,    51,    58,    73,    35,    63,    72,    36,    77,    35,
      35,    73,    73,    53,    58,    35,    73,    61,    62,    63,
      15,    73,    73,    76,    68,    69,    73,    73,    72,    72,
      75,    35,    13,    77,    78,    79,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    78,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    72,    76,    22,    43,    44,   301,    77,
     211,    68,   146,   105,   228,   294,   234,   236,   180,   182,
     181,    58,    -1,    -1,    61,    62,    63,    -1,    -1,    -1,
      -1,    68,    69,    -1,    -1,    72,    -1,    -1,    -1,    -1,
      77,    78,    79,     3,     4,     5,     6,     7,     8,     9,
      10,    11,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    -1,    23,    -1,    25,    26,    -1,    -1,    29,
      30,    -1,    23,    -1,    25,    26,    -1,    -1,    29,    30,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    -1,    -1,    79,
      43,    44,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    58,    -1,    -1,    61,    62,
      63,    -1,    -1,    -1,    -1,    68,    69,    -1,    -1,    72,
      -1,    -1,    -1,    -1,    77,     3,     4,     5,     6,     7,
       8,     9,    10,    11,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    -1,    23,    -1,    25,    26,    -1,
      -1,    29,    30,    -1,    23,    -1,    25,    26,    -1,    -1,
      29,    30,    -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    41,    -1,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    73,    25,    26,    27,    -1,
      29,    30,    -1,    -1,    73,    -1,    35,    36,    37,    38,
      39,    -1,    -1,    -1,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    61,    62,    63,    -1,    -1,    -1,    -1,    68,
      69,    -1,    12,    72,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    -1,    -1,    -1,    -1,    27,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,
      -1,    61,    62,    63,    -1,    -1,    -1,    -1,    68,    69,
      -1,    12,    72,    14,    15,    16,    17,    77,    78,    20,
      21,    22,    -1,    -1,    -1,    -1,    27,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    35,    36,    37,    38,    39,    27,
      -1,    -1,    43,    44,    -1,    -1,    -1,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    43,    44,    58,    -1,    -1,
      61,    62,    63,    -1,    -1,    -1,    -1,    68,    69,    -1,
      58,    72,    -1,    61,    62,    63,    77,    78,    -1,    -1,
      68,    69,    -1,    -1,    72,    27,    -1,    -1,    -1,    -1,
      78,    79,    -1,    35,    36,    37,    38,    39,    27,    -1,
      -1,    43,    44,    -1,    -1,    -1,    35,    36,    37,    38,
      39,    -1,    -1,    -1,    43,    44,    58,    -1,    -1,    61,
      62,    63,    -1,    -1,    -1,    -1,    68,    69,    -1,    58,
      72,    -1,    61,    62,    63,    77,    -1,    -1,    -1,    68,
      69,    27,    -1,    72,    -1,    -1,    -1,    -1,    77,    35,
      36,    37,    38,    39,    27,    -1,    -1,    43,    44,    -1,
      -1,    -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      43,    44,    58,    -1,    -1,    61,    62,    63,    -1,    -1,
      -1,    -1,    68,    69,    -1,    58,    72,    73,    61,    62,
      63,    -1,    -1,    27,    -1,    68,    69,    -1,    -1,    72,
      73,    35,    36,    37,    38,    39,    27,    -1,    -1,    43,
      44,    -1,    -1,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    43,    44,    58,    -1,    -1,    61,    62,    63,
      -1,    -1,    -1,    -1,    68,    69,    -1,    58,    72,    -1,
      61,    62,    63,    -1,    -1,    -1,    -1,    68,    69,    -1,
      -1,    72,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    23,    -1,    25,    26,    -1,    -1,    29,    30,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      23,    24,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    23,    -1,    25,    26,    -1,    28,    29,    30,
      31,    32,    33,    34,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    -1,    25,    26,    -1,    -1,
      29,    30
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      23,    24,    25,    26,    28,    29,    30,    31,    32,    33,
      34,    81,    82,    83,    84,    85,    92,   100,   101,   102,
     103,   104,   107,     3,     3,     7,     9,     3,     5,     9,
       3,    35,    78,   101,   104,   107,    35,    78,    35,    78,
       0,    83,    78,   110,   101,    35,    77,    93,   101,   102,
      77,    77,     3,    78,   101,   105,   106,    95,    78,    35,
     108,   109,    78,   105,    12,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    27,    35,    36,    37,    38,    39,
      43,    44,    58,    61,    62,    63,    68,    69,    72,    77,
      79,    92,   101,   103,   110,   111,   112,   113,   114,   115,
     119,   121,   125,   126,   127,   128,   129,   131,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   145,    72,
      35,    63,    72,    96,    97,    35,    77,    94,   105,    77,
      96,    79,   106,    96,   108,    53,    74,    79,   105,    79,
      72,    72,    72,   124,    72,   126,    55,    77,    77,    77,
     126,    72,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   126,   101,    79,   112,    74,    77,    52,    54,    51,
      56,    57,    58,    49,    50,    47,    48,    59,    60,    45,
      46,    61,    62,    63,    64,    65,    40,    53,    42,    43,
      44,    70,    72,    75,    86,    87,    53,    72,    75,    35,
      63,    74,    77,    72,    96,    79,    77,    77,    79,    36,
     109,    79,   126,    92,   114,   122,   113,   126,    55,   118,
      77,   101,    73,   127,   130,   126,   132,   134,   141,   135,
     136,   137,   137,   138,   138,   138,   138,   139,   139,   140,
     140,   141,   141,   141,   127,   127,    35,    35,   143,   144,
     126,    41,    90,    91,   101,    73,    78,    98,   127,    73,
      90,    36,    76,    53,    35,    97,    88,    89,    77,    73,
     114,   114,   126,    15,    73,   118,   110,   114,   115,   120,
     121,   125,    73,   131,    55,   133,    73,   127,   146,    76,
      73,    74,    35,    63,    98,    99,    73,    76,    98,    73,
      90,    73,   113,    73,   126,    73,   126,    73,    72,   116,
     128,    73,    74,    41,    91,    75,    35,    74,    79,    72,
      73,    13,   113,    73,   113,    73,   123,   126,    78,   127,
      36,    76,    79,    98,    73,    90,   113,   113,   113,   113,
      73,   117,   119,    76,    73,    77,    79,   119
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    80,    81,    82,    82,    83,    83,    84,    86,    85,
      87,    85,    88,    85,    89,    85,    90,    90,    90,    90,
      91,    91,    91,    91,    91,    93,    92,    94,    92,    92,
      92,    95,    92,    92,    92,    96,    96,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    98,    98,    98,
      99,    99,   100,   100,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   102,   102,   102,   102,   102,
     103,   103,   104,   104,   104,   104,   104,   104,   105,   105,
     106,   106,   107,   107,   107,   108,   108,   109,   109,   110,
     110,   111,   111,   112,   112,   113,   113,   113,   113,   113,
     113,   114,   114,   115,   115,   116,   115,   117,   117,   118,
     118,   119,   119,   120,   120,   120,   120,   120,   122,   123,
     121,   124,   121,   121,   121,   121,   121,   125,   125,   125,
     125,   126,   126,   127,   127,   127,   128,   128,   129,   130,
     129,   131,   132,   131,   133,   133,   134,   134,   135,   135,
     136,   136,   136,   137,   137,   137,   137,   137,   138,   138,
     138,   139,   139,   139,   140,   140,   140,   140,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   142,
     142,   143,   142,   144,   142,   142,   142,   142,   142,   145,
     145,   145,   145,   145,   145,   146,   146
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     2,     0,     6,
       0,     5,     0,     7,     0,     6,     1,     3,     1,     3,
       2,     3,     4,     5,     1,     0,     4,     0,     5,     2,
       3,     0,     5,     2,     2,     1,     3,     1,     3,     2,
       4,     4,     3,     4,     3,     7,     6,     1,     3,     4,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     5,     4,     2,     5,     4,     2,     1,     2,
       3,     2,     5,     4,     2,     1,     3,     1,     3,     2,
       3,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     5,     7,     0,     8,     1,     2,     0,
       2,     4,     3,     1,     1,     1,     1,     1,     0,     0,
       7,     0,     8,     6,     7,     6,     7,     2,     3,     2,
       2,     1,     3,     1,     3,     3,     1,     5,     1,     0,
       4,     1,     0,     4,     1,     3,     1,     3,     1,     3,
       1,     3,     3,     1,     3,     3,     3,     3,     1,     3,
       3,     1,     3,     3,     1,     3,     3,     3,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     4,     1,
       4,     0,     4,     0,     5,     3,     3,     2,     2,     1,
       1,     1,     1,     1,     3,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: translation_unit  */
#line 1068 "cl_parser.y"
                           { (yyval.node_idx) = new_node("program \xe2\x86\x92 translation_unit"); }
#line 2793 "y.tab.c"
    break;

  case 3: /* translation_unit: external_decl  */
#line 1071 "cl_parser.y"
                    { (yyval.node_idx) = new_node("translation_unit \xe2\x86\x92 external_decl"); }
#line 2799 "y.tab.c"
    break;

  case 4: /* translation_unit: translation_unit external_decl  */
#line 1072 "cl_parser.y"
                                     { (yyval.node_idx) = new_node("translation_unit \xe2\x86\x92 translation_unit external_decl"); }
#line 2805 "y.tab.c"
    break;

  case 5: /* external_decl: function_def  */
#line 1075 "cl_parser.y"
                    { (yyval.node_idx) = new_node("external_decl \xe2\x86\x92 function_def"); }
#line 2811 "y.tab.c"
    break;

  case 6: /* external_decl: declaration  */
#line 1076 "cl_parser.y"
                    { cnt_global_decls++; (yyval.node_idx) = new_node("external_decl \xe2\x86\x92 declaration"); }
#line 2817 "y.tab.c"
    break;

  case 7: /* function_def: function_header compound_stmt  */
#line 1081 "cl_parser.y"
        { cnt_func_defs++; ir_emit("end_func","","","");
          sem_end_function();
          (yyval.node_idx) = new_node("function_def \xe2\x86\x92 function_header compound_stmt"); }
#line 2825 "y.tab.c"
    break;

  case 8: /* $@1: %empty  */
#line 1087 "cl_parser.y"
        { sem_begin_function((yyvsp[-1].str_val), sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]); }
#line 2831 "y.tab.c"
    break;

  case 9: /* function_header: type_spec IDENTIFIER '(' $@1 param_list ')'  */
#line 1089 "cl_parser.y"
        { strncpy(ir_cur_func,(yyvsp[-4].str_val)?(yyvsp[-4].str_val):"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(ir_get_argcount((yyvsp[-1].node_idx)));
          (yyval.node_idx) = new_node("function_header \xe2\x86\x92 type_spec IDENTIFIER ( param_list )"); }
#line 2839 "y.tab.c"
    break;

  case 10: /* $@2: %empty  */
#line 1093 "cl_parser.y"
        { sem_begin_function((yyvsp[-1].str_val), sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]); }
#line 2845 "y.tab.c"
    break;

  case 11: /* function_header: type_spec IDENTIFIER '(' $@2 ')'  */
#line 1095 "cl_parser.y"
        { strncpy(ir_cur_func,(yyvsp[-3].str_val)?(yyvsp[-3].str_val):"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(0);
          (yyval.node_idx) = new_node("function_header \xe2\x86\x92 type_spec IDENTIFIER ( )"); }
#line 2853 "y.tab.c"
    break;

  case 12: /* $@3: %empty  */
#line 1099 "cl_parser.y"
        { sem_begin_function((yyvsp[-1].str_val), sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]); }
#line 2859 "y.tab.c"
    break;

  case 13: /* function_header: storage_class_list type_spec IDENTIFIER '(' $@3 param_list ')'  */
#line 1101 "cl_parser.y"
        { strncpy(ir_cur_func,(yyvsp[-4].str_val)?(yyvsp[-4].str_val):"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(ir_get_argcount((yyvsp[-1].node_idx)));
          (yyval.node_idx) = new_node("function_header \xe2\x86\x92 storage_class_list type_spec IDENTIFIER ( param_list )"); }
#line 2867 "y.tab.c"
    break;

  case 14: /* $@4: %empty  */
#line 1105 "cl_parser.y"
        { sem_begin_function((yyvsp[-1].str_val), sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]); }
#line 2873 "y.tab.c"
    break;

  case 15: /* function_header: storage_class_list type_spec IDENTIFIER '(' $@4 ')'  */
#line 1107 "cl_parser.y"
        { strncpy(ir_cur_func,(yyvsp[-3].str_val)?(yyvsp[-3].str_val):"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(0);
          (yyval.node_idx) = new_node("function_header \xe2\x86\x92 storage_class_list type_spec IDENTIFIER ( )"); }
#line 2881 "y.tab.c"
    break;

  case 16: /* param_list: param_decl  */
#line 1114 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list \xe2\x86\x92 param_decl"); ir_set_argcount((yyval.node_idx), 1); }
#line 2887 "y.tab.c"
    break;

  case 17: /* param_list: param_list ',' param_decl  */
#line 1116 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list \xe2\x86\x92 param_list , param_decl"); ir_set_argcount((yyval.node_idx), ir_get_argcount((yyvsp[-2].node_idx))+1); }
#line 2893 "y.tab.c"
    break;

  case 18: /* param_list: ELLIPSIS  */
#line 1118 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list \xe2\x86\x92 ..."); ir_set_argcount((yyval.node_idx), 0); }
#line 2899 "y.tab.c"
    break;

  case 19: /* param_list: param_list ',' ELLIPSIS  */
#line 1120 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list \xe2\x86\x92 param_list , ..."); ir_set_argcount((yyval.node_idx), ir_get_argcount((yyvsp[-2].node_idx))); }
#line 2905 "y.tab.c"
    break;

  case 20: /* param_decl: type_spec IDENTIFIER  */
#line 1124 "cl_parser.y"
        { sem_declare_object((yyvsp[0].str_val), sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)]);
          (yyval.node_idx) = new_node("param_decl \xe2\x86\x92 type_spec IDENTIFIER"); }
#line 2912 "y.tab.c"
    break;

  case 21: /* param_decl: type_spec '*' IDENTIFIER  */
#line 1127 "cl_parser.y"
        { cnt_ptr_decls++;
          sem_declare_object((yyvsp[0].str_val), sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)] + 1);
          (yyval.node_idx) = new_node("param_decl \xe2\x86\x92 type_spec * IDENTIFIER"); }
#line 2920 "y.tab.c"
    break;

  case 22: /* param_decl: type_spec IDENTIFIER '[' ']'  */
#line 1131 "cl_parser.y"
        { sem_declare_object((yyvsp[-2].str_val), sem_base_node[(yyvsp[-3].node_idx)], sem_ptr_node[(yyvsp[-3].node_idx)] + 1);
          (yyval.node_idx) = new_node("param_decl \xe2\x86\x92 type_spec IDENTIFIER [ ]"); }
#line 2927 "y.tab.c"
    break;

  case 23: /* param_decl: type_spec IDENTIFIER '[' INT_CONST ']'  */
#line 1134 "cl_parser.y"
        { sem_declare_object((yyvsp[-3].str_val), sem_base_node[(yyvsp[-4].node_idx)], sem_ptr_node[(yyvsp[-4].node_idx)] + 1);
          (yyval.node_idx) = new_node("param_decl \xe2\x86\x92 type_spec IDENTIFIER [ INT_CONST ]"); }
#line 2934 "y.tab.c"
    break;

  case 24: /* param_decl: type_spec  */
#line 1136 "cl_parser.y"
                                        { (yyval.node_idx) = new_node("param_decl \xe2\x86\x92 type_spec"); }
#line 2940 "y.tab.c"
    break;

  case 25: /* $@5: %empty  */
#line 1142 "cl_parser.y"
        { sem_begin_decl(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]); }
#line 2946 "y.tab.c"
    break;

  case 26: /* declaration: type_spec $@5 declarator_list ';'  */
#line 1143 "cl_parser.y"
                          { (yyval.node_idx) = new_node("declaration \xe2\x86\x92 type_spec declarator_list ;"); }
#line 2952 "y.tab.c"
    break;

  case 27: /* $@6: %empty  */
#line 1145 "cl_parser.y"
        { sem_begin_decl(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]); }
#line 2958 "y.tab.c"
    break;

  case 28: /* declaration: storage_class_list type_spec $@6 declarator_list ';'  */
#line 1146 "cl_parser.y"
                          { (yyval.node_idx) = new_node("declaration \xe2\x86\x92 storage_class_list type_spec declarator_list ;"); }
#line 2964 "y.tab.c"
    break;

  case 29: /* declaration: type_spec ';'  */
#line 1147 "cl_parser.y"
                                                       { (yyval.node_idx) = new_node("declaration \xe2\x86\x92 type_spec ;"); }
#line 2970 "y.tab.c"
    break;

  case 30: /* declaration: storage_class_list type_spec ';'  */
#line 1148 "cl_parser.y"
                                                       { (yyval.node_idx) = new_node("declaration \xe2\x86\x92 storage_class_list type_spec ;"); }
#line 2976 "y.tab.c"
    break;

  case 31: /* $@7: %empty  */
#line 1150 "cl_parser.y"
        { sem_begin_decl(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]); }
#line 2982 "y.tab.c"
    break;

  case 32: /* declaration: TYPEDEF type_spec $@7 declarator_list ';'  */
#line 1151 "cl_parser.y"
                          { (yyval.node_idx) = new_node("declaration \xe2\x86\x92 typedef type_spec declarator_list ;"); }
#line 2988 "y.tab.c"
    break;

  case 33: /* declaration: struct_decl ';'  */
#line 1152 "cl_parser.y"
                        { cnt_struct_decls++; (yyval.node_idx) = new_node("declaration \xe2\x86\x92 struct_decl ;"); }
#line 2994 "y.tab.c"
    break;

  case 34: /* declaration: enum_decl ';'  */
#line 1153 "cl_parser.y"
                        { (yyval.node_idx) = new_node("declaration \xe2\x86\x92 enum_decl ;"); }
#line 3000 "y.tab.c"
    break;

  case 35: /* declarator_list: declarator  */
#line 1156 "cl_parser.y"
                                        { (yyval.node_idx) = new_node("declarator_list \xe2\x86\x92 declarator"); }
#line 3006 "y.tab.c"
    break;

  case 36: /* declarator_list: declarator_list ',' declarator  */
#line 1157 "cl_parser.y"
                                        { (yyval.node_idx) = new_node("declarator_list \xe2\x86\x92 declarator_list , declarator"); }
#line 3012 "y.tab.c"
    break;

  case 37: /* declarator: IDENTIFIER  */
#line 1161 "cl_parser.y"
        { sem_declare_object((yyvsp[0].str_val), sem_decl_base, sem_decl_ptr);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 IDENTIFIER"); }
#line 3019 "y.tab.c"
    break;

  case 38: /* declarator: IDENTIFIER '=' initializer  */
#line 1164 "cl_parser.y"
        { sem_declare_object((yyvsp[-2].str_val), sem_decl_base, sem_decl_ptr);
          if (!((sem_is_numeric(sem_decl_base, sem_decl_ptr) && sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)])) ||
                sem_types_match_exact(sem_decl_base, sem_decl_ptr, sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]) ||
                (sem_decl_ptr > 0 && sem_is_null_literal((yyvsp[0].node_idx))))) {
              char lhs_buf[32], rhs_buf[32];
              sem_type_to_string(sem_decl_base, sem_decl_ptr, lhs_buf, sizeof lhs_buf);
              sem_type_to_string(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)], rhs_buf, sizeof rhs_buf);
              sem_errorf("cannot initialize '%s' of type %s with expression of type %s", (yyvsp[-2].str_val), lhs_buf, rhs_buf);
          }
          const char *val=ir_getplace((yyvsp[0].node_idx));
          if(val[0]) ir_emit("=",val,"",ir_intern((yyvsp[-2].str_val)));
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 IDENTIFIER = initializer"); }
#line 3036 "y.tab.c"
    break;

  case 39: /* declarator: '*' IDENTIFIER  */
#line 1177 "cl_parser.y"
        { cnt_ptr_decls++; sem_declare_object((yyvsp[0].str_val), sem_decl_base, sem_decl_ptr + 1);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 * IDENTIFIER"); }
#line 3043 "y.tab.c"
    break;

  case 40: /* declarator: '*' IDENTIFIER '=' initializer  */
#line 1180 "cl_parser.y"
        { cnt_ptr_decls++;
          sem_declare_object((yyvsp[-2].str_val), sem_decl_base, sem_decl_ptr + 1);
          if (!(sem_types_match_exact(sem_decl_base, sem_decl_ptr + 1, sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]) ||
                ((sem_decl_ptr + 1) > 0 && sem_is_null_literal((yyvsp[0].node_idx))))) {
              char lhs_buf[32], rhs_buf[32];
              sem_type_to_string(sem_decl_base, sem_decl_ptr + 1, lhs_buf, sizeof lhs_buf);
              sem_type_to_string(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)], rhs_buf, sizeof rhs_buf);
              sem_errorf("cannot initialize '%s' of type %s with expression of type %s", (yyvsp[-2].str_val), lhs_buf, rhs_buf);
          }
          const char *val=ir_getplace((yyvsp[0].node_idx));
          if(val[0]) ir_emit("=",val,"",ir_intern((yyvsp[-2].str_val)));
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 * IDENTIFIER = initializer"); }
#line 3060 "y.tab.c"
    break;

  case 41: /* declarator: IDENTIFIER '[' INT_CONST ']'  */
#line 1193 "cl_parser.y"
        { sem_declare_object((yyvsp[-3].str_val), sem_decl_base, sem_decl_ptr + 1);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 IDENTIFIER [ INT_CONST ]"); }
#line 3067 "y.tab.c"
    break;

  case 42: /* declarator: IDENTIFIER '[' ']'  */
#line 1196 "cl_parser.y"
        { sem_declare_object((yyvsp[-2].str_val), sem_decl_base, sem_decl_ptr + 1);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 IDENTIFIER [ ]"); }
#line 3074 "y.tab.c"
    break;

  case 43: /* declarator: IDENTIFIER '(' param_list ')'  */
#line 1199 "cl_parser.y"
        { sem_declare_function((yyvsp[-3].str_val), sem_decl_base, sem_decl_ptr, ir_get_argcount((yyvsp[-1].node_idx)), 0);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 IDENTIFIER ( param_list )"); }
#line 3081 "y.tab.c"
    break;

  case 44: /* declarator: IDENTIFIER '(' ')'  */
#line 1202 "cl_parser.y"
        { sem_declare_function((yyvsp[-2].str_val), sem_decl_base, sem_decl_ptr, 0, 0);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 IDENTIFIER ( )"); }
#line 3088 "y.tab.c"
    break;

  case 45: /* declarator: '(' '*' IDENTIFIER ')' '(' param_list ')'  */
#line 1205 "cl_parser.y"
        { sem_declare_object((yyvsp[-4].str_val), sem_decl_base, sem_decl_ptr + 1);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 ( * IDENTIFIER ) ( param_list )"); }
#line 3095 "y.tab.c"
    break;

  case 46: /* declarator: '(' '*' IDENTIFIER ')' '(' ')'  */
#line 1208 "cl_parser.y"
        { sem_declare_object((yyvsp[-3].str_val), sem_decl_base, sem_decl_ptr + 1);
          (yyval.node_idx) = new_node("declarator \xe2\x86\x92 ( * IDENTIFIER ) ( )"); }
#line 3102 "y.tab.c"
    break;

  case 47: /* initializer: assignment_expr  */
#line 1213 "cl_parser.y"
        { (yyval.node_idx) = new_node("initializer \xe2\x86\x92 assignment_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 3110 "y.tab.c"
    break;

  case 48: /* initializer: '{' initializer_list '}'  */
#line 1216 "cl_parser.y"
                               { (yyval.node_idx) = new_node("initializer \xe2\x86\x92 { initializer_list }"); }
#line 3116 "y.tab.c"
    break;

  case 49: /* initializer: '{' initializer_list ',' '}'  */
#line 1217 "cl_parser.y"
                                   { (yyval.node_idx) = new_node("initializer \xe2\x86\x92 { initializer_list , }"); }
#line 3122 "y.tab.c"
    break;

  case 50: /* initializer_list: initializer  */
#line 1220 "cl_parser.y"
                                            { (yyval.node_idx) = new_node("initializer_list \xe2\x86\x92 initializer"); }
#line 3128 "y.tab.c"
    break;

  case 51: /* initializer_list: initializer_list ',' initializer  */
#line 1221 "cl_parser.y"
                                            { (yyval.node_idx) = new_node("initializer_list \xe2\x86\x92 initializer_list , initializer"); }
#line 3134 "y.tab.c"
    break;

  case 52: /* type_qualifier: CONST  */
#line 1225 "cl_parser.y"
               { (yyval.node_idx) = new_node("type_qualifier \xe2\x86\x92 const"); }
#line 3140 "y.tab.c"
    break;

  case 53: /* type_qualifier: VOLATILE  */
#line 1226 "cl_parser.y"
               { (yyval.node_idx) = new_node("type_qualifier \xe2\x86\x92 volatile"); }
#line 3146 "y.tab.c"
    break;

  case 54: /* type_spec: INT  */
#line 1229 "cl_parser.y"
           { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 int"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3152 "y.tab.c"
    break;

  case 55: /* type_spec: FLOAT  */
#line 1230 "cl_parser.y"
             { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 float"); sem_set_node((yyval.node_idx), SEM_FLOAT, 0, 0); }
#line 3158 "y.tab.c"
    break;

  case 56: /* type_spec: CHAR  */
#line 1231 "cl_parser.y"
           { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 char"); sem_set_node((yyval.node_idx), SEM_CHAR, 0, 0); }
#line 3164 "y.tab.c"
    break;

  case 57: /* type_spec: VOID  */
#line 1232 "cl_parser.y"
             { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 void"); sem_set_node((yyval.node_idx), SEM_VOID, 0, 0); }
#line 3170 "y.tab.c"
    break;

  case 58: /* type_spec: DOUBLE  */
#line 1233 "cl_parser.y"
             { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 double"); sem_set_node((yyval.node_idx), SEM_FLOAT, 0, 0); }
#line 3176 "y.tab.c"
    break;

  case 59: /* type_spec: SHORT  */
#line 1234 "cl_parser.y"
            { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 short"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3182 "y.tab.c"
    break;

  case 60: /* type_spec: LONG  */
#line 1235 "cl_parser.y"
           { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 long"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3188 "y.tab.c"
    break;

  case 61: /* type_spec: UNSIGNED  */
#line 1236 "cl_parser.y"
               { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 unsigned"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3194 "y.tab.c"
    break;

  case 62: /* type_spec: SIGNED  */
#line 1237 "cl_parser.y"
             { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 signed"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3200 "y.tab.c"
    break;

  case 63: /* type_spec: type_qualifier type_spec  */
#line 1238 "cl_parser.y"
                               { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 type_qualifier type_spec"); sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 3206 "y.tab.c"
    break;

  case 64: /* type_spec: SHORT INT  */
#line 1239 "cl_parser.y"
                { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 short int"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3212 "y.tab.c"
    break;

  case 65: /* type_spec: LONG INT  */
#line 1240 "cl_parser.y"
                { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 long int"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3218 "y.tab.c"
    break;

  case 66: /* type_spec: LONG DOUBLE  */
#line 1241 "cl_parser.y"
                  { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 long double"); sem_set_node((yyval.node_idx), SEM_FLOAT, 0, 0); }
#line 3224 "y.tab.c"
    break;

  case 67: /* type_spec: UNSIGNED INT  */
#line 1242 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 unsigned int"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3230 "y.tab.c"
    break;

  case 68: /* type_spec: UNSIGNED LONG  */
#line 1243 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 unsigned long"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3236 "y.tab.c"
    break;

  case 69: /* type_spec: UNSIGNED CHAR  */
#line 1244 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 unsigned char"); sem_set_node((yyval.node_idx), SEM_CHAR, 0, 0); }
#line 3242 "y.tab.c"
    break;

  case 70: /* type_spec: SIGNED INT  */
#line 1245 "cl_parser.y"
                  { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 signed int"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3248 "y.tab.c"
    break;

  case 71: /* type_spec: LONG LONG  */
#line 1246 "cl_parser.y"
                  { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 long long"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3254 "y.tab.c"
    break;

  case 72: /* type_spec: LONG LONG INT  */
#line 1247 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 long long int"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3260 "y.tab.c"
    break;

  case 73: /* type_spec: struct_decl  */
#line 1248 "cl_parser.y"
                  { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 struct_decl"); sem_set_node((yyval.node_idx), SEM_STRUCT, 0, 0); }
#line 3266 "y.tab.c"
    break;

  case 74: /* type_spec: enum_decl  */
#line 1249 "cl_parser.y"
                  { (yyval.node_idx) = new_node("type_spec \xe2\x86\x92 enum_decl"); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 3272 "y.tab.c"
    break;

  case 75: /* storage_class_spec: AUTO  */
#line 1252 "cl_parser.y"
           { (yyval.node_idx) = new_node("storage_class_spec \xe2\x86\x92 auto"); }
#line 3278 "y.tab.c"
    break;

  case 76: /* storage_class_spec: REGISTER  */
#line 1253 "cl_parser.y"
               { (yyval.node_idx) = new_node("storage_class_spec \xe2\x86\x92 register"); }
#line 3284 "y.tab.c"
    break;

  case 77: /* storage_class_spec: STATIC  */
#line 1254 "cl_parser.y"
             { (yyval.node_idx) = new_node("storage_class_spec \xe2\x86\x92 static"); }
#line 3290 "y.tab.c"
    break;

  case 78: /* storage_class_spec: EXTERN  */
#line 1255 "cl_parser.y"
             { (yyval.node_idx) = new_node("storage_class_spec \xe2\x86\x92 extern"); }
#line 3296 "y.tab.c"
    break;

  case 79: /* storage_class_spec: INLINE  */
#line 1256 "cl_parser.y"
             { (yyval.node_idx) = new_node("storage_class_spec \xe2\x86\x92 inline"); }
#line 3302 "y.tab.c"
    break;

  case 80: /* storage_class_list: storage_class_spec  */
#line 1259 "cl_parser.y"
                         { (yyval.node_idx) = new_node("storage_class_list \xe2\x86\x92 storage_class_spec"); }
#line 3308 "y.tab.c"
    break;

  case 81: /* storage_class_list: storage_class_list storage_class_spec  */
#line 1260 "cl_parser.y"
                                            { (yyval.node_idx) = new_node("storage_class_list \xe2\x86\x92 storage_class_list storage_class_spec"); }
#line 3314 "y.tab.c"
    break;

  case 82: /* struct_decl: STRUCT IDENTIFIER '{' member_decl_list '}'  */
#line 1264 "cl_parser.y"
                                                 { cnt_struct_decls++; (yyval.node_idx) = new_node("struct_decl \xe2\x86\x92 struct IDENTIFIER { member_decl_list }"); }
#line 3320 "y.tab.c"
    break;

  case 83: /* struct_decl: STRUCT '{' member_decl_list '}'  */
#line 1265 "cl_parser.y"
                                                  { cnt_struct_decls++; (yyval.node_idx) = new_node("struct_decl \xe2\x86\x92 struct { member_decl_list }"); }
#line 3326 "y.tab.c"
    break;

  case 84: /* struct_decl: STRUCT IDENTIFIER  */
#line 1266 "cl_parser.y"
                                                  { (yyval.node_idx) = new_node("struct_decl \xe2\x86\x92 struct IDENTIFIER"); }
#line 3332 "y.tab.c"
    break;

  case 85: /* struct_decl: UNION IDENTIFIER '{' member_decl_list '}'  */
#line 1267 "cl_parser.y"
                                                  { (yyval.node_idx) = new_node("struct_decl \xe2\x86\x92 union IDENTIFIER { member_decl_list }"); }
#line 3338 "y.tab.c"
    break;

  case 86: /* struct_decl: UNION '{' member_decl_list '}'  */
#line 1268 "cl_parser.y"
                                                  { (yyval.node_idx) = new_node("struct_decl \xe2\x86\x92 union { member_decl_list }"); }
#line 3344 "y.tab.c"
    break;

  case 87: /* struct_decl: UNION IDENTIFIER  */
#line 1269 "cl_parser.y"
                                                  { (yyval.node_idx) = new_node("struct_decl \xe2\x86\x92 union IDENTIFIER"); }
#line 3350 "y.tab.c"
    break;

  case 88: /* member_decl_list: member_decl  */
#line 1272 "cl_parser.y"
                                   { (yyval.node_idx) = new_node("member_decl_list \xe2\x86\x92 member_decl"); }
#line 3356 "y.tab.c"
    break;

  case 89: /* member_decl_list: member_decl_list member_decl  */
#line 1273 "cl_parser.y"
                                   { (yyval.node_idx) = new_node("member_decl_list \xe2\x86\x92 member_decl_list member_decl"); }
#line 3362 "y.tab.c"
    break;

  case 90: /* member_decl: type_spec declarator_list ';'  */
#line 1276 "cl_parser.y"
                                    { (yyval.node_idx) = new_node("member_decl \xe2\x86\x92 type_spec declarator_list ;"); }
#line 3368 "y.tab.c"
    break;

  case 91: /* member_decl: type_spec ';'  */
#line 1277 "cl_parser.y"
                                    { (yyval.node_idx) = new_node("member_decl \xe2\x86\x92 type_spec ;"); }
#line 3374 "y.tab.c"
    break;

  case 92: /* enum_decl: ENUM IDENTIFIER '{' enumerator_list '}'  */
#line 1280 "cl_parser.y"
                                              { (yyval.node_idx) = new_node("enum_decl \xe2\x86\x92 enum IDENTIFIER { enumerator_list }"); }
#line 3380 "y.tab.c"
    break;

  case 93: /* enum_decl: ENUM '{' enumerator_list '}'  */
#line 1281 "cl_parser.y"
                                              { (yyval.node_idx) = new_node("enum_decl \xe2\x86\x92 enum { enumerator_list }"); }
#line 3386 "y.tab.c"
    break;

  case 94: /* enum_decl: ENUM IDENTIFIER  */
#line 1282 "cl_parser.y"
                                              { (yyval.node_idx) = new_node("enum_decl \xe2\x86\x92 enum IDENTIFIER"); }
#line 3392 "y.tab.c"
    break;

  case 95: /* enumerator_list: enumerator  */
#line 1285 "cl_parser.y"
                                       { (yyval.node_idx) = new_node("enumerator_list \xe2\x86\x92 enumerator"); }
#line 3398 "y.tab.c"
    break;

  case 96: /* enumerator_list: enumerator_list ',' enumerator  */
#line 1286 "cl_parser.y"
                                       { (yyval.node_idx) = new_node("enumerator_list \xe2\x86\x92 enumerator_list , enumerator"); }
#line 3404 "y.tab.c"
    break;

  case 97: /* enumerator: IDENTIFIER  */
#line 1289 "cl_parser.y"
                              { (yyval.node_idx) = new_node("enumerator \xe2\x86\x92 IDENTIFIER"); }
#line 3410 "y.tab.c"
    break;

  case 98: /* enumerator: IDENTIFIER '=' INT_CONST  */
#line 1290 "cl_parser.y"
                               { (yyval.node_idx) = new_node("enumerator \xe2\x86\x92 IDENTIFIER = INT_CONST"); }
#line 3416 "y.tab.c"
    break;

  case 99: /* compound_stmt: '{' '}'  */
#line 1295 "cl_parser.y"
        { int m=ir_mark(); (yyval.node_idx) = new_node("compound_stmt \xe2\x86\x92 { }");
          ir_set_marker((yyval.node_idx),m); }
#line 3423 "y.tab.c"
    break;

  case 100: /* compound_stmt: '{' block_item_list '}'  */
#line 1298 "cl_parser.y"
        { (yyval.node_idx) = new_node("compound_stmt \xe2\x86\x92 { block_item_list }");
          ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-1].node_idx))); }
#line 3430 "y.tab.c"
    break;

  case 101: /* block_item_list: block_item  */
#line 1303 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item_list \xe2\x86\x92 block_item");
          ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3437 "y.tab.c"
    break;

  case 102: /* block_item_list: block_item_list block_item  */
#line 1306 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item_list \xe2\x86\x92 block_item_list block_item");
          ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-1].node_idx))); }
#line 3444 "y.tab.c"
    break;

  case 103: /* block_item: declaration  */
#line 1311 "cl_parser.y"
        { int m=ir_mark(); (yyval.node_idx) = new_node("block_item \xe2\x86\x92 declaration");
          ir_set_marker((yyval.node_idx),m); }
#line 3451 "y.tab.c"
    break;

  case 104: /* block_item: stmt  */
#line 1314 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item \xe2\x86\x92 stmt");
          ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3458 "y.tab.c"
    break;

  case 105: /* stmt: compound_stmt  */
#line 1318 "cl_parser.y"
                     { (yyval.node_idx) = new_node("stmt \xe2\x86\x92 compound_stmt");  ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3464 "y.tab.c"
    break;

  case 106: /* stmt: expr_stmt  */
#line 1319 "cl_parser.y"
                     { (yyval.node_idx) = new_node("stmt \xe2\x86\x92 expr_stmt");      ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3470 "y.tab.c"
    break;

  case 107: /* stmt: selection_stmt  */
#line 1320 "cl_parser.y"
                     { (yyval.node_idx) = new_node("stmt \xe2\x86\x92 selection_stmt"); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3476 "y.tab.c"
    break;

  case 108: /* stmt: iteration_stmt  */
#line 1321 "cl_parser.y"
                     { (yyval.node_idx) = new_node("stmt \xe2\x86\x92 iteration_stmt"); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3482 "y.tab.c"
    break;

  case 109: /* stmt: jump_stmt  */
#line 1322 "cl_parser.y"
                     { (yyval.node_idx) = new_node("stmt \xe2\x86\x92 jump_stmt");      ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3488 "y.tab.c"
    break;

  case 110: /* stmt: labeled_stmt  */
#line 1323 "cl_parser.y"
                     { (yyval.node_idx) = new_node("stmt \xe2\x86\x92 labeled_stmt");   ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3494 "y.tab.c"
    break;

  case 111: /* expr_stmt: ';'  */
#line 1327 "cl_parser.y"
        { int m=ir_mark(); (yyval.node_idx) = new_node("expr_stmt \xe2\x86\x92 ;"); ir_set_marker((yyval.node_idx),m); }
#line 3500 "y.tab.c"
    break;

  case 112: /* expr_stmt: expr ';'  */
#line 1329 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr_stmt \xe2\x86\x92 expr ;");
          ir_setplace((yyval.node_idx), ir_getplace((yyvsp[-1].node_idx)));
          ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[-1].node_idx)));
          ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-1].node_idx))); }
#line 3509 "y.tab.c"
    break;

  case 113: /* selection_stmt: IF '(' expr ')' stmt  */
#line 1341 "cl_parser.y"
        {
            cnt_if_without_else++;
            ifd_cur++; if(ifd_cur>ifd_max)ifd_max=ifd_cur;

            const char *cond  = ir_getplace((yyvsp[-2].node_idx));
            const char *Lend  = ir_newlabel();
            int M_cond = ir_get_marker((yyvsp[-2].node_idx));
            int M_s1   = ir_get_marker((yyvsp[0].node_idx));

            if (M_s1 >= 0) {
                ir_insert_before(M_s1, "jf", cond, "", Lend);
                ir_delete(M_s1 + 1);
            } else {
                ir_emit("jf", cond, "", Lend);
            }
            ir_emit("label", "", "", Lend);
            if (M_cond >= 0) ir_delete(M_cond);

            ifd_cur--;
            (yyval.node_idx) = new_node("selection_stmt \xe2\x86\x92 if ( expr ) stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3536 "y.tab.c"
    break;

  case 114: /* selection_stmt: IF '(' expr ')' stmt ELSE stmt  */
#line 1365 "cl_parser.y"
        {
            cnt_if_with_else++;
            ifd_cur++; if(ifd_cur>ifd_max)ifd_max=ifd_cur;

            const char *cond  = ir_getplace((yyvsp[-4].node_idx));
            const char *Lelse = ir_newlabel();
            const char *Lend  = ir_newlabel();
            int M_cond = ir_get_marker((yyvsp[-4].node_idx));
            int M_s1   = ir_get_marker((yyvsp[-2].node_idx));
            int M_s2   = ir_get_marker((yyvsp[0].node_idx));

            int ms2 = (M_s2 >= 0) ? M_s2 : ir_n;
            int ms1 = (M_s1 >= 0) ? M_s1 : ms2;

            ir_insert_before(ms1, "jf", cond, "", Lelse);
            ms2++;
            if (M_s1 >= 0) { ir_delete(ms1 + 1); ms2--; }

            ir_insert_before(ms2, "goto", "", "", Lend);
            ir_insert_before(ms2 + 1, "label", "", "", Lelse);
            if (M_s2 >= 0) ir_delete(ms2 + 2);

            ir_emit("label", "", "", Lend);
            if (M_cond >= 0) ir_delete(M_cond);

            ifd_cur--;
            (yyval.node_idx) = new_node("selection_stmt \xe2\x86\x92 if ( expr ) stmt else stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3570 "y.tab.c"
    break;

  case 115: /* $@8: %empty  */
#line 1396 "cl_parser.y"
        {
            /*
             * FIX BUG 1 MID-RULE: Push switch context and increment
             * switch_depth so break inside the body is accepted.
             * We allocate Lend here and store it in the switch context.
             */
            const char *Lend = ir_newlabel();
            sw_push(ir_getplace((yyvsp[-1].node_idx)), Lend);
            switch_depth++;
        }
#line 3585 "y.tab.c"
    break;

  case 116: /* selection_stmt: SWITCH '(' expr ')' $@8 '{' case_list '}'  */
#line 1407 "cl_parser.y"
        {
            cnt_switch_stmts++;
            switch_depth--;

            SwContext *sc = sw_cur();
            if (sc) {
                const char *sw_expr = sc->sw_expr;
                const char *Lend    = sc->sw_lend;
                int bsq = sc->body_start_quad;

                /*
                 * INSERT the dispatch chain BEFORE the first case body.
                 * For each non-default case:
                 *   t = (sw_expr == case_val)
                 *   jt t _ Lcase_N
                 * After all value comparisons, jump to default if present,
                 * or to Lend otherwise.
                 * Then emit:   goto default_label  (or goto Lend)
                 */
                int ins = bsq;  /* insertion point — before all case quads */
                const char *t_cmp = ir_newtemp();
                for (int ci = 0; ci < sc->n_cases; ci++) {
                    if (sc->case_is_default[ci]) continue;
                    /* insert:  t_cmp = sw_expr == case_val */
                    ir_insert_before(ins, "==", sw_expr, sc->case_vals[ci], t_cmp);
                    ins++;
                    /* insert:  jt t_cmp _ Lcase_ci */
                    ir_insert_before(ins, "jt", t_cmp, "", sc->case_lbls[ci]);
                    ins++;
                }
                /* Fall-through: goto default or Lend */
                if (sc->has_default && sc->default_idx >= 0) {
                    ir_insert_before(ins, "goto", "", "", sc->case_lbls[sc->default_idx]);
                } else {
                    ir_insert_before(ins, "goto", "", "", Lend);
                }
                ins++;

                /* Patch all unresolved raw break quads → goto Lend */
                for (int _k = ins; _k < ir_n; _k++) {
                    if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0])
                        { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lend,63); }
                }

                /* Emit the end label */
                ir_emit("label","","",Lend);
            }
            sw_pop();

            (yyval.node_idx) = new_node("selection_stmt \xe2\x86\x92 switch ( expr ) { case_list }");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3642 "y.tab.c"
    break;

  case 117: /* case_list: labeled_stmt  */
#line 1463 "cl_parser.y"
        { (yyval.node_idx) = new_node("case_list \xe2\x86\x92 labeled_stmt"); }
#line 3648 "y.tab.c"
    break;

  case 118: /* case_list: case_list labeled_stmt  */
#line 1465 "cl_parser.y"
        { (yyval.node_idx) = new_node("case_list \xe2\x86\x92 case_list labeled_stmt"); }
#line 3654 "y.tab.c"
    break;

  case 119: /* case_body_list: %empty  */
#line 1469 "cl_parser.y"
        { int m=ir_mark(); (yyval.node_idx) = new_node("case_body_list \xe2\x86\x92 <empty>"); ir_set_marker((yyval.node_idx),m); }
#line 3660 "y.tab.c"
    break;

  case 120: /* case_body_list: case_body_list non_case_stmt  */
#line 1471 "cl_parser.y"
        { (yyval.node_idx) = new_node("case_body_list \xe2\x86\x92 case_body_list non_case_stmt"); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-1].node_idx))); }
#line 3666 "y.tab.c"
    break;

  case 121: /* labeled_stmt: CASE expr ':' case_body_list  */
#line 1475 "cl_parser.y"
        {
            /*
             * FIX BUG 1: Register this case with the switch context and
             * emit a label quad so the case body is reachable from the
             * dispatch chain.  The dispatch quads themselves are inserted
             * retroactively by the SWITCH final action above.
             */
            const char *clbl = sw_add_case(ir_getplace((yyvsp[-2].node_idx)));
            /* Insert the case label BEFORE the first quad of the body.
               ir_get_marker($4) is the marker at the start of the body;
               if it is -1 (empty body), just append. */
            int mbody = ir_get_marker((yyvsp[0].node_idx));
            if (mbody >= 0)
                ir_insert_before(mbody, "label","","",clbl);
            else
                ir_emit("label","","",clbl);
            (yyval.node_idx) = new_node("labeled_stmt \xe2\x86\x92 case expr : case_body_list");
            ir_set_marker((yyval.node_idx), ir_get_marker((yyvsp[-2].node_idx)));
        }
#line 3690 "y.tab.c"
    break;

  case 122: /* labeled_stmt: DEFAULT ':' case_body_list  */
#line 1495 "cl_parser.y"
        {
            const char *dlbl = sw_add_default();
            int mbody = ir_get_marker((yyvsp[0].node_idx));
            if (mbody >= 0)
                ir_insert_before(mbody, "label","","",dlbl);
            else
                ir_emit("label","","",dlbl);
            (yyval.node_idx) = new_node("labeled_stmt \xe2\x86\x92 default : case_body_list");
            ir_set_marker((yyval.node_idx), ir_get_marker((yyvsp[0].node_idx)));
        }
#line 3705 "y.tab.c"
    break;

  case 123: /* non_case_stmt: compound_stmt  */
#line 1507 "cl_parser.y"
                     { (yyval.node_idx) = new_node("non_case_stmt \xe2\x86\x92 compound_stmt");  ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3711 "y.tab.c"
    break;

  case 124: /* non_case_stmt: expr_stmt  */
#line 1508 "cl_parser.y"
                     { (yyval.node_idx) = new_node("non_case_stmt \xe2\x86\x92 expr_stmt");      ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3717 "y.tab.c"
    break;

  case 125: /* non_case_stmt: selection_stmt  */
#line 1509 "cl_parser.y"
                     { (yyval.node_idx) = new_node("non_case_stmt \xe2\x86\x92 selection_stmt"); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3723 "y.tab.c"
    break;

  case 126: /* non_case_stmt: iteration_stmt  */
#line 1510 "cl_parser.y"
                     { (yyval.node_idx) = new_node("non_case_stmt \xe2\x86\x92 iteration_stmt"); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3729 "y.tab.c"
    break;

  case 127: /* non_case_stmt: jump_stmt  */
#line 1511 "cl_parser.y"
                     { (yyval.node_idx) = new_node("non_case_stmt \xe2\x86\x92 jump_stmt");      ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx))); }
#line 3735 "y.tab.c"
    break;

  case 128: /* @9: %empty  */
#line 1520 "cl_parser.y"
        {
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            ir_emit("label","","",Lstart);
            loop_push(Lstart, Lend);
            (yyval.node_idx) = (int)(Lstart - (const char *)0);
        }
#line 3747 "y.tab.c"
    break;

  case 129: /* $@10: %empty  */
#line 1528 "cl_parser.y"
        {
            const char *cond = ir_getplace((yyvsp[-1].node_idx));
            const char *Lend = loop_cur_lend();
            if (cond && cond[0] && Lend)
                ir_emit("jf", cond, "", Lend);
        }
#line 3758 "y.tab.c"
    break;

  case 130: /* iteration_stmt: WHILE '(' @9 expr ')' $@10 stmt  */
#line 1535 "cl_parser.y"
        {
            cnt_while_loops++;
            const char *Lstart = loop_cur_lstart();
            const char *Lend   = loop_cur_lend();

            for (int _k = 0; _k < ir_n; _k++) {
                if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lend,63); }
                else if (!strcmp(ir_q[_k].op,"continue") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lstart,63); }
            }

            ir_emit("goto","","",Lstart);
            ir_emit("label","","",Lend);
            loop_pop();

            (yyval.node_idx) = new_node("iteration_stmt \xe2\x86\x92 while ( expr ) stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3782 "y.tab.c"
    break;

  case 131: /* @11: %empty  */
#line 1556 "cl_parser.y"
        {
            /*
             * BUG A FIX: Record ir_n NOW (before any body quads are
             * generated) as the reliable body-start position.
             * M_body from ir_get_marker($3) is unreliable: selection_stmt
             * and iteration_stmt set their markers to -1, causing the old
             * code to use ir_n (end of table) as the insertion point for
             * label Lstart -- which placed the label AFTER the condition
             * evaluation, making the loop condition stale on every
             * iteration, and making the break-scan loop start past all
             * body quads so break was never patched to goto Lend.
             * Storing ir_n here bypasses the broken marker completely.
             *
             * Also push loop labels here so break/continue inside the
             * body (even inside nested if/else) see loop_depth > 0.
             */
            const char *_Lstart = ir_newlabel();
            const char *_Lend   = ir_newlabel();
            loop_push(_Lstart, _Lend);
            (yyval.node_idx) = ir_n;  /* body_start: first body quad will be at this index */
        }
#line 3808 "y.tab.c"
    break;

  case 132: /* iteration_stmt: DO @11 stmt WHILE '(' expr ')' ';'  */
#line 1578 "cl_parser.y"
        {
            cnt_do_while_loops++;
            const char *Lstart = loop_cur_lstart();
            const char *Lend   = loop_cur_lend();
            const char *cond   = ir_getplace((yyvsp[-2].node_idx));

            /*
             * BUG A FIX: Use the mid-rule recorded body_start ($<node_idx>2)
             * instead of ir_get_marker($3).  This is always the correct
             * first-body-quad position regardless of stmt type.
             */
            int body_start = (yyvsp[-6].node_idx);

            /* Insert  label Lstart  at the very beginning of the body */
            ir_insert_before(body_start, "label","","",Lstart);
            /* All body quads shifted right by 1; scan from body_start+1 */
            int scan_from = body_start + 1;

            /* Patch break -> goto Lend, continue -> goto Lstart */
            int had_break = 0;
            for (int _k = scan_from; _k < ir_n; _k++) {
                if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0]) {
                    strncpy(ir_q[_k].op,"goto",23);
                    strncpy(ir_q[_k].result,Lend,63);
                    had_break = 1;
                } else if (!strcmp(ir_q[_k].op,"continue") && !ir_q[_k].arg1[0]) {
                    strncpy(ir_q[_k].op,"goto",23);
                    strncpy(ir_q[_k].result,Lstart,63);
                }
            }

            ir_emit("jt", cond,"",Lstart);
            /*
             * BUG B FIX: Only emit label Lend when a break was actually
             * present.  Without a break, Lend is never referenced, so
             * emitting it creates a dead unreachable label that inflates
             * the label range reported in the IR footer.
             */
            if (had_break) ir_emit("label","","",Lend);
            loop_pop();

            (yyval.node_idx) = new_node("iteration_stmt \xe2\x86\x92 do stmt while ( expr ) ;");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3857 "y.tab.c"
    break;

  case 133: /* iteration_stmt: FOR '(' expr_stmt expr_stmt ')' stmt  */
#line 1624 "cl_parser.y"
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace((yyvsp[-2].node_idx));
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker((yyvsp[-2].node_idx));
            int M_body = ir_get_marker((yyvsp[0].node_idx));

            int mc = (M_cond>=0) ? M_cond : ir_n;
            int mb = (M_body>=0) ? M_body : ir_n;

            ir_insert_before(mc, "label","","", Lstart);
            if (mc <= mb) mb++;

            if (cond && cond[0]) {
                int insert_pt = (mb <= ir_n) ? mb : ir_n;
                ir_insert_before(insert_pt, "jf", cond, "", Lend);
            }

            int body_start = (mb < ir_n) ? mb+1 : ir_n;
            for (int _k = body_start; _k < ir_n; _k++) {
                if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lend,63); }
                else if (!strcmp(ir_q[_k].op,"continue") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lstart,63); }
            }

            ir_emit("goto","","",Lstart);
            ir_emit("label","","",Lend);

            (yyval.node_idx) = new_node("iteration_stmt \xe2\x86\x92 for ( expr_stmt expr_stmt ) stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3895 "y.tab.c"
    break;

  case 134: /* iteration_stmt: FOR '(' expr_stmt expr_stmt expr ')' stmt  */
#line 1659 "cl_parser.y"
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace((yyvsp[-3].node_idx));
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker((yyvsp[-3].node_idx));
            int M_step = ir_get_marker((yyvsp[-2].node_idx));
            int M_body = ir_get_marker((yyvsp[0].node_idx));

            int mc = (M_cond>=0)?M_cond:0;
            int ms = (M_step>=0)?M_step:ir_n;
            int mb = (M_body>=0)?M_body:ir_n;

            int step_count = 0;
            Quad step_buf[256];
            for (int i = ms+1; i < mb && step_count < 256; i++)
                if (!ir_q[i].is_marker) step_buf[step_count++] = ir_q[i];

            int del_count = mb - ms - 1;
            for (int d = 0; d < del_count && ms+1 < ir_n; d++) ir_delete(ms+1);
            mb -= del_count;

            ir_insert_before(mc,"label","","",Lstart);
            if (mc <= mb) mb++;

            if (cond && cond[0]) {
                int insert_pt = (mb <= ir_n) ? mb : ir_n;
                ir_insert_before(insert_pt, "jf", cond, "", Lend);
            }

            int body_start = (mb < ir_n) ? mb+1 : ir_n;
            for (int _k = body_start; _k < ir_n; _k++) {
                if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lend,63); }
                else if (!strcmp(ir_q[_k].op,"continue") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lstart,63); }
            }

            for (int i = 0; i < step_count; i++)
                if (ir_n < IR_MAXQ) ir_q[ir_n++] = step_buf[i];

            ir_emit("goto","","",Lstart);
            ir_emit("label","","",Lend);

            (yyval.node_idx) = new_node("iteration_stmt \xe2\x86\x92 for ( expr_stmt expr_stmt expr ) stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3947 "y.tab.c"
    break;

  case 135: /* iteration_stmt: FOR '(' declaration expr_stmt ')' stmt  */
#line 1708 "cl_parser.y"
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace((yyvsp[-2].node_idx));
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker((yyvsp[-2].node_idx));
            int M_body = ir_get_marker((yyvsp[0].node_idx));

            int mc = (M_cond>=0)?M_cond:ir_n;
            int mb = (M_body>=0)?M_body:ir_n;

            ir_insert_before(mc,"label","","",Lstart);
            if (mc <= mb) mb++;

            if (cond && cond[0]) {
                int insert_pt = (mb <= ir_n) ? mb : ir_n;
                ir_insert_before(insert_pt, "jf", cond, "", Lend);
            }

            int body_start = (mb < ir_n) ? mb+1 : ir_n;
            for (int _k = body_start; _k < ir_n; _k++) {
                if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lend,63); }
                else if (!strcmp(ir_q[_k].op,"continue") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lstart,63); }
            }

            ir_emit("goto","","",Lstart);
            ir_emit("label","","",Lend);

            (yyval.node_idx) = new_node("iteration_stmt \xe2\x86\x92 for ( declaration expr_stmt ) stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 3985 "y.tab.c"
    break;

  case 136: /* iteration_stmt: FOR '(' declaration expr_stmt expr ')' stmt  */
#line 1743 "cl_parser.y"
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace((yyvsp[-3].node_idx));
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker((yyvsp[-3].node_idx));
            int M_step = ir_get_marker((yyvsp[-2].node_idx));
            int M_body = ir_get_marker((yyvsp[0].node_idx));

            int mc = (M_cond>=0)?M_cond:0;
            int ms = (M_step>=0)?M_step:ir_n;
            int mb = (M_body>=0)?M_body:ir_n;

            int step_count = 0;
            Quad step_buf[256];
            for (int i = ms+1; i < mb && step_count < 256; i++)
                if (!ir_q[i].is_marker) step_buf[step_count++] = ir_q[i];

            int del_count = mb - ms - 1;
            for (int d = 0; d < del_count && ms+1 < ir_n; d++) ir_delete(ms+1);
            mb -= del_count;

            ir_insert_before(mc,"label","","",Lstart);
            if (mc <= mb) mb++;

            if (cond && cond[0]) {
                int insert_pt = (mb <= ir_n) ? mb : ir_n;
                ir_insert_before(insert_pt, "jf", cond, "", Lend);
            }

            int body_start = (mb < ir_n) ? mb+1 : ir_n;
            for (int _k = body_start; _k < ir_n; _k++) {
                if (!strcmp(ir_q[_k].op,"break") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lend,63); }
                else if (!strcmp(ir_q[_k].op,"continue") && !ir_q[_k].arg1[0])
                    { strncpy(ir_q[_k].op,"goto",23); strncpy(ir_q[_k].result,Lstart,63); }
            }

            for (int i = 0; i < step_count; i++)
                if (ir_n < IR_MAXQ) ir_q[ir_n++] = step_buf[i];
            ir_emit("goto","","",Lstart);
            ir_emit("label","","",Lend);

            (yyval.node_idx) = new_node("iteration_stmt \xe2\x86\x92 for ( declaration expr_stmt expr ) stmt");
            ir_set_marker((yyval.node_idx), -1);
        }
#line 4036 "y.tab.c"
    break;

  case 137: /* jump_stmt: RETURN ';'  */
#line 1793 "cl_parser.y"
        { cnt_return_stmts++;
          if (!(sem_current_return_base == SEM_VOID && sem_current_return_ptr == 0))
              sem_errorf("non-void function must return a value");
          int m=ir_mark(); ir_emit("return","","","");
          (yyval.node_idx) = new_node("jump_stmt \xe2\x86\x92 return ;"); ir_set_marker((yyval.node_idx),m); }
#line 4046 "y.tab.c"
    break;

  case 138: /* jump_stmt: RETURN expr ';'  */
#line 1799 "cl_parser.y"
        { cnt_return_stmts++;
          if (!((sem_is_numeric(sem_current_return_base, sem_current_return_ptr) &&
                 sem_is_numeric(sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)])) ||
                sem_types_match_exact(sem_current_return_base, sem_current_return_ptr,
                                      sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)]) ||
                (sem_current_return_ptr > 0 && sem_is_null_literal((yyvsp[-1].node_idx))))) {
              char ret_buf[32], expr_buf[32];
              sem_type_to_string(sem_current_return_base, sem_current_return_ptr, ret_buf, sizeof ret_buf);
              sem_type_to_string(sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)], expr_buf, sizeof expr_buf);
              sem_errorf("return type mismatch: function returns %s but expression is %s", ret_buf, expr_buf);
          }
          ir_emit("return",ir_getplace((yyvsp[-1].node_idx)),"","");
          (yyval.node_idx) = new_node("jump_stmt \xe2\x86\x92 return expr ;"); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-1].node_idx))); }
#line 4064 "y.tab.c"
    break;

  case 139: /* jump_stmt: BREAK ';'  */
#line 1813 "cl_parser.y"
        { if (loop_depth <= 0 && switch_depth <= 0) sem_errorf("'break' used outside loop or switch");
          int m=ir_mark(); ir_emit("break","","","");
          (yyval.node_idx) = new_node("jump_stmt \xe2\x86\x92 break ;"); ir_set_marker((yyval.node_idx),m); }
#line 4072 "y.tab.c"
    break;

  case 140: /* jump_stmt: CONTINUE ';'  */
#line 1817 "cl_parser.y"
        { if (loop_depth <= 0) sem_errorf("'continue' used outside loop");
          int m=ir_mark(); ir_emit("continue","","","");
          (yyval.node_idx) = new_node("jump_stmt \xe2\x86\x92 continue ;"); ir_set_marker((yyval.node_idx),m); }
#line 4080 "y.tab.c"
    break;

  case 141: /* expr: assignment_expr  */
#line 1828 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr \xe2\x86\x92 assignment_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4088 "y.tab.c"
    break;

  case 142: /* expr: expr ',' assignment_expr  */
#line 1832 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr \xe2\x86\x92 expr , assignment_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-2].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4096 "y.tab.c"
    break;

  case 143: /* assignment_expr: conditional_expr  */
#line 1839 "cl_parser.y"
        { (yyval.node_idx) = new_node("assignment_expr \xe2\x86\x92 conditional_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4104 "y.tab.c"
    break;

  case 144: /* assignment_expr: unary_expr '=' assignment_expr  */
#line 1843 "cl_parser.y"
        { cnt_assignments++;
          if (!sem_lvalue_node[(yyvsp[-2].node_idx)]) sem_errorf("left-hand side of assignment is not assignable");
          if (!sem_assignment_compatible((yyvsp[-2].node_idx), (yyvsp[0].node_idx))) {
              char lhs_buf[32], rhs_buf[32];
              sem_type_to_string(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)], lhs_buf, sizeof lhs_buf);
              sem_type_to_string(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)], rhs_buf, sizeof rhs_buf);
              sem_errorf("cannot assign expression of type %s to lvalue of type %s", rhs_buf, lhs_buf);
          }
          const char *lhs=ir_getplace((yyvsp[-2].node_idx)),*rhs=ir_getplace((yyvsp[0].node_idx));
          const char *lhs_addr = ir_getplace_addr((yyvsp[-2].node_idx));
          if (lhs_addr && lhs_addr[0]) {
              for (int k = ir_n - 1; k >= 0; k--) {
                  if (!strcmp(ir_q[k].op, "deref") &&
                      !strcmp(ir_q[k].arg1, lhs_addr) &&
                      !strcmp(ir_q[k].result, lhs)) {
                      ir_delete(k);
                      break;
                  }
              }
              ir_emit("store", rhs, "", lhs_addr);
          } else {
              int found_idx = -1;
              for (int k = 0; k < ir_n; k++)
                  if (ir_q[k].result[0] && !strcmp(ir_q[k].result, lhs)) { found_idx = k; }
              if (found_idx >= 0 && !strcmp(ir_q[found_idx].op, "deref")) {
                  const char *addr = ir_q[found_idx].arg1;
                  if (addr && addr[0]) ir_emit("store", rhs, "", addr);
                  else ir_emit("=", rhs, "", lhs);
              } else {
                  ir_emit("=", rhs, "", lhs);
              }
          }
          (yyval.node_idx) = new_node("assignment_expr \xe2\x86\x92 unary_expr = assignment_expr");
          ir_setplace((yyval.node_idx),lhs); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-2].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[-2].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4144 "y.tab.c"
    break;

  case 145: /* assignment_expr: unary_expr ASSIGN_OP assignment_expr  */
#line 1879 "cl_parser.y"
        { cnt_assignments++;
          if (!sem_lvalue_node[(yyvsp[-2].node_idx)]) sem_errorf("left-hand side of assignment is not assignable");
          if (!sem_assignment_compatible((yyvsp[-2].node_idx), (yyvsp[0].node_idx)))
              sem_errorf("compound assignment has incompatible operand types");
          sem_require_numeric_binary((yyvsp[-1].str_val) ? (yyvsp[-1].str_val) : "compound assignment", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          const char *lhs=ir_getplace((yyvsp[-2].node_idx)),*rhs=ir_getplace((yyvsp[0].node_idx));
          const char *optext = (yyvsp[-1].str_val) ? (yyvsp[-1].str_val) : "=";
          char baseop[8]; size_t ol = strlen(optext); if (ol>0 && optext[ol-1]=='=') ol--; if(ol>7) ol=7; strncpy(baseop, optext, ol); baseop[ol]='\0';
          const char *t = ir_newtemp();
          ir_emit(baseop, lhs, rhs, t);
          const char *lhs_addr = ir_getplace_addr((yyvsp[-2].node_idx));
          if (lhs_addr && lhs_addr[0]) {
              ir_emit("store", t, "", lhs_addr);
          } else {
              ir_emit("=", t, "", lhs);
          }
          (yyval.node_idx) = new_node("assignment_expr \xe2\x86\x92 unary_expr ASSIGN_OP assignment_expr");
          ir_setplace((yyval.node_idx),lhs); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-2].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[-2].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4168 "y.tab.c"
    break;

  case 146: /* conditional_expr: logical_or_expr  */
#line 1902 "cl_parser.y"
        { (yyval.node_idx) = new_node("conditional_expr \xe2\x86\x92 logical_or_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4176 "y.tab.c"
    break;

  case 147: /* conditional_expr: logical_or_expr '?' expr ':' conditional_expr  */
#line 1906 "cl_parser.y"
        { const char *cond=ir_getplace((yyvsp[-4].node_idx)),*e1=ir_getplace((yyvsp[-2].node_idx)),*e2=ir_getplace((yyvsp[0].node_idx));
          if (!sem_is_scalar(sem_base_node[(yyvsp[-4].node_idx)], sem_ptr_node[(yyvsp[-4].node_idx)]))
              sem_errorf("conditional operator requires scalar condition");
          if (!(sem_types_match_exact(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)], sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]) ||
                (sem_is_numeric(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]) && sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))))
              sem_errorf("conditional branches must have compatible types");
          const char *t=ir_newtemp(),*lf=ir_newlabel(),*le=ir_newlabel();
          ir_emit("jf",cond,"",lf); ir_emit("=",e1,"",t);
          ir_emit("goto","","",le); ir_emit("label","","",lf);
          ir_emit("=",e2,"",t); ir_emit("label","","",le);
          (yyval.node_idx) = new_node("conditional_expr \xe2\x86\x92 logical_or_expr ? expr : conditional_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-4].node_idx)));
          if (sem_is_numeric(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]) && sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_set_binary_result_numeric((yyval.node_idx), (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          else sem_copy_node((yyval.node_idx), (yyvsp[-2].node_idx));
          sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4197 "y.tab.c"
    break;

  case 148: /* logical_or_expr: logical_and_expr  */
#line 1926 "cl_parser.y"
        { (yyval.node_idx) = new_node("logical_or_expr \xe2\x86\x92 logical_and_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4205 "y.tab.c"
    break;

  case 149: /* @12: %empty  */
#line 1931 "cl_parser.y"
        {
            sc_or_ltrue[sc_or_depth] = ir_newlabel();
            ir_emit("jt", ir_getplace((yyvsp[-1].node_idx)), "", sc_or_ltrue[sc_or_depth]);
            (yyval.node_idx) = sc_or_depth++;
        }
#line 4215 "y.tab.c"
    break;

  case 150: /* logical_or_expr: logical_or_expr OR_OP @12 logical_and_expr  */
#line 1937 "cl_parser.y"
        {
            int d = (yyvsp[-1].node_idx);
            sc_or_depth--;
            const char *Ltrue  = sc_or_ltrue[d];
            const char *Lend   = ir_newlabel();
            const char *t      = ir_newtemp();
            ir_emit("jt",  ir_getplace((yyvsp[0].node_idx)), "", Ltrue);
            ir_emit("=",   "0", "", t);
            ir_emit("goto","",  "", Lend);
            ir_emit("label","", "", Ltrue);
            ir_emit("=",   "1", "", t);
            ir_emit("label","", "", Lend);
            (yyval.node_idx) = new_node("logical_or_expr \xe2\x86\x92 logical_or_expr || logical_and_expr");
            ir_setplace((yyval.node_idx), t);
            ir_set_marker((yyval.node_idx), ir_get_marker((yyvsp[-3].node_idx)));
            if (!sem_is_scalar(sem_base_node[(yyvsp[-3].node_idx)], sem_ptr_node[(yyvsp[-3].node_idx)]) ||
                !sem_is_scalar(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
                sem_errorf("operator '||' requires scalar operands");
            sem_set_node((yyval.node_idx), SEM_INT, 0, 0);
        }
#line 4240 "y.tab.c"
    break;

  case 151: /* logical_and_expr: inclusive_or_expr  */
#line 1961 "cl_parser.y"
        { (yyval.node_idx) = new_node("logical_and_expr \xe2\x86\x92 inclusive_or_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4248 "y.tab.c"
    break;

  case 152: /* @13: %empty  */
#line 1966 "cl_parser.y"
        {
            sc_and_lfalse[sc_and_depth] = ir_newlabel();
            ir_emit("jf", ir_getplace((yyvsp[-1].node_idx)), "", sc_and_lfalse[sc_and_depth]);
            (yyval.node_idx) = sc_and_depth++;
        }
#line 4258 "y.tab.c"
    break;

  case 153: /* logical_and_expr: logical_and_expr AND_OP @13 inclusive_or_expr  */
#line 1972 "cl_parser.y"
        {
            int d = (yyvsp[-1].node_idx);
            sc_and_depth--;
            const char *Lfalse = sc_and_lfalse[d];
            const char *Lend   = ir_newlabel();
            const char *t      = ir_newtemp();
            ir_emit("jf",  ir_getplace((yyvsp[0].node_idx)), "", Lfalse);
            ir_emit("=",   "1", "", t);
            ir_emit("goto","",  "", Lend);
            ir_emit("label","", "", Lfalse);
            ir_emit("=",   "0", "", t);
            ir_emit("label","", "", Lend);
            (yyval.node_idx) = new_node("logical_and_expr \xe2\x86\x92 logical_and_expr && inclusive_or_expr");
            ir_setplace((yyval.node_idx), t);
            ir_set_marker((yyval.node_idx), ir_get_marker((yyvsp[-3].node_idx)));
            if (!sem_is_scalar(sem_base_node[(yyvsp[-3].node_idx)], sem_ptr_node[(yyvsp[-3].node_idx)]) ||
                !sem_is_scalar(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
                sem_errorf("operator '&&' requires scalar operands");
            sem_set_node((yyval.node_idx), SEM_INT, 0, 0);
        }
#line 4283 "y.tab.c"
    break;

  case 154: /* inclusive_or_expr: exclusive_or_expr  */
#line 1996 "cl_parser.y"
        { (yyval.node_idx) = new_node("inclusive_or_expr \xe2\x86\x92 exclusive_or_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4291 "y.tab.c"
    break;

  case 155: /* inclusive_or_expr: inclusive_or_expr '|' exclusive_or_expr  */
#line 2000 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_integer_binary("|", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("|",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("inclusive_or_expr \xe2\x86\x92 inclusive_or_expr | exclusive_or_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4301 "y.tab.c"
    break;

  case 156: /* exclusive_or_expr: and_expr  */
#line 2008 "cl_parser.y"
        { (yyval.node_idx) = new_node("exclusive_or_expr \xe2\x86\x92 and_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4309 "y.tab.c"
    break;

  case 157: /* exclusive_or_expr: exclusive_or_expr '^' and_expr  */
#line 2012 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_integer_binary("^", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("^",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("exclusive_or_expr \xe2\x86\x92 exclusive_or_expr ^ and_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4319 "y.tab.c"
    break;

  case 158: /* and_expr: equality_expr  */
#line 2020 "cl_parser.y"
        { (yyval.node_idx) = new_node("and_expr \xe2\x86\x92 equality_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4327 "y.tab.c"
    break;

  case 159: /* and_expr: and_expr '&' equality_expr  */
#line 2024 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_integer_binary("&", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("&",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("and_expr \xe2\x86\x92 and_expr & equality_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4337 "y.tab.c"
    break;

  case 160: /* equality_expr: relational_expr  */
#line 2032 "cl_parser.y"
        { (yyval.node_idx) = new_node("equality_expr \xe2\x86\x92 relational_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4345 "y.tab.c"
    break;

  case 161: /* equality_expr: equality_expr EQ_OP relational_expr  */
#line 2036 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          if (!(sem_types_match_exact(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)], sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]) ||
                (sem_is_numeric(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]) && sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))))
              sem_errorf("operator '==' requires compatible operands");
          ir_emit("==",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("equality_expr \xe2\x86\x92 equality_expr == relational_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4357 "y.tab.c"
    break;

  case 162: /* equality_expr: equality_expr NE_OP relational_expr  */
#line 2044 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          if (!(sem_types_match_exact(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)], sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]) ||
                (sem_is_numeric(sem_base_node[(yyvsp[-2].node_idx)], sem_ptr_node[(yyvsp[-2].node_idx)]) && sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))))
              sem_errorf("operator '!=' requires compatible operands");
          ir_emit("!=",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("equality_expr \xe2\x86\x92 equality_expr != relational_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4369 "y.tab.c"
    break;

  case 163: /* relational_expr: shift_expr  */
#line 2054 "cl_parser.y"
        { (yyval.node_idx) = new_node("relational_expr \xe2\x86\x92 shift_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4377 "y.tab.c"
    break;

  case 164: /* relational_expr: relational_expr '<' shift_expr  */
#line 2058 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary("<", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("<",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("relational_expr \xe2\x86\x92 relational_expr < shift_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4387 "y.tab.c"
    break;

  case 165: /* relational_expr: relational_expr '>' shift_expr  */
#line 2064 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary(">", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit(">",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("relational_expr \xe2\x86\x92 relational_expr > shift_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4397 "y.tab.c"
    break;

  case 166: /* relational_expr: relational_expr LE_OP shift_expr  */
#line 2070 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary("<=", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("<=",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("relational_expr \xe2\x86\x92 relational_expr <= shift_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4407 "y.tab.c"
    break;

  case 167: /* relational_expr: relational_expr GE_OP shift_expr  */
#line 2076 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary(">=", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit(">=",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("relational_expr \xe2\x86\x92 relational_expr >= shift_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4417 "y.tab.c"
    break;

  case 168: /* shift_expr: additive_expr  */
#line 2084 "cl_parser.y"
        { (yyval.node_idx) = new_node("shift_expr \xe2\x86\x92 additive_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4425 "y.tab.c"
    break;

  case 169: /* shift_expr: shift_expr LEFT_SHIFT additive_expr  */
#line 2088 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_integer_binary("<<", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("<<",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("shift_expr \xe2\x86\x92 shift_expr << additive_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4435 "y.tab.c"
    break;

  case 170: /* shift_expr: shift_expr RIGHT_SHIFT additive_expr  */
#line 2094 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_integer_binary(">>", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit(">>",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("shift_expr \xe2\x86\x92 shift_expr >> additive_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4445 "y.tab.c"
    break;

  case 171: /* additive_expr: multiplicative_expr  */
#line 2102 "cl_parser.y"
        { (yyval.node_idx) = new_node("additive_expr \xe2\x86\x92 multiplicative_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4453 "y.tab.c"
    break;

  case 172: /* additive_expr: additive_expr '+' multiplicative_expr  */
#line 2106 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary("+", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("+",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("additive_expr \xe2\x86\x92 additive_expr + multiplicative_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_binary_result_numeric((yyval.node_idx), (yyvsp[-2].node_idx), (yyvsp[0].node_idx)); }
#line 4463 "y.tab.c"
    break;

  case 173: /* additive_expr: additive_expr '-' multiplicative_expr  */
#line 2112 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary("-", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("-",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("additive_expr \xe2\x86\x92 additive_expr - multiplicative_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_binary_result_numeric((yyval.node_idx), (yyvsp[-2].node_idx), (yyvsp[0].node_idx)); }
#line 4473 "y.tab.c"
    break;

  case 174: /* multiplicative_expr: unary_expr  */
#line 2120 "cl_parser.y"
        { (yyval.node_idx) = new_node("multiplicative_expr \xe2\x86\x92 unary_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4481 "y.tab.c"
    break;

  case 175: /* multiplicative_expr: multiplicative_expr '*' unary_expr  */
#line 2124 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary("*", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          ir_emit("*",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("multiplicative_expr \xe2\x86\x92 multiplicative_expr * unary_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_binary_result_numeric((yyval.node_idx), (yyvsp[-2].node_idx), (yyvsp[0].node_idx)); }
#line 4491 "y.tab.c"
    break;

  case 176: /* multiplicative_expr: multiplicative_expr '/' unary_expr  */
#line 2130 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_numeric_binary("/", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          if (sem_is_null_literal((yyvsp[0].node_idx)))
              sem_errorf("division by zero in expression");
          ir_emit("/",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("multiplicative_expr \xe2\x86\x92 multiplicative_expr / unary_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_binary_result_numeric((yyval.node_idx), (yyvsp[-2].node_idx), (yyvsp[0].node_idx)); }
#line 4503 "y.tab.c"
    break;

  case 177: /* multiplicative_expr: multiplicative_expr '%' unary_expr  */
#line 2138 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          sem_require_integer_binary("%", (yyvsp[-2].node_idx), (yyvsp[0].node_idx));
          if (sem_is_null_literal((yyvsp[0].node_idx)))
              sem_errorf("modulo by zero in expression");
          ir_emit("%",ir_getplace((yyvsp[-2].node_idx)),ir_getplace((yyvsp[0].node_idx)),t);
          (yyval.node_idx) = new_node("multiplicative_expr \xe2\x86\x92 multiplicative_expr % unary_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4515 "y.tab.c"
    break;

  case 178: /* unary_expr: postfix_expr  */
#line 2148 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 postfix_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4523 "y.tab.c"
    break;

  case 179: /* unary_expr: INC_OP unary_expr  */
#line 2152 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[0].node_idx)); const char*p=ir_getplace((yyvsp[0].node_idx));
          if (!sem_lvalue_node[(yyvsp[0].node_idx)] || !sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_errorf("operator '++' requires an assignable numeric operand");
          ir_emit("+",p,"1",p);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 ++ unary_expr");
          ir_setplace((yyval.node_idx),p); ir_set_marker((yyval.node_idx),m); sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4534 "y.tab.c"
    break;

  case 180: /* unary_expr: DEC_OP unary_expr  */
#line 2159 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[0].node_idx)); const char*p=ir_getplace((yyvsp[0].node_idx));
          if (!sem_lvalue_node[(yyvsp[0].node_idx)] || !sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_errorf("operator '--' requires an assignable numeric operand");
          ir_emit("-",p,"1",p);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 -- unary_expr");
          ir_setplace((yyval.node_idx),p); ir_set_marker((yyval.node_idx),m); sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4545 "y.tab.c"
    break;

  case 181: /* unary_expr: '-' unary_expr  */
#line 2166 "cl_parser.y"
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_errorf("unary '-' requires a numeric operand");
          ir_emit("minus",ir_getplace((yyvsp[0].node_idx)),"",t);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 - unary_expr [UMINUS]");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4556 "y.tab.c"
    break;

  case 182: /* unary_expr: '+' unary_expr  */
#line 2173 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 + unary_expr [UPLUS]");
          if (!sem_is_numeric(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_errorf("unary '+' requires a numeric operand");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0; }
#line 4566 "y.tab.c"
    break;

  case 183: /* unary_expr: '!' unary_expr  */
#line 2179 "cl_parser.y"
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_is_scalar(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_errorf("operator '!' requires a scalar operand");
          ir_emit("!",ir_getplace((yyvsp[0].node_idx)),"",t);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 ! unary_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4577 "y.tab.c"
    break;

  case 184: /* unary_expr: '~' unary_expr  */
#line 2186 "cl_parser.y"
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_is_integer_like(sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)]))
              sem_errorf("operator '~' requires an integer operand");
          ir_emit("~",ir_getplace((yyvsp[0].node_idx)),"",t);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 ~ unary_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4588 "y.tab.c"
    break;

  case 185: /* unary_expr: '*' unary_expr  */
#line 2193 "cl_parser.y"
        {
            int m=ir_mark(); const char*t=ir_newtemp();
            const char *ptr = ir_getplace((yyvsp[0].node_idx));
            if (sem_ptr_node[(yyvsp[0].node_idx)] <= 0)
                sem_errorf("cannot dereference non-pointer expression");
            ir_emit("deref", ptr, "", t);
            (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 * unary_expr [deref]");
            ir_setplace_addr((yyval.node_idx), ptr);
            ir_setplace((yyval.node_idx), t);
            ir_set_marker((yyval.node_idx), m);
            sem_set_node((yyval.node_idx), sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)] - 1, 1);
        }
#line 4605 "y.tab.c"
    break;

  case 186: /* unary_expr: '&' unary_expr  */
#line 2206 "cl_parser.y"
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_lvalue_node[(yyvsp[0].node_idx)]) sem_errorf("operator '&' requires an lvalue operand");
          ir_emit("addr",ir_getplace((yyvsp[0].node_idx)),"",t);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 & unary_expr [addr-of]");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m);
          sem_set_node((yyval.node_idx), sem_base_node[(yyvsp[0].node_idx)], sem_ptr_node[(yyvsp[0].node_idx)] + 1, 0); }
#line 4616 "y.tab.c"
    break;

  case 187: /* unary_expr: SIZEOF unary_expr  */
#line 2213 "cl_parser.y"
        { int m=ir_mark(); const char*t=ir_newtemp();
          ir_emit("sizeof",ir_getplace((yyvsp[0].node_idx)),"",t);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 sizeof unary_expr");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4625 "y.tab.c"
    break;

  case 188: /* unary_expr: SIZEOF '(' type_spec ')'  */
#line 2218 "cl_parser.y"
        { int m=ir_mark(); const char*t=ir_newtemp();
          ir_emit("sizeof_type","type","",t);
          (yyval.node_idx) = new_node("unary_expr \xe2\x86\x92 sizeof ( type_spec )");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4634 "y.tab.c"
    break;

  case 189: /* postfix_expr: primary_expr  */
#line 2241 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 primary_expr");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[0].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[0].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[0].node_idx)); }
#line 4642 "y.tab.c"
    break;

  case 190: /* postfix_expr: postfix_expr '[' expr ']'  */
#line 2245 "cl_parser.y"
        {
            int m=ir_get_marker((yyvsp[-3].node_idx));
            const char *addr = ir_newtemp();
            const char *val  = ir_newtemp();
            if (sem_ptr_node[(yyvsp[-3].node_idx)] <= 0)
                sem_errorf("subscripted expression is not an array or pointer");
            if (!sem_is_integer_like(sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)]))
                sem_errorf("array index must be an integer expression");
            ir_emit("elemaddr", ir_getplace((yyvsp[-3].node_idx)), ir_getplace((yyvsp[-1].node_idx)), addr);
            ir_emit("deref",    addr, "", val);
            (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr [ expr ]");
            ir_setplace_addr((yyval.node_idx), addr);
            ir_setplace((yyval.node_idx), val);
            ir_set_marker((yyval.node_idx), m);
            sem_set_node((yyval.node_idx), sem_base_node[(yyvsp[-3].node_idx)], sem_ptr_node[(yyvsp[-3].node_idx)] - 1, 1);
        }
#line 4663 "y.tab.c"
    break;

  case 191: /* $@14: %empty  */
#line 2262 "cl_parser.y"
        {
            /*
             * FIX BUG 1 MID-RULE: Push a new param-buffer level.
             * All argument expressions reduced while this level is
             * active will have their places recorded here instead of
             * being emitted immediately as param quads.
             */
            param_push_level();
        }
#line 4677 "y.tab.c"
    break;

  case 192: /* postfix_expr: postfix_expr '(' $@14 ')'  */
#line 2272 "cl_parser.y"
        {
            cnt_func_calls++;
            int m=ir_get_marker((yyvsp[-3].node_idx));
            const char *t=ir_newtemp();
            if (!sem_func_node[(yyvsp[-3].node_idx)])
                sem_errorf("attempt to call a non-function expression");
            if (sem_paramcount_node[(yyvsp[-3].node_idx)] > 0)
                sem_errorf("function '%s' expects %d argument(s), got 0",
                           ir_getplace((yyvsp[-3].node_idx)), sem_paramcount_node[(yyvsp[-3].node_idx)]);
            /* Flush buffered params (none in this case) then pop */
            param_flush_and_pop();
            ir_emit("call",ir_getplace((yyvsp[-3].node_idx)),"0",t);
            (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr ( )");
            ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m);
            sem_set_node((yyval.node_idx), sem_base_node[(yyvsp[-3].node_idx)], sem_ptr_node[(yyvsp[-3].node_idx)], 0);
        }
#line 4698 "y.tab.c"
    break;

  case 193: /* $@15: %empty  */
#line 2289 "cl_parser.y"
        {
            /* FIX BUG 1 MID-RULE: push param-buffer level */
            param_push_level();
        }
#line 4707 "y.tab.c"
    break;

  case 194: /* postfix_expr: postfix_expr '(' $@15 arg_list ')'  */
#line 2294 "cl_parser.y"
        {
            cnt_func_calls++;
            int m=ir_get_marker((yyvsp[-4].node_idx));
            const char *t=ir_newtemp();
            int ac = ir_get_argcount((yyvsp[-1].node_idx));
            char acbuf[32]; snprintf(acbuf,sizeof acbuf,"%d",ac);
            if (!sem_func_node[(yyvsp[-4].node_idx)])
                sem_errorf("attempt to call a non-function expression");
            if (sem_paramcount_node[(yyvsp[-4].node_idx)] >= 0 && sem_paramcount_node[(yyvsp[-4].node_idx)] != ac)
                sem_errorf("function '%s' expects %d argument(s), got %d",
                           ir_getplace((yyvsp[-4].node_idx)), sem_paramcount_node[(yyvsp[-4].node_idx)], ac);
            /*
             * FIX BUG 1: Flush all buffered param quads for this call
             * level NOW, immediately before the call quad.  This ensures
             * that all params appear together in the IR regardless of
             * how complex the argument expressions were (including nested
             * function calls).
             */
            param_flush_and_pop();
            ir_emit("call",ir_getplace((yyvsp[-4].node_idx)),acbuf,t);
            (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr ( arg_list )");
            ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m);
            sem_set_node((yyval.node_idx), sem_base_node[(yyvsp[-4].node_idx)], sem_ptr_node[(yyvsp[-4].node_idx)], 0);
        }
#line 4736 "y.tab.c"
    break;

  case 195: /* postfix_expr: postfix_expr '.' IDENTIFIER  */
#line 2319 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          ir_emit(".",ir_getplace((yyvsp[-2].node_idx)),(yyvsp[0].str_val),t);
          (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr . IDENTIFIER");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_UNKNOWN, 0, 1); }
#line 4745 "y.tab.c"
    break;

  case 196: /* postfix_expr: postfix_expr ARROW IDENTIFIER  */
#line 2324 "cl_parser.y"
        { int m=ir_get_marker((yyvsp[-2].node_idx)); const char*t=ir_newtemp();
          ir_emit("->",ir_getplace((yyvsp[-2].node_idx)),(yyvsp[0].str_val),t);
          (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr -> IDENTIFIER");
          ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_UNKNOWN, 0, 1); }
#line 4754 "y.tab.c"
    break;

  case 197: /* postfix_expr: postfix_expr INC_OP  */
#line 2329 "cl_parser.y"
        {
            /*
             * FIX BUG 3: We still emit the save of the old value because
             * in general  a[i++]  or  f(x++)  do need it.  When this
             * appears as a lone statement the dead-temp elimination pass
             * in ir_print() will silently remove the unused save quad.
             */
            int m=ir_get_marker((yyvsp[-1].node_idx));
            const char *p=ir_getplace((yyvsp[-1].node_idx));
            const char *t=ir_newtemp();
            if (!sem_lvalue_node[(yyvsp[-1].node_idx)] || !sem_is_numeric(sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)]))
                sem_errorf("operator '++' requires an assignable numeric operand");
            ir_emit("=",p,"",t);
            ir_emit("+",p,"1",p);
            (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr ++");
            ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m);
            sem_copy_node((yyval.node_idx), (yyvsp[-1].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0;
        }
#line 4777 "y.tab.c"
    break;

  case 198: /* postfix_expr: postfix_expr DEC_OP  */
#line 2348 "cl_parser.y"
        {
            int m=ir_get_marker((yyvsp[-1].node_idx));
            const char *p=ir_getplace((yyvsp[-1].node_idx));
            const char *t=ir_newtemp();
            if (!sem_lvalue_node[(yyvsp[-1].node_idx)] || !sem_is_numeric(sem_base_node[(yyvsp[-1].node_idx)], sem_ptr_node[(yyvsp[-1].node_idx)]))
                sem_errorf("operator '--' requires an assignable numeric operand");
            ir_emit("=",p,"",t);
            ir_emit("-",p,"1",p);
            (yyval.node_idx) = new_node("postfix_expr \xe2\x86\x92 postfix_expr --");
            ir_setplace((yyval.node_idx),t); ir_set_marker((yyval.node_idx),m);
            sem_copy_node((yyval.node_idx), (yyvsp[-1].node_idx)); sem_lvalue_node[(yyval.node_idx)] = 0;
        }
#line 4794 "y.tab.c"
    break;

  case 199: /* primary_expr: IDENTIFIER  */
#line 2364 "cl_parser.y"
        { int m=ir_mark();
          int sym = sem_lookup_symbol((yyvsp[0].str_val));
          if (sym < 0) sem_errorf("use of undeclared identifier '%s'", (yyvsp[0].str_val));
          (yyval.node_idx) = new_node("primary_expr \xe2\x86\x92 IDENTIFIER");
          ir_setplace((yyval.node_idx),ir_intern((yyvsp[0].str_val))); ir_set_marker((yyval.node_idx),m);
          sem_set_node((yyval.node_idx), sem_symbols[sym].base, sem_symbols[sym].ptr_depth, !sem_symbols[sym].is_function);
          sem_func_node[(yyval.node_idx)] = sem_symbols[sym].is_function;
          sem_paramcount_node[(yyval.node_idx)] = sem_symbols[sym].param_count; }
#line 4807 "y.tab.c"
    break;

  case 200: /* primary_expr: INT_CONST  */
#line 2373 "cl_parser.y"
        { cnt_int_consts++; int m=ir_mark();
          (yyval.node_idx) = new_node("primary_expr \xe2\x86\x92 INT_CONST");
          ir_setplace((yyval.node_idx),ir_intern((yyvsp[0].str_val))); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_INT, 0, 0); }
#line 4815 "y.tab.c"
    break;

  case 201: /* primary_expr: FLOAT_CONST  */
#line 2377 "cl_parser.y"
        { int m=ir_mark();
          (yyval.node_idx) = new_node("primary_expr \xe2\x86\x92 FLOAT_CONST");
          ir_setplace((yyval.node_idx),ir_intern((yyvsp[0].str_val))); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_FLOAT, 0, 0); }
#line 4823 "y.tab.c"
    break;

  case 202: /* primary_expr: CHAR_CONST  */
#line 2381 "cl_parser.y"
        { int m=ir_mark();
          (yyval.node_idx) = new_node("primary_expr \xe2\x86\x92 CHAR_CONST");
          ir_setplace((yyval.node_idx),ir_intern((yyvsp[0].str_val))); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_CHAR, 0, 0); }
#line 4831 "y.tab.c"
    break;

  case 203: /* primary_expr: STRING_LITERAL  */
#line 2385 "cl_parser.y"
        { int m=ir_mark();
          (yyval.node_idx) = new_node("primary_expr \xe2\x86\x92 STRING_LITERAL");
          ir_setplace((yyval.node_idx),ir_intern((yyvsp[0].str_val))); ir_set_marker((yyval.node_idx),m); sem_set_node((yyval.node_idx), SEM_CHAR, 1, 0); }
#line 4839 "y.tab.c"
    break;

  case 204: /* primary_expr: '(' expr ')'  */
#line 2389 "cl_parser.y"
        { (yyval.node_idx) = new_node("primary_expr \xe2\x86\x92 ( expr )");
          ir_setplace((yyval.node_idx),ir_getplace((yyvsp[-1].node_idx))); ir_setplace_addr((yyval.node_idx), ir_getplace_addr((yyvsp[-1].node_idx))); ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-1].node_idx)));
          sem_copy_node((yyval.node_idx), (yyvsp[-1].node_idx)); }
#line 4847 "y.tab.c"
    break;

  case 205: /* arg_list: assignment_expr  */
#line 2403 "cl_parser.y"
        {
            /* Record arg in deferred buffer — do NOT emit param yet */
            param_record(ir_getplace((yyvsp[0].node_idx)));
            (yyval.node_idx) = new_node("arg_list \xe2\x86\x92 assignment_expr");
            ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[0].node_idx)));
            ir_set_argcount((yyval.node_idx), 1);
        }
#line 4859 "y.tab.c"
    break;

  case 206: /* arg_list: arg_list ',' assignment_expr  */
#line 2411 "cl_parser.y"
        {
            /* Record arg in deferred buffer — do NOT emit param yet */
            param_record(ir_getplace((yyvsp[0].node_idx)));
            (yyval.node_idx) = new_node("arg_list \xe2\x86\x92 arg_list , assignment_expr");
            ir_set_marker((yyval.node_idx),ir_get_marker((yyvsp[-2].node_idx)));
            ir_set_argcount((yyval.node_idx), ir_get_argcount((yyvsp[-2].node_idx))+1);
        }
#line 4871 "y.tab.c"
    break;


#line 4875 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 2420 "cl_parser.y"


/* ================================================================
   MAIN
   ================================================================ */
int main(int argc, char *argv[]) {
    int show_table=0,show_extra=0,show_ir=1,show_tree=1;
    const char *yout_path="y.output";

    if (argc<2){
        fprintf(stderr,"Usage: %s <input_file> [--table] [--extra] [--no-ir] [--no-tree] [--yout <p>]\n",argv[0]);
        return 1;
    }

    for(int i=2;i<argc;i++){
        if(!strcmp(argv[i],"--table"))        show_table=1;
        else if(!strcmp(argv[i],"--extra"))   show_extra=1;
        else if(!strcmp(argv[i],"--no-ir"))   show_ir=0;
        else if(!strcmp(argv[i],"--no-tree")) show_tree=0;
        else if(!strcmp(argv[i],"--yout")&&i+1<argc) yout_path=argv[++i];
    }

    yyin=fopen(argv[1],"r");
    if(!yyin){
        fprintf(stderr,"***process terminated*** [input error]: cannot open '%s'\n",argv[1]);
        return 1;
    }

    ir_init();
    sem_init();

    printf("-------------------------------------------------------------\n");
    printf("  CS327 Assignment 4 -- IR Code Generator | LALR(1) Parser\n");
    printf("  Input file : %s\n",argv[1]);
    printf("-------------------------------------------------------------\n");

    ir_print_source(argv[1]);

    fflush(stdout);

    int parse_status = yyparse();

    fclose(yyin);

    if(parse_status == 0){
        printf("-------------------------------------------------------------\n");
        printf("  ***parsing successful***\n");
        printf("-------------------------------------------------------------\n");

        if(show_ir){
            ir_print();
        }
    } else {
        printf("-------------------------------------------------------------\n");
        printf("  ***parsing failed***\n");
        printf("-------------------------------------------------------------\n");
    }

    if(show_tree && parse_status == 0)  print_tree_reverse();
    if(show_table && parse_status == 0) print_parsing_table(yout_path);
    if(show_extra && parse_status == 0) print_grammar_summary();

    return 0;
}
