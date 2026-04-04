#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

char *strcat(char *dest, const char *src)
{
    char *d = dest;
    while (*d)
        d++;
    while ((*d++ = *src++))
        ;
    return dest;
}

char *strchr(const char *s, int c)
{
    char ch = (char)c;

    while (*s) {
        if (*s == ch)
            return (char *)s;
        s++;
    }

    return 0;
}

char *strsep(char **string, const char *delim)
{
    char *s;
    char *token;

    if (!string || !*string)
        return 0;

    s = *string;
    token = s;

    for (;; s++) {
        char c = *s;
        const char *d = delim;

        if (c == 0) {
            *string = 0;
            return token;
        }

        while (*d) {
            if (c == *d) {
                *s = 0;
                *string = s + 1;
                return token;
            }
            d++;
        }
    }
}

#ifndef RALLY
char *strdup(const char *s)
{
    size_t len;
    char *out;

    len = strlen(s);

    out = malloc(len);
    if (!out)
        return NULL;

    strcpy(out, (char *)s);

    return out;
}
#endif

int strcmp(const char *str1, const char *str2)
{
    while (*str1 && *str1 == *str2) {
        ++str1;
        ++str2;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

long strtol(const char *str, int base)
{
    long result = 0;
    bool neg = 0;

    if (*str == '+' || *str == '-') {
        if (*str == '-') {
            neg = true;
        }
        str++;
    }

    if (base == 0) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            base = 16;
            str += 2;
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            str += 2;
        }
    }

    while (str) {
        int digit;

        if (*str >= '0' && *str <= '9') {
            digit = *str - '0';
        } else if (*str >= 'a' && *str <= 'f') {
            digit = *str - 'a' + 10;
        } else if (*str >= 'A' && *str <= 'F') {
            digit = *str - 'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        result = result * base + digit;
        str++;
    }

    return neg ? -result : result;
}

size_t strlen(const char *str)
{
    size_t len = 0;

    for (; *str != '\0'; str++, len++)
        ;

    return len;
}

size_t strnlen(const char *str, size_t maxlen)
{
    size_t len = 0;

    while (len < maxlen && str[len] != '\0')
        len++;

    return len;
}

void memset(void *m, int c, size_t n)
{
    while (n--)
        *(unsigned char *)m++ = (unsigned char)c;
}

#define BIG_BLOCK_SIZE (sizeof(long) << 3)
#define MEDIUM_BLOCK_SIZE (sizeof(long) << 2)
#define SMALL_BLOCK_SIZE (sizeof(long))

void memcpy(void *d, const void *s, size_t len)
{
    char *dst = d;
    const char *src = s;
    long *aligned_dst;
    const long *aligned_src;

    if (len >= BIG_BLOCK_SIZE &&
        !(((long)src & (sizeof(long) - 1)) | ((long)dst & (sizeof(long) - 1)))) {
        aligned_dst = (long *)dst;
        aligned_src = (long *)src;

        while (len >= BIG_BLOCK_SIZE) {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len -= BIG_BLOCK_SIZE;
        }

        while (len >= MEDIUM_BLOCK_SIZE) {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len -= MEDIUM_BLOCK_SIZE;
        }

        while (len >= SMALL_BLOCK_SIZE) {
            *aligned_dst++ = *aligned_src++;
            len -= SMALL_BLOCK_SIZE;
        }

        dst = (char *)aligned_dst;
        src = (char *)aligned_src;
    }

    while (len--)
        *dst++ = *src++;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;

    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }

        ++p1;
        ++p2;
    }

    return 0;
}