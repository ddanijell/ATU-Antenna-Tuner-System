// ATU-130 Project
// PIC18F2520
// main.c
//
// Fosc = 16 MHz (4 MHz HFINTOSC + 4xPLL), Fcpu = 4 MHz
// UART: 9600 baud, TX=RB1 (blocking bit-bang), RX=RB2 (ISR/INT2, kružni bafer)

#include "main.h"
#include "selftest.h"
#include "help.h"

// CONFIG BITS
#pragma config OSC    = INTIO67
#pragma config FCMEN  = OFF
#pragma config IESO   = OFF
#pragma config PWRT   = ON
#pragma config BOREN  = SBORDIS
#pragma config BORV   = 3
#pragma config WDT    = ON
#pragma config WDTPS  = 32768
#pragma config CCP2MX = PORTC
#pragma config PBADEN = OFF
#pragma config MCLRE  = ON
#pragma config STVREN = ON
#pragma config LVP    = OFF
#pragma config XINST  = OFF

// ============================================================
// Timer0 helper - citanje 16-bit free-running countera
// TMR0L citanje latchuje TMR0H (PIC18 datasheet)
// ============================================================
#define TIMER0_TICKS_500MS  31250u      // 500ms / 16us po ticku

static uint16_t timer0_read(void) {
    uint8_t lo = TMR0L;                 // latch TMR0H
    return ((uint16_t)TMR0H << 8) | lo;
}

// ============================================================
// Komandni parser - ceka '\r', poredi sa poznatim komandama
// ============================================================
#define FW_VERSION      "v1.0"
#define CMD_BUF_SIZE    16

// "Lxxxxxxx\r" - postavi induktivnosti
// redosljed bita: Ind_45 Ind_22 Ind_1 Ind_045 Ind_022 Ind_011 Ind_005
// primjer: L1010011 -> Ind_45=1, Ind_22=0, Ind_1=1, Ind_045=0, Ind_022=0, Ind_011=1, Ind_005=1
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
// primjer: C10000001 -> Cap_sw=1, Cap_1000=0...Cap_22=0, Cap_10=1
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
        // "help" -> ispisi pomoc
        print_help();
    } else if (cmd[0]=='t' && cmd[1]=='u' && cmd[2]=='n' && cmd[3]=='e' && cmd[4]=='\0') {
        // "tune" -> autotune()
        autotune();
    } else if (cmd[0]=='t' && cmd[1]=='e' && cmd[2]=='s' && cmd[3]=='t' && cmd[4]=='\0') {
        // "test" -> self_test()
        self_test();
    } else if (cmd[0] == 'L' && cmd[8] == '\0') {
        // "Lxxxxxxx" -> postavi induktivnosti (7 bita)
        cmd_set_inductors(&cmd[1]);
    } else if (cmd[0] == 'C' && cmd[9] == '\0') {
        // "Cxxxxxxxx" -> postavi kondenzatore (8 bita)
        cmd_set_capacitors(&cmd[1]);
    } else if (cmd[0]=='p' && cmd[1]=='w' && cmd[2]=='r' && cmd[3]=='\0') {
        // "pwr" -> izmjeri i ispisi PWR i SWR
        measure_pwr_swr();
        print_pwr_swr();
    } else if (cmd[0]=='c' && cmd[1]=='a' && cmd[2]=='l' && cmd[3]=='\0') {
        // "cal" -> ispisi trenutnu kalibraciju
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'K' && cmd[1] != '\0') {
        // "Kxx" -> postavi K_MULT (kalibracija), sacuvaj u EEPROM
        cal_K_mult = parse_uint8(&cmd[1]);
        cal_save();
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'M' && cmd[1] != '\0') {
        // "Mxx" -> postavi MIN_POWER_W, sacuvaj u EEPROM
        cal_min_power = parse_uint8(&cmd[1]);
        cal_save();
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'D' && cmd[1] != '\0') {
        // "Dxxx" -> postavi divider x100 (npr. D134=1.34, D300=3.00), sacuvaj u EEPROM
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
        // "Sxx" -> postavi coarse step (npr. S8), sacuvaj u EEPROM
        uint8_t s = parse_uint8(&cmd[1]);
        if (s > 0) { cal_coarse_step = s; cal_save(); }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'R' && cmd[1] != '\0') {
        // "Rxx" -> postavi relay delay [ms], sacuvaj u EEPROM
        uint8_t r = parse_uint8(&cmd[1]);
        if (r > 0) { cal_relay_ms = r; cal_save(); }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'W' && cmd[1] != '\0') {
        // "Wxx" -> postavi retry wait x10ms (npr. W10=100ms), sacuvaj u EEPROM
        uint8_t w = parse_uint8(&cmd[1]);
        if (w > 0) { cal_retry_wait = w; cal_save(); }
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0]=='l' && cmd[1]=='\0') {
        // "l" -> L + 1
        if (at_ind < 127) { at_ind++; at_set_ind(at_ind); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='k' && cmd[1]=='\0') {
        // "k" -> L - 1
        if (at_ind > 0) { at_ind--; at_set_ind(at_ind); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='c' && cmd[1]=='\0') {
        // "c" -> C + 1
        if (at_cap < 127) { at_cap++; at_set_cap(at_cap); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='x' && cmd[1]=='\0') {
        // "x" -> C - 1
        if (at_cap > 0) { at_cap--; at_set_cap(at_cap); }
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='z' && cmd[1]=='\0') {
        // "z" -> SW toggle
        at_sw = at_sw ? 0 : 1;
        at_set_sw(at_sw);
        if (cal_debug) print_lcsw_values(); else print_pwr_swr_struct();
    } else if (cmd[0]=='s' && cmd[1]>='0' && cmd[1]<='9') {
        // "s01"-"s30" -> snimi L/C/SW u slot
        uint8_t n = parse_uint8(&cmd[1]);
        if (n >= 1 && n <= EE_CONFIG_MAX) relay_config_save(n);
        else DBG("?\r\n");
    } else if (cmd[0]=='r' && cmd[1]>='0' && cmd[1]<='9') {
        // "r01"-"r30" -> ucitaj L/C/SW iz slota
        uint8_t n = parse_uint8(&cmd[1]);
        if (n >= 1 && n <= EE_CONFIG_MAX) relay_config_recall(n);
        else DBG("?\r\n");
    } else if (cmd[0] == 'V' && cmd[1] != '\0') {
        // "V0" -> debug OFF, "V1" -> debug ON, sacuvaj u EEPROM
        cal_debug = (cmd[1] == '1') ? 1 : 0;
        ee_write(EE_DEBUG, cal_debug);
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0] == 'A' && cmd[1] != '\0') {
        // "A0" -> ADC swap OFF, "A1" -> ADC swap ON, sacuvaj u EEPROM
        cal_adc_swap = (cmd[1] == '1') ? 1 : 0;
        ee_write(EE_ADC_SWAP, cal_adc_swap);
        if (cal_debug) cal_print(); else cal_print_struct();
    } else if (cmd[0]=='f' && cmd[1]=='a' && cmd[2]=='c' && cmd[3]=='t' && cmd[4]=='o' && cmd[5]=='r' && cmd[6]=='y' && cmd[7]=='\0') {
        // "factory" -> obrisi EEPROM magic, PIC reset -> defaults na sljedecem boot-u
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
    eeprom_init();                          // ucitaj kalibraciju iz EEPROM-a (ili upiši defaults)
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

    uart_print("ATU-130 " FW_VERSION "\r\n");
    //self_test();

    char     cmd_buf[CMD_BUF_SIZE];
    uint8_t  cmd_len  = 0;
    uint16_t t_last   = timer0_read();
    uint16_t prev_PWR      = 0xFFFF;
    uint16_t prev_SWR      = 0xFFFF;
    uint16_t last_valid_swr = 999;   // posljednji SWR dok je snaga bila iznad min

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

        // --- PWR/SWR mjerenje svakih 500ms (non-blocking) ---
        uint16_t t_now = timer0_read();
        if ((uint16_t)(t_now - t_last) >= TIMER0_TICKS_500MS) {
            t_last = t_now;
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
                // snaga pala na nulu - posalji PWR=0W sa posljednjim validnim SWR
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
