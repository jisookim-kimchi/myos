#include "io.h"
#include "../kernel.h"
#include "../keyboard/keyboard.h"
#include "../task/process.h"
#include "../task/task.h"

void *sys_call1_print(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  if (!t)
  {
    return 0;
  }

  // 1. Get the pointer from user stack (Virtual Address)
  void *user_space_msg_buffer = task_get_stack_item(t, 0);

  // 2. Allocate kernel buffer
  char buf[1024]; // Simple static buffer for now, or use kernel_malloc

  // 3. Copy string from User Virtual to Kernel Physical
  int res = copy_string_from_task(t, user_space_msg_buffer, buf, sizeof(buf));
  if (res < 0)
  {
    return 0;
  }

  // 4. Print!
  print(buf);

  return 0;
}

void *sys_call2_getkey(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  int c = 0;
  while (1)
  {
    c = keyboard_pop();
    if (c > 0)
      break;
    task_block(&t->process->keyboard);
  }
  return (void *)c;
}