#include <mach_o.h>
#include <mem.h>
#include <patcher.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static struct mach_o_header *kc_header;
static uint64_t kc_text_base;

void patcher_init(void *kernel, uint64_t text_base)
{
    kc_header = kernel;
    kc_text_base = text_base;
}

static void patch(struct mach_o_range range, char *segment_name, char *name,
                  const uint32_t *pattern, const uint32_t *mask, uint32_t len,
                  PatcherCallback callback, void *ctx, bool silient)
{
    uint64_t end = range.end - len * 4;

    uint32_t first_mask = mask[0];
    uint32_t first_pattern = pattern[0];

    for (uint64_t offset = range.base; offset <= end; offset += sizeof(uint32_t)) {
        uint32_t *ptr = (uint32_t *)offset;

        if ((ptr[0] & first_mask) != first_pattern)
            continue;

        uint32_t i = 1;
        for (; i < len; i++) {
            if ((ptr[i] & mask[i]) != pattern[i])
                break;
        }

        if (i != len)
            continue;

        bool applied = callback(ctx, (void *)offset);
        if (applied && !silient) {
            printf("Patch '%s' is applied to '%s' at 0x%p\n", name, segment_name, PA_TO_VA(offset));
        }
    }
}

static void callback_patch(char *name, char *fileset_name, char *segment_name, char *section_name,
                           const uint32_t *pattern, const uint32_t *mask, uint32_t len,
                           PatcherCallback callback, void *ctx, bool silient)
{
    for (uint32_t i = 0; i < len; i++) {
        if ((pattern[i] & mask[i]) != pattern[i]) {
            panic("Bad maskpattern in patch %s index %d\n", name, i);
        }
    }

    if (kc_header->file_type != MH_FILESET) {
        printf("Only fileset kernelcache is supported\n");
        return;
    }

    union mach_o_command *lc = (union mach_o_command *)(kc_header + 1);
    for (int i = 0; i < kc_header->n_cmds; i++) {
        if (lc->cmd.cmd == LC_FILESET_ENTRY) {
            struct mach_o_header *fileset = (void *)kc_header + lc->fileset.file_off;

            if (fileset_name == NULL ||
                strcmp((char *)lc + lc->fileset.entry_id, fileset_name) == 0) {
                union mach_o_command *fs_lc = (union mach_o_command *)(fileset + 1);

                for (int i = 0; i < fileset->n_cmds; i++) {
                    if (fs_lc->cmd.cmd == LC_SEGMENT_64) {
                        if (segment_name == NULL ||
                            strcmp(fs_lc->segment.segname, segment_name) == 0) {

                            struct mach_o_section *section =
                                (struct mach_o_section *)((struct mach_o_segment_command *)fs_lc +
                                                          1);
                            for (int i = 0; i < fs_lc->segment.n_sects; i++) {
                                if (section->flags &
                                    (S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS)) {
                                    if (section_name == NULL ||
                                        strcmp(section->sectname, section_name) == 0) {
                                        struct mach_o_range range;

                                        range.base =
                                            section->vm_addr - kc_text_base + (uintptr_t)kc_header;
                                        range.end = section->vm_addr + section->size -
                                                    kc_text_base + (uintptr_t)kc_header;

                                        patch(range, (char *)&fs_lc->segment.segname, name, pattern,
                                              mask, len, callback, ctx, silient);
                                    }
                                }
                                section = (void *)section + sizeof(struct mach_o_section);
                            }
                        }
                    }
                    fs_lc = (void *)fs_lc + fs_lc->cmd.cmd_size;
                }
            }
        }
        lc = (void *)lc + lc->cmd.cmd_size;
    }
}

void patcher_callback_patch(char *name, char *fileset, char *segment, char *section,
                            const uint32_t *pattern, const uint32_t *mask, uint32_t len,
                            PatcherCallback callback, bool silient)
{
    return callback_patch(name, fileset, segment, section, pattern, mask, len, callback, NULL,
                          silient);
}

struct ReplacePatchContext {
    const uint32_t *replace;
    const uint32_t *mask;
    uint32_t offset;
    uint32_t len;
};

bool replace_callback(void *_ctx, uint32_t *opcode)
{
    struct ReplacePatchContext *ctx = _ctx;

    if (ctx->mask) {
        for (int i = 0; i < ctx->len; i++) {
            opcode[i + ctx->offset] = (opcode[i + ctx->offset] & ctx->mask[i]) | ctx->replace[i];
        }
    } else {
        memcpy(opcode + ctx->offset, ctx->replace, ctx->len * sizeof(uint32_t));
        for (int i = 0; i < ctx->len; i++) {
            opcode[i + ctx->offset] = ctx->replace[i];
        }
    }

    return true;
}

void patcher_replace_patch(char *name, char *fileset, char *segment, char *section,
                           const uint32_t *pattern, const uint32_t *mask, uint32_t len,
                           const uint32_t *replace, const uint32_t *replace_mask,
                           uint32_t replace_off, uint32_t replace_len, bool silient)
{
    struct ReplacePatchContext ctx = {
        .replace = replace,
        .mask = replace_mask,
        .offset = replace_off,
        .len = replace_len,
    };

    return callback_patch(name, fileset, segment, section, pattern, mask, len, replace_callback,
                          &ctx, silient);
}