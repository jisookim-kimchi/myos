#ifndef ISR80H_PROCESS_H
#define ISR80H_PROCESS_H

struct interrupt_frame;

void *sys_process_exit(struct interrupt_frame *frame);
void *sys_process_wait(struct interrupt_frame *frame);
void *sys_process_exec(struct interrupt_frame *frame);

#endif
