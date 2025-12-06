#include "gdt.h"
#include "../kernel.h"
#include "../config.h"
#include "../string/string.h"

void kernel_gdt_to_cpu_gdt(struct gdt* gdt, struct kernel_gdt* kernel_gdt, int total_entires)
{
    int i = 0;
    while (i < total_entires)
    {
        uint32_t limit = kernel_gdt[i].limit;
        uint8_t flags = 0x40; // 0x40 = 0100 0000 (G=0, D/B=1) - 1 byte granularity, 32-bit mode

        // Check if limit > 65536 (64KB)
        if (limit > 0xFFFF)
        {
            // Check alignment: if using 4KB pages, lower 12 bits must be 1s (0xFFF)
            if ((limit & 0xFFF) != 0xFFF)
            {
                panic("kernel_gdt_to_cpu_gdt: Invalid argument\n");
            }
            
            limit = limit >> 12; // divide by 4096 (4kB)
            flags = 0xC0; // 0xC0 = 1100 0000 (G=1, D/B=1) - 4KB granularity, 32-bit mode
        }

        gdt[i].limit_low = limit & 0xFFFF;
        gdt[i].base_low = kernel_gdt[i].base & 0xFFFF;
        gdt[i].base_middle = (kernel_gdt[i].base >> 16) & 0xFF;
        gdt[i].access = kernel_gdt[i].type;
        
        // Limit 16-19 bits + Flags
        //0100 0000
        gdt[i].granularity = (limit >> 16) & 0x0F;
        gdt[i].granularity |= flags; 
        
        gdt[i].base_high = (kernel_gdt[i].base >> 24) & 0xFF;
        i++;
    }
}