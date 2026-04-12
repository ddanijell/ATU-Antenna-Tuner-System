// ATU-130 Project
// PIC18F2520
// main.h - Pin definitions and function prototypes
//
// Oscillator: 4 MHz HFINTOSC + 4xPLL = 16 MHz Fosc
// Fcpu = Fosc/4 = 4 MHz

#ifndef MAIN_H
#define MAIN_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// === OSCILLATOR ===
#define _XTAL_FREQ      16000000UL

// === UART ===
#define UART_BIT_US     104         // bit period za 9600 baud @ 16MHz (1/9600 = 104.17 us)
#define RX_BUF_SIZE     32          // RX kružni bafer, mora biti stepen broja 2

// === UART PINOVI ===
// TX: RB7 - LATBbits.LATB7
// RX: RB6 - PORTBbits.RB6 (IOC, RBIF)
#define UART_OUT_PIN    LATBbits.LATB7
#define UART_OUT_TRIS   TRISBbits.TRISB7
#define UART_IN_PIN     PORTBbits.RB6
#define UART_IN_TRIS    TRISBbits.TRISB6

// === I/O PINOVI iz pinout.txt ===

// Pin 1:  Relej - Referenca (Cap switch)
#define Cap_sw      LATCbits.LATC0

// Pin 2-8: Kondenzatori pojedinacno
#define Cap_10      LATCbits.LATC7
#define Cap_22      LATCbits.LATC3
#define Cap_47      LATCbits.LATC6
#define Cap_100     LATCbits.LATC2
#define Cap_220     LATCbits.LATC5
#define Cap_470     LATCbits.LATC1
#define Cap_1000    LATCbits.LATC4

// Pin 9-15: Induktivnosti pojedinacno
#define Ind_005     LATBbits.LATB3
#define Ind_011     LATAbits.LATA2
#define Ind_022     LATBbits.LATB4
#define Ind_045     LATAbits.LATA3
#define Ind_1       LATBbits.LATB5
#define Ind_22      LATAbits.LATA5
#define Ind_45      LATAbits.LATA4

// === DEBUG MACRO ===
// Koristi cal_debug iz eeprom.h (extern uint8_t cal_debug)
#define DBG(s) do { if (cal_debug) uart_print(s); } while(0)

// === PROTOTIPI FUNKCIJA ===
void pic_init(void);

#include "uart.h"
#include "selftest.h"
#include "adc.h"
#include "eeprom.h"
#include "autotune.h"

#endif // MAIN_H
