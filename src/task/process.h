#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../config.h"
#include "task.h"
#include "../keyboard/keyboard.h"

struct process
{
    uint16_t id;
    char filename[MYOS_MAX_PATH_LENGTH];
    
    struct task *task;

    //in a process you can malloc ans ask to the kernel memory
    //to track allocated memory
    void *allocations[MYOS_MAX_ALLOCATIONS];

    //프로세스 binary image(코드)의 시작 물리 주소.
    void *ptr;

    void *stack;

    uint32_t size;
    
    // The program break (current end of the heap)
    void* cur_end_heap;
    // The absolute end of the program data (initial break addr)
    void* bin_end_addr;

    struct keyboard keyboard;
    uint16_t parent_id;
    int exit_code;

    void* entry_point; //ELF header entry point
};

int process_map_binary(struct process *proc);
int process_load_for_slot(const char *filename, struct process **out_proc, int pid);
int process_map_memory(struct process* process);
int process_load_data(const char* filename, struct process* process);
struct process* get_process(int pid);
struct process* get_cur_process();
void set_cur_process(struct process* process);
void* process_sbrk(struct process* proc, int increment);
int process_load(const char *filename, struct process **process);
int process_exit(int exit_code);
int process_wait(int* status);
void process_setup_arguments(struct process* process, const char* command_line);
#endif