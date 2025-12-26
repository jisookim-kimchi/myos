#include "../io/io.h"
#include "../kernel.h"
#include "../task/process.h"
#include "../task/task.h"

void *sys_call6_sbrk(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  if (!t || !t->process)
  {
    return (void*)-1;
  }

  int amounts = (int)(intptr_t)task_get_stack_item(t, 1);
  return process_sbrk(t->process, amounts);
}