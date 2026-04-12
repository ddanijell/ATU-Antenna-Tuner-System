// ATU-130 Project
// PIC18F2520
// selftest.c - Self-test i all_outputs_low

#include "main.h"
#include "selftest.h"

// ============================================================
void all_outputs_low(void) {
    Cap_sw = 0; Cap_10 = 0; Cap_22 = 0; Cap_47  = 0;
    Cap_100 = 0; Cap_220 = 0; Cap_470 = 0; Cap_1000 = 0;
    Ind_005 = 0; Ind_011 = 0; Ind_022 = 0; Ind_045 = 0;
    Ind_1 = 0; Ind_22 = 0; Ind_45 = 0;
}

// ============================================================
// Self-test: svaki pin HIGH 1 sec -> LOW, ispis na UART 9600
// Pinout (iz pinout.txt):
//   01 Cap_sw   - LATC0    09 Ind_005 - LATB3
//   02 Cap_10   - LATC7    10 Ind_011 - LATA2
//   03 Cap_22   - LATC3    11 Ind_022 - LATB4
//   04 Cap_47   - LATC6    12 Ind_045 - LATA3
//   05 Cap_100  - LATC2    13 Ind_1   - LATB5
//   06 Cap_220  - LATC5    14 Ind_22  - LATA5
//   07 Cap_470  - LATC1    15 Ind_45  - LATA4
//   08 Cap_1000 - LATC4
// ============================================================
void self_test(void) {
    uart_print("\r\n=== ATU-130 SELF TEST ===\r\n");
 	__delay_ms(500); CLRWDT();
    Cap_sw   = 1; uart_print("01 Cap_sw  = 1\r\n");
 	__delay_ms(500); CLRWDT();
	Cap_sw   = 0; uart_print("01 Cap_sw  = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_10   = 1; uart_print("02 Cap_10  = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_10   = 0; uart_print("02 Cap_10  = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_22   = 1; uart_print("03 Cap_22  = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_22   = 0; uart_print("03 Cap_22  = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_47   = 1; uart_print("04 Cap_47  = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_47   = 0; uart_print("04 Cap_47  = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_100  = 1; uart_print("05 Cap_100 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_100  = 0; uart_print("05 Cap_100 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_220  = 1; uart_print("06 Cap_220 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_220  = 0; uart_print("06 Cap_220 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_470  = 1; uart_print("07 Cap_470 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_470  = 0; uart_print("07 Cap_470 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Cap_1000 = 1; uart_print("08 Cap_1000= 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Cap_1000 = 0; uart_print("08 Cap_1000= 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_005  = 1; uart_print("09 Ind_005 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_005  = 0; uart_print("09 Ind_005 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_011  = 1; uart_print("10 Ind_011 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_011  = 0; uart_print("10 Ind_011 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_022  = 1; uart_print("11 Ind_022 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_022  = 0; uart_print("11 Ind_022 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_045  = 1; uart_print("12 Ind_045 = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_045  = 0; uart_print("12 Ind_045 = 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_1    = 1; uart_print("13 Ind_1   = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_1    = 0; uart_print("13 Ind_1   = 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_22   = 1; uart_print("14 Ind_22  = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_22   = 0; uart_print("14 Ind_22  = 0\r\n");
	__delay_ms(500); CLRWDT();
    Ind_45   = 1; uart_print("15 Ind_45  = 1\r\n"); 
 	__delay_ms(500); CLRWDT();
	Ind_45   = 0; uart_print("15 Ind_45  = 0\r\n");
 	__delay_ms(500); CLRWDT();
    all_outputs_low();
    uart_print("=== TEST OK ===\r\n");
}
