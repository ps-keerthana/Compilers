// Edge case 9: Struct with dot and arrow access
struct Node {
    int val;
    struct Node *next;
};
struct Node n;
struct Node *ptr;
int a = n.val;
int b = ptr->val;
struct Node **dptr;
int c = (*dptr)->val;
