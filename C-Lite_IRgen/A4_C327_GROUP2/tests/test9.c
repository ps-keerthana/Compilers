/* test9.c — Pointers and address-of / dereference */
int main() {
    int a = 42;
    int *p;
    p = &a;
    int b = *p;
    *p = 100;
    int arr[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = arr[0] + arr[1];
    return b + arr[2];
}
