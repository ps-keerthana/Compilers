/* test5.c — Functions with return values and calls */
int square(int x) {
    return x * x;
}
int add(int a, int b) {
    return a + b;
}
int main() {
    int a = square(4);
    int b = add(a, 6);
    int c = add(square(2), square(3));
    return c;
}
