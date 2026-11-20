#include <stdio.h>

int a[10];
int n = 10;

void fill(int seed)
{
    for (int i = 0; i < n; i++) {
        seed = (seed * 37 + 11) % 101;
        a[i] = seed;
    }
}

void print(void)
{
    for (int i = 0; i < n; i++)
        printf("%3d", a[i]);
    printf("\n");
}

void sort(void)
{
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
}

int main(void)
{
    fill(7);
    print();
    sort();
    print();
    return 0;
}
