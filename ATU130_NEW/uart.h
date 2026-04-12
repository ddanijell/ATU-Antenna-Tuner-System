// ATU-130 Project
// uart.h - UART prototipi
// TX: RB1 (blocking bit-bang), RX: RB2 (ISR-driven, INT2)

#ifndef UART_H
#define UART_H

#include <stdint.h>

void        uart_putch(char c);
void        uart_print(const char *s);
uint8_t     uart_available(void);
uint8_t     uart_getch(void);

#endif // UART_H
