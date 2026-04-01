#ifndef STRING_H_
#define STRING_H_

#include <stddef.h>

char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *strchr(const char *s, int c);
char *strsep(char **string, const char *delim);
#ifndef RALLY
char *strdup(const char *s);
#endif
int strcmp(const char *str1, const char *str2);
long strtol(const char *str, int base);
size_t strlen(const char *str);
size_t strnlen(const char *str, size_t maxlen);

void memset(void *m, int c, size_t n);
void memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

#endif