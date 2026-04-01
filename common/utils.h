#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

#define MAX_ERRNO 4095

#define ERR_PTR(err) ((void *)((int64_t)(err)))
#define IS_ERR(ptr) ((uintptr_t)(ptr) >= (uintptr_t) - MAX_ERRNO)

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))

static inline void write64(uintptr_t addr, uint64_t val)
{
    *(volatile uint64_t *)addr = val;
    __asm__ volatile("dsb st" ::: "memory");
}

static inline uint64_t read64(uintptr_t addr)
{
    uint64_t val = *(volatile uint64_t *)addr;
    __asm__ volatile("dsb ld" ::: "memory");
    return val;
}

static inline void write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
    __asm__ volatile("dsb st" ::: "memory");
}

static inline uint32_t read32(uintptr_t addr)
{
    uint32_t val = *(volatile uint32_t *)addr;
    __asm__ volatile("dsb ld" ::: "memory");
    return val;
}

static inline void write16(uintptr_t addr, uint16_t val)
{
    *(volatile uint16_t *)addr = val;
    __asm__ volatile("dsb st" ::: "memory");
}

static inline uint16_t read16(uintptr_t addr)
{
    uint16_t val = *(volatile uint16_t *)addr;
    __asm__ volatile("dsb ld" ::: "memory");
    return val;
}

static inline void write8(uintptr_t addr, uint8_t val)
{
    *(volatile uint8_t *)addr = val;
    __asm__ volatile("dsb st" ::: "memory");
}

static inline uint8_t read8(uintptr_t addr)
{
    uint8_t val = *(volatile uint8_t *)addr;
    __asm__ volatile("dsb ld" ::: "memory");
    return val;
}

#endif