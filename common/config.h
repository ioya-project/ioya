#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

#define MAX_CONFIG_STRING 128

struct boot_config {
    char kernel[MAX_CONFIG_STRING];
    char devicetree[MAX_CONFIG_STRING];
    char ramdisk[MAX_CONFIG_STRING];
    char cmdline[608];
    uint64_t load_address;
    uint64_t memory_size;
};

#endif