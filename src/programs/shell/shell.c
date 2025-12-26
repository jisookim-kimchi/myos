#include "../stdlib/stdlib.h"

int main()
{
  print("Shell: Starting...\n");


  // Malloc/Free Test
  print("Malloc Test 1\n");
  void* p1 = malloc(512);
  print("p1 allocated at: ");
  print_hex((uint32_t)p1);
  print("\n");

  print("Freeing p1\n");
  free(p1);

  print("Malloc Test 2:\n");
  void* p2 = malloc(256);
  print("p2 allocated at: ");
  print_hex((uint32_t)p2);
  print("\n");

  if (p1 == p2)
  {
      print("SUCCESS: Memory Reused!\n");
  }
  else
  {
      print("NOTE: Memory not reused.\n");
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
