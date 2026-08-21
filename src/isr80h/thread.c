#include "thread.h"
#include "../task/task.h"
#include "../system_control/system_control.h"
#include <stdint.h>
#include <stddef.h>

void *sys_call_thread_create(struct interrupt_frame *frame)
{
    struct task *t = get_cur_task();
    if (!t || !t->process)
    {
        return (void *)(uintptr_t)-1;
    }
    void *entry_point = task_get_stack_item(t, 1);
    void *user_stack  = task_get_stack_item(t, 2);
    int priority      = (int)(uintptr_t)task_get_stack_item(t, 3);

    struct task *new_task = task_create(t->process, entry_point, user_stack);
    if (!new_task)
    {
        return (void *)(uintptr_t)-1;
    }
    if (priority >= TASK_PRIORITY_HIGH && priority <= TASK_PRIORITY_LOW)
    {
        new_task->priority = priority;
    }
    return (void *)(uintptr_t)new_task->thread_id;
}

void *sys_call_thread_exit(struct interrupt_frame *frame)
{
    struct task* current_thread = get_cur_task();
    task_wakeup_by_event((void*)current_thread);

    tasks_log();
    task_delete(current_thread);
    while (1)
    {
        struct task* next_task = get_next_task();
        if (next_task && next_task->state == TASK_READY)
        {
            next_task->state = TASK_RUNNING;
            task_switch(next_task);
            task_return(&next_task->regs);
        }
        else
        {
            enable_interrupts();
            halt();
        }
    }

    return 0;
}

void *sys_thread_join(struct interrupt_frame *frame)
{
    struct task *current_task = get_cur_task();
    int target_tid = (int)(uintptr_t)task_get_stack_item(current_task, 1);
    struct task *target_task = task_get_by_id(target_tid);
    if (target_task == NULL || target_task->state == TASK_DEAD)
    {
        return (void*)0;
    }
    else if (target_task && target_task->state != TASK_DEAD)
    {
        current_task->event_wait_channel = (void *)target_task;
        current_task->state = TASK_BLOCKED;
        schedule();
    }

    return (void *)0;
}

void *sys_call_task_block(struct interrupt_frame *frame)
{
    struct task *current_task = get_cur_task();
    if (!current_task)
    {
        return (void*)0;
    }
    void *channel = task_get_stack_item(current_task, 1);
    task_block(channel);
    return (void*)0;
}

void *sys_call_task_wakeup(struct interrupt_frame *frame)
{
    struct task *current_task = get_cur_task();
    if (!current_task)
    {
        return (void*)0;
    }
    void *channel = task_get_stack_item(current_task, 1);
    task_wakeup_by_event(channel);
    return (void*)0;
}
