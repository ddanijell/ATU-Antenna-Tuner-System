// ATU_display
// ESP32 CYD (Cheap Yellow Display) 240x320 - ATU front-end GUI
//
// Ekran 1 (Main):    PWR/SWR + relay + band info + [TUNE][Band-][Band+][CONFIG]
// Ekran 2 (Config):  CAL + [L-][L+][C-][C+] + [SW][RESET][REBOOT][SAVE] + [BACK][Next>]
// Ekran 3 (Bright):  Brightness slider + [BACK]
//
// ESP-NOW protokol (isti kao ATU_transiver):
//   [0xAA][TYPE][poruka]
//   TYPE: 0x01=$PWR, 0x02=$CAL, 0x03=CMD, 0x04=$TUNE, 0x05=ANN
//
// Biblioteke: lvgl 9.2, TFT_eSPI, XPT2046_Touchscreen

#define APP_VERSION  "v1.04"

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include "driver/ledc.h"
#include <SPI.h>
#include <SD.h>
    //#define LV_USE_FONT_COMPRESSION 1
    LV_FONT_DECLARE(lv_font_montserrat_sb_16);
    LV_FONT_DECLARE(lv_font_montserrat_sb_20);
    LV_FONT_DECLARE(lv_font_montserrat_sb_24);
    LV_FONT_DECLARE(lv_font_montserrat_sb_28);



// ============================================================
// CYD PINOUT - definisan u tft_setup.h (BL_PIN, XPT2046_*, CYD_SHARED_SPI)
// ============================================================



// Baterija
#define BAT_PIN         35   // ADC1_CH7, input-only, deljac 470k/100k

// SD kartica
#define SD_MOSI         23
#define SD_MISO         19
#define SD_CLK          18
#define SD_CS           5
static SPIClass sdSPI(VSPI);   // VSPI - sekvencijalno sa touchscreenom, nema konflikta sa TFT/HSPI
#define LEDC_CH         LEDC_CHANNEL_0
#define LEDC_TIM        LEDC_TIMER_0
#define LEDC_FREQ       5000
#define LEDC_RES        LEDC_TIMER_8_BIT
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   320

// ============================================================
// ESP-NOW
// ============================================================
#define ESPNOW_MAGIC     0xAA
#define ESPNOW_TYPE_PWR  0x01
#define ESPNOW_TYPE_CAL  0x02
#define ESPNOW_TYPE_CMD  0x03
#define ESPNOW_TYPE_TUNE 0x04
#define ESPNOW_TYPE_ANN  0x05    // Announce/pairing: payload = 6-byte MAC
#define ESPNOW_TYPE_PING 0x06    // Ping: Display -> Transiver (no payload)
#define ESPNOW_TYPE_PONG 0x07    // Pong: Transiver -> Display (no payload)
#define ESPNOW_CHANNEL   14
static uint8_t broadcast_mac[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

// === PAIRING STATE ===
static uint8_t  paired_mac[6]    = {0};
static bool     is_paired         = false;
static Preferences prefs;
#define NVS_NS      "atapair"
#define NVS_KEY_MAC "mac"

// Pending pairing request waiting for user confirmation
static volatile bool pair_request   = false;
static uint8_t       pair_req_mac[6] = {0};

// ============================================================
// Bandovi (11 amatersih opsega)
// ============================================================
struct Band { const char *label; uint8_t slot; };
static const Band bands[] = {
    {"1.8 MHz", 1}, {"3.5 MHz", 2}, {"5 MHz",  3},
    {"7 MHz",   4}, {"10 MHz",  5}, {"14 MHz", 6},
    {"18 MHz",  7}, {"21 MHz",  8}, {"25 MHz", 9},
    {"27 MHz", 10}, {"28 MHz", 11}
};
#define BAND_COUNT 11
static uint8_t g_band = 5;   // default: 14MHz (index 5)

// ============================================================
// CAL parametri za editovanje (screen4)
// ============================================================
struct CalParam {
    const char *label;    // kratko ime za dugme
    const char *desc;     // pun naziv
    char        cmd;      // slovo komande (K, M, D, S, R, W, V, A)
    int         min_val;
    int         max_val;
    bool        is_bool;
};
static const CalParam cal_params[] = {
    { "K", "K_MULT calibration\ndefault=16   range=1-255",        'K', 1,    255,  false },
    { "M", "Minimum power for tuning [W]\ndefault=5    range=1-20",    'M', 1,    20,   false },
    { "D", "ADC divider ratio\nrange 1.00-3.55",                      'D', 100,  355,  false },
    { "S", "Coarse search step\ndefault=8    range=1-32", 'S', 1,    32,   false },
    { "R", "Relay delay [ms]\ndefault=25   range=10-50",'R', 10,   50,   false },
    { "W", "Wait for PWR to continue\ndefault=500ms  range=100-1000ms",         'W', 100,  1000, false },
    { "V", "Debug output OFF / ON\ndefault=1",                     'V', 0,    1,    true  },
    { "A", "ADC input swap OFF / ON\ndefault=0",                        'A', 0,    1,    true  },
};
#define CAL_PARAM_COUNT 8

// ============================================================
// Podaci
// ============================================================
struct PwrData { uint16_t pwr, swr; uint8_t ind, cap, sw; bool overload; };
struct CalData {
    uint8_t k_mult, min_pwr, coarse_step, relay_ms, retry_wait, debug, adc_swap;
    uint16_t div;       // x100
    uint8_t last_slot;  // poslednji aktivni slot (1-30)
    uint8_t ind, cap, sw; // trenutno stanje releja iz ATU
};
static PwrData  g_pwr       = {0, 999, 0, 0, 0, false};
static CalData  g_cal       = {};
static bool     g_cal_valid = false;
static uint32_t last_rx_ms  = 0;   // zadnji primljeni paket
static uint32_t last_hb_ms  = 0;   // zadnji heartbeat poslan
#define CONNECTED_TIMEOUT_MS  15000
#define HEARTBEAT_MS          30000

// Thread-safe bufer
static volatile bool new_pwr = false, new_cal = false, new_tune = false;
static char pwr_buf[64], cal_buf[64], tune_buf[12];

// Zadnje validne vrijednosti PWR/SWR (za prikaz kad padne na 0)
static uint16_t g_last_valid_pwr = 0;
static uint16_t g_last_valid_swr = 999;

// ESP-NOW signal strength (RSSI zadnjeg paketa)
static int8_t g_rssi = 0;

// SAVE potvrda
static bool     g_save_pending   = false;
static uint32_t g_save_ms        = 0;
#define SAVE_TIMEOUT_MS  3000

// Nakon RESET komande: pošalji cal nakon kratke pauze
static uint32_t g_reset_cal_ms   = 0;   // != 0 kad čekamo na cal refresh
#define RESET_CAL_DELAY_MS  1000        // 1s nakon reset

// $TUNE status
static uint8_t  g_tune_status    = 0;
static uint32_t g_tune_done_ms   = 0;   // timestamp kad je stigao finalni status
#define TUNE_CLEAR_MS  5000             // 5s pa reset na "TUNE"
static const char* const tune_labels[] = {
    "TUNE",       // 0 - idle
    "WAIT PWR",   // 1
    "TUNING...",  // 2
    "RETRY",      // 3
    "COARSE",     // 4
    "SCAP",       // 5
    "SIND",       // 6
    "SW1",        // 7
    "SW2",        // 8
    "SW1 BETTER", // 9
    "SW2 BETTER", // 10
    "FINE",       // 11
    "TUNE OK",    // 12
    "DONE",       // 13
    "ABORT",      // 14
    "SKIP"        // 15
};

// ============================================================
// Brightness
// ============================================================
static int g_brightness = 180;
static void bl_init(void) {
    ledc_timer_config_t t = { .speed_mode=LEDC_LOW_SPEED_MODE,
        .duty_resolution=LEDC_RES, .timer_num=LEDC_TIM,
        .freq_hz=LEDC_FREQ, .clk_cfg=LEDC_AUTO_CLK };
    ledc_timer_config(&t);
    ledc_channel_config_t ch = { .gpio_num=BL_PIN,
        .speed_mode=LEDC_LOW_SPEED_MODE, .channel=LEDC_CH,
        .timer_sel=LEDC_TIM, .duty=0, .hpoint=0,
        .flags={.output_invert=0} };
    ledc_channel_config(&ch);
}
static float read_bat_voltage(void) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(BAT_PIN);
    float v_pin_mv = sum / 16.0f;
    return v_pin_mv * 5.7f / 1000.0f;  // mV->V, deljac 470k/100k, faktor 5.7
}

static void bl_set(int v) {
    v = v < 0 ? 0 : v > 255 ? 255 : v;
    g_brightness = v;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CH, v);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CH);
}

// ============================================================
// LVGL ekrani i objekti
// ============================================================
static lv_obj_t *scr_main;
static lv_obj_t *scr_config;
static lv_obj_t *scr_bright;
static lv_obj_t *scr_cal_edit;

// --- Main screen objekti ---
static lv_obj_t *lbl_status;
static lv_obj_t *lbl_ovr;
static lv_obj_t *lbl_pwr_val;
static lv_obj_t *lbl_swr_val;
static lv_obj_t *bar_pwr;
static lv_obj_t *bar_swr;
static lv_obj_t *lbl_relay;
static lv_obj_t *lbl_band_freq;
static lv_obj_t *lbl_bat;

// --- Main screen - TUNE dugme label ---
static lv_obj_t *lbl_tune_btn;

// --- Config screen objekti ---
static lv_obj_t *lbl_cfg_status;   // status u headeru bright screena (screen3)
static lv_obj_t *lbl_cfg_status2;  // status u headeru config screena (screen2)
static lv_obj_t *slider_brt;
static lv_obj_t *slider_l_direct = NULL;
static lv_obj_t *slider_c_direct = NULL;
static lv_obj_t *lbl_bypass_btn  = NULL;
static bool      g_bypass_active  = false;
static uint8_t   g_saved_ind = 0, g_saved_cap = 0, g_saved_sw = 0;
static lv_obj_t *lbl_l_val;       // trenutna L vrijednost (npr. "0.67 uH")
static lv_obj_t *lbl_c_val;       // trenutna C vrijednost (npr. "870 pF")
static lv_obj_t *btn_sw;          // SW toggle dugme
static lv_obj_t *lbl_sw_state;    // label unutar SW dugmeta ("SW0" / "SW1")
static lv_obj_t *lbl_save_btn;    // label unutar SAVE dugmeta (dinamicki slot)
static lv_obj_t *lbl_unpair_btn;  // label unutar UNPAIR dugmeta (dinamicki)
// --- CAL edit screen objekti ---
static lv_obj_t *lbl_cedit_name;
static lv_obj_t *lbl_cedit_val;
static lv_obj_t *slider_cedit;
static lv_obj_t *lbl_cedit_send;
static lv_obj_t *cedit_slider_row;   // kontejner slider + fine buttons (sakrij za bool)
static lv_obj_t *cedit_toggle_row;   // kontejner toggle dugmeta (za bool)
static lv_obj_t *lbl_cedit_toggle;
static lv_obj_t *param_btns[CAL_PARAM_COUNT];
static uint8_t   g_cedit_idx = 0;
static int       g_cedit_val = 0;
static lv_obj_t *pair_dialog = NULL;  // overlay pairing dialog

// ============================================================
// Touchscreen
// ============================================================
#ifdef CYD_SHARED_SPI
SPIClass touchscreenSPI(HSPI);   // dijeli HSPI sa TFT, razlicit CS
#else
SPIClass touchscreenSPI(VSPI);   // odvojen VSPI, razliciti pinovi
#endif
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf[DRAW_BUF_SIZE / 4];
static void touchscreen_read(lv_indev_t*, lv_indev_data_t *data) {
    if (touchscreen.tirqTouched() && touchscreen.touched()) {
        TS_Point p = touchscreen.getPoint();
#ifdef CYD_TOUCH_MIRROR_X
        int16_t x = (int16_t)map(p.x, TOUCH_X_MAX, TOUCH_X_MIN, 0, SCREEN_WIDTH  - 1);
#else
        int16_t x = (int16_t)map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_WIDTH  - 1);
#endif
#ifdef CYD_TOUCH_MIRROR_Y
        int16_t y = (int16_t)map(p.y, TOUCH_Y_MAX, TOUCH_Y_MIN, 0, SCREEN_HEIGHT - 1);
#else
        int16_t y = (int16_t)map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_HEIGHT - 1);
#endif


        if (x < 0) x = 0; if (x > SCREEN_WIDTH  - 1) x = SCREEN_WIDTH  - 1;
        if (y < 0) y = 0; if (y > SCREEN_HEIGHT - 1) y = SCREEN_HEIGHT - 1;
        data->point.x = x;
        data->point.y = y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

#include "parsing.h"
#include "espnow_comm.h"
#include "gui_update.h"
#include "callbacks.h"
#include "gui_screens.h"
#include "ota_update.h"

// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[ATU_display_SD] boot " APP_VERSION);

    // --- SD kartica init (VSPI - sekvencijalno sa touchscreenom, nema konflikta sa TFT/HSPI) ---
    sdSPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, sdSPI)) {
        Serial.println("[SD] Mount FAIL - nema kartice ili greska");
    } else {
        Serial.printf("[SD] Mount OK  - velicina: %llu MB\n", SD.cardSize() / (1024 * 1024));
        if (SD.exists("/firmware.bin")) {
            Serial.println("[SD] firmware.bin PRONADJEN - startujem OTA prompt");
        } else {
            Serial.println("[SD] firmware.bin nije pronadjen");
        }
        check_and_ota();  // prikazuje prompt i radi update ako korisnik potvrdi
    }

    lv_init();
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(2);

    lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT,
                                             draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchscreen_read);

    bl_init();
    g_brightness = nvs_load_brightness(g_brightness);
    bl_set(g_brightness);

    create_main_screen();
    create_config_screen();
    create_bright_screen();
    create_cal_edit_screen();
    lv_screen_load(scr_main);

    espnow_init();
    gui_update_pair_btn();  // osvjezi nakon sto espnow_init ucita NVS
    gui_update_bat();       // prvi ocitaj baterije odmah pri startu
    delay(500);
    send_cmd("V0\r");
    delay(200);
    send_cmd("cal\r");
}

// ============================================================
void loop() {
    uint32_t now = millis();

    // Obrada novih podataka
    if (new_pwr) {
        new_pwr = false;
        parse_pwr(pwr_buf);
        gui_update_pwr();
        gui_update_status();
        g_save_pending = false;
    }
    if (new_cal) {
        new_cal = false;
        parse_cal(cal_buf);
        gui_update_cal();
        gui_update_band();
        gui_update_pwr();
        gui_update_status();
    }
    if (new_tune) {
        new_tune = false;
        // parse "$TUNE:xx"
        const char *p = tune_buf;
        while (*p && *p != ':') p++;
        if (*p == ':') {
            p++;
            uint8_t n = 0;
            while (*p >= '0' && *p <= '9') { n = n*10 + (*p-'0'); p++; }
            g_tune_status = (n < 16) ? n : 0;
            gui_update_tune_btn();
            // finalni statusi: OK(12), DONE(13), ABORT(14), SKIP(15)
            if (g_tune_status >= 12) { g_tune_done_ms = millis(); bypass_cancel(); }
            else                       g_tune_done_ms = 0;
        }
    }

    // --- Pairing request: prikazi dialog ako je stigao ANN ---
    if (pair_request) {
        pair_request = false;
        show_pair_dialog(pair_req_mac);
    }

    // Heartbeat: ako nema podataka 30s, pošalji cal
    if (last_hb_ms == 0 || (now - last_rx_ms > HEARTBEAT_MS &&
                             now - last_hb_ms  > HEARTBEAT_MS)) {
        last_hb_ms = now;
        send_cmd("cal\r");
        g_last_valid_pwr = 0;
        g_last_valid_swr = 999;
        gui_update_pwr();
    }

    // cal refresh nakon RESET komande (1s delay da ATU završi reset)
    if (g_reset_cal_ms != 0 && (now - g_reset_cal_ms) >= RESET_CAL_DELAY_MS) {
        g_reset_cal_ms = 0;
        send_cmd("cal\r");
    }

    // Reset TUNE dugmeta na "TUNE" nakon 5s od finalnog statusa
    if (g_tune_done_ms != 0 && (now - g_tune_done_ms) >= TUNE_CLEAR_MS) {
        g_tune_done_ms = 0;
        g_tune_status  = 0;
        gui_update_tune_btn();
    }

    // Provjeri connection status svakih 2s
    static uint32_t prev_status = 0;
    if (now - prev_status > 2000) {
        prev_status = now;
        gui_update_status();
    }

    // Ocitaj napon baterije svakih 10s
    static uint32_t prev_bat = 0;
    if (now - prev_bat > 10000) {
        prev_bat = now;
        gui_update_bat();
    }

    // PING svakih 5s za monitoring konekcije
    static uint32_t prev_ping = 0;
    if (is_paired && (now - prev_ping > 5000)) {
        prev_ping = now;
        send_ping();
    }



    lv_task_handler();
    lv_tick_inc(5);
    delay(5);
}
