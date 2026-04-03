/* test4.c — intentional syntax error: missing semicolon
   Tests Part 5: error diagnostics

   Expected output:
     Line         : 9
     Near token   : 'int'   (the token AFTER the missing ';')
     Lookahead    : INT (token id 258)
     Message      : syntax error
     Hints        : Missing ';' at end of statement?
*/

int main() {
    int x = 10         /* <── missing semicolon here */
    int y = 20;
    return 0;
}
