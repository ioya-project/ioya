#ifndef STDDEF_H_
#define STDDEF_H_

#define NULL ((void *)0)

#define offsetof(t, d) __builtin_offsetof(t, d)

typedef unsigned long size_t;
typedef long int ptrdiff_t;

#endif