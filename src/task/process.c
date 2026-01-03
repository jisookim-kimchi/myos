#include "process.h"
#include "../memory/memory.h"
#include "../filesystem/file.h"
#include "../string/string.h"
#include "../memory/heap/kernel_heap.h"
#include "../kernel.h"
#include "task.h"
#include "elfloader.h"
#include "elf.h"

// The current process that is running
struct process* cur_process = NULL;

static struct process* processes[MYOS_MAX_PROCESSES] = {};

static void process_init(struct process* process)
{
    ft_memset(process, 0, sizeof(struct process));
    
    process->parent_id = -1; // No parent
    process->exit_code = 0;  // Default exit code
}

struct process* get_cur_process()
{
    return cur_process;
}

struct process* get_process(int pid)
{
    if(pid < 0 || pid >= MYOS_MAX_PROCESSES)
    {
        return NULL;
    }
    return processes[pid];
}

void set_cur_process(struct process* process)
{
    cur_process = process;
}

int get_process_free_slot()
{
    for (int i = 0; i < MYOS_MAX_PROCESSES; i++)
    {
        if (processes[i] == NULL)
            return i;
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

int process_load_data(const char* filename, struct process* process)
{
    struct elf_file elf = {0};
    ft_strcpy(elf.filename, filename);

    if(elfloader_load_elf(&elf) == MYOS_ALL_OK)
    {
        process->ptr = elf.physical_base_address;
        process->size = elf.in_memory_size;
        process->elf_entry_point = (void*)elf_get_header(&elf)->e_entry;

        if (elf.elf_memory)
        {
            kernel_free(elf.elf_memory);
        }
        return MYOS_ALL_OK;
    }
    return process_load_binary(filename, process);
}

int process_map_memory(struct process* process)
{
    int res = 0;
    res = process_map_binary(process);
    if (res < 0)
    {
        return res;
    }
    paging_map_to(process->task->page_directory, (void*)MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack+MYOS_USER_PROGRAM_STACK_SIZE), PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
    return res;
}

/* -------------------------------------------------------------
 *  프로세스가 로드한 바이너리를 가상 주소에 매핑한다.
 * ------------------------------------------------------------- */
int process_map_binary(struct process *proc)
{
    int res = 0;
    res = paging_map_to(proc->task->page_directory, 
                        (void*) MYOS_PROGRAM_VIRTUAL_ADDRESS,
                        proc->ptr,
                        paging_align_address(proc->ptr + proc->size),
                        PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
    return res;
}

/*
    process_slot : process id
    process : process pointer
    filename : process binary filename
*/int process_load_for_slot(const char *filename, struct process **process, int pid)
{
    if (!filename || !process)
        return -MYOS_INVALID_ARG;
    
    if (get_process(pid) != NULL)
        return -MYOS_IO_ERROR;

    struct process *proc = (struct process *)kernel_malloc(sizeof(*proc));
    if (!proc)
        return -MYOS_ERROR_NO_MEMORY;
    void *stack_ptr = kernel_malloc(MYOS_USER_PROGRAM_STACK_SIZE);
    if (!stack_ptr) 
    {
        kernel_free(proc);
        return -MYOS_ERROR_NO_MEMORY;
    }

    struct task *t = new_task(proc);
    // print("Kernel: process_load_for_slot: new_task returned ");
    // print_int((uint32_t)(uintptr_t)t);
    // print("\n");

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
    
    // print("Kernel: Created Process PID ");
    // print_int(pid);
    // print(" Parent PID ");
    // print_int(proc->parent_id);
    // print("\n");

    char binary_name[1024];
    ft_strlcpy(binary_name, filename, sizeof(binary_name));
    char* first_space = ft_strchr(binary_name, ' ');
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

    if (proc->elf_entry_point)
    {
        proc->task->regs.ip = (uint32_t)proc->elf_entry_point;
    }

    ft_strlcpy(proc->filename, binary_name, sizeof(proc->filename));
    proc->id   = pid;
    proc->task = t;
    proc->stack = stack_ptr;

    //셋팅 256MB이후부터 프로그램이 로드됩니다.
    //그리고 프로그램이 로드된 위치를 bin_end_addr에 저장합니다.
    //그리고 4096바이트 단위로 페이지를 매핑해서 cur_end_heap에저장.
    //ex bin_end_addr 가 256라면? 256+4096 라면?
    //cur_end_heap은 4096올림해서 8192가됨.
    proc->bin_end_addr = (void*)(MYOS_PROGRAM_VIRTUAL_ADDRESS + proc->size);
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

    // 0MB ~ 256 MB : kernel
    // 256MB ~ 448 MB : user space //data code heap
    // 448MB is kind of an arbitrary boundary between user space and user stack
    // 448MB ~ 512MB: user stack
    if (new_break > 0x1C000000) 
    {
        return (void *)-1;
    }

    if (amounts > 0) 
    {
        uint32_t diff = (uint32_t)paging_align_address((void *)new_break) - (uint32_t)paging_align_address(old_break);
        if (diff > 0) 
        {
            for (uint32_t addr = (uint32_t)paging_align_address(old_break); addr < (uint32_t)paging_align_address((void *)new_break); addr += PAGING_PAGE_SIZE_BYTES) 
            {
                void *phys = kernel_zero_alloc(PAGING_PAGE_SIZE_BYTES);
                if (!phys) 
                {
                    return (void *)-1;
                }
                paging_map_to(proc->task->page_directory, (void *)addr, phys, (void *)(addr + PAGING_PAGE_SIZE_BYTES), PAGING_PRESENT | PAGING_USER_ACCESS | PAGING_WRITEABLE);
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
            //because parent is waiting for child to exit
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

int process_wait(int* status)
{
    struct process* current = get_cur_process();

    while(1)
    {
        int have_children = 0;
        for (int i = 0; i < MYOS_MAX_PROCESSES; i++)
        {
            struct process* proc = processes[i];
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


/*
    handle argc, argv
*/

char* get_token(const char* command_line, int index)
{
    int count = 0;
    const char* start = NULL;
    bool in_arg = false;

    for (int i = 0; command_line[i] != '\0'; i++)
    {
        if (command_line[i] == ' ')
        {
            if (in_arg && count - 1 == index)
            {
                int len = &command_line[i] - start;
                char* token = kernel_malloc(len + 1);
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
        char* token = kernel_malloc(len + 1);
        ft_strcpy(token, start);
        return token;
    }

    return NULL;
}

int process_count_args(const char* command_line)
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

//현재 커널버퍼에 이미 command_line이 복사되어있으니까.... execve에서 이미 복사했음.
void process_setup_arguments(struct process* process, const char* command_line)
{
    int argc = process_count_args(command_line);
    if (argc == 0)
        return;
        
    uint32_t virtual_stack_ptr = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    uint32_t arg_vaddrs[64];

    // copy arguments to the top of the user stack
    for (int i = argc - 1; i >= 0; i--)
    {
        char *arg = get_token(command_line, i);
        if (!arg)
            continue;
        
        int arg_len = ft_strlen(arg) + 1;
        virtual_stack_ptr -= arg_len;
        /*
        copy_to_task 커널에서 유저영역으로 복사해야함.
        prco->task는 shell은부모 여기서 prco->task는 walter.bin이겟지
        호출하는순간 커널이 cr3 레지스터의 값을 자식의 지도로 스위치
        이제 cpu는 자식의 지도를 보고 물건을 배달하지.
        배달이 끄탄면 원래 부모의 것으로 cr3를 스위치.
        */
        copy_to_task(process->task, arg, (void*)virtual_stack_ptr, arg_len);
        arg_vaddrs[i] = virtual_stack_ptr;
        kernel_free(arg);
    }
    
    // align stack to 4 bytes
    virtual_stack_ptr &= ~0x03;

    // allocate space for argv array (pointers + NULL terminator)
    virtual_stack_ptr -= (sizeof(uint32_t) * (argc + 1)); 
    uint32_t argv_base = virtual_stack_ptr; 
    
    // copy the pointers to user stack
    for (int i = 0; i < argc; i ++)
    {
        copy_to_task(process->task, &arg_vaddrs[i], (void*)argv_base + (i * 4), 4);
    }
    
    // add NULL terminator to argv array
    uint32_t null_ptr = 0;
    copy_to_task(process->task, &null_ptr, (void*)argv_base + (argc * 4), 4);

    // push argv and argc for main(int argc, char** argv)
    virtual_stack_ptr -= 4;
    copy_to_task(process->task, &argv_base, (void*)virtual_stack_ptr, 4);
    virtual_stack_ptr -= 4;
    copy_to_task(process->task, &argc, (void*)virtual_stack_ptr, 4);

    // set ESP to point to argc
    process->task->regs.esp = virtual_stack_ptr;
}
