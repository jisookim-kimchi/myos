#ifndef IDT_H
#define IDT_H

#include <stdint.h>

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

#endif