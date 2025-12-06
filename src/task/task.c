#include "task.h"
#include "../memory/memory.h"
#include "../status.h"
#include "../config.h"
#include "process.h"
#include "../memory/heap/kernel_heap.h"
#include "../kernel.h"

struct task* task_cur = NULL;
struct task* task_head = NULL;
struct task* task_tail = NULL;

int init_task(struct task* task, struct process* proc)
{
    ft_memset(task, 0 ,sizeof(struct task));

    task->page_directory = paging_new_4gb(PAGING_PRESENT | PAGING_USER_ACCESS);
    if (!task->page_directory)
    {
        return -MYOS_ERROR_NO_MEMORY;
    }
    task->regs.ip = MYOS_PROGRAM_VIRTUAL_ADDRESS; 
    task->regs.ss = MYOS_USER_DATA_SEGMENT;
    task->regs.cs = MYOS_USER_CODE_SEGMENT;
    task->regs.esp = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    task->process = proc;
    return 0;
}

void task_add_head(struct task *t)
{
    t->next = task_head;
    t->prev = NULL;
    if (task_head)
        task_head->prev = t;
    else
        task_tail = t;
    task_head = t;
}

void task_add_tail(struct task *t)
{
    t->next = NULL;
    t->prev = task_tail;
    if (task_tail)
        task_tail->next = t;
    else
        task_head = t;
    task_tail = t;
}

struct task* new_task(struct process* proc)
{
    struct task *task = (struct task*)kernel_malloc(sizeof(struct task));
    if (!task)
    {
        return NULL;
    }
    if (init_task(task, proc) < 0)
    {
        kernel_free(task);
        return NULL;
    }
    task_add_tail(task);
    if (task_cur == NULL)
    {
        task_cur = task;
    }
    return task;
}

struct task* get_cur_task()
{
    return task_cur;
}

struct task* get_next_task()
{
    if (task_cur->next == NULL)
    {
        return task_head;
    }
    return task_cur->next;
}

void task_delete(struct task* task)
{
    if (task->prev)
        task->prev->next = task->next;
    else
        task_head = task->next;

    if (task->next)
        task->next->prev = task->prev;
    else
        task_tail = task->prev;

    if (task == task_cur)
        task_cur = task->next;
    paging_free_4gb(task->page_directory);
    kernel_free(task);
}

int task_switch(struct task* task)
{
    task_cur = task;
    paging_switch(task->page_directory->directory_entry);
    return 0;
}

int task_page()
{
    user_registers();
    task_switch(task_cur);
    return 0;
}

void task_run_first_ever_task()
{
    if (!task_cur)
    {
        panic("task_run_first_ever_task(): No current task exists!\n");
    }

    task_switch(task_head);
    task_return(&task_head->regs);
}
