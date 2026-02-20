// Test case 1: Valid C-Lite program covering all token categories
// Tests: keywords, identifiers, int/float/char/string literals,
//        operators, punctuation, single and multi-line comments

struct Point {
    int x;
    int y;
};

int add(int a, int b);

int main() {
    /* multi-line
       comment test */
    int count = 0;
    float pi = 3.14;
    char ch = 'A';
    char *msg = "hello" " world";

    int arr[5];
    int i;

    for (i = 0; i < 5; i++) {
        arr[i] = i * 2;
    }

    while (count <= 10) {
        count++;
    }

    if (count == 10 && pi >= 3.0) {
        count += 1;
    } else {
        count -= 1;
    }

    int bits = count << 2;
    bits = bits >> 1;
    bits ^= 0;
    bits |= 1;
    bits &= 255;

    return count;
}

int add(int a, int b) {
    return a + b;
}
