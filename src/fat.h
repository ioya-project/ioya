#ifndef FAT_H_
#define FAT_H_

#include <block_dev.h>
#include <stdbool.h>

#define FAT_NAME_MAX 13

enum fat_type {
    FAT_TYPE_NONE,
    FAT_TYPE_FAT12,
    FAT_TYPE_FAT16,
    FAT_TYPE_FAT32,
};

enum fat_result {
    FAT_ERR_EOF = -5,
    FAT_ERR_INVAL,
    FAT_ERR_NOENTRY,
    FAT_ERR_NOTFAT,
    FAT_ERR_IO,
    FAT_OK,
};

_Static_assert(FAT_OK == 0, "FAT_OK must be 0");

struct fat_bpb {
    uint8_t jmp[3];
    char oem[8];

    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    union {
        struct {
            uint8_t drive_number;
            uint8_t reserved1;
            uint8_t boot_signature;
            uint32_t volume_id;
            char volume_label[11];
            char fs_type[8];
        } fat16;

        struct {
            uint32_t fat_size_32;
            uint16_t ext_flags;
            uint16_t fs_version;
            uint32_t root_cluster;
            uint16_t fs_info;
            uint16_t backup_boot_sector;
            uint8_t reserved[12];

            uint8_t drive_number;
            uint8_t reserved1;
            uint8_t boot_signature;
            uint32_t volume_id;
            uint8_t volume_label[11];
            uint8_t fs_type[8];
        } fat32;
    };
} __attribute__((packed));

struct fat_dir_entry {
    char name[11];
    uint8_t attr;
    uint8_t nt_reserved;

    uint8_t create_time_ms;
    uint16_t create_time;
    uint16_t create_date;

    uint16_t access_date;

    uint16_t cluster_hi;

    uint16_t write_time;
    uint16_t write_date;

    uint16_t cluster_lo;

    uint32_t size;
} __attribute__((packed));

struct fat_lfn_entry {
    uint8_t order;

    uint16_t name1[5];

    uint8_t attr;
    uint8_t type;
    uint8_t checksum;

    uint16_t name2[6];

    uint16_t first_cluster;

    uint16_t name3[2];
} __attribute__((packed));

struct fat_fs {
    struct block_dev *dev;
    enum fat_type type;
    void *buffer;

    uint32_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint32_t bytes_per_cluster;
    uint32_t reserved_sectors;
    uint8_t num_fats;
    uint32_t fat_size_sectors;
    uint64_t fat_lba;
    uint64_t root_dir_lba;
    uint32_t root_dir_sectors;
    uint32_t root_cluster;
    uint64_t cluster_heap_lba;
    uint32_t total_clusters;
};

struct fat_file {
    struct fat_fs *fs;
    uint32_t first_cluster;
    uint32_t size;
    uint32_t pos;
    uint32_t cur_cluster;
    uint32_t cur_cluster_idx;
    bool is_dir;
};

static inline const char *fat_type_to_str(enum fat_type type)
{
    switch (type) {
        case FAT_TYPE_NONE:
            return "NONE";
        case FAT_TYPE_FAT12:
            return "FAT12";
        case FAT_TYPE_FAT16:
            return "FAT16";
        case FAT_TYPE_FAT32:
            return "FAT32";
        default:
            return "UNKNOWN";
    }
}

int fat_mount(struct fat_fs *fs, struct block_dev *dev, uint64_t start_lba);
void fat_umount(struct fat_fs *fs);
int fat_open(struct fat_fs *fs, struct fat_file *file, const char *path);

int fat_read(struct fat_file *file, void *buf, uint32_t len);
int fat_seek(struct fat_file *file, uint32_t offset);

#endif