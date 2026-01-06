#include "kernel_print.h"
#include "string/string.h"

uint16_t *video_memory = 0;
uint16_t terminal_row = 0;
uint16_t terminal_column = 0;

uint16_t terminal_make_char(char c, uint8_t color)
{
  return (uint16_t)c | (uint16_t)color << 8;
}

void terminal_putchar(char c, uint8_t color, size_t x, size_t y)
{
  const size_t index = y * VGA_WIDTH + x;
  video_memory[index] = terminal_make_char(c, color);
}

void terminal_scroll()
{
  for (int y = 0; y < VGA_HEIGHT - 1; y++)
  {
    for (int x = 0; x < VGA_WIDTH; x++)
    {
      video_memory[y * VGA_WIDTH + x] = video_memory[(y + 1) * VGA_WIDTH + x];
    }
  }
  for (int x = 0; x < VGA_WIDTH; x++)
  {
    terminal_putchar(' ', 0x00, x, VGA_HEIGHT - 1);
  }
  terminal_row = VGA_HEIGHT - 1;
}

void terminal_write_char(char c, uint8_t color)
{
  if (c == '\n')
  {
    terminal_column = 0;
    terminal_row++;
    if (terminal_row >= VGA_HEIGHT)
    {
      terminal_scroll();
    }
    return;
  }
  if (c == 0x08) // backspace
  {
    if (terminal_column > 0)
    {
      terminal_column--;
      terminal_putchar(' ', 0x0F, terminal_column, terminal_row);
    }
    return;
  }
  terminal_putchar(c, color, terminal_column, terminal_row);
  terminal_column++;
  if (terminal_column >= VGA_WIDTH)
  {
    terminal_column = 0;
    terminal_row++;
    if (terminal_row >= VGA_HEIGHT)
    {
      terminal_scroll();
    }
  }
}

void init_terminal()
{
  video_memory = (uint16_t *)VIDEO_MEMORY_ADDRESS;
  terminal_column = 0;
  terminal_row = 0;
  for (int y = 0; y < VGA_HEIGHT; y++)
  {
    for (int x = 0; x < VGA_WIDTH; x++)
    {
      terminal_putchar(' ', 0x00, x, y); // 검은 배경으로 초기화
    }
  }
}

void print(const char *str)
{
  size_t len = ft_strlen(str);
  for (size_t i = 0; i < len; i++)
  {
    terminal_write_char(str[i], 0x0F); // 하얀색 글자, 검은 배경
  }
}

void panic(const char *msg)
{
  print(msg);
  while (1)
  {
  }
}

void itoa(int n, char s[])
{
  int i, sign;
  if ((sign = n) < 0)
    n = -n;
  i = 0;
  do
  {
    s[i++] = n % 10 + '0';
  }
  while ((n /= 10) > 0);
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

void print_int(int v)
{
  char buf[20];
  itoa(v, buf);
  print(buf);
}

void print_hex(uint32_t n)
{
  char hex_chars[] = "0123456789ABCDEF";
  char buf[11];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 7; i >= 0; i--)
  {
    buf[i + 2] = hex_chars[(n >> (i * 4)) & 0xF];
  }
  buf[10] = '\0';
  print(buf);
}