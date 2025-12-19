#include "stdlib.h"

void putchar(char c)
{
    char buf[2];
    buf[0] = c;
    buf[1] = 0;
    print(buf);
}

