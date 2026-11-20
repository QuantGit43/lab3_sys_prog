#include <stdio.h>

int main(void)
{
    int sieve[100];
    int count = 0;
    for (int i = 0; i < 100; i++)
        sieve[i] = 1;
    for (int i = 2; i * i < 100; i++)
        if (sieve[i])
            for (int j = i * i; j < 100; j += i)
                sieve[j] = 0;
    for (int i = 2; i < 100; i++) {
        if (!sieve[i])
            continue;
        printf("%d ", i);
        count++;
    }
    printf("\ncount = %d\n", count);
    return 0;
}
