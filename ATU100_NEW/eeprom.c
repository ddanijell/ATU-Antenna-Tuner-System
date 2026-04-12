// ATU-100 Project
// PIC16F1938
// eeprom.c - EEPROM pristup i kalibracija
//
// Koristi direktne registre (EEADRL, EEDATL, EECON1, EECON2)
// kompatibilno sa XC8 v3.x LLVM

#include "main.h"
#include "eeprom.h"

// === RUNTIME VARIJABLE (punjene iz EEPROM-a) ===
uint8_t cal_K_mult       = DEFAULT_K_MULT;
uint8_t cal_min_power    = DEFAULT_MIN_POWER;
uint8_t cal_divider_x100 = DEFAULT_DIVIDER;
uint8_t cal_coarse_step  = DEFAULT_COARSE_STEP;
uint8_t cal_relay_ms     = DEFAULT_RELAY_MS;
uint8_t cal_retry_wait   = DEFAULT_RETRY_WAIT;
uint8_t cal_debug        = DEFAULT_DEBUG;
uint8_t cal_adc_swap     = DEFAULT_ADC_SWAP;
uint8_t cal_last_slot    = DEFAULT_LAST_SLOT;

// ============================================================
// Low-level EEPROM pristup
// PIC16F1938: EEADRL (adresa), EEDATL (podaci)
// ============================================================

uint8_t ee_read(uint8_t addr) {
    EEADRL            = addr;
    EECON1bits.EEPGD  = 0;     // selektuj EEPROM (ne Flash)
    EECON1bits.CFGS   = 0;     // selektuj EEPROM/Flash (ne Config)
    EECON1bits.RD     = 1;     // pokretaj citanje
    return EEDATL;
}

void ee_write(uint8_t addr, uint8_t data) {
    EEADRL            = addr;
    EEDATL            = data;
    EECON1bits.EEPGD  = 0;     // selektuj EEPROM
    EECON1bits.CFGS   = 0;
    EECON1bits.WREN   = 1;     // omoguci upis

    INTCONbits.GIE    = 0;     // zabrani interrupt tokom unlock sekvence
    EECON2            = 0x55;  // obavezna unlock sekvenca
    EECON2            = 0xAA;
    EECON1bits.WR     = 1;     // pokretaj upis
    INTCONbits.GIE    = 1;     // ponovo ukljuci interrupt

    while (EECON1bits.WR);     // cekaj kraj upisa (~2ms)
    EECON1bits.WREN   = 0;     // onemoguci upis
    CLRWDT();
}

// ============================================================
// Kalibracija
// ============================================================

void cal_load(void) {
    cal_K_mult       = ee_read(EE_K_MULT);
    cal_min_power    = ee_read(EE_MIN_POWER);
    cal_divider_x100 = ee_read(EE_DIVIDER);
    cal_coarse_step  = ee_read(EE_COARSE_STEP);
    cal_relay_ms     = ee_read(EE_RELAY_MS);
    cal_retry_wait   = ee_read(EE_RETRY_WAIT);
    cal_debug        = ee_read(EE_DEBUG);
    cal_adc_swap     = ee_read(EE_ADC_SWAP);
    cal_last_slot    = ee_read(EE_LAST_SLOT);
    if (cal_last_slot > EE_CONFIG_MAX)
        cal_last_slot = 0;  // 0 = bypass (reset je bio posljednja komanda)
}

void cal_save(void) {
    ee_write(EE_K_MULT,       cal_K_mult);
    ee_write(EE_MIN_POWER,    cal_min_power);
    ee_write(EE_DIVIDER,      cal_divider_x100);
    ee_write(EE_COARSE_STEP,  cal_coarse_step);
    ee_write(EE_RELAY_MS,     cal_relay_ms);
    ee_write(EE_RETRY_WAIT,   cal_retry_wait);
    ee_write(EE_DEBUG,        cal_debug);
    ee_write(EE_ADC_SWAP,     cal_adc_swap);
    ee_write(EE_LAST_SLOT,    cal_last_slot);
}

void eeprom_init(void) {
    if (ee_read(EE_MAGIC) != EE_MAGIC_VAL) {
        ee_write(EE_MAGIC,      EE_MAGIC_VAL);
        ee_write(EE_K_MULT,     DEFAULT_K_MULT);
        ee_write(EE_MIN_POWER,  DEFAULT_MIN_POWER);
        ee_write(EE_DIVIDER,    DEFAULT_DIVIDER);
        ee_write(EE_COARSE_STEP, DEFAULT_COARSE_STEP);
        ee_write(EE_RELAY_MS,   DEFAULT_RELAY_MS);
        ee_write(EE_RETRY_WAIT, DEFAULT_RETRY_WAIT);
        ee_write(EE_DEBUG,      DEFAULT_DEBUG);
        ee_write(EE_ADC_SWAP,   DEFAULT_ADC_SWAP);
        ee_write(EE_LAST_SLOT,  DEFAULT_LAST_SLOT);
        // inicijalizuj sve memorijske slotove na 0x00
        for (uint8_t i = 0; i < (uint8_t)(EE_CONFIG_MAX * 2); i++) {
            ee_write((uint8_t)(EE_CONFIG_BASE + i), 0x00);
        }
        DBG("EEPROM: factory defaults\r\n");
    }
    cal_load();
}

// ============================================================
// Ispis kalibracije na serial
// ============================================================

static void print_u8(uint8_t n) {
    char buf[4];
    uint8_t i = 3;
    buf[i] = '\0';
    if (n == 0) { uart_putch('0'); return; }
    while (n > 0) { buf[--i] = (char)('0' + n % 10); n /= 10; }
    uart_print(&buf[i]);
}

// $CAL:K,MINPWR,DIV,CSTEP,RLY,WAIT,DBG,ADCSWAP,LASTSLOT,IND,CAP,SW
void cal_print_struct(void) {
    uart_print("$CAL:");
    print_u8(cal_K_mult);
    uart_putch(',');
    print_u8(cal_min_power);
    uart_putch(',');
    print_u8(cal_divider_x100);  // raw EEPROM (0-255); display dodaje +100 za prikaz
    uart_putch(',');
    print_u8(cal_coarse_step);
    uart_putch(',');
    print_u8(cal_relay_ms);
    uart_putch(',');
    print_u8(cal_retry_wait);
    uart_putch(',');
    print_u8(cal_debug);
    uart_putch(',');
    print_u8(cal_adc_swap);
    uart_putch(',');
    print_u8(cal_last_slot);
    uart_putch(',');
    print_u8(at_ind);
    uart_putch(',');
    print_u8(at_cap);
    uart_putch(',');
    print_u8(at_sw);
    uart_print("\r\n");
}

void cal_print(void) {
    uart_print("K_MULT=");
    print_u8(cal_K_mult);
    uart_print(" MIN_PWR=");
    print_u8(cal_min_power);
    uart_print(" DIV=");
    uint16_t div_real = (uint16_t)cal_divider_x100 + 100;
    uint8_t div_dec = (uint8_t)(div_real % 100);
    print_u8((uint8_t)(div_real / 100));
    uart_putch('.');
    if (div_dec < 10) uart_putch('0');  // vodeća nula (npr. 05 umjesto 5)
    print_u8(div_dec);
    uart_print(" CSTEP=");
    print_u8(cal_coarse_step);
    uart_print(" RLY=");
    print_u8(cal_relay_ms);
    uart_print(" WAIT=");
    print_u8(cal_retry_wait);
    uart_print(" DBG=");
    print_u8(cal_debug);
    uart_print(" ADCSWAP=");
    print_u8(cal_adc_swap);
    uart_print(" SLOT=");
    print_u8(cal_last_slot);
    uart_print(" IND=");
    print_u8(at_ind);
    uart_print(" CAP=");
    print_u8(at_cap);
    uart_print(" SW=");
    print_u8(at_sw);
    uart_print("\r\n");
}

// Obrisi magic byte - pri sljedecem boot-u eeprom_init() upisuje factory defaults
void eeprom_factory_reset(void) {
    ee_write(EE_MAGIC, 0x00);
}

// ============================================================
// Snimanje i ucitavanje konfiguracija releja (30 slotova)
// Slot n (1-30): adresa = EE_CONFIG_BASE + (n-1)*2
//   byte 0: at_ind
//   byte 1: at_cap (bit0-6) | at_sw (bit7)
// ============================================================

void relay_config_save(uint8_t n) {
    if (n < 1 || n > EE_CONFIG_MAX) return;
    uint8_t addr = EE_CONFIG_BASE + (uint8_t)((n - 1) * 2);
    ee_write(addr,     at_ind);
    ee_write(addr + 1, at_cap | (at_sw << 7));
    cal_last_slot = n;
    ee_write(EE_LAST_SLOT, cal_last_slot);
    if (cal_debug) {
        uart_print("SAVED ");
        print_u8(n);
        uart_print("\r\n");
        print_lcsw_values();
    } else {
        print_pwr_swr_struct();
    }
}

void relay_config_recall(uint8_t n) {
    if (n < 1 || n > EE_CONFIG_MAX) return;
    uint8_t addr  = EE_CONFIG_BASE + (uint8_t)((n - 1) * 2);
    uint8_t raw   = ee_read(addr + 1);
    at_ind = ee_read(addr);
    at_cap = raw & 0x7F;
    at_sw  = (raw >> 7) & 1;
    at_set_ind(at_ind);
    at_set_cap(at_cap);
    at_set_sw(at_sw);
    if (cal_last_slot != n) {
        cal_last_slot = n;
        ee_write(EE_LAST_SLOT, cal_last_slot);
    }
    if (cal_debug) {
        uart_print("RECALL ");
        print_u8(n);
        uart_print("\r\n");
        print_lcsw_values();
    } else {
        print_pwr_swr_struct();
    }
}
