# C-Lite Compiler

> A complete compiler frontend for a **C-like language** built from scratch using **Flex** and **Bison**, developed across four lab assignments for **CS327: Compilers** (IIT Gandhinagar, Jan–May 2026).

The compiler pipeline is implemented end-to-end — from raw source text all the way to **Three Address Code (TAC) quadruples** — covering every classical frontend phase: tokenization, lexical analysis, LALR(1) parsing, and intermediate code generation via Syntax Directed Translation.

---

## Pipeline at a Glance

```
Source Program (.c)
        │
        ▼
┌─────────────────────┐
│  Lexical Analysis   │  ← Flex  (cl_lexer.l)
│  Tokenizer          │
└────────┬────────────┘
         │  token stream
         ▼
┌─────────────────────┐
│  LALR(1) Parsing    │  ← Bison  (cl_parser.y)
│  Syntax Analysis    │
└────────┬────────────┘
         │  parse tree reductions
         ▼
┌─────────────────────┐
│  Syntax Directed    │  ← Grammar actions in cl_parser.y
│  Translation (SDT)  │
└────────┬────────────┘
         │  quadruple table
         ▼
┌─────────────────────┐
│  TAC IR Output      │  ← (op, arg1, arg2, result)
│  Quadruple Format   │
└─────────────────────┘
```

---

## Repository Structure

```
Compilers/
│
├── Compiler_foundations/          # Assignment 1 — Compiler Foundations
│   └── A1_23110229/
│       ├── Part1/                 # GCC pipeline stages (hello.c → .i → .s → .o → binary)
│       ├── Part2/                 # O0 vs O2 optimisation analysis (optimize.c)
│       └── Part3/                 # Hand-written tokenizer in C (tokenizer.c)
│
├── Lexical_analyzer/              # Assignment 2 — Lexical Analysis
│   └── lexical_analyzer/
│       ├── cl_lexer.l             # Full Flex lexer for C-Lite
│       ├── cl_parser.y            # Skeleton Bison parser (token declarations)
│       ├── tests/                 # test1–test4.c  (valid + error cases)
│       ├── edgecases/             # edge1–edge25.c (25 targeted edge cases)
│       └── makefile
│
├── LALR(1) Parser/                # Assignment 3 — Full LALR(1) Parser
│   └── Assignment_3/
│       ├── cl_lexer.l             # Lexer (carries forward from A2)
│       ├── cl_parser.y            # Full grammar + conflict resolution + Parts 1–6
│       ├── tests/                 # test1–test5.c
│       ├── output/                # Parsing table CSV, HTML viewer, derivation tree outputs
│       └── makefile
│
└── C-Lite_IRgen/                  # Assignment 4 — Intermediate Code Generation
    └── A4_C327_GROUP2/
        ├── cl_lexer.l             # Lexer (carries forward from A3)
        ├── cl_parser.y            # Parser + SDT + IR engine + semantic analysis
        ├── tests/                 # test1–test10.c  +  test_error1–18.c
        ├── output/                # ir1–ir10.txt  +  test_error1–18.txt
        └── makefile
```

---

## Assignment Breakdown

### A1 — Compiler Foundations

**Goal:** Understand the classical GCC compilation pipeline and write a basic tokenizer from scratch in C (no libraries).

**What was done:**
- **Part 1:** Compiled a `hello.c` program through each GCC stage (`-E`, `-S`, `-c`, link) and analysed the differences between `hello.c`, `hello.i`, `hello.s`, `hello.o`, and the final binary.
- **Part 2:** Compared `-O0` vs `-O2` assembly output for an arithmetic expression. `-O2` collapsed all operations to a single constant (`movl $9`) via constant folding — zero arithmetic instructions at runtime.
- **Part 3:** Implemented a hand-written tokenizer in C from scratch. Recognises `KEYWORD`, `IDENTIFIER`, `NUMBER`, `OPERATOR`, `PUNCTUATION`, and `UNKNOWN` tokens. No external libraries used.

**Sample output (Part 3):**
```
Input:  int x = 10 + 20;
Output: <KEYWORD, int>  <IDENTIFIER, x>  <OPERATOR, =>
        <NUMBER, 10>    <OPERATOR, +>     <NUMBER, 20>  <PUNCTUATION, ;>
```

**Run:**
```bash
gcc tokenizer.c -o tokenizer
./tokenizer < public_test1.txt
```

---

### A2 — Lexical Analysis (C-Lite)

**Goal:** Build a complete, robust lexer for a C-like language using Flex.

**What was done:**
- Full token recognition: all 30+ C keywords, identifiers, integer/float/hex constants, character and string literals, all operators (multi-character longest-match), punctuation.
- Block comment handling via a Flex start condition (`BLOCK_COMMENT`), including unterminated-comment detection.
- Preprocessor directive rejection (`#include` → immediate lexer error).
- Line number tracking (`yylineno`) for precise error reporting.
- 4 main test cases + **25 dedicated edge cases** covering hex literals, escape sequences, nested comments, multi-level pointers, ellipsis (`...`), all operator combinations, and more.

**Sample output:**
```
Line 1    KEYWORD            int
Line 1    IDENTIFIER         main
Line 1    PUNCTUATION        (
...
***parsing successful***
```

**Run:**
```bash
cd Lexical_analyzer/lexical_analyzer
make clean && make
./parser test1.c          # valid program
./parser test2.c          # invalid token @ → lexer error
./parser test3.c          # unclosed /* comment → lexer error
```

---

### A3 — LALR(1) Parser

**Goal:** Build a complete LALR(1) parser for the C-Lite language, resolve all grammar conflicts, and produce a derivation tree, parsing table, and detailed error diagnostics.

**What was done (6 parts):**

| Part | Description |
|------|-------------|
| 1 | LALR(1) automaton construction via Bison (`y.output` — ~330 rules, ~331 states) |
| 2 | Identification and resolution of all 5 grammar conflict categories |
| 3 | Reverse derivation tree printed after every successful parse |
| 4 | Full ACTION/GOTO parsing table — terminal output + interactive HTML viewer |
| 5 | Detailed error diagnostics with line number, offending token, and hints |
| 6 | Grammar statistics (rule count, states, conflict summary) via `--extra` flag |

**Conflicts resolved:**

| Conflict | Type | Resolution |
|----------|------|------------|
| Dangling `else` | S/R | `%prec LOWER_THAN_ELSE` |
| Operator precedence (`a+b*c`) | S/R (50+) | 15-level `%left`/`%right` table |
| Unary vs binary `+`/`-` | S/R | `%prec UMINUS` / `UPLUS` |
| TYPEDEF declarator redundancy (State 62) | S/R | Deleted redundant rule |
| `case_body_list` ambiguity (States 233/284) | S/R (38) | `non_case_stmt` + `%expect 34` |

**Run:**
```bash
cd "LALR(1) Parser/Assignment_3"
make clean && make
make run1            # basic declarations and expressions
make run2            # dangling else demo
make table           # generates parsing table + HTML viewer
make full            # everything: tree + table + stats
make runall          # all 5 test cases
./parser tests/test4.c   # parse error demo
```

**Error output example:**
```
+--------------------------------------------------------------+
|                       PARSE ERROR                            |
+--------------------------------------------------------------+
  Line         : 14
  Near token   : 'int'
  Lookahead    : int (token id 258)
  Message      : syntax error
  Hints:
    - Missing ';' at end of statement?
    - Unmatched '{', '}', '(', or ')'?
```

---

### A4 — Intermediate Code Generation (C-Lite IR)

**Goal:** Implement a complete IR code generator that translates C-Lite source programs into Three Address Code (TAC) in Quadruple format using Syntax Directed Translation.

**Quadruple format:**
```
| # | stmt          | op    | arg1 | arg2 | result |
|---|---------------|-------|------|------|--------|
| 3 | t1 = minus c  | minus | c    | _    | t1     |
| 4 | t2 = b * t1   | *     | b    | t1   | t2     |
| 7 | t5 = t2 + t4  | +     | t2   | t4   | t5     |
| 8 | a = t5        | =     | t5   | _    | a      |
```
*(Spec example: `a = b * -c + b * -c` — matches assignment exactly)*

**Constructs generating IR:**

| Construct | IR Strategy |
|-----------|-------------|
| Arithmetic / assignment | Direct temp allocation + emit |
| `if` | `jf cond _ Lend` + `label Lend` |
| `if-else` | `jf` + `goto` + two labels |
| `while` | `label Lstart` + `jf` + `goto Lstart` |
| `do-while` | `label Lstart` + body + `jt cond Lstart` |
| `for` | init + `label` + `jf` + body + step + `goto` |
| `&&` / `\|\|` | Short-circuit via mid-rule Bison actions |
| Function call | `param` quads + `call fn, N, result` |
| Recursion | Same `param + call` mechanism |
| Pointers | `addr`, `deref`, `store`, `elemaddr` |
| `break` / `continue` | Patched to `goto Lend` / `goto Lstart` |

**10 test programs:**

| Test | Feature | Quads | Labels |
|------|---------|-------|--------|
| test1 | Arithmetic + assignment (spec example) | 16 | none |
| test2 | if / if-else | 31 | L0..L5 |
| test3 | while + do-while | 24 | L0..L2 |
| test4 | for loop (C89 + C99) | 22 | L0..L3 |
| test5 | Functions + nested calls | 26 | none |
| test6 | Logical / relational + short-circuit | 49 | L0..L7 |
| test7 | Unary operators | 26 | none |
| test8 | Bitwise + compound assignments | 28 | none |
| test9 | Pointers + arrays | 26 | none |
| test10 | Full pipeline (recursive + loops + if) | 53 | L0..L6 |

**Error handling — 18 negative test cases:**

| Category | Count | Examples |
|----------|-------|---------|
| Lexer errors | 3 | Invalid token `@`, `#include`, unterminated string |
| Parse errors | 4 | Missing `;`, unmatched `(`, incomplete expression |
| Semantic errors | 11 | Undeclared identifier, division by zero, type mismatch, `continue` outside loop |

**Run:**
```bash
cd C-Lite_IRgen/A4_C327_GROUP2
make clean && make
make irall        # run all 10 IR generation tests
make errorall     # run all 18 error tests
make ir1          # arithmetic
make ir5          # functions
make ir10         # comprehensive
./parser tests/test1.c --no-tree   # run any file directly
```

**Sample IR output:**
```
+============================================================+
| SOURCE PROGRAM                                             |
+============================================================+
   4 | int main() {
   5 |     int a;
   6 |     int b = 5;
   ...

+============================================================+
| INTERMEDIATE CODE  (Three Address Code -- Quadruples)      |
+============================================================+
+-----+------------------+-------+------+------+--------+
| #   | stmt             | op    | arg1 | arg2 | result |
+-----+------------------+-------+------+------+--------+
| 3   | t1 = minus c     | minus | c    | _    | t1     |
| 4   | t2 = b * t1      | *     | b    | t1   | t2     |
...
  Total quads: 16    Temporaries: t1..t7    Labels: (none)
```

---

## Tech Stack

| Tool | Purpose |
|------|---------|
| **Flex** | Lexical analyser generator |
| **Bison** | LALR(1) parser generator |
| **GCC** | Compilation of generated C code |
| **C** | Implementation language throughout |
| **Make** | Build automation |

**Platform:** Linux / Unix (Ubuntu 24.04 tested). WSL works on Windows.

---

## Quick Start (A4 — the final compiler)

```bash
# Clone and navigate
git clone <repo-url>
cd Compilers/C-Lite_IRgen/A4_C327_GROUP2

# Build
make clean && make

# Run the spec example (a = b * -c + b * -c)
make ir1

# Run all 10 IR examples
make irall

# Run all error tests
make errorall

# Run your own C-Lite program
./parser myprogram.c --no-tree
```

---

## Language Supported (C-Lite)

The compiler handles a substantial subset of C:

- **Types:** `int`, `float`, `char`, `void`, `double`, `short`, `long`, `unsigned`, `signed`
- **Declarations:** variables, arrays, pointers (`*`, `**`), function prototypes
- **Expressions:** full arithmetic, bitwise, logical, relational, ternary, comma
- **Control flow:** `if`, `if-else`, `while`, `do-while`, `for` (C89 + C99 init), `switch/case`, `break`, `continue`
- **Functions:** definitions, calls, recursion, parameters, return values
- **Operators:** all C operators including `++`/`--` (prefix + postfix), `+=`/`-=`/`*=` etc., `<<`, `>>`
- **Literals:** integer, float, hex (`0x`), character (`'A'`), string (`"hello"`)
- **Comments:** `//` single-line and `/* */` block comments
- **Not supported:** `#include` / preprocessor (rejected by design), `goto` labels, `struct` member IR

---

## Known Limitations (A4)

- **For-loop step ordering:** When the step is in the `for(;;step)` header position, LALR(1) bottom-up reduction causes step quads to appear before body quads. Test programs place the step inside the loop body to produce correct IR ordering.
- **Nested `if` IR structure:** In deeply nested `if` statements, the outer `jf` guard can appear after the inner body due to LALR(1) bottom-up reduction order. The 10 test programs are unaffected.
- **Pointer model:** Abstract (`addr`/`deref`/`store`/`elemaddr`) rather than byte-level addressing.
- **No optimisation** — as required by the assignment spec.

---

## Course

**CS327: Compilers** — IIT Gandhinagar  
Instructor: Shouvick Mondal  
Semester: January – May 2026
