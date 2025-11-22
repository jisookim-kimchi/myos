#include "idt.h"
#include "../config.h"
#include "../memory/memory.h"
#include "../kernel.h"
#include "../io/io.h"

struct idt_descriptor idt_desc[MYOS_TOTAL_INTERRUPTS];
struct idtr idtr_descriptor;

extern void idt_load(struct idtr *ptr);
extern void int21h();
extern void int20h();
extern void no_interrupts();

void no_interrupts_handler()
{
    outsb(0x20, 0x20); // PIC에 EOI 신호 전송
}

void int21h_handler()
{
    print("\n*** KEYBOARD INTERRUPT CAUGHT! ***\n");
    outsb(0x20, 0x20); // PIC에 EOI 신호 전송
}

void int20h_handler()
{
    print("T"); // 타이머 틱마다 T 출력
    outsb(0x20, 0x20); // PIC에 EOI 신호 전송
}

void idt_zero()
{
    // 나누기 오류 인터럽트 핸들러
    print("\n*** DIVISION BY ZERO INTERRUPT CAUGHT! ***\n");
    print("IDT is working correctly!\n");
}

void idt_set(int interrupt_number, void *address)
{
    struct idt_descriptor *idt = &idt_desc[interrupt_number];
    idt->bottom_offset = (uint32_t)address & 0x0000FFFF;  // 하위 16비트
    idt->selector = MYOS_KERNEL_CODE_SELECTOR;
    idt->zero = 0;
    idt->type_attr = 0xEE; // 11101110b : present, ring 0, 32-bit interrupt gate
    idt->top_offset = (uint32_t)address >> 16;            // 상위 16비트
}

void idt_init()
{
    ft_memset(idt_desc, 0, sizeof(idt_desc));

    idtr_descriptor.limit = sizeof(idt_desc) - 1;
    idtr_descriptor.base = (uint32_t) idt_desc;

    for (int i = 0; i < MYOS_TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, no_interrupts);
    }

    idt_set(0, idt_zero);
    //idt_set(0x20, int20h);      // 타이머 인터럽트 (IRQ 0)
    idt_set(0x21, int21h);      // 키보드 인터럽트 (IRQ 1)
    idt_load(&idtr_descriptor);
}