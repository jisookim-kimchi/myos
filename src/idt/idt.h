#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct interrupt_frame;
typedef void* (*ISR80_COMMAND)(struct interrupt_frame *frame);

struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

struct idt_descriptor
{
    uint16_t bottom_offset; // 0 ~ 15 (하위 16비트가 먼저!)
    uint16_t selector; //selector that in our GDT global descriptor table , which contains memory segment, the code segment, data segment, stack and other segments.
    uint8_t zero; //반드시 0
    uint8_t type_attr; //descriptor type and attributes
    uint16_t top_offset; // 16 ~ 31 (상위 16비트가 나중!)
} __attribute__((packed)); //패딩넣지마!

struct idtr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_init();
void idt_set(int interrupt_number, void *address);
void idt_zero();
void enable_interrupts();
void disable_interrupts();
void registers_save(struct interrupt_frame *frame);
void isr80h_register_command(int ask_id, ISR80_COMMAND command);
void *isr80h_handle_command(int ask, struct interrupt_frame* frame);
void* isr80h_handler(struct interrupt_frame* frame);
#endif