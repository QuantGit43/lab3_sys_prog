#include <stdio.h>

int collatz(int n)
{
    int steps = 0;
    while (1) {
        if (n == 1)
            break;
        if (n % 2 == 0)
            n /= 2;
        else
            n = 3 * n + 1;
        ++steps;
    }
    return steps;
}

int main(void)
{
    int best = 0, arg = 0;
    for (int i = 1; i < 30; i++) {
        int s = collatz(i);
        if (s > best) {
            best = s;
            arg = i;
        }
    }
    printf("longest collatz below 30: %d (%d steps)\n", arg, best);
    int k = 0;
    for (;;) {
        k++;
        if (k % 3) continue;
        if (k > 15) break;
        putchar('0' + k / 3);
    }
    putchar('\n');
    return 0;
}
