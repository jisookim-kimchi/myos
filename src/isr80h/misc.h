#ifndef MISC_H
#define MISC_H

struct interrupt_frame;

void *sys_call0_sum(struct interrupt_frame *frame);
#endif