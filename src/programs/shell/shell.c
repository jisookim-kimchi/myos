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
      print("SUCCESS\n");
  }
  else
  {
      print("FAILED\n");
  }

  int fd = fopen("0:/test.txt", "w");
  if (fd < 0)
  {
      print("Error for writing\n");
  }
  else
  {
      char* message = "Write Test!";
      fwrite(message, 1, 11, fd);
      print("bytes written.\n");

      fclose(fd);

      fd = fopen("0:/test.txt", "r");
      if (fd >= 0)
      {
          char buf[1025];
          int res = fread(fd, buf, 1, 1024);
          if (res > 0)
          {
              buf[res] = '\0';
              print("Read: ");
              print(buf);
              print("\n");
          }
          else
          {
              print("Read failed\n");
          }
          fclose(fd);
      }
      else
      {
          print("Failed to open file\n");
      }
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
