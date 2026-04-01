#ifndef HOOK_H_
#define HOOK_H_

#include <stdint.h>

#define TRAMPOLINE_SIZE 16

#define TRAMPOLINE_JUMP(where, target)                                                             \
    do {                                                                                           \
        *(uint32_t *)((where) + 0x0) = 0x58000050;                                                 \
        *(uint32_t *)((where) + 0x4) = 0xd61f0200;                                                 \
        *(uint32_t *)((where) + 0x8) = (uint64_t)(target) & 0xFFFFFFFF;                            \
        *(uint32_t *)((where) + 0xC) = (uint64_t)(target) >> 32;                                   \
    } while (0);

struct hook {
    void *target;
    void *hook_fn;
    void *original;
    uint8_t saved[TRAMPOLINE_SIZE * 2];
};

#define INSTALL_HOOK(target, hook_fn, hook) install_hook(#target, target, hook_fn, hook)

void install_hook(const char *name, void *target, void *hook_fn, struct hook *t);

#endif