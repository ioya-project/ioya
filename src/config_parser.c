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
            strcpy(config.general.kernel, value);
        } else if (strcmp(key, "devicetree") == 0) {
            strcpy(config.general.devicetree, value);
        } else if (strcmp(key, "ramdisk") == 0) {
            strcpy(config.general.ramdisk, value);
        } else if (strcmp(key, "cmdline") == 0) {
            strcpy(config.general.cmdline, value);
        } else if (strcmp(key, "load_address") == 0) {
            config.general.load_address = strtol(value, 0);
        } else if (strcmp(key, "memory_size") == 0) {
            config.general.memory_size = strtol(value, 0);
        }

#ifndef QEMU
        if (strcmp(key, "fb_base") == 0) {
            config.fb.base = strtol(value, 0);
        } else if (strcmp(key, "fb_width") == 0) {
            config.fb.width = strtol(value, 0);
        } else if (strcmp(key, "fb_height") == 0) {
            config.fb.height = strtol(value, 0);
        } else if (strcmp(key, "fb_stride") == 0) {
            config.fb.stride = strtol(value, 0);
        }
#endif
    }
}

void config_parser_validate()
{
    if (strlen(config.general.kernel) == 0) {
        panic("No kernel path is provided in config");
    }

    if (strlen(config.general.devicetree) == 0) {
        panic("No devicetree path is provided in config");
    }

    if (strlen(config.general.ramdisk) == 0) {
        panic("No ramdisk path is provided in config");
    }

    if (config.general.load_address == 0) {
        panic("No load address is provided in config");
    }

    if (config.general.memory_size == 0) {
        panic("No memory size is provided in config");
    }

#ifndef QEMU
    if (config.fb.base == 0) {
        panic("No framebuffer base is provided in config");
    }

    if (config.fb.width == 0) {
        panic("No framebuffer width is provided in config");
    }

    if (config.fb.height == 0) {
        panic("No framebuffer height is provided in config");
    }

    if (config.fb.stride == 0) {
        panic("No framebuffer stride is provided in config");
    }
#endif
}