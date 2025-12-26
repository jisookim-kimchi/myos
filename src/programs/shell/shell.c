#include "../stdlib/stdlib.h"

int main()
{
  print("Shell: Starting...\n");
  
//   print("Code Address: ");
//   print_hex((uint32_t)main);
//   print("\n");
  
//   int stack_var = 0;
//   print("Stack Address: ");
//   print_hex((uint32_t)&stack_var);
//   print("\n");

  int fd = fopen("0:/test.txt", "r");
  sleep(1);
  if (fd >= 0)
  {
    char buf[1025];
    print("Shell: Success \n");
    int res = fread(fd, buf, 1, 1024);
    if (res >= 0)
    {
      buf[res] = '\0';
    }
    print(buf);
  }
  else
  {
    print("Shell: Open failed!\n");
  }
  print("MYOS>> ");
  while (1)
  {
    char c = getkey();
    if (c == '\n')
    {
      print("\nMYOS>> ");
      continue;
    }
    putchar(c);
    if (c == 'q')
    {
      break;
    }
  }
  return 0;
}
