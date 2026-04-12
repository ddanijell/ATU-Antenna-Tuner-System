// ATU-130 Project
// PIC18F2520
// pic_init.c - Hardware inicijalizacija
//
// Fosc = 16 MHz (4 MHz HFINTOSC + 4xPLL = 16 MHz)
// Fcpu = Fosc/4 = 4 MHz

#include "main.h"

void pic_init(void) {
    CLRWDT();

    // === OSCILLATOR: 4 MHz HFINTOSC + 4xPLL = 16 MHz ===
    // IRCF = 110 -> 4 MHz interni oscilator (ulaz za PLL)
    // SCS  = 00  -> koristi FOSC config bit (INTIO67)
    OSCCON = 0b01100000;        // IRCF=110 (4 MHz), SCS=00
    OSCTUNEbits.PLLEN = 1;      // 4xPLL ukljucen -> Fosc = 16 MHz

    // Sacekaj stabilizaciju HFINTOSC
    while (!OSCCONbits.IOFS);   // cekaj IOFS=1 (interni oscilator stabilan)

    // === ADC ===
    // AN0, AN1 analogni ulazi, ostali pinovi digitalni
    ADCON1 = 0x0D;              // VCFG=00, PCFG=1101 (AN0, AN1 analogni)
    ADCON2 = 0x92;              // desno, 4 TAD, Fosc/32 (za 16 MHz)
    ADCON0bits.ADON = 1;        // ADC ukljucen

    // === KOMPARATORI: iskljuceni ===
    CMCON = 0x07;

    // === PORT LATCH: sve na 0 (LOW) ===
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;

    // === TRIS - smjer pinova ===
    //
    // PORTA:
    //   RA0 - INPUT  (AN0 - forward power)
    //   RA1 - INPUT  (AN1 - reverse power)
    //   RA2 - OUTPUT (Ind_011)
    //   RA3 - OUTPUT (Ind_045)
    //   RA4 - OUTPUT (Ind_45)
    //   RA5 - OUTPUT (Ind_22)
    //   RA6 - OUTPUT (slobodan, definisan kao output jer INTIO67)
    //   RA7 - nema na DIP28 paketu
    TRISA = 0b00000011;         // RA0, RA1 input; RA2-RA6 output

    // PORTB:
    //   RB0 - INPUT  (rezerva / dugme)
    //   RB1 - OUTPUT (slobodan)
    //   RB2 - OUTPUT (slobodan)
    //   RB3 - OUTPUT (Ind_005)
    //   RB4 - OUTPUT (Ind_022)
    //   RB5 - OUTPUT (Ind_1)
    //   RB6 - INPUT  (UART RX, IOC RBIF)
    //   RB7 - OUTPUT (UART TX)
    TRISB = 0b01000001;         // RB0, RB6 input; ostali output

    UART_OUT_PIN = 1;           // UART TX idle = HIGH (stop bit level)

    // PORTC:
    //   RC0 - OUTPUT (Cap_sw)
    //   RC1 - OUTPUT (Cap_470)
    //   RC2 - OUTPUT (Cap_100)
    //   RC3 - OUTPUT (Cap_22)
    //   RC4 - OUTPUT (Cap_1000)
    //   RC5 - OUTPUT (Cap_220)
    //   RC6 - OUTPUT (Cap_47)
    //   RC7 - OUTPUT (Cap_10)
    TRISC = 0b00000000;         // svi RC output

    // === TIMER0: free-running 16-bit, Fosc/4, prescaler 1:64 ===
    // tick = 4/Fosc * 64 = 16us -> 500ms = 31250 tickova
    T0CON = 0b10000101;         // TMR0ON=1, 16-bit, internal clk, PS=1:64
    TMR0H = 0;
    TMR0L = 0;

    // === PORTB PULL-UP ===
    INTCON2bits.nRBPU = 0;      // ukljuci weak pull-up na svim RB input pinovima (RB0, RB6)

    // === RBIF (RB6) - UART RX, detekcija start bita (IOC falling edge) ===
    // PIC18F2520: RB4-RB7 imaju IOC, RBIF okida na promjenu bilo kojeg od njih
    // Falling edge se detektuje provjerom RB6==0 u ISR-u
    (void)PORTB;                // ocisti IOC mismatch stanje
    INTCONbits.RBIF = 0;        // ocisti flag
    INTCONbits.RBIE = 1;        // ukljuci RB change interrupt

    // === INTERRUPT: ukljucen ===
    INTCONbits.GIE = 1;
}
