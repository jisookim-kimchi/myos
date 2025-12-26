#ifndef HEAP_H
#define HEAP_H

#include "../../config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE  0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

typedef struct heap_table
{
    HEAP_BLOCK_TABLE_ENTRY *entries;
    size_t total_size;
}   heap_table_t;

typedef struct heap
{
    heap_table_t* table;    // struct 제거, typedef 이름 사용
    void *start_address;    // 이거 절대 건드리면 안됨.
}   heap_t;

int heap_init(heap_t *heap, void *ptr, void *end, heap_table_t *table);
void *my_malloc(heap_t *heap, size_t size);
void my_free(heap_t *heap, void *ptr);
void heap_mark_blocks_as_taken(heap_t *heap, int start_block, int total_blocks);
void *heap_allocate_blocks(heap_t *heap, uint32_t total_blocks);
int heap_get_start_block(heap_t *heap, uint32_t total_blocks);
void *heap_block_to_addr(heap_t *heap, int start_block);    
int heap_address_to_block(heap_t *heap, void *addr);
void heap_mark_blocks_free(heap_t *heap, int start_block);
uint32_t heap_calculate_required_blocks(size_t size);

#endif