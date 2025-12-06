#ifndef GDT_H
#define GDT_H

#include <stdint.h>

//cpu version
struct gdt
{
    uint16_t limit_low;      // Limit (bits 0-15)
    uint16_t base_low;       // Base (bits 0-15)
    uint8_t base_middle;     // Base (bits 16-23)
    uint8_t access;          // Access Byte cod or data or write or read or execute and so on
    uint8_t granularity;     // Limit (bits 16-19) & Flags
    uint8_t base_high;       // Base (bits 24-31)
} __attribute__((packed));

//kernel version
struct kernel_gdt
{
    uint32_t base;
    uint32_t limit;
    uint8_t type; //equivalent to access byte
} __attribute__((packed));

void gdt_load(struct gdt* gdt, int size);
void kernel_gdt_to_cpu_gdt(struct gdt* gdt, struct kernel_gdt* kernel_gdt, int total_entires);

#endif