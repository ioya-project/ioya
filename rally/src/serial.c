#include <pl011.h>
#include <stdbool.h>
#include <stddef.h>
#include <symbols.h>
#include <utils.h>

#include <pexpert/arm/protos.h>

static vm_offset_t uart0_base_vaddr = 0;

struct pe_serial_functions {
    void (*init)();
    unsigned int (*transmit_ready)();
    void (*transmit_data)(uint8_t c);
    unsigned int (*receive_ready)();
    uint8_t (*receive_data)();
    void (*enable_irq)();
    bool (*disable_irq)();
    bool (*acknowledge_irq)();
    bool has_irq;
    serial_device_t device;
    struct pe_serial_functions *next;
};

static void uart_init()
{
#ifdef QEMU
    pl011_uart_init(uart0_base_vaddr);
#endif
}

static unsigned int uart_transmit_ready()
{
#ifdef QEMU
    return pl011_uart_transmit_ready(uart0_base_vaddr);
#else
    return 1;
#endif
}

static void uart_transmit_data(uint8_t c)
{
#ifdef QEMU
    pl011_uart_transmit_data(uart0_base_vaddr, c);
#endif
}

static unsigned int uart_receive_ready()
{
#ifdef QEMU
    return pl011_uart_receive_ready(uart0_base_vaddr);
#else
    return 0;
#endif
}

static uint8_t uart_receive_data()
{
#ifdef QEMU
    return pl011_uart_receive_data(uart0_base_vaddr);
#else
    return 0;
#endif
}

static struct pe_serial_functions uart_serial_functions = {
    .device = SERIAL_VMAPPLE_UART,
    .init = uart_init,
    .transmit_ready = uart_transmit_ready,
    .transmit_data = uart_transmit_data,
    .receive_ready = uart_receive_ready,
    .receive_data = uart_receive_data,
};

static void register_serial_functions(struct pe_serial_functions *fns)
{
    fns->next = *symbols.serial.gPESF;
    *symbols.serial.gPESF = fns;
}

int serial_init_hooked()
{
    DTEntry entryP = NULL;
    vm_offset_t soc_base;
    uint32_t prop_size;
    uintptr_t const *reg_prop;
    struct pe_serial_functions *fns = *symbols.serial.gPESF;

    if (*symbols.serial.initted) {
        while (fns != NULL) {
            fns->init();
            fns = fns->next;
        }

        return 1;
    }

    soc_base = symbols.dt.pe_arm_get_soc_base_phys();
    if (soc_base == 0) {
        return 0;
    }

    if (SecureDTFindEntry("name", "uart0", &entryP) == kSuccess) {
        symbols.dt.secure_dt_get_property(entryP, "reg", (void const **)&reg_prop, &prop_size);
        uart0_base_vaddr = symbols.vm.io_map(soc_base + *reg_prop, *(reg_prop + 1), VM_WIMG_IO,
                                             VM_PROT_DEFAULT, false);
    }

    if (uart0_base_vaddr != 0) {
        register_serial_functions(&uart_serial_functions);
    }

    if (*symbols.serial.gPESF == NULL) {
        return 0;
    }

    fns = *symbols.serial.gPESF;
    while (fns != NULL) {
        *symbols.serial.do_transmit = 1;
        fns->init();
        if (fns->has_irq) {
            *symbols.serial.irq_status |= fns->device;
        }
        fns = fns->next;
    }

    *symbols.serial.initted = true;
    return 1;
}