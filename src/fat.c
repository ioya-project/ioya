#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LFN_MAX_NAME 260

struct lfn_state {
    uint16_t buf[LFN_MAX_NAME * 2];
    uint8_t checksum;
    bool valid;
    int len;
};

static uint8_t lfn_checksum(uint8_t *sfn)
{
    uint8_t s = 0;
    for (int i = 0; i < 11; i++) {
        s = (uint8_t)(((s & 1) << 7) | (s >> 1)) + sfn[i];
    }
    return s;
}

static void lfn_reset(struct lfn_state *state)
{
    state->valid = false;
    state->len = 0;
}

static void lfn_feed(struct lfn_state *state, struct fat_lfn_entry *entry)
{
    if (entry->order & 0x40) {
        memset(state->buf, 0xFF, sizeof(state->buf));
        state->checksum = entry->checksum;
        state->valid = true;
        state->len = 0;
    }

    if (!state->valid) {
        return;
    }

    if (state->checksum != entry->checksum) {
        lfn_reset(state);
        return;
    }

    uint8_t slot = entry->order & 0x1F;
    if (slot < 1 || slot > 20) {
        lfn_reset(state);
        return;
    }

    uint16_t tmp[13];
    memcpy(&tmp[0], entry->name1, sizeof(entry->name1));
    memcpy(&tmp[5], entry->name2, sizeof(entry->name2));
    memcpy(&tmp[11], entry->name3, sizeof(entry->name3));

    int base = (slot - 1) * 13;
    memcpy(state->buf + base, tmp, sizeof(tmp));

    int len = 0;
    for (int i = 0; i < LFN_MAX_NAME; i++) {
        uint16_t c = state->buf[i];
        if (c == 0 || c == 0xFFFF) {
            break;
        }
        len++;
    }
    state->len = len;
}

static int lfn_finish(struct lfn_state *state, struct fat_dir_entry *entry, char *out,
                      size_t out_len)
{
    if (!state->valid) {
        return 0;
    }

    if (lfn_checksum((uint8_t *)entry) != state->checksum) {
        lfn_reset(state);
        return 0;
    }

    int i = 0;
    for (i = 0; i < state->len && i < out_len - 1; i++) {
        uint16_t c = state->buf[i];
        if (c == 0 || c == 0xFFFF) {
            break;
        }
        out[i] = (c < 0x80) ? (char)c : '?';
    }
    out[i] = '\0';

    lfn_reset(state);
    return 1;
}

static uint64_t cluster_to_lba(struct fat_fs *fs, uint32_t cluster)
{
    return fs->cluster_heap_lba + (uint64_t)(cluster - 2) * fs->sectors_per_cluster;
}

static int get_next_cluster(struct fat_fs *fs, uint32_t cluster, uint32_t *next)
{
    uint32_t off;
    uint32_t sector;
    uint32_t boff;
    uint32_t value;

    switch (fs->type) {
        case FAT_TYPE_FAT12:
            off = cluster + (cluster >> 1);
            sector = off / fs->bytes_per_sector;
            boff = off % fs->bytes_per_sector;
            if (fs->dev->read(fs->dev, fs->fat_lba + sector, 1, fs->buffer)) {
                return FAT_ERR_IO;
            }
            if (boff == fs->bytes_per_sector - 1) {
                uint8_t lo = *(uint8_t *)(fs->buffer + boff);
                if (fs->dev->read(fs->dev, fs->fat_lba + sector + 1, 1, fs->buffer)) {
                    return FAT_ERR_IO;
                }
                off = (uint32_t)(lo | (*(uint8_t *)fs->buffer << 8));
            } else {
                off = *(uint16_t *)(fs->buffer + boff);
            }
            off = (cluster & 1) ? (off >> 4) : (off & 0xFFF);
            *next = (off >= 0xFF8) ? 0x0FFFFFFF : off;
            return FAT_OK;
        case FAT_TYPE_FAT16:
            off = cluster * 2;
            sector = off / fs->bytes_per_sector;
            boff = off % fs->bytes_per_sector;
            if (fs->dev->read(fs->dev, fs->fat_lba + sector, 1, fs->buffer)) {
                return FAT_ERR_IO;
            }
            value = *(uint16_t *)(fs->buffer + boff);
            *next = (value >= 0xFFF8) ? 0x0FFFFFFF : value;
            return FAT_OK;
        case FAT_TYPE_FAT32:
            off = cluster * 4;
            sector = off / fs->bytes_per_sector;
            boff = off % fs->bytes_per_sector;
            if (fs->dev->read(fs->dev, fs->fat_lba + sector, 1, fs->buffer)) {
                return FAT_ERR_IO;
            }
            value = *(uint32_t *)(fs->buffer + boff) & 0x0FFFFFFF;
            *next = (value >= 0x0FFFFFF8) ? 0x0FFFFFFF : value;
            return FAT_OK;
        default:
            return FAT_ERR_INVAL;
    }
}

static void sfn_to_str(char *out, char *sfn)
{
    int pos = 0;

    for (int i = 0; i < 8; i++) {
        if (sfn[i] == ' ')
            break;

        if (i == 0 && sfn[i] == 0x05) {
            out[pos++] = 0xE5;
        } else {
            out[pos++] = sfn[i];
        }
    }

    bool has_ext = 0;
    for (int i = 8; i < 11; i++) {
        if (sfn[i] != ' ') {
            has_ext = true;
            break;
        }
    }

    if (has_ext) {
        out[pos++] = '.';

        for (int i = 8; i < 11; i++) {
            if (sfn[i] == ' ')
                break;

            out[pos++] = sfn[i];
        }
    }

    out[pos] = '\0';
}

static int dir_find(struct fat_fs *fs, uint32_t first_cluster, bool fixed_root, const char *name,
                    uint32_t *next_cluster, uint32_t *size, bool *is_dir)
{
    uint32_t cur_cluster = first_cluster;
    uint32_t entry_in_sector = 0;
    uint32_t sector_in_cluster = 0;
    uint32_t entry_per_sector = fs->bytes_per_sector / sizeof(struct fat_dir_entry);
    uint32_t lba = fixed_root ? fs->root_dir_lba : cluster_to_lba(fs, first_cluster);

    struct lfn_state lfn = {0};

    for (;;) {
        if (entry_in_sector >= entry_per_sector) {
            entry_in_sector = 0;
            if (fixed_root) {
                lba++;
                if (lba >= fs->root_dir_lba + fs->root_dir_sectors) {
                    return FAT_ERR_NOENTRY;
                }
            } else {
                sector_in_cluster++;
                if (sector_in_cluster >= fs->sectors_per_cluster) {
                    sector_in_cluster = 0;
                    uint32_t next;
                    int ret = get_next_cluster(fs, cur_cluster, &next);
                    if (ret < 0) {
                        return ret;
                    }

                    if (!next || next >= 0xFFFFFF8) {
                        return FAT_ERR_NOENTRY;
                    }
                    cur_cluster = next;
                }
                lba = cluster_to_lba(fs, cur_cluster) + sector_in_cluster;
            }
        }

        if (fs->dev->read(fs->dev, lba, 1, fs->buffer)) {
            return FAT_ERR_IO;
        }

        struct fat_dir_entry *entry = fs->buffer + entry_in_sector * 32;
        entry_in_sector++;

        // End of dir
        if (entry->name[0] == 0x00) {
            return FAT_ERR_NOENTRY;
        }

        // Deleted
        if (entry->name[0] == 0xE5) {
            lfn_reset(&lfn);
            continue;
        }

        // LFN
        if ((entry->attr & 0x3F) == 0xF) {
            lfn_feed(&lfn, (struct fat_lfn_entry *)entry);
            continue;
        }

        // Volume label
        if (entry->attr & 0x8) {
            lfn_reset(&lfn);
            continue;
        }

        char entry_name[LFN_MAX_NAME];
        if (!lfn_finish(&lfn, entry, entry_name, sizeof(entry_name))) {
            // Fallback to SFN
            sfn_to_str(entry_name, entry->name);
        }

        if (strcmp(name, entry_name)) {
            continue;
        }

        *next_cluster = (entry->cluster_hi << 16) | entry->cluster_lo;
        *size = entry->size;
        *is_dir = (entry->attr & 0x10) ? true : false;
        return FAT_OK;
    }
}

static int walk_chain(struct fat_fs *fs, uint32_t start, uint32_t steps, uint32_t *out)
{
    uint32_t cluster = start;

    while (steps--) {
        uint32_t next;
        int ret = get_next_cluster(fs, cluster, &next);
        if (ret < 0) {
            return ret;
        }

        if (next == 0 || next >= 0x0FFFFFF8) {
            return FAT_ERR_EOF;
        }

        cluster = next;
    }

    *out = cluster;
    return FAT_OK;
}

int fat_mount(struct fat_fs *fs, struct block_dev *dev, uint64_t start_lba)
{
    memset(fs, 0, sizeof(*fs));
    fs->buffer = calloc(1, dev->block_size);

    if (dev->read(dev, start_lba, 1, fs->buffer)) {
        return FAT_ERR_IO;
    }

    fs->dev = dev;
    if (*(uint16_t *)(fs->buffer + 0x1fe) != 0xAA55) {
        free(fs->buffer);
        return FAT_ERR_NOTFAT;
    }

    struct fat_bpb *bpb = (struct fat_bpb *)fs->buffer;

    if (strcmp(bpb->oem, "EXFAT") == 0) {
        free(fs->buffer);
        return FAT_ERR_NOTFAT;
    }

    fs->bytes_per_sector = bpb->bytes_per_sector;
    fs->sectors_per_cluster = bpb->sectors_per_cluster;
    fs->bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;

    fs->reserved_sectors = bpb->reserved_sectors;
    fs->num_fats = bpb->num_fats;

    fs->fat_size_sectors = bpb->fat_size_16 ? bpb->fat_size_16 : bpb->fat32.fat_size_32;

    fs->fat_lba = start_lba + fs->reserved_sectors;
    fs->root_dir_sectors =
        ((bpb->root_entry_count * 32) + fs->bytes_per_sector - 1) / fs->bytes_per_sector;
    fs->root_dir_lba = start_lba + fs->reserved_sectors + fs->num_fats * fs->fat_size_sectors;

    uint32_t total_sectors = bpb->total_sectors_16 ? bpb->total_sectors_16 : bpb->total_sectors_32;
    fs->cluster_heap_lba = fs->root_dir_lba + fs->root_dir_sectors;
    fs->total_clusters =
        (total_sectors - fs->cluster_heap_lba + start_lba) / fs->sectors_per_cluster;

    if (fs->total_clusters < 4085) {
        fs->type = FAT_TYPE_FAT12;
        fs->root_cluster = 0;
    } else if (fs->total_clusters < 65525) {
        fs->type = FAT_TYPE_FAT16;
        fs->root_cluster = 0;
    } else {
        fs->type = FAT_TYPE_FAT32;
        fs->root_cluster = bpb->fat32.root_cluster;
    }

    if (fs->bytes_per_sector == 0 || fs->sectors_per_cluster == 0) {
        free(fs->buffer);
        return FAT_ERR_INVAL;
    }

    return FAT_OK;
}

void fat_umount(struct fat_fs *fs)
{
    free(fs->buffer);
    memset(fs, 0, sizeof(*fs));
}

int fat_open(struct fat_fs *fs, struct fat_file *file, const char *path)
{
    memset(file, 0, sizeof(*file));
    file->fs = fs;

    const char *p = path;
    while (*p == '/') {
        p++;
    }

    uint32_t cur_cluster = fs->root_cluster;
    bool is_fixed = fs->type != FAT_TYPE_FAT32;

    char comp[LFN_MAX_NAME];
    while (*p) {
        int len = 0;

        while (*p && *p != '/') {
            if (len < LFN_MAX_NAME - 1) {
                comp[len++] = *p;
            }
            p++;
        }

        comp[len] = '\0';
        while (*p == '/') {
            p++;
        }

        uint32_t next_cluster, size;
        bool is_dir;
        int ret = dir_find(fs, cur_cluster, is_fixed, comp, &next_cluster, &size, &is_dir);
        if (ret < 0) {
            return ret;
        }

        if (*p && !is_dir) {
            return FAT_ERR_NOENTRY;
        }

        cur_cluster = next_cluster;
        is_fixed = false;
        if (*p == '\0') {
            if (is_dir) {
                return FAT_ERR_NOENTRY;
            }

            file->first_cluster = file->cur_cluster = next_cluster;
            file->size = size;
            return FAT_OK;
        }
    }

    return FAT_ERR_NOENTRY;
}

int fat_read(struct fat_file *file, void *buf, uint32_t len)
{
    struct fat_fs *fs = file->fs;

    if (!fs || !file->first_cluster) {
        return FAT_ERR_INVAL;
    }

    if (file->pos >= file->size) {
        return FAT_ERR_EOF;
    }

    if (len > file->size - file->pos) {
        len = file->size - file->pos;
    }

    if (!len) {
        return 0;
    }

    uint32_t done = 0;

    while (done < len) {
        uint32_t cluster = file->cur_cluster;
        if (cluster == 0 || cluster >= 0x0FFFFFF8) {
            break;
        }

        uint32_t offset_in_cluster = file->pos % fs->bytes_per_cluster;
        uint32_t want = len - done;
        if (want > fs->bytes_per_cluster - offset_in_cluster)
            want = fs->bytes_per_cluster - offset_in_cluster;

        uint64_t base = cluster_to_lba(fs, cluster);
        uint32_t sector_id = offset_in_cluster / fs->bytes_per_sector;
        uint32_t boff = offset_in_cluster % fs->bytes_per_sector;

        if (boff) {
            uint32_t copy = fs->bytes_per_cluster - boff;

            if (copy > want) {
                copy = want;
            }

            if (fs->dev->read(fs->dev, base + sector_id, 1, fs->buffer)) {
                return FAT_ERR_IO;
            }
            memcpy(buf + done, fs->buffer + boff, copy);
            done += copy;
            file->pos += copy;
            want -= copy;
            sector_id++;
        }

        if (want >= fs->bytes_per_sector) {
            uint32_t num_sec = want / fs->bytes_per_sector;
            uint32_t secs_left = fs->sectors_per_cluster - sector_id;

            if (num_sec > secs_left) {
                num_sec = secs_left;
            }
            uint32_t bytes = num_sec * fs->bytes_per_sector;

            if (fs->dev->read(fs->dev, base + sector_id, num_sec, buf + done)) {
                return FAT_ERR_IO;
            }

            done += bytes;
            file->pos += bytes;
            want -= bytes;
            sector_id += num_sec;
        }

        if (want > 0) {
            if (fs->dev->read(fs->dev, base + sector_id, 1, fs->buffer)) {
                return FAT_ERR_IO;
            }
            memcpy(buf + done, fs->buffer, want);
            done += want;
            file->pos += want;
            want = 0;
        }

        if (file->pos % fs->bytes_per_cluster == 0) {
            uint32_t next;
            if (get_next_cluster(fs, cluster, &next) || next >= 0x0FFFFFF8) {
                break;
            }
            file->cur_cluster = next;
        }
    }

    return (int)done;
}

int fat_seek(struct fat_file *file, uint32_t offset)
{
    struct fat_fs *fs = file->fs;
    uint32_t cluster;

    if (offset > file->size) {
        return FAT_ERR_INVAL;
    }

    int ret = walk_chain(fs, file->first_cluster, offset / fs->bytes_per_cluster, &cluster);
    if (ret < 0) {
        return ret;
    }

    file->pos = offset;
    file->cur_cluster = cluster;
    return FAT_OK;
}