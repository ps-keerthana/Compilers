#include <stdio.h>

/* Helper functions */

int is_letter(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int is_digit(int c) {
    return (c >= '0' && c <= '9');
}

int is_id_start(int c) {
    return is_letter(c) || c == '_';
}

int is_id_char(int c) {
    return is_letter(c) || is_digit(c) || c == '_';
}

int my_strlen(const char *s) {
    int i = 0;
    while (s[i] != '\0') i++;
    return i;
}

int my_strcmp(const char *a, const char *b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return 1;
        i++;
    }
    return (a[i] == '\0' && b[i] == '\0') ? 0 : 1;
}

int is_keyword(const char *s) {
    if (my_strcmp(s, "int") == 0) return 1;
    if (my_strcmp(s, "float") == 0) return 1;
    if (my_strcmp(s, "char") == 0) return 1;
    if (my_strcmp(s, "if") == 0) return 1;
    if (my_strcmp(s, "else") == 0) return 1;
    if (my_strcmp(s, "while") == 0) return 1;
    if (my_strcmp(s, "return") == 0) return 1;
    return 0;
}

void classify_and_print(char *token) {
    int i;

    if (token[0] == '\0') return;

    if (is_keyword(token)) {
        printf("<KEYWORD, %s>\n", token);
        return;
    }

    int all_digits = 1;
    for (i = 0; token[i] != '\0'; i++) {
        if (!is_digit(token[i])) {
            all_digits = 0;
            break;
        }
    }
    if (all_digits) {
        printf("<NUMBER, %s>\n", token);
        return;
    }

    if (is_id_start(token[0])) {
        int valid = 1;
        for (i = 1; token[i] != '\0'; i++) {
            if (!is_id_char(token[i])) {
                valid = 0;
                break;
            }
        }
        if (valid) {
            printf("<IDENTIFIER, %s>\n", token);
            return;
        }
    }

    printf("<UNKNOWN, %s>\n", token);
}

int main() {
    int ch;
    char token[256];
    int pos = 0;

    while ((ch = getchar()) != EOF) {

        /* Whitespace */
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            if (pos > 0) {
                token[pos] = '\0';
                classify_and_print(token);
                pos = 0;
            }
            continue;
        }

        /* Operators */
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '=') {
            if (pos > 0) {
                token[pos] = '\0';
                classify_and_print(token);
                pos = 0;
            }
            printf("<OPERATOR, %c>\n", ch);
            continue;
        }

        /* Punctuation */
        if (ch == '(' || ch == ')' || ch == '{' || ch == '}' ||
            ch == ';' || ch == ',') {
            if (pos > 0) {
                token[pos] = '\0';
                classify_and_print(token);
                pos = 0;
            }
            printf("<PUNCTUATION, %c>\n", ch);
            continue;
        }

        /* Unknown characters */
        if (!is_letter(ch) && !is_digit(ch) && ch != '_') {
            if (pos > 0) {
                token[pos] = '\0';
                classify_and_print(token);
                pos = 0;
            }
            printf("<UNKNOWN, %c>\n", ch);
            continue;
        }

        /* Build token */
        if (pos < 255) {
            token[pos++] = (char)ch;
        }
    }

    if (pos > 0) {
        token[pos] = '\0';
        classify_and_print(token);
    }

    return 0;
}
