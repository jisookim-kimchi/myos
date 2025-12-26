#ifndef ISR80H_IO_H
#define ISR80H_IO_H

struct interrupt_frame;
void *sys_call1_print(struct interrupt_frame *frame);
void *sys_call2_getkey(struct interrupt_frame *frame);
void *sys_call3_sleep(struct interrupt_frame *frame);
void *sys_call4_fopen(struct interrupt_frame *frame);
void *sys_call5_fread(struct interrupt_frame *frame);
void *sys_call6_sbrk(struct interrupt_frame *frame);
void *sys_call7_fclose(struct interrupt_frame *frame);
void *sys_call8_fwrite(struct interrupt_frame *frame);
#endif