#include "paging.h"
#include "../heap/kernel_heap.h"
#include "../../kernel.h"
#include "../../config.h"

static uint32_t* cur_dir = 0;

void paging_load_dir(uint32_t *dir);

//각 프로세스마다 4GB 가상 메모리 공간을 할당해주는 함수
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t offset = 0;
    uint32_t *dir = kernel_zero_alloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_DIRECTORY);
    if (dir == NULL)
    {
        return NULL;
    }
    for (uint32_t i = 0; i < PAGING_TOTAL_ENTRIES_PER_DIRECTORY; i++)
    {
        uint32_t entry = 0;
        if (offset < MYOS_MEMORY_BOUNDARY)
        {
            uint32_t *table_entry = kernel_zero_alloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
            if (table_entry == NULL)
            {
                return NULL;
            }
            for (uint32_t j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++)
            {
                uint32_t current_offset = offset + (j * PAGING_PAGE_SIZE_BYTES);
                uint32_t final_flags = flags;
                if (current_offset < MYOS_MEMORY_BOUNDARY)
                {
                    // Force bit 2 (User/Supervisor) to 0 for kernel zone
                    final_flags &= ~PAGING_USER_ACCESS;
                }
                table_entry[j] = current_offset | final_flags;
            }
            uint32_t directory_flags = flags | PAGING_WRITEABLE;
            if (offset < MYOS_MEMORY_BOUNDARY)
            {
                directory_flags &= ~PAGING_USER_ACCESS;
            }
            entry = (uint32_t)table_entry | directory_flags;
        }
        else
        {
            // for user space
            entry = 0;
        }

        dir[i] = entry;
        offset += PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE_BYTES;
    }
    paging_4gb_chunk_t *chunk_4gb = kernel_zero_alloc(sizeof(paging_4gb_chunk_t));
    if (chunk_4gb == NULL)
    {
        return NULL;
    }
    chunk_4gb->directory_entry = dir;
    return chunk_4gb;
}

uint32_t *get_paging_4gb_dir(paging_4gb_chunk_t* chunk)
{
    return chunk->directory_entry;
}

//페이지 디렉토리를 바꾸는 함수, 페이징 시스템에서 "어떤 가상 주소 공간을 사용할지" 선택하는 역할입니다.
void paging_switch(paging_4gb_chunk_t* dir)
{
    paging_load_dir(dir->directory_entry);
    cur_dir = dir->directory_entry;
}

bool is_paging_aligned(void *addr)
{
    return ((uint32_t)addr % PAGING_PAGE_SIZE_BYTES) == 0;
}

//virtual_addr 가 페이지 정렬되어 있는지 확인하고, 디렉토리 및 테이블 인덱스를 반환
int get_paging_indexes(void *virtual_addr, uint32_t *dir_index, uint32_t *table_index)
{
    int res = 0;
    if (!is_paging_aligned(virtual_addr) || dir_index == NULL || table_index == NULL)
    {
        res = -MYOS_INVALID_ARG;
        goto out;
    }

    *dir_index = ((uint32_t)virtual_addr / (PAGING_TOTAL_ENTRIES_PER_DIRECTORY * PAGING_PAGE_SIZE_BYTES));
    *table_index = ((uint32_t)virtual_addr % (PAGING_TOTAL_ENTRIES_PER_DIRECTORY * PAGING_PAGE_SIZE_BYTES)) / PAGING_PAGE_SIZE_BYTES;
out:
    return res;
}

//
uint32_t paging_get(uint32_t *directory, void *virt)
{
    uint32_t dir_index = 0;
    uint32_t table_index = 0;
    get_paging_indexes(virt, &dir_index, &table_index);
    uint32_t entry = directory[dir_index];
    uint32_t *table = (uint32_t*)(entry & 0xfffff000); //low bits(12bits) set to 0 because they are flags
    return table[table_index];
}

void paging_free_4gb(struct paging_4gb_chunk* chunk)
{
    for (int i = 0; i < 1024; i++)
    {
        uint32_t entry = chunk->directory_entry[i];
        uint32_t* table = (uint32_t*)(entry & 0xfffff000); //low bits(12bits) set to 0 because they are flags
        kernel_free(table);
    }

    kernel_free(chunk->directory_entry);
    kernel_free(chunk);
}

void* paging_align_address(void* ptr)
{
    if ((uint32_t)ptr % PAGING_PAGE_SIZE_BYTES)
    {
        return (void*)((uint32_t)ptr + PAGING_PAGE_SIZE_BYTES - ((uint32_t)ptr % PAGING_PAGE_SIZE_BYTES));
    }

    return ptr;
}

int paging_set(uint32_t *dir, void *virtual_addr, uint32_t val)
{
    if (!is_paging_aligned(virtual_addr) || dir == NULL)
    {
        return -MYOS_INVALID_ARG;
    }

    uint32_t dir_index = 0;
    uint32_t table_index = 0;
    int res = get_paging_indexes(virtual_addr, &dir_index, &table_index);
    if (res < 0)
    {
        return res;
    }
    uint32_t dir_entry = dir[dir_index];
    if ((dir_entry & PAGING_PRESENT) == 0) // Check if page table is missing
    {
        uint32_t *new_table = kernel_zero_alloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        if (new_table == NULL)
        {
            return -MYOS_IO_ERROR; // Out of memory
        }

        dir[dir_index] = (uint32_t)new_table | PAGING_PRESENT | PAGING_WRITEABLE | PAGING_USER_ACCESS;
        dir_entry = dir[dir_index];
    }
    uint32_t *table = (uint32_t *)(dir_entry & 0xFFFFF000);
    table[table_index] = val;

    return 0;
}

//virt addr mapping with phys addr 
int paging_map(paging_4gb_chunk_t* directory, void* virt, void* phys, int flags)
{
    if (((unsigned int)virt % PAGING_PAGE_SIZE_BYTES))
    {
        return -MYOS_INVALID_ARG;
    }
    //하위비트 제거하면 페이지 정렬된 주소가 됨 왜? 4096바이트 단위로 정렬
    uint32_t phys_addr = (uint32_t)phys & 0xFFFFF000;
    return paging_set(directory->directory_entry, virt, phys_addr | flags);
}

int paging_map_range(paging_4gb_chunk_t* directory, void* virt, void* phys,int count, int flags)
{
    int res = 0;
    for (int i = 0; i < count; i++)
    {
        res = paging_map(directory, virt, phys, flags);
        if (res < 0)
            break;
        virt = (void *)((uintptr_t)virt + PAGING_PAGE_SIZE_BYTES);
        phys = (void *)((uintptr_t)phys + PAGING_PAGE_SIZE_BYTES);
    }
    return res;
}

static int check_page_alignment(void *addr)
{
    return ((uintptr_t)addr % PAGING_PAGE_SIZE_BYTES) ? -MYOS_INVALID_ARG : 0;
}

//연속된 메모리 
int paging_map_to(paging_4gb_chunk_t *directory, void *virt, void *phys, void *phys_end, int flags)
{
    if (check_page_alignment(virt) ||
        check_page_alignment(phys) ||
        check_page_alignment(phys_end))
    {
        return -MYOS_INVALID_ARG;
    }

    if ((uint32_t)phys_end < (uint32_t)phys)
    {
        return -MYOS_IO_ERROR;
    }

    uint32_t total_bytes = phys_end - phys;
    int total_pages = total_bytes / PAGING_PAGE_SIZE_BYTES;
    return paging_map_range(directory, virt, phys, total_pages, flags);
}