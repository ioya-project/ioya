#include <heap.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>

#define HAVE_MORECORE 1
#define HAVE_MMAP 0
#define MORECORE sbrk
#define MORECORE_CONTIGUOUS 1
#define MALLOC_ALIGNMENT 16
#define ABORT panic("malloc: internal error\n")
#define NO_MALLINFO 1
#define NO_MALLOC_STATS 1
#define LACKS_FCNTL_H 1
#define LACKS_SYS_MMAN_H 1
#define LACKS_SYS_PARAM_H 1
#define LACKS_SYS_TYPES_H 1
#define LACKS_STRINGS_H 1
#define LACKS_STRING_H 1
#define LACKS_STDLIB_H 1
#define LACKS_SCHED_H 1
#define LACKS_TIME_H 1
#define LACKS_UNISTD_H 1
#define MALLOC_FAILURE_ACTION panic("malloc: out of memory\n");

static void *sbrk(intptr_t inc)
{
    if (inc < 0)
        return (void *)-1;

    return heap_alloc(inc);
}