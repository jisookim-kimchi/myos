#include "../task/task.h"
#include "../io/io.h"
#include "../kernel.h"

void *sys_call0_sum(struct interrupt_frame *frame)
{
    struct task* t = get_cur_task();
    if (!t)
    {
        print("sys_call0_sum: No current task!\n");
        return 0;
    }

    int v1 = (int) task_get_stack_item(t, 0);
    int v2 = (int) task_get_stack_item(t, 1);
    
    return (void*)(v1 + v2);
}