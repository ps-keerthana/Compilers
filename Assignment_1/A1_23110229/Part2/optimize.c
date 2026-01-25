#include <stdio.h>
int main() {
    int x = 0;
    x += 2;
    x += 2;
    --x;
    x += 5;
    ++x;

    printf("x = %d\n", x);
    return 0;
}