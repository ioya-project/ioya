#include <gpt.h>
#include <guid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_gpt(struct block_dev *dev, struct gpt_header *header, struct gpt_entry *entries,
              uint32_t *num_entries)
{
    void *buf = calloc(1, dev->block_size);

    uint64_t header_lba = 1;
    if (dev->read(dev, header_lba, 1, buf)) {
        return GPT_ERR_IO;
    }

    memcpy(header, buf, sizeof(struct gpt_header));

    const char sig[] = "EFI PART";
    if (strcmp(sig, (const char *)&header->signature) != 0) {
        return GPT_ERR_INVAL;
    }

    uint32_t entry_count = header->num_partition_entries;
    if (entry_count > GPT_MAX_PART_ENTRIES)
        entry_count = GPT_MAX_PART_ENTRIES;

    uint32_t total_size = entry_count * header->size_of_partition_entry;
    uint32_t blocks = (total_size + dev->block_size - 1) / dev->block_size;

    if (dev->read(dev, header->partition_entries_lba, blocks, entries)) {
        return GPT_ERR_IO;
    }

    *num_entries = entry_count;
    return 0;
}

struct gpt_entry *gpt_get_next_by_type(guid_t *guid, struct gpt_entry *entries, uint32_t count,
                                       uint32_t *iterator)
{
    guid_t empty_guid = {0};
    for (uint32_t i = *iterator; i < count; i++) {
        struct gpt_entry *entry = &entries[i];

        if (memcmp(&entry->type_guid, &empty_guid, sizeof(guid_t)) == 0) {
            continue;
        }

        if (guid == NULL) {
            *iterator = i + 1;
            return entry;
        }

        if (memcmp(&entry->type_guid, guid, sizeof(guid_t)) == 0) {
            *iterator = i + 1;
            return entry;
        }
    }

    return NULL;
}

void gpt_print_partition_info(struct gpt_entry *entry)
{
    uint64_t size = entry->last_lba - entry->first_lba + 1;

    printf("Partition:\n");
    printf("  First LBA : %llu\n", entry->first_lba);
    printf("  Last  LBA : %llu\n", entry->last_lba);
    printf("  Size (sectors): %llu\n", size);
    printf("  Type GUID : %s\n", guid_to_str(&entry->type_guid));
    printf("  Unique GUID : %s\n", guid_to_str(&entry->unique_guid));
    printf("  Name : %ls\n", entry->name);
}