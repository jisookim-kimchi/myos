#ifndef STRING_H
#define STRING_H

#include <stdbool.h>

int ft_strlen(const char *str);
int ft_strnlen(const char *str, int len);
bool ft_isdigit(char c);
int ft_to_numeric_digit(char c);
int ft_atoi(const char *str);
int ft_strcmp(const char *s1, const char *s2);
char *ft_strcpy(char *dest, const char *src);
int strnlen_terminator(const char* str, int max, char terminator);
int istrncmp(const char* s1, const char* s2, int n);
char ft_tolower(char s1);

#endif