// Edge case 23: Function declaration, definition, and call
int add(int a, int b);
void printMsg(char *msg);
int add(int a, int b) {
    return a + b;
}
void printMsg(char *msg) {
    return;
}
int main() {
    int result = add(3, 4);
    printMsg("hello");
    return 0;
}
