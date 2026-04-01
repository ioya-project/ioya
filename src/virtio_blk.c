#include <assert.h>
#include <utils.h>
#include <virtio.h>
#include <virtio_blk.h>

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
};

static struct virtq_desc desc[VIRTIO_QUEUE_SIZE] __attribute__((aligned(64)));
static struct virtq_avail avail __attribute__((aligned(64)));
static struct virtq_used used __attribute__((aligned(64)));

static struct virtio_blk_req req __attribute__((aligned(64)));
static uint8_t status;

static uint16_t last_used;

static int virtio_read(struct block_dev *dev, uint64_t lba, uint32_t count, void *buf)
{
    if (lba + count > dev->block_count)
        return -1;

    req.type = VIRTIO_BLK_T_IN;
    req.reserved = 0;
    req.sector = lba;
    status = 0xFF;

    desc[0].addr = (uint64_t)&req;
    desc[0].len = sizeof(req);
    desc[0].flags = VIRTQ_DESC_F_NEXT;
    desc[0].next = 1;

    desc[1].addr = (uint64_t)buf;
    desc[1].len = count * 512;
    desc[1].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
    desc[1].next = 2;

    desc[2].addr = (uint64_t)&status;
    desc[2].len = 1;
    desc[2].flags = VIRTQ_DESC_F_WRITE;
    desc[2].next = 0;

    avail.ring[avail.idx % VIRTIO_QUEUE_SIZE] = 0;
    avail.idx++;

    write32(VIRTIO_QUEUE_NOTIFY, 0);

    while (used.idx == last_used) {
        __asm__ volatile("dsb ld" ::: "memory");
    }

    last_used++;

    return status;
}

static int virtio_write(struct block_dev *dev, uint64_t lba, uint32_t count, const void *buf)
{
    if (lba + count > dev->block_count)
        return -1;

    req.type = VIRTIO_BLK_T_OUT;
    req.reserved = 0;
    req.sector = lba;
    status = 0xFF;

    desc[0].addr = (uint64_t)&req;
    desc[0].len = sizeof(req);
    desc[0].flags = VIRTQ_DESC_F_NEXT;
    desc[0].next = 1;

    desc[1].addr = (uint64_t)buf;
    desc[1].len = count * 512;
    desc[1].flags = VIRTQ_DESC_F_NEXT;
    desc[1].next = 2;

    desc[2].addr = (uint64_t)&status;
    desc[2].len = 1;
    desc[2].flags = VIRTQ_DESC_F_WRITE;
    desc[2].next = 0;

    avail.ring[avail.idx % VIRTIO_QUEUE_SIZE] = 0;
    avail.idx++;

    write32(VIRTIO_QUEUE_NOTIFY, 0);

    while (used.idx == last_used) {
        __asm__ volatile("dsb ld" ::: "memory");
    }

    last_used++;

    return status;
}

static struct block_dev virtio_dev = {
    .name = "virtio-blk",
    .block_size = 0,
    .block_count = 0,
    .read = virtio_read,
    .write = virtio_write,
    .priv = NULL,
};

void virtio_blk_init()
{
    uint32_t val = read32(VIRTIO_MAGIC);
    assert(val == 0x74726976 && "Invalid VirtIO Magic");

    val = read32(VIRTIO_DEVICE_ID);
    assert(val == 2 && "Invalid VirtIO Device ID");

    val = read32(VIRTIO_VERSION);
    assert(val == 2 && "Invalid VirtIO Version");

    last_used = 0;

    write32(VIRTIO_STATUS, 0);
    write32(VIRTIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    write32(VIRTIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    write32(VIRTIO_DRIVER_FEATURES, 0);
    write32(VIRTIO_STATUS,
            VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

    write32(VIRTIO_QUEUE_SEL, 0);

    val = read32(VIRTIO_QUEUE_NUM_MAX);
    assert(val != 0 && "VIRTIO_QUEUE_NUM_MAX == 0");

    write32(VIRTIO_QUEUE_NUM, VIRTIO_QUEUE_SIZE);

    write32(VIRTIO_QUEUE_DESC_LOW, (uintptr_t)&desc);
    write32(VIRTIO_QUEUE_DESC_HIGH, 0);
    write32(VIRTIO_QUEUE_AVAIL_LOW, (uintptr_t)&avail);
    write32(VIRTIO_QUEUE_AVAIL_HIGH, 0);
    write32(VIRTIO_QUEUE_USED_LOW, (uintptr_t)&used);
    write32(VIRTIO_QUEUE_USED_HIGH, 0);
    write32(VIRTIO_QUEUE_READY, 1);
    write32(VIRTIO_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
                               VIRTIO_STATUS_DRIVER_OK | VIRTIO_STATUS_FEATURES_OK);

    volatile struct virtio_blk_config *cfg = (void *)(VIRTIO_CONFIG_LOW);

    virtio_dev.block_count = cfg->capacity;
    virtio_dev.block_size = cfg->blk_size;

    block_dev_register(&virtio_dev);
}