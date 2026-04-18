/* test7.c — Unary operators: minus, not, increment, decrement */
int main() {
    int a = 5;
    int b = -a;
    int c = !b;
    int d = ~a;
    int e = ++a;
    int f = --a;
    int g = a++;
    int h = a--;
    return b + c + d + e + f + g + h;
}
