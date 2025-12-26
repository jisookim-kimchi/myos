#ifndef MYOS_STDLIB_H
#define MYOS_STDLIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "malloc.h"

void print(const char *str);
void putchar(char c);
void sleep(uint32_t ms);
void itoa(int n, char s[]);
void print_hex(uint32_t n);
char *ft_strcpy(char *dest, const char *src);

// System Calls
int getkey();
int fopen(const char *filename, const char *mode);
int fread(int fd, void *ptr, uint32_t size, uint32_t nmemb);

#endif