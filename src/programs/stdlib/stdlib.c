#include "stdlib.h"

void putchar(char c)
{
    char buf[2];
    buf[0] = c;
    buf[1] = 0;
    print(buf);
}

void itoa(int n, char s[])
{
  int i, sign;
  if ((sign = n) < 0)
    n = -n;
  i = 0;
  if (n == 0)
  {
    s[i++] = '0';
  }
  while (n > 0)
  {
    s[i++] = n % 10 + '0';
    n /= 10;
  }
  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';

  // reverse
  int j, k;
  char c;
  for (j = 0, k = i - 1; j < k; j++, k--)
  {
    c = s[j];
    s[j] = s[k];
    s[k] = c;
  }
}

void print_hex(uint32_t n)
{
    char hex_chars[] = "0123456789ABCDEF";
    char buf[11]; // 0x + 8 chars + null
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--)
    {
        buf[i + 2] = hex_chars[n & 0xF];
        n >>= 4;
    }
    buf[10] = '\0';
    print(buf);
}

size_t ft_strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

char *ft_strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

