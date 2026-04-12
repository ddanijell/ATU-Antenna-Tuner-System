// ATU-100 Project
// PIC16F1938
// pic_init.c - Hardware inicijalizacija
//
// Fosc = 32 MHz (8 MHz HFINTOSC + 4xPLL)
// Fcpu = Fosc/4 = 8 MHz

#include "main.h"

void pic_init(void) {
    CLRWDT();

    // === OSCILLATOR: 8 MHz HFINTOSC + 4xPLL = 32 MHz ===
    // IRCF = 1110 -> 8 MHz interni oscilator (ulaz za PLL)
    // SCS  = 00   -> koristi FOSC config bit (INTOSC)
    // PLLEN je ukljucen via CONFIG2 PLLEN=ON
    OSCCON = 0b01110000;            // IRCF=1110 (8 MHz), SCS=00

    // Sacekaj stabilizaciju HFINTOSC (i PLL)
    while (!OSCSTATbits.HFIOFR);    // cekaj HFINTOSC Frequency Ready

    // === ADC ===
    // AN0 (RA0), AN1 (RA1) analogni ulazi, ostali pinovi digitalni
    ANSELA = 0x03;                  // RA0, RA1 analogni
    ANSELB = 0x00;                  // RB svi digitalni

    // === FVR: Fixed Voltage Reference 2.048V za ADC ===
    // Redosljed kao u originalnom ATU-100: ADPREF bits -> FVRCON -> ADCON1 full -> ADON
    ADCON1bits.ADPREF0 = 1;         // ADPREF=11 -> FVR referenca
    ADCON1bits.ADPREF1 = 1;
    FVRCON = 0x83;                  // FVREN=1, ADFVR=11 (4.096V), 1 LSB = 4.0 mV
    while (!FVRCONbits.FVRRDY);    // cekaj FVR stabilizaciju
    ADCON1 = 0xE3;                  // ADFM=1 (desno), ADCS=110 (Fosc/64, TAD=2us @ 32MHz), ADNREF=00 (Vss), ADPREF=11 (FVR)
    ADCON0bits.ADON = 1;            // ADC ukljucen

    // === KOMPARATORI: iskljuceni (default je OFF nakon reset-a) ===
    CM1CON0 = 0x00;
    CM2CON0 = 0x00;

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
    //   RA6 - OUTPUT (slobodan, CLKOUTEN=OFF pa je I/O)
    //   RA7 - OUTPUT (slobodan)
    TRISA = 0b00000011;             // RA0, RA1 input; RA2-RA7 output

    // PORTB:
    //   RB0 - INPUT  (rezerva / dugme)
    //   RB1 - OUTPUT (slobodan)
    //   RB2 - OUTPUT (slobodan)
    //   RB3 - OUTPUT (Ind_005)
    //   RB4 - OUTPUT (Ind_022)
    //   RB5 - OUTPUT (Ind_1)
    //   RB6 - INPUT  (UART RX, IOC negative edge)
    //   RB7 - OUTPUT (UART TX)
    TRISB = 0b01000001;             // RB0, RB6 input; ostali output

    UART_OUT_PIN = 1;               // UART TX idle = HIGH (stop bit level)

    // PORTC:
    //   RC0-RC7 - OUTPUT (svi relej pinovi)
    TRISC = 0b00000000;             // svi RC output

    // === WEAK PULL-UP na PORTB ===
    OPTION_REGbits.nWPUEN = 0;      // ukljuci weak pull-up globalno
    WPUB = 0x41;                    // pull-up na RB0 i RB6

    // === TIMER1: 16-bit free-running, Fosc/4, prescaler 1:8 ===
    // Fcpu = 8 MHz -> Timer1 sa 1:8 prescaler -> 1 us/tick
    // Preload za 62.5ms intervale: 65536 - 62500 = 3036 = 0x0BDC
    // 8 intervala x 62.5ms = 500ms
    TMR1H  = 0x0B;
    TMR1L  = 0xDC;
    T1CON  = 0b00110001;            // TMR1CS=00 (Fosc/4), T1CKPS=11 (1:8), TMR1ON=1
    PIR1bits.TMR1IF  = 0;           // ocisti flag
    PIE1bits.TMR1IE  = 1;           // ukljuci Timer1 interrupt

    // === IOC na RB6 - UART RX detekcija start bita (falling edge) ===
    IOCBN = 0x40;                   // IOCBN6 = 1 (negative edge na RB6)
    IOCBP = 0x00;                   // nema positive edge detekcije
    IOCBF = 0x00;                   // ocisti sve IOC flagove
    INTCONbits.IOCIE = 1;           // ukljuci IOC interrupt

    // === INTERRUPT: ukljucen ===
    INTCONbits.PEIE = 1;            // peripheral interrupts (Timer1)
    INTCONbits.GIE  = 1;
}
