/* test6.c — Logical and relational operators */
int main() {
    int x = 5;
    int y = 10;
    int z = 0;
    int r1 = x < y;
    int r2 = x >= 5;
    int r3 = y != 10;
    int l1 = x > 0 && y > 0;
    int l2 = x == 0 || y == 10;
    if (x > 0 && y > x) {
        z = x + y;
    } else {
        z = x - y;
    }
    return z;
}
