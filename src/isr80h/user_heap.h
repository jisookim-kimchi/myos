#ifndef USER_HEAP_H
#define USER_HEAP_H

struct interrupt_frame;

void *sys_call6_sbrk(struct interrupt_frame *frame);
#endif  