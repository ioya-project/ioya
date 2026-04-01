#include <nvram.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

#define NVRAM_SIZE 0x2000

struct chrp_nvram_part_hdr {
    uint8_t signature;
    uint8_t checksum;
    uint16_t len;
    char name[12];
};

struct apple_nvram_part_hdr {
    struct chrp_nvram_part_hdr chrp;
    uint32_t adler;
    uint32_t generation;
    uint8_t padding[8];
};

static uint8_t chrp_checksum(const struct chrp_nvram_part_hdr *hdr)
{
    const uint8_t *p = (const uint8_t *)hdr;
    uint16_t sum = p[0];

    for (int i = 0; i < 14; i++) {
        sum += p[2 + i];
        sum = (sum & 0xFF) + (sum >> 8);
    }

    return (uint8_t)(sum & 0xFF);
}

static uint32_t adler32(uint32_t adler, const uint8_t *data, size_t len)
{
    const uint32_t MOD_ADLER = 65521;

    uint32_t a = adler & 0xFFFF;
    uint32_t b = (adler >> 16) & 0xFFFF;

    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}

static struct chrp_nvram_part_hdr *add_partition(uint8_t *buf, uint32_t *offset, uint8_t sig,
                                                 const char *name, uint32_t size_bytes)
{
    struct chrp_nvram_part_hdr *hdr = (struct chrp_nvram_part_hdr *)(buf + *offset);

    uint32_t total_size = ALIGN_UP(size_bytes + sizeof(*hdr), 16);

    hdr->signature = sig;
    hdr->len = total_size / 16;

    if (name)
        strcpy(hdr->name, name);

    hdr->checksum = chrp_checksum(hdr);

    *offset += total_size;
    return hdr;
}

void *generate_nvram_data(uint32_t *size)
{
    *size = NVRAM_SIZE;
    void *data = calloc(NVRAM_SIZE, sizeof(uint8_t));

    struct apple_nvram_part_hdr *apple = data;
    apple->chrp.signature = 0x5A;
    apple->chrp.len = 0x2;
    strcpy(apple->chrp.name, "nvram");

    apple->generation = 0;
    apple->chrp.checksum = chrp_checksum(&apple->chrp);

    uint32_t offset = 0x20;
    add_partition(data, &offset, 0x70, "common", 0x7F0);

    struct chrp_nvram_part_hdr *last = (struct chrp_nvram_part_hdr *)(data + offset);
    last->signature = 0x7F;
    last->len = (NVRAM_SIZE - offset) / 16;
    last->checksum = chrp_checksum(last);

    apple->adler = adler32(1, data + 0x14, NVRAM_SIZE - 0x14);

    return data;
}