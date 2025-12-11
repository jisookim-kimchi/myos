#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../status.h"

#define PAGING_CACHE_DISABLED   0b00010000
#define PAGING_WRITE_THROUGH    0b00001000
#define PAGING_USER_ACCESS      0b00000100
#define PAGING_WRITEABLE        0b00000010
#define PAGING_PRESENT          0b00000001


#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_TOTAL_ENTRIES_PER_DIRECTORY 1024
#define PAGING_PAGE_SIZE_BYTES 4096

typedef struct paging_4gb_chunk
{
    uint32_t *directory_entry;
} paging_4gb_chunk_t;

paging_4gb_chunk_t* paging_new_4gb(uint8_t flags);
uint32_t *get_paging_4gb_dir(paging_4gb_chunk_t* chunk);
void paging_switch(paging_4gb_chunk_t* chunk);
void enable_paging();
int get_paging_indexes(void *virtual_addr, uint32_t *dir_index, uint32_t *table_index);
int paging_set(uint32_t *dir, void *virtual_addr, uint32_t val);
void paging_free_4gb(struct paging_4gb_chunk* chunk);
void* paging_align_address(void* ptr);
int paging_map_to(paging_4gb_chunk_t *directory, void *virt, void *phys, void *phys_end, int flags);
int paging_map(paging_4gb_chunk_t* directory, void* virt, void* phys, int flags);
int paging_map_range(paging_4gb_chunk_t* directory, void* virt, void* phys, int count, int flags);
uint32_t paging_get(uint32_t *directory, void *virt);
#endif