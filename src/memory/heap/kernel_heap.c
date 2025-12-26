#include "kernel_heap.h"
#include "heap.h"
#include "../../lock/lock.h"
#include "../../config.h"

#include "../../kernel.h"
#include "../../memory/memory.h"
#include <stdint.h>

struct heap kernel_heap;
struct heap_table kernel_heap_table;

//heap lock
spinlock_t kernel_heap_lock = 0;

void kernel_heap_init()
{
    int total_table_entries = MYOS_HEAP_SIZE_BYTES / MYOS_HEAP_BLOCK_SIZE;
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)HEAP_TABLE_ADDRESS;
    kernel_heap_table.total_size = total_table_entries;

    void *heap_end = (void*)(HEAP_ADDRESS + MYOS_HEAP_SIZE_BYTES);
    int res = heap_init(&kernel_heap, (void*)HEAP_ADDRESS, heap_end, &kernel_heap_table);
    if (res < 0)
    {
        print("Kernel heap creation failed!\n");
        return ;
    }
    print("Kernel heap initialized successfully.\n");
}

void kernel_free(void *ptr)
{
    spin_lock(&kernel_heap_lock);
    my_free(&kernel_heap, ptr);
    spin_unlock(&kernel_heap_lock);
}

void *kernel_malloc(size_t size)
{
    spin_lock(&kernel_heap_lock);
    void *ptr = my_malloc(&kernel_heap, size);
    spin_unlock(&kernel_heap_lock);
    return ptr;
}

void *kernel_zero_alloc(size_t size)
{
    void *ptr = my_malloc(&kernel_heap, size);
    if (ptr)
    {
        ft_memset(ptr, 0, size);
    }
    return ptr;
}