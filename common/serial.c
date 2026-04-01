#include <fb.h>
#include <pl011.h>
#include <serial.h>

static struct serial_funcs *funcs = NULL;

void serial_register(struct serial_funcs *func)
{
    func->next = funcs;
    funcs = func;
}

void serial_setup()
{
    fb_setup();
#ifdef QEMU
    pl011_setup();
#endif

    struct serial_funcs *func = funcs;
    while (func != NULL) {
        func->init();
        func = func->next;
    }
}

void serial_write(const void *buf, size_t len)
{
    struct serial_funcs *func = funcs;

    while (func != NULL) {
        func->write(buf, len);
        func = func->next;
    }
}