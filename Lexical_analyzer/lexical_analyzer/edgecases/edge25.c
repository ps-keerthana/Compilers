// Edge case 25: if condition using literals/constants directly (spec allows this)
if (1) { int a = 0; }
if (0) { int b = 0; }
if (3.14) { int c = 0; }
if ('A') { int d = 0; }
if ("hello") { int e = 0; }
