#include "stdlib.h"

int main(int argc, char **argv)
{
  print("WAITER: running!\n");
  sleep(1);
  print("WAITER: exiting with code 123.\n");

  if (argc > 1)
  {
    print("WAITER: first arg = ");
    print(argv[1]);
    print("\n");
  }


  exit(0);  
  return 0;
}
