%{
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

%}

%union {
    char  *str_val;
    int    node_idx;
}

%token INT FLOAT CHAR VOID DOUBLE SHORT LONG UNSIGNED SIGNED
%token IF ELSE FOR WHILE DO SWITCH CASE DEFAULT
%token BREAK CONTINUE RETURN
%token STRUCT TYPEDEF ENUM UNION
%token SIZEOF AUTO CONST VOLATILE STATIC EXTERN REGISTER INLINE

%token <str_val> IDENTIFIER
%token <str_val> INT_CONST FLOAT_CONST CHAR_CONST STRING_LITERAL

%token <str_val> ASSIGN_OP ELLIPSIS ARROW INC_OP DEC_OP
%token LEFT_SHIFT RIGHT_SHIFT LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP

%right '=' ASSIGN_OP
%right '?' ':'
%left  OR_OP
%left  AND_OP
%left  '|'
%left  '^'
%left  '&'
%left  EQ_OP NE_OP
%left  '<' '>' LE_OP GE_OP
%left  LEFT_SHIFT RIGHT_SHIFT
%left  '+' '-'
%left  '*' '/' '%'
%right UMINUS UPLUS '!' '~'
%left  INC_OP DEC_OP ARROW '.'
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%expect 38

%type <node_idx> program translation_unit external_decl
%type <node_idx> function_def function_header
%type <node_idx> declaration declarator_list declarator
%type <node_idx> type_spec type_qualifier storage_class_spec storage_class_list
%type <node_idx> stmt compound_stmt block_item block_item_list
%type <node_idx> expr_stmt selection_stmt iteration_stmt jump_stmt labeled_stmt
%type <node_idx> non_case_stmt case_body_list case_list
%type <node_idx> expr assignment_expr conditional_expr
%type <node_idx> logical_or_expr logical_and_expr inclusive_or_expr
%type <node_idx> exclusive_or_expr and_expr equality_expr relational_expr
%type <node_idx> shift_expr additive_expr multiplicative_expr
%type <node_idx> unary_expr postfix_expr primary_expr
%type <node_idx> arg_list param_list param_decl
%type <node_idx> struct_decl member_decl_list member_decl
%type <node_idx> enum_decl enumerator_list enumerator
%type <node_idx> initializer initializer_list

%%

program : translation_unit { $$ = new_node("program \xe2\x86\x92 translation_unit"); } ;

translation_unit
    : external_decl { $$ = new_node("translation_unit \xe2\x86\x92 external_decl"); }
    | translation_unit external_decl { $$ = new_node("translation_unit \xe2\x86\x92 translation_unit external_decl"); }
    ;
external_decl
    : function_def  { $$ = new_node("external_decl \xe2\x86\x92 function_def"); }
    | declaration   { cnt_global_decls++; $$ = new_node("external_decl \xe2\x86\x92 declaration"); }
    ;

function_def
    : function_header compound_stmt
        { cnt_func_defs++; ir_emit("end_func","","","");
          sem_end_function();
          $$ = new_node("function_def \xe2\x86\x92 function_header compound_stmt"); }
    ;
function_header
    : type_spec IDENTIFIER '('
        { sem_begin_function($2, sem_base_node[$1], sem_ptr_node[$1]); }
      param_list ')'
        { strncpy(ir_cur_func,$2?$2:"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(ir_get_argcount($5));
          $$ = new_node("function_header \xe2\x86\x92 type_spec IDENTIFIER ( param_list )"); }
    | type_spec IDENTIFIER '('
        { sem_begin_function($2, sem_base_node[$1], sem_ptr_node[$1]); }
      ')'
        { strncpy(ir_cur_func,$2?$2:"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(0);
          $$ = new_node("function_header \xe2\x86\x92 type_spec IDENTIFIER ( )"); }
    | storage_class_list type_spec IDENTIFIER '('
        { sem_begin_function($3, sem_base_node[$2], sem_ptr_node[$2]); }
      param_list ')'
        { strncpy(ir_cur_func,$3?$3:"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(ir_get_argcount($6));
          $$ = new_node("function_header \xe2\x86\x92 storage_class_list type_spec IDENTIFIER ( param_list )"); }
    | storage_class_list type_spec IDENTIFIER '('
        { sem_begin_function($3, sem_base_node[$2], sem_ptr_node[$2]); }
      ')'
        { strncpy(ir_cur_func,$3?$3:"?",127); ir_emit("begin_func",ir_cur_func,"","");
          sem_finish_function_header(0);
          $$ = new_node("function_header \xe2\x86\x92 storage_class_list type_spec IDENTIFIER ( )"); }
    ;

param_list
    : param_decl
        { $$ = new_node("param_list \xe2\x86\x92 param_decl"); ir_set_argcount($$, 1); }
    | param_list ',' param_decl
        { $$ = new_node("param_list \xe2\x86\x92 param_list , param_decl"); ir_set_argcount($$, ir_get_argcount($1)+1); }
    | ELLIPSIS
        { $$ = new_node("param_list \xe2\x86\x92 ..."); ir_set_argcount($$, 0); }
    | param_list ',' ELLIPSIS
        { $$ = new_node("param_list \xe2\x86\x92 param_list , ..."); ir_set_argcount($$, ir_get_argcount($1)); }
    ;
param_decl
    : type_spec IDENTIFIER
        { sem_declare_object($2, sem_base_node[$1], sem_ptr_node[$1]);
          $$ = new_node("param_decl \xe2\x86\x92 type_spec IDENTIFIER"); }
    | type_spec '*' IDENTIFIER
        { cnt_ptr_decls++;
          sem_declare_object($3, sem_base_node[$1], sem_ptr_node[$1] + 1);
          $$ = new_node("param_decl \xe2\x86\x92 type_spec * IDENTIFIER"); }
    | type_spec IDENTIFIER '[' ']'
        { sem_declare_object($2, sem_base_node[$1], sem_ptr_node[$1] + 1);
          $$ = new_node("param_decl \xe2\x86\x92 type_spec IDENTIFIER [ ]"); }
    | type_spec IDENTIFIER '[' INT_CONST ']'
        { sem_declare_object($2, sem_base_node[$1], sem_ptr_node[$1] + 1);
          $$ = new_node("param_decl \xe2\x86\x92 type_spec IDENTIFIER [ INT_CONST ]"); }
    | type_spec                         { $$ = new_node("param_decl \xe2\x86\x92 type_spec"); }
    ;
    ;

declaration
    : type_spec
        { sem_begin_decl(sem_base_node[$1], sem_ptr_node[$1]); }
      declarator_list ';' { $$ = new_node("declaration \xe2\x86\x92 type_spec declarator_list ;"); }
    | storage_class_list type_spec
        { sem_begin_decl(sem_base_node[$2], sem_ptr_node[$2]); }
      declarator_list ';' { $$ = new_node("declaration \xe2\x86\x92 storage_class_list type_spec declarator_list ;"); }
    | type_spec ';'                                    { $$ = new_node("declaration \xe2\x86\x92 type_spec ;"); }
    | storage_class_list type_spec ';'                 { $$ = new_node("declaration \xe2\x86\x92 storage_class_list type_spec ;"); }
    | TYPEDEF type_spec
        { sem_begin_decl(sem_base_node[$2], sem_ptr_node[$2]); }
      declarator_list ';' { $$ = new_node("declaration \xe2\x86\x92 typedef type_spec declarator_list ;"); }
    | struct_decl ';'   { cnt_struct_decls++; $$ = new_node("declaration \xe2\x86\x92 struct_decl ;"); }
    | enum_decl ';'     { $$ = new_node("declaration \xe2\x86\x92 enum_decl ;"); }
    ;
declarator_list
    : declarator                        { $$ = new_node("declarator_list \xe2\x86\x92 declarator"); }
    | declarator_list ',' declarator    { $$ = new_node("declarator_list \xe2\x86\x92 declarator_list , declarator"); }
    ;
declarator
    : IDENTIFIER
        { sem_declare_object($1, sem_decl_base, sem_decl_ptr);
          $$ = new_node("declarator \xe2\x86\x92 IDENTIFIER"); }
    | IDENTIFIER '=' initializer
        { sem_declare_object($1, sem_decl_base, sem_decl_ptr);
          if (!((sem_is_numeric(sem_decl_base, sem_decl_ptr) && sem_is_numeric(sem_base_node[$3], sem_ptr_node[$3])) ||
                sem_types_match_exact(sem_decl_base, sem_decl_ptr, sem_base_node[$3], sem_ptr_node[$3]) ||
                (sem_decl_ptr > 0 && sem_is_null_literal($3)))) {
              char lhs_buf[32], rhs_buf[32];
              sem_type_to_string(sem_decl_base, sem_decl_ptr, lhs_buf, sizeof lhs_buf);
              sem_type_to_string(sem_base_node[$3], sem_ptr_node[$3], rhs_buf, sizeof rhs_buf);
              sem_errorf("cannot initialize '%s' of type %s with expression of type %s", $1, lhs_buf, rhs_buf);
          }
          const char *val=ir_getplace($3);
          if(val[0]) ir_emit("=",val,"",ir_intern($1));
          $$ = new_node("declarator \xe2\x86\x92 IDENTIFIER = initializer"); }
    | '*' IDENTIFIER
        { cnt_ptr_decls++; sem_declare_object($2, sem_decl_base, sem_decl_ptr + 1);
          $$ = new_node("declarator \xe2\x86\x92 * IDENTIFIER"); }
    | '*' IDENTIFIER '=' initializer
        { cnt_ptr_decls++;
          sem_declare_object($2, sem_decl_base, sem_decl_ptr + 1);
          if (!(sem_types_match_exact(sem_decl_base, sem_decl_ptr + 1, sem_base_node[$4], sem_ptr_node[$4]) ||
                ((sem_decl_ptr + 1) > 0 && sem_is_null_literal($4)))) {
              char lhs_buf[32], rhs_buf[32];
              sem_type_to_string(sem_decl_base, sem_decl_ptr + 1, lhs_buf, sizeof lhs_buf);
              sem_type_to_string(sem_base_node[$4], sem_ptr_node[$4], rhs_buf, sizeof rhs_buf);
              sem_errorf("cannot initialize '%s' of type %s with expression of type %s", $2, lhs_buf, rhs_buf);
          }
          const char *val=ir_getplace($4);
          if(val[0]) ir_emit("=",val,"",ir_intern($2));
          $$ = new_node("declarator \xe2\x86\x92 * IDENTIFIER = initializer"); }
    | IDENTIFIER '[' INT_CONST ']'
        { sem_declare_object($1, sem_decl_base, sem_decl_ptr + 1);
          $$ = new_node("declarator \xe2\x86\x92 IDENTIFIER [ INT_CONST ]"); }
    | IDENTIFIER '[' ']'
        { sem_declare_object($1, sem_decl_base, sem_decl_ptr + 1);
          $$ = new_node("declarator \xe2\x86\x92 IDENTIFIER [ ]"); }
    | IDENTIFIER '(' param_list ')'
        { sem_declare_function($1, sem_decl_base, sem_decl_ptr, ir_get_argcount($3), 0);
          $$ = new_node("declarator \xe2\x86\x92 IDENTIFIER ( param_list )"); }
    | IDENTIFIER '(' ')'
        { sem_declare_function($1, sem_decl_base, sem_decl_ptr, 0, 0);
          $$ = new_node("declarator \xe2\x86\x92 IDENTIFIER ( )"); }
    | '(' '*' IDENTIFIER ')' '(' param_list ')'
        { sem_declare_object($3, sem_decl_base, sem_decl_ptr + 1);
          $$ = new_node("declarator \xe2\x86\x92 ( * IDENTIFIER ) ( param_list )"); }
    | '(' '*' IDENTIFIER ')' '(' ')'
        { sem_declare_object($3, sem_decl_base, sem_decl_ptr + 1);
          $$ = new_node("declarator \xe2\x86\x92 ( * IDENTIFIER ) ( )"); }
    ;
initializer
    : assignment_expr
        { $$ = new_node("initializer \xe2\x86\x92 assignment_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | '{' initializer_list '}' { $$ = new_node("initializer \xe2\x86\x92 { initializer_list }"); }
    | '{' initializer_list ',' '}' { $$ = new_node("initializer \xe2\x86\x92 { initializer_list , }"); }
    ;
initializer_list
    : initializer                           { $$ = new_node("initializer_list \xe2\x86\x92 initializer"); }
    | initializer_list ',' initializer      { $$ = new_node("initializer_list \xe2\x86\x92 initializer_list , initializer"); }
    ;

type_qualifier
    : CONST    { $$ = new_node("type_qualifier \xe2\x86\x92 const"); }
    | VOLATILE { $$ = new_node("type_qualifier \xe2\x86\x92 volatile"); }
    ;
type_spec
    : INT  { $$ = new_node("type_spec \xe2\x86\x92 int"); sem_set_node($$, SEM_INT, 0, 0); }
    | FLOAT  { $$ = new_node("type_spec \xe2\x86\x92 float"); sem_set_node($$, SEM_FLOAT, 0, 0); }
    | CHAR { $$ = new_node("type_spec \xe2\x86\x92 char"); sem_set_node($$, SEM_CHAR, 0, 0); }
    | VOID   { $$ = new_node("type_spec \xe2\x86\x92 void"); sem_set_node($$, SEM_VOID, 0, 0); }
    | DOUBLE { $$ = new_node("type_spec \xe2\x86\x92 double"); sem_set_node($$, SEM_FLOAT, 0, 0); }
    | SHORT { $$ = new_node("type_spec \xe2\x86\x92 short"); sem_set_node($$, SEM_INT, 0, 0); }
    | LONG { $$ = new_node("type_spec \xe2\x86\x92 long"); sem_set_node($$, SEM_INT, 0, 0); }
    | UNSIGNED { $$ = new_node("type_spec \xe2\x86\x92 unsigned"); sem_set_node($$, SEM_INT, 0, 0); }
    | SIGNED { $$ = new_node("type_spec \xe2\x86\x92 signed"); sem_set_node($$, SEM_INT, 0, 0); }
    | type_qualifier type_spec { $$ = new_node("type_spec \xe2\x86\x92 type_qualifier type_spec"); sem_copy_node($$, $2); }
    | SHORT INT { $$ = new_node("type_spec \xe2\x86\x92 short int"); sem_set_node($$, SEM_INT, 0, 0); }
    | LONG INT  { $$ = new_node("type_spec \xe2\x86\x92 long int"); sem_set_node($$, SEM_INT, 0, 0); }
    | LONG DOUBLE { $$ = new_node("type_spec \xe2\x86\x92 long double"); sem_set_node($$, SEM_FLOAT, 0, 0); }
    | UNSIGNED INT  { $$ = new_node("type_spec \xe2\x86\x92 unsigned int"); sem_set_node($$, SEM_INT, 0, 0); }
    | UNSIGNED LONG { $$ = new_node("type_spec \xe2\x86\x92 unsigned long"); sem_set_node($$, SEM_INT, 0, 0); }
    | UNSIGNED CHAR { $$ = new_node("type_spec \xe2\x86\x92 unsigned char"); sem_set_node($$, SEM_CHAR, 0, 0); }
    | SIGNED INT  { $$ = new_node("type_spec \xe2\x86\x92 signed int"); sem_set_node($$, SEM_INT, 0, 0); }
    | LONG LONG   { $$ = new_node("type_spec \xe2\x86\x92 long long"); sem_set_node($$, SEM_INT, 0, 0); }
    | LONG LONG INT { $$ = new_node("type_spec \xe2\x86\x92 long long int"); sem_set_node($$, SEM_INT, 0, 0); }
    | struct_decl { $$ = new_node("type_spec \xe2\x86\x92 struct_decl"); sem_set_node($$, SEM_STRUCT, 0, 0); }
    | enum_decl   { $$ = new_node("type_spec \xe2\x86\x92 enum_decl"); sem_set_node($$, SEM_INT, 0, 0); }
    ;
storage_class_spec
    : AUTO { $$ = new_node("storage_class_spec \xe2\x86\x92 auto"); }
    | REGISTER { $$ = new_node("storage_class_spec \xe2\x86\x92 register"); }
    | STATIC { $$ = new_node("storage_class_spec \xe2\x86\x92 static"); }
    | EXTERN { $$ = new_node("storage_class_spec \xe2\x86\x92 extern"); }
    | INLINE { $$ = new_node("storage_class_spec \xe2\x86\x92 inline"); }
    ;
storage_class_list
    : storage_class_spec { $$ = new_node("storage_class_list \xe2\x86\x92 storage_class_spec"); }
    | storage_class_list storage_class_spec { $$ = new_node("storage_class_list \xe2\x86\x92 storage_class_list storage_class_spec"); }
    ;

struct_decl
    : STRUCT IDENTIFIER '{' member_decl_list '}' { cnt_struct_decls++; $$ = new_node("struct_decl \xe2\x86\x92 struct IDENTIFIER { member_decl_list }"); }
    | STRUCT '{' member_decl_list '}'             { cnt_struct_decls++; $$ = new_node("struct_decl \xe2\x86\x92 struct { member_decl_list }"); }
    | STRUCT IDENTIFIER                           { $$ = new_node("struct_decl \xe2\x86\x92 struct IDENTIFIER"); }
    | UNION IDENTIFIER '{' member_decl_list '}'   { $$ = new_node("struct_decl \xe2\x86\x92 union IDENTIFIER { member_decl_list }"); }
    | UNION '{' member_decl_list '}'              { $$ = new_node("struct_decl \xe2\x86\x92 union { member_decl_list }"); }
    | UNION IDENTIFIER                            { $$ = new_node("struct_decl \xe2\x86\x92 union IDENTIFIER"); }
    ;
member_decl_list
    : member_decl                  { $$ = new_node("member_decl_list \xe2\x86\x92 member_decl"); }
    | member_decl_list member_decl { $$ = new_node("member_decl_list \xe2\x86\x92 member_decl_list member_decl"); }
    ;
member_decl
    : type_spec declarator_list ';' { $$ = new_node("member_decl \xe2\x86\x92 type_spec declarator_list ;"); }
    | type_spec ';'                 { $$ = new_node("member_decl \xe2\x86\x92 type_spec ;"); }
    ;
enum_decl
    : ENUM IDENTIFIER '{' enumerator_list '}' { $$ = new_node("enum_decl \xe2\x86\x92 enum IDENTIFIER { enumerator_list }"); }
    | ENUM '{' enumerator_list '}'            { $$ = new_node("enum_decl \xe2\x86\x92 enum { enumerator_list }"); }
    | ENUM IDENTIFIER                         { $$ = new_node("enum_decl \xe2\x86\x92 enum IDENTIFIER"); }
    ;
enumerator_list
    : enumerator                       { $$ = new_node("enumerator_list \xe2\x86\x92 enumerator"); }
    | enumerator_list ',' enumerator   { $$ = new_node("enumerator_list \xe2\x86\x92 enumerator_list , enumerator"); }
    ;
enumerator
    : IDENTIFIER              { $$ = new_node("enumerator \xe2\x86\x92 IDENTIFIER"); }
    | IDENTIFIER '=' INT_CONST { $$ = new_node("enumerator \xe2\x86\x92 IDENTIFIER = INT_CONST"); }
    ;

compound_stmt
    : '{' '}'
        { int m=ir_mark(); $$ = new_node("compound_stmt \xe2\x86\x92 { }");
          ir_set_marker($$,m); }
    | '{' block_item_list '}'
        { $$ = new_node("compound_stmt \xe2\x86\x92 { block_item_list }");
          ir_set_marker($$,ir_get_marker($2)); }
    ;
block_item_list
    : block_item
        { $$ = new_node("block_item_list \xe2\x86\x92 block_item");
          ir_set_marker($$,ir_get_marker($1)); }
    | block_item_list block_item
        { $$ = new_node("block_item_list \xe2\x86\x92 block_item_list block_item");
          ir_set_marker($$,ir_get_marker($1)); }
    ;
block_item
    : declaration
        { int m=ir_mark(); $$ = new_node("block_item \xe2\x86\x92 declaration");
          ir_set_marker($$,m); }
    | stmt
        { $$ = new_node("block_item \xe2\x86\x92 stmt");
          ir_set_marker($$,ir_get_marker($1)); }
    ;
stmt
    : compound_stmt  { $$ = new_node("stmt \xe2\x86\x92 compound_stmt");  ir_set_marker($$,ir_get_marker($1)); }
    | expr_stmt      { $$ = new_node("stmt \xe2\x86\x92 expr_stmt");      ir_set_marker($$,ir_get_marker($1)); }
    | selection_stmt { $$ = new_node("stmt \xe2\x86\x92 selection_stmt"); ir_set_marker($$,ir_get_marker($1)); }
    | iteration_stmt { $$ = new_node("stmt \xe2\x86\x92 iteration_stmt"); ir_set_marker($$,ir_get_marker($1)); }
    | jump_stmt      { $$ = new_node("stmt \xe2\x86\x92 jump_stmt");      ir_set_marker($$,ir_get_marker($1)); }
    | labeled_stmt   { $$ = new_node("stmt \xe2\x86\x92 labeled_stmt");   ir_set_marker($$,ir_get_marker($1)); }
    ;
expr_stmt
    : ';'
        { int m=ir_mark(); $$ = new_node("expr_stmt \xe2\x86\x92 ;"); ir_set_marker($$,m); }
    | expr ';'
        { $$ = new_node("expr_stmt \xe2\x86\x92 expr ;");
          ir_setplace($$, ir_getplace($1));
          ir_setplace_addr($$, ir_getplace_addr($1));
          ir_set_marker($$,ir_get_marker($1)); }
    ;

/* ================================================================
   SELECTION STATEMENTS
   ================================================================ */

selection_stmt
    : IF '(' expr ')' stmt  %prec LOWER_THAN_ELSE
        {
            cnt_if_without_else++;
            ifd_cur++; if(ifd_cur>ifd_max)ifd_max=ifd_cur;

            const char *cond  = ir_getplace($3);
            const char *Lend  = ir_newlabel();
            int M_cond = ir_get_marker($3);
            int M_s1   = ir_get_marker($5);

            if (M_s1 >= 0) {
                ir_insert_before(M_s1, "jf", cond, "", Lend);
                ir_delete(M_s1 + 1);
            } else {
                ir_emit("jf", cond, "", Lend);
            }
            ir_emit("label", "", "", Lend);
            if (M_cond >= 0) ir_delete(M_cond);

            ifd_cur--;
            $$ = new_node("selection_stmt \xe2\x86\x92 if ( expr ) stmt");
            ir_set_marker($$, -1);
        }

    | IF '(' expr ')' stmt ELSE stmt
        {
            cnt_if_with_else++;
            ifd_cur++; if(ifd_cur>ifd_max)ifd_max=ifd_cur;

            const char *cond  = ir_getplace($3);
            const char *Lelse = ir_newlabel();
            const char *Lend  = ir_newlabel();
            int M_cond = ir_get_marker($3);
            int M_s1   = ir_get_marker($5);
            int M_s2   = ir_get_marker($7);

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
            $$ = new_node("selection_stmt \xe2\x86\x92 if ( expr ) stmt else stmt");
            ir_set_marker($$, -1);
        }

    | SWITCH '(' expr ')'
        {
            /*
             * FIX BUG 1 MID-RULE: Push switch context and increment
             * switch_depth so break inside the body is accepted.
             * We allocate Lend here and store it in the switch context.
             */
            const char *Lend = ir_newlabel();
            sw_push(ir_getplace($3), Lend);
            switch_depth++;
        }
      '{' case_list '}'
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

            $$ = new_node("selection_stmt \xe2\x86\x92 switch ( expr ) { case_list }");
            ir_set_marker($$, -1);
        }
    ;

case_list
    : labeled_stmt
        { $$ = new_node("case_list \xe2\x86\x92 labeled_stmt"); }
    | case_list labeled_stmt
        { $$ = new_node("case_list \xe2\x86\x92 case_list labeled_stmt"); }
    ;
case_body_list
    : /* empty */
        { int m=ir_mark(); $$ = new_node("case_body_list \xe2\x86\x92 <empty>"); ir_set_marker($$,m); }
    | case_body_list non_case_stmt
        { $$ = new_node("case_body_list \xe2\x86\x92 case_body_list non_case_stmt"); ir_set_marker($$,ir_get_marker($1)); }
    ;
labeled_stmt
    : CASE expr ':' case_body_list
        {
            /*
             * FIX BUG 1: Register this case with the switch context and
             * emit a label quad so the case body is reachable from the
             * dispatch chain.  The dispatch quads themselves are inserted
             * retroactively by the SWITCH final action above.
             */
            const char *clbl = sw_add_case(ir_getplace($2));
            /* Insert the case label BEFORE the first quad of the body.
               ir_get_marker($4) is the marker at the start of the body;
               if it is -1 (empty body), just append. */
            int mbody = ir_get_marker($4);
            if (mbody >= 0)
                ir_insert_before(mbody, "label","","",clbl);
            else
                ir_emit("label","","",clbl);
            $$ = new_node("labeled_stmt \xe2\x86\x92 case expr : case_body_list");
            ir_set_marker($$, ir_get_marker($2));
        }
    | DEFAULT ':' case_body_list
        {
            const char *dlbl = sw_add_default();
            int mbody = ir_get_marker($3);
            if (mbody >= 0)
                ir_insert_before(mbody, "label","","",dlbl);
            else
                ir_emit("label","","",dlbl);
            $$ = new_node("labeled_stmt \xe2\x86\x92 default : case_body_list");
            ir_set_marker($$, ir_get_marker($3));
        }
    ;
non_case_stmt
    : compound_stmt  { $$ = new_node("non_case_stmt \xe2\x86\x92 compound_stmt");  ir_set_marker($$,ir_get_marker($1)); }
    | expr_stmt      { $$ = new_node("non_case_stmt \xe2\x86\x92 expr_stmt");      ir_set_marker($$,ir_get_marker($1)); }
    | selection_stmt { $$ = new_node("non_case_stmt \xe2\x86\x92 selection_stmt"); ir_set_marker($$,ir_get_marker($1)); }
    | iteration_stmt { $$ = new_node("non_case_stmt \xe2\x86\x92 iteration_stmt"); ir_set_marker($$,ir_get_marker($1)); }
    | jump_stmt      { $$ = new_node("non_case_stmt \xe2\x86\x92 jump_stmt");      ir_set_marker($$,ir_get_marker($1)); }
    ;

/* ================================================================
   ITERATION STATEMENTS
   ================================================================ */

iteration_stmt
    : WHILE '('
        {
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            ir_emit("label","","",Lstart);
            loop_push(Lstart, Lend);
            $<node_idx>$ = (int)(Lstart - (const char *)0);
        }
      expr ')'
        {
            const char *cond = ir_getplace($4);
            const char *Lend = loop_cur_lend();
            if (cond && cond[0] && Lend)
                ir_emit("jf", cond, "", Lend);
        }
      stmt
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

            $$ = new_node("iteration_stmt \xe2\x86\x92 while ( expr ) stmt");
            ir_set_marker($$, -1);
        }

    | DO
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
            $<node_idx>$ = ir_n;  /* body_start: first body quad will be at this index */
        }
      stmt WHILE '(' expr ')' ';'
        {
            cnt_do_while_loops++;
            const char *Lstart = loop_cur_lstart();
            const char *Lend   = loop_cur_lend();
            const char *cond   = ir_getplace($6);

            /*
             * BUG A FIX: Use the mid-rule recorded body_start ($<node_idx>2)
             * instead of ir_get_marker($3).  This is always the correct
             * first-body-quad position regardless of stmt type.
             */
            int body_start = $<node_idx>2;

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

            $$ = new_node("iteration_stmt \xe2\x86\x92 do stmt while ( expr ) ;");
            ir_set_marker($$, -1);
        }

    | FOR '(' expr_stmt expr_stmt ')' stmt
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace($4);
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker($4);
            int M_body = ir_get_marker($6);

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

            $$ = new_node("iteration_stmt \xe2\x86\x92 for ( expr_stmt expr_stmt ) stmt");
            ir_set_marker($$, -1);
        }

    | FOR '(' expr_stmt expr_stmt expr ')' stmt
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace($4);
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker($4);
            int M_step = ir_get_marker($5);
            int M_body = ir_get_marker($7);

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

            $$ = new_node("iteration_stmt \xe2\x86\x92 for ( expr_stmt expr_stmt expr ) stmt");
            ir_set_marker($$, -1);
        }

    | FOR '(' declaration expr_stmt ')' stmt
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace($4);
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker($4);
            int M_body = ir_get_marker($6);

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

            $$ = new_node("iteration_stmt \xe2\x86\x92 for ( declaration expr_stmt ) stmt");
            ir_set_marker($$, -1);
        }

    | FOR '(' declaration expr_stmt expr ')' stmt
        {
            cnt_for_loops++;
            const char *cond   = ir_getplace($4);
            const char *Lstart = ir_newlabel();
            const char *Lend   = ir_newlabel();
            int M_cond = ir_get_marker($4);
            int M_step = ir_get_marker($5);
            int M_body = ir_get_marker($7);

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

            $$ = new_node("iteration_stmt \xe2\x86\x92 for ( declaration expr_stmt expr ) stmt");
            ir_set_marker($$, -1);
        }
    ;

jump_stmt
    : RETURN ';'
        { cnt_return_stmts++;
          if (!(sem_current_return_base == SEM_VOID && sem_current_return_ptr == 0))
              sem_errorf("non-void function must return a value");
          int m=ir_mark(); ir_emit("return","","","");
          $$ = new_node("jump_stmt \xe2\x86\x92 return ;"); ir_set_marker($$,m); }
    | RETURN expr ';'
        { cnt_return_stmts++;
          if (!((sem_is_numeric(sem_current_return_base, sem_current_return_ptr) &&
                 sem_is_numeric(sem_base_node[$2], sem_ptr_node[$2])) ||
                sem_types_match_exact(sem_current_return_base, sem_current_return_ptr,
                                      sem_base_node[$2], sem_ptr_node[$2]) ||
                (sem_current_return_ptr > 0 && sem_is_null_literal($2)))) {
              char ret_buf[32], expr_buf[32];
              sem_type_to_string(sem_current_return_base, sem_current_return_ptr, ret_buf, sizeof ret_buf);
              sem_type_to_string(sem_base_node[$2], sem_ptr_node[$2], expr_buf, sizeof expr_buf);
              sem_errorf("return type mismatch: function returns %s but expression is %s", ret_buf, expr_buf);
          }
          ir_emit("return",ir_getplace($2),"","");
          $$ = new_node("jump_stmt \xe2\x86\x92 return expr ;"); ir_set_marker($$,ir_get_marker($2)); }
    | BREAK ';'
        { if (loop_depth <= 0 && switch_depth <= 0) sem_errorf("'break' used outside loop or switch");
          int m=ir_mark(); ir_emit("break","","","");
          $$ = new_node("jump_stmt \xe2\x86\x92 break ;"); ir_set_marker($$,m); }
    | CONTINUE ';'
        { if (loop_depth <= 0) sem_errorf("'continue' used outside loop");
          int m=ir_mark(); ir_emit("continue","","","");
          $$ = new_node("jump_stmt \xe2\x86\x92 continue ;"); ir_set_marker($$,m); }
    ;

/* ================================================================
   EXPRESSION RULES
   ================================================================ */

expr
    : assignment_expr
        { $$ = new_node("expr \xe2\x86\x92 assignment_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | expr ',' assignment_expr
        { $$ = new_node("expr \xe2\x86\x92 expr , assignment_expr");
          ir_setplace($$,ir_getplace($3)); ir_setplace_addr($$, ir_getplace_addr($3)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $3); }
    ;

assignment_expr
    : conditional_expr
        { $$ = new_node("assignment_expr \xe2\x86\x92 conditional_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | unary_expr '=' assignment_expr
        { cnt_assignments++;
          if (!sem_lvalue_node[$1]) sem_errorf("left-hand side of assignment is not assignable");
          if (!sem_assignment_compatible($1, $3)) {
              char lhs_buf[32], rhs_buf[32];
              sem_type_to_string(sem_base_node[$1], sem_ptr_node[$1], lhs_buf, sizeof lhs_buf);
              sem_type_to_string(sem_base_node[$3], sem_ptr_node[$3], rhs_buf, sizeof rhs_buf);
              sem_errorf("cannot assign expression of type %s to lvalue of type %s", rhs_buf, lhs_buf);
          }
          const char *lhs=ir_getplace($1),*rhs=ir_getplace($3);
          const char *lhs_addr = ir_getplace_addr($1);
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
          $$ = new_node("assignment_expr \xe2\x86\x92 unary_expr = assignment_expr");
          ir_setplace($$,lhs); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); sem_lvalue_node[$$] = 0; }
    | unary_expr ASSIGN_OP assignment_expr
        { cnt_assignments++;
          if (!sem_lvalue_node[$1]) sem_errorf("left-hand side of assignment is not assignable");
          if (!sem_assignment_compatible($1, $3))
              sem_errorf("compound assignment has incompatible operand types");
          sem_require_numeric_binary($2 ? $2 : "compound assignment", $1, $3);
          const char *lhs=ir_getplace($1),*rhs=ir_getplace($3);
          const char *optext = $2 ? $2 : "=";
          char baseop[8]; size_t ol = strlen(optext); if (ol>0 && optext[ol-1]=='=') ol--; if(ol>7) ol=7; strncpy(baseop, optext, ol); baseop[ol]='\0';
          const char *t = ir_newtemp();
          ir_emit(baseop, lhs, rhs, t);
          const char *lhs_addr = ir_getplace_addr($1);
          if (lhs_addr && lhs_addr[0]) {
              ir_emit("store", t, "", lhs_addr);
          } else {
              ir_emit("=", t, "", lhs);
          }
          $$ = new_node("assignment_expr \xe2\x86\x92 unary_expr ASSIGN_OP assignment_expr");
          ir_setplace($$,lhs); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); sem_lvalue_node[$$] = 0; }
    ;

conditional_expr
    : logical_or_expr
        { $$ = new_node("conditional_expr \xe2\x86\x92 logical_or_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | logical_or_expr '?' expr ':' conditional_expr
        { const char *cond=ir_getplace($1),*e1=ir_getplace($3),*e2=ir_getplace($5);
          if (!sem_is_scalar(sem_base_node[$1], sem_ptr_node[$1]))
              sem_errorf("conditional operator requires scalar condition");
          if (!(sem_types_match_exact(sem_base_node[$3], sem_ptr_node[$3], sem_base_node[$5], sem_ptr_node[$5]) ||
                (sem_is_numeric(sem_base_node[$3], sem_ptr_node[$3]) && sem_is_numeric(sem_base_node[$5], sem_ptr_node[$5]))))
              sem_errorf("conditional branches must have compatible types");
          const char *t=ir_newtemp(),*lf=ir_newlabel(),*le=ir_newlabel();
          ir_emit("jf",cond,"",lf); ir_emit("=",e1,"",t);
          ir_emit("goto","","",le); ir_emit("label","","",lf);
          ir_emit("=",e2,"",t); ir_emit("label","","",le);
          $$ = new_node("conditional_expr \xe2\x86\x92 logical_or_expr ? expr : conditional_expr");
          ir_setplace($$,t); ir_set_marker($$,ir_get_marker($1));
          if (sem_is_numeric(sem_base_node[$3], sem_ptr_node[$3]) && sem_is_numeric(sem_base_node[$5], sem_ptr_node[$5]))
              sem_set_binary_result_numeric($$, $3, $5);
          else sem_copy_node($$, $3);
          sem_lvalue_node[$$] = 0; }
    ;

logical_or_expr
    : logical_and_expr
        { $$ = new_node("logical_or_expr \xe2\x86\x92 logical_and_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }

    | logical_or_expr OR_OP
        {
            sc_or_ltrue[sc_or_depth] = ir_newlabel();
            ir_emit("jt", ir_getplace($1), "", sc_or_ltrue[sc_or_depth]);
            $<node_idx>$ = sc_or_depth++;
        }
      logical_and_expr
        {
            int d = $<node_idx>3;
            sc_or_depth--;
            const char *Ltrue  = sc_or_ltrue[d];
            const char *Lend   = ir_newlabel();
            const char *t      = ir_newtemp();
            ir_emit("jt",  ir_getplace($4), "", Ltrue);
            ir_emit("=",   "0", "", t);
            ir_emit("goto","",  "", Lend);
            ir_emit("label","", "", Ltrue);
            ir_emit("=",   "1", "", t);
            ir_emit("label","", "", Lend);
            $$ = new_node("logical_or_expr \xe2\x86\x92 logical_or_expr || logical_and_expr");
            ir_setplace($$, t);
            ir_set_marker($$, ir_get_marker($1));
            if (!sem_is_scalar(sem_base_node[$1], sem_ptr_node[$1]) ||
                !sem_is_scalar(sem_base_node[$4], sem_ptr_node[$4]))
                sem_errorf("operator '||' requires scalar operands");
            sem_set_node($$, SEM_INT, 0, 0);
        }
    ;

logical_and_expr
    : inclusive_or_expr
        { $$ = new_node("logical_and_expr \xe2\x86\x92 inclusive_or_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }

    | logical_and_expr AND_OP
        {
            sc_and_lfalse[sc_and_depth] = ir_newlabel();
            ir_emit("jf", ir_getplace($1), "", sc_and_lfalse[sc_and_depth]);
            $<node_idx>$ = sc_and_depth++;
        }
      inclusive_or_expr
        {
            int d = $<node_idx>3;
            sc_and_depth--;
            const char *Lfalse = sc_and_lfalse[d];
            const char *Lend   = ir_newlabel();
            const char *t      = ir_newtemp();
            ir_emit("jf",  ir_getplace($4), "", Lfalse);
            ir_emit("=",   "1", "", t);
            ir_emit("goto","",  "", Lend);
            ir_emit("label","", "", Lfalse);
            ir_emit("=",   "0", "", t);
            ir_emit("label","", "", Lend);
            $$ = new_node("logical_and_expr \xe2\x86\x92 logical_and_expr && inclusive_or_expr");
            ir_setplace($$, t);
            ir_set_marker($$, ir_get_marker($1));
            if (!sem_is_scalar(sem_base_node[$1], sem_ptr_node[$1]) ||
                !sem_is_scalar(sem_base_node[$4], sem_ptr_node[$4]))
                sem_errorf("operator '&&' requires scalar operands");
            sem_set_node($$, SEM_INT, 0, 0);
        }
    ;

inclusive_or_expr
    : exclusive_or_expr
        { $$ = new_node("inclusive_or_expr \xe2\x86\x92 exclusive_or_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | inclusive_or_expr '|' exclusive_or_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_integer_binary("|", $1, $3);
          ir_emit("|",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("inclusive_or_expr \xe2\x86\x92 inclusive_or_expr | exclusive_or_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
exclusive_or_expr
    : and_expr
        { $$ = new_node("exclusive_or_expr \xe2\x86\x92 and_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | exclusive_or_expr '^' and_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_integer_binary("^", $1, $3);
          ir_emit("^",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("exclusive_or_expr \xe2\x86\x92 exclusive_or_expr ^ and_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
and_expr
    : equality_expr
        { $$ = new_node("and_expr \xe2\x86\x92 equality_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | and_expr '&' equality_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_integer_binary("&", $1, $3);
          ir_emit("&",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("and_expr \xe2\x86\x92 and_expr & equality_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
equality_expr
    : relational_expr
        { $$ = new_node("equality_expr \xe2\x86\x92 relational_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | equality_expr EQ_OP relational_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          if (!(sem_types_match_exact(sem_base_node[$1], sem_ptr_node[$1], sem_base_node[$3], sem_ptr_node[$3]) ||
                (sem_is_numeric(sem_base_node[$1], sem_ptr_node[$1]) && sem_is_numeric(sem_base_node[$3], sem_ptr_node[$3]))))
              sem_errorf("operator '==' requires compatible operands");
          ir_emit("==",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("equality_expr \xe2\x86\x92 equality_expr == relational_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | equality_expr NE_OP relational_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          if (!(sem_types_match_exact(sem_base_node[$1], sem_ptr_node[$1], sem_base_node[$3], sem_ptr_node[$3]) ||
                (sem_is_numeric(sem_base_node[$1], sem_ptr_node[$1]) && sem_is_numeric(sem_base_node[$3], sem_ptr_node[$3]))))
              sem_errorf("operator '!=' requires compatible operands");
          ir_emit("!=",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("equality_expr \xe2\x86\x92 equality_expr != relational_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
relational_expr
    : shift_expr
        { $$ = new_node("relational_expr \xe2\x86\x92 shift_expr");
          ir_setplace($$,ir_getplace($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | relational_expr '<' shift_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary("<", $1, $3);
          ir_emit("<",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("relational_expr \xe2\x86\x92 relational_expr < shift_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | relational_expr '>' shift_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary(">", $1, $3);
          ir_emit(">",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("relational_expr \xe2\x86\x92 relational_expr > shift_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | relational_expr LE_OP shift_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary("<=", $1, $3);
          ir_emit("<=",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("relational_expr \xe2\x86\x92 relational_expr <= shift_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | relational_expr GE_OP shift_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary(">=", $1, $3);
          ir_emit(">=",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("relational_expr \xe2\x86\x92 relational_expr >= shift_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
shift_expr
    : additive_expr
        { $$ = new_node("shift_expr \xe2\x86\x92 additive_expr");
          ir_setplace($$,ir_getplace($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | shift_expr LEFT_SHIFT additive_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_integer_binary("<<", $1, $3);
          ir_emit("<<",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("shift_expr \xe2\x86\x92 shift_expr << additive_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | shift_expr RIGHT_SHIFT additive_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_integer_binary(">>", $1, $3);
          ir_emit(">>",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("shift_expr \xe2\x86\x92 shift_expr >> additive_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
additive_expr
    : multiplicative_expr
        { $$ = new_node("additive_expr \xe2\x86\x92 multiplicative_expr");
          ir_setplace($$,ir_getplace($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | additive_expr '+' multiplicative_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary("+", $1, $3);
          ir_emit("+",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("additive_expr \xe2\x86\x92 additive_expr + multiplicative_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_binary_result_numeric($$, $1, $3); }
    | additive_expr '-' multiplicative_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary("-", $1, $3);
          ir_emit("-",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("additive_expr \xe2\x86\x92 additive_expr - multiplicative_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_binary_result_numeric($$, $1, $3); }
    ;
multiplicative_expr
    : unary_expr
        { $$ = new_node("multiplicative_expr \xe2\x86\x92 unary_expr");
          ir_setplace($$,ir_getplace($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | multiplicative_expr '*' unary_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary("*", $1, $3);
          ir_emit("*",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("multiplicative_expr \xe2\x86\x92 multiplicative_expr * unary_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_binary_result_numeric($$, $1, $3); }
    | multiplicative_expr '/' unary_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_numeric_binary("/", $1, $3);
          if (sem_is_null_literal($3))
              sem_errorf("division by zero in expression");
          ir_emit("/",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("multiplicative_expr \xe2\x86\x92 multiplicative_expr / unary_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_binary_result_numeric($$, $1, $3); }
    | multiplicative_expr '%' unary_expr
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          sem_require_integer_binary("%", $1, $3);
          if (sem_is_null_literal($3))
              sem_errorf("modulo by zero in expression");
          ir_emit("%",ir_getplace($1),ir_getplace($3),t);
          $$ = new_node("multiplicative_expr \xe2\x86\x92 multiplicative_expr % unary_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;
unary_expr
    : postfix_expr
        { $$ = new_node("unary_expr \xe2\x86\x92 postfix_expr");
          ir_setplace($$,ir_getplace($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | INC_OP unary_expr
        { int m=ir_get_marker($2); const char*p=ir_getplace($2);
          if (!sem_lvalue_node[$2] || !sem_is_numeric(sem_base_node[$2], sem_ptr_node[$2]))
              sem_errorf("operator '++' requires an assignable numeric operand");
          ir_emit("+",p,"1",p);
          $$ = new_node("unary_expr \xe2\x86\x92 ++ unary_expr");
          ir_setplace($$,p); ir_set_marker($$,m); sem_copy_node($$, $2); sem_lvalue_node[$$] = 0; }
    | DEC_OP unary_expr
        { int m=ir_get_marker($2); const char*p=ir_getplace($2);
          if (!sem_lvalue_node[$2] || !sem_is_numeric(sem_base_node[$2], sem_ptr_node[$2]))
              sem_errorf("operator '--' requires an assignable numeric operand");
          ir_emit("-",p,"1",p);
          $$ = new_node("unary_expr \xe2\x86\x92 -- unary_expr");
          ir_setplace($$,p); ir_set_marker($$,m); sem_copy_node($$, $2); sem_lvalue_node[$$] = 0; }
    | '-' unary_expr   %prec UMINUS
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_is_numeric(sem_base_node[$2], sem_ptr_node[$2]))
              sem_errorf("unary '-' requires a numeric operand");
          ir_emit("minus",ir_getplace($2),"",t);
          $$ = new_node("unary_expr \xe2\x86\x92 - unary_expr [UMINUS]");
          ir_setplace($$,t); ir_set_marker($$,m); sem_copy_node($$, $2); sem_lvalue_node[$$] = 0; }
    | '+' unary_expr   %prec UPLUS
        { $$ = new_node("unary_expr \xe2\x86\x92 + unary_expr [UPLUS]");
          if (!sem_is_numeric(sem_base_node[$2], sem_ptr_node[$2]))
              sem_errorf("unary '+' requires a numeric operand");
          ir_setplace($$,ir_getplace($2)); ir_setplace_addr($$, ir_getplace_addr($2)); ir_set_marker($$,ir_get_marker($2));
          sem_copy_node($$, $2); sem_lvalue_node[$$] = 0; }
    | '!' unary_expr
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_is_scalar(sem_base_node[$2], sem_ptr_node[$2]))
              sem_errorf("operator '!' requires a scalar operand");
          ir_emit("!",ir_getplace($2),"",t);
          $$ = new_node("unary_expr \xe2\x86\x92 ! unary_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | '~' unary_expr
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_is_integer_like(sem_base_node[$2], sem_ptr_node[$2]))
              sem_errorf("operator '~' requires an integer operand");
          ir_emit("~",ir_getplace($2),"",t);
          $$ = new_node("unary_expr \xe2\x86\x92 ~ unary_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | '*' unary_expr
        {
            int m=ir_mark(); const char*t=ir_newtemp();
            const char *ptr = ir_getplace($2);
            if (sem_ptr_node[$2] <= 0)
                sem_errorf("cannot dereference non-pointer expression");
            ir_emit("deref", ptr, "", t);
            $$ = new_node("unary_expr \xe2\x86\x92 * unary_expr [deref]");
            ir_setplace_addr($$, ptr);
            ir_setplace($$, t);
            ir_set_marker($$, m);
            sem_set_node($$, sem_base_node[$2], sem_ptr_node[$2] - 1, 1);
        }
    | '&' unary_expr
        { int m=ir_mark(); const char*t=ir_newtemp();
          if (!sem_lvalue_node[$2]) sem_errorf("operator '&' requires an lvalue operand");
          ir_emit("addr",ir_getplace($2),"",t);
          $$ = new_node("unary_expr \xe2\x86\x92 & unary_expr [addr-of]");
          ir_setplace($$,t); ir_set_marker($$,m);
          sem_set_node($$, sem_base_node[$2], sem_ptr_node[$2] + 1, 0); }
    | SIZEOF unary_expr
        { int m=ir_mark(); const char*t=ir_newtemp();
          ir_emit("sizeof",ir_getplace($2),"",t);
          $$ = new_node("unary_expr \xe2\x86\x92 sizeof unary_expr");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | SIZEOF '(' type_spec ')'
        { int m=ir_mark(); const char*t=ir_newtemp();
          ir_emit("sizeof_type","type","",t);
          $$ = new_node("unary_expr \xe2\x86\x92 sizeof ( type_spec )");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    ;

/* ================================================================
   postfix_expr
   FIX BUG 1: Call sites use param_push_level / param_record /
   param_flush_and_pop so that all param quads for a call are emitted
   together, immediately before the call quad, regardless of how
   deeply nested the argument expressions are.

   FIX BUG 3: Post-increment / post-decrement used as a statement
   previously saved the old value into a temporary that was never
   referenced again. The dead-temp elimination pass in ir_print()
   handles this generically, but the postfix rules are also written to
   always save the old value (the old value IS needed when post-
   increment appears as a sub-expression, e.g. a[i++]).  The
   elimination pass will remove it when it is genuinely dead.
   ================================================================ */
postfix_expr
    : primary_expr
        { $$ = new_node("postfix_expr \xe2\x86\x92 primary_expr");
          ir_setplace($$,ir_getplace($1)); ir_setplace_addr($$, ir_getplace_addr($1)); ir_set_marker($$,ir_get_marker($1));
          sem_copy_node($$, $1); }
    | postfix_expr '[' expr ']'
        {
            int m=ir_get_marker($1);
            const char *addr = ir_newtemp();
            const char *val  = ir_newtemp();
            if (sem_ptr_node[$1] <= 0)
                sem_errorf("subscripted expression is not an array or pointer");
            if (!sem_is_integer_like(sem_base_node[$3], sem_ptr_node[$3]))
                sem_errorf("array index must be an integer expression");
            ir_emit("elemaddr", ir_getplace($1), ir_getplace($3), addr);
            ir_emit("deref",    addr, "", val);
            $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr [ expr ]");
            ir_setplace_addr($$, addr);
            ir_setplace($$, val);
            ir_set_marker($$, m);
            sem_set_node($$, sem_base_node[$1], sem_ptr_node[$1] - 1, 1);
        }
    | postfix_expr '('
        {
            /*
             * FIX BUG 1 MID-RULE: Push a new param-buffer level.
             * All argument expressions reduced while this level is
             * active will have their places recorded here instead of
             * being emitted immediately as param quads.
             */
            param_push_level();
        }
      ')'
        {
            cnt_func_calls++;
            int m=ir_get_marker($1);
            const char *t=ir_newtemp();
            if (!sem_func_node[$1])
                sem_errorf("attempt to call a non-function expression");
            if (sem_paramcount_node[$1] > 0)
                sem_errorf("function '%s' expects %d argument(s), got 0",
                           ir_getplace($1), sem_paramcount_node[$1]);
            /* Flush buffered params (none in this case) then pop */
            param_flush_and_pop();
            ir_emit("call",ir_getplace($1),"0",t);
            $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr ( )");
            ir_setplace($$,t); ir_set_marker($$,m);
            sem_set_node($$, sem_base_node[$1], sem_ptr_node[$1], 0);
        }
    | postfix_expr '('
        {
            /* FIX BUG 1 MID-RULE: push param-buffer level */
            param_push_level();
        }
      arg_list ')'
        {
            cnt_func_calls++;
            int m=ir_get_marker($1);
            const char *t=ir_newtemp();
            int ac = ir_get_argcount($4);
            char acbuf[32]; snprintf(acbuf,sizeof acbuf,"%d",ac);
            if (!sem_func_node[$1])
                sem_errorf("attempt to call a non-function expression");
            if (sem_paramcount_node[$1] >= 0 && sem_paramcount_node[$1] != ac)
                sem_errorf("function '%s' expects %d argument(s), got %d",
                           ir_getplace($1), sem_paramcount_node[$1], ac);
            /*
             * FIX BUG 1: Flush all buffered param quads for this call
             * level NOW, immediately before the call quad.  This ensures
             * that all params appear together in the IR regardless of
             * how complex the argument expressions were (including nested
             * function calls).
             */
            param_flush_and_pop();
            ir_emit("call",ir_getplace($1),acbuf,t);
            $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr ( arg_list )");
            ir_setplace($$,t); ir_set_marker($$,m);
            sem_set_node($$, sem_base_node[$1], sem_ptr_node[$1], 0);
        }
    | postfix_expr '.' IDENTIFIER
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          ir_emit(".",ir_getplace($1),$3,t);
          $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr . IDENTIFIER");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_UNKNOWN, 0, 1); }
    | postfix_expr ARROW IDENTIFIER
        { int m=ir_get_marker($1); const char*t=ir_newtemp();
          ir_emit("->",ir_getplace($1),$3,t);
          $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr -> IDENTIFIER");
          ir_setplace($$,t); ir_set_marker($$,m); sem_set_node($$, SEM_UNKNOWN, 0, 1); }
    | postfix_expr INC_OP
        {
            /*
             * FIX BUG 3: We still emit the save of the old value because
             * in general  a[i++]  or  f(x++)  do need it.  When this
             * appears as a lone statement the dead-temp elimination pass
             * in ir_print() will silently remove the unused save quad.
             */
            int m=ir_get_marker($1);
            const char *p=ir_getplace($1);
            const char *t=ir_newtemp();
            if (!sem_lvalue_node[$1] || !sem_is_numeric(sem_base_node[$1], sem_ptr_node[$1]))
                sem_errorf("operator '++' requires an assignable numeric operand");
            ir_emit("=",p,"",t);
            ir_emit("+",p,"1",p);
            $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr ++");
            ir_setplace($$,t); ir_set_marker($$,m);
            sem_copy_node($$, $1); sem_lvalue_node[$$] = 0;
        }
    | postfix_expr DEC_OP
        {
            int m=ir_get_marker($1);
            const char *p=ir_getplace($1);
            const char *t=ir_newtemp();
            if (!sem_lvalue_node[$1] || !sem_is_numeric(sem_base_node[$1], sem_ptr_node[$1]))
                sem_errorf("operator '--' requires an assignable numeric operand");
            ir_emit("=",p,"",t);
            ir_emit("-",p,"1",p);
            $$ = new_node("postfix_expr \xe2\x86\x92 postfix_expr --");
            ir_setplace($$,t); ir_set_marker($$,m);
            sem_copy_node($$, $1); sem_lvalue_node[$$] = 0;
        }
    ;

primary_expr
    : IDENTIFIER
        { int m=ir_mark();
          int sym = sem_lookup_symbol($1);
          if (sym < 0) sem_errorf("use of undeclared identifier '%s'", $1);
          $$ = new_node("primary_expr \xe2\x86\x92 IDENTIFIER");
          ir_setplace($$,ir_intern($1)); ir_set_marker($$,m);
          sem_set_node($$, sem_symbols[sym].base, sem_symbols[sym].ptr_depth, !sem_symbols[sym].is_function);
          sem_func_node[$$] = sem_symbols[sym].is_function;
          sem_paramcount_node[$$] = sem_symbols[sym].param_count; }
    | INT_CONST
        { cnt_int_consts++; int m=ir_mark();
          $$ = new_node("primary_expr \xe2\x86\x92 INT_CONST");
          ir_setplace($$,ir_intern($1)); ir_set_marker($$,m); sem_set_node($$, SEM_INT, 0, 0); }
    | FLOAT_CONST
        { int m=ir_mark();
          $$ = new_node("primary_expr \xe2\x86\x92 FLOAT_CONST");
          ir_setplace($$,ir_intern($1)); ir_set_marker($$,m); sem_set_node($$, SEM_FLOAT, 0, 0); }
    | CHAR_CONST
        { int m=ir_mark();
          $$ = new_node("primary_expr \xe2\x86\x92 CHAR_CONST");
          ir_setplace($$,ir_intern($1)); ir_set_marker($$,m); sem_set_node($$, SEM_CHAR, 0, 0); }
    | STRING_LITERAL
        { int m=ir_mark();
          $$ = new_node("primary_expr \xe2\x86\x92 STRING_LITERAL");
          ir_setplace($$,ir_intern($1)); ir_set_marker($$,m); sem_set_node($$, SEM_CHAR, 1, 0); }
    | '(' expr ')'
        { $$ = new_node("primary_expr \xe2\x86\x92 ( expr )");
          ir_setplace($$,ir_getplace($2)); ir_setplace_addr($$, ir_getplace_addr($2)); ir_set_marker($$,ir_get_marker($2));
          sem_copy_node($$, $2); }
    ;

/* ================================================================
   arg_list
   FIX BUG 1: Instead of emitting "param" quads immediately, record
   each argument's place in the deferred param buffer for this call
   level (param_record).  The actual param quads are flushed in the
   postfix_expr rule immediately before the call quad is emitted.
   ================================================================ */
arg_list
    : assignment_expr
        {
            /* Record arg in deferred buffer — do NOT emit param yet */
            param_record(ir_getplace($1));
            $$ = new_node("arg_list \xe2\x86\x92 assignment_expr");
            ir_set_marker($$,ir_get_marker($1));
            ir_set_argcount($$, 1);
        }
    | arg_list ',' assignment_expr
        {
            /* Record arg in deferred buffer — do NOT emit param yet */
            param_record(ir_getplace($3));
            $$ = new_node("arg_list \xe2\x86\x92 arg_list , assignment_expr");
            ir_set_marker($$,ir_get_marker($1));
            ir_set_argcount($$, ir_get_argcount($1)+1);
        }
    ;

%%

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
