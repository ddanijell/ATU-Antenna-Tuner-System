// ATU-130 Project
// help.c - Serial help ispis
//
// Pozivati sa: print_help()
// Triggerise se komandom "help\r" na serijskom portu.
//
// Stringovi su const -> smjesteni u Flash, ne trose RAM.

#include "main.h"

void print_help(void) {
    uart_print("\r\n");
    uart_print("ATU-130 -- Serial komande\r\n");
    uart_print("------------------------------------------\r\n");
    uart_print("Komanda      Opis\r\n");
    uart_print("------------------------------------------\r\n");
    uart_print("tune         Pokreni autotune\r\n");
    uart_print("test         Self-test svih releja\r\n");
    uart_print("pwr          Izmjeri i ispisi PWR i SWR\r\n");
    uart_print("cal          Ispisi trenutnu kalibraciju\r\n");
    uart_print("reset        Svi releji na 0 (bypass)\r\n");
    uart_print("factory      Factory reset EEPROM + PIC reset\r\n");
    uart_print("help         Ovaj ispis\r\n");
    uart_print("------------------------------------------\r\n");
    uart_print("Kxx          K_MULT kalibracija           default=16   opseg=1-255\r\n");
    uart_print("Mxx          MIN_POWER za tuning [W]      default=5    opseg=1-255\r\n");
    uart_print("Dxxx         DIVIDER razdelnik x100       default=134  opseg=100-355\r\n");
    uart_print("             (D134=1.34, D300=3.00, D355=3.55)\r\n");
    uart_print("Sxx          COARSE_STEP gruba pretraga   default=8    opseg=1-127\r\n");
    uart_print("Rxx          RELAY_MS delay releja [ms]   default=25   opseg=1-255\r\n");
    uart_print("Wxx          RETRY_WAIT x10ms             default=50   opseg=1-255\r\n");
    uart_print("             (W50 = 500ms)\r\n");
    uart_print("V0 / V1      Debug ispis OFF / ON         default=1\r\n");
    uart_print("A0 / A1      ADC swap OFF / ON            default=0\r\n");
    uart_print("             A0: fwd=AN0 rev=AN1\r\n");
    uart_print("             A1: fwd=AN1 rev=AN0\r\n");
    uart_print("------------------------------------------\r\n");
    uart_print("l            L + 1 (induktivnost gore)\r\n");
    uart_print("k            L - 1 (induktivnost dolje)\r\n");
    uart_print("c            C + 1 (kapacitivnost gore)\r\n");
    uart_print("x            C - 1 (kapacitivnost dolje)\r\n");
    uart_print("z            SW toggle (0/1)\r\n");
    uart_print("sNN          Snimi L/C/SW u slot NN (01-30)\r\n");
    uart_print("rNN          Ucitaj L/C/SW iz slota NN (01-30)\r\n");
    uart_print("------------------------------------------\r\n");
    uart_print("Lxxxxxxx     Postavi induktivnosti (7 bita)\r\n");
    uart_print("             MSB->LSB: Ind_45 Ind_22 Ind_1\r\n");
    uart_print("                       Ind_045 Ind_022 Ind_011 Ind_005\r\n");
    uart_print("             Primjer: L1010011\r\n");
    uart_print("Cxxxxxxxx    Postavi kondenzatore (8 bita)\r\n");
    uart_print("             MSB->LSB: Cap_sw Cap_1000 Cap_470 Cap_220\r\n");
    uart_print("                       Cap_100 Cap_47 Cap_22 Cap_10\r\n");
    uart_print("             Primjer: C10000001\r\n");
    uart_print("------------------------------------------\r\n");
    uart_print("Napomena: K M D S R W komande snimaju u EEPROM\r\n");
    uart_print("Sve komande zavrsavaju sa CR (\\r)\r\n");
    uart_print("\r\n");
}
