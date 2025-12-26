#include "io.h"
#include "../filesystem/file.h"
#include "../kernel.h"
#include "../keyboard/keyboard.h"
#include "../memory/heap/kernel_heap.h"
#include "../task/process.h"
#include "../task/task.h"

void *sys_call1_print(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  if (!t)
  {
    return 0;
  }

  void *user_space_msg_buffer = task_get_stack_item(t, 1);
  char buf[1024];
  int res = copy_string_from_task(t, user_space_msg_buffer, buf, sizeof(buf));
  if (res < 0)
  {
    return 0;
  }

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

void *sys_call3_sleep(struct interrupt_frame *frame)
{
  // Index 1 because Index 0 is the Return Address
  uint32_t ticks = (uint32_t)(uintptr_t)task_get_stack_item(get_cur_task(), 1);

  task_sleep_until(ticks);

  return 0;
}

void *sys_call4_fopen(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  if (!t)
  {
    return 0;
  }

  // Index 1 and 2 because Index 0 is the Return Address
  void *filename_user_ptr = task_get_stack_item(t, 1);
  void *mode_user_ptr = task_get_stack_item(t, 2);

  char filename[1024];
  char mode[10];

  if (copy_string_from_task(t, filename_user_ptr, filename, sizeof(filename)) < 0)
  {
    return 0;
  }

  if (copy_string_from_task(t, mode_user_ptr, mode, sizeof(mode)) < 0)
  {
    return 0;
  }

  int res = fopen(filename, mode);
  return (void *)(uintptr_t)res;
}

void *sys_call5_fread(struct interrupt_frame *frame)
{
  struct task *t = get_cur_task();
  if (!t)
  {
    return 0;
  }

  // fread(void *ptr, uint32_t size, uint32_t nmemb, int fd)
  int fd = (int)(uintptr_t)task_get_stack_item(t, 1);
  void *buffer = task_get_stack_item(t, 2);
  uint32_t size = (uint32_t)(uintptr_t)task_get_stack_item(t, 3);
  uint32_t nmemb = (uint32_t)(uintptr_t)task_get_stack_item(t, 4);

  uint32_t total_size = size * nmemb;
  if (total_size == 0 || total_size > 1024 * 1024)
  {
    return 0;
  }

  char *kernel_buf = kernel_malloc(total_size);
  if (!kernel_buf)
  {
    return 0;
  }

  int res = fread(fd, kernel_buf, size, nmemb);
  if (res > 0)
  {
    copy_to_task(t, kernel_buf, buffer, res * size);
  }
  kernel_free(kernel_buf);
  return (void *)(uintptr_t)res;
}