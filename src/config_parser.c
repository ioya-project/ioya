#include <config_parser.h>
#include <stdio.h>
#include <string.h>

struct boot_config config;

void config_parser_parse(char *buf, uint32_t len)
{
    char *p = buf;
    char *end = buf + len;

    while (p < end) {
        char *line = p;

        while (p < end && *p != '\n' && *p != '\r') {
            p++;
        }

        char *line_end = p;

        while (p < end && (*p == '\n' || *p == '\r')) {
            *p++ = 0;
        }

        *line_end = 0;

        if (*line == 0)
            continue;

        char *eq = strchr(line, '=');
        if (!eq)
            continue;

        *eq = 0;

        char *key = line;
        char *value = eq + 1;

        if (strcmp(key, "kernel") == 0) {
            strcpy(config.kernel, value);
        } else if (strcmp(key, "devicetree") == 0) {
            strcpy(config.devicetree, value);
        } else if (strcmp(key, "ramdisk") == 0) {
            strcpy(config.ramdisk, value);
        } else if (strcmp(key, "cmdline") == 0) {
            strcpy(config.cmdline, value);
        } else if (strcmp(key, "load_address") == 0) {
            config.load_address = strtol(value, 0);
        } else if (strcmp(key, "memory_size") == 0) {
            config.memory_size = strtol(value, 0);
        } else if (strcmp(key, "fb_base") == 0) {
            config.fb_base = strtol(value, 0);
        } else if (strcmp(key, "fb_width") == 0) {
            config.fb_width = strtol(value, 0);
        } else if (strcmp(key, "fb_height") == 0) {
            config.fb_height = strtol(value, 0);
        } else if (strcmp(key, "fb_stride") == 0) {
            config.fb_stride = strtol(value, 0);
        }
    }
}

void config_parser_validate()
{
    if (strlen(config.kernel) == 0) {
        panic("No kernel path is provided in config");
    }

    if (strlen(config.devicetree) == 0) {
        panic("No devicetree path is provided in config");
    }

    if (strlen(config.ramdisk) == 0) {
        panic("No ramdisk path is provided in config");
    }

    if (config.load_address == 0) {
        panic("No load address is provided in config");
    }

    if (config.memory_size == 0) {
        panic("No memory size is provided in config");
    }

    if (config.fb_base == 0) {
        panic("No framebuffer base is provided in config");
    }

    if (config.fb_width == 0) {
        panic("No framebuffer width is provided in config");
    }

    if (config.fb_height == 0) {
        panic("No framebuffer height is provided in config");
    }

    if (config.fb_stride == 0) {
        panic("No framebuffer stride is provided in config");
    }
}