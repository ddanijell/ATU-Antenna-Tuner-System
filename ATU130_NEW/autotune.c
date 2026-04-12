// ATU-130 Project
// PIC18F2520
// autotune.c - Automatsko podešavanje antenskog tjunera
//
// Portovano iz ATU-100 (David Fainitski, 2016), prilagođeno za XC8 v3.x
//
// Algoritam: 3-fazni hill-climbing
//   1. coarse_tune()  - gruba pretraga L i C u koracima od 3
//   2. sharp_ind()    - fina pretraga induktivnosti oko pronađene tačke
//   3. sharp_cap()    - fina pretraga kondenzatora oko pronađene tačke
//
// Sekvenca se ponavlja za SW=0 i SW=1, uzima se bolji rezultat.
//
// Izlazni kriterij:
//   SWR < 1.10 (110) -> odmah stani, odlično
//   SWR < 1.20 (120) -> stani, dovoljno dobro
//   SWR == 0         -> nema signala, abort

#include "main.h"
#include "adc.h"
#include "eeprom.h"
#include "autotune.h"


// === TRENUTNE POSTAVKE RELEJI ===
uint8_t at_ind = 0;     // bitmap: bit0=Ind_005, bit1=Ind_011, ..., bit6=Ind_45
uint8_t at_cap = 0;     // bitmap: bit0=Cap_10,  bit1=Cap_22,  ..., bit6=Cap_1000
uint8_t at_sw  = 0;     // 0=IN, 1=OUT

// Interni parametri za finu pretragu (postavljaju se u coarse fazama)
static uint8_t range_ind;   // raspon fine pretrage induktivnosti (+-range oko at_ind)
static uint8_t range_cap;   // raspon fine pretrage kondenzatora  (+-range oko at_cap)

// ============================================================
// $TUNE:x strukturalni status (V0 mode)
// 0=NONE 1=WAIT FOR PWR 2=TUNE START 3=TUNE RETRY
// 4=COARSE DONE 5=SCAP DONE 6=SIND DONE
// 7=SW1 START 8=SW2 START 9=SW1 BETTER 10=SW2 BETTER
// 11=TUNE FINE 12=TUNE OK 13=TUNE DONE 14=TUNE ABORT 15=TUNE SKIP
// ============================================================

static void tune_stat(uint8_t n) {
    uart_print("$TUNE:");
    if (n >= 10) uart_putch((char)('0' + n / 10));
    uart_putch((char)('0' + n % 10));
    uart_print("\r\n");
}

// U V1 (debug) ispisuje tekst, u V0 salje $TUNE:n
#define TSTAT(n, msg) do { if (cal_debug) uart_print(msg); else tune_stat(n); } while(0)

// ============================================================
// Ispis rezultata tuninga
// ============================================================

static void print_u16(uint16_t n) {
    char buf[6];
    uint8_t i = 5;
    buf[i] = '\0';
    if (n == 0) { uart_putch('0'); return; }
    while (n > 0) { buf[--i] = (char)('0' + n % 10); n /= 10; }
    uart_print(&buf[i]);
}

void print_lcsw(void) {
    if (!cal_debug) return;
    uart_print("L=0x");
    uart_putch("0123456789ABCDEF"[at_ind >> 4]);
    uart_putch("0123456789ABCDEF"[at_ind & 0x0F]);
    uart_print(" C=0x");
    uart_putch("0123456789ABCDEF"[at_cap >> 4]);
    uart_putch("0123456789ABCDEF"[at_cap & 0x0F]);
    uart_print(" SW=");
    uart_putch('0' + at_sw);
    uart_print("\r\n");
}

void print_lcsw_values(void) {
    if (!cal_debug) return;
    // Kapacitivnost [pF]
    uint16_t cap_pF = 0;
    if (at_cap & 0x01) cap_pF += 10;
    if (at_cap & 0x02) cap_pF += 22;
    if (at_cap & 0x04) cap_pF += 47;
    if (at_cap & 0x08) cap_pF += 100;
    if (at_cap & 0x10) cap_pF += 220;
    if (at_cap & 0x20) cap_pF += 470;
    if (at_cap & 0x40) cap_pF += 1000;

    // Induktivnost x100 [uH*100]
    uint16_t ind_x100 = 0;
    if (at_ind & 0x01) ind_x100 += 5;    // 0.05 uH
    if (at_ind & 0x02) ind_x100 += 11;   // 0.11 uH
    if (at_ind & 0x04) ind_x100 += 22;   // 0.22 uH
    if (at_ind & 0x08) ind_x100 += 45;   // 0.45 uH
    if (at_ind & 0x10) ind_x100 += 100;  // 1.00 uH
    if (at_ind & 0x20) ind_x100 += 220;  // 2.20 uH
    if (at_ind & 0x40) ind_x100 += 450;  // 4.50 uH

    uart_print("Cap=");
    print_u16(cap_pF);
    uart_print("pF Ind=");
    uart_putch('0' + (uint8_t)(ind_x100 / 100));
    uart_putch('.');
    uart_putch('0' + (uint8_t)((ind_x100 % 100) / 10));
    uart_putch('0' + (uint8_t)(ind_x100 % 10));
    uart_print("uH SW=");
    uart_putch('0' + at_sw);
    uart_print("\r\n");
}

// ============================================================
// Postavljanje releji
// ============================================================

static void relay_delay(void) {
    uint8_t i;
    for (i = 0; i < cal_relay_ms; i++) { __delay_ms(1); CLRWDT(); }
}

void at_set_ind(uint8_t val) {
    Ind_005 = (val >> 0) & 1;
    Ind_011 = (val >> 1) & 1;
    Ind_022 = (val >> 2) & 1;
    Ind_045 = (val >> 3) & 1;
    Ind_1   = (val >> 4) & 1;
    Ind_22  = (val >> 5) & 1;
    Ind_45  = (val >> 6) & 1;
    relay_delay();
}

void at_set_cap(uint8_t val) {
    Cap_10   = (val >> 0) & 1;
    Cap_22   = (val >> 1) & 1;
    Cap_47   = (val >> 2) & 1;
    Cap_100  = (val >> 3) & 1;
    Cap_220  = (val >> 4) & 1;
    Cap_470  = (val >> 5) & 1;
    Cap_1000 = (val >> 6) & 1;
    relay_delay();
}

void at_set_sw(uint8_t sw) {
    Cap_sw = sw & 1;
    relay_delay();
}

// Reset: sve L i C na 0, SW=0 (bypass pozicija)
void at_reset(void) {
    at_ind = 0;
    at_cap = 0;
    at_sw  = 0;
    at_set_ind(0);
    at_set_cap(0);
    at_set_sw(0);
}

// ============================================================
// Mjerenje SWR tokom tuninga
// Ako PWR padne ispod minimuma -> g_SWR = 0 (signal za abort)
// ============================================================

static void at_get_swr(void) {
    CLRWDT();
    measure_pwr_swr();
    if (g_PWR < cal_min_power)
        g_SWR = 0;
}

// Verzija sa retry za coarse fazu - kratki gubitak signala ne prekida pretragu
static void at_get_swr_retry(void) {
    at_get_swr();
    if (g_SWR == 0) {
        __delay_ms(200);
        CLRWDT();
        at_get_swr();
    }
}

// ============================================================
// Gruba pretraga kondenzatora (za fiksni at_ind)
// Koraci od 3, traži minimum SWR
// Interna funkcija, poruke se ispisuju iz coarse_tune
// ============================================================

static void coarse_cap(void) {
    uint8_t step = cal_coarse_step;
    uint8_t count;
    uint16_t min_swr;

    at_cap = 0;
    at_set_cap(0);
    range_cap = step;

    at_get_swr_retry(); if (g_SWR == 0) return;
    min_swr = g_SWR + g_SWR / 20;

    for (count = step; count <= 127; ) {
        at_set_cap(count);
        at_get_swr_retry(); if (g_SWR == 0) return;
        if (g_SWR < min_swr) {
            min_swr  = g_SWR + g_SWR / 20;
            at_cap   = count;
            range_cap = step;
            if (g_SWR < AT_SWR_OK) break;
            count += step;
        } else {
            break;
        }
    }
    at_set_cap(at_cap);
}

// ============================================================
// Gruba pretraga induktivnosti + kondenzatora
// Za svaki L korak, traži optimalni C (coarse_cap)
// ============================================================

static void coarse_tune(void) {
    uint8_t step = cal_coarse_step;
    uint8_t count;
    uint8_t mem_cap, mem_range_cap;
    uint16_t min_swr;

    mem_cap      = 0;
    mem_range_cap = step;
    range_ind     = step;
    min_swr      = g_SWR + g_SWR / 20;

    for (count = 0; count <= 127; ) {
        at_ind = count;
        at_set_ind(count);
        coarse_cap();
        at_get_swr_retry();
        if (g_SWR == 0) {
            DBG("COARSE_ABORT\r\n");
            return;
        }
        if (g_SWR < min_swr) {
            min_swr      = g_SWR + g_SWR / 20;
            at_ind       = count;
            mem_cap      = at_cap;
            range_ind     = step;
            mem_range_cap = range_cap;
            if (g_SWR < AT_SWR_OK) {
                DBG("COARSE_OK\r\n");
                break;
            }
            count += step;
        } else {
            TSTAT(4, "COARSE_DONE\r\n");
            break;
        }
    }
    at_cap   = mem_cap;
    range_cap = mem_range_cap;
    at_set_ind(at_ind);
    at_set_cap(at_cap);
    __delay_ms(10);
}

// ============================================================
// Fina pretraga kondenzatora oko pronađene tačke
// ============================================================

static void sharp_cap(void) {
    uint8_t count, max_range, min_range, best_cap;
    uint16_t min_swr;

    max_range = (at_cap + range_cap > 127) ? 127 : at_cap + range_cap;
    min_range = (at_cap > range_cap)        ? at_cap - range_cap : 0;

    // Izmjeri na trenutnoj (coarse) poziciji kao baseline
    best_cap = at_cap;
    at_get_swr();
    if (g_SWR == 0) {
        DBG("SCAP_ABORT\r\n");
        return;
    }
    min_swr = g_SWR;

    for (count = min_range; count <= max_range; count++) {
        if (count == best_cap) continue;   // vec izmjereno
        at_set_cap(count);
        at_get_swr();
        if (g_SWR >= min_swr) { __delay_ms(10); at_get_swr(); }
        if (g_SWR >= min_swr) { __delay_ms(10); at_get_swr(); }
        if (g_SWR == 0) {
            DBG("SCAP_ABORT\r\n");
            at_cap = best_cap;
            at_set_cap(at_cap);
            return;
        }
        if (g_SWR < min_swr) {
            min_swr  = g_SWR;
            best_cap = count;
            if (g_SWR < AT_SWR_OK) {
                DBG("SCAP_OK\r\n");
                break;
            }
        }
    }
    at_cap = best_cap;
    at_set_cap(at_cap);
    TSTAT(5, "SCAP_DONE\r\n");
}

// ============================================================
// Fina pretraga induktivnosti oko pronađene tačke
// ============================================================

static void sharp_ind(void) {
    uint8_t count, max_range, min_range, best_ind;
    uint16_t min_swr;

    max_range = (at_ind + range_ind > 127) ? 127 : at_ind + range_ind;
    min_range = (at_ind > range_ind)        ? at_ind - range_ind : 0;

    // Izmjeri na trenutnoj (coarse) poziciji kao baseline
    best_ind = at_ind;
    at_get_swr();
    if (g_SWR == 0) {
        DBG("SIND_ABORT\r\n");
        return;
    }
    min_swr = g_SWR;

    for (count = min_range; count <= max_range; count++) {
        if (count == best_ind) continue;   // vec izmjereno
        at_set_ind(count);
        at_get_swr();
        if (g_SWR >= min_swr) { __delay_ms(10); at_get_swr(); }
        if (g_SWR >= min_swr) { __delay_ms(10); at_get_swr(); }
        if (g_SWR == 0) {
            DBG("SIND_ABORT\r\n");
            at_ind = best_ind;
            at_set_ind(at_ind);
            return;
        }
        if (g_SWR < min_swr) {
            min_swr  = g_SWR;
            best_ind = count;
            if (g_SWR < AT_SWR_OK) {
                DBG("SIND_OK\r\n");
                break;
            }
        }
    }
    at_ind = best_ind;
    at_set_ind(at_ind);
    TSTAT(6, "SIND_DONE\r\n");
}

// ============================================================
// sub_tune: jedan ciklus coarse+sharp, pa proba drugog SW položaja
// Uzima bolji od dva SW rezultata
// ============================================================

static void sub_tune(void) {
    uint16_t swr_mem;
    uint8_t  ind_mem, cap_mem;

    // --- SW položaj 1 ---
    TSTAT(7, "SW1 START\r\n");
    swr_mem = g_SWR;

    coarse_tune();
    if (g_SWR == 0) { DBG("SW1 ABORT\r\n"); return; }
    at_get_swr();
    if (g_SWR == 0) { DBG("SW1 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW1 OK\r\n"); return; }

    sharp_ind();
    if (g_SWR == 0) { DBG("SW1 ABORT\r\n"); return; }
    at_get_swr();
    if (g_SWR == 0) { DBG("SW1 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW1 OK\r\n"); return; }

    sharp_cap();
    if (g_SWR == 0) { DBG("SW1 ABORT\r\n"); return; }
    at_get_swr();
    if (g_SWR == 0) { DBG("SW1 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW1 OK\r\n"); return; }

    // Ako je poboljšanje dovoljno veliko, ne treba isprobati drugi SW
    if (g_SWR < 200 && g_SWR < swr_mem && (swr_mem - g_SWR) > 100) {
        DBG("SW1 DONE\r\n");
        return;
    }

    // Sačuvaj rezultat SW položaja 1
    swr_mem = g_SWR;
    ind_mem = at_ind;
    cap_mem = at_cap;

    // --- SW položaj 2 ---
    TSTAT(8, "SW2 START\r\n");
    at_sw  = at_sw ? 0 : 1;
    at_ind = 0; at_set_ind(0);
    at_cap = 0; at_set_cap(0);
    at_set_sw(at_sw);
    __delay_ms(50);

    at_get_swr();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW2 OK\r\n"); return; }

    coarse_tune();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    at_get_swr();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW2 OK\r\n"); return; }

    sharp_ind();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    at_get_swr();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW2 OK\r\n"); return; }

    sharp_cap();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    at_get_swr();
    if (g_SWR == 0) { DBG("SW2 ABORT\r\n"); return; }
    if (g_SWR < AT_SWR_OK) { DBG("SW2 OK\r\n"); return; }

    // Ako je SW položaj 1 bio bolji, vrati se na njega
    if (g_SWR > swr_mem) {
        TSTAT(9, "SW1 BETTER\r\n");
        at_sw = at_sw ? 0 : 1;
        at_set_sw(at_sw);
        at_ind = ind_mem;
        at_cap = cap_mem;
        at_set_ind(at_ind);
        at_set_cap(at_cap);
        g_SWR = swr_mem;
    } else {
        TSTAT(10, "SW2 BETTER\r\n");
    }

    CLRWDT();
}

// ============================================================
// autotune() - glavni entry point
//
// Pozivati kada je detektovan visoki SWR tokom emisije.
// ============================================================

void autotune(void) {
    CLRWDT();

    at_get_swr();
    if (g_SWR == 0) {
        TSTAT(1, "TUNE WAIT FOR PWR\r\n");
        uint8_t w;
        for (w = 0; w < 20; w++) {
            uint8_t d;
            for (d = 0; d < 5; d++) { __delay_ms(100); CLRWDT(); }
            at_get_swr();
            if (g_SWR != 0) break;
        }
        if (g_SWR == 0) {
            TSTAT(15, "TUNE SKIP\r\n");
            return;
        }
    } else if (g_SWR < AT_SWR_GOOD) {
        TSTAT(15, "TUNE SKIP\r\n");
        return;
    }

    // Reset na bypass, izmjeri ponovo
    at_reset();
    __delay_ms(50);
    at_get_swr();
    if (g_SWR < AT_SWR_GOOD) {
        TSTAT(15, "TUNE SKIP\r\n");
        return;
    }

    TSTAT(2, "TUNE START\r\n");

    sub_tune();

    if (g_SWR == 0) {
        uint8_t w;
        for (w = 0; w < cal_retry_wait; w++) { __delay_ms(10); CLRWDT(); }
        at_get_swr();
        if (g_SWR == 0) {
            at_reset();
            TSTAT(14, "TUNE ABORT\r\n");
            return;
        }
        TSTAT(3, "TUNE RETRY\r\n");
        // signal se vratio, nastavi finom pretragom
    }

    if (g_SWR < AT_SWR_OK) {
        TSTAT(12, "TUNE OK\r\n");
        print_lcsw();
        print_lcsw_values();
        if (cal_debug) print_pwr_swr();
        return;
    }

    // Dodatna fina pretraga oko trenutne pozicije
    TSTAT(11, "TUNE FINE\r\n");
    range_ind = (cal_coarse_step > 1) ? cal_coarse_step : 1;
    range_cap = range_ind;
    sharp_ind();
    sharp_cap();

    CLRWDT();
    TSTAT(13, "TUNE DONE\r\n");
    print_lcsw();
    print_lcsw_values();
    if (cal_debug) print_pwr_swr();
}
