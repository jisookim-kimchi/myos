#include "process.h"
#include "../filesystem/file.h"
#include "../system_control/system_control.h"
#include "../kernel_print.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../string/string.h"
#include "elf.h"
#include "elfloader.h"
#include "task.h"

// The current process that is running
struct process *cur_process = NULL;

static struct process *processes[MYOS_MAX_PROCESSES] = {};

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

static int process_load_binary(const char *filename, struct process *proc)
{
  int fd = fopen(filename, "r");
  if (fd < 0)
  {
    return -MYOS_IO_ERROR;
  }

  struct file_stat stat;
  if (fstat(fd, &stat) < 0)
  {
    fclose(fd);
    return -MYOS_IO_ERROR;
  }

  void *program_data_ptr = kernel_zero_alloc(stat.size);
  if (!program_data_ptr)
  {
    fclose(fd);
    return -MYOS_ERROR_NO_MEMORY;
  }

  if (fread(fd, program_data_ptr, stat.size, 1) < 0)
  {
    fclose(fd);
    kernel_free(program_data_ptr);
    return -MYOS_IO_ERROR;
  }

  fclose(fd);
  proc->ptr = program_data_ptr;
  proc->size = stat.size;
  return 0;
}

int process_load_data(const char *filename, struct process *process)
{
  struct elf_file elf = {0};
  ft_strcpy(elf.filename, filename);

  if (elfloader_load_elf(&elf) == MYOS_ALL_OK)
  {
    process->ptr = elf.physical_base_address;
    process->size = elf.in_memory_size;
    process->elf_entry_point = (void *)elf_get_header(&elf)->e_entry;
    process->virtual_base_address = elf.virtual_base_address;
    process->virtual_end_address = elf.virtual_end_address;
    if (elf.elf_memory)
    {
      kernel_free(elf.elf_memory);
    }
    return 0;
  }
  return process_load_binary(filename, process);
}

int process_map_memory(struct process *process)
{
  int res = 0;
  res = process_map_binary(process);
  if (res < 0)
  {
    return res;
  }

  paging_map_to(
      process->task->page_directory,
      (void *)MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack,
      paging_align_address(process->stack + MYOS_USER_PROGRAM_STACK_SIZE),
      PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
  return res;
}

int process_map_binary(struct process *proc)
{
  int res = 0;
  void *virtual_addr = proc->virtual_base_address
                           ? proc->virtual_base_address
                           : (void *)MYOS_PROGRAM_VIRTUAL_ADDRESS;
  res = paging_map_to(proc->task->page_directory, virtual_addr, proc->ptr,
                      paging_align_address(proc->ptr + proc->size),
                      PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
  return res;
}

int process_load_for_slot(const char *filename, struct process **process,
                          int pid)
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

  print("Kernel: Loaded ");
  print(binary_name);
  print(" - Entry: ");
  print_int((uint32_t)proc->elf_entry_point);
  print("\n");

  if (proc->elf_entry_point)
  {
    proc->task->regs.ip = (uint32_t)proc->elf_entry_point;
  }

  ft_strlcpy(proc->filename, binary_name, sizeof(proc->filename));
  proc->id = pid;
  proc->task = t;
  proc->stack = stack_ptr;

  if (proc->virtual_end_address)
  {
    proc->bin_end_addr = proc->virtual_end_address;
  }
  else
  {
    void *base = proc->virtual_base_address
                     ? proc->virtual_base_address
                     : (void *)MYOS_PROGRAM_VIRTUAL_ADDRESS;
    proc->bin_end_addr = (void *)((uint32_t)base + proc->size);
  }
  proc->cur_end_heap = paging_align_address(proc->bin_end_addr);

  int rc = process_map_memory(proc);
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

char *get_token(const char *command_line, int index)
{
  int count = 0;
  const char *start = NULL;
  bool in_arg = false;

  for (int i = 0; command_line[i] != '\0'; i++)
  {
    if (command_line[i] == ' ')
    {
      if (in_arg && count - 1 == index)
      {
        int len = &command_line[i] - start;
        char *token = kernel_malloc(len + 1);
        ft_strlcpy(token, start, len + 1);
        return token;
      }
      in_arg = false;
    }
    else if (!in_arg)
    {
      if (count == index)
      {
        start = &command_line[i];
      }
      count++;
      in_arg = true;
    }
  }

  if (in_arg && count - 1 == index)
  {
    int len = ft_strlen(start);
    char *token = kernel_malloc(len + 1);
    ft_strcpy(token, start);
    return token;
  }

  return NULL;
}

int process_count_args(const char *command_line)
{
  int count = 0;
  bool in_arg = false;

  for (int i = 0; command_line[i] != '\0'; i++)
  {
    if (command_line[i] == ' ')
    {
      in_arg = false;
    }
    else if (!in_arg)
    {
      count++;
      in_arg = true;
    }
  }
  return count;
}

void process_setup_arguments(struct process *process,
                             const char *command_line)
{
  int argc = process_count_args(command_line);

  print("Kernel: Setup Arguments - argc: ");
  print_int(argc);
  print("\n");

  if (argc == 0)
  {
    return;
  }

  uint32_t virtual_stack_ptr = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
  uint32_t arg_vaddrs[64];

  for (int i = argc - 1; i >= 0; i--)
  {
    char *arg = get_token(command_line, i);

    print("Kernel: Arg[");
    print_int(i);
    print("] = ");
    print(arg);
    print(" at Vaddr: ");
    print_int(virtual_stack_ptr);
    print("\n");

    if (!arg)
    {
      continue;
    }

    int arg_len = ft_strlen(arg) + 1;
    virtual_stack_ptr -= arg_len;
    copy_to_task(process->task, arg, (void *)virtual_stack_ptr, arg_len);
    arg_vaddrs[i] = virtual_stack_ptr;
    kernel_free(arg);
  }

  virtual_stack_ptr &= ~0x03;
  virtual_stack_ptr -= (sizeof(uint32_t) * (argc + 1));
  uint32_t argv_base = virtual_stack_ptr;

  for (int i = 0; i < argc; i++)
  {
    copy_to_task(process->task, &arg_vaddrs[i], (void *)argv_base + (i * 4), 4);
  }

  uint32_t null_ptr = 0;
  copy_to_task(process->task, &null_ptr, (void *)argv_base + (argc * 4), 4);

  virtual_stack_ptr -= 4;
  copy_to_task(process->task, &argv_base, (void *)virtual_stack_ptr, 4);
  virtual_stack_ptr -= 4;
  copy_to_task(process->task, &argc, (void *)virtual_stack_ptr, 4);

  // Dummy return address for C calling convention
  virtual_stack_ptr -= 4;
  uint32_t dummy_ret = 0;
  copy_to_task(process->task, &dummy_ret, (void *)virtual_stack_ptr, 4);

  process->task->regs.esp = virtual_stack_ptr;
}
