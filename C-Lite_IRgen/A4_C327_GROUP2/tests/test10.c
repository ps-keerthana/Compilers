/* test10.c — Combined: functions, loops, conditionals, arithmetic */
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int sum_range(int lo, int hi) {
    int total = 0;
    int i = lo;
    while (i <= hi) {
        total = total + i;
        i = i + 1;
    }
    return total;
}

int main() {
    int f5 = factorial(5);
    int s = sum_range(1, 10);
    int result;
    if (f5 > s) {
        result = f5 - s;
    } else {
        result = s - f5;
    }
    for (int k = 0; k < 3; k++) {
        result = result + k;
    }
    return result;
}
