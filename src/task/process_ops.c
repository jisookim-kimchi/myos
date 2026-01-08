#include "process.h"
#include "../system_control/system_control.h"
#include "../memory/heap/kernel_heap.h"

int process_exit(int exit_code)
{
  struct process *p = get_cur_process();
  if (!p)
  {
    return -MYOS_INVALID_ARG;
  }

  p->exit_code = exit_code;
  for (int i = 0; i < MYOS_MAX_FILE_DESCRIPTORS; i++)
  {
    if (p->allocations[i])
    {
      kernel_free(p->allocations[i]);
      p->allocations[i] = NULL;
    }
  }

  p->task->state = TASK_ZOMBIE;
  if (p->parent_id >= 0)
  {
    struct process *parent = get_process(p->parent_id);
    if (parent && parent->task)
    {
      task_wakeup(parent);
    }
  }

  struct task *next_task = get_next_task();
  if (next_task)
  {
    task_switch(next_task);
    task_return(&next_task->regs);
  }

  while (1)
  {
    enable_interrupts();
    halt();
  }
}


int process_wait(int *status)
{
  struct process *current = get_cur_process();

  while (1)
  {
    int have_children = 0;
    for (int i = 0; i < MYOS_MAX_PROCESSES; i++)
    {
      struct process *proc = processes[i];
      if (!proc)
      {
        continue;
      }

      if (proc->parent_id == current->id)
      {
        have_children = 1;
        if (proc->task->state == TASK_ZOMBIE)
        {
          if (status)
          {
            *status = proc->exit_code;
          }
          int pid = proc->id;

          task_delete(proc->task);
          kernel_free(proc->stack);
          kernel_free(proc);
          processes[i] = NULL;

          return pid;
        }
      }
    }

    if (!have_children)
    {
      return -MYOS_INVALID_ARG;
    }

    task_block(current);
  }
}


void *process_sbrk(struct process *proc, int amounts)
{
  if (amounts == 0)
  {
    return proc->cur_end_heap;
  }

  void *old_break = proc->cur_end_heap;
  uint32_t new_break = (uint32_t)old_break + amounts;

  if (new_break > 0x1C000000)
  {
    return (void *)-1;
  }

  if (amounts > 0)
  {
    uint32_t diff = (uint32_t)paging_align_address((void *)new_break) -
                    (uint32_t)paging_align_address(old_break);
    if (diff > 0)
    {
      for (uint32_t addr = (uint32_t)paging_align_address(old_break); addr < (uint32_t)paging_align_address((void *)new_break); addr += PAGING_PAGE_SIZE_BYTES)
      {
        void *phys = kernel_zero_alloc(PAGING_PAGE_SIZE_BYTES);
        if (!phys)
        {
          return (void *)-1;
        }
        paging_map_to(proc->task->page_directory, (void *)addr, phys,
                      (void *)(addr + PAGING_PAGE_SIZE_BYTES),
                      PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
      }
    }
  }
  proc->cur_end_heap = (void *)new_break;
  return old_break;
}

int process_load(const char *filename, struct process **process)
{
  int process_slot = get_process_free_slot();
  if (process_slot < 0)
  {
    return process_slot;
  }

  return process_load_for_slot(filename, process, process_slot);
}
