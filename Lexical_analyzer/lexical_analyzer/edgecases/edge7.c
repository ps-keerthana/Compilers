// Edge case 7: All operators from spec
// Unary
int a = 1;
int b = +a;
int c = -a;
int d = !a;
int *e = &a;
int f = *e;
int g = ~a;
a++;
a--;
++a;
--a;
// Binary arithmetic
int h = a + b;
int i = a - b;
int j = a * b;
int k = a / b;
int l = a % b;
// Relational
int m = a < b;
int n = a > b;
int o = a <= b;
int p = a >= b;
int q = a == b;
int r = a != b;
// Bitwise
int s = a & b;
int t = a | b;
int u = a ^ b;
int v = a << b;
int w = a >> b;
int x = ~a;
// Logical
int y = a && b;
int z = a || b;
// Assignment
a = b;
a += b;
a -= b;
a *= b;
a /= b;
a %= b;
a &= b;
a |= b;
a ^= b;
a <<= b;
a >>= b;
// Ternary
int res = a ? b : c;
// sizeof
int sz = sizeof(a);
