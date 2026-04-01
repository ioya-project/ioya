#include <fb.h>
#include <serial.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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
} fb = {
    .fb = (uint32_t *)0x9C000000,

    .width = 1080,
    .height = 2400,
    .stride = 4,
};

void fb_clear()
{
    memset(fb.fb, 0x00, fb.size);
}

static void fb_putchar(char c);

void fb_init()
{
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
}

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
    .init = fb_init,
    .write = fb_write,
};

void fb_setup()
{
    serial_register(&fb_funcs);
}