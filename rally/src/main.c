#include <config.h>
#include <fb.h>
#include <hook.h>
#include <kernel_hooks.h>
#include <kernel_patches.h>
#include <mem.h>
#include <patcher.h>
#include <reloc.h>
#include <serial.h>
#include <stdio.h>
#include <symbols.h>

uint64_t virt_base;
uint64_t phys_base;
uint64_t payload_offset;

void _exception_handler_sp0();
void _exception_handler_sp1();
void _low_exception_handler();

struct boot_config config;

struct kernel_symbols symbols = {
    .serial =
        {
            .serial_init = (serial_init_type)0xfffffff00840fc1c,
            .gPESF = (struct pe_serial_functions **)0xfffffff00a3ae1c0,
            .initted = (bool *)0xfffffff00a3ae1c8,
            .do_transmit = (bool *)0xfffffff00a31fad8,
            .irq_status = (uint32_t *)0xfffffff00a3ae210,
        },
    .vm =
        {
            .arm_vm_init = (arm_vm_init_type)0xfffffff007e85608,
            .arm_vm_prot_init = (arm_vm_prot_init_type)0xfffffff007e84430,
            .arm_vm_page_granular_prot = (arm_vm_page_granular_prot_type)0xfffffff007e84c88,
            .io_map = (io_map_type)0xfffffff007e87630,
        },
    .dt =
        {
            .pe_arm_get_soc_base_phys = (pe_arm_get_soc_base_phys_type)0xfffffff00840e710,
            .secure_dt_get_property = (secure_dt_get_property_type)0xfffffff00840cedc,
            .find_entry = (find_entry_type)0xfffffff00840c9c8,
            .starting_p = (const char **)0xfffffff00a3ae110,
            .initialized = (bool **)0xfffffff00793cd88,
            .root_node = (void **)0xfffffff00793cd70,

        },
    .exception =
        {
            .exception_vectors_base = (void *)0xfffffff007d22000,
            .low_exception_vector_base = (void *)0xfffffff007d29000,
            .el1_sp0_synchronous_vector_long = (void *)0xfffffff007d23000,
            .el1_sp1_synchronous_vector_long = (void *)0xfffffff007d23594,
        },

    .kernel_debug_early = (kernel_debug_early_type)0xfffffff00812955c,
    .printf = (printf_type)0xfffffff007d84398,
    .mh_execute_header = (struct mach_o_header *)0xfffffff007004000,
    .startup_phase = (startup_subsystem_id_t *)0xfffffff0078fb69c,
};

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

    TRAMPOLINE_JUMP(VA_TO_PA(symbols.exception.exception_vectors_base) + 0x0,
                    PA_TO_VA(&_exception_handler_sp0));
    TRAMPOLINE_JUMP(VA_TO_PA(symbols.exception.exception_vectors_base) + 0x200,
                    PA_TO_VA(&_exception_handler_sp1));

    TRAMPOLINE_JUMP(VA_TO_PA(symbols.exception.low_exception_vector_base) + 0x0,
                    &_low_exception_handler);
    TRAMPOLINE_JUMP(VA_TO_PA(symbols.exception.low_exception_vector_base) + 0x200,
                    &_low_exception_handler);

    patcher_init(VA_TO_PA(symbols.mh_execute_header), (uint64_t)symbols.mh_execute_header);
    apply_kernel_patches();

    install_kernel_hooks();
}