// ATU-130 Project
// PIC18F2520
// adc.c - Mjerenje snage i SWR
//
// AN0: Forward power (prednja snaga)
// AN1: Reverse power (reflektovana snaga)
//
// Formula:
//   fwd_mV  = ADC(AN0) * 4.883
//   rev_mV  = ADC(AN1) * 4.883
//   SWR     = (fwd + rev) / (fwd - rev) * 100  [x100, 150=1.50]
//   V       = ge_correction(fwd_mV * DIVIDER_RATIO) * K_MULT / 1000 / 1.414  [RMS]
//   PWR     = V^2 / 5  [Low power, 0-150W]

#include "main.h"
#include "eeprom.h"
#include "adc.h"

// === GLOBALNE VARIJABLE ===
uint16_t g_PWR     = 0;
uint16_t g_SWR     = 999;
uint8_t  g_Overload = 0;

// ============================================================
// ADC - citanje kanala (10-bit, desno poravnanje, ADCON2=0x92)
// ============================================================
uint16_t adc_read(uint8_t channel) {
    ADCON0bits.CHS = channel;
    __delay_us(20);             // acquisition time
    ADCON0bits.GO  = 1;
    while (ADCON0bits.GO);
    return ((uint16_t)ADRESH << 8) | ADRESL;
}

// Forward napon na detektoru [mV] - average 16 mjerenja
// cal_adc_swap=0: AN0=fwd, cal_adc_swap=1: AN1=fwd
uint16_t get_forward(void) {
    uint8_t ch = cal_adc_swap ? 1 : 0;
    uint32_t sum = 0;
    uint8_t i;
    for (i = 0; i < 16; i++) sum += adc_read(ch);
    g_Overload = ((sum >> 4) > 1000) ? 1 : 0;
    return (uint16_t)((sum >> 4) * 4.883f);
}

// Reverse napon na detektoru [mV] - average 16 mjerenja
// cal_adc_swap=0: AN1=rev, cal_adc_swap=1: AN0=rev
uint16_t get_reverse(void) {
    uint8_t ch = cal_adc_swap ? 0 : 1;
    uint32_t sum = 0;
    uint8_t i;
    for (i = 0; i < 16; i++) sum += adc_read(ch);
    return (uint16_t)((sum >> 4) * 4.883f);
}

// ============================================================
// Korekcija za Ge (germanijum) diodu
// Kompenzuje nelinearnu karakteristiku detektora
// Portovano 1:1 iz ATU-100 main.h
// ============================================================
static int ge_correction(int input) {
    if (input <= 80)         return 0;
    if (input <= 171)        input += 244;
    else if (input <= 328)   input += 254;
    else if (input <= 582)   input += 280;
    else if (input <= 820)   input += 297;
    else if (input <= 1100)  input += 310;
    else if (input <= 2181)  input += 430;
    else if (input <= 3322)  input += 484;
    else if (input <= 4623)  input += 530;
    else if (input <= 5862)  input += 648;
    else if (input <= 7146)  input += 743;
    else if (input <= 8502)  input += 800;
    else if (input <= 10500) input += 840;
    else                     input += 860;
    return input;
}

// ============================================================
// Izracun PWR i SWR, rezultati u g_PWR i g_SWR
// ============================================================
void measure_pwr_swr(void) {
    uint16_t fwd_mV = get_forward();
    uint16_t rev_mV = get_reverse();

    // --- SWR ---
    if (rev_mV >= fwd_mV) {
        g_SWR = 999;
    } else {
        uint32_t swr = ((uint32_t)(fwd_mV + rev_mV) * 100UL) / (fwd_mV - rev_mV);
        g_SWR = (swr > 999) ? 999 : (uint16_t)swr;
    }

    // --- PWR ---
    float v = (float)ge_correction((int)((float)fwd_mV * ((float)(cal_divider_x100 + 100) / 100.0f)));
    v = v * cal_K_mult / 1000.0f;  // skaliranje (tandem match + kalibracija iz EEPROM)
    v = v / 1.414f;                 // peak -> RMS
    float pwr = v * v / 5.0f;      // formula vraca desetinke vata (ATU-100 original)
    pwr /= 10.0f;                   // konverzija u wate
    pwr += 0.5f;                    // zaokruzivanje
    g_PWR = (pwr > 150.0f) ? 150 : (uint16_t)pwr;

    // Ispod minimalne snage SWR nije relevantan
    if (g_PWR < cal_min_power)
        g_SWR = 999;

    CLRWDT();
}

// ============================================================
// Pomocne funkcije za ispis bez printf
// ============================================================
static void print_uint(uint16_t n) {
    char buf[6];
    uint8_t i = 5;
    buf[i] = '\0';
    if (n == 0) { uart_putch('0'); return; }
    while (n > 0) { buf[--i] = '0' + (n % 10); n /= 10; }
    uart_print(&buf[i]);
}

static void print_swr(uint16_t swr) {
    if (swr >= 999) { uart_print("---"); return; }
    uart_putch((char)('0' + swr / 100));
    uart_putch('.');
    uart_putch((char)('0' + (swr / 10) % 10));
    uart_putch((char)('0' + swr % 10));
}

// Ispis rezultata na serial TX - plain (human readable)
// Format: PWR=16W SWR=2.10 L=0.67uH C=870pF SW=0
void print_pwr_swr(void) {
    // L induktivnost x100 [uH*100]
    uint16_t ind_x100 = 0;
    if (at_ind & 0x01) ind_x100 += 5;
    if (at_ind & 0x02) ind_x100 += 11;
    if (at_ind & 0x04) ind_x100 += 22;
    if (at_ind & 0x08) ind_x100 += 45;
    if (at_ind & 0x10) ind_x100 += 100;
    if (at_ind & 0x20) ind_x100 += 220;
    if (at_ind & 0x40) ind_x100 += 450;

    // C kapacitivnost [pF]
    uint16_t cap_pF = 0;
    if (at_cap & 0x01) cap_pF += 10;
    if (at_cap & 0x02) cap_pF += 22;
    if (at_cap & 0x04) cap_pF += 47;
    if (at_cap & 0x08) cap_pF += 100;
    if (at_cap & 0x10) cap_pF += 220;
    if (at_cap & 0x20) cap_pF += 470;
    if (at_cap & 0x40) cap_pF += 1000;

    uart_print("PWR=");
    print_uint(g_PWR);
    uart_print("W SWR=");
    print_swr(g_SWR);
    if (g_Overload) uart_print(" OVR");
    uart_print(" L=");
    uart_putch('0' + (uint8_t)(ind_x100 / 100));
    uart_putch('.');
    uart_putch('0' + (uint8_t)((ind_x100 % 100) / 10));
    uart_putch('0' + (uint8_t)(ind_x100 % 10));
    uart_print("uH C=");
    print_uint(cap_pF);
    uart_print("pF SW=");
    uart_putch('0' + at_sw);
    uart_print("\r\n");
}

// Ispis rezultata na serial TX - strukturalni (za ESP32)
// Format: $PWR:10,210,LL,CC,S
//   10  = snaga [W]
//   210 = SWR x100
//   LL  = L bitmap hex (npr. 05)
//   CC  = C bitmap hex (npr. 13)
//   S   = SW relay (0 ili 1)
void print_pwr_swr_struct(void) {
    static const char hex[] = "0123456789ABCDEF";
    uart_print("$PWR:");
    print_uint(g_PWR);
    uart_putch(',');
    print_uint(g_SWR);
    uart_putch(',');
    uart_putch(hex[at_ind >> 4]);
    uart_putch(hex[at_ind & 0x0F]);
    uart_putch(',');
    uart_putch(hex[at_cap >> 4]);
    uart_putch(hex[at_cap & 0x0F]);
    uart_putch(',');
    uart_putch('0' + at_sw);
    uart_print("\r\n");
}
