#include "../stdlib/stdlib.h"

int main()
{
  print("Shell: Starting...\n");

  // Heap Test 1: Get initial break
  void* initial_break = sbrk(0);
  print("Initial break: ");
  print_hex((uint32_t)initial_break);
  print("\n");

  // Raw sbrk test
  print("SBRK Test: Requesting 4096 bytes...\n");
  void* old_break = sbrk(0);
  print("Current break: ");
  print_hex((uint32_t)old_break);
  print("\n");

  void* new_ptr = sbrk(5111);
  if (new_ptr == (void*)-1)
  {
      print("sbrk(5111) failed!\n");
  }
  else
  {
      print("New memory allocated at: ");
      print_hex((uint32_t)new_ptr);
      print("\n");

      // Verify we can write to it
      char* access = (char*)new_ptr;
      access[0] = 'T';
      access[1] = 'E';
      access[2] = 'S';
      access[3] = 'T';
      access[4] = '\0';
      print("\n");
      print(access);
      print("\n");

      void* final_break = sbrk(0);
      print("Final break: ");
      print_hex((uint32_t)final_break);
      print("\n");
  }

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
    print("\n");
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
