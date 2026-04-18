/* test8.c — Bitwise operators and compound assignments */
int main() {
    int x = 255;
    int y = 15;
    int a = x & y;
    int b = x | y;
    int c = x ^ y;
    int d = x << 2;
    int e = x >> 1;
    int f = ~x;
    x += 5;
    x -= 2;
    x *= 3;
    return a + b + c + d + e + f;
}
