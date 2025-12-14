#include "isr80h.h"
#include "../idt/idt.h"
#include "misc.h"
#include "io.h"

void isr80h_register_command_call()
{
    isr80h_register_command(SYSTEM_COMMAND0_SUM, sys_call0_sum);
    isr80h_register_command(SYSTEM_COMMAND1_PRINT, sys_call1_print);
    isr80h_register_command(SYSTEM_COMMAND2_GETKEY, sys_call2_getkey);
}