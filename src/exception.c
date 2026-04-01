#include <stdint.h>
#include <stdio.h>
#include <utils.h>

void exception_handler(uint64_t *regs)
{
    printf("Exception\n");

    asm volatile("isb");
    asm volatile("dsb sy");

    uint64_t current_el;
    uint64_t elr;
    uint64_t mpidr;
    uint64_t spsr;
    uint64_t esr;
    uint64_t far;
    uint64_t sp;

    asm volatile("mrs %0, CurrentEL" : "=r"(current_el));
    asm volatile("mrs %0, ELR_EL1" : "=r"(elr));
    asm volatile("mrs %0, MPIDR_EL1" : "=r"(mpidr));
    asm volatile("mrs %0, SPSR_EL1" : "=r"(spsr));
    asm volatile("mrs %0, ESR_EL1" : "=r"(esr));
    asm volatile("mrs %0, FAR_EL1" : "=r"(far));

    asm volatile("mov %0, sp" : "=r"(sp));

    printf("Running at EL%llu\n", current_el >> 2);

    printf("  x0-x3: %016llx %016llx %016llx %016llx\n", regs[0], regs[1], regs[2], regs[3]);
    printf("  x4-x7: %016llx %016llx %016llx %016llx\n", regs[4], regs[5], regs[6], regs[7]);
    printf(" x8-x11: %016llx %016llx %016llx %016llx\n", regs[8], regs[9], regs[10], regs[11]);
    printf("x12-x15: %016llx %016llx %016llx %016llx\n", regs[12], regs[13], regs[14], regs[15]);
    printf("x16-x19: %016llx %016llx %016llx %016llx\n", regs[16], regs[17], regs[18], regs[19]);
    printf("x20-x23: %016llx %016llx %016llx %016llx\n", regs[20], regs[21], regs[22], regs[23]);
    printf("x24-x27: %016llx %016llx %016llx %016llx\n", regs[24], regs[25], regs[26], regs[27]);
    printf("x28-x30: %016llx %016llx %016llx\n", regs[28], regs[29], regs[30]);

    printf("PC:       0x%016llx\n", elr);
    printf("MPIDR:    0x%016llx\n", mpidr);
    printf("SP:       0x%016llx\n", sp);
    printf("SPSR_EL1: 0x%016llx\n", spsr);
    printf("ESR_EL1:  0x%016llx\n", esr);
    printf("FAR_EL1:  0x%016llx\n", far);

    uint64_t ec = (esr >> 26) & 0x3F;
    uint64_t il = (esr >> 25) & 1;
    uint64_t iss = esr & 0x1FFFFFF;
    printf("ESR decode: EC = 0x%llx IL = %llu ISS = 0x%llx\n", ec, il, iss);

    uint64_t *stack = (uint64_t *)sp;

    printf("Stack dump:\n");
    for (int i = 0; i < 16; i++) {
        printf("  [%p] = %016llx\n", &stack[i], stack[i]);
    }

    printf("Backtrace:\n");
    uint64_t *cur_fp = (uint64_t *)regs[29];
    for (int i = 0; i < 16; i++) {
        if (!cur_fp)
            break;

        uint64_t prev_fp;
        uint64_t ret_addr;

        prev_fp = cur_fp[0];
        ret_addr = cur_fp[1];

        if (prev_fp == 0 || ret_addr == 0)
            break;

        if (prev_fp <= (uint64_t)cur_fp)
            break;

        printf("  #%02d FP=%p LR=0x%016llx\n", i, cur_fp, ret_addr);

        cur_fp = (uint64_t *)prev_fp;
    }

    while (1) {
    }
}