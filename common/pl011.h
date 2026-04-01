#ifndef PL011_H_
#define PL011_H_

#include <stddef.h>
#include <stdint.h>

void pl011_setup();

void pl011_uart_init(uintptr_t base);
unsigned int pl011_uart_transmit_ready(uintptr_t base);
void pl011_uart_transmit_data(uintptr_t base, uint8_t c);
unsigned int pl011_uart_receive_ready(uintptr_t base);
uint8_t pl011_uart_receive_data(uintptr_t base);

#endif