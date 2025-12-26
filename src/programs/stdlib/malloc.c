#include "malloc.h"

struct malloc_header *malloc_head = NULL;

void *malloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    struct malloc_header *cur = malloc_head;
    struct malloc_header *prev = NULL;

    while (cur)
    {
        if (cur->is_free && cur->size >= size)
        {
            cur->is_free = 0;
            return (void *)(cur + 1); //jump 12bytes(header size) to the data
        }
        prev = cur;
        cur = cur->next;
    }

    uint32_t total_size = size + sizeof(struct malloc_header);
    struct malloc_header *new_chunk = (struct malloc_header *) sbrk(total_size);
    if (new_chunk == (void *)-1)
    {
        return NULL;
    }
    new_chunk->size = size;
    new_chunk->is_free = 0;
    new_chunk->next = NULL;
    
    if (prev)
    {
        prev->next = new_chunk;
    }
    else
    {
        malloc_head = new_chunk;
    }
    return (void *)(new_chunk + 1);
}

void free(void *ptr)
{
    if (ptr)
    {
        struct malloc_header *chunk = (struct malloc_header *)ptr - 1;
        chunk->is_free = 1;
    }
}