#include "string.h"

int ft_strlen(const char *str)
{
    int i = 0;
    while(str[i] != '\0')
    {
        i++;
    }
    return i;
}

int ft_strnlen(const char *str, int len)
{
    int i = 0;
    while(i < len && str[i] != '\0')
    {
        i++;
    }
    return i;
}

bool ft_isdigit(char c)
{
    return (c >= '0' && c <= '9');
}

int ft_to_numeric_digit(char c)
{
    return c - '0';
}

int ft_atoi(const char *str)
{
    int result = 0;
    int i = 0;
    //int sign = 1;

    while (str[i] == ' ' || str[i] == '\t')
    {
        i++;
    }
    // while (str[i] == '+' || str[i] == '-')
    // {
    //     if (str[i] == '-')
    //     {
    //         // 음수는 지원하지 않음
    //         sign = -1;
    //     }
    //     i++;
    // }
    while (str[i] != '\0' && ft_isdigit(str[i]))
    {
        result = result * 10 + str[i] - '0';
        i++;
    }
    return result;
}

char *ft_strcpy(char *dest, const char *src)
{
    char *d = dest;
    while (*src)
    {
        *d++ = *src++;
    }
    *d = '\0'; // 널 종료 문자 추가
    return dest;
}

int ft_strcmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int ft_strnlen_terminator(const char* str, int max, char terminator)
{
    int i = 0;
    for(i = 0; i < max; i++)
    {
        if (str[i] == '\0' || str[i] == terminator)
            break;
    }

    return i;
}

int ft_istrncmp(const char* s1, const char* s2, int n)
{
    unsigned char u1, u2;
    while(n-- > 0)
    {
        u1 = ft_tolower((unsigned char)*s1++);
        u2 = ft_tolower((unsigned char)*s2++);

        // 2. 소문자화된 두 문자가 다르면, 그 차이값을 반환합니다
        if (u1 != u2)
        {
            return u1 - u2; 
        }
        if (u1 == '\0')
            return 0;
    }
    return 0;
}

char ft_tolower(char s1)
{
    if (s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}