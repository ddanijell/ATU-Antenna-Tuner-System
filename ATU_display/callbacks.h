#pragma once

// ============================================================
// Event callbacks
// ============================================================
static lv_timer_t *tune_hold_timer = NULL;
static lv_timer_t *save_hold_timer = NULL;

static void tune_hold_cb(lv_timer_t*) {
    lv_timer_del(tune_hold_timer);
    tune_hold_timer = NULL;
    send_cmd("tune\r");
}

static void save_green_cb(lv_timer_t*) {
    if (lbl_save_btn) {
        lv_obj_t *btn = lv_obj_get_parent(lbl_save_btn);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0A4A8A), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
    }
}

static void save_hold_cb(lv_timer_t*) {
    lv_timer_del(save_hold_timer);
    save_hold_timer = NULL;
    char cmd[8];
    snprintf(cmd, sizeof(cmd), "s%02d\r", bands[g_band].slot);
    send_cmd(cmd);
    g_save_pending = true;
    g_save_ms = millis();
    if (lbl_save_btn) {
        lv_obj_t *btn = lv_obj_get_parent(lbl_save_btn);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x005500), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x005500), LV_STATE_PRESSED);
        lv_timer_t *t = lv_timer_create(save_green_cb, 5000, NULL);
        lv_timer_set_repeat_count(t, 1);
    }
}

static void cb_tune(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        if (!tune_hold_timer) {
            tune_hold_timer = lv_timer_create(tune_hold_cb, 2000, NULL);
            lv_timer_set_repeat_count(tune_hold_timer, 1);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (tune_hold_timer) { lv_timer_del(tune_hold_timer); tune_hold_timer = NULL; }
    }
}

static void bypass_cancel(void) {
    if (!g_bypass_active) return;
    g_bypass_active = false;
    if (lbl_bypass_btn) lv_label_set_text(lbl_bypass_btn, "bypass OFF");
}

static void cb_band_plus(lv_event_t*) {
    bypass_cancel();
    g_band = (g_band+1) % BAND_COUNT;
    gui_update_band();
    char cmd[8];
    snprintf(cmd, sizeof(cmd), "r%d\r", bands[g_band].slot);
    send_cmd(cmd);
}
static void cb_band_minus(lv_event_t*) {
    bypass_cancel();
    g_band = (g_band==0) ? BAND_COUNT-1 : g_band-1;
    gui_update_band();
    char cmd[8];
    snprintf(cmd, sizeof(cmd), "r%d\r", bands[g_band].slot);
    send_cmd(cmd);
}
static void cb_config(lv_event_t*) {
    gui_update_cal();
    gui_update_lcsw_config();
    gui_update_status();
    lv_screen_load(scr_config);
}
static void cb_back(lv_event_t*) {
    lv_screen_load(scr_main);
}
static void cb_next(lv_event_t*) {
    lv_screen_load(scr_bright);
}
static void cb_back_bright(lv_event_t*) {
    lv_screen_load(scr_config);
}
static void cb_slider_l(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    int raw = (int)lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    int send_val = (raw * 4) - 1;
    if (send_val < 0) send_val = 0;

    if (code != LV_EVENT_RELEASED) return;
    bypass_cancel();

    char cmd[10];
    cmd[0] = 'L';
    for (int i = 6; i >= 0; i--)
        cmd[7 - i] = ((send_val >> i) & 1) ? '1' : '0';
    cmd[8] = '\r';
    cmd[9] = '\0';
    send_cmd(cmd);
}

static void cb_bypass(lv_event_t*) {
    if (!g_bypass_active) {
        // Snimi trenutno stanje i posalje reset
        g_saved_ind = g_pwr.ind;
        g_saved_cap = g_pwr.cap;
        g_saved_sw  = g_pwr.sw;
        send_cmd("reset\r");
        g_bypass_active = true;
        if (lbl_bypass_btn) lv_label_set_text(lbl_bypass_btn, "bypass ON");
        send_cmd("cal\r");
    } else {
        // Restauriraj snimljeno stanje
        char cmd[11];
        cmd[0] = 'L';
        for (int i = 6; i >= 0; i--)
            cmd[7-i] = ((g_saved_ind >> i) & 1) ? '1' : '0';
        cmd[8] = '\r'; cmd[9] = '\0';
        send_cmd(cmd);

        cmd[0] = 'C';
        cmd[1] = g_saved_sw ? '1' : '0';
        for (int i = 6; i >= 0; i--)
            cmd[2+(6-i)] = ((g_saved_cap >> i) & 1) ? '1' : '0';
        cmd[9] = '\r'; cmd[10] = '\0';
        send_cmd(cmd);

        g_bypass_active = false;
        if (lbl_bypass_btn) lv_label_set_text(lbl_bypass_btn, "bypass OFF");
        send_cmd("cal\r");
    }
}

static void cb_slider_c(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    int raw = (int)lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    int send_val = (raw * 4) - 1;
    if (send_val < 0) send_val = 0;

    if (code != LV_EVENT_RELEASED) return;
    bypass_cancel();

    char cmd[11];
    cmd[0] = 'C';
    cmd[1] = g_pwr.sw ? '1' : '0';  // Cap_sw = trenutni SW status
    for (int i = 6; i >= 0; i--)
        cmd[2 + (6 - i)] = ((send_val >> i) & 1) ? '1' : '0';
    cmd[9] = '\r';
    cmd[10] = '\0';
    send_cmd(cmd);
}
static void cb_l_plus(lv_event_t*)  { bypass_cancel(); send_cmd("l\r"); }
static void cb_l_minus(lv_event_t*) { bypass_cancel(); send_cmd("k\r"); }
static void cb_c_plus(lv_event_t*)  { bypass_cancel(); send_cmd("c\r"); }
static void cb_c_minus(lv_event_t*) { bypass_cancel(); send_cmd("x\r"); }
static void cb_sw_toggle(lv_event_t*) { bypass_cancel(); send_cmd("z\r"); }
static void cb_reset(lv_event_t*)   { send_cmd("reset\r"); g_reset_cal_ms = millis(); }
static void cb_reboot(lv_event_t*)  { send_cmd("reboot\r"); }
static void cb_save(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        if (!save_hold_timer) {
            save_hold_timer = lv_timer_create(save_hold_cb, 2000, NULL);
            lv_timer_set_repeat_count(save_hold_timer, 1);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (save_hold_timer) { lv_timer_del(save_hold_timer); save_hold_timer = NULL; }
    }
}
static void cb_brt_minus(lv_event_t*) { bl_set(g_brightness-30); lv_slider_set_value(slider_brt, g_brightness, LV_ANIM_OFF); }
static void cb_brt_plus(lv_event_t*)  { bl_set(g_brightness+30); lv_slider_set_value(slider_brt, g_brightness, LV_ANIM_OFF); }
static void cb_back_cal_edit(lv_event_t*) { lv_screen_load(scr_bright); }
static void cb_next_to_cal_edit(lv_event_t*) {
    // Ucitaj trenutne vrijednosti iz g_cal
    g_cedit_idx = 0;
    g_cedit_val = cedit_get_current(0);
    gui_update_cal_edit();
    lv_screen_load(scr_cal_edit);
}
static void cb_cedit_param(lv_event_t *e) {
    uint8_t idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    g_cedit_idx = idx;
    g_cedit_val = cedit_get_current(idx);
    gui_update_cal_edit();
}
static void cb_cedit_slider(lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t*)lv_event_get_target(e);
    g_cedit_val = (int)lv_slider_get_value(sl);
    gui_update_cal_edit();
}
static void cb_cedit_minus(lv_event_t*) {
    const CalParam &p = cal_params[g_cedit_idx];
    if (g_cedit_val > p.min_val) { g_cedit_val--; gui_update_cal_edit(); }
}
static void cb_cedit_plus(lv_event_t*) {
    const CalParam &p = cal_params[g_cedit_idx];
    if (g_cedit_val < p.max_val) { g_cedit_val++; gui_update_cal_edit(); }
}
static void cb_cedit_toggle(lv_event_t*) {
    g_cedit_val = g_cedit_val ? 0 : 1;
    gui_update_cal_edit();
}
static void cb_cedit_send(lv_event_t*) {
    char cmd[16];
    int send_val = g_cedit_val;
    if (g_cedit_idx == 5) send_val = g_cedit_val / 10;  // W: ms -> ATU jedinica
    snprintf(cmd, sizeof(cmd), "%c%d\r", cal_params[g_cedit_idx].cmd, send_val);
    send_cmd(cmd);
}

static lv_timer_t *unpair_hold_timer = NULL;

static void unpair_hold_cb(lv_timer_t*) {
    lv_timer_del(unpair_hold_timer);
    unpair_hold_timer = NULL;
    if (!is_paired) return;
    send_cmd("unpair\r");
    delay(100);
    nvs_clear_mac();
    memset(paired_mac, 0, 6);
    is_paired    = false;
    pair_request = false;
    gui_update_pair_btn();
    Serial.println("[UNPAIR] Pairing cleared.");
}

static void cb_unpair(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        if (is_paired && !unpair_hold_timer) {
            unpair_hold_timer = lv_timer_create(unpair_hold_cb, 2000, NULL);
            lv_timer_set_repeat_count(unpair_hold_timer, 1);
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (unpair_hold_timer) {
            lv_timer_del(unpair_hold_timer);
            unpair_hold_timer = NULL;
        }
    }
}
static void cb_slider_brt(lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t*)lv_event_get_target(e);
    int v = (int)lv_slider_get_value(sl);
    bl_set(v);  // PWM odmah, na svakom pomaku
    if (lv_event_get_code(e) == LV_EVENT_RELEASED)
        nvs_save_brightness((uint8_t)v);  // NVS samo po otpustanju
}
