// ATU-100 Project
// adc.h - Mjerenje snage i SWR
//
// Tandem match: 50 ohm, 1.12V = 40W, K_MULT = 16
// Razdelnik: 1.34:1 -> 40W = 2490mV na ADC, max ~161W
// Dioda: Ge (germanijum), korekcijska tablica
// Power range: Low (0 - 150W)
// Min snaga za tuning: 10W

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// === KALIBRACIJA ===
#define K_MULT          16      // tandem match kalibracija (1.12V = 40W @ 50ohm)
#define MIN_POWER_W     10      // minimalna snaga za tuning [W]
// DIVIDER_RATIO je sada u EEPROM-u kao cal_divider_x100 (134 = 1.34)

// === GLOBALNI REZULTATI MJERENJA ===
extern uint16_t g_PWR;          // snaga [W], 0-150
extern uint16_t g_SWR;          // SWR x100 (150=1.50, 999=beskonacno)
extern uint8_t  g_Overload;     // 1 ako je ADC kanal 0 > 1000 (overload)

// === API ===
uint16_t adc_read(uint8_t channel);
uint16_t get_forward(void);
uint16_t get_reverse(void);
void     measure_pwr_swr(void);
void     print_pwr_swr(void);
void     print_pwr_swr_struct(void);

#endif // ADC_H
