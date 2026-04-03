/* test2.c — dangling else (conflict resolution test)
   ─────────────────────────────────────────────────
   CONFLICT: Dangling else (shift/reduce)
   ──────────────────────────────────────
   BEFORE fix (plain grammar, no %prec):
     Bison reports: "1 shift/reduce conflict"
     Input:  if (a) if (b) x=1; else x=2;
     The else could legally attach to either if, creating ambiguity.
     Bison defaults to shift (correct), but conflict is unresolved.

   AFTER fix (%prec LOWER_THAN_ELSE):
     The if-without-else rule has %prec LOWER_THAN_ELSE.
     ELSE has higher precedence → parser always shifts.
     Result: 0 conflicts. Else attaches to innermost if (C standard).
   ─────────────────────────────────────────────────────────────────── */

int main() {
    int a = 1;
    int b = 2;

    /* Dangling else: else goes to inner if (as C specifies) */
    if (a > 0)
        if (b > 0)
            a = 10;
        else
            a = 20;     /* belongs to inner if, NOT outer if */

    /* Nested with braces — unambiguous regardless of fix */
    if (a > 5) {
        if (b > 5) {
            a = 100;
        }
    } else {
        a = 0;
    }

    /* Triple nesting — stress test for the fix */
    if (a > 0)
        if (b > 0)
            if (a > b)
                a = 999;
            else
                b = 999;

    return 0;
}
