#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct file_stat
{
    uint32_t size;
    unsigned int mode;
};

int sys_thread_create(void *entry_point, void *user_stack, int priority);
int sys_thread_exit(void);
int thread_create(void *entry_point, int priority);
int thread_exit(void);
void mutex_lock(volatile int *lock_ptr);
void mutex_unlock(volatile int *lock_ptr);

void print(const char *str);
void putchar(char c);
void print_int(int i);
void print_hex(uint32_t n);

char getkey();
void *malloc(size_t size);
void free(void *ptr);
int getkey_block();
void terminal_readline(char *out, int max, bool output_while_typing);
void sleep(int wait_ticks);
void itoa(int val, char *str);

int strncmp(const char *str1, const char *str2, int n);
int strlen(const char *str);
int strcmp(const char *str1, const char *str2);
int strcpy(char *dst, const char *src);
size_t ft_strlen(const char *str);
char *ft_strcpy(char *dest, const char *src);
bool is_digit(char c);
int to_numeric_digit(char c);
int atoi(const char *str);

int fopen(const char *filename, const char *mode);
int fread(int fd, void *ptr, uint32_t size, uint32_t nmemb);
int fwrite(void *ptr, uint32_t size, uint32_t nmemb, int fd);
int fclose(int fd);
int fstat(int fd, struct file_stat *stat);
int set_focus(int pid);

void exit(int status);
int wait_pid(int *status);
int exec(const char *filename);

#endif
