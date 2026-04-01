#include <heap.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <utils.h>

extern char _heap_start;
extern char _heap_end;

static uintptr_t heap_ptr = 0;
static uintptr_t heap_end = 0;

void heap_init()
{
    heap_ptr = (uintptr_t)&_heap_start;
    heap_end = (uintptr_t)&_heap_end;
}

void *heap_alloc(size_t size)
{
    uintptr_t cur = ALIGN_UP(heap_ptr, 64);
    uintptr_t next = cur + size;

    if (next > heap_end)
        return NULL;

    heap_ptr = next;

    return (void *)cur;
}