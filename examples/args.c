#include <stdio.h>

int weighted(int a, int b, int c, int d, int e, int f, int g, int h)
{
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h;
}

int depth(int n)
{
    if (n == 0)
        return 0;
    return 1 + depth(n - 1) + weighted(n, 0, 0, 0, 0, 0, 0, 1) - n - 8;
}

int main(void)
{
    printf("%d\n", weighted(1, 2, 3, 4, 5, 6, 7, 8));
    printf("%d %d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7, weighted(8, 7, 6, 5, 4, 3, 2, 1));
    printf("%d\n", 10 + weighted(1, 1, 1, 1, 1, 1, 1, depth(5)));
    return 0;
}
