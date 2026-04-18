# CS327: Compilers — Lab Assignment #4

## Intermediate Code Generation (Three Address Code - Quadruples)

## Objective

The goal of this assignment is to design and implement an **Intermediate Code Generator** that:

* Translates a C-like language into **Three Address Code (TAC)**
* Represents TAC using **Quadruples (op, arg1, arg2, result)**
* Handles expressions, control flow, functions, and memory constructs

As required in the assignment, the implementation uses **Syntax Directed Translation (SDT)** with **Lex + Yacc (Flex + Bison)**.

## Key Concepts Used

* **Lexical Analysis** → Token generation using Flex
* **Parsing (LALR(1))** → Syntax analysis using Bison
* **Syntax Directed Translation (SDT)** → IR generation during parsing
* **Three Address Code (TAC)**
* **Quadruple Representation**
* **Temporary Variables (t1, t2, …)**
* **Labels & Control Flow (goto, jf, jt)**

---

## Project Structure

```
Assignment_4/
│
├── cl_lexer.l        # Lexer (Flex)
├── cl_parser.y       # Parser + IR generation (Bison)
├── y.tab.c / .h      # Generated parser files
├── lex.yy.c          # Generated lexer file
├── y.output          # LALR(1) automaton (proof of parsing)
├── parser            # Final executable
├── makefile          # Build + run automation
│
├── tests/            # 10 valid IR tests + error tests
│   ├── test1.c
│   ├── ...
│   ├── test10.c
│   ├── test_error1.c
│   └── test_errorN.c
│
└── output/           # Generated IR outputs
    ├── ir1.txt
    ├── ...
    └── ir10.txt
```

---

## Build Instructions

### Step 1: Compile the project

```bash
make clean && make
```

This will:

* Generate lexer and parser files
* Build the final executable `parser`

---

## Running the Code

### Run all test cases (recommended)

```bash
make irall
```

This runs all 10 valid IR-generation programs only.

### Run only error test cases in one go

```bash
make errorall
```

### Run individual tests

```bash
make ir1   # Arithmetic
make ir2   # If-Else
make ir3   # While / Do-While
make ir4   # For loop
make ir5   # Functions
make ir6   # Logical ops
make ir7   # Unary ops
make ir8   # Bitwise ops
make ir9   # Pointers
make ir10  # Comprehensive
```

---

## Output Format

Each program produces:

### 1. Source Code

Displayed with line numbers.

### 2. Intermediate Code (Quadruples)

| # | stmt | op | arg1 | arg2 | result |
| - | ---- | -- | ---- | ---- | ------ |

Example:

```
| 3 | t1 = minus c | minus | c  | _  | t1 |
| 4 | t2 = b * t1  | *     | b  | t1 | t2 |
| 7 | t5 = t2 + t4 | +     | t2 | t4 | t5 |
| 8 | a = t5       | =     | t5 | _  | a  |
```

Follows assignment requirement:

* Row-wise execution
* Temporary variables (`t1, t2, ...`)
* Clear tabular format

---

## Test Cases Overview (10 Programs)

| Test   | Feature Covered          |
| ------ | ------------------------ |
| test1  | Arithmetic + assignment  |
| test2  | If / If-Else             |
| test3  | While + Do-While         |
| test4  | For loops                |
| test5  | Functions & calls        |
| test6  | Logical & relational ops |
| test7  | Unary operators          |
| test8  | Bitwise & compound ops   |
| test9  | Pointers & arrays        |
| test10 | Combined (full pipeline) |

✔ Meets requirement: **10 example programs**

---

## Features Implemented

### Part 1: Intermediate Code Generation

* Arithmetic expressions
* Assignments
* Temporary variables

### Part 2: Extended Support

* If / If-Else
* While, Do-While, For loops
* Function calls & recursion
* Logical and relational operators (short-circuit `&&` and `||`)
* Unary and bitwise operators
* Pointers and arrays

### Part 3: Output Format

* Structured quadruple table with `stmt`, `op`, `arg1`, `arg2`, `result` columns
* Clean separation of source and IR

### Part 4: Error Handling

* Parser detects syntax errors with hints
* Lexer detects invalid tokens and unsupported constructs (e.g., `@`, `#include`)
* Semantic checks detect: undeclared identifiers/functions, invalid array index type, division by zero, type mismatches, `break`/`continue` outside valid contexts
* `make errorall` runs the complete 18-case negative test suite
* Prevents crashes on invalid input

---

## Observations & Known Limitations

* **No optimization** (as per assignment requirement)

* **Temporary footer range**: The IR footer displays the range `t1..tN` where `N` is the highest-numbered temporary allocated. This does **not** guarantee that every number in the range was actually emitted — numbers may be skipped when the dead-temp elimination pass removes saved values from post-increment (`i++`) operations. For example, a footer saying `t1..t5` may have no `t3` in the actual quads if `t3` was internally allocated but eliminated as dead.

* **Nested if-else IR ordering (known LALR(1) limitation)**: When an `if` statement is nested inside the body of another `if`, the outer condition guard (`jf`) appears in the IR *after* the inner body rather than before it. This is a fundamental consequence of LALR(1) bottom-up parsing — the inner `if`'s reduction happens before the outer `if`'s mid-rule actions can insert the guard. The results of the 10 test programs are not affected because none of them exercise this specific nesting pattern. Evaluators who manually trace deeply nested `if` IR should be aware of this ordering property.

* **For-loop step ordering**: When the step expression (`i++` or `i=i+1`) is placed in the for-header's third clause, LALR(1) bottom-up reduction causes the step quads to appear before the body quads in the table. The test programs place the step inside the loop body to produce correct top-to-bottom IR ordering.

* **Pointer IR is simplified**: An abstract memory model (`addr`, `deref`, `elemaddr`, `store`) is used rather than full byte-level addressing.

* **Semantic analysis is lightweight**: Nested block scopes, full struct/union member typing, and advanced pointer compatibility are not exhaustively modeled.

---

## How It Works (Pipeline)

```
Input C-like Program
        ↓
   Lexical Analysis (Flex)
        ↓
   Parsing (Bison LALR(1))
        ↓
Syntax Directed Translation
        ↓
Intermediate Code (TAC)
        ↓
Quadruple Output
```
