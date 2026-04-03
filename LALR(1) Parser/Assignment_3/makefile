CC      = gcc
CFLAGS  = -w
# bison --yacc produces y.tab.c/y.tab.h (POSIX names the lexer expects).
# We add -Wno-yacc to suppress the POSIX-extension warning for %expect.
YACC    = bison --yacc -Wno-yacc

# ── Folders ──────────────────────────────────────────────────────────
TESTS   = tests
OUT     = output

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
# Run targets — each saves output to output/<name>.txt
# View any result with:  cat output/test1.txt
# ─────────────────────────────────────────────────────────────────────

# Part 3 — reverse derivation tree
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

# Part 5 — error diagnostics (test4 is supposed to fail)
run4: parser dirs
	@echo "Running test4.c  →  output/test4.txt  (expect error)"
	./parser $(TESTS)/test4.c > $(OUT)/test4.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test4.txt"

run5: parser dirs
	@echo "Running test5.c  →  output/test5.txt"
	./parser $(TESTS)/test5.c > $(OUT)/test5.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/test5.txt"

# Part 4 — parsing table
table: parser dirs
	@echo "Running test1.c --table  →  output/table.txt"
	./parser $(TESTS)/test1.c --table --yout y.output > $(OUT)/table.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/table.txt"

# Parts 3 + 4 + 6 together — full demo output
full: parser dirs
	@echo "Running test3.c --table --extra  →  output/full.txt"
	./parser $(TESTS)/test3.c --table --extra --yout y.output > $(OUT)/full.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/full.txt"

# Part 6 — additional information
extra: parser dirs
	@echo "Running test3.c --extra  →  output/extra.txt"
	./parser $(TESTS)/test3.c --extra --yout y.output > $(OUT)/extra.txt 2>&1 || true
	@echo "Done. View with:  cat $(OUT)/extra.txt"

# ─────────────────────────────────────────────────────────────────────
# runall — run every test and save each to its own file
# ─────────────────────────────────────────────────────────────────────
runall: parser dirs
	@echo "=========================================="
	@echo "  Running all tests → output/ folder"
	@echo "=========================================="
	@echo ""
	@echo "[1/6] test1.c  (basic declarations)"
	@./parser $(TESTS)/test1.c > $(OUT)/test1.txt 2>&1 || true
	@echo "      saved → $(OUT)/test1.txt"
	@echo ""
	@echo "[2/6] test2.c  (dangling-else conflict demo)"
	@./parser $(TESTS)/test2.c > $(OUT)/test2.txt 2>&1 || true
	@echo "      saved → $(OUT)/test2.txt"
	@echo ""
	@echo "[3/6] test3.c  (comprehensive)"
	@./parser $(TESTS)/test3.c > $(OUT)/test3.txt 2>&1 || true
	@echo "      saved → $(OUT)/test3.txt"
	@echo ""
	@echo "[4/6] test4.c  (error diagnostics — expects failure)"
	@./parser $(TESTS)/test4.c > $(OUT)/test4.txt 2>&1 || true
	@echo "      saved → $(OUT)/test4.txt"
	@echo ""
	@echo "[5/6] test5.c  (const/volatile/unsigned features)"
	@./parser $(TESTS)/test5.c > $(OUT)/test5.txt 2>&1 || true
	@echo "      saved → $(OUT)/test5.txt"
	@echo ""
	@echo "[6/6] test1.c --table  (parsing table)"
	@./parser $(TESTS)/test1.c --table --yout y.output > $(OUT)/table.txt 2>&1 || true
	@echo "      saved → $(OUT)/table.txt"
	@echo ""
	@echo "[+]   test3.c --table --extra  (full output)"
	@./parser $(TESTS)/test3.c --table --extra --yout y.output > $(OUT)/full.txt 2>&1 || true
	@echo "      saved → $(OUT)/full.txt"
	@echo ""
	@echo "=========================================="
	@echo "  All done. Files in output/ folder:"
	@echo "=========================================="
	@ls -lh $(OUT)/
	@echo ""
	@echo "  Quick view:  cat output/test1.txt"
	@echo "  Scroll view: less output/full.txt"

# ─────────────────────────────────────────────────────────────────────
# before — Part 2: full before/after conflict resolution demo
# Saves ALL conflict demos + all test case parse results to
# output/conflicts.txt  (also prints to terminal)
# ─────────────────────────────────────────────────────────────────────
before: parser dirs
	@OUT_FILE=$(OUT)/conflicts.txt; \
	: > $$OUT_FILE; \
	P() { echo "$$*" | tee -a $$OUT_FILE; }; \
	\
	P ""; \
	P "══════════════════════════════════════════════════════════════════"; \
	P "  PART 2 — Before/After Conflict Resolution Demo"; \
	P "  Saved to: output/conflicts.txt"; \
	P "══════════════════════════════════════════════════════════════════"; \
	P ""; \
	\
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  CONFLICT 1 — Dangling Else  (Shift/Reduce)"; \
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  Rule:  selection_stmt : IF '(' expr ')' stmt"; \
	P "                        | IF '(' expr ')' stmt ELSE stmt"; \
	P "  Problem: After seeing stmt, ELSE can shift (attach to inner IF)"; \
	P "           or reduce (attach to outer IF) — ambiguous."; \
	P ""; \
	P "── BEFORE (no %%prec LOWER_THAN_ELSE, no %%nonassoc directives) ──"; \
	sed 's/%prec LOWER_THAN_ELSE//' cl_parser.y \
	  | sed '/^%nonassoc LOWER_THAN_ELSE/d' \
	  | sed '/^%nonassoc ELSE/d' \
	  | sed 's/%expect 34/%expect 0/' \
	  > /tmp/cl_c1_before.y; \
	bison --yacc -d /tmp/cl_c1_before.y -o /tmp/cl_c1_before_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE || true; \
	P ""; \
	P "── AFTER (%%nonassoc LOWER_THAN_ELSE / %%nonassoc ELSE + %%prec) ──"; \
	bison --yacc -Wno-yacc -d cl_parser.y -o /tmp/cl_after_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE \
	  || echo "  (no warnings — conflict resolved)" | tee -a $$OUT_FILE; \
	P ""; \
	P "── Parse demo: test2.c (nested if/else — dangling else input) ──"; \
	P "  Source:  if (a) if (b) x=1; else x=2;"; \
	P "  Expect:  else attaches to INNER if  (C standard)"; \
	./parser $(TESTS)/test2.c > $(OUT)/test2.txt 2>&1 || true; \
	RESULT=$$(grep -E "successful|PARSE ERROR" $(OUT)/test2.txt | head -1); \
	P "  Result:  $$RESULT"; \
	P "  Full tree → output/test2.txt"; \
	P ""; \
	\
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  CONFLICT 2 — Operator Precedence  (Shift/Reduce, 50+ conflicts)"; \
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  Problem: Without precedence rules, a+b*c is ambiguous."; \
	P "           After reducing a+b, seeing * can shift (correct)"; \
	P "           or reduce (wrong) — 50+ S/R conflicts."; \
	P ""; \
	P "── BEFORE (no %%left/%%right precedence declarations) ──────────"; \
	grep -v '^%right\|^%left\|^%nonassoc\|^%expect' cl_parser.y \
	  | sed 's/%prec [A-Z_]*//g' \
	  > /tmp/cl_c2_before.y; \
	bison --yacc -d /tmp/cl_c2_before.y -o /tmp/cl_c2_before_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE || true; \
	P ""; \
	P "── AFTER (full 15-level C precedence table) ─────────────────────"; \
	P "  %%right '=' ASSIGN_OP   ← lowest"; \
	P "  %%right '?' ':'"; \
	P "  %%left  OR_OP  AND_OP  '|'  '^'  '&'"; \
	P "  %%left  EQ_OP NE_OP  '<' '>' LE_OP GE_OP"; \
	P "  %%left  LEFT_SHIFT RIGHT_SHIFT"; \
	P "  %%left  '+' '-'"; \
	P "  %%left  '*' '/' '%%'"; \
	P "  %%right UMINUS UPLUS '!' '~'  ← highest unary"; \
	P "  %%left  INC_OP DEC_OP ARROW '.'"; \
	bison --yacc -Wno-yacc -d cl_parser.y -o /tmp/cl_after_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE \
	  || echo "  (no warnings — all operator conflicts resolved)" | tee -a $$OUT_FILE; \
	P ""; \
	P "── Parse demo: test1.c (uses a+b, a-1, result>10) ─────────────"; \
	P "  Source:  return a + b;  /  if (result > 10)"; \
	P "  Expect:  correct precedence — a+(b*c) not (a+b)*c"; \
	./parser $(TESTS)/test1.c > $(OUT)/test1.txt 2>&1 || true; \
	RESULT=$$(grep -E "successful|PARSE ERROR" $(OUT)/test1.txt | head -1); \
	P "  Result:  $$RESULT"; \
	P "  Full tree → output/test1.txt"; \
	P ""; \
	\
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  CONFLICT 3 — Unary vs Binary +/-  (Shift/Reduce)"; \
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  Problem: After 'a', seeing '-' could be binary minus (a-b)"; \
	P "           or unary prefix of next expr (-b) — ambiguous."; \
	P "  Fix:     unary_expr : '-' unary_expr  %%prec UMINUS"; \
	P "           UMINUS/UPLUS have higher precedence than binary +/-"; \
	P ""; \
	P "── AFTER (%%prec UMINUS / UPLUS on unary rules) ─────────────────"; \
	bison --yacc -Wno-yacc -d cl_parser.y -o /tmp/cl_after_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE \
	  || echo "  (no warnings — unary/binary conflict resolved)" | tee -a $$OUT_FILE; \
	P ""; \
	P "── Parse demo: test3.c (uses unary minus, bitwise ops) ─────────"; \
	P "  Source:  -x,  ~flags,  a | b,  x << 2"; \
	./parser $(TESTS)/test3.c > $(OUT)/test3.txt 2>&1 || true; \
	RESULT=$$(grep -E "successful|PARSE ERROR" $(OUT)/test3.txt | head -1); \
	P "  Result:  $$RESULT"; \
	P "  Full tree → output/test3.txt"; \
	P ""; \
	\
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  CONFLICT 4 — TYPEDEF Declarator Ambiguity  (State 62, 1 S/R)"; \
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  Problem: Two overlapping rules:"; \
	P "    rule 22: declaration -> TYPEDEF type_spec IDENTIFIER ';'"; \
	P "    rule 23: declaration -> TYPEDEF type_spec declarator_list ';'"; \
	P "    declarator_list -> declarator -> IDENTIFIER  (rule 22 redundant)"; \
	P "    After TYPEDEF type_spec IDENTIFIER, seeing ';': 1 S/R conflict."; \
	P "  Fix:     Delete rule 22. Rule 23 covers all typedef forms."; \
	P ""; \
	P "── AFTER (redundant typedef rule removed) ───────────────────────"; \
	bison --yacc -Wno-yacc -d cl_parser.y -o /tmp/cl_after_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE \
	  || echo "  (no warnings — State 62 conflict resolved)" | tee -a $$OUT_FILE; \
	P ""; \
	P "── Parse demo: test5.c (uses typedef, const, volatile, static) ─"; \
	P "  Source:  typedef unsigned long size_t;  static inline int f();"; \
	./parser $(TESTS)/test5.c > $(OUT)/test5.txt 2>&1 || true; \
	RESULT=$$(grep -E "successful|PARSE ERROR" $(OUT)/test5.txt | head -1); \
	P "  Result:  $$RESULT"; \
	P "  Full tree → output/test5.txt"; \
	./parser $(TESTS)/test5.c > $(OUT)/test5.txt 2>&1 || true; \
	P ""; \
	\
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  CONFLICT 5 — case_body_list Ambiguity  (States 233/284, 38 S/R)"; \
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  Problem: case_body_list : empty | case_body_list stmt"; \
	P "    After DEFAULT ':' case_body_list, seeing IF (or any stmt token):"; \
	P "    shift (IF starts another stmt in this case) vs"; \
	P "    reduce (labeled_stmt done, IF belongs to outer context)"; \
	P "    19 tokens x 2 states = 38 S/R conflicts."; \
	P "  Fix A: non_case_stmt excludes labeled_stmt from case bodies."; \
	P "  Fix B: %%expect 34 for remaining inherent conflicts (shift correct)."; \
	P ""; \
	P "── BEFORE (case_body_list uses stmt, no non_case_stmt) ──────────"; \
	sed 's/non_case_stmt/stmt/g; s/%expect 34/%expect 0/' cl_parser.y \
	  > /tmp/cl_c5_before.y; \
	bison --yacc -d /tmp/cl_c5_before.y -o /tmp/cl_c5_before_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE || true; \
	P ""; \
	P "── AFTER (non_case_stmt + %%expect 34) ──────────────────────────"; \
	bison --yacc -Wno-yacc -d cl_parser.y -o /tmp/cl_after_tab.c 2>&1 \
	  | grep -E "conflict|warning|error" \
	  | tee -a $$OUT_FILE \
	  || echo "  (no warnings — 38 S/R conflicts resolved)" | tee -a $$OUT_FILE; \
	P ""; \
	P "── Parse demo: test3.c (uses switch/case/default/break) ────────"; \
	P "  Source:  switch(x) { case 1: break; default: x=0; }"; \
	./parser $(TESTS)/test3.c > $(OUT)/test3.txt 2>&1 || true; \
	RESULT=$$(grep -E "successful|PARSE ERROR" $(OUT)/test3.txt | head -1); \
	P "  Result:  $$RESULT"; \
	P ""; \
	\
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  ERROR DIAGNOSTICS DEMO — test4.c  (Part 5)"; \
	P "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"; \
	P "  Source:  int x = 10   ← missing semicolon"; \
	P "           int y = 20;"; \
	./parser $(TESTS)/test4.c 2>&1 | tee -a $$OUT_FILE || true; \
	./parser $(TESTS)/test4.c > $(OUT)/test4.txt 2>&1 || true; \
	P ""; \
	\
	P "══════════════════════════════════════════════════════════════════"; \
	P "  ALL DONE — output/conflicts.txt written"; \
	P "══════════════════════════════════════════════════════════════════"; \
	P ""

# ── Clean ─────────────────────────────────────────────────────────────
clean:
	rm -f lex.yy.c y.tab.c y.tab.h y.output parser
	rm -rf $(OUT)