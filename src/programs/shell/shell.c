#include "../stdlib/stdlib.h"


int main()
{
    while(1)
    {
        print("MyOS> ");
        while(1)
        {
            int key = getkey();
            // Enter key (13 = \r, 10 = \n)
            if (key == 13 || key == 10)
            {
                print("\n");
                break;
            }
            
            // Backspace (8)
            if (key == 8)
            {
                print("\b");
                continue;
            }
            
            // Normal character
            putchar(key);
            // if (key > 0)
            //     sleep(300);
        }
    }
    return 0;
}
