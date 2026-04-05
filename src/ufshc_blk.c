
#include <config.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ufshc_blk.h>
#include <utils.h>

#define UFS_HC_BASE (0x01D84000)
#define UFS_HC_CONTROLLER_STATUS (UFS_HC_BASE + 0x30)

#define UFS_HC_UTP_TRANSFER_REQ_LIST_BASE_L (UFS_HC_BASE + 0x50)
#define UFS_HC_UTP_TRANSFER_REQ_LIST_BASE_H (UFS_HC_BASE + 0x54)
#define UFS_HC_UTP_TRANSFER_REQ_DOOR_BELL (UFS_HC_BASE + 0x58)
#define UFS_HC_UTP_TRANSFER_REQ_LIST_RUN_STOP (UFS_HC_BASE + 0x60)

#define UFS_HC_UTP_TASK_REQ_LIST_BASE_L (UFS_HC_BASE + 0x70)
#define UFS_HC_UTP_TASK_REQ_LIST_BASE_H (UFS_HC_BASE + 0x74)
#define UFS_HC_UTP_TASK_REQ_DOOR_BELL (UFS_HC_BASE + 0x78)
#define UFS_HC_UTP_TASK_REQ_LIST_RUN_STOP (UFS_HC_BASE + 0x80)

#define UTP_TRANSFER_REQ_LIST_READY 0x2
#define UTP_TASK_REQ_LIST_READY 0x3
#define UIC_COMMAND_READY 0x4

#define UFSHCD_STATUS_READY                                                                        \
    (UTP_TRANSFER_REQ_LIST_READY | UTP_TASK_REQ_LIST_READY | UIC_COMMAND_READY)

#define UPIU_CMD_FLAGS_WRITE 0x20
#define UPIU_CMD_FLAGS_READ 0x40

#define OCS_SUCCESS 0x0
#define OCS_INVALID_COMMAND_STATUS 0xF

#define MASK_OCS 0xF

#define UTP_CMD_TYPE_UFS (1 << 28)

#define ALIGNED_UPIU_SIZE 512
#define MAX_PRDT_ENTRIES 1

#define WLUN_DEVICE 0xD0

enum utp_dir {
    UTP_NO_DATA = 0,
    UTP_HOST_TO_DEVICE = 1,
    UTP_DEVICE_TO_HOST = 2,
};

struct utp_transfer_req_desc {
    struct {
        uint32_t dword_0;
        uint32_t dword_1;
        uint32_t dword_2;
        uint32_t dword_3;
    } header;

    uint32_t command_desc_base_addr_lo;
    uint32_t command_desc_base_addr_hi;

    uint16_t response_upiu_length;
    uint16_t response_upiu_offset;

    uint16_t prdt_length;
    uint16_t prdt_offset;
};

struct ufshci_prdt_entry {
    uint32_t base_addr;
    uint32_t upper_addr;
    uint32_t reserved;
    uint32_t size;
};

struct utp_transfer_cmd_desc {
    uint8_t command_upiu[ALIGNED_UPIU_SIZE];
    uint8_t response_upiu[ALIGNED_UPIU_SIZE];
    struct ufshci_prdt_entry prdt[MAX_PRDT_ENTRIES];
};

static struct utp_transfer_req_desc utrd __attribute__((aligned(1024)));
static struct utp_transfer_cmd_desc ucd __attribute__((aligned(1024)));

static int ufs_wait_doorbell_clear()
{
    for (int i = 0; i < 1000000; i++) {
        if (read32(UFS_HC_UTP_TRANSFER_REQ_DOOR_BELL) == 0)
            return 0;
    }
    return -1;
}

static int ufshc_scsi_cmd(uint8_t *cdb, size_t cdb_len, void *buf, uint32_t len, enum utp_dir dir,
                          uint8_t lun)
{
    memset(&utrd, 0, sizeof(utrd));
    memset(&ucd, 0, sizeof(ucd));

    if (len) {
        ucd.prdt[0].base_addr = (uint32_t)((uintptr_t)buf);
        ucd.prdt[0].upper_addr = (uint32_t)((uintptr_t)(buf) >> 32);
        ucd.prdt[0].size = len - 1;
    }

    uint8_t *upiu = ucd.command_upiu;
    memset(upiu, 0, ALIGNED_UPIU_SIZE);

    uint8_t flags = 0;

    switch (dir) {
        case UTP_NO_DATA:
            break;
        case UTP_HOST_TO_DEVICE:
            flags |= UPIU_CMD_FLAGS_WRITE;
            break;
        case UTP_DEVICE_TO_HOST:
            flags |= UPIU_CMD_FLAGS_READ;
            break;
    }

    upiu[0] = 1; // COMMAND
    upiu[1] = flags;
    upiu[2] = lun;

    upiu[12] = (len >> 24) & 0xFF;
    upiu[13] = (len >> 16) & 0xFF;
    upiu[14] = (len >> 8) & 0xFF;
    upiu[15] = (len) & 0xFF;

    memcpy(&upiu[16], cdb, cdb_len);

    utrd.header.dword_0 = (dir << 25) | UTP_CMD_TYPE_UFS;
    utrd.header.dword_1 = 0;
    utrd.header.dword_2 = OCS_INVALID_COMMAND_STATUS;
    utrd.header.dword_3 = 0;

    utrd.command_desc_base_addr_lo = (uint32_t)((uintptr_t)&ucd);
    utrd.command_desc_base_addr_hi = (uint32_t)((uintptr_t)(&ucd) >> 32);

    utrd.response_upiu_offset = offsetof(struct utp_transfer_cmd_desc, response_upiu) >> 2;
    utrd.response_upiu_length = ALIGNED_UPIU_SIZE >> 2;

    utrd.prdt_offset = offsetof(struct utp_transfer_cmd_desc, prdt) >> 2;
    utrd.prdt_length = (len ? 1 : 0);

    write32(UFS_HC_UTP_TRANSFER_REQ_DOOR_BELL, 1);

    if (ufs_wait_doorbell_clear() != 0)
        return -1;

    if ((utrd.header.dword_2 & MASK_OCS) != OCS_SUCCESS)
        return -1;

    return 0;
}

static int ufshc_read(struct block_dev *dev, uint64_t lba, uint32_t count, void *buf)
{
    uint32_t len = count * dev->block_size;
    uint8_t cdb[10] = {0};

    cdb[0] = 0x28; // READ(10)

    cdb[2] = (lba >> 24);
    cdb[3] = (lba >> 16);
    cdb[4] = (lba >> 8);
    cdb[5] = lba;

    cdb[7] = (count >> 8);
    cdb[8] = count;

    return ufshc_scsi_cmd(cdb, sizeof(cdb), buf, len, UTP_DEVICE_TO_HOST, 0);
}

static struct block_dev ufshc_dev = {
    .name = "ufshc-blk",
    .block_size = 0,
    .block_count = 0,
    .read = ufshc_read,
    .priv = NULL,
};

void ufshc_blk_init()
{
    write32(UFS_HC_UTP_TRANSFER_REQ_LIST_BASE_L, (uint32_t)(uintptr_t)&utrd);
    write32(UFS_HC_UTP_TRANSFER_REQ_LIST_BASE_H, (uint32_t)((uintptr_t)(&utrd) >> 32));
    write32(UFS_HC_UTP_TASK_REQ_LIST_BASE_L, 0);
    write32(UFS_HC_UTP_TASK_REQ_LIST_BASE_H, 0);

    if ((read32(UFS_HC_CONTROLLER_STATUS) & UFSHCD_STATUS_READY) == UFSHCD_STATUS_READY) {
        write32(UFS_HC_UTP_TRANSFER_REQ_LIST_RUN_STOP, 1);
        write32(UFS_HC_UTP_TASK_REQ_LIST_RUN_STOP, 1);
    } else {
        panic("UFS controller isn't ready");
    }

    uint8_t cdb[10] = {0};
    uint8_t buf[8] = {0};

    cdb[0] = 0x1B;     // START STOP UNIT(10)
    cdb[4] = (1 << 4); // ACTIVE
    if (ufshc_scsi_cmd(cdb, sizeof(cdb), buf, sizeof(buf), UTP_DEVICE_TO_HOST, WLUN_DEVICE)) {
        panic("Failed to wake up ufs");
    }

    memset(cdb, 0, sizeof(cdb));

    cdb[0] = 0x25; // READ CAPACITY(10)
    if (ufshc_scsi_cmd(cdb, sizeof(cdb), buf, sizeof(buf), UTP_DEVICE_TO_HOST, 0)) {
        panic("Failed to read capacity of lun");
    }

    ufshc_dev.block_count = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    ufshc_dev.block_size = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];

    if (ufshc_dev.block_size == 0 || ufshc_dev.block_count == 0) {
        panic("Invalid block size (%d) or block count(0x%x)\n", ufshc_dev.block_size,
              ufshc_dev.block_count);
    }

    block_dev_register(&ufshc_dev);
}