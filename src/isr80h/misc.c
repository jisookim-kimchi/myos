#include "../io/io.h"
#include "../kernel.h"
#include "../task/task.h"

void *sys_call0_sum(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  if (!t)
  {
    return 0;
  }

  // Index 1 and 2 because Index 0 is the Return Address
  int v1 = (int)task_get_stack_item(t, 1);
  int v2 = (int)task_get_stack_item(t, 2);

  return (void *)(v1 + v2);
}