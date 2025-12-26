#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>

int sum(int a, int b);
void print(const char *str);
int getkey();
void putchar(char c);
void sleep(uint32_t tick);
void itoa(int n, char s[]);
void print_hex(uint32_t n);
int fopen(const char *filename, const char *mode);
int fread(int fd, void *buf, uint32_t size, uint32_t nmemb);
#endif