#ifndef BLOCK_DEVICE_H_
#define BLOCK_DEVICE_H_

#include <stddef.h>
#include <stdint.h>

typedef struct block_dev block_dev;

struct block_dev {
    const char *name;
    uint32_t block_size;
    uint32_t block_count;

    int (*read)(struct block_dev *dev, uint64_t lba, uint32_t count, void *buf);
    int (*write)(struct block_dev *dev, uint64_t lba, uint32_t count, const void *buf);

    void *priv;
};

void block_dev_setup();
int block_dev_register(struct block_dev *dev);
int block_dev_get_count();
struct block_dev *block_dev_get(int index);

#endif