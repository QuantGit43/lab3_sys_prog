#include <stdio.h>

int fib(int n)
{
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

int main()
{
    int a = 0, b = 1, t;
    int i = 0;
    while (i < 20) {
        printf("%d ", a);
        t = a + b;
        a = b;
        b = t;
        i += 1;
    }
    printf("\nfib(25) = %d\n", fib(25));
    return 0;
}
