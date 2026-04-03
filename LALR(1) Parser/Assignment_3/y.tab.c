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
 * cl_parser.y  —  LALR(1) Parser for CS327 Assignment 3
 *
 * Implements:
 *   Part 1  – LALR(1) automaton (via bison/yacc -v → y.output)
 *   Part 2  – Conflict identification and resolution (documented below)
 *   Part 3  – Reverse derivation tree after successful parse
 *   Part 4  – LALR(1) parsing table printed as a matrix
 *   Part 5  – Error diagnostics with line number and token name
 *   Part 6  – Additional information (grammar stats, conflict summary)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int  yylineno;
extern FILE *yyin;
extern char  last_token[256];   /* set by lexer on every token */
extern char *yytext;
extern int   yychar;            /* lookahead token at time of error */

int yylex(void);

/* ═══════════════════════════════════════════════════════════════════
   PART 3 — Reverse Derivation Tree (indented, tree-like structure)
   ═══════════════════════════════════════════════════════════════════
   HOW THE INDENTED TREE WORKS:
   ─────────────────────────────
   Each grammar rule has the form:  LHS → RHS
   We extract the LHS (left-hand side non-terminal) from the rule string.

   Every node stores:
     - rule      : the full rule string  e.g. "additive_expr → multiplicative_expr"
     - parent    : index of the node whose RHS contains this node as a child
     - depth     : indentation level (0 = root)

   To assign parents and depths we use a stack-based algorithm that
   mirrors the LALR(1) parse stack:
     - When we REDUCE using a rule with N symbols on the RHS, the top N
       nodes on the stack become children of the new node.
     - We push the new node onto the stack.
   This reconstructs the parse tree structure from the reduction sequence.

   After building the tree, we print it top-down with indentation:
     program
       translation_unit
         external_decl
           function_def
             function_header
               ...
   ═══════════════════════════════════════════════════════════════════ */

#define MAX_TREE_NODES 20000

typedef struct {
    char rule[512];    /* full rule string, e.g. "declaration → type_spec declarator_list ;" */
    char lhs[128];     /* left-hand side non-terminal, e.g. "declaration" */
    int  parent;       /* index of parent node (-1 = root) */
    int  depth;        /* indentation depth */
    int  children[32]; /* child indices (up to 32) */
    int  nchildren;
    int  line;         /* source line when this reduction occurred */
} TreeNode;

static TreeNode tree_nodes[MAX_TREE_NODES];
static int      tree_size = 0;
static int      reduction_count = 0;

/* Stack for tracking which nodes are still "unreduced" (not yet consumed
   as children of a higher rule). */
static int node_stack[MAX_TREE_NODES];
static int stack_top = 0;

/* ── PART 6: Per-parse construct counters ────────────────────────── */
static int cnt_global_decls    = 0;  /* global variable declarations  */
static int cnt_func_defs       = 0;  /* function definitions          */
static int cnt_if_without_else = 0;  /* if-without-else stmts         */
static int cnt_if_with_else    = 0;  /* if-else stmts                 */
static int cnt_for_loops       = 0;  /* for loops                     */
static int cnt_while_loops     = 0;  /* while loops                   */
static int cnt_do_while_loops  = 0;  /* do-while loops                */
static int cnt_switch_stmts    = 0;  /* switch statements             */
static int cnt_return_stmts    = 0;  /* return statements             */
static int cnt_ptr_decls       = 0;  /* pointer declarators           */
static int cnt_int_consts      = 0;  /* integer constants used        */
static int cnt_func_calls      = 0;  /* function call expressions     */
static int cnt_assignments     = 0;  /* assignment expressions        */
static int cnt_struct_decls    = 0;  /* struct/union declarations     */
static int if_depth_current    = 0;  /* current if nesting depth      */
static int if_depth_max        = 0;  /* maximum if nesting depth seen */

/* Extract the LHS name from a rule string like "foo → bar baz"
   Result is written into dest (up to destlen-1 chars). */
static void extract_lhs(const char *rule, char *dest, int destlen) {
    /* Copy up to the → arrow (multi-byte: UTF-8 0xE2 0x86 0x92) */
    const char *arrow = strstr(rule, "→");  /* UTF-8 arrow = → */
    if (!arrow) {
        /* fallback: copy whole string */
        strncpy(dest, rule, destlen - 1);
        dest[destlen - 1] = '\0';
        return;
    }
    int len = (int)(arrow - rule);
    if (len >= destlen) len = destlen - 1;
    strncpy(dest, rule, len);
    dest[len] = '\0';
    /* trim trailing spaces */
    while (len > 0 && dest[len-1] == ' ') dest[--len] = '\0';
}

/* Count the number of NON-TERMINAL children in a rule's RHS.
   We only count words that are known non-terminal names, because only
   non-terminals get pushed onto the node_stack by new_node().
   Terminals (IDENTIFIER, INT_CONST, ';', 'int', etc.) are never pushed
   and must NOT be counted — otherwise the depth calculation explodes.

   The complete set of non-terminals is taken from the %type declarations
   in the grammar.  If you add a new non-terminal, add it here too.       */
static int count_rhs_symbols(const char *rule) {
    /* All non-terminal names from %type declarations */
    static const char *nt_names[] = {
        "additive_expr", "and_expr", "arg_list", "assignment_expr",
        "block_item", "block_item_list", "case_body_list", "case_list",
        "compound_stmt", "conditional_expr", "declaration", "declarator",
        "declarator_list", "enum_decl", "enumerator", "enumerator_list",
        "equality_expr", "exclusive_or_expr", "expr", "expr_stmt",
        "external_decl", "function_def", "function_header",
        "inclusive_or_expr", "initializer", "initializer_list",
        "iteration_stmt", "jump_stmt", "labeled_stmt",
        "logical_and_expr", "logical_or_expr",
        "member_decl", "member_decl_list", "multiplicative_expr",
        "non_case_stmt", "param_decl", "param_list",
        "postfix_expr", "primary_expr", "program",
        "relational_expr", "selection_stmt", "shift_expr", "stmt",
        "storage_class_list", "storage_class_spec", "struct_decl",
        "translation_unit", "type_qualifier", "type_spec", "unary_expr",
        NULL
    };

    const char *arrow = strstr(rule, "\xe2\x86\x92");  /* UTF-8 → */
    if (!arrow) return 0;
    const char *p = arrow + 3;  /* skip 3-byte → */
    while (*p == ' ') p++;
    if (*p == '\0') return 0;  /* empty RHS */

    /* Walk through space-separated words in RHS, count non-terminals */
    int count = 0;
    char word[128];
    int  wlen = 0;

    while (1) {
        if (*p == ' ' || *p == '\t' || *p == '\0') {
            if (wlen > 0) {
                word[wlen] = '\0';
                /* Check if this word is a known non-terminal */
                for (int i = 0; nt_names[i]; i++) {
                    if (strcmp(word, nt_names[i]) == 0) { count++; break; }
                }
                wlen = 0;
            }
            if (*p == '\0') break;
        } else {
            if (wlen < 127) word[wlen++] = *p;
        }
        p++;
    }
    return count;
}

int new_node(const char *rule) {
    if (tree_size >= MAX_TREE_NODES) {
        fprintf(stderr, "  [Warning] Derivation tree overflow at %d nodes\n", MAX_TREE_NODES);
        return -1;
    }

    int idx = tree_size++;
    TreeNode *n = &tree_nodes[idx];

    strncpy(n->rule, rule, 511);
    n->rule[511] = '\0';
    extract_lhs(rule, n->lhs, sizeof(n->lhs));
    n->parent   = -1;
    n->depth    = 0;
    n->nchildren = 0;
    n->line     = yylineno;
    reduction_count++;

    /* Pop RHS symbols off the stack and make them children of this node */
    int rhs = count_rhs_symbols(rule);
    if (rhs > stack_top) rhs = stack_top;  /* safety */

    /* The top `rhs` items on the stack are the children (in reverse order) */
    int child_start = stack_top - rhs;
    for (int i = child_start; i < stack_top; i++) {
        int child = node_stack[i];
        if (n->nchildren < 32) {
            n->children[n->nchildren++] = child;
        }
        tree_nodes[child].parent = idx;
    }
    stack_top -= rhs;

    /* Push this new node */
    node_stack[stack_top++] = idx;

    return idx;
}

/* Recursively print the tree top-down with indentation */
static void print_subtree(int idx, int depth) {
    if (idx < 0 || idx >= tree_size) return;
    TreeNode *n = &tree_nodes[idx];

    /* Print indentation — 2 spaces per level */
    for (int i = 0; i < depth; i++) printf("  ");

    /* Print the rule */
    printf("%s\n", n->rule);

    /* Print children in order */
    for (int i = 0; i < n->nchildren; i++) {
        print_subtree(n->children[i], depth + 1);
    }
}

void print_tree_reverse(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║     DERIVATION TREE  (indented, top-down structure)          ║\n");
    printf("║  Each rule is shown indented under its parent rule.          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("  Total reductions: %d\n\n", reduction_count);

    /* The root is the last node pushed onto the stack (should be program) */
    if (stack_top > 0) {
        int root = node_stack[stack_top - 1];
        print_subtree(root, 0);
    } else if (tree_size > 0) {
        /* fallback: find node with no parent */
        for (int i = 0; i < tree_size; i++) {
            if (tree_nodes[i].parent == -1) {
                print_subtree(i, 0);
                break;
            }
        }
    }
    printf("\n");
}

/* ═══════════════════════════════════════════════════════════════════
   PART 4 — Parsing Table (matrix format)
   ═══════════════════════════════════════════════════════════════════
   bison/yacc -v writes a human-readable automaton to y.output.
   We parse that file here and re-display it as a compact matrix.

   Columns:  ACTION section (terminals) | GOTO section (non-terminals)
   Cells:    sN  = shift to state N
             rN  = reduce by rule N
             N   = goto state N
             acc = accept
             .   = error (no entry)
   ═══════════════════════════════════════════════════════════════════ */

#define MAX_PT_STATES   600
#define MAX_PT_SYMBOLS  200
#define PT_SYM_LEN       48
#define PT_CELL_LEN      16

typedef struct { char sym[PT_SYM_LEN]; int is_terminal; } PtSymbol;
typedef struct { char action[PT_CELL_LEN]; }              PtCell;

static PtSymbol  pt_symbols[MAX_PT_SYMBOLS];
static int       pt_nsymbols = 0;
static PtCell    pt_table[MAX_PT_STATES][MAX_PT_SYMBOLS];
static int       pt_nstates  = 0;

static int pt_find_or_add(const char *s, int is_term) {
    for (int i = 0; i < pt_nsymbols; i++)
        if (strcmp(pt_symbols[i].sym, s) == 0) return i;
    if (pt_nsymbols >= MAX_PT_SYMBOLS) return -1;
    strncpy(pt_symbols[pt_nsymbols].sym, s, PT_SYM_LEN - 1);
    pt_symbols[pt_nsymbols].sym[PT_SYM_LEN-1] = '\0';
    pt_symbols[pt_nsymbols].is_terminal = is_term;
    return pt_nsymbols++;
}

static void pt_set(int state, const char *sym, int is_term, const char *val) {
    int col = pt_find_or_add(sym, is_term);
    if (col < 0 || state < 0 || state >= MAX_PT_STATES) return;
    /* first-write wins (resolves any residual bison warnings) */
    if (pt_table[state][col].action[0] == '\0')
        strncpy(pt_table[state][col].action, val, PT_CELL_LEN - 1);
}

/* Strip surrounding single-quotes from a token name like 'int' → int */
static void strip_quotes(char *tok) {
    int len = strlen(tok);
    if (len >= 2 && tok[0] == '\'' && tok[len-1] == '\'') {
        tok[len-1] = '\0';
        memmove(tok, tok + 1, len - 1);  /* FIX: was len (off-by-one); now len-1 */
        tok[len - 2] = '\0';
    }
}

void print_parsing_table(const char *youtput_path) {
    FILE *f = fopen(youtput_path, "r");
    if (!f) {
        /* Try the local directory as fallback */
        f = fopen("y.output", "r");
        if (!f) {
            printf("\n  (y.output not found — ensure you built with: bison -d -v cl_parser.y)\n");
            return;
        }
    }

    memset(pt_table, 0, sizeof(pt_table));

    char line_buf[1024];
    int  cur_state = -1;

    while (fgets(line_buf, sizeof(line_buf), f)) {
        /* ── New state heading ── */
        if (strncmp(line_buf, "State ", 6) == 0 ||
            strncmp(line_buf, "state ", 6) == 0) {
            cur_state = atoi(line_buf + 6);
            if (cur_state + 1 > pt_nstates) pt_nstates = cur_state + 1;
            continue;
        }
        if (cur_state < 0) continue;

        char tok[80], dest[32];
        tok[0] = dest[0] = '\0';

        /* ── Shift ── */
        if (strstr(line_buf, "shift, and go to state")) {
            if (sscanf(line_buf, " %79s shift, and go to state %31s", tok, dest) == 2) {
                char cell[PT_CELL_LEN];
                snprintf(cell, sizeof(cell), "s%s", dest);
                strip_quotes(tok);
                pt_set(cur_state, tok, 1, cell);
            }
            continue;
        }

        /* ── Goto (non-terminal transition) ── */
        if (strstr(line_buf, "go to state") && !strstr(line_buf, "shift")) {
            if (sscanf(line_buf, " %79s go to state %31s", tok, dest) == 2) {
                pt_set(cur_state, tok, 0, dest);
            }
            continue;
        }

        /* ── Reduce ── */
        if (strstr(line_buf, "reduce using rule")) {
            char *p = strstr(line_buf, "rule ");
            if (!p) continue;
            int rulenum = atoi(p + 5);
            char cell[PT_CELL_LEN];
            snprintf(cell, sizeof(cell), "r%d", rulenum);

            if (sscanf(line_buf, " %79s", tok) == 1 &&
                strcmp(tok, "$default") != 0 &&
                strcmp(tok, "/*")       != 0) {
                strip_quotes(tok);
                pt_set(cur_state, tok, 1, cell);
            } else {
                /* $default: fill every terminal column not yet set */
                for (int c = 0; c < pt_nsymbols; c++)
                    if (pt_symbols[c].is_terminal &&
                        pt_table[cur_state][c].action[0] == '\0')
                        strncpy(pt_table[cur_state][c].action, cell, PT_CELL_LEN-1);
            }
            continue;
        }

        /* ── Accept ── */
        if (strstr(line_buf, "accept")) {
            pt_set(cur_state, "$end", 1, "acc");
            continue;
        }
    }
    fclose(f);

    pt_find_or_add("$end", 1);   /* always ensure $end column exists */

    /* ── Count terminal / non-terminal columns ── */
    int n_term = 0, n_nonterm = 0;
    for (int c = 0; c < pt_nsymbols; c++) {
        if (pt_symbols[c].is_terminal) n_term++;
        else                            n_nonterm++;
    }

    /* ─────────────────────────────────────────────────────────────────
       DISPLAY STRATEGY (per TA advice):
       • On screen  — show only a few meaningful states:
           State 0  (initial state)
           States where shift/reduce conflicts were detected
           State with the accept action
       • Full table — exported to output/parsing_table.csv
         The CSV has every state × every symbol, suitable for
         spreadsheet viewing (Excel / LibreOffice Calc).
       ───────────────────────────────────────────────────────────── */

    #define MAX_DISPLAY_COLS 200

    /* Build active terminal columns (have at least one entry anywhere) */
    int term_cols[MAX_DISPLAY_COLS];
    int n_term_cols = 0;
    for (int c = 0; c < pt_nsymbols && n_term_cols < MAX_DISPLAY_COLS; c++) {
        if (!pt_symbols[c].is_terminal) continue;
        for (int s = 0; s < pt_nstates; s++) {
            if (pt_table[s][c].action[0] != '\0') {
                term_cols[n_term_cols++] = c;
                break;
            }
        }
    }

    /* Build active non-terminal (GOTO) columns */
    int nt_cols[MAX_DISPLAY_COLS];
    int n_nt_cols = 0;
    for (int c = 0; c < pt_nsymbols && n_nt_cols < MAX_DISPLAY_COLS; c++) {
        if (pt_symbols[c].is_terminal) continue;
        for (int s = 0; s < pt_nstates; s++) {
            if (pt_table[s][c].action[0] != '\0') {
                nt_cols[n_nt_cols++] = c;
                break;
            }
        }
    }

    /* ── 1. Export FULL table to CSV ─────────────────────────────── */
    /* Create the output directory if it does not exist */
    system("mkdir -p output");
    FILE *csv = fopen("output/parsing_table.csv", "w");
    if (csv) {
        /* Header row */
        fprintf(csv, "State");
        for (int i = 0; i < n_term_cols; i++)
            fprintf(csv, ",%s", pt_symbols[term_cols[i]].sym);
        for (int i = 0; i < n_nt_cols; i++)
            fprintf(csv, ",%s", pt_symbols[nt_cols[i]].sym);
        fprintf(csv, "\n");

        /* One row per state */
        for (int s = 0; s < pt_nstates; s++) {
            fprintf(csv, "%d", s);
            for (int i = 0; i < n_term_cols; i++) {
                const char *v = pt_table[s][term_cols[i]].action;
                fprintf(csv, ",%s", v[0] ? v : "");
            }
            for (int i = 0; i < n_nt_cols; i++) {
                const char *v = pt_table[s][nt_cols[i]].action;
                fprintf(csv, ",%s", v[0] ? v : "");
            }
            fprintf(csv, "\n");
        }
        fclose(csv);
        printf("\n  [Part 4] Full LALR(1) table (%d states x %d symbols)\n",
               pt_nstates, n_term_cols + n_nt_cols);
        printf("           saved to: output/parsing_table.csv\n");
        printf("           Open in Excel / LibreOffice Calc for full view.\n");
    } else {
        printf("\n  [Warning] Could not write output/parsing_table.csv\n");
    }

    /* ── 2. Identify which states to show on screen ──────────────── */
    /*  We show:
        (a) State 0        — the initial/start state
        (b) Conflict states — states that appear in y.output conflict lines
        (c) Accept state   — state with "acc" entry for $end
        (d) A few early non-trivial states for illustration            */

    int show_states[32];
    int n_show = 0;
    #define ADD_SHOW(s) do {         int _s = (s);         int _dup = 0;         for (int _k = 0; _k < n_show; _k++) if (show_states[_k] == _s) { _dup=1; break; }         if (!_dup && n_show < 32) show_states[n_show++] = _s;     } while(0)

    ADD_SHOW(0);   /* initial state always shown */

    /* Find conflict states from y.output */
    {
        FILE *yf = fopen(youtput_path, "r");
        if (!yf) yf = fopen("y.output", "r");
        if (yf) {
            char buf[256];
            while (fgets(buf, sizeof(buf), yf)) {
                int st = -1;
                if (sscanf(buf, "State %d conflicts:", &st) == 1 && st >= 0)
                    ADD_SHOW(st);
            }
            fclose(yf);
        }
    }

    /* Find accept state (has "acc" for $end) */
    {
        int end_col = -1;
        for (int c = 0; c < pt_nsymbols; c++)
            if (strcmp(pt_symbols[c].sym, "$end") == 0) { end_col = c; break; }
        if (end_col >= 0) {
            for (int s = 0; s < pt_nstates; s++) {
                if (strcmp(pt_table[s][end_col].action, "acc") == 0) {
                    ADD_SHOW(s);
                    break;
                }
            }
        }
    }

    /* Add a few early non-trivial states (states 1-5 that have entries) */
    {
        int added = 0;
        for (int s = 1; s < pt_nstates && added < 4; s++) {
            int has = 0;
            for (int i = 0; i < n_term_cols && !has; i++)
                if (pt_table[s][term_cols[i]].action[0]) has = 1;
            if (has) { ADD_SHOW(s); added++; }
        }
    }

    /* Sort show_states ascending */
    for (int i = 0; i < n_show-1; i++)
        for (int j = i+1; j < n_show; j++)
            if (show_states[i] > show_states[j]) {
                int tmp = show_states[i];
                show_states[i] = show_states[j];
                show_states[j] = tmp;
            }

    /* ── 3. Print selected states on screen ──────────────────────── */
    const int CW = 7;
    const int SW = 5;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║      LALR(1) PARSING TABLE  (selected states shown)          ║\n");
    printf("║  Showing: State 0, conflict states, accept state + samples   ║\n");
    printf("║  Full table → output/parsing_table.csv                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("  Legend:  sN=shift  rN=reduce  N=goto  acc=accept  .=error\n\n");

    /* Header */
    printf("%*s |", SW, "State");
    printf(" %-*s|", n_term_cols * CW - 1, " ACTION");
    printf(" GOTO\n");

    printf("%*s |", SW, "");
    for (int i = 0; i < n_term_cols; i++)
        printf(" %-*.*s", CW-1, CW-1, pt_symbols[term_cols[i]].sym);
    printf("|");
    for (int i = 0; i < n_nt_cols; i++)
        printf(" %-*.*s", CW-1, CW-1, pt_symbols[nt_cols[i]].sym);
    printf("\n");

    printf("%*s-+", SW, "-----");
    for (int i = 0; i < n_term_cols + n_nt_cols; i++) printf("-------+");
    printf("\n");

    for (int si = 0; si < n_show; si++) {
        int s = show_states[si];
        if (s < 0 || s >= pt_nstates) continue;

        /* Label conflict states clearly */
        int is_conflict = 0;
        for (int k = 2; k < n_show && k <= 4; k++)  /* crude: conflict states added 2nd */
            if (show_states[k] == s) { is_conflict = 1; break; }

        printf("%*d |", SW, s);
        for (int i = 0; i < n_term_cols; i++) {
            const char *v = pt_table[s][term_cols[i]].action;
            printf(" %-*.*s", CW-1, CW-1, v[0] ? v : ".");
        }
        printf("|");
        for (int i = 0; i < n_nt_cols; i++) {
            const char *v = pt_table[s][nt_cols[i]].action;
            printf(" %-*.*s", CW-1, CW-1, v[0] ? v : ".");
        }
        printf("\n");
    }

    printf("\n  Showing %d / %d states on screen.\n", n_show, pt_nstates);
    printf("  See output/parsing_table.csv for all %d states.\n\n", pt_nstates);
}

/* ═══════════════════════════════════════════════════════════════════
   PART 5 — Error Diagnostics
   ═══════════════════════════════════════════════════════════════════
   tok_name_safe() maps a Bison token number to a human-readable name.

   WHY a local table instead of extern yytname[]:
     yytname[] is only exported by bison when YYDEBUG is enabled or
     %define api.token.raw is used.  With plain -d (our build flags),
     yytname is NOT exported — the linker would throw:
       "undefined reference to `yytname'"
     So we keep a self-contained table here instead.

   HOW to keep it in sync:
     Run:  bison -d -v cl_parser.y
     Open y.tab.h and verify the #define values match the entries
     below (they start at 258 and follow %token declaration order).
   ═══════════════════════════════════════════════════════════════════ */

/* Self-contained token-name table.
   Entry index = (token_number - 258).
   Single-character tokens (< 256) are handled separately below.    */
static const char *const tok_names[] = {
    /* 258 */ "int",
    /* 259 */ "float",
    /* 260 */ "char",
    /* 261 */ "void",
    /* 262 */ "double",
    /* 263 */ "short",
    /* 264 */ "long",
    /* 265 */ "unsigned",
    /* 266 */ "signed",
    /* 267 */ "if",
    /* 268 */ "else",
    /* 269 */ "for",
    /* 270 */ "while",
    /* 271 */ "do",
    /* 272 */ "switch",
    /* 273 */ "case",
    /* 274 */ "default",
    /* 275 */ "break",
    /* 276 */ "continue",
    /* 277 */ "return",
    /* 278 */ "struct",
    /* 279 */ "typedef",
    /* 280 */ "enum",
    /* 281 */ "union",
    /* 282 */ "sizeof",
    /* 283 */ "auto",
    /* 284 */ "const",
    /* 285 */ "volatile",
    /* 286 */ "static",
    /* 287 */ "extern",
    /* 288 */ "register",
    /* 289 */ "inline",
    /* 290 */ "IDENTIFIER",
    /* 291 */ "INT_CONST",
    /* 292 */ "FLOAT_CONST",
    /* 293 */ "CHAR_CONST",
    /* 294 */ "STRING_LITERAL",
    /* 295 */ "ASSIGN_OP (+=/-=/*=/etc.)",
    /* 296 */ "... (ellipsis)",
    /* 297 */ "-> (arrow)",
    /* 298 */ "++ (inc)",
    /* 299 */ "-- (dec)",
    /* 300 */ "<< (left-shift)",
    /* 301 */ ">> (right-shift)",
    /* 302 */ "<= (le)",
    /* 303 */ ">= (ge)",
    /* 304 */ "== (eq)",
    /* 305 */ "!= (ne)",
    /* 306 */ "&& (logical-and)",
    /* 307 */ "|| (logical-or)",
    /* 308 */ "UMINUS (precedence marker)",
    /* 309 */ "UPLUS  (precedence marker)",
    /* 310 */ "LOWER_THAN_ELSE (precedence marker)",
    /* NOTE: token 310 is the last real token.  ELSE = 268 (already listed above).
       The old entry "311 → else (ELSE nonassoc)" was wrong — token 311 does not
       exist.  Removed to prevent tok_name_safe() returning stale names. */
};
static const int tok_names_count =
    (int)(sizeof(tok_names) / sizeof(tok_names[0]));

static const char *tok_name_safe(int tok) {
    if (tok == 0)    return "end-of-file";
    if (tok == 256)  return "<error-token>";
    if (tok < 256) {
        /* single-character token: print as quoted char */
        static char buf[8];
        if (tok >= 32 && tok < 127)
            snprintf(buf, sizeof(buf), "'%c'", (char)tok);
        else
            snprintf(buf, sizeof(buf), "0x%02x", tok);
        return buf;
    }
    int idx = tok - 258;
    if (idx >= 0 && idx < tok_names_count)
        return tok_names[idx];
    return "unknown-token";
}

void yyerror(const char *s) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╔══════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║                       PARSE ERROR                            ║\n");
    fprintf(stderr, "╚══════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, "  Line         : %d\n", yylineno);
    fprintf(stderr, "  Near token   : '%s'\n",
            last_token[0] ? last_token : "(unknown)");
    fprintf(stderr, "  Lookahead    : %s (token id %d)\n",
            tok_name_safe(yychar), yychar);
    fprintf(stderr, "  Message      : %s\n", s);
    fprintf(stderr, "\n");
    fprintf(stderr, "  Hints:\n");
    fprintf(stderr, "    - Missing ';' at end of statement?\n");
    fprintf(stderr, "    - Unmatched '{', '}', '(', or ')'?\n");
    fprintf(stderr, "    - Invalid expression near line %d?\n", yylineno);
    fprintf(stderr, "    - Declaration after statement (C89 mode)?\n");
    fprintf(stderr, "      (Move all declarations to the top of the block.)\n");
    fprintf(stderr, "\n");
    exit(1);
}


#line 781 "y.tab.c"

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
#line 714 "cl_parser.y"

    char  *str_val;   /* literal text for terminals */
    int    node_idx;  /* tree node index for non-terminals */

#line 949 "y.tab.c"

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
  YYSYMBOL_75_ = 75,                       /* ';'  */
  YYSYMBOL_76_ = 76,                       /* '['  */
  YYSYMBOL_77_ = 77,                       /* ']'  */
  YYSYMBOL_78_ = 78,                       /* '{'  */
  YYSYMBOL_79_ = 79,                       /* '}'  */
  YYSYMBOL_YYACCEPT = 80,                  /* $accept  */
  YYSYMBOL_program = 81,                   /* program  */
  YYSYMBOL_translation_unit = 82,          /* translation_unit  */
  YYSYMBOL_external_decl = 83,             /* external_decl  */
  YYSYMBOL_function_def = 84,              /* function_def  */
  YYSYMBOL_function_header = 85,           /* function_header  */
  YYSYMBOL_param_list = 86,                /* param_list  */
  YYSYMBOL_param_decl = 87,                /* param_decl  */
  YYSYMBOL_declaration = 88,               /* declaration  */
  YYSYMBOL_declarator_list = 89,           /* declarator_list  */
  YYSYMBOL_declarator = 90,                /* declarator  */
  YYSYMBOL_initializer = 91,               /* initializer  */
  YYSYMBOL_initializer_list = 92,          /* initializer_list  */
  YYSYMBOL_type_qualifier = 93,            /* type_qualifier  */
  YYSYMBOL_type_spec = 94,                 /* type_spec  */
  YYSYMBOL_storage_class_spec = 95,        /* storage_class_spec  */
  YYSYMBOL_storage_class_list = 96,        /* storage_class_list  */
  YYSYMBOL_struct_decl = 97,               /* struct_decl  */
  YYSYMBOL_member_decl_list = 98,          /* member_decl_list  */
  YYSYMBOL_member_decl = 99,               /* member_decl  */
  YYSYMBOL_enum_decl = 100,                /* enum_decl  */
  YYSYMBOL_enumerator_list = 101,          /* enumerator_list  */
  YYSYMBOL_enumerator = 102,               /* enumerator  */
  YYSYMBOL_compound_stmt = 103,            /* compound_stmt  */
  YYSYMBOL_block_item_list = 104,          /* block_item_list  */
  YYSYMBOL_block_item = 105,               /* block_item  */
  YYSYMBOL_stmt = 106,                     /* stmt  */
  YYSYMBOL_expr_stmt = 107,                /* expr_stmt  */
  YYSYMBOL_selection_stmt = 108,           /* selection_stmt  */
  YYSYMBOL_non_case_stmt = 109,            /* non_case_stmt  */
  YYSYMBOL_case_list = 110,                /* case_list  */
  YYSYMBOL_case_body_list = 111,           /* case_body_list  */
  YYSYMBOL_labeled_stmt = 112,             /* labeled_stmt  */
  YYSYMBOL_iteration_stmt = 113,           /* iteration_stmt  */
  YYSYMBOL_jump_stmt = 114,                /* jump_stmt  */
  YYSYMBOL_expr = 115,                     /* expr  */
  YYSYMBOL_assignment_expr = 116,          /* assignment_expr  */
  YYSYMBOL_conditional_expr = 117,         /* conditional_expr  */
  YYSYMBOL_logical_or_expr = 118,          /* logical_or_expr  */
  YYSYMBOL_logical_and_expr = 119,         /* logical_and_expr  */
  YYSYMBOL_inclusive_or_expr = 120,        /* inclusive_or_expr  */
  YYSYMBOL_exclusive_or_expr = 121,        /* exclusive_or_expr  */
  YYSYMBOL_and_expr = 122,                 /* and_expr  */
  YYSYMBOL_equality_expr = 123,            /* equality_expr  */
  YYSYMBOL_relational_expr = 124,          /* relational_expr  */
  YYSYMBOL_shift_expr = 125,               /* shift_expr  */
  YYSYMBOL_additive_expr = 126,            /* additive_expr  */
  YYSYMBOL_multiplicative_expr = 127,      /* multiplicative_expr  */
  YYSYMBOL_unary_expr = 128,               /* unary_expr  */
  YYSYMBOL_postfix_expr = 129,             /* postfix_expr  */
  YYSYMBOL_primary_expr = 130,             /* primary_expr  */
  YYSYMBOL_arg_list = 131                  /* arg_list  */
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
#define YYFINAL  48
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1258

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  80
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  52
/* YYNRULES -- Number of rules.  */
#define YYNRULES  185
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  334

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
       2,     2,     2,     2,     2,     2,     2,     2,    55,    75,
      59,    53,    60,    54,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    76,     2,    77,    57,     2,     2,     2,     2,     2,
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
       0,   962,   962,   967,   969,   974,   976,   983,   988,   990,
     993,   995,  1000,  1002,  1004,  1006,  1011,  1013,  1015,  1022,
    1024,  1026,  1028,  1036,  1043,  1045,  1050,  1052,  1054,  1056,
    1058,  1060,  1062,  1064,  1069,  1071,  1073,  1078,  1080,  1100,
    1101,  1105,  1106,  1107,  1108,  1109,  1110,  1111,  1112,  1113,
    1114,  1116,  1117,  1118,  1119,  1120,  1121,  1122,  1123,  1124,
    1125,  1126,  1130,  1131,  1132,  1133,  1134,  1146,  1148,  1155,
    1157,  1159,  1161,  1163,  1165,  1170,  1172,  1177,  1183,  1185,
    1187,  1192,  1194,  1199,  1201,  1227,  1229,  1234,  1236,  1241,
    1243,  1250,  1251,  1252,  1253,  1254,  1255,  1259,  1261,  1283,
    1285,  1287,  1317,  1318,  1319,  1320,  1321,  1326,  1327,  1335,
    1336,  1341,  1343,  1345,  1379,  1381,  1384,  1386,  1389,  1391,
    1396,  1398,  1400,  1402,  1409,  1411,  1416,  1418,  1420,  1425,
    1427,  1432,  1434,  1439,  1441,  1446,  1448,  1453,  1455,  1460,
    1462,  1467,  1469,  1471,  1476,  1478,  1480,  1482,  1484,  1489,
    1491,  1493,  1498,  1500,  1502,  1507,  1509,  1511,  1513,  1523,
    1525,  1527,  1529,  1531,  1533,  1535,  1537,  1539,  1541,  1543,
    1563,  1565,  1567,  1569,  1571,  1573,  1575,  1577,  1582,  1584,
    1586,  1588,  1590,  1592,  1597,  1599
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
  "LOWER_THAN_ELSE", "'('", "')'", "','", "';'", "'['", "']'", "'{'",
  "'}'", "$accept", "program", "translation_unit", "external_decl",
  "function_def", "function_header", "param_list", "param_decl",
  "declaration", "declarator_list", "declarator", "initializer",
  "initializer_list", "type_qualifier", "type_spec", "storage_class_spec",
  "storage_class_list", "struct_decl", "member_decl_list", "member_decl",
  "enum_decl", "enumerator_list", "enumerator", "compound_stmt",
  "block_item_list", "block_item", "stmt", "expr_stmt", "selection_stmt",
  "non_case_stmt", "case_list", "case_body_list", "labeled_stmt",
  "iteration_stmt", "jump_stmt", "expr", "assignment_expr",
  "conditional_expr", "logical_or_expr", "logical_and_expr",
  "inclusive_or_expr", "exclusive_or_expr", "and_expr", "equality_expr",
  "relational_expr", "shift_expr", "additive_expr", "multiplicative_expr",
  "unary_expr", "postfix_expr", "primary_expr", "arg_list", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-227)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1164,  -227,  -227,  -227,  -227,  -227,    67,   124,   177,    79,
      10,  1228,    16,    17,  -227,  -227,  -227,  -227,  -227,  -227,
    -227,    19,  1164,  -227,  -227,    24,  -227,  1228,    62,  -227,
    1196,  -227,  -227,  -227,  -227,  -227,   111,  -227,  -227,  -227,
    -227,    48,  1228,    85,    54,   107,    71,  1228,  -227,  -227,
     290,  -227,  -227,    68,   117,    97,  -227,    64,  -227,    78,
    -227,  -227,  1228,    85,    33,  -227,    47,   126,   107,   128,
     -46,  -227,  1228,   260,   113,   147,   155,   744,   173,  1017,
     152,   159,   172,   866,  1055,   196,  -227,  -227,  -227,  -227,
    1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,  -227,
    -227,  -227,    83,  1196,  -227,   367,  -227,  -227,  -227,  -227,
    -227,  -227,  -227,   135,  -227,  -227,    70,   202,   201,   227,
     203,   167,    69,   191,   188,   129,   -22,   182,  -227,   853,
     554,    29,   222,   297,    85,  -227,   130,  -227,   181,   444,
     185,  -227,  -227,  -227,   110,   318,   107,  -227,   453,  -227,
    1017,   481,  1017,   342,  1017,  -227,   -40,  -227,  -227,  -227,
    -227,   199,   685,  -227,   744,  -227,  -227,  -227,  -227,  -227,
    -227,  -227,  -227,   204,    84,  -227,  -227,  1017,  -227,  1017,
    1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,
    1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,  1017,
     325,  -227,  -227,   326,   952,  1017,   853,  -227,  -227,  -227,
    -227,   206,  -227,    85,  1196,   286,  -227,   853,   291,  -227,
     600,  -227,  -227,  -227,  -227,  -227,  -227,  -227,   208,   909,
     909,   214,   294,   218,  -227,   809,  -227,   307,  -227,  -227,
    -227,   202,  -227,    -7,   201,   227,   203,   167,    69,    69,
     191,   191,   191,   191,   188,   188,   129,   129,  -227,  -227,
    -227,  -227,  -227,  -227,  -227,  -227,  -227,   257,   -28,  -227,
     156,  -227,  1125,  -227,    85,  -227,  -227,   295,  -227,   262,
     744,   965,  1004,   744,  1017,   329,   809,  -227,  -227,  -227,
    -227,  -227,  -227,  -227,  1017,  -227,  1017,  -227,   160,  -227,
    -227,  -227,  -227,   646,  -227,   395,   744,   264,   744,   267,
    -227,   269,  -227,  -227,  -227,  -227,  -227,  -227,   271,   744,
    -227,   744,  -227,   744,   334,    36,  -227,  -227,  -227,  -227,
    -227,   196,  -227,  -227
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,    41,    42,    43,    44,    45,    46,    47,    48,    49,
       0,     0,     0,     0,    62,    39,    40,    64,    65,    63,
      66,     0,     2,     3,     5,     0,     6,     0,     0,    67,
       0,    60,    61,    51,    52,    53,    58,    54,    56,    55,
      57,    71,     0,     0,    80,     0,    74,     0,     1,     4,
       0,     7,    50,    26,     0,     0,    21,     0,    24,     0,
      68,    59,     0,     0,     0,    75,    26,     0,     0,    83,
       0,    81,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   178,   179,   180,   181,   182,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    97,
      85,    89,     0,     0,    91,     0,    87,    90,    92,    93,
      96,    94,    95,     0,   124,   126,   129,   131,   133,   135,
     137,   139,   141,   144,   149,   152,   155,   159,   170,     0,
       0,     0,    27,     0,     0,    19,    26,    22,     0,     0,
       0,    70,    76,    23,     0,     0,     0,    79,     0,    73,
       0,     0,     0,     0,     0,   178,     0,   109,   122,   123,
     120,     0,     0,   168,     0,   160,   161,   167,   163,   162,
     166,   164,   165,     0,     0,    86,    88,     0,    98,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   176,   177,     0,     0,     0,     0,    30,    34,    14,
       9,     0,    12,    17,     0,     0,    29,     0,     0,    25,
       0,    20,    69,    77,    78,    84,    82,    72,     0,     0,
       0,     0,     0,     0,   109,   112,   121,     0,   113,   183,
     125,   132,   155,     0,   134,   136,   138,   140,   142,   143,
     147,   148,   145,   146,   150,   151,   153,   154,   156,   157,
     158,   128,   127,   175,   174,   172,   184,     0,     0,    37,
       0,     8,     0,    16,     0,    28,    31,     0,    11,     0,
       0,     0,     0,     0,     0,     0,   111,   102,   103,   104,
     110,   105,   106,   169,     0,   173,     0,   171,     0,    35,
      15,    13,    18,     0,    10,    99,     0,     0,     0,     0,
     114,     0,   107,   130,   185,    36,    38,    33,     0,     0,
     118,     0,   116,     0,     0,     0,    32,   100,   119,   117,
     115,     0,   101,   108
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -227,  -227,  -227,   390,  -227,  -227,  -204,   141,   -44,   -39,
    -129,  -194,  -227,  -227,     0,   -23,     3,  -227,   -15,   -38,
    -227,   346,   270,   -24,  -227,   310,   -75,  -143,  -226,  -227,
    -227,   183,    93,  -222,  -218,   -69,  -108,   125,  -227,   241,
     240,   242,   239,   243,   161,    53,   158,   162,   -18,  -227,
    -227,  -227
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    21,    22,    23,    24,    25,   211,   212,    26,    57,
      58,   207,   270,    27,    63,    29,   214,    31,    64,    65,
      32,    70,    71,   104,   105,   106,   107,   108,   109,   290,
     325,   235,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   267
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      28,    51,   153,    30,    67,   219,   101,    60,   230,   289,
     156,    43,   269,   291,   161,   234,   279,   292,   198,    48,
     138,   208,    28,   276,   140,    30,   142,    52,   146,   173,
      59,   199,    73,   147,   177,   142,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    41,   177,   139,   294,   297,
     102,    44,    46,   103,    79,    80,    10,   148,    12,    13,
     289,   101,    15,    16,   291,   215,   163,   177,   292,   240,
      33,   331,   165,   166,   167,   168,   169,   170,   171,   172,
      60,   228,    40,   231,   273,   233,   281,   282,    42,   238,
     261,   262,   288,   173,    45,    47,   266,    53,   208,   318,
     129,   142,    50,   174,   316,   102,   216,   229,   103,   208,
     142,   243,   141,   136,    61,   332,   187,   188,    66,    66,
      66,   129,   179,   131,   180,    54,    62,    34,   189,   190,
     213,    35,    68,    36,    55,   138,   268,    56,   134,   135,
     130,    54,    69,   288,   131,   302,    54,    54,    54,    72,
      55,   102,   132,   137,   103,    55,    55,    55,    56,   137,
     133,   242,   237,   242,   242,   242,   242,   242,   242,   242,
     242,   242,   242,   242,   242,   242,   242,   258,   259,   260,
      37,   145,    38,   129,   146,   150,    39,    84,   314,   224,
     208,    60,   195,   196,   197,   155,    86,    87,    88,    89,
     134,   143,   220,    90,    91,   305,   131,   157,   310,   177,
     178,   287,   307,   309,   274,   311,   185,   186,    92,   151,
     213,    93,    94,    95,   200,   201,   202,   152,    96,    97,
     298,   320,    98,   322,   158,   299,   191,   192,   206,   315,
     250,   251,   252,   253,   327,   154,   328,   159,   329,   193,
     194,   164,   203,   181,   204,   134,   221,   182,   205,   134,
     223,   184,   287,     1,     2,     3,     4,     5,     6,     7,
       8,     9,   213,   177,   236,   217,   242,   239,   177,   271,
     272,   280,   177,    10,   183,    12,    13,   283,   177,    15,
      16,   285,   177,     1,     2,     3,     4,     5,     6,     7,
       8,     9,    74,   213,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    10,    11,    12,    13,    84,    14,    15,
      16,    17,    18,    19,    20,    85,    86,    87,    88,    89,
     295,   296,   218,    90,    91,   304,   272,   321,   177,   149,
     323,   177,   324,   177,   326,   272,   248,   249,    92,   254,
     255,    93,    94,    95,   225,   256,   257,   232,    96,    97,
     263,   264,    98,   275,   277,    99,   284,   303,    50,   100,
       1,     2,     3,     4,     5,     6,     7,     8,     9,    74,
     293,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      10,    11,    12,    13,    84,    14,    15,    16,    17,    18,
      19,    20,    85,    86,    87,    88,    89,   312,   319,   330,
      90,    91,    49,   301,   144,   176,   226,   286,   333,   313,
     241,   244,   246,     0,   245,    92,     0,   247,    93,    94,
      95,     0,     0,     0,     0,    96,    97,     0,     0,    98,
       0,     0,    99,     0,     0,    50,   175,     1,     2,     3,
       4,     5,     6,     7,     8,     9,     1,     2,     3,     4,
       5,     6,     7,     8,     9,     0,     0,    10,     0,    12,
      13,     0,     0,    15,    16,     0,    10,     0,    12,    13,
       0,     0,    15,    16,     1,     2,     3,     4,     5,     6,
       7,     8,     9,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    10,    11,    12,    13,    84,    14,
      15,    16,    17,    18,    19,    20,   155,    86,    87,    88,
      89,     0,     0,   222,    90,    91,     0,     0,     0,     0,
       0,     0,   227,     0,     0,     0,     0,     0,     0,    92,
       0,     0,    93,    94,    95,     0,     0,     0,     0,    96,
      97,     0,     0,    98,     0,     0,    99,     1,     2,     3,
       4,     5,     6,     7,     8,     9,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    10,     0,    12,
      13,     0,    14,    15,    16,    17,    18,    19,    20,     0,
       0,     0,     0,     0,     0,   209,     0,     0,     0,     0,
       0,     0,     0,     1,     2,     3,     4,     5,     6,     7,
       8,     9,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    10,     0,    12,    13,   210,    14,    15,
      16,    17,    18,    19,    20,     0,     0,     0,     0,     0,
       0,   209,     0,     0,     0,     0,     0,     0,     0,     1,
       2,     3,     4,     5,     6,     7,     8,     9,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    10,
       0,    12,    13,   278,    14,    15,    16,    17,    18,    19,
      20,     0,     0,     0,     0,     0,     0,   209,     1,     2,
       3,     4,     5,     6,     7,     8,     9,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    10,     0,
      12,    13,    84,     0,    15,    16,     0,     0,     0,   317,
     155,    86,    87,    88,    89,     0,     0,     0,    90,    91,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    92,     0,     0,    93,    94,    95,     0,
       0,     0,     0,    96,    97,     0,    74,    98,    75,    76,
      77,    78,    79,    80,    81,    82,    83,     0,     0,     0,
       0,    84,     0,     0,     0,     0,     0,     0,     0,    85,
      86,    87,    88,    89,     0,     0,     0,    90,    91,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    92,     0,     0,    93,    94,    95,     0,     0,
       0,     0,    96,    97,     0,     0,    98,     0,     0,    99,
       0,    74,    50,    75,    76,    77,    78,     0,     0,    81,
      82,    83,     0,     0,     0,     0,    84,     0,     0,     0,
       0,     0,     0,     0,   155,    86,    87,    88,    89,     0,
       0,     0,    90,    91,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    92,     0,     0,
      93,    94,    95,     0,     0,     0,     0,    96,    97,     0,
      84,    98,     0,     0,    99,     0,     0,    50,   155,    86,
      87,    88,    89,    84,     0,     0,    90,    91,     0,     0,
       0,   155,    86,    87,    88,    89,     0,     0,     0,    90,
      91,    92,     0,     0,    93,    94,    95,     0,     0,     0,
       0,    96,    97,     0,    92,    98,     0,    93,    94,    95,
       0,   206,     0,     0,    96,    97,    84,     0,    98,     0,
       0,   160,     0,     0,   155,    86,    87,    88,    89,     0,
       0,     0,    90,    91,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    92,     0,     0,
      93,    94,    95,     0,     0,     0,     0,    96,    97,    84,
       0,    98,     0,     0,    99,     0,     0,   155,    86,    87,
      88,    89,    84,     0,     0,    90,    91,     0,     0,     0,
     155,    86,    87,    88,    89,     0,     0,     0,    90,    91,
      92,     0,     0,    93,    94,    95,     0,     0,     0,     0,
      96,    97,     0,    92,    98,   265,    93,    94,    95,     0,
       0,    84,     0,    96,    97,     0,     0,    98,   306,   155,
      86,    87,    88,    89,    84,     0,     0,    90,    91,     0,
       0,     0,   155,    86,    87,    88,    89,     0,     0,     0,
      90,    91,    92,     0,     0,    93,    94,    95,     0,     0,
       0,     0,    96,    97,     0,    92,    98,   308,    93,    94,
      95,     0,    84,     0,     0,    96,    97,     0,     0,    98,
     155,    86,    87,    88,    89,     0,     0,     0,    90,    91,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    92,     0,     0,    93,    94,    95,     0,
       0,     0,     0,    96,    97,     0,     0,   162,     1,     2,
       3,     4,     5,     6,     7,     8,     9,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    10,     0,
      12,    13,     0,    14,    15,    16,    17,    18,    19,    20,
       0,     0,     0,     0,     0,     0,   300,     1,     2,     3,
       4,     5,     6,     7,     8,     9,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    10,    11,    12,
      13,     0,    14,    15,    16,    17,    18,    19,    20,     1,
       2,     3,     4,     5,     6,     7,     8,     9,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    10,
       0,    12,    13,     0,    14,    15,    16,    17,    18,    19,
      20,     1,     2,     3,     4,     5,     6,     7,     8,     9,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    10,     0,    12,    13,     0,     0,    15,    16
};

static const yytype_int16 yycheck[] =
{
       0,    25,    77,     0,    43,   134,    50,    30,   151,   235,
      79,    11,   206,   235,    83,    55,   220,   235,    40,     0,
      59,   129,    22,   217,    63,    22,    64,    27,    74,    98,
      30,    53,    47,    79,    74,    73,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    35,    74,    62,    55,    77,
      50,    35,    35,    50,    18,    19,    23,    72,    25,    26,
     286,   105,    29,    30,   286,    36,    84,    74,   286,   177,
       3,    35,    90,    91,    92,    93,    94,    95,    96,    97,
     103,   150,     3,   152,   213,   154,   229,   230,    78,   164,
     198,   199,   235,   162,    78,    78,   204,    35,   206,   303,
      53,   139,    78,   103,   298,   105,    77,   151,   105,   217,
     148,   180,    79,    35,     3,    79,    47,    48,    35,    35,
      35,    53,    52,    76,    54,    63,    78,     3,    59,    60,
     130,     7,    78,     9,    72,   174,   205,    75,    74,    75,
      72,    63,    35,   286,    76,   274,    63,    63,    63,    78,
      72,   151,    35,    75,   151,    72,    72,    72,    75,    75,
      63,   179,   162,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
       3,    53,     5,    53,    74,    72,     9,    27,   296,    79,
     298,   214,    63,    64,    65,    35,    36,    37,    38,    39,
      74,    75,    72,    43,    44,   280,    76,    55,   283,    74,
      75,   235,   281,   282,   214,   284,    49,    50,    58,    72,
     220,    61,    62,    63,    42,    43,    44,    72,    68,    69,
      74,   306,    72,   308,    75,    79,    45,    46,    78,    79,
     187,   188,   189,   190,   319,    72,   321,    75,   323,    61,
      62,    55,    70,    51,    72,    74,    75,    56,    76,    74,
      75,    58,   286,     3,     4,     5,     6,     7,     8,     9,
      10,    11,   272,    74,    75,    53,   294,    73,    74,    73,
      74,    73,    74,    23,    57,    25,    26,    73,    74,    29,
      30,    73,    74,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,   303,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      73,    74,    35,    43,    44,    73,    74,    73,    74,    79,
      73,    74,    73,    74,    73,    74,   185,   186,    58,   191,
     192,    61,    62,    63,    36,   193,   194,    15,    68,    69,
      35,    35,    72,    77,    73,    75,    72,    72,    78,    79,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      73,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    78,    13,    75,
      43,    44,    22,   272,    68,   105,   146,   234,   325,   294,
     179,   181,   183,    -1,   182,    58,    -1,   184,    61,    62,
      63,    -1,    -1,    -1,    -1,    68,    69,    -1,    -1,    72,
      -1,    -1,    75,    -1,    -1,    78,    79,     3,     4,     5,
       6,     7,     8,     9,    10,    11,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    -1,    -1,    23,    -1,    25,
      26,    -1,    -1,    29,    30,    -1,    23,    -1,    25,    26,
      -1,    -1,    29,    30,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    -1,    -1,    79,    43,    44,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    58,
      -1,    -1,    61,    62,    63,    -1,    -1,    -1,    -1,    68,
      69,    -1,    -1,    72,    -1,    -1,    75,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    23,    -1,    25,    26,    73,    28,    29,
      30,    31,    32,    33,    34,    -1,    -1,    -1,    -1,    -1,
      -1,    41,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      -1,    25,    26,    73,    28,    29,    30,    31,    32,    33,
      34,    -1,    -1,    -1,    -1,    -1,    -1,    41,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,
      25,    26,    27,    -1,    29,    30,    -1,    -1,    -1,    73,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    -1,    -1,    61,    62,    63,    -1,
      -1,    -1,    -1,    68,    69,    -1,    12,    72,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    -1,    -1,
      -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    43,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    -1,    -1,    61,    62,    63,    -1,    -1,
      -1,    -1,    68,    69,    -1,    -1,    72,    -1,    -1,    75,
      -1,    12,    78,    14,    15,    16,    17,    -1,    -1,    20,
      21,    22,    -1,    -1,    -1,    -1,    27,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,    -1,
      61,    62,    63,    -1,    -1,    -1,    -1,    68,    69,    -1,
      27,    72,    -1,    -1,    75,    -1,    -1,    78,    35,    36,
      37,    38,    39,    27,    -1,    -1,    43,    44,    -1,    -1,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    43,
      44,    58,    -1,    -1,    61,    62,    63,    -1,    -1,    -1,
      -1,    68,    69,    -1,    58,    72,    -1,    61,    62,    63,
      -1,    78,    -1,    -1,    68,    69,    27,    -1,    72,    -1,
      -1,    75,    -1,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    43,    44,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,    -1,
      61,    62,    63,    -1,    -1,    -1,    -1,    68,    69,    27,
      -1,    72,    -1,    -1,    75,    -1,    -1,    35,    36,    37,
      38,    39,    27,    -1,    -1,    43,    44,    -1,    -1,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    43,    44,
      58,    -1,    -1,    61,    62,    63,    -1,    -1,    -1,    -1,
      68,    69,    -1,    58,    72,    73,    61,    62,    63,    -1,
      -1,    27,    -1,    68,    69,    -1,    -1,    72,    73,    35,
      36,    37,    38,    39,    27,    -1,    -1,    43,    44,    -1,
      -1,    -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      43,    44,    58,    -1,    -1,    61,    62,    63,    -1,    -1,
      -1,    -1,    68,    69,    -1,    58,    72,    73,    61,    62,
      63,    -1,    27,    -1,    -1,    68,    69,    -1,    -1,    72,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    58,    -1,    -1,    61,    62,    63,    -1,
      -1,    -1,    -1,    68,    69,    -1,    -1,    72,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,
      25,    26,    -1,    28,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    -1,    -1,    41,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,    24,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    23,
      -1,    25,    26,    -1,    28,    29,    30,    31,    32,    33,
      34,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    23,    -1,    25,    26,    -1,    -1,    29,    30
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      23,    24,    25,    26,    28,    29,    30,    31,    32,    33,
      34,    81,    82,    83,    84,    85,    88,    93,    94,    95,
      96,    97,   100,     3,     3,     7,     9,     3,     5,     9,
       3,    35,    78,    94,    35,    78,    35,    78,     0,    83,
      78,   103,    94,    35,    63,    72,    75,    89,    90,    94,
      95,     3,    78,    94,    98,    99,    35,    89,    78,    35,
     101,   102,    78,    98,    12,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    27,    35,    36,    37,    38,    39,
      43,    44,    58,    61,    62,    63,    68,    69,    72,    75,
      79,    88,    94,    96,   103,   104,   105,   106,   107,   108,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,    53,
      72,    76,    35,    63,    74,    75,    35,    75,    89,    98,
      89,    79,    99,    75,   101,    53,    74,    79,    98,    79,
      72,    72,    72,   106,    72,    35,   115,    55,    75,    75,
      75,   115,    72,   128,    55,   128,   128,   128,   128,   128,
     128,   128,   128,   115,    94,    79,   105,    74,    75,    52,
      54,    51,    56,    57,    58,    49,    50,    47,    48,    59,
      60,    45,    46,    61,    62,    63,    64,    65,    40,    53,
      42,    43,    44,    70,    72,    76,    78,    91,   116,    41,
      73,    86,    87,    94,    96,    36,    77,    53,    35,    90,
      72,    75,    79,    75,    79,    36,   102,    79,   115,    88,
     107,   115,    15,   115,    55,   111,    75,    94,   106,    73,
     116,   119,   128,   115,   120,   121,   122,   123,   124,   124,
     125,   125,   125,   125,   126,   126,   127,   127,   128,   128,
     128,   116,   116,    35,    35,    73,   116,   131,   115,    91,
      92,    73,    74,    90,    94,    77,    91,    73,    73,    86,
      73,   107,   107,    73,    72,    73,   111,   103,   107,   108,
     109,   113,   114,    73,    55,    73,    74,    77,    74,    79,
      41,    87,    90,    72,    73,   106,    73,   115,    73,   115,
     106,   115,    78,   117,   116,    79,    91,    73,    86,    13,
     106,    73,   106,    73,    73,   110,    73,   106,   106,   106,
      75,    35,    79,   112
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    80,    81,    82,    82,    83,    83,    84,    85,    85,
      85,    85,    86,    86,    86,    86,    87,    87,    87,    88,
      88,    88,    88,    88,    89,    89,    90,    90,    90,    90,
      90,    90,    90,    90,    91,    91,    91,    92,    92,    93,
      93,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    94,    94,    94,    94,    94,    94,    94,    94,
      94,    94,    95,    95,    95,    95,    95,    96,    96,    97,
      97,    97,    97,    97,    97,    98,    98,    99,   100,   100,
     100,   101,   101,   102,   102,   103,   103,   104,   104,   105,
     105,   106,   106,   106,   106,   106,   106,   107,   107,   108,
     108,   108,   109,   109,   109,   109,   109,   110,   110,   111,
     111,   112,   112,   112,   113,   113,   113,   113,   113,   113,
     114,   114,   114,   114,   115,   115,   116,   116,   116,   117,
     117,   118,   118,   119,   119,   120,   120,   121,   121,   122,
     122,   123,   123,   123,   124,   124,   124,   124,   124,   125,
     125,   125,   126,   126,   126,   127,   127,   127,   127,   128,
     128,   128,   128,   128,   128,   128,   128,   128,   128,   128,
     129,   129,   129,   129,   129,   129,   129,   129,   130,   130,
     130,   130,   130,   130,   131,   131
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     2,     5,     4,
       6,     5,     1,     3,     1,     3,     2,     1,     3,     3,
       4,     2,     3,     4,     1,     3,     1,     2,     4,     3,
       3,     4,     7,     6,     1,     3,     4,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     5,
       4,     2,     5,     4,     2,     1,     2,     3,     5,     4,
       2,     1,     3,     1,     3,     2,     3,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     5,
       7,     7,     1,     1,     1,     1,     1,     0,     2,     0,
       2,     4,     3,     3,     5,     7,     6,     7,     6,     7,
       2,     3,     2,     2,     1,     3,     1,     3,     3,     1,
       5,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     3,     1,     3,     3,     3,     3,     1,
       3,     3,     1,     3,     3,     1,     3,     3,     3,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     4,
       1,     4,     3,     4,     3,     3,     2,     2,     1,     1,
       1,     1,     1,     3,     1,     3
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
#line 963 "cl_parser.y"
        { (yyval.node_idx) = new_node("program → translation_unit"); }
#line 2486 "y.tab.c"
    break;

  case 3: /* translation_unit: external_decl  */
#line 968 "cl_parser.y"
        { (yyval.node_idx) = new_node("translation_unit → external_decl"); }
#line 2492 "y.tab.c"
    break;

  case 4: /* translation_unit: translation_unit external_decl  */
#line 970 "cl_parser.y"
        { (yyval.node_idx) = new_node("translation_unit → translation_unit external_decl"); }
#line 2498 "y.tab.c"
    break;

  case 5: /* external_decl: function_def  */
#line 975 "cl_parser.y"
        { (yyval.node_idx) = new_node("external_decl → function_def"); }
#line 2504 "y.tab.c"
    break;

  case 6: /* external_decl: declaration  */
#line 977 "cl_parser.y"
        { cnt_global_decls++; (yyval.node_idx) = new_node("external_decl → declaration"); }
#line 2510 "y.tab.c"
    break;

  case 7: /* function_def: function_header compound_stmt  */
#line 984 "cl_parser.y"
        { cnt_func_defs++; (yyval.node_idx) = new_node("function_def → function_header compound_stmt"); }
#line 2516 "y.tab.c"
    break;

  case 8: /* function_header: type_spec IDENTIFIER '(' param_list ')'  */
#line 989 "cl_parser.y"
        { (yyval.node_idx) = new_node("function_header → type_spec IDENTIFIER ( param_list )"); }
#line 2522 "y.tab.c"
    break;

  case 9: /* function_header: type_spec IDENTIFIER '(' ')'  */
#line 991 "cl_parser.y"
        { (yyval.node_idx) = new_node("function_header → type_spec IDENTIFIER ( )"); }
#line 2528 "y.tab.c"
    break;

  case 10: /* function_header: storage_class_list type_spec IDENTIFIER '(' param_list ')'  */
#line 994 "cl_parser.y"
        { (yyval.node_idx) = new_node("function_header → storage_class_list type_spec IDENTIFIER ( param_list )"); }
#line 2534 "y.tab.c"
    break;

  case 11: /* function_header: storage_class_list type_spec IDENTIFIER '(' ')'  */
#line 996 "cl_parser.y"
        { (yyval.node_idx) = new_node("function_header → storage_class_list type_spec IDENTIFIER ( )"); }
#line 2540 "y.tab.c"
    break;

  case 12: /* param_list: param_decl  */
#line 1001 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list → param_decl"); }
#line 2546 "y.tab.c"
    break;

  case 13: /* param_list: param_list ',' param_decl  */
#line 1003 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list → param_list , param_decl"); }
#line 2552 "y.tab.c"
    break;

  case 14: /* param_list: ELLIPSIS  */
#line 1005 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list → ..."); }
#line 2558 "y.tab.c"
    break;

  case 15: /* param_list: param_list ',' ELLIPSIS  */
#line 1007 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_list → param_list , ..."); }
#line 2564 "y.tab.c"
    break;

  case 16: /* param_decl: type_spec declarator  */
#line 1012 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_decl → type_spec declarator"); }
#line 2570 "y.tab.c"
    break;

  case 17: /* param_decl: type_spec  */
#line 1014 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_decl → type_spec"); }
#line 2576 "y.tab.c"
    break;

  case 18: /* param_decl: storage_class_list type_spec declarator  */
#line 1016 "cl_parser.y"
        { (yyval.node_idx) = new_node("param_decl → storage_class_list type_spec declarator"); }
#line 2582 "y.tab.c"
    break;

  case 19: /* declaration: type_spec declarator_list ';'  */
#line 1023 "cl_parser.y"
        { (yyval.node_idx) = new_node("declaration → type_spec declarator_list ;"); }
#line 2588 "y.tab.c"
    break;

  case 20: /* declaration: storage_class_list type_spec declarator_list ';'  */
#line 1025 "cl_parser.y"
        { (yyval.node_idx) = new_node("declaration → storage_class_list type_spec declarator_list ;"); }
#line 2594 "y.tab.c"
    break;

  case 21: /* declaration: type_spec ';'  */
#line 1027 "cl_parser.y"
        { (yyval.node_idx) = new_node("declaration → type_spec ;"); }
#line 2600 "y.tab.c"
    break;

  case 22: /* declaration: storage_class_list type_spec ';'  */
#line 1029 "cl_parser.y"
        { (yyval.node_idx) = new_node("declaration → storage_class_list type_spec ;"); }
#line 2606 "y.tab.c"
    break;

  case 23: /* declaration: TYPEDEF type_spec declarator_list ';'  */
#line 1037 "cl_parser.y"
        { (yyval.node_idx) = new_node("declaration → typedef type_spec declarator_list ;"); }
#line 2612 "y.tab.c"
    break;

  case 24: /* declarator_list: declarator  */
#line 1044 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator_list → declarator"); }
#line 2618 "y.tab.c"
    break;

  case 25: /* declarator_list: declarator_list ',' declarator  */
#line 1046 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator_list → declarator_list , declarator"); }
#line 2624 "y.tab.c"
    break;

  case 26: /* declarator: IDENTIFIER  */
#line 1051 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator → IDENTIFIER"); }
#line 2630 "y.tab.c"
    break;

  case 27: /* declarator: '*' IDENTIFIER  */
#line 1053 "cl_parser.y"
        { cnt_ptr_decls++; (yyval.node_idx) = new_node("declarator → * IDENTIFIER"); }
#line 2636 "y.tab.c"
    break;

  case 28: /* declarator: IDENTIFIER '[' INT_CONST ']'  */
#line 1055 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator → IDENTIFIER [ INT_CONST ]"); }
#line 2642 "y.tab.c"
    break;

  case 29: /* declarator: IDENTIFIER '[' ']'  */
#line 1057 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator → IDENTIFIER [ ]"); }
#line 2648 "y.tab.c"
    break;

  case 30: /* declarator: IDENTIFIER '=' initializer  */
#line 1059 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator → IDENTIFIER = initializer"); }
#line 2654 "y.tab.c"
    break;

  case 31: /* declarator: '*' IDENTIFIER '=' initializer  */
#line 1061 "cl_parser.y"
        { cnt_ptr_decls++; (yyval.node_idx) = new_node("declarator → * IDENTIFIER = initializer"); }
#line 2660 "y.tab.c"
    break;

  case 32: /* declarator: '(' '*' IDENTIFIER ')' '(' param_list ')'  */
#line 1063 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator → ( * IDENTIFIER ) ( param_list )"); }
#line 2666 "y.tab.c"
    break;

  case 33: /* declarator: '(' '*' IDENTIFIER ')' '(' ')'  */
#line 1065 "cl_parser.y"
        { (yyval.node_idx) = new_node("declarator → ( * IDENTIFIER ) ( )"); }
#line 2672 "y.tab.c"
    break;

  case 34: /* initializer: assignment_expr  */
#line 1070 "cl_parser.y"
        { (yyval.node_idx) = new_node("initializer → assignment_expr"); }
#line 2678 "y.tab.c"
    break;

  case 35: /* initializer: '{' initializer_list '}'  */
#line 1072 "cl_parser.y"
        { (yyval.node_idx) = new_node("initializer → { initializer_list }"); }
#line 2684 "y.tab.c"
    break;

  case 36: /* initializer: '{' initializer_list ',' '}'  */
#line 1074 "cl_parser.y"
        { (yyval.node_idx) = new_node("initializer → { initializer_list , }"); }
#line 2690 "y.tab.c"
    break;

  case 37: /* initializer_list: initializer  */
#line 1079 "cl_parser.y"
        { (yyval.node_idx) = new_node("initializer_list → initializer"); }
#line 2696 "y.tab.c"
    break;

  case 38: /* initializer_list: initializer_list ',' initializer  */
#line 1081 "cl_parser.y"
        { (yyval.node_idx) = new_node("initializer_list → initializer_list , initializer"); }
#line 2702 "y.tab.c"
    break;

  case 39: /* type_qualifier: CONST  */
#line 1100 "cl_parser.y"
               { (yyval.node_idx) = new_node("type_qualifier → const"); }
#line 2708 "y.tab.c"
    break;

  case 40: /* type_qualifier: VOLATILE  */
#line 1101 "cl_parser.y"
               { (yyval.node_idx) = new_node("type_qualifier → volatile"); }
#line 2714 "y.tab.c"
    break;

  case 41: /* type_spec: INT  */
#line 1105 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → int"); }
#line 2720 "y.tab.c"
    break;

  case 42: /* type_spec: FLOAT  */
#line 1106 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → float"); }
#line 2726 "y.tab.c"
    break;

  case 43: /* type_spec: CHAR  */
#line 1107 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → char"); }
#line 2732 "y.tab.c"
    break;

  case 44: /* type_spec: VOID  */
#line 1108 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → void"); }
#line 2738 "y.tab.c"
    break;

  case 45: /* type_spec: DOUBLE  */
#line 1109 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → double"); }
#line 2744 "y.tab.c"
    break;

  case 46: /* type_spec: SHORT  */
#line 1110 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → short"); }
#line 2750 "y.tab.c"
    break;

  case 47: /* type_spec: LONG  */
#line 1111 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → long"); }
#line 2756 "y.tab.c"
    break;

  case 48: /* type_spec: UNSIGNED  */
#line 1112 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → unsigned"); }
#line 2762 "y.tab.c"
    break;

  case 49: /* type_spec: SIGNED  */
#line 1113 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → signed"); }
#line 2768 "y.tab.c"
    break;

  case 50: /* type_spec: type_qualifier type_spec  */
#line 1115 "cl_parser.y"
        { (yyval.node_idx) = new_node("type_spec → type_qualifier type_spec"); }
#line 2774 "y.tab.c"
    break;

  case 51: /* type_spec: SHORT INT  */
#line 1116 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → short int"); }
#line 2780 "y.tab.c"
    break;

  case 52: /* type_spec: LONG INT  */
#line 1117 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → long int"); }
#line 2786 "y.tab.c"
    break;

  case 53: /* type_spec: LONG DOUBLE  */
#line 1118 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → long double"); }
#line 2792 "y.tab.c"
    break;

  case 54: /* type_spec: UNSIGNED INT  */
#line 1119 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → unsigned int"); }
#line 2798 "y.tab.c"
    break;

  case 55: /* type_spec: UNSIGNED LONG  */
#line 1120 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → unsigned long"); }
#line 2804 "y.tab.c"
    break;

  case 56: /* type_spec: UNSIGNED CHAR  */
#line 1121 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → unsigned char"); }
#line 2810 "y.tab.c"
    break;

  case 57: /* type_spec: SIGNED INT  */
#line 1122 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → signed int"); }
#line 2816 "y.tab.c"
    break;

  case 58: /* type_spec: LONG LONG  */
#line 1123 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → long long"); }
#line 2822 "y.tab.c"
    break;

  case 59: /* type_spec: LONG LONG INT  */
#line 1124 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → long long int"); }
#line 2828 "y.tab.c"
    break;

  case 60: /* type_spec: struct_decl  */
#line 1125 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → struct_decl"); }
#line 2834 "y.tab.c"
    break;

  case 61: /* type_spec: enum_decl  */
#line 1126 "cl_parser.y"
                    { (yyval.node_idx) = new_node("type_spec → enum_decl"); }
#line 2840 "y.tab.c"
    break;

  case 62: /* storage_class_spec: AUTO  */
#line 1130 "cl_parser.y"
                { (yyval.node_idx) = new_node("storage_class_spec → auto"); }
#line 2846 "y.tab.c"
    break;

  case 63: /* storage_class_spec: REGISTER  */
#line 1131 "cl_parser.y"
                { (yyval.node_idx) = new_node("storage_class_spec → register"); }
#line 2852 "y.tab.c"
    break;

  case 64: /* storage_class_spec: STATIC  */
#line 1132 "cl_parser.y"
                { (yyval.node_idx) = new_node("storage_class_spec → static"); }
#line 2858 "y.tab.c"
    break;

  case 65: /* storage_class_spec: EXTERN  */
#line 1133 "cl_parser.y"
                { (yyval.node_idx) = new_node("storage_class_spec → extern"); }
#line 2864 "y.tab.c"
    break;

  case 66: /* storage_class_spec: INLINE  */
#line 1134 "cl_parser.y"
                { (yyval.node_idx) = new_node("storage_class_spec → inline"); }
#line 2870 "y.tab.c"
    break;

  case 67: /* storage_class_list: storage_class_spec  */
#line 1147 "cl_parser.y"
        { (yyval.node_idx) = new_node("storage_class_list → storage_class_spec"); }
#line 2876 "y.tab.c"
    break;

  case 68: /* storage_class_list: storage_class_list storage_class_spec  */
#line 1149 "cl_parser.y"
        { (yyval.node_idx) = new_node("storage_class_list → storage_class_list storage_class_spec"); }
#line 2882 "y.tab.c"
    break;

  case 69: /* struct_decl: STRUCT IDENTIFIER '{' member_decl_list '}'  */
#line 1156 "cl_parser.y"
        { cnt_struct_decls++; (yyval.node_idx) = new_node("struct_decl → struct IDENTIFIER { member_decl_list }"); }
#line 2888 "y.tab.c"
    break;

  case 70: /* struct_decl: STRUCT '{' member_decl_list '}'  */
#line 1158 "cl_parser.y"
        { cnt_struct_decls++; (yyval.node_idx) = new_node("struct_decl → struct { member_decl_list }"); }
#line 2894 "y.tab.c"
    break;

  case 71: /* struct_decl: STRUCT IDENTIFIER  */
#line 1160 "cl_parser.y"
        { (yyval.node_idx) = new_node("struct_decl → struct IDENTIFIER"); }
#line 2900 "y.tab.c"
    break;

  case 72: /* struct_decl: UNION IDENTIFIER '{' member_decl_list '}'  */
#line 1162 "cl_parser.y"
        { (yyval.node_idx) = new_node("struct_decl → union IDENTIFIER { member_decl_list }"); }
#line 2906 "y.tab.c"
    break;

  case 73: /* struct_decl: UNION '{' member_decl_list '}'  */
#line 1164 "cl_parser.y"
        { (yyval.node_idx) = new_node("struct_decl → union { member_decl_list }"); }
#line 2912 "y.tab.c"
    break;

  case 74: /* struct_decl: UNION IDENTIFIER  */
#line 1166 "cl_parser.y"
        { (yyval.node_idx) = new_node("struct_decl → union IDENTIFIER"); }
#line 2918 "y.tab.c"
    break;

  case 75: /* member_decl_list: member_decl  */
#line 1171 "cl_parser.y"
        { (yyval.node_idx) = new_node("member_decl_list → member_decl"); }
#line 2924 "y.tab.c"
    break;

  case 76: /* member_decl_list: member_decl_list member_decl  */
#line 1173 "cl_parser.y"
        { (yyval.node_idx) = new_node("member_decl_list → member_decl_list member_decl"); }
#line 2930 "y.tab.c"
    break;

  case 77: /* member_decl: type_spec declarator_list ';'  */
#line 1178 "cl_parser.y"
        { (yyval.node_idx) = new_node("member_decl → type_spec declarator_list ;"); }
#line 2936 "y.tab.c"
    break;

  case 78: /* enum_decl: ENUM IDENTIFIER '{' enumerator_list '}'  */
#line 1184 "cl_parser.y"
        { (yyval.node_idx) = new_node("enum_decl → enum IDENTIFIER { enumerator_list }"); }
#line 2942 "y.tab.c"
    break;

  case 79: /* enum_decl: ENUM '{' enumerator_list '}'  */
#line 1186 "cl_parser.y"
        { (yyval.node_idx) = new_node("enum_decl → enum { enumerator_list }"); }
#line 2948 "y.tab.c"
    break;

  case 80: /* enum_decl: ENUM IDENTIFIER  */
#line 1188 "cl_parser.y"
        { (yyval.node_idx) = new_node("enum_decl → enum IDENTIFIER"); }
#line 2954 "y.tab.c"
    break;

  case 81: /* enumerator_list: enumerator  */
#line 1193 "cl_parser.y"
        { (yyval.node_idx) = new_node("enumerator_list → enumerator"); }
#line 2960 "y.tab.c"
    break;

  case 82: /* enumerator_list: enumerator_list ',' enumerator  */
#line 1195 "cl_parser.y"
        { (yyval.node_idx) = new_node("enumerator_list → enumerator_list , enumerator"); }
#line 2966 "y.tab.c"
    break;

  case 83: /* enumerator: IDENTIFIER  */
#line 1200 "cl_parser.y"
        { (yyval.node_idx) = new_node("enumerator → IDENTIFIER"); }
#line 2972 "y.tab.c"
    break;

  case 84: /* enumerator: IDENTIFIER '=' INT_CONST  */
#line 1202 "cl_parser.y"
        { (yyval.node_idx) = new_node("enumerator → IDENTIFIER = INT_CONST"); }
#line 2978 "y.tab.c"
    break;

  case 85: /* compound_stmt: '{' '}'  */
#line 1228 "cl_parser.y"
        { (yyval.node_idx) = new_node("compound_stmt → { }"); }
#line 2984 "y.tab.c"
    break;

  case 86: /* compound_stmt: '{' block_item_list '}'  */
#line 1230 "cl_parser.y"
        { (yyval.node_idx) = new_node("compound_stmt → { block_item_list }"); }
#line 2990 "y.tab.c"
    break;

  case 87: /* block_item_list: block_item  */
#line 1235 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item_list → block_item"); }
#line 2996 "y.tab.c"
    break;

  case 88: /* block_item_list: block_item_list block_item  */
#line 1237 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item_list → block_item_list block_item"); }
#line 3002 "y.tab.c"
    break;

  case 89: /* block_item: declaration  */
#line 1242 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item → declaration"); }
#line 3008 "y.tab.c"
    break;

  case 90: /* block_item: stmt  */
#line 1244 "cl_parser.y"
        { (yyval.node_idx) = new_node("block_item → stmt"); }
#line 3014 "y.tab.c"
    break;

  case 91: /* stmt: compound_stmt  */
#line 1250 "cl_parser.y"
                       { (yyval.node_idx) = new_node("stmt → compound_stmt"); }
#line 3020 "y.tab.c"
    break;

  case 92: /* stmt: expr_stmt  */
#line 1251 "cl_parser.y"
                       { (yyval.node_idx) = new_node("stmt → expr_stmt"); }
#line 3026 "y.tab.c"
    break;

  case 93: /* stmt: selection_stmt  */
#line 1252 "cl_parser.y"
                       { (yyval.node_idx) = new_node("stmt → selection_stmt"); }
#line 3032 "y.tab.c"
    break;

  case 94: /* stmt: iteration_stmt  */
#line 1253 "cl_parser.y"
                       { (yyval.node_idx) = new_node("stmt → iteration_stmt"); }
#line 3038 "y.tab.c"
    break;

  case 95: /* stmt: jump_stmt  */
#line 1254 "cl_parser.y"
                       { (yyval.node_idx) = new_node("stmt → jump_stmt"); }
#line 3044 "y.tab.c"
    break;

  case 96: /* stmt: labeled_stmt  */
#line 1255 "cl_parser.y"
                       { (yyval.node_idx) = new_node("stmt → labeled_stmt"); }
#line 3050 "y.tab.c"
    break;

  case 97: /* expr_stmt: ';'  */
#line 1260 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr_stmt → ;"); }
#line 3056 "y.tab.c"
    break;

  case 98: /* expr_stmt: expr ';'  */
#line 1262 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr_stmt → expr ;"); }
#line 3062 "y.tab.c"
    break;

  case 99: /* selection_stmt: IF '(' expr ')' stmt  */
#line 1284 "cl_parser.y"
        { cnt_if_without_else++; if_depth_current++; if (if_depth_current > if_depth_max) if_depth_max = if_depth_current; (yyval.node_idx) = new_node("selection_stmt → if ( expr ) stmt"); if_depth_current--; }
#line 3068 "y.tab.c"
    break;

  case 100: /* selection_stmt: IF '(' expr ')' stmt ELSE stmt  */
#line 1286 "cl_parser.y"
        { cnt_if_with_else++; if_depth_current++; if (if_depth_current > if_depth_max) if_depth_max = if_depth_current; (yyval.node_idx) = new_node("selection_stmt → if ( expr ) stmt else stmt"); if_depth_current--; }
#line 3074 "y.tab.c"
    break;

  case 101: /* selection_stmt: SWITCH '(' expr ')' '{' case_list '}'  */
#line 1288 "cl_parser.y"
        { cnt_switch_stmts++; (yyval.node_idx) = new_node("selection_stmt → switch ( expr ) { case_list }"); }
#line 3080 "y.tab.c"
    break;

  case 102: /* non_case_stmt: compound_stmt  */
#line 1317 "cl_parser.y"
                      { (yyval.node_idx) = new_node("non_case_stmt → compound_stmt"); }
#line 3086 "y.tab.c"
    break;

  case 103: /* non_case_stmt: expr_stmt  */
#line 1318 "cl_parser.y"
                      { (yyval.node_idx) = new_node("non_case_stmt → expr_stmt"); }
#line 3092 "y.tab.c"
    break;

  case 104: /* non_case_stmt: selection_stmt  */
#line 1319 "cl_parser.y"
                      { (yyval.node_idx) = new_node("non_case_stmt → selection_stmt"); }
#line 3098 "y.tab.c"
    break;

  case 105: /* non_case_stmt: iteration_stmt  */
#line 1320 "cl_parser.y"
                      { (yyval.node_idx) = new_node("non_case_stmt → iteration_stmt"); }
#line 3104 "y.tab.c"
    break;

  case 106: /* non_case_stmt: jump_stmt  */
#line 1321 "cl_parser.y"
                      { (yyval.node_idx) = new_node("non_case_stmt → jump_stmt"); }
#line 3110 "y.tab.c"
    break;

  case 107: /* case_list: %empty  */
#line 1326 "cl_parser.y"
        { (yyval.node_idx) = 0; }
#line 3116 "y.tab.c"
    break;

  case 108: /* case_list: case_list labeled_stmt  */
#line 1328 "cl_parser.y"
        { (yyval.node_idx) = new_node("case_list → case_list labeled_stmt"); }
#line 3122 "y.tab.c"
    break;

  case 109: /* case_body_list: %empty  */
#line 1335 "cl_parser.y"
        { (yyval.node_idx) = 0; }
#line 3128 "y.tab.c"
    break;

  case 110: /* case_body_list: case_body_list non_case_stmt  */
#line 1337 "cl_parser.y"
        { (yyval.node_idx) = new_node("case_body_list → case_body_list non_case_stmt"); }
#line 3134 "y.tab.c"
    break;

  case 111: /* labeled_stmt: CASE expr ':' case_body_list  */
#line 1342 "cl_parser.y"
        { (yyval.node_idx) = new_node("labeled_stmt → case expr : case_body_list"); }
#line 3140 "y.tab.c"
    break;

  case 112: /* labeled_stmt: DEFAULT ':' case_body_list  */
#line 1344 "cl_parser.y"
        { (yyval.node_idx) = new_node("labeled_stmt → default : case_body_list"); }
#line 3146 "y.tab.c"
    break;

  case 113: /* labeled_stmt: IDENTIFIER ':' stmt  */
#line 1346 "cl_parser.y"
        { (yyval.node_idx) = new_node("labeled_stmt → IDENTIFIER : stmt"); }
#line 3152 "y.tab.c"
    break;

  case 114: /* iteration_stmt: WHILE '(' expr ')' stmt  */
#line 1380 "cl_parser.y"
        { cnt_while_loops++; (yyval.node_idx) = new_node("iteration_stmt → while ( expr ) stmt"); }
#line 3158 "y.tab.c"
    break;

  case 115: /* iteration_stmt: DO stmt WHILE '(' expr ')' ';'  */
#line 1382 "cl_parser.y"
        { cnt_do_while_loops++; (yyval.node_idx) = new_node("iteration_stmt → do stmt while ( expr ) ;"); }
#line 3164 "y.tab.c"
    break;

  case 116: /* iteration_stmt: FOR '(' expr_stmt expr_stmt ')' stmt  */
#line 1385 "cl_parser.y"
        { cnt_for_loops++; (yyval.node_idx) = new_node("iteration_stmt → for ( expr_stmt expr_stmt ) stmt"); }
#line 3170 "y.tab.c"
    break;

  case 117: /* iteration_stmt: FOR '(' expr_stmt expr_stmt expr ')' stmt  */
#line 1387 "cl_parser.y"
        { cnt_for_loops++; (yyval.node_idx) = new_node("iteration_stmt → for ( expr_stmt expr_stmt expr ) stmt"); }
#line 3176 "y.tab.c"
    break;

  case 118: /* iteration_stmt: FOR '(' declaration expr_stmt ')' stmt  */
#line 1390 "cl_parser.y"
        { cnt_for_loops++; (yyval.node_idx) = new_node("iteration_stmt → for ( declaration expr_stmt ) stmt"); }
#line 3182 "y.tab.c"
    break;

  case 119: /* iteration_stmt: FOR '(' declaration expr_stmt expr ')' stmt  */
#line 1392 "cl_parser.y"
        { cnt_for_loops++; (yyval.node_idx) = new_node("iteration_stmt → for ( declaration expr_stmt expr ) stmt"); }
#line 3188 "y.tab.c"
    break;

  case 120: /* jump_stmt: RETURN ';'  */
#line 1397 "cl_parser.y"
        { cnt_return_stmts++; (yyval.node_idx) = new_node("jump_stmt → return ;"); }
#line 3194 "y.tab.c"
    break;

  case 121: /* jump_stmt: RETURN expr ';'  */
#line 1399 "cl_parser.y"
        { cnt_return_stmts++; (yyval.node_idx) = new_node("jump_stmt → return expr ;"); }
#line 3200 "y.tab.c"
    break;

  case 122: /* jump_stmt: BREAK ';'  */
#line 1401 "cl_parser.y"
        { (yyval.node_idx) = new_node("jump_stmt → break ;"); }
#line 3206 "y.tab.c"
    break;

  case 123: /* jump_stmt: CONTINUE ';'  */
#line 1403 "cl_parser.y"
        { (yyval.node_idx) = new_node("jump_stmt → continue ;"); }
#line 3212 "y.tab.c"
    break;

  case 124: /* expr: assignment_expr  */
#line 1410 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr → assignment_expr"); }
#line 3218 "y.tab.c"
    break;

  case 125: /* expr: expr ',' assignment_expr  */
#line 1412 "cl_parser.y"
        { (yyval.node_idx) = new_node("expr → expr , assignment_expr"); }
#line 3224 "y.tab.c"
    break;

  case 126: /* assignment_expr: conditional_expr  */
#line 1417 "cl_parser.y"
        { (yyval.node_idx) = new_node("assignment_expr → conditional_expr"); }
#line 3230 "y.tab.c"
    break;

  case 127: /* assignment_expr: unary_expr '=' assignment_expr  */
#line 1419 "cl_parser.y"
        { cnt_assignments++; (yyval.node_idx) = new_node("assignment_expr → unary_expr = assignment_expr"); }
#line 3236 "y.tab.c"
    break;

  case 128: /* assignment_expr: unary_expr ASSIGN_OP assignment_expr  */
#line 1421 "cl_parser.y"
        { cnt_assignments++; (yyval.node_idx) = new_node("assignment_expr → unary_expr ASSIGN_OP assignment_expr"); }
#line 3242 "y.tab.c"
    break;

  case 129: /* conditional_expr: logical_or_expr  */
#line 1426 "cl_parser.y"
        { (yyval.node_idx) = new_node("conditional_expr → logical_or_expr"); }
#line 3248 "y.tab.c"
    break;

  case 130: /* conditional_expr: logical_or_expr '?' expr ':' conditional_expr  */
#line 1428 "cl_parser.y"
        { (yyval.node_idx) = new_node("conditional_expr → logical_or_expr ? expr : conditional_expr"); }
#line 3254 "y.tab.c"
    break;

  case 131: /* logical_or_expr: logical_and_expr  */
#line 1433 "cl_parser.y"
        { (yyval.node_idx) = new_node("logical_or_expr → logical_and_expr"); }
#line 3260 "y.tab.c"
    break;

  case 132: /* logical_or_expr: logical_or_expr OR_OP logical_and_expr  */
#line 1435 "cl_parser.y"
        { (yyval.node_idx) = new_node("logical_or_expr → logical_or_expr || logical_and_expr"); }
#line 3266 "y.tab.c"
    break;

  case 133: /* logical_and_expr: inclusive_or_expr  */
#line 1440 "cl_parser.y"
        { (yyval.node_idx) = new_node("logical_and_expr → inclusive_or_expr"); }
#line 3272 "y.tab.c"
    break;

  case 134: /* logical_and_expr: logical_and_expr AND_OP inclusive_or_expr  */
#line 1442 "cl_parser.y"
        { (yyval.node_idx) = new_node("logical_and_expr → logical_and_expr && inclusive_or_expr"); }
#line 3278 "y.tab.c"
    break;

  case 135: /* inclusive_or_expr: exclusive_or_expr  */
#line 1447 "cl_parser.y"
        { (yyval.node_idx) = new_node("inclusive_or_expr → exclusive_or_expr"); }
#line 3284 "y.tab.c"
    break;

  case 136: /* inclusive_or_expr: inclusive_or_expr '|' exclusive_or_expr  */
#line 1449 "cl_parser.y"
        { (yyval.node_idx) = new_node("inclusive_or_expr → inclusive_or_expr | exclusive_or_expr"); }
#line 3290 "y.tab.c"
    break;

  case 137: /* exclusive_or_expr: and_expr  */
#line 1454 "cl_parser.y"
        { (yyval.node_idx) = new_node("exclusive_or_expr → and_expr"); }
#line 3296 "y.tab.c"
    break;

  case 138: /* exclusive_or_expr: exclusive_or_expr '^' and_expr  */
#line 1456 "cl_parser.y"
        { (yyval.node_idx) = new_node("exclusive_or_expr → exclusive_or_expr ^ and_expr"); }
#line 3302 "y.tab.c"
    break;

  case 139: /* and_expr: equality_expr  */
#line 1461 "cl_parser.y"
        { (yyval.node_idx) = new_node("and_expr → equality_expr"); }
#line 3308 "y.tab.c"
    break;

  case 140: /* and_expr: and_expr '&' equality_expr  */
#line 1463 "cl_parser.y"
        { (yyval.node_idx) = new_node("and_expr → and_expr & equality_expr"); }
#line 3314 "y.tab.c"
    break;

  case 141: /* equality_expr: relational_expr  */
#line 1468 "cl_parser.y"
        { (yyval.node_idx) = new_node("equality_expr → relational_expr"); }
#line 3320 "y.tab.c"
    break;

  case 142: /* equality_expr: equality_expr EQ_OP relational_expr  */
#line 1470 "cl_parser.y"
        { (yyval.node_idx) = new_node("equality_expr → equality_expr == relational_expr"); }
#line 3326 "y.tab.c"
    break;

  case 143: /* equality_expr: equality_expr NE_OP relational_expr  */
#line 1472 "cl_parser.y"
        { (yyval.node_idx) = new_node("equality_expr → equality_expr != relational_expr"); }
#line 3332 "y.tab.c"
    break;

  case 144: /* relational_expr: shift_expr  */
#line 1477 "cl_parser.y"
        { (yyval.node_idx) = new_node("relational_expr → shift_expr"); }
#line 3338 "y.tab.c"
    break;

  case 145: /* relational_expr: relational_expr '<' shift_expr  */
#line 1479 "cl_parser.y"
        { (yyval.node_idx) = new_node("relational_expr → relational_expr < shift_expr"); }
#line 3344 "y.tab.c"
    break;

  case 146: /* relational_expr: relational_expr '>' shift_expr  */
#line 1481 "cl_parser.y"
        { (yyval.node_idx) = new_node("relational_expr → relational_expr > shift_expr"); }
#line 3350 "y.tab.c"
    break;

  case 147: /* relational_expr: relational_expr LE_OP shift_expr  */
#line 1483 "cl_parser.y"
        { (yyval.node_idx) = new_node("relational_expr → relational_expr <= shift_expr"); }
#line 3356 "y.tab.c"
    break;

  case 148: /* relational_expr: relational_expr GE_OP shift_expr  */
#line 1485 "cl_parser.y"
        { (yyval.node_idx) = new_node("relational_expr → relational_expr >= shift_expr"); }
#line 3362 "y.tab.c"
    break;

  case 149: /* shift_expr: additive_expr  */
#line 1490 "cl_parser.y"
        { (yyval.node_idx) = new_node("shift_expr → additive_expr"); }
#line 3368 "y.tab.c"
    break;

  case 150: /* shift_expr: shift_expr LEFT_SHIFT additive_expr  */
#line 1492 "cl_parser.y"
        { (yyval.node_idx) = new_node("shift_expr → shift_expr << additive_expr"); }
#line 3374 "y.tab.c"
    break;

  case 151: /* shift_expr: shift_expr RIGHT_SHIFT additive_expr  */
#line 1494 "cl_parser.y"
        { (yyval.node_idx) = new_node("shift_expr → shift_expr >> additive_expr"); }
#line 3380 "y.tab.c"
    break;

  case 152: /* additive_expr: multiplicative_expr  */
#line 1499 "cl_parser.y"
        { (yyval.node_idx) = new_node("additive_expr → multiplicative_expr"); }
#line 3386 "y.tab.c"
    break;

  case 153: /* additive_expr: additive_expr '+' multiplicative_expr  */
#line 1501 "cl_parser.y"
        { (yyval.node_idx) = new_node("additive_expr → additive_expr + multiplicative_expr"); }
#line 3392 "y.tab.c"
    break;

  case 154: /* additive_expr: additive_expr '-' multiplicative_expr  */
#line 1503 "cl_parser.y"
        { (yyval.node_idx) = new_node("additive_expr → additive_expr - multiplicative_expr"); }
#line 3398 "y.tab.c"
    break;

  case 155: /* multiplicative_expr: unary_expr  */
#line 1508 "cl_parser.y"
        { (yyval.node_idx) = new_node("multiplicative_expr → unary_expr"); }
#line 3404 "y.tab.c"
    break;

  case 156: /* multiplicative_expr: multiplicative_expr '*' unary_expr  */
#line 1510 "cl_parser.y"
        { (yyval.node_idx) = new_node("multiplicative_expr → multiplicative_expr * unary_expr"); }
#line 3410 "y.tab.c"
    break;

  case 157: /* multiplicative_expr: multiplicative_expr '/' unary_expr  */
#line 1512 "cl_parser.y"
        { (yyval.node_idx) = new_node("multiplicative_expr → multiplicative_expr / unary_expr"); }
#line 3416 "y.tab.c"
    break;

  case 158: /* multiplicative_expr: multiplicative_expr '%' unary_expr  */
#line 1514 "cl_parser.y"
        { (yyval.node_idx) = new_node("multiplicative_expr → multiplicative_expr % unary_expr"); }
#line 3422 "y.tab.c"
    break;

  case 159: /* unary_expr: postfix_expr  */
#line 1524 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → postfix_expr"); }
#line 3428 "y.tab.c"
    break;

  case 160: /* unary_expr: INC_OP unary_expr  */
#line 1526 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → ++ unary_expr"); }
#line 3434 "y.tab.c"
    break;

  case 161: /* unary_expr: DEC_OP unary_expr  */
#line 1528 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → -- unary_expr"); }
#line 3440 "y.tab.c"
    break;

  case 162: /* unary_expr: '-' unary_expr  */
#line 1530 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → - unary_expr  [UMINUS]"); }
#line 3446 "y.tab.c"
    break;

  case 163: /* unary_expr: '+' unary_expr  */
#line 1532 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → + unary_expr  [UPLUS]"); }
#line 3452 "y.tab.c"
    break;

  case 164: /* unary_expr: '!' unary_expr  */
#line 1534 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → ! unary_expr"); }
#line 3458 "y.tab.c"
    break;

  case 165: /* unary_expr: '~' unary_expr  */
#line 1536 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → ~ unary_expr"); }
#line 3464 "y.tab.c"
    break;

  case 166: /* unary_expr: '*' unary_expr  */
#line 1538 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → * unary_expr  [deref]"); }
#line 3470 "y.tab.c"
    break;

  case 167: /* unary_expr: '&' unary_expr  */
#line 1540 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → & unary_expr  [addr-of]"); }
#line 3476 "y.tab.c"
    break;

  case 168: /* unary_expr: SIZEOF unary_expr  */
#line 1542 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → sizeof unary_expr"); }
#line 3482 "y.tab.c"
    break;

  case 169: /* unary_expr: SIZEOF '(' type_spec ')'  */
#line 1544 "cl_parser.y"
        { (yyval.node_idx) = new_node("unary_expr → sizeof ( type_spec )"); }
#line 3488 "y.tab.c"
    break;

  case 170: /* postfix_expr: primary_expr  */
#line 1564 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr → primary_expr"); }
#line 3494 "y.tab.c"
    break;

  case 171: /* postfix_expr: postfix_expr '[' expr ']'  */
#line 1566 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr → postfix_expr [ expr ]"); }
#line 3500 "y.tab.c"
    break;

  case 172: /* postfix_expr: postfix_expr '(' ')'  */
#line 1568 "cl_parser.y"
        { cnt_func_calls++; (yyval.node_idx) = new_node("postfix_expr → postfix_expr ( )"); }
#line 3506 "y.tab.c"
    break;

  case 173: /* postfix_expr: postfix_expr '(' arg_list ')'  */
#line 1570 "cl_parser.y"
        { cnt_func_calls++; (yyval.node_idx) = new_node("postfix_expr → postfix_expr ( arg_list )"); }
#line 3512 "y.tab.c"
    break;

  case 174: /* postfix_expr: postfix_expr '.' IDENTIFIER  */
#line 1572 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr → postfix_expr . IDENTIFIER"); }
#line 3518 "y.tab.c"
    break;

  case 175: /* postfix_expr: postfix_expr ARROW IDENTIFIER  */
#line 1574 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr → postfix_expr -> IDENTIFIER"); }
#line 3524 "y.tab.c"
    break;

  case 176: /* postfix_expr: postfix_expr INC_OP  */
#line 1576 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr → postfix_expr ++"); }
#line 3530 "y.tab.c"
    break;

  case 177: /* postfix_expr: postfix_expr DEC_OP  */
#line 1578 "cl_parser.y"
        { (yyval.node_idx) = new_node("postfix_expr → postfix_expr --"); }
#line 3536 "y.tab.c"
    break;

  case 178: /* primary_expr: IDENTIFIER  */
#line 1583 "cl_parser.y"
        { (yyval.node_idx) = new_node("primary_expr → IDENTIFIER"); }
#line 3542 "y.tab.c"
    break;

  case 179: /* primary_expr: INT_CONST  */
#line 1585 "cl_parser.y"
        { cnt_int_consts++; (yyval.node_idx) = new_node("primary_expr → INT_CONST"); }
#line 3548 "y.tab.c"
    break;

  case 180: /* primary_expr: FLOAT_CONST  */
#line 1587 "cl_parser.y"
        { (yyval.node_idx) = new_node("primary_expr → FLOAT_CONST"); }
#line 3554 "y.tab.c"
    break;

  case 181: /* primary_expr: CHAR_CONST  */
#line 1589 "cl_parser.y"
        { (yyval.node_idx) = new_node("primary_expr → CHAR_CONST"); }
#line 3560 "y.tab.c"
    break;

  case 182: /* primary_expr: STRING_LITERAL  */
#line 1591 "cl_parser.y"
        { (yyval.node_idx) = new_node("primary_expr → STRING_LITERAL"); }
#line 3566 "y.tab.c"
    break;

  case 183: /* primary_expr: '(' expr ')'  */
#line 1593 "cl_parser.y"
        { (yyval.node_idx) = new_node("primary_expr → ( expr )"); }
#line 3572 "y.tab.c"
    break;

  case 184: /* arg_list: assignment_expr  */
#line 1598 "cl_parser.y"
        { (yyval.node_idx) = new_node("arg_list → assignment_expr"); }
#line 3578 "y.tab.c"
    break;

  case 185: /* arg_list: arg_list ',' assignment_expr  */
#line 1600 "cl_parser.y"
        { (yyval.node_idx) = new_node("arg_list → arg_list , assignment_expr"); }
#line 3584 "y.tab.c"
    break;


#line 3588 "y.tab.c"

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

#line 1603 "cl_parser.y"


/* ═══════════════════════════════════════════════════════════════════
   PART 6 — Additional Information
   ═══════════════════════════════════════════════════════════════════ */

static void print_grammar_summary(void) {
    FILE *f = fopen("y.output", "r");
    int rule_count   = 0;
    int state_count  = 0;
    int sr_conflicts = 0;
    int rr_conflicts = 0;

    if (f) {
        char buf[512];
        while (fgets(buf, sizeof(buf), f)) {
            if (buf[0]==' ' && buf[1]==' ' && buf[2]==' ' && buf[3]>='0' && buf[3]<='9')
                rule_count++;
            if (strncmp(buf,"State ",6)==0 || strncmp(buf,"state ",6)==0) {
                int s = atoi(buf+6);
                if (s+1 > state_count) state_count = s+1;
            }
            if (strstr(buf, "shift/reduce")) {
                int n = 0;
                sscanf(buf, "State %*d conflicts: %d", &n);
                sr_conflicts += (n > 0) ? n : 1;
            }
            if (strstr(buf, "reduce/reduce")) {
                int n = 0;
                sscanf(buf, "State %*d conflicts: %d", &n);
                rr_conflicts += (n > 0) ? n : 1;
            }
        }
        fclose(f);
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                 PART 6 — ADDITIONAL INFORMATION              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* ── Per-parse construct counts (like your friend's output) ── */
    printf("  ── Program Analysis (this parse) ──────────────────────────\n");
    printf("  #global_declarations         = %d\n", cnt_global_decls);
    printf("  #function_definitions        = %d\n", cnt_func_defs);
    printf("  #if_without_else             = %d\n", cnt_if_without_else);
    printf("  #if_with_else                = %d\n", cnt_if_with_else);
    printf("  if_else_max_nesting_depth    = %d\n", if_depth_max);
    printf("  #for_loops                   = %d\n", cnt_for_loops);
    printf("  #while_loops                 = %d\n", cnt_while_loops);
    printf("  #do_while_loops              = %d\n", cnt_do_while_loops);
    printf("  #switch_statements           = %d\n", cnt_switch_stmts);
    printf("  #return_statements           = %d\n", cnt_return_stmts);
    printf("  #pointer_declarations        = %d\n", cnt_ptr_decls);
    printf("  #integer_constants           = %d\n", cnt_int_consts);
    printf("  #function_calls              = %d\n", cnt_func_calls);
    printf("  #assignments                 = %d\n", cnt_assignments);
    printf("  #struct_union_declarations   = %d\n", cnt_struct_decls);
    printf("  #total_reductions            = %d\n", reduction_count);
    printf("\n");

    /* ── Grammar-level statistics ── */
    printf("  ── Grammar Statistics ───────────────────────────────────────\n");
    printf("  Productions (grammar rules)  : %d\n", rule_count  ? rule_count  : 0);
    printf("  LALR(1) states               : %d\n", state_count ? state_count : 0);
    printf("  Shift/reduce conflicts       : %d  (target: 0)\n", sr_conflicts);
    printf("  Reduce/reduce conflicts      : %d  (target: 0)\n", rr_conflicts);
    printf("\n");

    /* ── Conflict resolution summary ── */
    printf("  ── Conflict Resolution Summary ──────────────────────────────\n");
    printf("  1. Dangling-else (S/R)  → %%prec LOWER_THAN_ELSE\n");
    printf("     Effect: ELSE attaches to innermost if (C standard).\n");
    printf("\n");
    printf("  2. Operator precedence (S/R) → %%left/%%right declarations\n");
    printf("     15 levels: = (lowest) ... postfix ++ -- (highest).\n");
    printf("\n");
    printf("  3. Unary vs binary +/- (S/R) → %%prec UMINUS/UPLUS\n");
    printf("     Unary -/+ have higher precedence than binary -/+.\n");
    printf("\n");
    printf("  4. TYPEDEF declarator S/R (State 62)\n");
    printf("     → Removed redundant  TYPEDEF type_spec IDENTIFIER ';'\n");
    printf("       (covered by TYPEDEF type_spec declarator_list ';').\n");
    printf("\n");
    printf("  5. case_body_list S/R (States 233 & 284, 38 conflicts)\n");
    printf("     → Replaced  case_body_list stmt  with\n");
    printf("       case_body_list non_case_stmt  (excludes labeled_stmt).\n");
    printf("       CASE/DEFAULT always terminate the case body cleanly.\n");
    printf("\n");

    /* ── Key grammar features ── */
    printf("  ── Key Grammar Features ─────────────────────────────────────\n");
    printf("  - C99 mixed declarations/statements (block_item_list)\n");
    printf("  - Full 15-level expression hierarchy\n");
    printf("  - Structs, unions, enums, typedefs\n");
    printf("  - if/else, for, while, do-while, switch/case\n");
    printf("  - Pointer declarators, array declarators, function pointers\n");
    printf("  - sizeof, address-of, dereference, compound assignment\n");
    printf("  - const/volatile qualifiers, stacked storage classes\n");
    printf("  - C99 for-loop init declarations: for(int i=0; ...)\n");
    printf("\n");
}

/* ═══════════════════════════════════════════════════════════════════
   main
   ═══════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    int show_table = 0;
    int show_extra = 0;
    const char *yout_path = "y.output";

    if (argc < 2) {
        fprintf(stderr,
            "Usage: %s <input_file> [--table] [--extra] [--yout <path>]\n"
            "\n"
            "  --table    Show key parsing table states + export full table\n"
            "             to output/parsing_table.csv  (Part 4)\n"
            "  --extra    Print per-parse counters + grammar info  (Part 6)\n"
            "  --yout P   Path to y.output (default: ./y.output)\n"
            "\n"
            "  Output files written to output/ folder:\n"
            "    parsing_table.csv  — complete LALR(1) table (all states)\n",
            argv[0]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--table") == 0)       show_table = 1;
        else if (strcmp(argv[i], "--extra") == 0)  show_extra = 1;
        else if (strcmp(argv[i], "--yout") == 0 && i+1 < argc)
            yout_path = argv[++i];
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr,
            "***process terminated*** [input error]: cannot open '%s'\n",
            argv[1]);
        return 1;
    }

    printf("─────────────────────────────────────────────────────────────\n");
    printf("  CS327 LALR(1) Parser\n");
    printf("  Input file : %s\n", argv[1]);
    printf("─────────────────────────────────────────────────────────────\n\n");

    fflush(stdout);   /* FIX: flush header to stdout before yyparse() calls
                         yyerror() on stderr — ensures header always prints
                         first even when both streams are redirected to same file */
    yyparse();

    fclose(yyin);

    printf("─────────────────────────────────────────────────────────────\n");
    printf("  ***parsing successful***\n");
    printf("─────────────────────────────────────────────────────────────\n");

    /* Part 3 — always print reverse derivation tree */
    print_tree_reverse();

    /* Part 4 — optional parsing table
       Prints key states on screen + exports full table to output/parsing_table.csv */
    if (show_table)
        print_parsing_table(yout_path);

    /* Part 6 — optional additional info */
    if (show_extra)
        print_grammar_summary();

    return 0;
}
