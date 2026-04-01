#include "config.h"
#include <assert.h>
#include <block_dev.h>
#include <cache.h>
#include <config_parser.h>
#include <fat.h>
#include <fb.h>
#include <gpt.h>
#include <heap.h>
#include <injector.h>
#include <libadt.h>
#include <mach_o.h>
#include <nvram.h>
#include <reloc.h>
#include <serial.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#define PA_TO_VA(x) ((x) - config.load_address + info.range.base)
#define VA_TO_PA(x) ((x) - info.range.base + config.load_address)

void populate_adt(struct adt_node *root, struct boot_config *config, uint64_t ramdisk_addr,
                  uint64_t ramdisk_size)
{
    struct adt_prop *prop;
    struct adt_node *child;

    adt_set_prop_stringn(root, "platform-name", "t8020", 32);
    adt_set_prop_null(root, "no-rtc");

    child = adt_get_node(root, "chosen");
    assert(child != NULL && "Failed to get 'chosen' node");

    prop = adt_get_prop(child, "random-seed");
    assert(child != NULL && "Failed to get 'random-seed' prop");
    for (uint32_t i = 0; i < prop->len; i++) {
        *(uint8_t *)(prop->data + i) = i;
    }

    prop = adt_get_prop(child, "boot-nonce");
    assert(child != NULL && "Failed to get 'boot-nonce' prop");
    for (uint32_t i = 0; i < prop->len; i++) {
        *(uint8_t *)(prop->data + i) = i;
    }

    adt_set_prop_uint64(child, "dram-base", config->load_address);
    adt_set_prop_uint64(child, "dram-size", config->memory_size);
    adt_set_prop_string(child, "firmware-version", "IOYA");

    uint32_t nvram_size = 0;
    void *nvram_data = generate_nvram_data(&nvram_size);
    adt_set_prop(child, "nvram-proxy-data", nvram_data, nvram_size);
    adt_set_prop_uint32(child, "nvram-total-size", nvram_size);
    adt_set_prop_uint32(child, "nvram-bank-size", 0x2000);

    adt_set_prop_uint32(child, "effective-production-status-ap", 1);
    adt_set_prop_uint32(child, "effective-security-mode-ap", 1);
    adt_set_prop_uint32(child, "security-domain", 1);
    adt_set_prop_uint32(child, "chip-epoch", 1);
    adt_set_prop_uint32(child, "debug-enabled", 1);

    adt_set_prop_uint32(child, "display-scale", 2);

    child = adt_get_node(child, "lock-regs/amcc");
    adt_set_prop_uint32(child, "aperture-count", 0);
    adt_set_prop_uint32(child, "aperture-size", 0);
    adt_set_prop_uint32(child, "plane-count", 0);
    adt_set_prop_uint32(child, "plane-stride", 0);
    adt_set_prop_null(child, "aperture-phys-addr");
    adt_set_prop_uint32(child, "cache-status-reg-offset", 0);
    adt_set_prop_uint32(child, "cache-status-reg-mask", 0);
    adt_set_prop_uint32(child, "cache-status-reg-value", 0);

    child = adt_get_node(child, "amcc-ctrr-a");
    adt_set_prop_uint32(child, "page-size-shift", 0);
    adt_set_prop_uint32(child, "lower-limit-reg-offset", 0);
    adt_set_prop_uint32(child, "lower-limit-reg-mask", 0);
    adt_set_prop_uint32(child, "upper-limit-reg-offset", 0);
    adt_set_prop_uint32(child, "upper-limit-reg-mask", 0);
    adt_set_prop_uint32(child, "lock-reg-offset", 0);
    adt_set_prop_uint32(child, "lock-reg-mask", 0);
    adt_set_prop_uint32(child, "lock-reg-value", 0);
    adt_set_prop_uint32(child, "enable-reg-offset", 0);
    adt_set_prop_uint32(child, "enable-reg-value", 0);
    adt_set_prop_uint32(child, "enable-reg-mask", 0);

    child = adt_get_node(root, "arm-io");
    struct adt_node *rtc = adt_alloc();
    adt_set_prop_string(rtc, "name", "rtc");
    adt_child_add(child, rtc);

    child = adt_get_node(root, "chosen/memory-map");
    prop = adt_set_prop(child, "RAMDisk", NULL, sizeof(uint64_t) * 2);
    *(uint64_t *)prop->data = ramdisk_addr;
    *(uint64_t *)(prop->data + sizeof(uint64_t)) = ramdisk_size;
}

static struct fat_fs find_ioya_partition(bool *found)
{
    // TODO Scan all block devs
    struct block_dev *dev = block_dev_get(0);
    assert(dev != NULL && "There is no block dev");

    struct gpt_header header;
    struct gpt_entry entries[128];
    uint32_t count;
    int ret = parse_gpt(dev, &header, entries, &count);
    if (ret < 0) {
        panic("Failed to parse gpt");
    }

    uint32_t it = 0;
    struct gpt_entry *gpt_entry;
    struct fat_fs fs;
    struct fat_file config_file;
    while ((gpt_entry = gpt_get_next_by_type(NULL, entries, count, &it)) != NULL) {
        ret = fat_mount(&fs, dev, gpt_entry->first_lba);
        if (ret < 0) {
            continue;
        }

        ret = fat_open(&fs, &config_file, "ioya.cfg");
        if (ret < 0) {
            continue;
        }

        printf("Found ioya config file at %ls partition\n", gpt_entry->name);
        *found = true;
        break;
    }

    return fs;
}

static int read_file(struct fat_fs *fs, const char *path, void *buffer, uint32_t *size)
{
    struct fat_file file;

    int ret = fat_open(fs, &file, path);
    if (ret < 0) {
        return ret;
    }

    if (size) {
        *size = file.size;
    }

    if (buffer) {
        printf("Loading %s file\n", path);
        ret = fat_read(&file, buffer, file.size);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

void main(uintptr_t base, void *payload, size_t payload_size)
{
    apply_relocations(base);
    heap_init();
    serial_setup();
    cache_enable();

    block_dev_setup();

    bool found_partition = false;
    struct fat_fs fs = find_ioya_partition(&found_partition);
    if (!found_partition) {
        panic("Failed to find ioya partition\n");
    }

    uint32_t config_len;
    int ret = read_file(&fs, "ioya.cfg", NULL, &config_len);
    if (ret < 0) {
        panic("Failed to get config file size: %d\n", ret);
    }

    char *config_text = (char *)malloc(config_len);
    ret = read_file(&fs, "ioya.cfg", config_text, NULL);
    if (ret < 0) {
        panic("Failed to read config file: %d\n", ret);
    }

    config_parser_parse(config_text, config_len);
    config_parser_validate();

    fb_setup();

    printf(".___________ _____.___.  _____\n");
    printf("|   \\_____  \\\\__  |   | /  _  \\\n");
    printf("|   |/   |   \\/   |   |/  /_\\  \\\n");
    printf("|   /    |    \\____   /    |    \\\n");
    printf("|___\\_______  / ______\\____|__  /\n");
    printf("            \\/\\/              \\/\n");
    printf("IOYA %s\n", IOYA_VERSION);
    printf("Built with Clang %s\n", __clang_version__);

    uint32_t kernel_len;
    ret = read_file(&fs, config.kernel, NULL, &kernel_len);
    if (ret < 0) {
        panic("Failed to get kernel file size: %d\n", ret);
    }

    void *kernel = malloc(kernel_len + payload_size);
    if (!kernel) {
        panic("Failed to allocate memory for kernel\n");
    }

    ret = read_file(&fs, config.kernel, kernel, NULL);
    if (ret < 0) {
        panic("Failed to read kernel file: %d\n", ret);
    }

    uint32_t build_version = mach_o_get_build_version(kernel);
    void *rally_entry = mach_o_inject_rally(kernel, payload, payload_size);

    struct mach_o_load_info info = mach_o_load_image(kernel, config.load_address);
    if (info.range.base > info.range.end) {
        panic("No mach-o image at address 0x%08llx\n", config.load_address);
    }

    void *entry = VA_TO_PA(info.entry);
    rally_entry = VA_TO_PA(rally_entry);
    uint64_t phys_ptr = VA_TO_PA(info.range.end);

    phys_ptr = ALIGN_UP(phys_ptr, 0x4000);

    uint64_t ramdisk_addr = phys_ptr;
    uint64_t ramdisk_size = 0;
    ret = read_file(&fs, config.ramdisk, (void *)ramdisk_addr, (uint32_t *)&ramdisk_size);
    if (ret < 0) {
        panic("Failed to get ramdisk file size: %d\n", ret);
    }

    phys_ptr += ramdisk_size;
    phys_ptr = ALIGN_UP(phys_ptr, 0x4000);

    struct xnu_boot_arguments *args = (void *)phys_ptr;
    memset(args, 0, sizeof(struct xnu_boot_arguments));
    phys_ptr += 0x4000;

    strcpy(args->cmdline, config.cmdline);

    uint32_t devicetree_size;
    ret = read_file(&fs, config.devicetree, NULL, &devicetree_size);
    if (ret < 0) {
        panic("Failed to get devicetree file size: %d\n", ret);
    }

    void *devicetree = malloc(devicetree_size);
    if (!devicetree) {
        panic("Failed to allocate memory for devicetree\n");
    }

    ret = read_file(&fs, config.devicetree, devicetree, NULL);
    if (ret < 0) {
        panic("Failed to read devicetree file: %d\n", ret);
    }

    struct adt_node *root = adt_deserialize(devicetree, devicetree_size);
    if (root == NULL) {
        panic("Failed to deserialize adt\n");
    }

    printf("Populating adt\n");
    populate_adt(root, &config, ramdisk_addr, ramdisk_size);

    args->device_tree_addr = PA_TO_VA(phys_ptr);
    args->device_tree_size = adt_serialize_len(root);
    if (adt_serialize(root, (void *)phys_ptr, args->device_tree_size) == false) {
        panic("Failed to serialize adt\n");
    }

    phys_ptr += args->device_tree_size;

    phys_ptr = ALIGN_UP(phys_ptr, 0x4000);

    args->revision = 2;
    args->version = 2;
    args->virt_base = info.range.base;
    args->phys_base = config.load_address;
    args->mem_size = config.memory_size;
    args->kernel_top = phys_ptr;

    args->video_args.base_addr = config.fb_base | 1;
    args->video_args.display = false;
    args->video_args.row_bytes = config.fb_width * sizeof(uint32_t);
    args->video_args.width = config.fb_width;
    args->video_args.height = config.fb_height;
    args->video_args.depth.depth = sizeof(uint32_t) * 8;
    args->video_args.depth.rotate = 0;
    args->video_args.depth.scale = 1;
    args->video_args.depth.boot_rotate = 0;

    printf("XNU Base: 0x%08llx, End: 0x%08llx\n", info.range.base, info.range.end);
    printf("XNU Loaded At 0x%p\n", info.kernel);
    printf("XNU Build version: 0x%x\n", build_version);

    printf("Jumping to RALLY payload\n");
    ((void (*)(uint64_t, uint64_t, uint64_t, struct boot_config))rally_entry)(
        info.range.base, config.load_address, (uint64_t)rally_entry - config.load_address, config);

    cache_disable();
    printf("Jumping to XNU Entry\n");
    ((void (*)(struct xnu_boot_arguments *))entry)(args);

    panic("bad things happened\n");
}