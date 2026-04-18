/* test1.c — Arithmetic expressions and assignment
   Demonstrates: a = b * -c + b * -c (the assignment spec example)
   Plus basic arithmetic quadruples */
int main() {
    int a;
    int b = 5;
    int c = 3;
    a = b * -c + b * -c;
    int x = 10;
    int y = 4;
    int z = x + y * 2;
    return z;
}
