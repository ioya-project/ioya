#ifndef INJECTOR_H_
#define INJECTOR_H_

#include <stddef.h>

void *mach_o_inject_rally(void *image, void *rally, size_t rally_size);

#endif