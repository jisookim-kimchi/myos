#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

#define VIDEO_MEMORY_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)

void kernel_main();
void terminal_putchar(char c, uint8_t color, size_t x, size_t y);
uint16_t terminal_make_char(char c, uint8_t color);
size_t strlen(const char *str);
void print(const char *str);
void init_terminal();
void terminal_write_char(char c, uint8_t color);
void panic(const char* msg);
void change_to_kernel_page(void);
void kernel_registers();
#endif