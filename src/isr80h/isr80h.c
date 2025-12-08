#include "isr80h.h"
#include "../idt/idt.h"
#include "misc.h"

void isr80h_register_command_call()
{
    isr80h_register_command(SYSTEM_COMMAND0_SUM, sys_call0_sum);
}