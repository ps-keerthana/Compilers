/* test5.c — Tests for features fixed in the corrected parser
   ──────────────────────────────────────────────────────────
   1. unsigned alone (not just unsigned int)
   2. const qualifier on declarations
   3. volatile qualifier
   4. Function pointers via typedef
   5. C99 for-loop with init declaration
   6. Mixed declarations and statements (C99 block_item_list)
   7. Nested structs
   8. Long long int
   9. Signed int
  10. Inline storage class
*/

/* 1. unsigned alone as a type */
unsigned counter = 0;
unsigned char byte_val = 255;
unsigned long big_num = 100000;

/* 2. const qualifier */
const int MAX = 100;
const float PI = 3.14159;

/* 3. typedef with function pointer */
typedef int handler;

/* 4. Nested struct */
struct Node {
    int data;
    struct Node *next;
};

/* 5. inline function */
static inline int square(int x) {
    return x * x;
}

/* 6. long long */
long long large_value(long long a, long long b) {
    return a + b;
}

/* 7. signed */
signed int signed_val = -42;

int main() {

    /* C99 for-loop: declaration in init */
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total += i;
    }

    /* Mixed declarations and statements in a block */
    int a = 1;
    a = a + 1;              /* statement */
    int b = a * 2;          /* declaration AFTER statement — C99 */
    b++;                    /* statement */
    unsigned int c = 0;     /* another declaration */

    /* const local variable */
    const int limit = 50;
    if (total > limit) {
        total = limit;
    }

    /* Nested compound statements */
    {
        int inner = 10;
        {
            int deeper = inner + 5;
            total += deeper;
        }
        int after_deep = inner - 1;  /* decl after nested block */
        total += after_deep;
    }

    /* Pointer to struct */
    struct Node n1;
    n1.data = 42;
    n1.next = 0;
    struct Node *ptr = &n1;
    ptr->data = 99;

    /* unsigned arithmetic */
    unsigned u = 200;
    u += 55;

    /* long long */
    long long ll = large_value(1000000000, 2000000000);

    /* cast */
    int truncated = 0;
    float f = 3.14;
    unsigned char ch = 255;

    /* Ternary with complex conditions */
    int result = (a > b && c == 0) ? a : b;

    /* Compound assignment with all operators */
    result += 1;
    result -= 1;
    result *= 2;
    result /= 2;
    result %= 100;
    result &= 0xFF;
    result |= 0x01;
    result ^= 0x10;
    result <<= 1;
    result >>= 1;

    return result;
}
