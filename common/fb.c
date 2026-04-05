#include <config.h>
#include <fb.h>
#include <fw_cfg.h>
#include <serial.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>

#include "font.h"

#define FONT_W 8
#define FONT_H 16
#define FONT_SCALE 2
#define FONT_W_SCALE (FONT_W * FONT_SCALE)
#define FONT_H_SCALE (FONT_H * FONT_SCALE)

static struct {
    uint32_t *fb;
    uint32_t size;

    uint32_t width;
    uint32_t height;
    uint32_t stride;

    uint32_t *col;
    uint32_t *row;

    uint32_t max_col;
    uint32_t max_row;
} fb = {0};

struct ramfb {
    uint64_t addr;
    uint32_t fourcc;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} __attribute__((packed));

void fb_clear()
{
    memset(fb.fb, 0x00, fb.size);
}

static void fb_putchar(char c);

static void fb_set_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    fb.fb[y * fb.width + x] = color;
}

static void fb_drawchar(char c)
{
    uint32_t cur_x = *fb.col * FONT_W_SCALE;
    uint32_t cur_y = *fb.row * FONT_H_SCALE;
    const uint8_t *glyph = font[c - 32];
    for (int y = 0; y < FONT_H; y++) {
        uint8_t row_bits = glyph[y];

        for (int x = 0; x < FONT_W; x++) {
            for (int dy = 0; dy < FONT_SCALE; dy++) {
                for (int dx = 0; dx < FONT_SCALE; dx++) {
                    fb_set_pixel(cur_x + x * FONT_SCALE + dx, cur_y + y * FONT_SCALE + dy,
                                 (row_bits & (1 << (7 - x))) ? 0xFFFFFFFF : 0x0);
                }
            }
        }
    }
}

static void fb_scroll()
{
    uint32_t line_bytes = fb.width * FONT_H_SCALE * fb.stride;

    uint8_t *base = (uint8_t *)fb.fb;

    memcpy(base, base + line_bytes, fb.size - line_bytes);
    memset(base + (fb.size - line_bytes), 0, line_bytes);
}

static void fb_putchar(char c)
{
    if (c == '\r') {
        *fb.col = 0;
    } else if (c == '\n') {
        *fb.row = *fb.row + 1;
        *fb.col = 0;
    } else if (c >= 32 && c < 127) {
        fb_drawchar(c);
        *fb.col = *fb.col + 1;
    }

    if (*fb.col == fb.max_col) {
        *fb.row = *fb.row + 1;
        *fb.col = 0;
    }

    if (*fb.row == fb.max_row) {
        *fb.row = fb.max_row - 1;
        *fb.col = 0;
        fb_scroll();
    }
}

void fb_write(const char *buf, size_t len)
{
    while (len--) {
        fb_putchar(*buf++);
    }
}

static struct serial_funcs fb_funcs = {
    .name = "Framebuffer",
    .write = fb_write,
};

void fb_setup()
{
#ifdef QEMU
    struct ramfb cfg = {
        .addr = BE64(0x9C000000),
        .fourcc = BE32(0x34325258), // XRGB8888
        .flags = BE32(0),
        .width = BE32(1080),
        .height = BE32(2400),
        .stride = BE32(4 * 1080),
    };

    uint16_t sel = fw_cfg_find_file("etc/ramfb");
    if (sel == 0) {
        panic("Failed to get \"etc/ramfb\" fw_Cfg");
    }
    fw_cfg_dma_write(&cfg, sel, sizeof(cfg));

    fb.fb = (uint32_t *)BE64(cfg.addr);
    fb.width = BE32(cfg.width);
    fb.height = BE32(cfg.height);
    fb.stride = BE32(cfg.stride) / BE32(cfg.width);
#else
    fb.fb = (uint32_t *)config.fb.base;
    fb.width = config.fb.width;
    fb.height = config.fb.height;
    fb.stride = config.fb.stride;
#endif

    fb.size = fb.width * fb.height * fb.stride;

    fb.max_col = fb.width / FONT_W_SCALE;
    fb.max_row = fb.height / FONT_H_SCALE;

    fb.col = (uint32_t *)(fb.fb + fb.size);
    fb.row = (uint32_t *)(fb.fb + 4 + fb.size);

#ifndef RALLY
    *fb.col = 0;
    *fb.row = 0;

    fb_clear();
#endif

    serial_register(&fb_funcs);
}

uint64_t fb_get_base()
{
    return (uint64_t)fb.fb;
}

uint32_t fb_get_width()
{
    return fb.width;
}

uint32_t fb_get_height()
{
    return fb.height;
}