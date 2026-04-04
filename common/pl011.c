#include <pl011.h>
#include <serial.h>
#include <stdbool.h>
#include <utils.h>

#ifdef RALLY
#include <symbols.h>
#endif

#define PL011_BASE (0x9000000)

#define PL011_DR (0x00)
#define PL011_ECR (0x04)
#define PL011_FR (0x18)
#define PL011_IBRD (0x24)
#define PL011_FBRD (0x28)
#define PL011_LCRH (0x2C)
#define PL011_CR (0x30)
#define PL011_TIMSC (0x38)
#define PL011_ICR (0x44)

#define PL011_LCRH_FIFO_EN (1 << 4)
#define PL011_LCRH_WORD_LEN_8 (3 << 5)

#define PL011_IBRD_115200 (13)
#define PL011_FBRD_115200 (1)

#define PL011_CR_UART_EN (1 << 0)
#define PL011_CR_TX_EN (1 << 8)
#define PL011_CR_RX_EN (1 << 9)

#define PL011_FR_RXFE (1 << 4)
#define PL011_FR_TXFF (1 << 5)

void pl011_uart_init(uintptr_t base)
{
    write32(base + PL011_CR, 0);
    write32(base + PL011_ECR, 0);
    write32(base + PL011_LCRH, PL011_LCRH_WORD_LEN_8 | PL011_LCRH_FIFO_EN);
    write32(base + PL011_IBRD, PL011_IBRD_115200);
    write32(base + PL011_FBRD, PL011_FBRD_115200);
    write32(base + PL011_TIMSC, 0);
    write32(base + PL011_ICR, 0x7FF);
    write32(base + PL011_CR, PL011_CR_UART_EN | PL011_CR_TX_EN | PL011_CR_RX_EN);
}

unsigned int pl011_uart_transmit_ready(uintptr_t base)
{
    return !(read32(base + PL011_FR) & PL011_FR_TXFF);
}

void pl011_uart_transmit_data(uintptr_t base, uint8_t c)
{
    write32(base + PL011_DR, c);
}

unsigned int pl011_uart_receive_ready(uintptr_t base)
{
    return !(read32(base + PL011_FR) & PL011_FR_RXFE);
}

uint8_t pl011_uart_receive_data(uintptr_t base)
{
    return read32(base + PL011_FR) & 0xFF;
}

static uint64_t pl011_base = PL011_BASE;

static void pl011_init()
{
    pl011_uart_init(pl011_base);
}

#ifdef RALLY
extern bool vm_inited;
#endif

static void pl011_write(const char *buf, size_t len)
{
#ifdef RALLY
    uint64_t sctlr = __builtin_arm_rsr64("SCTLR_EL1");
    if ((sctlr & 1) && pl011_base == PL011_BASE) {
        if (!vm_inited)
            return;

        pl011_base = symbols.vm.io_map(PL011_BASE, 0x4000, VM_WIMG_IO, VM_PROT_DEFAULT, false);
    }
#endif

    while (len--) {
        while (!pl011_uart_transmit_ready(pl011_base))
            ;

        if (*buf == '\n')
            pl011_uart_transmit_data(pl011_base, '\r');

        pl011_uart_transmit_data(pl011_base, *buf++);
    }
}

static struct serial_funcs pl011_funcs = {
    .name = "PL011 UART",
    .init = pl011_init,
    .write = pl011_write,
};

void pl011_setup()
{
    serial_register(&pl011_funcs);
}