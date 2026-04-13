#pragma once

// ============================================================
// Pomocne
// ============================================================
static bool is_connected(void) {
    return g_cal_valid && last_rx_ms!=0 &&
           (millis()-last_rx_ms) < CONNECTED_TIMEOUT_MS;
}
static lv_color_t swr_color(uint16_t swr) {
    if (swr>=999) return lv_color_hex(0x444444);
    if (swr<150)  return lv_color_hex(0x00CC44);
    if (swr<250)  return lv_color_hex(0xFFAA00);
    return              lv_color_hex(0xFF2200);
}
static void fmt_swr(char *buf, uint16_t swr) {
    if (swr>=999) strcpy(buf,"---");
    else sprintf(buf,"%d.%02d", swr/100, swr%100);
}
// Kreira dugme sa labelom na zadanom screenu
static lv_obj_t* make_btn(lv_obj_t *parent, const char *txt,
                           int x, int y, int w, int h,
                           uint32_t color, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl);
    return btn;
}
// Horizontalni separator
static void make_sep(lv_obj_t *parent, int y) {
    lv_obj_t *s = lv_obj_create(parent);
    lv_obj_set_size(s, 320, 1);
    lv_obj_set_pos(s, 0, y);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x222244), 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_pad_all(s, 0, 0);
}

// ============================================================
// GUI update - Main screen
// ============================================================
static void gui_update_status(void) {
    const char *txt; uint32_t col;
    static char status_buf[48];
    if (is_connected()) {
        snprintf(status_buf, sizeof(status_buf), LV_SYMBOL_OK " ATU  %ddBm", (int)g_rssi);
        txt = status_buf; col = 0x00CC44;
    } else if (g_cal_valid) {
        txt = LV_SYMBOL_CLOSE " ATU - no signal"; col = 0xFF2200;
    } else {
        snprintf(status_buf, sizeof(status_buf), LV_SYMBOL_WIFI " ATU - waiting...");
        txt = status_buf; col = 0x888888;
    }
    if (lbl_status) {
        lv_label_set_text(lbl_status, txt);
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(col), 0);
        lv_label_set_text(lbl_ovr, g_pwr.overload ? "OVR!" : "");
    }
    if (lbl_cfg_status) {
        lv_label_set_text(lbl_cfg_status, txt);
        lv_obj_set_style_text_color(lbl_cfg_status, lv_color_hex(col), 0);
    }
    if (lbl_cfg_status2) {
        lv_label_set_text(lbl_cfg_status2, txt);
        lv_obj_set_style_text_color(lbl_cfg_status2, lv_color_hex(col), 0);
    }
}

static void gui_update_lcsw_config(void);

static void gui_update_pwr(void) {
    bool conn = is_connected();
    char buf[16];

    // Pamti zadnje validne vrijednosti dok je snaga bila > 0
    if (conn && g_pwr.pwr > 0) {
        g_last_valid_pwr = g_pwr.pwr;
        g_last_valid_swr = g_pwr.swr;
    }

    // PWR: ako connected ali pwr=0 → prikazi zadnju vrijednost sivom, ako nema → ---
    if (!conn || (g_pwr.pwr == 0 && g_last_valid_pwr == 0)) {
        strcpy(buf, "---");
        lv_obj_set_style_text_color(lbl_pwr_val, lv_color_hex(0x444444), 0);
    } else if (g_pwr.pwr == 0 && g_last_valid_pwr > 0) {
        sprintf(buf, "%d", g_last_valid_pwr);
        lv_obj_set_style_text_color(lbl_pwr_val, lv_color_hex(0x222222), 0);
    } else {
        sprintf(buf, "%d", g_pwr.pwr);
        lv_obj_set_style_text_color(lbl_pwr_val, lv_color_hex(0xFFFFFF), 0);
    }
    lv_label_set_text(lbl_pwr_val, buf);

    // SWR: ako connected ali pwr=0 → prikazi zadnji validni SWR sivom, ako nema → ---
    uint16_t swr;
    bool swr_stale = false;
    if (!conn || (g_pwr.pwr == 0 && g_last_valid_swr == 999)) {
        swr = 999;
    } else if (g_pwr.pwr == 0 && g_last_valid_swr != 999) {
        swr = g_last_valid_swr;
        swr_stale = true;
    } else {
        swr = g_pwr.swr;
    }
    fmt_swr(buf, swr);
    lv_label_set_text(lbl_swr_val, buf);
    lv_obj_set_style_text_color(lbl_swr_val,
        swr_stale ? lv_color_hex(0x222222) : swr_color(swr), 0);

    // PWR bar (0-100W, zelena)
    int pval = 0;
    if (conn && g_pwr.pwr > 0)
        pval = (int)(g_pwr.pwr > 100 ? 100 : g_pwr.pwr);
    lv_bar_set_value(bar_pwr, pval, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_pwr,
        (conn && g_pwr.pwr > 0) ? lv_color_hex(0x00CC44) : lv_color_hex(0x222222),
        LV_PART_INDICATOR);

    // SWR bar: ne mijenjaj kad je stale
    int bval = 0;
    if (conn && !swr_stale && swr < 999)
        bval = (int)((swr > 400 ? 400 : swr) * 100 / 400);
    lv_bar_set_value(bar_swr, bval, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_swr,
        swr_stale ? lv_color_hex(0x444444) : swr_color(swr), LV_PART_INDICATOR);

    // Relay
    if (conn) {
        uint16_t uh100 = ind_to_uh100(g_pwr.ind);
        uint16_t pf    = cap_to_pf(g_pwr.cap);
        char rs[52];
        if (g_pwr.sw == 0) {
            snprintf(rs, sizeof(rs), "C:%dpF  L:%d.%02duH  CL Filter",
                     pf, uh100/100, uh100%100);
        } else {
            snprintf(rs, sizeof(rs), "L:%d.%02duH  C:%dpF  LC Filter",
                     uh100/100, uh100%100, pf);
        }
        lv_label_set_text(lbl_relay, rs);
        lv_obj_set_style_text_color(lbl_relay, lv_color_hex(0xCCCCCC), 0);
    } else {
        lv_label_set_text(lbl_relay, "L: ---   C: ---");
        lv_obj_set_style_text_color(lbl_relay, lv_color_hex(0x444444), 0);
    }
    gui_update_lcsw_config();
}

static void gui_update_band(void) {
    if (lbl_band_freq) lv_label_set_text(lbl_band_freq, bands[g_band].label);
    if (lbl_save_btn) {
        char sbuf[14];
        snprintf(sbuf, sizeof(sbuf), "SAVE (%d)", bands[g_band].slot);
        lv_label_set_text(lbl_save_btn, sbuf);
    }
}

static void gui_update_lcsw_config(void) {
    if (!lbl_l_val) return;
    char buf[24];
    uint16_t uh100 = ind_to_uh100(g_pwr.ind);
    uint16_t pf    = cap_to_pf(g_pwr.cap);
    snprintf(buf, sizeof(buf), "%d.%02d uH", uh100/100, uh100%100);
    lv_label_set_text(lbl_l_val, buf);
    snprintf(buf, sizeof(buf), "%d pF", pf);
    lv_label_set_text(lbl_c_val, buf);
    if (slider_l_direct) {
        int spos = (g_pwr.ind + 1) / 4;
        lv_slider_set_value(slider_l_direct, spos, LV_ANIM_OFF);
    }
    if (slider_c_direct) {
        int spos = (g_pwr.cap + 1) / 4;
        lv_slider_set_value(slider_c_direct, spos, LV_ANIM_OFF);
    }
    if (btn_sw && lbl_sw_state) {
        if (g_pwr.sw) {
            lv_label_set_text(lbl_sw_state, "LC Filter");
            lv_obj_set_style_bg_color(btn_sw, lv_color_hex(0x0A4A8A), 0);
            lv_obj_set_style_bg_color(btn_sw, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
        } else {
            lv_label_set_text(lbl_sw_state, "CL Filter");
            lv_obj_set_style_bg_color(btn_sw, lv_color_hex(0x0A4A8A), 0);
            lv_obj_set_style_bg_color(btn_sw, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
        }
    }
}

static void gui_update_tune_btn(void) {
    if (!lbl_tune_btn) return;
    uint8_t s = g_tune_status < 16 ? g_tune_status : 0;
    lv_label_set_text(lbl_tune_btn, tune_labels[s]);
    // boja: idle=plava, tuning=zelena, ok/done=tamnozelena, abort/skip=crvena
    uint32_t col;
    if      (s == 0)              col = 0x0A4A8A;  // idle - plava
    else if (s == 12 || s == 13)  col = 0x1A5A1A;  // OK/DONE - tamnozelena
    else if (s == 14 || s == 15)  col = 0x5A1A1A;  // ABORT/SKIP - crvena
    else                          col = 0x6A5A00;  // u toku - tamnonarandzasta
    lv_obj_t *btn = lv_obj_get_parent(lbl_tune_btn);
    lv_obj_set_style_bg_color(btn, lv_color_hex(col), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(col), LV_STATE_PRESSED);
}

static void gui_update_bat(void) {
    if (!lbl_bat) return;
    float v = read_bat_voltage();
    char buf[10];
    snprintf(buf, sizeof(buf), "%.2fV", v);
    lv_label_set_text(lbl_bat, buf);
    lv_color_t c = (v > 3.8f) ? lv_color_hex(0x00CC44) :
                   (v > 3.5f) ? lv_color_hex(0xFFAA00) :
                                 lv_color_hex(0xFF2200);
    lv_obj_set_style_text_color(lbl_bat, c, 0);
}

// ============================================================
// CAL EDIT helpers
// ============================================================
static int cedit_get_current(uint8_t idx) {
    if (!g_cal_valid) return cal_params[idx].min_val;
    switch (idx) {
        case 0: return g_cal.k_mult;
        case 1: return g_cal.min_pwr;
        case 2: return g_cal.div;
        case 3: return g_cal.coarse_step;
        case 4: return g_cal.relay_ms;
        case 5: return g_cal.retry_wait * 10;  // W: prikazujemo ms (ATU jedinica * 10)
        case 6: return g_cal.debug;
        case 7: return g_cal.adc_swap;
        default: return 0;
    }
}

static void cedit_format_val(char *buf, int bufsz, uint8_t idx, int val) {
    if (idx == 2) {  // DIVIDER: prikazi kao x.xx
        snprintf(buf, bufsz, "= %d.%02d", val / 100, val % 100);
    } else if (idx == 5) {  // WAIT: val je ms direktno
        snprintf(buf, bufsz, "= %d ms", val);
    } else if (cal_params[idx].is_bool) {
        snprintf(buf, bufsz, "%s", val ? "ON" : "OFF");
    } else {
        snprintf(buf, bufsz, "= %d", val);
    }
}

static void gui_update_cal_edit(void) {
    if (!lbl_cedit_name) return;
    const CalParam &p = cal_params[g_cedit_idx];

    // Highlighted param dugme
    for (int i = 0; i < CAL_PARAM_COUNT; i++) {
        if (!param_btns[i]) continue;
        bool sel = (i == (int)g_cedit_idx);
        lv_obj_set_style_bg_color(param_btns[i], lv_color_hex(sel ? 0x0033AA : 0x111111), 0);
        lv_obj_set_style_bg_color(param_btns[i], lv_color_hex(sel ? 0x0033AA : 0x111111), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(param_btns[i], lv_color_hex(0x44AAFF), 0);
        lv_obj_set_style_border_width(param_btns[i], sel ? 2 : 0, 0);
    }

    lv_label_set_text(lbl_cedit_name, p.desc);

    // Prikazi slider ili toggle red
    if (p.is_bool) {
        if (lbl_cedit_val) lv_obj_add_flag(lbl_cedit_val, LV_OBJ_FLAG_HIDDEN);
        if (cedit_slider_row) lv_obj_add_flag(cedit_slider_row, LV_OBJ_FLAG_HIDDEN);
        if (cedit_toggle_row) lv_obj_clear_flag(cedit_toggle_row, LV_OBJ_FLAG_HIDDEN);
        if (lbl_cedit_toggle) {
            lv_label_set_text(lbl_cedit_toggle, g_cedit_val ? "ON" : "OFF");
            lv_obj_t *tbtn = lv_obj_get_parent(lbl_cedit_toggle);
            lv_obj_set_style_bg_color(tbtn, lv_color_hex(0x0A4A8A), 0);
            lv_obj_set_style_bg_color(tbtn, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
        }
    } else {
        if (lbl_cedit_val) {
            lv_obj_clear_flag(lbl_cedit_val, LV_OBJ_FLAG_HIDDEN);
            char vbuf[32];
            cedit_format_val(vbuf, sizeof(vbuf), g_cedit_idx, g_cedit_val);
            lv_label_set_text(lbl_cedit_val, vbuf);
        }
        if (cedit_slider_row) lv_obj_clear_flag(cedit_slider_row, LV_OBJ_FLAG_HIDDEN);
        if (cedit_toggle_row) lv_obj_add_flag(cedit_toggle_row, LV_OBJ_FLAG_HIDDEN);
        if (slider_cedit) {
            lv_slider_set_range(slider_cedit, p.min_val, p.max_val);
            lv_slider_set_value(slider_cedit, g_cedit_val, LV_ANIM_OFF);
        }
    }

    // SEND dugme label
    if (lbl_cedit_send) {
        char sbuf[20];
        int send_val = (g_cedit_idx == 5) ? g_cedit_val / 10 : g_cedit_val;
        snprintf(sbuf, sizeof(sbuf), "SEND  %c%d", p.cmd, send_val);
        lv_label_set_text(lbl_cedit_send, sbuf);
    }
}

static void gui_update_pair_btn(void) {
    if (!lbl_unpair_btn) return;
    lv_obj_t *btn = lv_obj_get_parent(lbl_unpair_btn);
    if (is_paired) {
        char buf[64];
        snprintf(buf, sizeof(buf), "PAIRED  MAC:%02X:%02X:%02X:%02X:%02X:%02X\npress&hold 2sec for unpair",
                 paired_mac[0], paired_mac[1], paired_mac[2],
                 paired_mac[3], paired_mac[4], paired_mac[5]);
        lv_label_set_text(lbl_unpair_btn, buf);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x004400), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x006600), LV_STATE_PRESSED);
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
    } else {
        lv_label_set_text(lbl_unpair_btn, "NOT PAIRED");
        lv_obj_set_style_text_color(lbl_unpair_btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x550000), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x550000), LV_STATE_PRESSED);
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
    }
}

static void gui_update_cal(void) {
    // CAL raw prikaz uklonjen; podaci dostupni samo u screen4 CAL EDIT
}
