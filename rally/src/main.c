#include "fb.h"
#include <config.h>
#include <hook.h>
#include <kernel_hooks.h>
#include <kernel_patches.h>
#include <mem.h>
#include <patcher.h>
#include <reloc.h>
#include <serial.h>
#include <stdio.h>

uint64_t virt_base;
uint64_t phys_base;
uint64_t payload_offset;

void _exception_handler_sp0();
void _exception_handler_sp1();
void _low_exception_handler();

extern void *ExceptionVectorsBase;
extern void *LowExceptionVectorBase;

extern struct mach_o_header mh_execute_header;

struct boot_config config;

__attribute__((section(".entry"))) void _start(uint64_t virt, uint64_t phys, uint64_t offset,
                                               struct boot_config cfg)
{
    apply_relocations(phys + offset);
    config = cfg;

    fb_setup();
    serial_setup();

    virt_base = virt;
    phys_base = phys;
    payload_offset = offset;

    printf("virt_base: 0x%08llx, phys_base: 0x%08llx, payload_offset: 0x%08llx\n", virt_base,
           phys_base, payload_offset);

    // ExceptionVectorBase
    TRAMPOLINE_JUMP(VA_TO_PA(&ExceptionVectorsBase) + 0x0, PA_TO_VA(&_exception_handler_sp0));
    TRAMPOLINE_JUMP(VA_TO_PA(&ExceptionVectorsBase) + 0x200, PA_TO_VA(&_exception_handler_sp1));

    // LowExceptionVectorBase
    TRAMPOLINE_JUMP(VA_TO_PA(&LowExceptionVectorBase) + 0x0, &_low_exception_handler);
    TRAMPOLINE_JUMP(VA_TO_PA(&LowExceptionVectorBase) + 0x200, &_low_exception_handler);

    *(uint32_t *)VA_TO_PA(0xfffffff007e222e4) = 0xd65f03c0;
    *(uint32_t *)VA_TO_PA(0xfffffff007e15428) = 0xd2800000;
    *(uint32_t *)VA_TO_PA(0xfffffff007e151c8) = 0xd503201f;

    patcher_init(VA_TO_PA(&mh_execute_header), (uint64_t)&mh_execute_header);
    apply_kernel_patches();

    // Hack to make mmu work on qcom
    *(uint32_t *)VA_TO_PA(0xfffffff007d2c418) = 0xf28f2720;
    *(uint32_t *)VA_TO_PA(0xfffffff007d2c438) = 0xd503201f;

    // Another hack to make it boot on qcom
    *(uint32_t *)VA_TO_PA(0xfffffff007d9a8f4) = 0xd503201f;

    // Skip setuping apple specific system regs
    *(uint32_t *)VA_TO_PA(0xfffffff007d2c454) = 0x14000049;

    install_kernel_hooks();
}