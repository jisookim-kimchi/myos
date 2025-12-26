#include "malloc.h"

/*
    first fit based malloc
    TODO: coalescing
*/
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
            // 공간이 너무 많이 남으면 자른다!
            // 최소한 (헤더 + 1바이트) 공간은 남아야 자를 가치가 있음
            if (cur->size >= size + sizeof(struct malloc_header) + 1)
            {
                struct malloc_header *new_free = (struct malloc_header *)((char *)(cur + 1) + size);
                new_free->size = cur->size - size - sizeof(struct malloc_header);
                new_free->is_free = 1;
                new_free->next = cur->next;
                new_free->prev = cur;

                if (cur->next)
                    cur->next->prev = new_free;
                cur->next = new_free;
                cur->size = size;
            }

            cur->is_free = 0;
            return (void *)(cur + 1);
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
    new_chunk->prev = prev;

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
    if (!ptr)
        return;

    struct malloc_header *chunk = (struct malloc_header *)ptr - 1;
    chunk->is_free = 1; // Free
    
    // 빈 주소 합치기!
    // 1. 뒤쪽(next) 합치기 
    if (chunk->next && chunk->next->is_free)
    {
        chunk->size += sizeof(struct malloc_header) + chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next)
            chunk->next->prev = chunk;
    }

    // 2. 앞쪽(prev) 합치기
    if (chunk->prev && chunk->prev->is_free)
    {
        chunk->prev->size += sizeof(struct malloc_header) + chunk->size;
        chunk->prev->next = chunk->next;
        if (chunk->next)
            chunk->next->prev = chunk->prev;
    }
}