#include "cache_check.h"
#include "../paging/paging.h"
#include "../../kernel_print.h"

static uint32_t test_buffer[1024] __attribute__((aligned(4096)));


/*
    @brief : Cache speed test function
    @details : testbuffer compare speed when cache is enabled and disabled
*/
void test_cache_speed(void)
{
    void *test_virt = (void*)test_buffer;
    void *test_phys = (void*)test_buffer;

    paging_4gb_chunk_t *kernel_chunk = paging_get_kernel_chunk();

    paging_map(kernel_chunk, test_virt, test_phys, PAGING_PRESENT | PAGING_WRITEABLE | PAGING_CACHE_DISABLED );
    paging_switch(kernel_chunk);

    for (volatile int i = 0; i < 10000; i++)
    {
        test_buffer[i & 1023] = i;
    }

    //uint64_t start_time = read_tsc();
    for (volatile int i = 0; i < 100000; i++)
    {
        test_buffer[i & 1023] = i;
        (void)test_buffer[i & 1023];
    }
    //uint64_t end_time = read_tsc();
    //uint32_t cache_off_cycles = (uint32_t)(end_time - start_time);

    paging_map(kernel_chunk, test_virt, test_phys, PAGING_PRESENT | PAGING_WRITEABLE);
    paging_switch(kernel_chunk);

    for (volatile int i = 0; i < 10000; i++)
    {
        test_buffer[i & 1023] = i;
    }
    //start_time = read_tsc();
    for (volatile int i = 0; i < 100000; i++)
    {
        test_buffer[i & 1023] = i;
        (void)test_buffer[i & 1023];
    }
    //end_time = read_tsc();
    //uint32_t cache_on_cycles = (uint32_t)(end_time - start_time);

    // Restore
    paging_map(kernel_chunk, test_virt, test_phys, PAGING_PRESENT | PAGING_WRITEABLE);
    paging_switch(kernel_chunk);

    // Output
    // print("\n === Cache Benchmark ===\n");
    // print("Cache OFF:   ");
    // print_int(cache_off_cycles);
    // print("\nCache ON: ");
    // print_int(cache_on_cycles);
    // print("\n====================== \n");

    uint32_t dir_idx = 0, table_idx = 0;
    get_paging_indexes(test_virt, &dir_idx, &table_idx);
    uint32_t pte = paging_get(kernel_chunk->directory_entry, test_virt);
    uint32_t phys_addr = (pte & 0xFFFFF000) | ((uint32_t)test_virt & 0xFFF);

    print("\n === Page Table  ===\n");
    print("Virtual Address: ");
    print_hex((uint32_t)test_virt);
    print(" -> Page Directory Index: ");
    print_int(dir_idx);
    print(", Page Table Index: ");
    print_int(table_idx);
    print("\n -> Physical Address: ");
    print_hex(phys_addr);
    print(" [Flags: ");
    print((pte & PAGING_PRESENT) ? "PRESENT" : "NOT_PRESENT");
    print((pte & PAGING_WRITEABLE) ? ", READ_WRITE" : ", READ_ONLY");
    print((pte & PAGING_USER_ACCESS) ? ", USER" : ", SUPERVISOR");
    if (pte & PAGING_CACHE_DISABLED) print(", CACHE_DISABLED");
    print("]\n=============================\n");
}

