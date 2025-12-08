#include "misc.h"
#include "../idt/idt.h"

void *sys_call0_sum(struct interrupt_frame *frame)
{
    return (void*)3;
}