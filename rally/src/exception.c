#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_SYSREGS 32

static uint64_t sysreg_key[MAX_SYSREGS];
static uint64_t sysreg_val[MAX_SYSREGS];
static int sysreg_count = 0;

static uint32_t make_sysreg_key(uint32_t op0, uint32_t op1, uint32_t CRn, uint32_t CRm,
                                uint32_t op2)
{
    return (op0 << 14) | (op1 << 11) | (CRn << 7) | (CRm << 3) | (op2);
}

static void sysreg_write(uint32_t key, uint64_t val)
{
    for (int i = 0; i < sysreg_count; i++) {
        if (sysreg_key[i] == key) {
            sysreg_val[i] = val;
            return;
        }
    }

    if (sysreg_count < MAX_SYSREGS) {
        sysreg_key[sysreg_count] = key;
        sysreg_val[sysreg_count] = val;
        sysreg_count++;
    } else {
        panic("The maximum number of sysreg's is reached\n");
    }
}

static uint64_t sysreg_read(uint32_t key)
{
    for (int i = 0; i < sysreg_count; i++) {
        if (sysreg_key[i] == key)
            return sysreg_val[i];
    }

    return 0;
}

extern bool vm_inited;

bool exception_handler(uint64_t *regs)
{
    uint64_t esr = 0;
    asm volatile("mrs %0, esr_el1" : "=r"(esr));
    uint32_t ec = (esr >> 26) & 0x3F;

    uint64_t elr = 0;
    asm volatile("mrs %0, elr_el1" : "=r"(elr));
    uint32_t instr = *(uint32_t *)elr;

    // printf("Exc: ELR: 0x%llx (instr: 0x%08x), ESR: 0x%llx\n", elr, instr, esr);

    if (ec != 0x00) {
        return false;
    }

    if ((instr & 0xFFC00000) != 0xD5000000) {
        return false;
    }

    uint32_t rt = instr & 0x1F;

    uint32_t op0 = (instr >> 19) & 0x3;
    uint32_t op1 = (instr >> 16) & 0x7;
    uint32_t CRn = (instr >> 12) & 0xF;
    uint32_t CRm = (instr >> 8) & 0xF;
    uint32_t op2 = (instr >> 5) & 0x7;

    uint32_t key = make_sysreg_key(op0, op1, CRn, CRm, op2);

    if ((instr & 0xFFF00000) == 0xD5100000) {
        // MSR
        uint64_t val;
        if (rt == 0x1F)
            val = 0;
        else
            val = regs[rt];

        // printf("Emulating: msr s%u_%u_c%u_c%u_%u, x%d(0x%016lx) at 0x%016lx\n", op0, op1, CRn,
        // CRm, op2, rt, val, elr);
        sysreg_write(key, val);
    } else if ((instr & 0xFFF00000) == 0xD5300000) {
        // MRS
        uint64_t val = sysreg_read(key);
        // printf("Emulating: mrs x%d, s%u_%u_c%u_c%u_%u(0x%016lx) at 0x%016lx\n", rt, op0, op1,
        // CRn, CRm, op2, val, elr);
        if (rt != 0x1F)
            regs[rt] = val;
    }

    elr += 4;
    asm volatile("msr elr_el1, %0" ::"r"(elr));

    return true;
}
