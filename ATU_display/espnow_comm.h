#pragma once

// ============================================================
// NVS pairing helpers
// ============================================================
static bool nvs_load_mac(uint8_t *mac) {
    prefs.begin(NVS_NS, true);
    size_t len = prefs.getBytes(NVS_KEY_MAC, mac, 6);
    prefs.end();
    if (len != 6) return false;
    for (int i = 0; i < 6; i++) if (mac[i] != 0x00) return true;
    return false;
}
static void nvs_save_mac(const uint8_t *mac) {
    prefs.begin(NVS_NS, false);
    prefs.putBytes(NVS_KEY_MAC, mac, 6);
    prefs.end();
}
static void nvs_clear_mac(void) {
    prefs.begin(NVS_NS, false);
    prefs.remove(NVS_KEY_MAC);
    prefs.end();
}
static void nvs_save_brightness(uint8_t v) {
    prefs.begin(NVS_NS, false);
    prefs.putUChar("brightness", v);
    prefs.end();
}
static uint8_t nvs_load_brightness(uint8_t def) {
    prefs.begin(NVS_NS, true);
    uint8_t v = prefs.getUChar("brightness", def);
    prefs.end();
    return v;
}

// ============================================================
// ESP-NOW send/recv
// ============================================================
static void add_unicast_peer(const uint8_t *mac) {
    if (esp_now_is_peer_exist(mac)) return;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
}

static void send_cmd(const char *cmd) {
    if (!is_paired) return;  // blokira slanje dok nije uparen
    uint8_t pkt[32];
    uint8_t len = strlen(cmd);
    pkt[0]=ESPNOW_MAGIC; pkt[1]=ESPNOW_TYPE_CMD;
    memcpy(&pkt[2], cmd, len);
    esp_now_send(paired_mac, pkt, len+2);
    Serial.printf("[TX] %s", cmd);
}

static void send_ann_reply(const uint8_t *dest_mac) {
    uint8_t pkt[8];
    pkt[0] = ESPNOW_MAGIC;
    pkt[1] = ESPNOW_TYPE_ANN;
    uint8_t own_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, own_mac);
    memcpy(&pkt[2], own_mac, 6);
    esp_now_send(dest_mac, pkt, 8);
}

static void on_data_sent(const uint8_t*, esp_now_send_status_t) {}

static void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len < 2 || data[0] != ESPNOW_MAGIC) return;

    uint8_t type = data[1];

    // --- Pairing announce od transivera ---
    if (type == ESPNOW_TYPE_ANN) {
        if (!is_paired && !pair_request) {
            memcpy(pair_req_mac, info->src_addr, 6);
            pair_request = true;
        }
        return;
    }

    // Dok nije uparen ne prihvataj nista osim ANN (vec obradjen iznad)
    if (!is_paired) return;

    // Uparen: prihvati samo od paired_mac
    if (memcmp(info->src_addr, paired_mac, 6) != 0) return;

    if (len < 3) return;
    last_rx_ms = millis();
    if (info->rx_ctrl) g_rssi = (int8_t)info->rx_ctrl->rssi;
    int mlen = len-2; if(mlen>62) mlen=62;
    if (type == ESPNOW_TYPE_PWR) {
        memcpy(pwr_buf, &data[2], mlen); pwr_buf[mlen]='\0'; new_pwr=true;
    } else if (type == ESPNOW_TYPE_CAL) {
        memcpy(cal_buf, &data[2], mlen); cal_buf[mlen]='\0'; new_cal=true;
    } else if (type == ESPNOW_TYPE_TUNE) {
        int tlen = mlen < 11 ? mlen : 11;
        memcpy(tune_buf, &data[2], tlen); tune_buf[tlen]='\0'; new_tune=true;
    }
}

static void espnow_init(void) {
    WiFi.mode(WIFI_STA); WiFi.disconnect();
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init()!=ESP_OK) { Serial.println("[ERR] ESP-NOW"); return; }
    esp_now_register_send_cb(on_data_sent);
    esp_now_register_recv_cb(on_data_recv);
    esp_now_peer_info_t peer={};
    memcpy(peer.peer_addr, broadcast_mac, 6);
    peer.channel=ESPNOW_CHANNEL; peer.encrypt=false;
    esp_now_add_peer(&peer);
    Serial.printf("[OK] ESP-NOW MAC: %s, channel: %d\n", WiFi.macAddress().c_str(), ESPNOW_CHANNEL);

    if (nvs_load_mac(paired_mac)) {
        add_unicast_peer(paired_mac);
        is_paired = true;
        Serial.printf("[PAIR] Paired with: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      paired_mac[0],paired_mac[1],paired_mac[2],
                      paired_mac[3],paired_mac[4],paired_mac[5]);
    } else {
        Serial.println("[PAIR] No paired device.");
    }
}
