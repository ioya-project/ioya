#include <serial.h>
#include <stdio.h>
#include <string.h>

#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_ALT_FORM_FLAG 1

#include <nanoprintf.h>

void printf(const char *fmt, ...)
{
    char buffer[512];
    va_list args;

    va_start(args, fmt);
    npf_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    serial_write(buffer, strlen(buffer));
}

void sprintf(char *buffer, size_t size, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    npf_vsnprintf(buffer, size, fmt, args);
    va_end(args);
}

void panic(const char *fmt, ...)
{
    char buffer[512];
    va_list args;

    va_start(args, fmt);
    npf_vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    serial_write("PANIC: ", strlen("PANIC: "));
    serial_write(buffer, strlen(buffer));

    while (1) {
    }
}
