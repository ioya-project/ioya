#ifndef KERNEL_PATCHES_H_
#define KERNEL_PATCHES_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void apply_kernel_patches();

static inline int64_t sign_extend(int64_t value, int bits)
{
    int64_t shift = 64 - bits;
    return (value << shift) >> shift;
}

static inline int64_t adrp_offset(uint32_t insn)
{
    uint32_t immlo = (insn >> 29) & 0x3;
    uint32_t immhi = (insn >> 5) & 0x7FFFF;

    int64_t imm = ((int64_t)immhi << 2) | immlo;
    imm = sign_extend(imm, 21);

    return imm << 12;
}

static inline uint64_t add_imm(uint32_t insn)
{
    return (insn >> 10) & 0xFFF;
}

static inline uint32_t *find_prev_instr(uint32_t *opcode, uint32_t count, uint32_t instr,
                                        uint32_t mask, uint32_t skip)
{
    for (int i = skip; i < count; i++) {
        if ((opcode[-i] & mask) == instr) {
            return opcode - i;
        }
    }

    return NULL;
}

static inline uint32_t *find_next_instr(uint32_t *opcode, uint32_t count, uint32_t instr,
                                        uint32_t mask, uint32_t skip)
{
    for (uint32_t i = skip; i < count; i++) {
        if ((opcode[i] & mask) == instr) {
            return opcode + i;
        }
    }

    return NULL;
}

#endif