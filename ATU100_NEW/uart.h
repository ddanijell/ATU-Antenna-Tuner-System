// ATU-100 Project
// PIC16F1938
// uart.h - UART prototipi
// TX: RB1 (blocking bit-bang), RX: RB2 (ISR-driven, IOC)

#ifndef UART_H
#define UART_H

#include <stdint.h>

void        uart_putch(char c);
void        uart_print(const char *s);
uint8_t     uart_available(void);
uint8_t     uart_getch(void);
uint8_t     timer1_500ms_ready(void);  // vraca 1 svakih ~500ms (8 x 62.5ms)

#endif // UART_H
