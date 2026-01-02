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
      print("error for writing\n");
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
              print("read: ");
              print(buf);
              print("\n");
          }
          else
          {
              print("read failed\n");
          }
          fclose(fd);
      }
      else
      {
          print("failed open file\n");
      }
  }
  char cmd[256];
  int idx = 0;
  
  print("MYOS>> ");
  while (1)
  {
    char c = getkey();
    if (c == '\n')
    {
      print("\n");
      cmd[idx] = 0;
      
      if (idx > 0) 
      {
          // Simple command parser
          if (cmd[0] == 'r' && cmd[1] == 'u' && cmd[2] == 'n' && cmd[3] == ' ') 
          {
               // run filename
               char* filename = &cmd[4];
               int pid = exec(filename);
               if (pid < 0) 
               {
                   print("failed run process\n");
               }
               else
               {
                   print("process started with PID \n");
                   itoa(pid, cmd); // reuse cmd buffer for itoa
                   print(cmd);
                   print("waiting for exit...\n");
                   
                   int status = 0;
                   int waited_pid = wait_pid(&status);
                   
                   print("process finished: PID ");
                   itoa(waited_pid, cmd);
                   print(cmd);
                   print("status");
                   itoa(status, cmd);
                   print(cmd);
                   print("\n");
               }
          }
          else if (cmd[0] == 'q')
          {
              break;
          }
          else
          {
              print("unknow command\n");
          }
      }

      idx = 0;
      print("MYOS>> ");
      continue;
    }
    
    // Echo and buffer
    if (c == 8) // Backspace
    {
        if (idx > 0)
        {
            idx--;
            putchar(c);
        }
        continue;
    }

    putchar(c);
    if (idx < 255)
    {
        cmd[idx++] = c;
    }
  }
  return 0;
}
