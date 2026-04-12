// ATU-100 Project
// eeprom.h - EEPROM kalibracija i perzistentne varijable
//
// PIC16F1938: 256 bajtova EEPROM (adrese 0x00 - 0xFF)
//
// Mapa:
//   0x00  Magic byte (0xAA = inicijalizovan)
//   0x01  K_MULT       - kalibracija tandem match (default 16)
//   0x02  MIN_POWER    - min snaga za tuning u W   (default 5)
//   0x03  DIVIDER      - otpornicki razdelnik, cuva se (val*100 - 100), opseg 1.00-3.55
//   0x04  COARSE_STEP  - korak grube pretrage (default 8, opseg 1-127)
//   0x05  RELAY_MS     - delay nakon preklapanja releja [ms] (default 20)
//   0x06  RETRY_WAIT   - cekanje pri gubitku signala x10ms (default 50 = 500ms)
//   0x07  DEBUG        - verbose serial ispis (default 1 = ukljucen)

#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

// === EEPROM ADRESE ===
#define EE_MAGIC        0x00
#define EE_K_MULT       0x01
#define EE_MIN_POWER    0x02
#define EE_DIVIDER      0x03
#define EE_COARSE_STEP  0x04
#define EE_RELAY_MS     0x05
#define EE_RETRY_WAIT   0x06
#define EE_DEBUG        0x07
#define EE_ADC_SWAP     0x08    // swap ADC kanala: 0=fwd AN0/rev AN1, 1=fwd AN1/rev AN0
#define EE_LAST_SLOT    0x09    // poslednji aktivni slot (1-30)
// 0x0A-0x9F: rezerva za buduće parametre
#define EE_CONFIG_BASE  0xA0    // baza za 30 konfiguracija releja
#define EE_CONFIG_MAX   30      // maks broj konfiguracija

// === FACTORY DEFAULTS ===
#define EE_MAGIC_VAL        0xAA
#define DEFAULT_K_MULT      10
#define DEFAULT_MIN_POWER   5
#define DEFAULT_DIVIDER     205     // (3.05*100) - 100 = 205, stvarna vrijednost 3.05
#define DEFAULT_COARSE_STEP 8       // korak grube pretrage (opseg 1-127)
#define DEFAULT_RELAY_MS    25      // relay settling time [ms]
#define DEFAULT_RETRY_WAIT  50      // cekanje pri gubitku signala x10ms (50=500ms)
#define DEFAULT_DEBUG       1       // verbose serial ispis (1=ukljucen)
#define DEFAULT_ADC_SWAP    0       // 0=fwd AN0/rev AN1, 1=fwd AN1/rev AN0
#define DEFAULT_LAST_SLOT   1       // default slot pri prvom boot-u

// === RUNTIME KALIBRACIJSKE VARIJABLE ===
// Punjene iz EEPROM-a pri boot-u via eeprom_init()
extern uint8_t cal_K_mult;
extern uint8_t cal_min_power;
extern uint8_t cal_divider_x100;   // razdelnik offset: stvarna = (val+100)/100.0 (34=1.34, 200=3.00)
extern uint8_t cal_coarse_step;    // korak grube pretrage autotune
extern uint8_t cal_relay_ms;       // delay nakon preklapanja releja [ms]
extern uint8_t cal_retry_wait;     // cekanje pri gubitku signala x10ms
extern uint8_t cal_debug;          // verbose serial ispis (1=ukljucen)
extern uint8_t cal_adc_swap;       // swap ADC kanala (0=default, 1=swap)
extern uint8_t cal_last_slot;      // poslednji aktivni slot (1-30)

// === API ===
uint8_t ee_read(uint8_t addr);
void    ee_write(uint8_t addr, uint8_t data);
void    eeprom_init(void);
void    cal_load(void);
void    cal_save(void);
void    cal_print(void);
void    cal_print_struct(void);
void    relay_config_save(uint8_t n);    // snimi L/C/SW u slot n (1-30)
void    relay_config_recall(uint8_t n);  // ucitaj L/C/SW iz slota n (1-30)
void    eeprom_factory_reset(void);      // obrisi magic byte -> defaults pri sljedecem boot-u

#endif // EEPROM_H
