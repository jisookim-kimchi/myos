#include "task.h"
#include "../config.h"
#include "../kernel.h"
#include "../memory/heap/kernel_heap.h"
#include "../memory/memory.h"
#include "../memory/paging/paging.h"
#include "../string/string.h"
#include "../timer/timer.h"
#include "process.h"

struct task *task_cur = NULL;
struct task *task_head = NULL;
struct task *task_tail = NULL;

int init_task(struct task *task, struct process *proc)
{
  ft_memset(task, 0, sizeof(struct task));

  // Fix: Add PAGING_WRITEABLE so kernel can write to stack after switching CR3
  task->page_directory =
      paging_new_4gb(PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
  if (!task->page_directory)
  {
    return -MYOS_ERROR_NO_MEMORY;
  }
  task->regs.ip = MYOS_PROGRAM_VIRTUAL_ADDRESS;
  task->regs.ss = MYOS_USER_DATA_SEGMENT;
  task->regs.cs = MYOS_USER_CODE_SEGMENT;
  task->regs.esp = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
  task->regs.flags = 0x202;
  task->process = proc;
  task->state = TASK_RUNNING;
  return 0;
}

void task_add_head(struct task *t)
{
  t->next = task_head;
  t->prev = NULL;
  if (task_head)
    task_head->prev = t;
  else
    task_tail = t;
  task_head = t;
}

void task_add_tail(struct task *t)
{
  t->next = NULL;
  t->prev = task_tail;
  if (task_tail)
    task_tail->next = t;
  else
    task_head = t;
  task_tail = t;
}

struct task *new_task(struct process *proc)
{
  struct task *task = (struct task *)kernel_malloc(sizeof(struct task));
  if (!task)
  {
    return NULL;
  }
  if (init_task(task, proc) < 0)
  {
    kernel_free(task);
    return NULL;
  }
  task_add_tail(task);
  if (task_cur == NULL)
  {
    task_cur = task;
    print("Kernel: new_task: task_cur set to ");
    print_int((uint32_t)(uintptr_t)task_cur);
    print("\n");
  }
  return task;
}

struct task *get_cur_task()
{
  return task_cur;
}

struct task *get_next_task()
{
  if (!task_head)
    return 0;
  struct task *t = task_cur->next;
  if (!t)
    t = task_head;
  struct task *start_task = t;
  while (1)
  {
    if (t->state == TASK_RUNNING)
    {
      return t;
    }
    t = t->next;
    if (!t)
      t = task_head;
    if (t == start_task)
      break;
  }
  return task_cur;
}

void task_block(void *event_wait_channel)
{
  task_cur->state = TASK_BLOCKED;
  task_cur->event_wait_channel = event_wait_channel;
  struct task *next_task = get_next_task();
  if (next_task != task_cur)
  {
    task_switch(next_task);
  }
  else
  {
    enable_interrupts();
    halt();
  }
}

void task_delete(struct task *task)
{
  if (task->prev)
    task->prev->next = task->next;
  else
    task_head = task->next;

  if (task->next)
    task->next->prev = task->prev;
  else
    task_tail = task->prev;

  if (task == task_cur)
    task_cur = task->next;
  paging_free_4gb(task->page_directory);
  kernel_free(task);
}

int task_switch(struct task *task)
{
  task_cur = task;
  paging_switch(task->page_directory);
  set_cur_process(task->process);
  return 0;
}

// change to user page and switch to argument task's directory
// task_cur use when we want to check another task's memory space now task means
// another process
int task_page_task(struct task *task)
{
  user_registers();
  paging_switch(task->page_directory);
  return 0;
}

// change to user page and switch to current task
int task_page()
{
  user_registers();
  task_switch(task_cur);
  return 0;
}

// Index 0 is the return address. Index 1 is arg1, Index 2 is the second arg,
// and so on. Since data is stacked on ESP, we use the index to access a
// specific value.

void *task_get_stack_item(struct task *task, int index)
{
  void *result = 0;

  // Calculate virtual address of item on user stack
  uint32_t virtual_addr = task->regs.esp + (index * sizeof(uint32_t));

  // Security Check: Ensure the stack item is within the user zone (>= 256MB)
  if (virtual_addr < MYOS_MEMORY_BOUNDARY)
  {
      return NULL;
  }

  // Paging_get requires ALIGNED address, otherwise it returns error/index 0
  uint32_t aligned_virtual_addr = virtual_addr & 0xFFFFF000;
  uint32_t offset = virtual_addr & 0xFFF;

  // Get the physical address using the task's page directory
  uint32_t entry = paging_get(task->page_directory->directory_entry,
                              (void *)aligned_virtual_addr);

  // Extract physical page address (mask out flags)
  uint32_t phys_page = entry & 0xFFFFF000;
  uint32_t phys_addr = phys_page + offset;

  // Since Kernel Identity Maps all memory, we can access Physical Address
  // directly
  result = (void *)(*(uint32_t *)phys_addr);

  return result;
}

void task_run_first_ever_task()
{
  print("Kernel: task_run_first_ever_task: task_cur=");
  print_int((uint32_t)(uintptr_t)task_cur);
  print("\n");
  if (!task_cur)
  {
    panic("task_run_first_ever_task(): No current task exists!\n");
  }

  task_switch(task_head);
  task_return(&task_head->regs);
}

// save registers
void save_registers(struct interrupt_frame *frame)
{
  if (!get_cur_task())
  {
    return;
  }

  struct task *task = get_cur_task();
  task->regs.ip = frame->ip;
  task->regs.cs = frame->cs;
  task->regs.flags = frame->flags;
  
  // Only save SS and ESP if we came from user mode (Ring 3)
  // Low 2 bits of CS are the Current Privilege Level (CPL)
  if ((frame->cs & 0x03) == 0x03)
  {
      task->regs.esp = frame->esp;
      task->regs.ss = frame->ss;
  }
  
  task->regs.eax = frame->eax;
  task->regs.ebp = frame->ebp;
  task->regs.ebx = frame->ebx;
  task->regs.ecx = frame->ecx;
  task->regs.edi = frame->edi;
  task->regs.edx = frame->edx;
  task->regs.esi = frame->esi;
}

// to make print sys call we need to copy string from task's memory space to
// kernel it means, we need to pass the string from user space ---> kernel space
int copy_string_from_task(struct task *task, void *virtual, void *phys, int max)
{
  if (max <= 0 || !virtual || !phys)
    return -MYOS_INVALID_ARG;

  // Security Check: Ensure the virtual address is within the user zone (>= 256MB)
  if ((uint32_t)virtual < MYOS_MEMORY_BOUNDARY)
  {
      return -MYOS_INVALID_ARG;
  }

  paging_switch(task->page_directory);
  ft_strncpy(phys, virtual, max);
  ((char*)phys)[max-1] = '\0';
  paging_switch(paging_get_kernel_chunk());

  return 0;
}

int copy_to_task(struct task *task, void *kernel_buf, void *user_buf, int size)
{
  if (size <= 0 || !kernel_buf || !user_buf)
    return -MYOS_INVALID_ARG;

  // Security Check: Ensure the virtual address and the end of the buffer are within the user zone (>= 256MB)
  if ((uint32_t)user_buf < MYOS_MEMORY_BOUNDARY || ((uint32_t)user_buf + size) < MYOS_MEMORY_BOUNDARY)
  {
      return -MYOS_INVALID_ARG;
  }

  paging_switch(task->page_directory);
  ft_memcpy(user_buf, kernel_buf, size);
  paging_switch(paging_get_kernel_chunk());

  return 0;
}

void task_wakeup(void *event_wait_channel)
{
  struct task *t = task_head;
  if (!t)
    return;
  while (t)
  {
    if (t->state == TASK_BLOCKED &&
        t->event_wait_channel == event_wait_channel)
    {
      t->state = TASK_RUNNING;
      t->event_wait_channel = NULL;
    }
    t = t->next;
  }
}

void task_sleep_until(int wait_ticks)
{
  task_cur->sleep_expiry = get_tick() + wait_ticks;
  task_cur->state = TASK_BLOCKED;
  struct task *next = get_next_task();
  // 나 자신이면? (세상에 나뿐) -> 쉴 틈 없이 바로 리턴됨
  if (next == task_cur)
  {
    // 타이머가 깨워줄 때까지 여기서 무한 대기 (Busy Wait 아님, Halt Wait)
    while (task_cur->state == TASK_BLOCKED)
    {
      enable_interrupts();
      halt();
    }
  }
  else
  {
    task_switch(next);
  }
}

void task_run_scheduled_tasks(uint32_t cur_tick)
{
  struct task *task = task_head;
  if (!task)
    return;
  while (task)
  {
    if (task->state == TASK_BLOCKED && task->sleep_expiry > 0 &&
        cur_tick >= task->sleep_expiry)
    {
      task->state = TASK_RUNNING;
      task->sleep_expiry = 0;
    }
    task = task->next;
  }
}
