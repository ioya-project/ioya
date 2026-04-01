#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

#define PA_TO_VA(x) ((void *)(x) - phys_base + virt_base)
#define VA_TO_PA(x) ((void *)(x) - virt_base + phys_base)

extern uint64_t virt_base;
extern uint64_t phys_base;

#endif