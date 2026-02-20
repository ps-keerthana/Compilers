// Edge case 15: sizeof with type and with expression
int a = sizeof(int);
int b = sizeof(char *);
int c = sizeof(struct Node);
int d = sizeof a;
int e = sizeof(a + b);
