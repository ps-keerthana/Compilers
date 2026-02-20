// Edge case 12: No preprocessor directives allowed
// This should FAIL with lexer error because # is invalid
#include <stdio.h>
int main() { return 0; }
