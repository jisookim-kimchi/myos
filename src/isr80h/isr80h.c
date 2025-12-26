#include "isr80h.h"
#include "../idt/idt.h"
#include "io.h"
#include "misc.h"

void isr80h_register_command_call()
{
  isr80h_register_command(SYSTEM_COMMAND0_SUM, sys_call0_sum);
  isr80h_register_command(SYSTEM_COMMAND1_PRINT, sys_call1_print);
  isr80h_register_command(SYSTEM_COMMAND2_GETKEY, sys_call2_getkey);
  isr80h_register_command(SYSTEM_COMMAND3_SLEEP, sys_call3_sleep);
  isr80h_register_command(SYSTEM_COMMAND4_OPEN, sys_call4_fopen);
  isr80h_register_command(SYSTEM_COMMAND5_READ, sys_call5_fread);
}