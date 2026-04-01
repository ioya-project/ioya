#ifndef GPT_H_
#define GPT_H_

#include <block_dev.h>
#include <guid.h>
#include <stdint.h>

#define GPT_MAX_PART_ENTRIES 128

struct gpt_header {
    uint8_t signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    guid_t disk_guid;
    uint64_t partition_entries_lba;
    uint32_t num_partition_entries;
    uint32_t size_of_partition_entry;
    uint32_t partition_entries_crc32;
} __attribute__((packed));

struct gpt_entry {
    guid_t type_guid;
    guid_t unique_guid;
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    wchar_t name[36];
} __attribute__((packed));

enum gpt_result {
    GPT_ERR_INVAL = -2,
    GPT_ERR_IO,
    GPT_OK,
};

_Static_assert(GPT_OK == 0, "GPT_OK must be 0");

int parse_gpt(struct block_dev *dev, struct gpt_header *header, struct gpt_entry *entries,
              uint32_t *num_entries);
struct gpt_entry *gpt_get_next_by_type(guid_t *guid, struct gpt_entry *entries, uint32_t count,
                                       uint32_t *iterator);
void gpt_print_partition_info(struct gpt_entry *entry);

#endif