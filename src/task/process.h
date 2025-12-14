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

    struct keyboard keyboard;
};

int process_map_binary(struct process *proc);
int process_load_for_slot(const char *filename, struct process **out_proc, int pid);
int process_map_memory(struct process* process);
int process_load_data(const char* filename, struct process* process);
struct process* get_process(int pid);
struct process* get_cur_process();
int process_load(const char *filename, struct process **process);
#endif