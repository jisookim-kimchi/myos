#include "process.h"
#include "../string/string.h"
#include "../task/process.h"
#include "../task/task.h"

void *sys_process_exit(struct interrupt_frame *frame)
{
  int code = (int)task_get_stack_item(get_cur_task(), 1);
  process_exit(code);
  return 0;
}

void *sys_process_wait(struct interrupt_frame *frame)
{
  int *status_ptr = (int *)task_get_stack_item(get_cur_task(), 1);
  int temp_status = 0;
  int res = process_wait(&temp_status);

  if (status_ptr && res >= 0)
  {
    copy_to_task(get_cur_task(), (void *)status_ptr, &temp_status, sizeof(int));
  }
  return (void *)res;
}

void* sys_process_exec(struct interrupt_frame* frame)
{
    char* filename = (char*)task_get_stack_item(get_cur_task(), 1);
    char buf[1024];
    int res = copy_string_from_task(get_cur_task(), filename, buf, sizeof(buf));
    if (res < 0)
    {
        return (void*)res;
    }
    
    struct process* proc = 0;
    res = process_load(buf, &proc);
    
    // if successful, return 0 (or pid?) 
    // process_load returns 0 on success.
    // let's return PID on success if possible, but process_load signature returns int error.
    // however, proc->id would be valid.
    if (res >= 0 && proc)
    {
        return (void*)(int)proc->id;
    }
    return (void*)res;
}
