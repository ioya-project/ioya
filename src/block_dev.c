#include <block_dev.h>
#include <qcom_ufs_blk.h>
#include <virtio_blk.h>

#define MAX_BLOCK_DEV 8

static struct block_dev *devices[MAX_BLOCK_DEV];
static int dev_count;

void block_dev_setup()
{
#ifdef QEMU
    virtio_blk_init();
#else
    qcom_ufs_blk_init();
#endif
}

int block_dev_register(struct block_dev *dev)
{
    if (dev_count >= MAX_BLOCK_DEV)
        return -1;

    devices[dev_count++] = dev;
    return 0;
}

int block_dev_get_count()
{
    return dev_count;
}

struct block_dev *block_dev_get(int index)
{
    if (index < 0 || index >= dev_count)
        return NULL;

    return devices[index];
}