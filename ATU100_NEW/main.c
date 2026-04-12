// ATU-100 Project
// PIC16F1938
// main.c
//
// Fosc = 32 MHz (8 MHz HFINTOSC + 4xPLL), Fcpu = 8 MHz
// UART: 9600 baud, TX=RB1 (blocking bit-bang), RX=RB2 (ISR/IOC, kružni bafer)

#include "main.h"
#include "selftest.h"
#include "help.h"

// CONFIG BITS - PIC16F1938
// CONFIG1
#pragma config FOSC     = INTOSC    // INTOSC oscilator, I/O na CLKIN pinu
#pragma config WDTE     = ON        // Watchdog Timer ukljucen
#pragma config PWRTE    = ON        // Power-up Timer ukljucen
#pragma config MCLRE    = ON        // MCLR pin je MCLR (RE3)
#pragma config CP       = OFF       // Nema code protection
#pragma config CPD      = OFF       // Nema data EEPROM protection
#pragma config BOREN    = ON        // Brown-out Reset ukljucen
#pragma config CLKOUTEN = OFF       // CLKOUT onemogucen (RA6 kao I/O)
#pragma config IESO     = OFF       // Internal/External Switch Over onemogucen
#pragma config FCMEN    = OFF       // Fail-Safe Clock Monitor onemogucen
// CONFIG2
#pragma config WRT      = OFF       // Flash write protection iskljucena
#pragma config VCAPEN   = OFF       // Voltage Regulator Capacitor onemogucen
#pragma config PLLEN    = ON        // 4xPLL ukljucen -> 8MHz * 4 = 32MHz
#pragma config STVREN   = ON        // Stack Overflow/Underflow reset ukljucen
#pragma config BORV     = LO        // BOR napon nizak (~1.9V)
#pragma config LVP      = OFF       // Low-Voltage Programming iskljucen

// ============================================================
// Komandni parser - ceka '\r', poredi sa poznatim komandama
// ============================================================
#define FW_VERSION      "v1.0"
#define CMD_BUF_SIZE    16

// "Lxxxxxxx\r" - postavi induktivnosti
// redosljed bita: Ind_45 Ind_22 Ind_1 Ind_045 Ind_022 Ind_011 Ind_005
static void cmd_set_inductors(const char *bits) {
    Ind_45  = (bits[0] == '1') ? 1 : 0;
    Ind_22  = (bits[1] == '1') ? 1 : 0;
    Ind_1   = (bits[2] == '1') ? 1 : 0;
    Ind_045 = (bits[3] == '1') ? 1 : 0;
    Ind_022 = (bits[4] == '1') ? 1 : 0;
    Ind_011 = (bits[5] == '1') ? 1 : 0;
    Ind_005 = (bits[6] == '1') ? 1 : 0;
    at_ind = (uint8_t)((Ind_45<<6)|(Ind_22<<5)|(Ind_1<<4)|(Ind_045<<3)|(Ind_022<<2)|(Ind_011<<1)|Ind_005);
    print_pwr_swr_struct();
}

// "Cxxxxxxxx\r" - postavi kondenzatore
// redosljed bita: Cap_sw Cap_1000 Cap_470 Cap_220 Cap_100 Cap_47 Cap_22 Cap_10
static void cmd_set_capacitors(const char *bits) {
    Cap_sw   = (bits[0] == '1') ? 1 : 0;
    Cap_1000 = (bits[1] == '1') ? 1 : 0;
    Cap_470  = (bits[2] == '1') ? 1 : 0;
    Cap_220  = (bits[3] == '1') ? 1 : 0;
    Cap_100  = (bits[4] == '1') ? 1 : 0;
    Cap_47   = (bits[5] == '1') ? 1 : 0;
    Cap_22   = (bits[6] == '1') ? 1 : 0;
    Cap_10   = (bits[7] == '1') ? 1 : 0;
    at_sw  = Cap_sw;
    at_cap = (uint8_t)((Cap_1000<<6)|(Cap_470<<5)|(Cap_220<<4)|(Cap_100<<3)|(Cap_47<<2)|(Cap_22<<1)|Cap_10);
    print_pwr_swr_struct();
}

// Parsiraj decimalni broj iz stringa (max 3 cifre)
static uint8_t parse_uint8(const char *s) {
    uint8_t val = 0;
    while (*s >= '0' && *s <= '9') {
        val = (uint8_t)(val * 10 + (*s - '0'));
        s++;
    }
    return val;
}

static void process_cmd(const char *cmd) {
    if (cmd[0]=='h' && cmd[1]=='e' && cmd[2]=='l' && cmd[3]=='p' && cmd[4]=='\0') {
        print_help();
    } else if (cmd[0]=='t' && cmd[1]=='u' && cmd[2]=='n' && cmd[3]=='e' && cmd[4]=='\0') {
        autotune();
    } else if (cmd[0]=='t' && cmd[1]=='e' && cmd[2]=='s' && cmd[3]=='t' && cmd[4]=='\0') {
        self_test();
    } else if (cmd[0] == 'L' && cmd[8] == '\0') {
        cmd_set_inductors(&cmd[1]);
    } else if (cmd[0] == 'C' && cmd[9] == '\0') {
        cmd_set_capacitors(&cmd[1]);
    } else if (cmd[0]=='p' && cmd[1]=='w' && cmd[2]=='r' && cmd[3]=='\0') {
        measure_pwr_swr();
        print_pwr_swr();
    } else if (cmd[0]=='c' && cmd[1]=='a' && cmd[2]=='l' && cmd[3]=='\0') {
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'K' && cmd[1] != '\0') {
        cal_K_mult = parse_uint8(&cmd[1]);
        cal_save();
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'M' && cmd[1] != '\0') {
        cal_min_power = parse_uint8(&cmd[1]);
        cal_save();
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'D' && cmd[1] != '\0') {
        // Unos je stvarna vrijednost x100 (npr. D134=1.34, D300=3.00)
        // Cuva se kao (val - 100), opseg 100-355 -> stored 0-255
        uint16_t d = 0;
        const char *p = &cmd[1];
        while (*p >= '0' && *p <= '9') { d = (uint16_t)(d * 10 + (*p - '0')); p++; }
        if (d >= 100 && d <= 355) {
            cal_divider_x100 = (uint8_t)(d - 100);
            cal_save();
        }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'S' && cmd[1] != '\0') {
        uint8_t s = parse_uint8(&cmd[1]);
        if (s > 0) { cal_coarse_step = s; cal_save(); }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'R' && cmd[1] != '\0') {
        uint8_t r = parse_uint8(&cmd[1]);
        if (r > 0) { cal_relay_ms = r; cal_save(); }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'W' && cmd[1] != '\0') {
        uint8_t w = parse_uint8(&cmd[1]);
        if (w > 0) { cal_retry_wait = w; cal_save(); }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0]=='l' && cmd[1]=='\0') {
        if (at_ind < 127) { at_ind++; at_set_ind(at_ind); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='k' && cmd[1]=='\0') {
        if (at_ind > 0) { at_ind--; at_set_ind(at_ind); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='c' && cmd[1]=='\0') {
        if (at_cap < 127) { at_cap++; at_set_cap(at_cap); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='x' && cmd[1]=='\0') {
        if (at_cap > 0) { at_cap--; at_set_cap(at_cap); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='z' && cmd[1]=='\0') {
        at_sw = at_sw ? 0 : 1;
        at_set_sw(at_sw);
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='s' && cmd[1]>='0' && cmd[1]<='9') {
        uint8_t n = parse_uint8(&cmd[1]);
        if (n >= 1 && n <= EE_CONFIG_MAX) relay_config_save(n);
        else DBG("?\r\n");
    } else if (cmd[0]=='r' && cmd[1]>='0' && cmd[1]<='9') {
        uint8_t n = parse_uint8(&cmd[1]);
        if (n >= 1 && n <= EE_CONFIG_MAX) relay_config_recall(n);
        else DBG("?\r\n");
    } else if (cmd[0] == 'V' && cmd[1] != '\0') {
        cal_debug = (cmd[1] == '1') ? 1 : 0;
        ee_write(EE_DEBUG, cal_debug);
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'A' && cmd[1] != '\0') {
        cal_adc_swap = (cmd[1] == '1') ? 1 : 0;
        ee_write(EE_ADC_SWAP, cal_adc_swap);
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0]=='f' && cmd[1]=='a' && cmd[2]=='c' && cmd[3]=='t' && cmd[4]=='o' && cmd[5]=='r' && cmd[6]=='y' && cmd[7]=='\0') {
        uart_print("FACTORY RESET\r\n");
        eeprom_factory_reset();
        asm("reset");
    } else if (cmd[0]=='r' && cmd[1]=='e' && cmd[2]=='b' && cmd[3]=='o' && cmd[4]=='o' && cmd[5]=='t' && cmd[6]=='\0') {
        // "reboot" -> PIC reset
        uart_print("REBOOT\r\n");
        asm("reset");
    } else if (cmd[0]=='r' && cmd[1]=='e' && cmd[2]=='s' && cmd[3]=='e' && cmd[4]=='t' && cmd[5]=='\0') {
        // "reset" -> svi releji na 0 (bypass), obrisi last slot
        cal_last_slot = 0;
        ee_write(EE_LAST_SLOT, 0x00);
        at_reset();
        uart_print("RESET OK\r\n");
    } else {
        DBG("?\r\n");
    }
}

// ============================================================
void main(void) {
    pic_init();
    // 2s cekanje za stabilizaciju POR/BOR (4x 500ms sa CLRWDT)
    __delay_ms(500); CLRWDT();
    __delay_ms(500); CLRWDT();
    __delay_ms(500); CLRWDT();
    __delay_ms(500); CLRWDT();
    eeprom_init();                          // ucitaj kalibraciju iz EEPROM-a (ili upisi defaults)
    __delay_ms(100); CLRWDT();             // cekaj stabilizaciju relay drajvera
    __delay_ms(100); CLRWDT();

    // DIJAGNOSTIKA: direktno citaj EEPROM slot prije recall-a
    {
        static const char hx[] = "0123456789ABCDEF";
        uint8_t daddr = EE_CONFIG_BASE + (uint8_t)((cal_last_slot - 1) * 2);
        uint8_t draw0 = ee_read(daddr);
        uint8_t draw1 = ee_read(daddr + 1);
        uart_print("BOOT:slot=");
        uart_putch('0' + cal_last_slot / 10);
        uart_putch('0' + cal_last_slot % 10);
        uart_print(" EE_IND=0x");
        uart_putch(hx[draw0 >> 4]); uart_putch(hx[draw0 & 0x0F]);
        uart_print(" EE_RAWCAP=0x");
        uart_putch(hx[draw1 >> 4]); uart_putch(hx[draw1 & 0x0F]);
        uart_print("\r\n");
    }

    if (cal_last_slot >= 1) relay_config_recall(cal_last_slot);  // vrati releje na poslednji aktivni slot

    // DIJAGNOSTIKA: ispisi stanje nakon recall-a
    //uart_print("AFTER_RECALL:");
    cal_print_struct();

    uart_print("ATU-100 " FW_VERSION "\r\n");

    char     cmd_buf[CMD_BUF_SIZE];
    uint8_t  cmd_len  = 0;
    uint16_t prev_PWR      = 0xFFFF;
    uint16_t prev_SWR      = 0xFFFF;
    uint16_t last_valid_swr = 999;

    while (1) {
        CLRWDT();

        // --- serial RX ---
        if (uart_available()) {
            char c = (char)uart_getch();
            if (c == '\r') {
                cmd_buf[cmd_len] = '\0';
                process_cmd(cmd_buf);
                cmd_len = 0;
            } else if (cmd_len < (CMD_BUF_SIZE - 1)) {
                cmd_buf[cmd_len++] = c;
            }
        }

        // --- PWR/SWR mjerenje svakih 500ms (non-blocking, Timer1 based) ---
        if (timer1_500ms_ready()) {
            measure_pwr_swr();
            if (g_PWR > 2) {
                if (g_PWR >= cal_min_power)
                    last_valid_swr = g_SWR;
                if (g_PWR != prev_PWR || g_SWR != prev_SWR) {
                    if (cal_debug) print_pwr_swr();
                    else           print_pwr_swr_struct();
                    prev_PWR = g_PWR;
                    prev_SWR = g_SWR;
                }
            } else if (prev_PWR > 2) {
                g_PWR = 0;
                g_SWR = last_valid_swr;
                if (cal_debug) print_pwr_swr();
                else           print_pwr_swr_struct();
                prev_PWR = 0;
                prev_SWR = 0;
            }
        }
    }
}
