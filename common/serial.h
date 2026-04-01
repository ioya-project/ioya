#ifndef SERIAL_H_
#define SERIAL_H_

#include <stddef.h>
#include <stdint.h>

struct serial_funcs {
    char *name;
    void (*init)();
    void (*write)(const char *buf, size_t len);
    struct serial_funcs *next;
};

void serial_register(struct serial_funcs *func);
void serial_setup();

void serial_write(const void *buf, size_t len);

#endif