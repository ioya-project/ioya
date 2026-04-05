#ifndef FB_H_
#define FB_H_

#include <stddef.h>
#include <stdint.h>

void fb_setup();
uint64_t fb_get_base();
uint32_t fb_get_width();
uint32_t fb_get_height();

#endif