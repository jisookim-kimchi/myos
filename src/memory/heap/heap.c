#include "heap.h"
#include "../../kernel.h"
#include "memory/memory.h"
#include "../../status.h"

static int heap_validate_table(void *ptr, void *end, heap_table_t *table)
{
    int res = 0;
    size_t table_size = (size_t)(end - ptr); //104857600 - 0x01000000 = 100MB
    size_t total_blocks = table_size / MYOS_HEAP_BLOCK_SIZE; //100MB / 4096 bytes = 25600
    if (table->total_size < total_blocks)
    {
        res = -MYOS_INVALID_ARG;
        goto out;
    }
out:
    return res;
}

//4096 bytes 배수인지 확인.
static bool heap_validate_alignment(void *ptr)
{
    if ((unsigned int)ptr % MYOS_HEAP_BLOCK_SIZE == 0)
        return true;
    else
        return false;
}

int heap_init(heap_t *heap, void *ptr, void *end, heap_table_t *table)
{
    int res = 0;

    if (heap_validate_alignment(ptr) == false || heap_validate_alignment(end) == false)
    {
        res = -MYOS_INVALID_ARG;
        goto out;
    }
    ft_memset(heap, 0, sizeof(heap_t));
    heap->start_address = ptr;
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if (res < 0)
        goto out;

    size_t table_size = (sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total_size);
    ft_memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);
out:
    return res;
}

uint32_t heap_calculate_required_blocks(size_t size)
{
    return (size + MYOS_HEAP_BLOCK_SIZE - 1) / MYOS_HEAP_BLOCK_SIZE;
}

// static uint32_t heap_calculate_required_blocks(size_t size)
// {
//     if (size % MYOS_HEAP_BLOCK_SIZE == 0)
//         return size;
//     else
//     {
//         size = size - (size % MYOS_HEAP_BLOCK_SIZE);
//         size += MYOS_HEAP_BLOCK_SIZE;
//         return size;
//     }
// }

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return  entry & 0b1111; 
}

int heap_get_start_block(heap_t *heap, uint32_t total_blocks)
{
    heap_table_t* heap_table = heap->table;
    int bc = 0;
    int bs = -1;
    for (size_t i = 0; i < heap_table->total_size; i++)
    {
        if (heap_get_entry_type(heap_table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            bc = 0;
            bs = -1;
            continue;
        }

        // if this is the first block
        if (bs == -1)
        {
            bs = i;
        }
        bc++;

        if (bc == total_blocks)
        {
            break;
        }
    }
    if (bs == -1)
    {
        return -MYOS_ERROR_NO_MEMORY;
    }

    return bs;
}

void *heap_block_to_addr(heap_t *heap, int start_block)
{
    if (!heap || start_block < 0)
        return NULL;
    if (start_block >= heap->table->total_size)
        return NULL;
    return (char*)heap->start_address + (start_block * MYOS_HEAP_BLOCK_SIZE);
}

int heap_address_to_block(heap_t *heap, void *addr)
{
    if (!heap || !addr)
        return -MYOS_INVALID_ARG;
    if (addr < heap->start_address)
        return -MYOS_INVALID_ARG;

    size_t offset = (size_t)((char*)addr - (char*)heap->start_address);
    return offset / MYOS_HEAP_BLOCK_SIZE;
}

void heap_mark_blocks_free(heap_t *heap, int start_block)
{
    if (!heap || start_block < 0)
        return ;
    heap_table_t* heap_table = heap->table;
    int cur = start_block;
    while (cur < (int)heap_table->total_size)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = heap_table->entries[cur];

        /* If this block is not marked as taken, stop. */
        if ((entry & HEAP_BLOCK_TABLE_ENTRY_TAKEN) == 0)
            break;
        /* Free this block */
        heap_table->entries[cur] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        cur++;
    }
}

// Mark blocks as taken in the heap table
void heap_mark_blocks_as_taken(heap_t *heap, int start_block, int total_blocks)
{
    if (!heap || start_block < 0)
        return ;    

    int end_block_index = (start_block + total_blocks) - 1;

    //first block and set taken flags
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if (total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    // Mark first block
    heap->table->entries[start_block] = entry;
    
    // Mark remaining blocks
    for (int i = start_block + 1; i < start_block + total_blocks; i++)
    {
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i < end_block_index)  // Not the last block
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
        heap->table->entries[i] = entry;
    }
}

void *heap_allocate_blocks(heap_t *heap, uint32_t total_blocks)
{
    void *addr = NULL;
    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }
    addr = heap_block_to_addr(heap, start_block);
    if (addr == NULL)
    {
        goto out;
    }
    heap_mark_blocks_as_taken(heap, start_block, total_blocks);

out:
    return addr;
}

void *my_malloc(heap_t *heap, size_t size)
{
    uint32_t total_blocks = heap_calculate_required_blocks(size);
    return heap_allocate_blocks(heap, total_blocks);
}

void my_free(heap_t *heap, void *ptr)
{
    if (ptr == NULL)
        return;
    // Calculate the block index and free the blocks
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}