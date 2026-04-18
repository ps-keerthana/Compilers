CC      = gcc
CFLAGS  = -w
# bison --yacc produces y.tab.c/y.tab.h (POSIX names the lexer expects).
# -Wno-yacc suppresses the POSIX-extension warning for %expect.
YACC    = bison --yacc -Wno-yacc

# ── Folders ──────────────────────────────────────────────────────────
TESTS   = tests
OUT     = output
ERROR_TESTS = $(sort $(wildcard $(TESTS)/test_error*.c))

# ── Default: build everything ─────────────────────────────────────────
all: dirs parser

# Create output and tests directories if they don't exist
dirs:
	mkdir -p $(OUT) $(TESTS)

# ── Final binary ──────────────────────────────────────────────────────
parser: lex.yy.c y.tab.c y.tab.h
	$(CC) $(CFLAGS) lex.yy.c y.tab.c -o parser

# ── Lexer ─────────────────────────────────────────────────────────────
lex.yy.c: cl_lexer.l y.tab.h
	flex cl_lexer.l

# ── Parser + automaton ────────────────────────────────────────────────
# -d  → y.tab.h  (token definitions)
# -v  → y.output (full LALR(1) automaton — Part 1 evidence)
y.tab.c y.tab.h: cl_parser.y
	$(YACC) -d -v cl_parser.y

# ─────────────────────────────────────────────────────────────────────
# IR Run targets — Part 1: Arithmetic & Assignment
# ─────────────────────────────────────────────────────────────────────
run_all: irall

# test1: Arithmetic expressions + assignment (spec example: a = b * -c + b * -c)
ir1: parser dirs
	@echo "==================================================="
	@echo "  test1.c — Arithmetic expressions & assignment"
	@echo "  (Includes spec example: a = b * -c + b * -c)"
	@echo "==================================================="
	./parser $(TESTS)/test1.c --no-tree > $(OUT)/ir1.txt 2>&1 || true
	@cat $(OUT)/ir1.txt
	@echo "  Saved → $(OUT)/ir1.txt"

# test2: Conditional statements (if / if-else)
ir2: parser dirs
	@echo "==================================================="
	@echo "  test2.c — If / If-Else conditional statements"
	@echo "==================================================="
	./parser $(TESTS)/test2.c --no-tree > $(OUT)/ir2.txt 2>&1 || true
	@cat $(OUT)/ir2.txt
	@echo "  Saved → $(OUT)/ir2.txt"

# test3: While and Do-While loops
ir3: parser dirs
	@echo "==================================================="
	@echo "  test3.c — While loop and Do-While loop"
	@echo "==================================================="
	./parser $(TESTS)/test3.c --no-tree > $(OUT)/ir3.txt 2>&1 || true
	@cat $(OUT)/ir3.txt
	@echo "  Saved → $(OUT)/ir3.txt"

# test4: For loop (C89 and C99)
ir4: parser dirs
	@echo "==================================================="
	@echo "  test4.c — For loop (C89 and C99 styles)"
	@echo "==================================================="
	./parser $(TESTS)/test4.c --no-tree > $(OUT)/ir4.txt 2>&1 || true
	@cat $(OUT)/ir4.txt
	@echo "  Saved → $(OUT)/ir4.txt"

# test5: Function definitions and calls
ir5: parser dirs
	@echo "==================================================="
	@echo "  test5.c — Functions: definitions and calls"
	@echo "==================================================="
	./parser $(TESTS)/test5.c --no-tree > $(OUT)/ir5.txt 2>&1 || true
	@cat $(OUT)/ir5.txt
	@echo "  Saved → $(OUT)/ir5.txt"

# test6: Logical and relational operators
ir6: parser dirs
	@echo "==================================================="
	@echo "  test6.c — Logical and relational operators"
	@echo "==================================================="
	./parser $(TESTS)/test6.c --no-tree > $(OUT)/ir6.txt 2>&1 || true
	@cat $(OUT)/ir6.txt
	@echo "  Saved → $(OUT)/ir6.txt"

# test7: Unary operators
ir7: parser dirs
	@echo "==================================================="
	@echo "  test7.c — Unary operators (minus, not, ++, --)"
	@echo "==================================================="
	./parser $(TESTS)/test7.c --no-tree > $(OUT)/ir7.txt 2>&1 || true
	@cat $(OUT)/ir7.txt
	@echo "  Saved → $(OUT)/ir7.txt"

# test8: Bitwise operators and compound assignments
ir8: parser dirs
	@echo "==================================================="
	@echo "  test8.c — Bitwise operators & compound assignment"
	@echo "==================================================="
	./parser $(TESTS)/test8.c --no-tree > $(OUT)/ir8.txt 2>&1 || true
	@cat $(OUT)/ir8.txt
	@echo "  Saved → $(OUT)/ir8.txt"

# test9: Pointers
ir9: parser dirs
	@echo "==================================================="
	@echo "  test9.c — Pointers (addr-of, deref, array index)"
	@echo "==================================================="
	./parser $(TESTS)/test9.c --no-tree > $(OUT)/ir9.txt 2>&1 || true
	@cat $(OUT)/ir9.txt
	@echo "  Saved → $(OUT)/ir9.txt"

# test10: Comprehensive
ir10: parser dirs
	@echo "==================================================="
	@echo "  test10.c — Comprehensive: functions + loops + if"
	@echo "==================================================="
	./parser $(TESTS)/test10.c --no-tree > $(OUT)/ir10.txt 2>&1 || true
	@cat $(OUT)/ir10.txt
	@echo "  Saved → $(OUT)/ir10.txt"

# ─────────────────────────────────────────────────────────────────────
# errorall — run all negative/error test cases discovered automatically
# Add new files as tests/test_error3.c, tests/test_error4.c, ...
# and they will be picked up without changing this makefile.
# ─────────────────────────────────────────────────────────────────────
errorall: parser dirs
	@echo ""
	@echo "=========================================================="
	@echo "  Part 4 — Error Handling Test Suite"
	@echo "=========================================================="
	@echo ""
	@if [ -z "$(ERROR_TESTS)" ]; then \
		echo "No error test files found matching $(TESTS)/test_error*.c"; \
		exit 0; \
	fi
	@count=1; \
	for f in $(ERROR_TESTS); do \
		name=$$(basename $$f .c); \
		echo "──────────────────────────────────────────────────────"; \
		echo "  [E$$count] Running $$name.c"; \
		echo "──────────────────────────────────────────────────────"; \
		./parser $$f --no-tree > $(OUT)/$$name.txt 2>&1 || true; \
		cat $(OUT)/$$name.txt; \
		echo ""; \
		count=$$((count+1)); \
	done
	@echo "=========================================================="
	@echo "  Error test suite complete. Output files saved to $(OUT)/"
	@echo "=========================================================="
	@ls -lh $(OUT)/test_error*.txt

# ─────────────────────────────────────────────────────────────────────
# irall — run only the 10 valid IR-generation tests
# ─────────────────────────────────────────────────────────────────────
irall: parser dirs
	@echo ""
	@echo "=========================================================="
	@echo "  CS327 Assignment 4 — All 10 IR Code Generation Examples"
	@echo "=========================================================="
	@echo ""
	@for i in 1 2 3 4 5 6 7 8 9 10; do \
		echo "──────────────────────────────────────────────────────"; \
		echo "  [$$i/10] Running test$$i.c"; \
		echo "──────────────────────────────────────────────────────"; \
		./parser $(TESTS)/test$$i.c --no-tree > $(OUT)/ir$$i.txt 2>&1 || true; \
		cat $(OUT)/ir$$i.txt; \
		echo ""; \
	done
	@echo "=========================================================="
	@echo "  All 10 tests complete. IR files saved to $(OUT)/"
	@echo "=========================================================="

# ─────────────────────────────────────────────────────────────────────
# A3-compatible targets (kept for backward compatibility)
# ─────────────────────────────────────────────────────────────────────
run1: parser dirs
	@echo "Running test1.c  →  output/test1.txt"
	./parser $(TESTS)/test1.c > $(OUT)/test1.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test1.txt"

run2: parser dirs
	@echo "Running test2.c  →  output/test2.txt"
	./parser $(TESTS)/test2.c > $(OUT)/test2.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test2.txt"

run3: parser dirs
	@echo "Running test3.c  →  output/test3.txt"
	./parser $(TESTS)/test3.c > $(OUT)/test3.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test3.txt"

run4: parser dirs
	@echo "Running test4.c  →  output/test4.txt"
	./parser $(TESTS)/test4.c > $(OUT)/test4.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test4.txt"

run5: parser dirs
	@echo "Running test5.c  →  output/test5.txt"
	./parser $(TESTS)/test5.c > $(OUT)/test5.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test5.txt"

# Parsing table demo
table: parser dirs
	@echo "Running test1.c --table  →  output/table.txt"
	./parser $(TESTS)/test1.c --table --no-tree --yout y.output > $(OUT)/table.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/table.txt"

# Full output (IR + tree + table + stats)
full: parser dirs
	@echo "Running test10.c (full)  →  output/full.txt"
	./parser $(TESTS)/test10.c --table --extra --yout y.output > $(OUT)/full.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/full.txt"

# ── Clean ─────────────────────────────────────────────────────────────
clean:
	rm -f lex.yy.c y.tab.c y.tab.h y.output parser
	rm -rf $(OUT)

cleanobj:
	rm -f lex.yy.c y.tab.c y.tab.h y.output
