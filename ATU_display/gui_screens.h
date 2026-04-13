#pragma once

// ============================================================
// Kreiranje Main screena
// ============================================================
static void create_main_screen(void) {
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x000000), 0);

    // --- Status bar (y=0, h=22) ---
    lv_obj_t *sb = lv_obj_create(scr_main);
    lv_obj_set_size(sb, 320, 22);
    lv_obj_set_pos(sb, 0, 0);
    lv_obj_set_style_bg_color(sb, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(sb, 0, 0);
    lv_obj_set_style_pad_all(sb, 0, 0);
    lv_obj_clear_flag(sb, LV_OBJ_FLAG_SCROLLABLE);

    lbl_status = lv_label_create(sb);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_status, LV_ALIGN_LEFT_MID, 4, 0);
    lv_label_set_text(lbl_status, LV_SYMBOL_WIFI " ATU - waiting...");
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x888888), 0);

    lbl_ovr = lv_label_create(sb);
    lv_obj_set_style_text_font(lbl_ovr, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_ovr, lv_color_hex(0xFF2200), 0);
    lv_obj_align(lbl_ovr, LV_ALIGN_RIGHT_MID, -78, 0);
    lv_label_set_text(lbl_ovr, "");

    lbl_bat = lv_label_create(sb);
    lv_obj_set_style_text_font(lbl_bat, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_bat, LV_ALIGN_RIGHT_MID, -120, 0);
    lv_label_set_text(lbl_bat, "");

    // CONFIG dugme – gore desno u status baru
    lv_obj_t *btn_cfg_top = make_btn(sb, "CONFIG", 246, 0, 74, 22, 0x2A2A2A, cb_config);
    lv_obj_set_style_text_font(lv_obj_get_child(btn_cfg_top, 0), &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lv_obj_get_child(btn_cfg_top, 0), lv_color_hex(0x000000), 0);

    make_sep(scr_main, 22);

    // --- PWR panel (x=0, y=23, w=152, h=110) ---
    lv_obj_t *pp = lv_obj_create(scr_main);
    lv_obj_set_size(pp, 152, 110);
    lv_obj_set_pos(pp, 0, 23);
    lv_obj_set_style_bg_color(pp, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(pp, 0, 0);
    lv_obj_set_style_pad_all(pp, 0, 0);
    lv_obj_clear_flag(pp, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t1 = lv_label_create(pp);
    lv_label_set_text(t1, "PWR");
    lv_obj_set_style_text_font(t1, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(t1, lv_color_hex(0x666688), 0);
    lv_obj_align(t1, LV_ALIGN_TOP_MID, 0, 4);

    lbl_pwr_val = lv_label_create(pp);
    lv_label_set_text(lbl_pwr_val, "--");
    lv_obj_set_style_text_font(lbl_pwr_val, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_pwr_val, lv_color_hex(0x444444), 0);
    lv_obj_align(lbl_pwr_val, LV_ALIGN_CENTER, 0, 0);

    bar_pwr = lv_bar_create(pp);
    lv_obj_set_size(bar_pwr, 130, 8);
    lv_obj_align(bar_pwr, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_bar_set_range(bar_pwr, 0, 100);
    lv_bar_set_value(bar_pwr, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_pwr, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_pwr, lv_color_hex(0x00CC44), LV_PART_INDICATOR);

    // Vertikalni separator
    lv_obj_t *vs = lv_obj_create(scr_main);
    lv_obj_set_size(vs, 1, 110);
    lv_obj_set_pos(vs, 152, 23);
    lv_obj_set_style_bg_color(vs, lv_color_hex(0x222244), 0);
    lv_obj_set_style_border_width(vs, 0, 0);

    // --- SWR panel (x=153, y=23, w=167, h=110) ---
    lv_obj_t *sp = lv_obj_create(scr_main);
    lv_obj_set_size(sp, 167, 110);
    lv_obj_set_pos(sp, 153, 23);
    lv_obj_set_style_bg_color(sp, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(sp, 0, 0);
    lv_obj_set_style_pad_all(sp, 0, 0);
    lv_obj_clear_flag(sp, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t2 = lv_label_create(sp);
    lv_label_set_text(t2, "SWR");
    lv_obj_set_style_text_font(t2, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(t2, lv_color_hex(0x666688), 0);
    lv_obj_align(t2, LV_ALIGN_TOP_MID, 0, 4);

    lbl_swr_val = lv_label_create(sp);
    lv_label_set_text(lbl_swr_val, "---");
    lv_obj_set_style_text_font(lbl_swr_val, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_swr_val, lv_color_hex(0x222222), 0);
    lv_obj_align(lbl_swr_val, LV_ALIGN_CENTER, 0, -2);

    bar_swr = lv_bar_create(sp);
    lv_obj_set_size(bar_swr, 145, 8);
    lv_obj_align(bar_swr, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_bar_set_range(bar_swr, 0, 100);
    lv_bar_set_value(bar_swr, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_swr, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_swr, lv_color_hex(0x222222), LV_PART_INDICATOR);

    make_sep(scr_main, 133);

    // --- Relay state (y=135, h=24) ---
    lbl_relay = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_relay, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_relay, lv_color_hex(0x444444), 0);
    lv_obj_set_pos(lbl_relay, 6, 136);
    lv_label_set_text(lbl_relay, "L: ---   C: ---   SW: ---");

    make_sep(scr_main, 160);

    // --- Band display (y=161, h=36): [<<] xxMHz centriran [>>] ---
    lv_obj_t *b_minus = make_btn(scr_main, "<<", 0, 163, 74, 32, 0x1A3A1A, cb_band_minus);
    lv_obj_set_style_text_font(lv_obj_get_child(b_minus, 0), &lv_font_montserrat_sb_24, 0);

    lbl_band_freq = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_band_freq, &lv_font_montserrat_sb_28, 0);
    lv_obj_set_style_text_color(lbl_band_freq, lv_color_hex(0x44AAFF), 0);
    lv_obj_set_pos(lbl_band_freq, 74, 162);
    lv_obj_set_width(lbl_band_freq, 172);
    lv_obj_set_style_text_align(lbl_band_freq, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_band_freq, bands[g_band].label);

    lv_obj_t *b_plus = make_btn(scr_main, ">>", 246, 163, 74, 32, 0x1A3A1A, cb_band_plus);
    lv_obj_set_style_text_font(lv_obj_get_child(b_plus, 0), &lv_font_montserrat_sb_24, 0);

    make_sep(scr_main, 198);

    // --- Controls (y=199, h=41): [TUNE lijevo] [SAVE desno] ---
    lv_obj_t *btn_tune = lv_button_create(scr_main);
    lv_obj_set_size(btn_tune, 194, 35);
    lv_obj_set_pos(btn_tune, 0, 201);
    lv_obj_set_style_bg_color(btn_tune, lv_color_hex(0x0A4A8A), 0);
    lv_obj_set_style_bg_color(btn_tune, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_tune, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_tune, 6, 0);
    lv_obj_add_event_cb(btn_tune, cb_tune, LV_EVENT_PRESSED,    NULL);
    lv_obj_add_event_cb(btn_tune, cb_tune, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(btn_tune, cb_tune, LV_EVENT_PRESS_LOST, NULL);
    lbl_tune_btn = lv_label_create(btn_tune);
    lv_label_set_text(lbl_tune_btn, "TUNE");
    lv_obj_set_style_text_font(lbl_tune_btn, &lv_font_montserrat_sb_20, 0);
    lv_obj_set_style_text_color(lbl_tune_btn, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_tune_btn);

    // SAVE dugme - dinamicki slot (lbl_save_btn), prebaceno sa screen2
    lv_obj_t *btn_save = lv_button_create(scr_main);
    lv_obj_set_size(btn_save, 122, 35);
    lv_obj_set_pos(btn_save, 198, 201);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x0A4A8A), 0);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_save, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_save, 6, 0);
    lv_obj_add_event_cb(btn_save, cb_save, LV_EVENT_PRESSED,    NULL);
    lv_obj_add_event_cb(btn_save, cb_save, LV_EVENT_RELEASED,   NULL);
    lv_obj_add_event_cb(btn_save, cb_save, LV_EVENT_PRESS_LOST, NULL);
    lbl_save_btn = lv_label_create(btn_save);
    char sbuf[14];
    snprintf(sbuf, sizeof(sbuf), "SAVE (%d)", bands[g_band].slot);
    lv_label_set_text(lbl_save_btn, sbuf);
    lv_obj_set_style_text_font(lbl_save_btn, &lv_font_montserrat_sb_20, 0);
    lv_obj_set_style_text_color(lbl_save_btn, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_save_btn);
}

// ============================================================
// Kreiranje Config screena (screen2)
// ============================================================
static void create_config_screen(void) {
    scr_config = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_config, lv_color_hex(0x000000), 0);

    // --- Header (y=0, h=30): [← Back] | [Next >] ---
    lv_obj_t *hdr = lv_obj_create(scr_config);
    lv_obj_set_size(hdr, 320, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    make_btn(hdr, LV_SYMBOL_LEFT " Back", 0, 0, 80, 30, 0x2A2A2A, cb_back);
    make_btn(hdr, "Next " LV_SYMBOL_RIGHT, 240, 0, 80, 30, 0x2A2A2A, cb_next);

    lbl_cfg_status2 = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_cfg_status2, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_cfg_status2, lv_color_hex(0x888888), 0);
    lv_obj_align(lbl_cfg_status2, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl_cfg_status2, LV_SYMBOL_WIFI " waiting...");

    make_sep(scr_config, 30);

    // --- L kontrola (y=31, h=42): [L-]  "0.00 uH"  [L+] ---
    make_btn(scr_config, "L-",  6, 33, 58, 36, 0x1A2A1A, cb_l_minus);
    make_btn(scr_config, "L+", 256, 33, 58, 36, 0x1A2A1A, cb_l_plus);

    lbl_l_val = lv_label_create(scr_config);
    lv_obj_set_style_text_font(lbl_l_val, &lv_font_montserrat_sb_28, 0);
    lv_obj_set_style_text_color(lbl_l_val, lv_color_hex(0x44DD88), 0);
    lv_obj_set_pos(lbl_l_val, 64, 35);
    lv_obj_set_width(lbl_l_val, 192);
    lv_obj_set_style_text_align(lbl_l_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_l_val, "0.00 uH");

    make_sep(scr_config, 73);

    // --- L slider (y=76): direktno postavljanje induktivnosti 0-127 ---
    slider_l_direct = lv_slider_create(scr_config);
    lv_obj_set_size(slider_l_direct, 268, 12);
    lv_obj_set_pos(slider_l_direct, 26, 82);
    lv_slider_set_range(slider_l_direct, 0, 32);
    lv_slider_set_value(slider_l_direct, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_l_direct, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_l_direct, lv_color_hex(0x225522), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_l_direct, lv_color_hex(0x88FF88), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_l_direct, cb_slider_l, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(slider_l_direct, cb_slider_l, LV_EVENT_RELEASED, NULL);

    make_sep(scr_config, 114);

    // --- C kontrola (y=117, h=42): [C-]  "0 pF"  [C+] ---
    make_btn(scr_config, "C-",  6, 117, 58, 36, 0x1A2A1A, cb_c_minus);
    make_btn(scr_config, "C+", 256, 117, 58, 36, 0x1A2A1A, cb_c_plus);

    lbl_c_val = lv_label_create(scr_config);
    lv_obj_set_style_text_font(lbl_c_val, &lv_font_montserrat_sb_28, 0);
    lv_obj_set_style_text_color(lbl_c_val, lv_color_hex(0x4488FF), 0);
    lv_obj_set_pos(lbl_c_val, 64, 119);
    lv_obj_set_width(lbl_c_val, 192);
    lv_obj_set_style_text_align(lbl_c_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_c_val, "0 pF");

    make_sep(scr_config, 157);

    // --- C slider (y=160): direktno postavljanje kondenzatora 0-127, Cap_sw = SW status ---
    slider_c_direct = lv_slider_create(scr_config);
    lv_obj_set_size(slider_c_direct, 268, 12);
    lv_obj_set_pos(slider_c_direct, 26, 163);
    lv_slider_set_range(slider_c_direct, 0, 32);
    lv_slider_set_value(slider_c_direct, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_c_direct, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_c_direct, lv_color_hex(0x1A2255), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_c_direct, lv_color_hex(0x88AAFF), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_c_direct, cb_slider_c, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(slider_c_direct, cb_slider_c, LV_EVENT_RELEASED, NULL);

    // --- Bottom row (dno ekrana, y=200, h=36): [CL/LC Filter] [Reset Relays] [Reboot ATU] ---
    btn_sw = lv_button_create(scr_config);
    lv_obj_set_size(btn_sw, 104, 36);
    lv_obj_set_pos(btn_sw, 0, 200);
    lv_obj_set_style_bg_color(btn_sw, lv_color_hex(0x0A4A8A), 0);
    lv_obj_set_style_bg_color(btn_sw, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_sw, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_sw, 6, 0);
    lv_obj_add_event_cb(btn_sw, cb_sw_toggle, LV_EVENT_CLICKED, NULL);
    lbl_sw_state = lv_label_create(btn_sw);
    lv_label_set_text(lbl_sw_state, "CL Filter");
    lv_obj_set_style_text_font(lbl_sw_state, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_sw_state, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_sw_state);

    lv_obj_t *btn_bypass = lv_button_create(scr_config);
    lv_obj_set_size(btn_bypass, 104, 36);
    lv_obj_set_pos(btn_bypass, 108, 200);
    lv_obj_set_style_bg_color(btn_bypass, lv_color_hex(0x0A4A8A), 0);
    lv_obj_set_style_bg_color(btn_bypass, lv_color_hex(0x0A4A8A), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_bypass, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_bypass, 6, 0);
    lv_obj_add_event_cb(btn_bypass, cb_bypass, LV_EVENT_CLICKED, NULL);
    lbl_bypass_btn = lv_label_create(btn_bypass);
    lv_label_set_text(lbl_bypass_btn, "bypass OFF");
    lv_obj_set_style_text_font(lbl_bypass_btn, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_bypass_btn, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_bypass_btn);
    make_btn(scr_config, "Reboot ATU",   216, 200, 104, 36, 0x0A4A8A, cb_reboot);

}

// ============================================================
// Kreiranje Brightness screena (screen3)
// ============================================================
static void create_bright_screen(void) {
    scr_bright = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_bright, lv_color_hex(0x000000), 0);

    // --- Header (y=0, h=30): [← Back] | "BRIGHTNESS" ---
    lv_obj_t *hdr = lv_obj_create(scr_bright);
    lv_obj_set_size(hdr, 320, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    make_btn(hdr, LV_SYMBOL_LEFT " Back", 0, 0, 80, 30, 0x2A2A2A, cb_back_bright);
    make_btn(hdr, "CAL " LV_SYMBOL_RIGHT, 240, 0, 80, 30, 0x2A2A2A, cb_next_to_cal_edit);

    // ATU status u headeru (centar)
    lbl_cfg_status = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_cfg_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_cfg_status, lv_color_hex(0x888888), 0);
    lv_obj_align(lbl_cfg_status, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl_cfg_status, LV_SYMBOL_WIFI " waiting...");

    make_sep(scr_bright, 30);

    // --- Brightness kontrola (y=36) ---
    lv_obj_t *lbl_brt = lv_label_create(scr_bright);
    lv_label_set_text(lbl_brt, "Brightness");
    lv_obj_set_style_text_font(lbl_brt, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_brt, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(lbl_brt, 0, 36);
    lv_obj_set_width(lbl_brt, 320);
    lv_obj_set_style_text_align(lbl_brt, LV_TEXT_ALIGN_CENTER, 0);

    slider_brt = lv_slider_create(scr_bright);
    lv_obj_set_size(slider_brt, 280, 12);
    lv_obj_set_pos(slider_brt, 20, 64);
    lv_slider_set_range(slider_brt, 20, 255);
    lv_slider_set_value(slider_brt, g_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_brt, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_brt, lv_color_hex(0x4488CC), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_brt, lv_color_hex(0xAADDFF), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_brt, cb_slider_brt, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(slider_brt, cb_slider_brt, LV_EVENT_RELEASED,      NULL);

    // --- Unpair dugme (dinamicki sadrzaj, press&hold 2s) ---
    make_sep(scr_bright, 100);
    lv_obj_t *btn_unpair = lv_button_create(scr_bright);
    lv_obj_set_size(btn_unpair, 300, 48);
    lv_obj_set_pos(btn_unpair, 10, 108);
    lv_obj_set_style_bg_opa(btn_unpair, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_unpair, 10, 0);
    lv_obj_set_style_border_width(btn_unpair, 0, 0);
    lv_obj_add_event_cb(btn_unpair, cb_unpair, LV_EVENT_PRESSED,      NULL);
    lv_obj_add_event_cb(btn_unpair, cb_unpair, LV_EVENT_RELEASED,     NULL);
    lv_obj_add_event_cb(btn_unpair, cb_unpair, LV_EVENT_PRESS_LOST,   NULL);
    lbl_unpair_btn = lv_label_create(btn_unpair);
    lv_obj_set_style_text_font(lbl_unpair_btn, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_unpair_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_align(lbl_unpair_btn, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl_unpair_btn);
    gui_update_pair_btn();  // postavi inicijalni sadrzaj i boju

    // --- Version label (dno ekrana) ---
    lv_obj_t *lbl_ver = lv_label_create(scr_bright);
    lv_label_set_text(lbl_ver, "SW " APP_VERSION);
    lv_obj_set_style_text_font(lbl_ver, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_ver, lv_color_hex(0x444444), 0);
    lv_obj_set_pos(lbl_ver, 0, 218);
    lv_obj_set_width(lbl_ver, 320);
    lv_obj_set_style_text_align(lbl_ver, LV_TEXT_ALIGN_CENTER, 0);
}

// ============================================================
// Kreiranje CAL Edit screena (screen4)
// ============================================================
static void create_cal_edit_screen(void) {
    scr_cal_edit = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_cal_edit, lv_color_hex(0x000000), 0);

    // --- Header (y=0, h=30): [← Back] | "CAL EDIT" ---
    lv_obj_t *hdr = lv_obj_create(scr_cal_edit);
    lv_obj_set_size(hdr, 320, 30);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    make_btn(hdr, LV_SYMBOL_LEFT " Back", 0, 0, 80, 30, 0x2A2A2A, cb_back_cal_edit);

    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_label_set_text(lbl_title, LV_SYMBOL_WARNING " SYSTEM SETTINGS");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xEE0000), 0);
    lv_obj_set_pos(lbl_title, 80, 7);
    lv_obj_set_width(lbl_title, 240);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);

    make_sep(scr_cal_edit, 30);

    // --- Param dugmad 2x4 (y=32 i y=60) ---
    const char *labels[CAL_PARAM_COUNT] = {"K","M","D","S","R","W","V","A"};
    for (int i = 0; i < CAL_PARAM_COUNT; i++) {
        int col = i % 4;
        int row = i / 4;
        int bx  = 2 + col * 79;
        int by  = 30 + row * 28;
        lv_obj_t *b = lv_button_create(scr_cal_edit);
        lv_obj_set_size(b, 75, 24);
        lv_obj_set_pos(b, bx, by);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x1A2A3A), 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x1A2A3A), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(b, 4, 0);
        lv_obj_set_style_border_width(b, 0, 0);
        lv_obj_add_event_cb(b, cb_cedit_param, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        lv_obj_t *lbl = lv_label_create(b);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_sb_16, 0);
        lv_obj_center(lbl);
        param_btns[i] = b;
    }

    make_sep(scr_cal_edit, 86);

    // --- Naziv parametra (y=89, 2 linije font_12) ---
    lbl_cedit_name = lv_label_create(scr_cal_edit);
    lv_obj_set_style_text_font(lbl_cedit_name, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_cedit_name, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(lbl_cedit_name, 4, 89);
    lv_obj_set_width(lbl_cedit_name, 312);
    lv_obj_set_style_text_align(lbl_cedit_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_cedit_name, "K_MULT calibration\ndefault=16   range=1-255");

    // --- Trenutna vrijednost (y=120, font_24) ---
    lbl_cedit_val = lv_label_create(scr_cal_edit);
    lv_obj_set_style_text_font(lbl_cedit_val, &lv_font_montserrat_sb_24, 0);
    lv_obj_set_style_text_color(lbl_cedit_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_cedit_val, 0, 120);
    lv_obj_set_width(lbl_cedit_val, 320);
    lv_obj_set_style_text_align(lbl_cedit_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_cedit_val, "= 16");

    // --- Slider red (y=152, sakrij za bool) ---
    cedit_slider_row = lv_obj_create(scr_cal_edit);
    lv_obj_set_size(cedit_slider_row, 320, 28);
    lv_obj_set_pos(cedit_slider_row, 0, 152);
    lv_obj_set_style_bg_opa(cedit_slider_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cedit_slider_row, 0, 0);
    lv_obj_set_style_pad_all(cedit_slider_row, 0, 0);
    lv_obj_clear_flag(cedit_slider_row, LV_OBJ_FLAG_SCROLLABLE);

    make_btn(cedit_slider_row, "-",  4,  4, 30, 26, 0x222222, cb_cedit_minus);
    make_btn(cedit_slider_row, "+", 286, 4, 30, 26, 0x222222, cb_cedit_plus);

    slider_cedit = lv_slider_create(cedit_slider_row);
    lv_obj_set_size(slider_cedit, 232, 12);
    lv_obj_set_pos(slider_cedit, 40, 11);
    lv_slider_set_range(slider_cedit, 1, 255);
    lv_slider_set_value(slider_cedit, 16, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider_cedit, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider_cedit, lv_color_hex(0x2255AA), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_cedit, lv_color_hex(0x88BBFF), LV_PART_KNOB);
    lv_obj_add_event_cb(slider_cedit, cb_cedit_slider, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Toggle red za bool (y=152, inicijalno skriveno) ---
    cedit_toggle_row = lv_obj_create(scr_cal_edit);
    lv_obj_set_size(cedit_toggle_row, 320, 28);
    lv_obj_set_pos(cedit_toggle_row, 0, 152);
    lv_obj_set_style_bg_opa(cedit_toggle_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cedit_toggle_row, 0, 0);
    lv_obj_set_style_pad_all(cedit_toggle_row, 0, 0);
    lv_obj_clear_flag(cedit_toggle_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(cedit_toggle_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *tbtn = lv_button_create(cedit_toggle_row);
    lv_obj_set_size(tbtn, 120, 26);
    lv_obj_align(tbtn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(tbtn, lv_color_hex(0x005500), 0);
    lv_obj_set_style_bg_color(tbtn, lv_color_hex(0x005500), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(tbtn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tbtn, 6, 0);
    lv_obj_set_style_border_width(tbtn, 0, 0);
    lv_obj_add_event_cb(tbtn, cb_cedit_toggle, LV_EVENT_CLICKED, NULL);
    lbl_cedit_toggle = lv_label_create(tbtn);
    lv_label_set_text(lbl_cedit_toggle, "ON");
    lv_obj_set_style_text_font(lbl_cedit_toggle, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_cedit_toggle, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_cedit_toggle);

    make_sep(scr_cal_edit, 183);

    // --- SEND dugme (y=200, h=36, zaobljeno) ---
    lv_obj_t *btn_send = lv_button_create(scr_cal_edit);
    lv_obj_set_size(btn_send, 304, 36);
    lv_obj_set_pos(btn_send, 8, 200);
    lv_obj_set_style_bg_color(btn_send, lv_color_hex(0x004400), 0);
    lv_obj_set_style_bg_color(btn_send, lv_color_hex(0x006600), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_send, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn_send, 10, 0);
    lv_obj_set_style_border_width(btn_send, 0, 0);
    lv_obj_add_event_cb(btn_send, cb_cedit_send, LV_EVENT_CLICKED, NULL);
    lbl_cedit_send = lv_label_create(btn_send);
    lv_label_set_text(lbl_cedit_send, "SEND  K16");
    lv_obj_set_style_text_font(lbl_cedit_send, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_cedit_send, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_cedit_send);

    // Inicijalni prikaz
    gui_update_cal_edit();
}

// ============================================================
// Pairing dialog (overlay)
// ============================================================
static void pair_dialog_close(void) {
    if (pair_dialog) {
        lv_obj_delete(pair_dialog);
        pair_dialog = NULL;
    }
}

static void cb_pair_confirm(lv_event_t*) {
    memcpy(paired_mac, pair_req_mac, 6);
    nvs_save_mac(paired_mac);
    add_unicast_peer(paired_mac);
    is_paired = true;
    send_ann_reply(paired_mac);
    pair_dialog_close();
    g_reset_cal_ms = millis();  // pošalji cal nakon 1s da osvježi display
    gui_update_pair_btn();
    Serial.printf("[PAIR] Uparen sa: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  paired_mac[0],paired_mac[1],paired_mac[2],
                  paired_mac[3],paired_mac[4],paired_mac[5]);
}

static void cb_pair_cancel(lv_event_t*) {
    pair_request = false;
    pair_dialog_close();
}

static void show_pair_dialog(const uint8_t *mac) {
    if (pair_dialog) return;

    // Nakon LV_DISPLAY_ROTATION_270 logicki ekran je 320x240
    lv_coord_t lw = lv_display_get_horizontal_resolution(NULL);
    lv_coord_t lh = lv_display_get_vertical_resolution(NULL);

    pair_dialog = lv_obj_create(lv_layer_top());
    lv_obj_set_size(pair_dialog, lw, lh);
    lv_obj_set_pos(pair_dialog, 0, 0);
    lv_obj_set_style_bg_color(pair_dialog, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(pair_dialog, LV_OPA_80, 0);
    lv_obj_set_style_border_width(pair_dialog, 0, 0);
    lv_obj_set_style_pad_all(pair_dialog, 0, 0);
    lv_obj_clear_flag(pair_dialog, LV_OBJ_FLAG_SCROLLABLE);

    // Veliki confirm taster sa crvenim okvirom
    lv_obj_t *btn = lv_button_create(pair_dialog);
    lv_obj_set_size(btn, 260, 90);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -22);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1A0000), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3A0000), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(btn, 3, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_add_event_cb(btn, cb_pair_confirm, LV_EVENT_CLICKED, NULL);

    char mac_str[48];
    snprintf(mac_str, sizeof(mac_str),
             "PAIR WITH\n%02X:%02X:%02X:%02X:%02X:%02X\n?",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, mac_str);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl);

    // Cancel taster ispod
    lv_obj_t *btn_cancel = lv_button_create(pair_dialog);
    lv_obj_set_size(btn_cancel, 110, 30);
    lv_obj_align(btn_cancel, LV_ALIGN_CENTER, 0, 62);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x222222), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn_cancel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_cancel, 0, 0);
    lv_obj_set_style_radius(btn_cancel, 6, 0);
    lv_obj_add_event_cb(btn_cancel, cb_pair_cancel, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_c = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_c, "CANCEL");
    lv_obj_set_style_text_font(lbl_c, &lv_font_montserrat_sb_16, 0);
    lv_obj_set_style_text_color(lbl_c, lv_color_hex(0x000000), 0);
    lv_obj_center(lbl_c);
}
