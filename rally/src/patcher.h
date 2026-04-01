#ifndef PATCHER_H_
#define PATCHER_H_

#include <stdbool.h>
#include <stdint.h>

typedef bool (*PatcherCallback)(void *ctx, uint32_t *opcode);

void patcher_init(void *image, uint64_t text_base);
void patcher_callback_patch(char *name, char *fileset, char *segment, char *section,
                            const uint32_t *pattern, const uint32_t *mask, uint32_t len,
                            PatcherCallback callback, bool silient);

void patcher_replace_patch(char *name, char *fileset, char *segment, char *section,
                           const uint32_t *pattern, const uint32_t *mask, uint32_t len,
                           const uint32_t *replace, const uint32_t *replace_mask,
                           uint32_t replace_off, uint32_t replace_len, bool silient);

#endif