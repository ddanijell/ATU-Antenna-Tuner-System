// ATU-130 Project
// PIC18F2520
// uart.c - Software bit-bang UART, 9600 baud
//
// TX: RB7 (blocking bit-bang)
// RX: RB6 (ISR-driven, RBIF IOC, kružni bafer)

#include "main.h"
#include "uart.h"

// ============================================================
// UART RX - ISR na RBIF (RB6 IOC), kružni bafer
// ============================================================

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

// RBIF okida na bilo koju promjenu RB4-RB7
// Provjera RB6==0 filtrira samo falling edge (start bit)
void __interrupt() isr(void) {
    if (INTCONbits.RBIF) {
        INTCONbits.RBIE = 0;        // zabrani novi RBIF tokom prijema
        (void)PORTB;                // ocisti IOC mismatch stanje (obavezno citanje)
        INTCONbits.RBIF = 0;        // ocisti flag

        if (!UART_IN_PIN) {         // provjeri da je RB6 LOW (falling edge = start bit)
            // Cekaj sredinu prvog data bita:
            // start bit traje 104us, uzorkujemo na 1.5 * 104 = 156us od falling edge
            __delay_us(156);

            uint8_t data = 0;
            if (UART_IN_PIN) data |= (1u << 0); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 1); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 2); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 3); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 4); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 5); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 6); __delay_us(UART_BIT_US);
            if (UART_IN_PIN) data |= (1u << 7);
            // stop bit - linija ce biti HIGH, RBIF nece okidati dok RB6 miruje

            // upisi u bafer ako nije pun
            uint8_t next = (rx_head + 1u) & (RX_BUF_SIZE - 1u);
            if (next != rx_tail) {
                rx_buf[rx_head] = data;
                rx_head = next;
            }
        }

        (void)PORTB;                // ocisti novi IOC mismatch koji se pojavio tokom prijema
        INTCONbits.RBIF = 0;        // ocisti eventualni novi flag
        INTCONbits.RBIE = 1;        // ponovo aktiviraj RBIF
    }
}

uint8_t uart_available(void) {
    return rx_head != rx_tail;
}

uint8_t uart_getch(void) {
    while (!uart_available());
    uint8_t c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1u) & (RX_BUF_SIZE - 1u);
    return c;
}

// ============================================================
// UART TX - blocking bit-bang, 9600 baud
// 1 bit = 1/9600 = 104.17 us = 416 cycles @ Fosc=16MHz
// TX: RB1 (UART_OUT_PIN)
// ============================================================

void uart_putch(char c) {
    UART_OUT_PIN = 0;           // start bit
    __delay_us(UART_BIT_US);
    // 8 data bita, LSB first
    UART_OUT_PIN = (c >> 0) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 1) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 2) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 3) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 4) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 5) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 6) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = (c >> 7) & 1; __delay_us(UART_BIT_US);
    UART_OUT_PIN = 1;           // stop bit
    __delay_us(UART_BIT_US);
}

void uart_print(const char *s) {
    while (*s) uart_putch(*s++);
}
