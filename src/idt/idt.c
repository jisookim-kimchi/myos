#include "idt.h"
#include "../config.h"
#include "../memory/memory.h"
#include "../kernel.h"
#include "../task/task.h"
#include "../io/io.h"
#include "../keyboard/keyboard.h"
#include "../mouse/mouse.h"
#include "../timer/timer.h"

struct idt_descriptor idt_desc[MYOS_TOTAL_INTERRUPTS];
static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[MYOS_TOTAL_INTERRUPTS];

struct idtr idtr_descriptor;
static ISR80_COMMAND isr80h_commands[MYOS_MAX_ISR80H_COMMANDS];
extern void *interrupt_table[MYOS_TOTAL_INTERRUPTS];

extern void idt_load(struct idtr *ptr);
//extern void int21h();
//extern void int20h();
//extern void isr80h_wrapper();
extern void no_interrupts();


int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback)
{
    if (interrupt < 0 || interrupt >= MYOS_TOTAL_INTERRUPTS)
    {
    return -1;
  }
  interrupt_callbacks[interrupt] = interrupt_callback;
  return 1;
}

void no_interrupts_handler()
{
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
    idt_set(i, interrupt_table[i]);
  }

    for (int i = 0; i < MYOS_TOTAL_INTERRUPTS; i++)
    {
    idt_register_interrupt_callback(i, no_interrupts_handler);
  }
  // idt_set(0, idt_zero);
  // //idt_set(0x20, int20h);      // 타이머 인터럽트 (IRQ 0)
  // idt_set(0x21, int21h);      // 키보드 인터럽트 (IRQ 1)
  // idt_set(0x80, isr80h_wrapper);      // 시스템 콜
  idt_register_interrupt_callback(0, idt_zero);
    idt_register_interrupt_callback(0x20, timer_handle_interrupt);
    //idt_register_interrupt_callback(0x2C, mouse_handle_interrupt);
  idt_register_interrupt_callback(0x21, keyboard_handle_interrupt);
  idt_register_interrupt_callback(0x80, isr80h_handler);

  idt_load(&idtr_descriptor);
}

//함수배열에 함수를 넣고
void isr80h_register_command(int ask_id, ISR80_COMMAND command)
{
    if (ask_id < 0 || ask_id >= MYOS_MAX_ISR80H_COMMANDS)
    {
        panic("The command is out of bound\n");
    }

    if (isr80h_commands[ask_id])
    {
        panic("Your attempting to overwrite an existing command\n");
    }
    isr80h_commands[ask_id] = command;
}

//함수배열에 들어있는 element를 가져와서 실행.
void *isr80h_handle_command(int ask, struct interrupt_frame* frame)
{
  // TODO: Implement system call handling based on 'ask' value
    if (ask < 0 || ask >= MYOS_MAX_ISR80H_COMMANDS)
    {
        return NULL;
    }

    ISR80_COMMAND command_func = isr80h_commands[ask];
    if(!command_func)
    {
        return NULL;
    }
    return command_func(frame);
}

//1.page directory change to kernel page.
//2.register save.
//3.system call comannd call
//4.again recovery to user page.
void isr80h_handler(struct interrupt_frame* frame)
{
    void* res = NULL;
    int ask = frame->eax; // System call command is in EAX
    change_to_kernel_page();
    save_registers(frame);
    
    /*
    print("Kernel: Syscall ID ");
    print_int(ask);
    print(" entry at IP ");
    print_int(frame->ip);
    print(" (RA: ");
    print_int((uint32_t)(uintptr_t)task_get_stack_item(get_cur_task(), 0));
    print(", Arg1: ");
    print_int((uint32_t)(uintptr_t)task_get_stack_item(get_cur_task(), 1));
    print(")\n");
    */

    res = isr80h_handle_command(ask, frame);
    frame->eax = (uint32_t)(uintptr_t)res;

    struct task* t = get_cur_task();
    if (t)
    {
        paging_switch(t->page_directory);
    }
    
    // print("Kernel: Syscall done, returning to user land\n");
}

//save the cur task;s state
//if we have a cur task, return its stack pointer
//otherwise return the original frame
void interrupt_handler(struct interrupt_frame* frame)
{
    int interrupt = frame->vector_number;

    //EOI send to PIC
    // Acknowledge hardware interrupts BEFORE calling the callback.
    // This is vital because if the callback switches tasks (like the timer), 
    // it never returns to this function, and the PIC would remain blocked.
    if (interrupt >= 0x20 && interrupt < 0x30)
    {
        outsb(0x20, 0x20); // Master PIC EOI
        if (interrupt >= 0x28)
        {
            outsb(0xA0, 0x20); // Slave PIC EOI
        }
    }

    if (interrupt_callbacks[interrupt] != 0)
    {
        interrupt_callbacks[interrupt](frame);
    }
}
