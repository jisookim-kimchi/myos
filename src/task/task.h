#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "../config.h"
#include "../memory/paging/paging.h"
#include "../idt/idt.h"

struct interrupt_frame;

enum task_state
{
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE,
};

//to store the state of a task
struct registers
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip; //the IP is where the task was last executing before there was an interrupt.
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
};

struct process;
 
struct task
{
    struct registers regs;               // renamed from "register"
    struct paging_4gb_chunk *page_directory;

    struct task *next;
    struct task *prev;
    struct process *process;
    int state;
    void *event_wait_channel;
};

int init_task(struct task* task, struct process* process);
void task_add_head(struct task *t);
void task_add_tail(struct task *t);
struct task* new_task(struct process* process);
struct task* get_cur_task(void);
struct task* get_next_task(void);
void task_delete(struct task* task);

int task_switch(struct task* task);
int task_page();
int task_page_task(struct task* task);

void task_run_first_ever_task();
void task_return(struct registers* regs);
void restore_registers(struct registers* regs);
void user_registers();
void save_registers(struct interrupt_frame *frame);
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max);
void* task_get_stack_item(struct task* task, int index);

void task_block(void *event_wait_channel);
void task_wakeup(void *event_wait_channel);
#endif