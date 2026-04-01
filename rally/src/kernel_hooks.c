#include <hook.h>
#include <kernel_hooks.h>
#include <mach_o.h>
#include <reloc.h>
#include <serial.h>

#include <arm/machine_routines.h>
#include <arm/misc_protos.h>
#include <arm/pmap_public.h>
#include <arm64/proc_reg.h>
#include <mach/vm_types.h>
#include <sys/kdebug_kernel.h>
#include <vm/memory_types.h>

static void (*arm_vm_page_granular_prot)(vm_offset_t start, unsigned long size,
                                         pmap_paddr_t pa_offset, int tte_prot_XN, int pte_prot_APX,
                                         int pte_prot_XN,
                                         unsigned granule) = (void *)0xfffffff007e84c88;

extern uint64_t virt_base;
extern uint64_t payload_offset;

extern struct mach_o_header mh_execute_header;

static startup_subsystem_id_t *_startup_phase = (startup_subsystem_id_t *)0xfffffff0078fb69c;

static struct hook arm_vm_hook;
static struct hook arm_vm_prot_hook;
static struct hook serial_hook;
static struct hook kernel_debug_hook;

void arm_vm_init_hooked(uint64_t memory_size, boot_args *args);
void arm_vm_prot_init_hooked(boot_args *args);
int serial_init_hooked();
void kernel_debug_early_hooked(uint32_t debugid, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                               uintptr_t arg4);

void install_kernel_hooks()
{
    INSTALL_HOOK(arm_vm_init, arm_vm_init_hooked, &arm_vm_hook);
    INSTALL_HOOK(arm_vm_prot_init, arm_vm_prot_init_hooked, &arm_vm_prot_hook);
    INSTALL_HOOK(serial_init, serial_init_hooked, &serial_hook);
    INSTALL_HOOK(kernel_debug_early, kernel_debug_early_hooked, &kernel_debug_hook);
}

bool vm_inited = false;

void arm_vm_init_hooked(uint64_t memory_size, boot_args *args)
{
    apply_relocations(virt_base + payload_offset);
    ((void (*)(uint64_t, boot_args *))(&arm_vm_hook)->original)(memory_size, args);
    vm_inited = true;
}

#define ARM64_GRANULE_ALLOW_BLOCK (1 << 0)
#define ARM64_GRANULE_ALLOW_HINT (1 << 1)

void arm_vm_prot_init_hooked(boot_args *args)
{
    struct mach_o_segment_command *segment = mach_o_get_segment(&mh_execute_header, "__RALLY");

    ((void (*)(boot_args *))(&arm_vm_prot_hook)->original)(args);
    arm_vm_page_granular_prot(segment->vm_addr, segment->vm_size, 0, 0, AP_RWNA, 0,
                              ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
}

void kernel_debug_early_hooked(uint32_t debugid, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                               uintptr_t arg4)
{
    if (*_startup_phase >= STARTUP_SUB_KPRINTF) {
        const char *fmt = "%x: %s\n";
        asm volatile("stp x29, x30, [sp, #-16]!\n"
                     "mov x29, sp\n"
                     "sub sp, sp, #32\n"
                     "stp %0, %1, [sp]\n"
                     "stp %2, %3, [sp, #16]\n"

                     "mov x9, sp\n"
                     "sub sp, sp, #16\n"
                     "stp %4, x9, [sp]\n"

                     "mov x0, %5\n"
                     "mov x1, #0xffffffffffff4398\n"
                     "movk x1, #0x7d8, lsl #16\n"
                     "movk x1, #0xfff0, lsl #32\n"
                     "blr x1\n"
                     "add sp, sp, #48\n"
                     "ldp x29, x30, [sp], #16\n"
                     :
                     : "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4), "r"(debugid), "r"(fmt)
                     : "x9", "memory");
    }
}