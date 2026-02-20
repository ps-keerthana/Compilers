// Edge case 24: Compound statements / nested blocks
int main() {
    int a = 1;
    {
        int b = 2;
        {
            int c = 3;
            {
                int d = 4;
            }
        }
    }
    return 0;
}
