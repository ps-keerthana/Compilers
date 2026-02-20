// Edge case 17: Typecast - (typename)expr
// ( and ) are PUNCTUATION, type inside is KEYWORD
int a = 5;
float b = (float)a;
int c = (int)b;
char *p = (char *)0;
int d = (int)(a + b);
