Name: Pappala Sai Keerthana
Roll Number: 23110229
Email: keerthana.pappala@iitgn.ac.in

Programming Language Used (Part 3): C

Compilation Instructions:
1. Compile the tokenizer using:
   gcc tokenizer.c -o tokenizer

2. Run the tokenizer by redirecting input from a file:
   ./tokenizer < public_test1.txt > public_output1.txt

   (This reads input from standard input until EOF and writes output to a file.)

Design Decisions:

1. Operator Handling:
   The OPERATOR category is strictly limited to the symbols specified in the assignment:
   +, -, *, /, =
   Multicharacter operators such as ==, >=, <=, != are not treated as single tokens.
   For example, the input "a >= 5" is tokenized as:
   <UNKNOWN, >>
   <OPERATOR, =>

2. Numeric and Alphanumeric Tokens:
   Only pure digit sequences are recognized as NUMBER tokens.
   Tokens that mix digits and letters without a separator (e.g., 123var) are treated as UNKNOWN,

3. Floatingpoint Numbers:
   Floatingpoint literals such as 3.14 are not supported, as NUMBER tokens are defined to be
   integers only. Such inputs are tokenized into NUMBER, UNKNOWN, NUMBER.

4. The tokenizer processes input strictly according to the rules specified in the assignment,
   even if some behaviors differ from general compiler design conventions (e.g., maximal munch).

