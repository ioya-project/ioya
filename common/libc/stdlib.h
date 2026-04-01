#ifndef MALLOC_H_
#define MALLOC_H_

#ifndef RALLY
#include <stddef.h>

void *malloc(size_t);
void free(void *);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
#endif

#endif