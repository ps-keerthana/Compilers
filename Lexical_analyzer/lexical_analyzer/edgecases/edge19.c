// Edge case 19: switch/case/default with fall-through
int x = 2;
switch (x) {
    case 1:
        x = 10;
    case 2:
        x = 20;
    case 3:
        x = 30;
        break;
    default:
        x = 0;
}
