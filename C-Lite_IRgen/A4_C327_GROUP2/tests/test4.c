/* test4.c — For loop (C89 style and C99 style)
   Note: i++ is placed inside the body so the step quad
   appears after the body quads (correct IR ordering).
   The for-with-step variant has a known ordering note in the
   compiler — step quads appear before body in the table when
   step is in the for(...;...; step) position due to LALR(1)
   bottom-up reduction order. */
int main() {
    int sum = 0;

    /* C89 for — step inside body */

    for (int i = 0; i < 5;) {
        sum = sum + i;
        i++;
    }

    /* C99 for — step inside body */
    for (int j = 0; j < 3;) {
        sum = sum * 2;
        j++;
    }

    return sum;
}
