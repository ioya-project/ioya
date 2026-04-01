#ifndef ASSERT_H
#define ASSERT_H

#include <stdio.h>

#define assert(cond)                                                                               \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            panic("assert failed: " #cond);                                                        \
        }                                                                                          \
    } while (0)

#endif