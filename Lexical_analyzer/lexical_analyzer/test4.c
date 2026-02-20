// Test case 4: Literals and operator coverage
int main() {
    // Integer literals
    int a = 0;
    int b = 42;

    // Float literals
    float f1 = 3.14;
    float f2 = .5;
    float f3 = 1.0e10;
    float f4 = 2.5e-3;

    // Char literals with escape sequences
    char c1 = 'z';
    char c2 = '\n';
    char c3 = '\'';

    // String with escape sequences and concatenation
    char *s = "hello\nworld" " more text";

    // All compound assignment operators
    a += 1;
    a -= 1;
    a *= 2;
    a /= 2;
    a %= 3;
    a &= 0xFF;
    a |= 0x01;
    a ^= 0x10;
    a <<= 1;
    a >>= 1;

    // Relational and logical operators
    int r = (a == b) || (a != b) && (a <= b) && (a >= b);

    // Unary operators
    int neg = -a;
    int pos = +a;
    int not = !r;
    int bits = ~a;
    int *ptr = &a;
    int val = *ptr;
    int sz = sizeof(a);

    // Pre/post increment/decrement
    a++;
    a--;
    ++b;
    --b;

    // Arrow and dot (struct access)
    struct Point { int x; int y; } p;
    p.x = 10;

    // Ternary operator
    int t = (a > 0) ? a : -a;

    return 0;
}
