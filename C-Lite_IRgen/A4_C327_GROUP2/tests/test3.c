/* test3.c — While loop and Do-While loop */
int main() {
    int sum = 0;
    int i = 1;
    while (i <= 10) {
        sum = sum + i;
        i = i + 1;
    }
    int count = 5;
    int product = 1;
    do {
        product = product * count;
        count = count - 1;
    } while (count > 0);
    return sum + product;
}
