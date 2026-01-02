#include "../stdlib/stdlib.h"

//VM BF TEST
int main(int argc, char **argv)
{
  if (argc < 2)
  {
    print("bf argc error\n");
    return -1;
  }

  int fd = fopen(argv[1], "r");
  if (fd < 0)
  {
    print("bf: could not open file\n");
    return -1;
  }

  struct file_stat stat;
  if (fstat(fd, &stat) < 0)
  {
    print("bf: could not stat file\n");
    fclose(fd);
    return -1;
  }

  char *code = malloc(stat.size + 1);
  if (!code)
  {
    print("bf: out of memory\n");
    fclose(fd);
    return -1;
  }

  if (fread(fd, code, stat.size, 1) != 1)
  {
    print("bf: read failure\n");
    free(code);
    fclose(fd);
    return -1;
  }
  code[stat.size] = '\0';
  fclose(fd);

  print("BF VM: Running: ");
  print(argv[1]);
  print("\n");

  //VM STATE
  char* tape = malloc(30000);
  if (!tape)
  {
    print("bf: out of memory for tape\n");
    free(code);
    return -1;
  }
  for (int i = 0; i < 30000; i++) tape[i] = 0;
  int ptr = 0;
  int pc = 0;

  while (code[pc])
  {
    switch (code[pc])
    {
    case '>':
      ptr++;
      break;
    case '<':
      ptr--;
      break;
    case '+':
      tape[ptr]++;
      break;
    case '-':
      tape[ptr]--;
      break;
    case '.':
      putchar(tape[ptr]);
      break;
    case ',':
      tape[ptr] = getkey();
      break;
    case '[':
      if (tape[ptr] == 0)
      {
        int loop = 1;
        while (loop > 0)
        {
          pc++;
          if (code[pc] == '[')
            loop++;
          else if (code[pc] == ']')
            loop--;
        }
      }
      break;
    case ']':
      if (tape[ptr] != 0)
      {
        int loop = 1;
        while (loop > 0)
        {
          pc--;
          if (code[pc] == ']')
            loop++;
          else if (code[pc] == '[')
            loop--;
        }
      }
      break;
    }
    pc++;
  }

  free(code);
  return 0;
}
