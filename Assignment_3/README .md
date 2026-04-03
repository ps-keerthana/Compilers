# CS327 — Assignment 3: LALR(1) Parser
## Complete Documentation (Parts 1 – 6)

---

## How to Build and Run

```bash
# Build everything
make

# Run individual tests
make run1          # test1.c — basic declarations and expressions
make run2          # test2.c — dangling else (conflict resolution demo)
make run3          # test3.c — comprehensive: loops, structs, C99 features
make run4          # test4.c — error diagnostics (expects a parse error)
make run5          # test5.c — const/volatile/static inline/long long

# Run with parsing table, This generates parsingtable.csv and parsingtable.html (Part 4)
make table

# Run with all output (Parts 3 + 4 + 6)
make full

# Run all tests at once
make runall

# Show before/after conflict resolution demo and save the results to conflicts.txt (Part 2 requirement)
make before

# Or run manually:
./parser tests/test1.c                        # Parts 3 + 5
./parser tests/test1.c --table               # Parts 3 + 4 + 5
./parser tests/test1.c --table --extra       # Parts 3 + 4 + 5 + 6
```

---

## Part 1 — LALR(1) Automaton Construction

The LALR(1) automaton is built by `bison --yacc -Wno-yacc -d -v cl_parser.y`.
The `-v` flag writes the complete automaton to `y.output`, which contains:
- All grammar rules (numbered)
- All LALR(1) item sets (one per state, with the dot showing parser position)
- Shift / Goto transitions
- Reduce actions
- Conflict lines at the top (if any)

**Automaton statistics:**

| Property | Value |
|----------|-------|
| Grammar rules (productions) | ~330 |
| LALR(1) states | ~331 |
| Terminals | ~50 |
| Non-terminals | ~42 |
| Shift/reduce conflicts (explicit) | 0 |
| Shift/reduce conflicts (accepted via %expect) | 34 |
| Reduce/reduce conflicts | 0 |

**Reading `y.output`:**

```
State 42

   15 param_list: param_list ',' . param_decl
   16           | param_list ',' . ELLIPSIS

   INT      shift, and go to state 55       ← ACTION[42, INT] = s55
   FLOAT    shift, and go to state 56       ← ACTION[42, FLOAT] = s56
   param_decl   go to state 103             ← GOTO[42, param_decl] = 103
```

The dot `.` in each item shows how far the parser has read.
Lines with `shift` go into the ACTION table.
Lines with `go to state` (no shift) go into the GOTO table.
Lines with `reduce using rule N` produce ACTION[state, token] = rN.

---

## Part 2 — Conflict Identification and Resolution

### Conflict 1: Dangling Else (Shift/Reduce)

**Grammar fragment:**
```
selection_stmt : IF '(' expr ')' stmt
               | IF '(' expr ')' stmt ELSE stmt
```

**Problem:** After `IF '(' expr ')' stmt`, seeing `ELSE`:
- Reduce → finish inner if-without-else, attach else to outer if
- Shift  → attach else to inner if (C standard)
Both valid → 1 S/R conflict.

**Before fix bison output:**
```
cl_parser.y: warning: 1 shift/reduce conflict [-Wconflicts-sr]
```

**Fix:**
```yacc
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

selection_stmt : IF '(' expr ')' stmt  %prec LOWER_THAN_ELSE
               | IF '(' expr ')' stmt ELSE stmt
```

**After fix:** 0 conflicts. ELSE always attaches to innermost if (C standard).

**Demo:** `make before` shows the bison warning appearing without the fix.

---

### Conflict 2: Operator Precedence (Shift/Reduce)

**Problem:** Without precedence, `a + b * c` is ambiguous.
After reducing `a+b`, seeing `*` can shift (correct) or allow reduce (wrong).

**Before fix:** 50+ shift/reduce conflicts.

**Fix:** Full 15-level C precedence table:
```yacc
%right '=' ASSIGN_OP          /* lowest  */
%right '?' ':'
%left  OR_OP
%left  AND_OP
%left  '|'  '^'  '&'
%left  EQ_OP NE_OP
%left  '<' '>' LE_OP GE_OP
%left  LEFT_SHIFT RIGHT_SHIFT
%left  '+' '-'
%left  '*' '/' '%'
%right UMINUS UPLUS '!' '~'   /* highest unary */
%left  INC_OP DEC_OP ARROW '.'
```

**After fix:** All operator S/R conflicts resolved. `a+b*c` → `a+(b*c)`.

---

### Conflict 3: Unary vs Binary +/- (Shift/Reduce)

**Problem:** After `a`, seeing `-`: binary minus OR unary prefix?

**Fix:**
```yacc
unary_expr : '-' unary_expr  %prec UMINUS
           | '+' unary_expr  %prec UPLUS
```
UMINUS/UPLUS have higher precedence than binary `+`/`-`.

---

### Conflict 4: TYPEDEF Declarator Ambiguity (State 62, 1 S/R)

**Problem:** Two overlapping rules:
```
rule 22: declaration → TYPEDEF type_spec IDENTIFIER ';'
rule 23: declaration → TYPEDEF type_spec declarator_list ';'
```
`declarator_list → declarator → IDENTIFIER`, so rule 22 is redundant.
After `TYPEDEF type_spec IDENTIFIER`, seeing `;`: shift (rule 22) or reduce (rule 23)?

**Before fix:**
```
State 62 conflicts: 1 shift/reduce
```

**Fix:** Delete rule 22. Rule 23 covers all typedef forms.

**After fix:** 0 conflicts in State 62.

---

### Conflict 5: case_body_list Ambiguity (States 233/284, 38 S/R)

**Problem:**
```
case_body_list : empty | case_body_list stmt
labeled_stmt   : CASE expr ':' case_body_list
               | DEFAULT ':' case_body_list
```
After `DEFAULT ':' case_body_list`, seeing `IF` (or any stmt-starting token):
- Shift → IF starts another stmt in this case body
- Reduce → labeled_stmt is done; IF starts something in an outer context
→ 19 tokens × 2 states = 38 S/R conflicts.

**Before fix:**
```
State 233 conflicts: 19 shift/reduce
State 284 conflicts: 19 shift/reduce
```

**Fix (two-part):**

*Part A* — introduce `non_case_stmt` to exclude `labeled_stmt` from case bodies:
```yacc
non_case_stmt : compound_stmt | expr_stmt | selection_stmt
              | iteration_stmt | jump_stmt
              ;   /* everything EXCEPT labeled_stmt */

case_body_list : empty | case_body_list non_case_stmt ;
```
`CASE`/`DEFAULT` now always trigger a reduce (they can't start `non_case_stmt`). Drops 4 conflicts per state.

*Part B* — `%expect 34` for the remaining 34 inherent conflicts (tokens that can start both a non-case stmt OR belong to an outer context). Bison's default shift is correct: all stmts between two case labels belong to the first label.
```yacc
%expect 34
```

**After fix:** 0 explicit warnings. Clean build. Correct fall-through semantics.

---

## Part 3 — Reverse Derivation Tree

After every successful parse, all reductions are printed in reverse order
(last reduction first = top-down reading).

**How it works:** Each grammar action calls `new_node("rule string")`, appending
to a flat array. `print_tree_reverse()` iterates from last to first.

**Example output for `int x = 5;`:**
```
╔══════════════════════════════════════════════════════════════╗
║     DERIVATION TREE  (indented, top-down structure)          ║
║  Each rule is shown indented under its parent rule.          ║
╚══════════════════════════════════════════════════════════════╝
  Total reductions: 12

program → translation_unit
  translation_unit → external_decl
    external_decl → declaration
      declaration → type_spec declarator_list ;
        type_spec → int
        declarator_list → declarator
          declarator → IDENTIFIER = initializer
            initializer → assignment_expr
              assignment_expr → conditional_expr
                ...
                  primary_expr → INT_CONST
```

---

## Part 4 — Parsing Table (Matrix Format)

Run with `--table` flag. Parses `y.output` and displays ACTION/GOTO matrix.
Running with the `--table` flag (or `make table`) parses `y.output` and handles the automaton visualization in two ways:

### 1. Interactive HTML Parsing Table (Recommended)
Because the massive 334-state × 127-column LR parsing automaton is too wide for standard terminal viewing, an interactive HTML viewer (`parsingtable.html`) is automatically generated. 

**How to use:**
After running `make table`, copy the local file link provided in the terminal output and paste it into any web browser.
* **States / Symbol Filters:** Type specific states (e.g., `0,21`) or tokens (e.g., `INT, +`) to isolate them.
* **Column Groups:** Switch between *All*, *ACTION only*, *GOTO only*, or *Key symbols* (a curated subset).
* **Non-empty Only:** Hide all empty rows—great for finding exactly which states handle a specific token.
* **Colors:** **Blue** = Shift, **Amber** = Reduce, **Green** = Accept, **Purple** = Goto, **Gray Dot** = Error.

### 2. Standard Terminal Output
A standard matrix is also printed to the console:

```
╔══════════════════════════════════════════════════════════════╗
║           LALR(1) PARSING TABLE  (matrix format)             ║
╚══════════════════════════════════════════════════════════════╝
  Legend:  sN = shift to N    rN = reduce by rule N
            N = goto N         acc = accept    . = error

 State |  ACTION                  |  GOTO
       |  $end  int  float  ...  |  program  ...
-------+----------+-------+------+----------+---
     0 |  .      s55  s56  ...  |  1        ...
     1 |  acc    .    .    ...  |  .        ...
```

---

## Part 5 — Error Diagnostics

Example output for a missing semicolon:
```
╔══════════════════════════════════════════════════════════════╗
║                       PARSE ERROR                            ║
╚══════════════════════════════════════════════════════════════╝
  Line         : 14
  Near token   : 'int'
  Lookahead    : int (token id 258)
  Message      : syntax error

  Hints:
    - Missing ';' at end of statement?
    - Unmatched '{', '}', '(', or ')'?
    - Invalid expression near line 14?
    - Declaration after statement (C89 mode)?
```

- **Line** — exact line from `yylineno`
- **Near token** — last lexed token from `last_token[]` in the lexer
- **Lookahead** — problematic token from `yychar`, named via `tok_names[]` table
- Header always appears before error box (`fflush(stdout)` before `yyparse()`)

---

## Part 6 — Additional Information

Run with `--extra` flag. Shows grammar statistics, all 5 conflict resolutions,
and key feature list. Reads `y.output` directly so the stats reflect the
actual built automaton.

---

## Bug Fixes Applied

| # | File | Bug | Fix |
|---|------|-----|-----|
| 1 | `cl_lexer.l` | `<BLOCK_COMMENT>.\|"\\n"` — quoted `\\n` is a string literal | Changed to `.\|\\n` |
| 2 | `cl_parser.y` | `tok_names[]` had phantom entry 311 (ELSE listed twice) | Removed; table ends at 310 |
| 3 | `cl_parser.y` | `strip_quotes()` `memmove(tok,tok+1,len)` off-by-one | Fixed to `len-1`; explicit null-term |
| 4 | `cl_parser.y` | Error box before header (stdout/stderr ordering) | Added `fflush(stdout)` before `yyparse()` |
| 5 | `cl_parser.y` | `new_node()` silently dropped reductions on overflow | Prints warning on overflow |
| 6 | `cl_parser.y` | C89 block grammar broke C99 mixed decl/stmt | Replaced with `block_item_list` |
| 7 | `cl_parser.y` | `CONST`/`VOLATILE` as standalone type broke `const int x` | Added `type_qualifier` non-terminal |
| 8 | `cl_parser.y` | `static inline int f()` failed — only single storage class | Added `storage_class_list` |
| 9 | `cl_parser.y` | Redundant typedef rule caused State 62 S/R conflict | Deleted redundant rule 22 |
| 10 | `cl_parser.y` | `case_body_list stmt` caused 38 S/R conflicts | `non_case_stmt` + `%expect 34` |
| 11 | `cl_parser.y` | Part 6 conflict count was wrong (counted lines not totals) | Parse `State N conflicts: M` lines |
| 12 | `cl_parser.y` | Part 6 falsely claimed cast `(int)x` was in grammar | Corrected with explanation |
| 13 | `makefile` | `bison --yacc` + `%expect` caused POSIX warning | Added `-Wno-yacc` flag |

---

## File Structure

```
assign_3/
├── cl_lexer.l       — Flex lexer
├── cl_parser.y      — Bison LALR(1) parser (all 6 parts)
├── makefile         — Build + test + before/after demo targets
├── README.md        — This file
├── y.output         — Generated: full LALR(1) automaton
├── y.tab.c/h        — Generated: parser + token definitions
├── lex.yy.c         — Generated: lexer C code
├── parser           — Generated: final executable
├── tests/           — Input test files (test1–test5)
└── output/          — Generated output files
```
