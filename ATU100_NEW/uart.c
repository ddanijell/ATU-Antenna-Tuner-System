// ATU-100 Project
// PIC16F1938
// uart.c - Software bit-bang UART, 9600 baud
//
// TX: RB7 (blocking bit-bang)
// RX: RB6 (ISR-driven, IOC negative edge, kružni bafer)
// Timer1: 62.5ms intervali, 8x = 500ms (koristi timer1_500ms_ready())

#include "main.h"
#include "uart.h"

// ============================================================
// UART RX - ISR na IOC RB6 (falling edge = start bit)
// ============================================================

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

// Timer1 brojac za 500ms mjerenje
static volatile uint8_t t1_count = 0;

void __interrupt() isr(void) {

    // --- Timer1 overflow: 62.5ms interval, preload i brojac ---
    if (PIR1bits.TMR1IF) {
        TMR1H = 0x0B;               // preload za sledecih 62.5ms
        TMR1L = 0xDC;
        PIR1bits.TMR1IF = 0;
        t1_count++;
    }

    // --- IOC na RB6: falling edge = start bit UART RX ---
    if (INTCONbits.IOCIF && IOCBFbits.IOCBF6) {
        INTCONbits.IOCIE = 0;       // zabrani novi IOC tokom prijema

        volatile uint8_t dummy = PORTB; // ocisti IOC mismatch latch
        IOCBFbits.IOCBF6 = 0;       // ocisti flag

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
        // stop bit - ne trebamo cekati, IOC nece okidati dok je linija HIGH

        // upisi u bafer ako nije pun
        uint8_t next = (rx_head + 1u) & (RX_BUF_SIZE - 1u);
        if (next != rx_tail) {
            rx_buf[rx_head] = data;
            rx_head = next;
        }

        // ocisti eventualni flag koji se pojavio tokom prijema
        dummy = PORTB;
        IOCBFbits.IOCBF6 = 0;
        INTCONbits.IOCIE = 1;       // ponovo aktiviraj IOC
    }
}

// ============================================================
// Timer1 500ms ready - vraca 1 (i resetuje brojac) kada prode 500ms
// ============================================================
uint8_t timer1_500ms_ready(void) {
    if (t1_count >= 8) {
        t1_count = 0;
        return 1;
    }
    return 0;
}

// ============================================================
// UART RX API
// ============================================================

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
// 1 bit = 1/9600 = 104.17 us
// TX: RB7 (UART_OUT_PIN)
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
