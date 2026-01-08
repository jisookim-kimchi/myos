#include "process.h"
#include "../filesystem/file.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../string/string.h"
#include "elf.h"
#include "elfloader.h"
#include "task.h"
#include "../memory/paging/paging.h"

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

int process_map_virtual_memory(struct process *process)
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

void process_setup_arguments(struct process *process, const char *command_line)
{
  int argc = process_count_args(command_line);
  if (argc == 0)
  {
    return;
  }

  uint32_t virtual_stack_ptr = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
  uint32_t arg_vaddrs[64];

  for (int i = argc - 1; i >= 0; i--)
  {
    char *arg = get_token(command_line, i);
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
