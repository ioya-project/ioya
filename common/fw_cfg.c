#include <fw_cfg.h>
#include <stdint.h>
#include <string.h>
#include <utils.h>

#define FW_CFG_BASE 0x09020000
#define FW_CFG_DATA (FW_CFG_BASE + 0x0)
#define FW_CFG_SELECT (FW_CFG_BASE + 0x8)
#define FW_CFG_DMA (FW_CFG_BASE + 0x10)

#define FW_CFG_SIGNATURE 0x00
#define FW_CFG_ID 0x01
#define FW_CFG_FILE_DIR 0x19

#define FW_CFG_DMA_CTL_ERROR 0x01
#define FW_CFG_DMA_CTL_READ 0x02
#define FW_CFG_DMA_CTL_SELECT 0x08
#define FW_CFG_DMA_CTL_WRITE 0x10

struct fw_cfg_dma {
    uint32_t control;
    uint32_t length;
    uint64_t address;
} __attribute__((packed));

struct fw_cfg_file {
    uint32_t size;
    uint16_t select;
    uint16_t reserved;
    char name[56];
} __attribute__((packed));

struct fw_cfg_files {
    uint32_t count;
    struct fw_cfg_file files[];
} __attribute__((packed));

void fw_cfg_dma_transfer(volatile void *address, uint32_t length, uint32_t control)
{
    volatile struct fw_cfg_dma dma;
    dma.control = BE32(control);
    dma.address = BE64((uint64_t)address);
    dma.length = BE32(length);

    write64(FW_CFG_DMA, BE64((uint64_t)&dma));
    while (BE32(dma.control) & FW_CFG_DMA_CTL_ERROR) {
        __asm__ volatile("dsb ld" ::: "memory");
    }
}

void fw_cfg_dma_read(volatile void *buf, int selector, int length)
{
    fw_cfg_dma_transfer(buf, length,
                        (selector << 16) | FW_CFG_DMA_CTL_SELECT | FW_CFG_DMA_CTL_READ);
}

void fw_cfg_dma_write(volatile void *buf, int selector, int length)
{
    fw_cfg_dma_transfer(buf, length,
                        (selector << 16) | FW_CFG_DMA_CTL_SELECT | FW_CFG_DMA_CTL_WRITE);
}

uint16_t fw_cfg_find_file(const char *name)
{
    uint32_t count = 0;
    fw_cfg_dma_read(&count, FW_CFG_FILE_DIR, sizeof(count));
    count = BE32(count);

    uint64_t directory_size = sizeof(struct fw_cfg_files) + sizeof(struct fw_cfg_file) * count;
    struct fw_cfg_files *directory = __builtin_alloca(directory_size);
    fw_cfg_dma_read(directory, FW_CFG_FILE_DIR, directory_size);

    for (uint32_t i = 0; i < count; i++) {
        struct fw_cfg_file *file = &directory->files[i];
        if (strcmp(name, file->name) == 0) {
            return BE16(file->select);
        }
    }

    return 0;
}