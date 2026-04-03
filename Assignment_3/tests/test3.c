/* test3.c — comprehensive: loops, switch, structs, pointers
   Also demonstrates BUG FIX: mixed declarations and statements
   inside blocks (C99 style) now works correctly.
   The original grammar failed here at line 23 because it required
   ALL declarations before ANY statements in a block. */

struct Point {
    int x;
    int y;
};

enum Direction { NORTH, SOUTH, EAST, WEST };

int factorial(int n) {
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

int main() {
    /* for loop — C89 style (declare before loop) */
    int sum = 0;
    int i;
    for (i = 0; i < 10; i++) {
        sum = sum + i;
    }

    /* ── BUG FIX DEMO: mixed decl + stmt ──────────────────────────
       Old grammar required: { ALL_DECLS ; THEN_STMTS }
       This would have failed: decl, stmt, then another decl.
       New grammar uses block_item_list — any order is fine. */
    int count = 0;          /* declaration */
    while (count < 5) {     /* statement   */
        count++;
    }
    int total = sum + count;  /* declaration AFTER statement — C99 OK */

    /* do-while */
    do {
        count--;
    } while (count > 0);

    /* switch */
    int day = 3;
    switch (day) {
        case 1:
            sum = 1;
            break;
        case 2:
            sum = 2;
            break;
        default:
            sum = 0;
            break;
    }

    /* struct */
    struct Point p;
    p.x = 10;
    p.y = 20;

    /* pointer */
    int *ptr;
    ptr = &sum;
    *ptr = 99;

    /* ternary operator */
    int max = (p.x > p.y) ? p.x : p.y;

    /* unary minus / plus */
    int neg = -sum;
    int pos = +sum;

    /* bitwise operators */
    int mask = 0xFF;
    int val  = sum & mask;
    val = val | 0x01;
    val = val ^ 0x10;
    val = val << 2;
    val = val >> 1;

    /* compound assignment */
    total += 5;
    total -= 2;
    total *= 3;

    /* sizeof */
    int sz = sizeof(int);
    int sz2 = sizeof total;

    /* enum */
    enum Direction dir = NORTH;
    if (dir == NORTH) {
        total = total + 1;
    }

    /* no cast — simplified for LALR(1) grammar */
    float f = 3.14;
    int truncated = 0;

    /* C99 for loop: declaration in init clause */
    for (int j = 0; j < 5; j++) {
        sum += j;
    }

    return total;
}
