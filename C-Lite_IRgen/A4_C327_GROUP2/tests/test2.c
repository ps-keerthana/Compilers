/* test2.c — If / If-else conditional statements */
int main() {
    int x = 10;
    int y = 20;
    int max;
    if (x > y) {
        max = x;
    } else {
        max = y;
    }
    if (max > 15) {
        max = max + 1;
    }
    int grade = 75;
    int result = 0;
    if (grade >= 90) {
        result = 4;
    }
    if (grade >= 75) {
        result = 3;
    }
    if (grade < 60) {
        result = 2;
    }
    return result;
}
