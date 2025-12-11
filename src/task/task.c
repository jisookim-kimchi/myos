#include "task.h"
#include "../memory/memory.h"
#include "../status.h"
#include "../config.h"
#include "process.h"
#include "../memory/heap/kernel_heap.h"
#include "../string/string.h"
#include "../memory/paging/paging.h"
#include "../kernel.h"

struct task* task_cur = NULL;
struct task* task_head = NULL;
struct task* task_tail = NULL;

int init_task(struct task* task, struct process* proc)
{
    ft_memset(task, 0 ,sizeof(struct task));
    
    // Fix: Add PAGING_WRITEABLE so kernel can write to stack after switching CR3
    task->page_directory = paging_new_4gb(PAGING_PRESENT | PAGING_USER_ACCESS);
    if (!task->page_directory)
    {
        return -MYOS_ERROR_NO_MEMORY;
    }
    task->regs.ip = MYOS_PROGRAM_VIRTUAL_ADDRESS; 
    task->regs.ss = MYOS_USER_DATA_SEGMENT;
    task->regs.cs = MYOS_USER_CODE_SEGMENT;
    task->regs.esp = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    task->process = proc;
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

struct task* new_task(struct process* proc)
{
    struct task *task = (struct task*)kernel_malloc(sizeof(struct task));
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
    }
    return task;
}

struct task* get_cur_task()
{
    return task_cur;
}

struct task* get_next_task()
{
    if (task_cur->next == NULL)
    {
        return task_head;
    }
    return task_cur->next;
}

void task_delete(struct task* task)
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

int task_switch(struct task* task)
{
    task_cur = task;
    paging_switch(task->page_directory);
    return 0;
}

//change to user page and switch to argument task's directory not change task_cur
//use when we want to check another task's memory space now task means another process
int task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

//change to user page and switch to current task
int task_page()
{
    user_registers();
    task_switch(task_cur);
    return 0;
}

// Index 0 is the return address. Index 1 is arg1, Index 2 is the second arg, and so on.
// Since data is stacked on ESP, we use the index to access a specific value.

void* task_get_stack_item(struct task* task, int index)
{
    void *result = 0;

    // Calculate virtual address of item on user stack
    uint32_t virtual_addr = task->regs.esp + (index * sizeof(uint32_t));
    
    // Paging_get requires ALIGNED address, otherwise it returns error/index 0
    uint32_t aligned_virtual_addr = virtual_addr & 0xFFFFF000;
    uint32_t offset = virtual_addr & 0xFFF;

    // Get the physical address using the task's page directory
    uint32_t entry = paging_get(task->page_directory->directory_entry, (void*)aligned_virtual_addr);
    
    // Extract physical page address (mask out flags)
    uint32_t phys_page = entry & 0xFFFFF000;
    uint32_t phys_addr = phys_page + offset;
    
    // Since Kernel Identity Maps all memory, we can access Physical Address directly
    result = (void*)(*(uint32_t*)phys_addr);
    
    return result;
}

void task_run_first_ever_task()
{
    if (!task_cur)
    {
        panic("task_run_first_ever_task(): No current task exists!\n");
    }

    task_switch(task_head);
    task_return(&task_head->regs);
}

//save registers
void save_registers(struct interrupt_frame *frame)
{
    if (!get_cur_task())
    {
        panic("No current task to save\n");
    }

    struct task *task = get_cur_task();
    task->regs.ip = frame->ip;
    task->regs.cs = frame->cs;
    task->regs.flags = frame->flags;
    task->regs.esp = frame->esp;
    task->regs.ss = frame->ss;
    task->regs.eax = frame->eax;
    task->regs.ebp = frame->ebp;
    task->regs.ebx = frame->ebx;
    task->regs.ecx = frame->ecx;
    task->regs.edi = frame->edi;
    task->regs.edx = frame->edx;
    task->regs.esi = frame->esi;
}

// to make print sys call we need to copy string from task's memory space to kernel
// it means, we need to pass the string from user space ---> kernel space
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max)
{
    if (max >= PAGING_PAGE_SIZE_BYTES)
    {
        return -MYOS_INVALID_ARG;
    }

    int res = 0;
    char* tmp = kernel_zero_alloc(max);
    if (!tmp)
    {
        res = -MYOS_ERROR_NO_MEMORY;
        goto out;
    }

    uint32_t* task_directory = task->page_directory->directory_entry;
    // 1. 기존 매핑 정보 저장 (나중에 복구 위해) 특정한 책의 장을 저장해놨고.
    uint32_t old_entry = paging_get(task_directory, tmp);
    
    // 2. 커널 버퍼(tmp)를 사용자 페이지 테이블에도 매핑 (책의 해당 주소 장을 찾기...)
    // 근데 안에 paging_set함수가있어서 tmp로 낙서하게됨..
    paging_map(task->page_directory, tmp, tmp, PAGING_WRITEABLE | PAGING_PRESENT | PAGING_USER_ACCESS);
    
    // 3. 사용자 페이지로 전환 (User World로 이동)
    paging_switch(task->page_directory);
    
    // 4. 데이터 복사 (User Virtual -> Kernel Temp)
    // 이제 tmp는 두 세계 모두에서 보입니다.
    ft_strncpy(tmp, virtual, max);
    
    // 5. 커널 페이지로 복귀
    change_to_kernel_page();

    // 6. 원래 페이지값으로 복구.
    res = paging_set(task_directory, tmp, old_entry);
    if (res < 0)
    {
        res = -MYOS_IO_ERROR;
        goto out_free;
    }

    // 7. 최종 목적지(phys)로 이동
    ft_strncpy(phys, tmp, max);

out_free:
    kernel_free(tmp);
out:
    return res;
}
