#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void* ft_memset(void* ptr, int value, size_t num);
void* ft_memcpy(void* dest, const void* src, size_t n);
int ft_memcmp(const void* ptr1, const void* ptr2, size_t num);
char *ft_strcpy(char *dest, const char *src);

#endif