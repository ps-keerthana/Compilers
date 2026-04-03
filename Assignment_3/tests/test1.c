/* test1.c — basic declarations and expressions */

int x = 10;
float y = 3.14;
char c = 'A';

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(x, 5);
    if (result > 10) {
        result = result - 1;
    } else {
        result = result + 1;
    }
    return 0;
}
