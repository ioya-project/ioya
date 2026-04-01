#include <mach_o.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct mach_o_header *mach_o_get_fileset_header(struct mach_o_header *header, const char *entry)
{
    union mach_o_command *lc = NULL;

    if (header->file_type != MH_FILESET) {
        return NULL;
    }

    lc = (union mach_o_command *)(header + 1);
    for (int i = 0; i < header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_FILESET_ENTRY) {
            const char *entry_id = (char *)lc + lc->fileset.entry_id;
            if (strcmp(entry_id, entry) == 0) {
                return (void *)header + lc->fileset.file_off;
            }
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }

    return NULL;
}

struct mach_o_segment_command *mach_o_get_segment(struct mach_o_header *header,
                                                  const char *segment_name)
{
    union mach_o_command *lc = NULL;

    lc = (union mach_o_command *)(header + 1);
    for (int i = 0; i < header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_SEGMENT_64) {
            if (strcmp(lc->segment.segname, segment_name) == 0) {
                return (struct mach_o_segment_command *)lc;
            }
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }

    printf("Failed to find segment %s\n", segment_name);
    return NULL;
}

struct mach_o_section *mach_o_get_section(struct mach_o_header *header, const char *segment_name,
                                          const char *section_name)
{
    struct mach_o_segment_command *segment = NULL;
    struct mach_o_section *section = NULL;

    segment = mach_o_get_segment(header, segment_name);
    if (segment == NULL) {
        printf("Failed to find segment %s\n", segment_name);
        return NULL;
    }

    section = (struct mach_o_section *)(segment + 1);
    for (int i = 0; i < segment->n_sects; i++) {
        if (strcmp(section->sectname, section_name) == 0) {
            return section;
        }
        section = (void *)section + sizeof(struct mach_o_section);
    }

    printf("Failed to find section %s in segment %s\n", section_name, segment_name);
    return NULL;
}

struct mach_o_range mach_o_get_section_va_range(struct mach_o_header *header,
                                                const char *segment_name, const char *section_name)
{
    struct mach_o_section *section = NULL;
    struct mach_o_range range;

    range.base = ~1ULL;
    range.end = 0;

    section = mach_o_get_section(header, segment_name, section_name);
    if (section == NULL) {
        return range;
    }

    range.base = section->vm_addr;
    range.end = section->vm_addr + section->size;

    return range;
}

struct mach_o_range mach_o_get_section_file_range(struct mach_o_header *header,
                                                  const char *segment_name,
                                                  const char *section_name)
{
    struct mach_o_section *section = NULL;
    struct mach_o_range range;

    range.base = ~1ULL;
    range.end = 0;

    section = mach_o_get_section(header, segment_name, section_name);
    if (section == NULL) {
        return range;
    }

    range.base = section->file_off;
    range.end = section->file_off + section->size;

    return range;
}

uint32_t mach_o_get_build_version(struct mach_o_header *header)
{
    union mach_o_command *lc = NULL;

    if (header->file_type == MH_FILESET) {
        header = mach_o_get_fileset_header(header, "com.apple.kernel");
    }

    lc = (union mach_o_command *)(header + 1);
    for (int i = 0; i < header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_BUILD_VERSION) {
            return lc->version.sdk;
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }

    return 0;
}

struct mach_o_load_info mach_o_load_image(void *image, uint64_t load_address)
{
    struct mach_o_header *header = image;
    union mach_o_command *lc = NULL;
    struct mach_o_load_info ret;

    ret.range.base = ~1ULL;
    ret.range.end = 0;

    if (header->magic != MACH_O_MAGIC || header->file_type != MH_FILESET) {
        panic("Invalid mach-o magic(%x) or file_type(%x)\n", header->magic, header->file_type);
    }

    lc = (union mach_o_command *)(header + 1);
    for (int i = 0; i < header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_SEGMENT_64) {
            if (lc->segment.file_off == 0) {
                ret.text_base = lc->segment.vm_addr;
            }

            if (lc->segment.vm_addr + lc->segment.vm_size > ret.range.end) {
                ret.range.end = lc->segment.vm_addr + lc->segment.vm_size;
            }

            if (lc->segment.vm_addr < ret.range.base) {
                ret.range.base = lc->segment.vm_addr;
            }
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }

    ret.range.base &= -0x2000000ull;

    lc = (union mach_o_command *)(header + 1);
    for (int i = 0; i < header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_SEGMENT_64) {
            void *load_from = image + lc->segment.file_off;
            void *load_to = (void *)load_address + lc->segment.vm_addr - ret.range.base;

            printf("Loading %s to 0x%p (vm_size: 0x%lx)\n", lc->segment.segname, load_to,
                   lc->segment.vm_size);

            memcpy(load_to, load_from, lc->segment.file_size);

            if (lc->segment.file_size < lc->segment.vm_size && lc->segment.vm_size)
                memset(load_to + lc->segment.file_size, 0,
                       lc->segment.vm_size - lc->segment.file_size);
        } else if (lc->cmd.cmd == LC_UNIXTHREAD) {
            ret.entry = (void *)lc->thread.state.pc;
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }

    ret.kernel = (void *)load_address + (ret.text_base - ret.range.base);

    return ret;
}