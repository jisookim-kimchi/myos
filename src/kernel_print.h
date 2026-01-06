#ifndef KERNEL_PRINT_H
#define KERNEL_PRINT_H

#define VIDEO_MEMORY_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25


#include <stddef.h>
#include <stdint.h>

extern uint16_t *video_memory;
extern uint16_t terminal_row;
extern uint16_t terminal_column;

void init_terminal();
void print(const char *str);
void print_int(int v);
void print_hex(uint32_t n);
void panic(const char *msg);
void terminal_write_char(char c, uint8_t color);
void terminal_putchar(char c, uint8_t color, size_t x, size_t y);

#endif
