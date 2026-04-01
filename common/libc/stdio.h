#ifndef STDIO_H_
#define STDIO_H_

#include <stdarg.h>
#include <stddef.h>

void printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void sprintf(char *buffer, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
void panic(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif