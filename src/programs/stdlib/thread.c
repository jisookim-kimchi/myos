#include "stdlib.h"

#define THREAD_STACK_SIZE 4096
#define THREAD_MAX_NUM 8

extern void mutex_lock(volatile int *lock_ptr);
extern void mutex_unlock(volatile int *lock_ptr);

__attribute__((section(".thread_stack")))
static uint8_t thread_stack_pool[THREAD_MAX_NUM][THREAD_STACK_SIZE];

int thread_create(void *entry_point, int priority)
{
    static int thread_idx = 0;
    if (thread_idx >= THREAD_MAX_NUM)
    {
        return -1;
    }
    uint32_t *stack_top = (uint32_t *)(thread_stack_pool[thread_idx] + THREAD_STACK_SIZE);
    print("stack_top: ");
    stack_top--;
    *stack_top = (uint32_t)thread_exit;
    print_hex((uintptr_t)stack_top);
    print("\n");
    thread_idx++;
    return sys_thread_create(entry_point, (void*)stack_top, priority);
}

int thread_exit(void)
{
    return sys_thread_exit();
}

int thread_join(int thread_id)
{
    return sys_thread_join(thread_id);
}
