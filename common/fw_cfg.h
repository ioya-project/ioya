#ifndef FW_CFG_
#define FW_CFG_

#include <stdint.h>

void fw_cfg_dma_read(volatile void *buf, int selector, int length);
void fw_cfg_dma_write(volatile void *buf, int selector, int length);
uint16_t fw_cfg_find_file(const char *name);

#endif