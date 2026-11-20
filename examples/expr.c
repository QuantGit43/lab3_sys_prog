#include <stdio.h>

int g = -5;

int main(void)
{
    int x = 7, y = 3, z;
    printf("%d %d %d %d %d\n", x + y * 2, (x + y) * 2, x - y - 1, x / y, x % y);
    printf("%d %d %d\n", -x + 1, !x, !(x - 7));
    printf("%d %d %d %d %d %d\n", x < y, x > y, x <= 7, y >= 4, x == 7, x != 7);
    printf("%d %d %d\n", x > 0 && y > 0, x < 0 || y < 0, x && 0 || 1);
    z = x++ + ++y;
    printf("%d %d %d\n", x, y, z);
    z *= 2;
    z -= g;
    z %= 7;
    printf("%d %d\n", z, g);
    x = y = z = 1;
    printf("%d %d %d %c\n", x, y, z, 'A' + 2);
    return 0;
}
