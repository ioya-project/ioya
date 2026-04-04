#ifndef SYMBOLS_H_
#define SYMBOLS_H_

#include <stdint.h>

#include <arm/machine_routines.h>
#include <arm/pmap_public.h>
#include <kern/startup.h>
#include <pexpert/device_tree.h>
#include <vm/memory_types.h>

typedef vm_offset_t (*io_map_type)(vm_map_offset_t phys_addr, vm_size_t size, unsigned int flags,
                                   vm_prot_t prot, bool unmappable);
typedef int (*find_entry_type)(const char *propName, const char *propValue, DTEntry *entryH);
typedef void (*arm_vm_page_granular_prot_type)(vm_offset_t start, unsigned long size,
                                               pmap_paddr_t pa_offset, int tte_prot_XN,
                                               int pte_prot_APX, int pte_prot_XN, unsigned granule);
typedef vm_offset_t (*pe_arm_get_soc_base_phys_type)();
typedef int (*secure_dt_get_property_type)(const DTEntry entry, const char *propertyName,
                                           void const **propertyValue, unsigned int *propertySize);
typedef void (*arm_vm_init_type)(uint64_t memory_size, boot_args *args);
typedef void (*arm_vm_prot_init_type)(boot_args *args);
typedef void (*kernel_debug_early_type)(uint32_t debugid, uintptr_t arg1, uintptr_t arg2,
                                        uintptr_t arg3, uintptr_t arg4);
typedef int (*serial_init_type)();
typedef void (*printf_type)(const char *fmt, ...);

struct kernel_symbols {
    struct {
        serial_init_type serial_init;
        struct pe_serial_functions **gPESF;
        bool *initted;
        bool *do_transmit;
        uint32_t *irq_status;
    } serial;

    struct {
        arm_vm_init_type arm_vm_init;
        arm_vm_prot_init_type arm_vm_prot_init;
        arm_vm_page_granular_prot_type arm_vm_page_granular_prot;
        io_map_type io_map;
    } vm;

    struct {
        pe_arm_get_soc_base_phys_type pe_arm_get_soc_base_phys;
        secure_dt_get_property_type secure_dt_get_property;
        find_entry_type find_entry;
        char const **starting_p;
        bool **initialized;
        void **root_node;
    } dt;

    struct {
        void *exception_vectors_base;
        void *low_exception_vector_base;
        void *el1_sp0_synchronous_vector_long;
        void *el1_sp1_synchronous_vector_long;
    } exception;

    kernel_debug_early_type kernel_debug_early;
    printf_type printf;
    struct mach_o_header *mh_execute_header;
    startup_subsystem_id_t *startup_phase;
};

#define KERNEL_SYMBOLS_EL1_SP0_SYNCHRONOUS_VECTOR_LONG                                             \
    offsetof(struct kernel_symbols, exception.el1_sp0_synchronous_vector_long)
#define KERNEL_SYMBOLS_EL1_SP1_SYNCHRONOUS_VECTOR_LONG                                             \
    offsetof(struct kernel_symbols, exception.el1_sp1_synchronous_vector_long)

extern struct kernel_symbols symbols;

#endif