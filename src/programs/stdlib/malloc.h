#ifndef MYOS_MALLOC_H
#define MYOS_MALLOC_H

#include <stdint.h>
#include <stddef.h>

struct malloc_header
{
    uint32_t size;
    uint32_t is_free;
    struct malloc_header *next;
};

// Memory Management
void *malloc(size_t size);
void free(void *ptr);
void *sbrk(int amounts);


#endif
