#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>

#define VIDEO_MEMORY_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define ERROR(value) (void *)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)

void kernel_main();
void terminal_putchar(char c, uint8_t color, size_t x, size_t y);
uint16_t terminal_make_char(char c, uint8_t color);
size_t strlen(const char *str);
void print(const char *str);
void init_terminal();
void terminal_write_char(char c, uint8_t color);
void print_int(int v);
void print_hex(uint32_t n);
void panic(const char *msg);
void change_to_kernel_page(void);
struct paging_4gb_chunk *paging_get_kernel_chunk(void);
void kernel_registers();
void enable_interrupts();
void disable_interrupts();
void halt();
#endif