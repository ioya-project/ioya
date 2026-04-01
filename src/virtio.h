#ifndef VIRTIO_H_
#define VIRTIO_H_

#include <stdint.h>

#define VIRTIO_BASE (0x0A003E00)
#define VIRTIO_MAGIC (VIRTIO_BASE + 0x00)
#define VIRTIO_VERSION (VIRTIO_BASE + 0x04)
#define VIRTIO_DEVICE_ID (VIRTIO_BASE + 0x08)
#define VIRTIO_DEVICE_FEATURES (VIRTIO_BASE + 0x10)
#define VIRTIO_DEVICE_FEATURES_SEL (VIRTIO_BASE + 0x14)
#define VIRTIO_DRIVER_FEATURES (VIRTIO_BASE + 0x20)
#define VIRTIO_DRIVER_FEATURES_SEL (VIRTIO_BASE + 0x24)
#define VIRTIO_QUEUE_SEL (VIRTIO_BASE + 0x30)
#define VIRTIO_QUEUE_NUM_MAX (VIRTIO_BASE + 0x34)
#define VIRTIO_QUEUE_NUM (VIRTIO_BASE + 0x38)
#define VIRTIO_QUEUE_READY (VIRTIO_BASE + 0x44)
#define VIRTIO_QUEUE_NOTIFY (VIRTIO_BASE + 0x50)
#define VIRTIO_STATUS (VIRTIO_BASE + 0x70)

#define VIRTIO_QUEUE_DESC_LOW (VIRTIO_BASE + 0x080)
#define VIRTIO_QUEUE_DESC_HIGH (VIRTIO_BASE + 0x084)
#define VIRTIO_QUEUE_AVAIL_LOW (VIRTIO_BASE + 0x090)
#define VIRTIO_QUEUE_AVAIL_HIGH (VIRTIO_BASE + 0x094)
#define VIRTIO_QUEUE_USED_LOW (VIRTIO_BASE + 0x0a0)
#define VIRTIO_QUEUE_USED_HIGH (VIRTIO_BASE + 0x0a4)
#define VIRTIO_CONFIG_LOW (VIRTIO_BASE + 0x100)
#define VIRTIO_CONFIG_HIGH (VIRTIO_BASE + 0x104)

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2

#define VIRTIO_QUEUE_SIZE 8

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[0];
} __attribute__((packed));

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[0];
} __attribute__((packed));

struct virtio_blk_config {
    uint64_t capacity;
    uint32_t size_max;
    uint32_t seg_max;

    struct virtio_blk_geometry {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;

    uint32_t blk_size;

    struct virtio_blk_topology {
        uint8_t physical_block_exp;
        uint8_t alignment_offset;
        uint16_t min_io_size;
        uint32_t opt_io_size;
    } topology;

    uint8_t writeback;

    uint8_t unused[3];

    uint32_t max_discard_sectors;
    uint32_t max_discard_seq;
    uint32_t discard_sector_alignment;

    uint32_t max_write_zeroes_sectors;
    uint32_t max_write_zeroes_seq;
    uint8_t write_zeros_may_unmap;

    uint8_t unused1[3];

    uint32_t max_secure_erase_sectors;
    uint32_t max_secure_erase_seg;
    uint32_t secure_erase_sector_alignment;

    struct virtio_blk_zoned_characteristics {
        uint32_t zone_sectors;
        uint32_t max_open_zones;
        uint32_t max_active_zones;
        uint32_t max_append_sectors;
        uint32_t write_granularity;
        uint8_t model;
        uint8_t unused2[3];
    } zoned;
} __attribute__((packed));

#endif