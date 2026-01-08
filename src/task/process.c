#include "process.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../string/string.h"

struct process *cur_process = NULL;

struct process *processes[MYOS_MAX_PROCESSES] = {};

static void process_init(struct process *process)
{
  ft_memset(process, 0, sizeof(struct process));

  process->parent_id = -1; // No parent
  process->exit_code = 0;  // Default exit code
}

struct process *get_cur_process()
{
  return cur_process;
}

struct process *get_process(int pid)
{
  if (pid < 0 || pid >= MYOS_MAX_PROCESSES)
  {
    return NULL;
  }
  return processes[pid];
}

void set_cur_process(struct process *process)
{
  cur_process = process;
}

int get_process_free_slot()
{
  for (int i = 0; i < MYOS_MAX_PROCESSES; i++)
  {
    if (processes[i] == NULL)
    {
      return i;
    }
  }
  return -MYOS_ERROR_NO_MEMORY;
}

int process_load_for_slot(const char *filename, struct process **process, int pid)
{
  if (!filename || !process)
  {
    return -MYOS_INVALID_ARG;
  }

  if (get_process(pid) != NULL)
  {
    return -MYOS_IO_ERROR;
  }

  struct process *proc = (struct process *)kernel_malloc(sizeof(*proc));
  if (!proc)
  {
    return -MYOS_ERROR_NO_MEMORY;
  }

  void *stack_ptr = kernel_malloc(MYOS_USER_PROGRAM_STACK_SIZE);
  if (!stack_ptr)
  {
    kernel_free(proc);
    return -MYOS_ERROR_NO_MEMORY;
  }

  struct task *t = new_task(proc);
  if (!t)
  {
    kernel_free(stack_ptr);
    kernel_free(proc);
    return -MYOS_ERROR_NO_MEMORY;
  }

  process_init(proc);
  if (get_cur_process())
  {
    proc->parent_id = get_cur_process()->id;
  }

  char binary_name[1024];
  ft_strlcpy(binary_name, filename, sizeof(binary_name));
  char *first_space = ft_strchr(binary_name, ' ');
  if (first_space)
  {
    *first_space = '\0';
  }

  int res = process_load_data(binary_name, proc);
  if (res < 0)
  {
    task_delete(t);
    kernel_free(stack_ptr);
    kernel_free(proc);
    return res;
  }

  // set the entry point of the process into ip register
  // ip register : instruction pointer : the address of the next instruction
  // so when iret is called...(in this moment when it back to user space)
  // it will start executing from the entry point
  if (proc->elf_entry_point)
  {
    proc->task->regs.ip = (uint32_t)proc->elf_entry_point;
  }

  ft_strlcpy(proc->filename, binary_name, sizeof(proc->filename));
  proc->id = pid;
  proc->task = t;
  proc->stack = stack_ptr;

  //if elf, has virtual address, use it
  if (proc->virtual_end_address)
  {
    proc->bin_end_addr = proc->virtual_end_address;
  }
  else
  {
    void *base = proc->virtual_base_address ? proc->virtual_base_address : (void *)MYOS_PROGRAM_VIRTUAL_ADDRESS;
    proc->bin_end_addr = (void *)((uint32_t)base + proc->size);
  }
  proc->cur_end_heap = paging_align_address(proc->bin_end_addr);

  //binding with virtual
  int rc = process_map_virtual_memory(proc);
  if (rc < 0)
  {
    task_delete(t);
    kernel_free(stack_ptr);
    kernel_free(proc);
    return rc;
  }

  process_setup_arguments(proc, filename);
  processes[pid] = proc;
  *process = proc;
  return 0;
}

