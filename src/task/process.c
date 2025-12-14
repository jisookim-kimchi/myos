#include "process.h"
#include "../memory/memory.h"
#include "../filesystem/file.h"
#include "../string/string.h"
#include "../memory/heap/kernel_heap.h"
#include "../kernel.h"

// The current process that is running
struct process* cur_process = NULL;

static struct process* processes[MYOS_MAX_PROCESSES] = {};

static void process_init(struct process* process)
{
    ft_memset(process, 0, sizeof(struct process));
    // Keyboard buffer is zeroed by the memset above,
    // explicitly ensuring head/tail are 0 is redundant but safe.
    ft_memset(process->keyboard.key_buffer, 0, sizeof(process->keyboard.key_buffer));
    process->keyboard.head = 0;
    process->keyboard.tail = 0;
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
    //ELF file size  : what is ELF? : 
    proc->size = stat.size;
    return 0;
}

int process_load_data(const char* filename, struct process* process)
{
    int res = 0;
    res = process_load_binary(filename, process);
    return res;
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
    if (!t) 
    {
        kernel_free(stack_ptr);
        kernel_free(proc);
        return -MYOS_ERROR_NO_MEMORY;
    }

    process_init(proc);
    int res = process_load_data(filename, proc);
    if (res < 0)
    {
        kernel_free(t);
        kernel_free(stack_ptr);
        kernel_free(proc);
        return res;
    }
    ft_strlcpy(proc->filename, filename, sizeof(proc->filename));
    proc->id   = pid;
    proc->task = t;
    proc->stack = stack_ptr;
    
    int rc = process_map_memory(proc);
    if (rc < 0) 
    {
        kernel_free(t);
        kernel_free(stack_ptr);
        kernel_free(proc);
        return rc;               
    }
    processes[pid] = proc;
    *process = proc;
    return 0;
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