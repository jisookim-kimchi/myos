#include "thread.h"
#include "../../src/task/task.h"

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
    if (priority > 0)
    {
        new_task->priority = priority;
    }
    return (void *)(uintptr_t)0;
}