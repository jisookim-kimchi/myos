#include "isr80h.h"
#include "../idt/idt.h"
#include "io.h"
#include "user_heap.h"

void isr80h_register_command_call()
{
  isr80h_register_command(SYSTEM_COMMAND1_PRINT, sys_call1_print);
  isr80h_register_command(SYSTEM_COMMAND2_GETKEY, sys_call2_getkey);
  isr80h_register_command(SYSTEM_COMMAND3_SLEEP, sys_call3_sleep);
  isr80h_register_command(SYSTEM_COMMAND4_OPEN, sys_call4_fopen);
  isr80h_register_command(SYSTEM_COMMAND5_READ, sys_call5_fread);
  isr80h_register_command(SYSTEM_COMMAND6_SBRK, sys_call6_sbrk);
  isr80h_register_command(SYSTEM_COMMAND7_CLOSE, sys_call7_fclose);
  isr80h_register_command(SYSTEM_COMMAND8_WRITE, sys_call8_fwrite);
}