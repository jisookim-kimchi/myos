#ifndef MYOS_STDLIB_H
#define MYOS_STDLIB_H

#include "malloc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void print(const char *str);
void putchar(char c);
void sleep(uint32_t ms);
void itoa(int n, char s[]);
void print_hex(uint32_t n);
size_t ft_strlen(const char *str);
char *ft_strcpy(char *dest, const char *src);

// System Calls
typedef uint32_t FILE_STAT_MODE;
struct file_stat
{
  uint32_t size;
  FILE_STAT_MODE mode;
};

int getkey();
int fopen(const char *filename, const char *mode);
int fread(int fd, void *ptr, uint32_t size, uint32_t nmemb);
int fclose(int fd);
int fwrite(void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fstat(int fd, struct file_stat *stat);
void exit(int status);
int wait_pid(int *status);
int exec(const char *filename);
#endif