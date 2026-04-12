// ATU-130 Project
// autotune.h - Automatsko podešavanje antenskog tjunera
//
// Algoritam portovan iz ATU-100 (David Fainitski, 2016)
// Prilagođen za XC8 v3.x / PIC18F2520

#ifndef AUTOTUNE_H
#define AUTOTUNE_H

#include <stdint.h>

// === PARAMETRI TUNINGA ===
#define AT_SWR_GOOD     110     // SWR < 1.10 -> odmah zaustavi (odlicno)
#define AT_SWR_OK       150     // SWR < 1.50 -> zaustavi fazu (dovoljno dobro)

// === TRENUTNE POSTAVKE RELEJI ===
extern uint8_t at_ind;          // bitmap induktivnosti (bit0=Ind_005 .. bit6=Ind_45)
extern uint8_t at_cap;          // bitmap kondenzatora  (bit0=Cap_10  .. bit6=Cap_1000)
extern uint8_t at_sw;           // SW relay (0=IN, 1=OUT)

// === API ===
void at_set_ind(uint8_t val);   // postavi induktivnosti i sacekaj relay
void at_set_cap(uint8_t val);   // postavi kondenzatore i sacekaj relay
void at_set_sw(uint8_t sw);     // postavi SW relay i sacekaj relay
void at_reset(void);            // sve na 0 (bypass)
void autotune(void);            // pokreni autotune sekvencu
void print_lcsw(void);          // ispisi L/C/SW hex vrijednosti
void print_lcsw_values(void);   // ispisi L/C fizicke vrijednosti (uH, pF)

#endif // AUTOTUNE_H
