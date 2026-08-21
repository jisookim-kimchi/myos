#ifndef THREAD_H
#define THREAD_H

struct interrupt_frame;
void *sys_call_thread_create(struct interrupt_frame *frame);
void *sys_call_thread_exit(struct interrupt_frame *frame);
void *sys_thread_join(struct interrupt_frame *frame);
void *sys_call_task_block(struct interrupt_frame *frame);
void *sys_call_task_wakeup(struct interrupt_frame *frame);

#endif
