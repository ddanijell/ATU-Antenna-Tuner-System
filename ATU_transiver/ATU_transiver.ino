// ATU_transiver
// ESP32 Serial <-> ESP-NOW bridge
//
// Serial2 (GPIO16=RX, GPIO17=TX, 9600 baud) <-> ESP-NOW
// Serial  (UART0, 115200) -> debug + unos komandi
//
// ATU poruke pocinju sa '$', zavrsavaju sa '\n'
// ESP-NOW paket:
//   Byte 0: MAGIC (0xAA)
//   Byte 1: TYPE  (0x01=$PWR, 0x02=$CAL, 0x03=CMD, 0x04=$TUNE, 0x05=ANN)
//   Byte 2+: originalna poruka (ili 6-byte MAC za ANN)
//
// Pairing:
//   - Boot bez NVS MAC -> broadcast ANNOUNCE svake 2s
//   - Display reply (TYPE_ANN) -> sačuvaj MAC u NVS, unicast od tada
//   - GPIO0 držati >5s -> brisanje NVS MAC, reboot, nova announce faza

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Preferences.h>

// === KONFIGURACIJA ===
#define ATU_SERIAL          Serial2
#define ATU_BAUD            9600
#define ATU_RX_PIN          16
#define ATU_TX_PIN          17

#define DBG_SERIAL          Serial
#define DBG_BAUD            115200

#define MSG_BUF_SIZE        64      // max duzina jedne ATU poruke

#define GPIO_UNPAIR_PIN     0       // GPIO0 = BOOT button
#define UNPAIR_HOLD_MS      5000    // 5s drzanje za unpair

// === ESP-NOW ===
#define ESPNOW_MAGIC        0xAA
#define ESPNOW_TYPE_PWR     0x01    // $PWR:...
#define ESPNOW_TYPE_CAL     0x02    // $CAL:...
#define ESPNOW_TYPE_CMD     0x03    // komanda prema ATU
#define ESPNOW_TYPE_TUNE    0x04    // $TUNE:x
#define ESPNOW_TYPE_ANN     0x05    // Announce/pairing: payload = 6-byte MAC
#define ESPNOW_TYPE_PING    0x06    // Ping: Display -> Transiver (no payload)
#define ESPNOW_TYPE_PONG    0x07    // Pong: Transiver -> Display (no payload)
#define ESPNOW_TYPE_OTHER   0x00

// Broadcast MAC
static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// === PAIRING STATE ===
static uint8_t  paired_mac[6]  = {0};
static bool     is_paired       = false;

static Preferences prefs;
#define NVS_NS      "atapair"
#define NVS_KEY_MAC "mac"

// === ANNOUNCE TIMER ===
#define ESPNOW_CHANNEL        14
#define ANNOUNCE_INTERVAL_MS  2000
#define ANNOUNCE_TIMEOUT_MS   120000   // 2 minute max, pa prestaje
static uint32_t last_ann_ms   = 0;
static uint32_t ann_start_ms  = 0;     // kad je krenuo announce (0 = nije aktivan)
static bool     ann_active    = false;

// === ATU SERIAL RX BAFER ===
static char     atu_rx_buf[MSG_BUF_SIZE];
static uint8_t  atu_rx_len    = 0;
static bool     atu_rx_active = false;

// === DBG SERIAL RX BAFER ===
static char     dbg_rx_buf[MSG_BUF_SIZE];
static uint8_t  dbg_rx_len = 0;

// ============================================================
// NVS: load / save / clear
// ============================================================
static bool nvs_load_mac(uint8_t *mac) {
    prefs.begin(NVS_NS, true);  // read-only
    size_t len = prefs.getBytes(NVS_KEY_MAC, mac, 6);
    prefs.end();
    if (len != 6) return false;
    // provjeri da nije sve nule (ocisceno)
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

// ============================================================
// Dodaje unicast peer (display MAC)
// ============================================================
static void add_unicast_peer(const uint8_t *mac) {
    if (esp_now_is_peer_exist(mac)) return;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
}

// ============================================================
// Salje PONG unicast na display
// ============================================================
static void send_pong(const uint8_t *dest_mac) {
    uint8_t pkt[2] = { ESPNOW_MAGIC, ESPNOW_TYPE_PONG };
    esp_now_send(dest_mac, pkt, 2);
}

// ============================================================
// Salje ANNOUNCE broadcast (vlastiti MAC kao payload)
// ============================================================
static void send_announce(void) {
    uint8_t pkt[8];
    pkt[0] = ESPNOW_MAGIC;
    pkt[1] = ESPNOW_TYPE_ANN;
    // payload: vlastiti MAC (6 bajtova)
    uint8_t own_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, own_mac);
    memcpy(&pkt[2], own_mac, 6);

    esp_now_send(broadcast_mac, pkt, 8);
    DBG_SERIAL.println("[ANN] Announce broadcast sent, waiting for display...");
}

// ============================================================
// Odredjuje TYPE byte na osnovu sadrzaja ATU poruke
// ============================================================
static uint8_t detect_type(const char *msg) {
    if (msg[1] == 'P' && msg[2] == 'W' && msg[3] == 'R') return ESPNOW_TYPE_PWR;
    if (msg[1] == 'C' && msg[2] == 'A' && msg[3] == 'L') return ESPNOW_TYPE_CAL;
    if (msg[1] == 'T' && msg[2] == 'U' && msg[3] == 'N' && msg[4] == 'E') return ESPNOW_TYPE_TUNE;
    return ESPNOW_TYPE_OTHER;
}

// ============================================================
// ESP-NOW send callback
// ============================================================
static void on_data_sent(const uint8_t *mac, esp_now_send_status_t status) {
    // DBG_SERIAL.printf("[TX] %s\n", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ============================================================
// ESP-NOW receive callback
// ============================================================
static void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len < 2) return;
    if (data[0] != ESPNOW_MAGIC) return;

    uint8_t type = data[1];

    // --- Pairing reply od displaya ---
    if (type == ESPNOW_TYPE_ANN) {
        if (is_paired) return;  // vec uparen, ignoriši

        // izvor paketa je MAC displaya
        const uint8_t *src = info->src_addr;
        memcpy(paired_mac, src, 6);
        nvs_save_mac(paired_mac);
        add_unicast_peer(paired_mac);
        is_paired  = true;
        ann_active = false;  // zaustavi announce

        DBG_SERIAL.printf("[PAIR] Uparen sa display MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                          paired_mac[0], paired_mac[1], paired_mac[2],
                          paired_mac[3], paired_mac[4], paired_mac[5]);
        return;
    }

    // Dok nije uparen ne prihvataj nista osim ANN (vec obradjen iznad)
    if (!is_paired) return;

    // Uparen: prihvati samo od paired_mac
    if (memcmp(info->src_addr, paired_mac, 6) != 0) {
        DBG_SERIAL.printf("[WARN] Odbijen paket od nepoznatog MAC\n");
        return;
    }

    // PING: odmah vrati PONG, ne proslijedjuj ATU
    if (type == ESPNOW_TYPE_PING) {
        send_pong(paired_mac);
        return;
    }

    if (len < 3) return;

    const uint8_t *msg     = &data[2];
    int            msg_len = len - 2;

    // Intercept "unpair\r" - ne proslijedjuj ATU, ocisti NVS i restartuj
    if (type == ESPNOW_TYPE_CMD &&
        msg_len >= 6 &&
        msg[0]=='u' && msg[1]=='n' && msg[2]=='p' &&
        msg[3]=='a' && msg[4]=='i' && msg[5]=='r') {
        DBG_SERIAL.println("[UNPAIR] Primljena unpair komanda, brisem NVS i restartujem...");
        nvs_clear_mac();
        delay(200);
        ESP.restart();
        return;
    }

    // Ispis na DBG
    if (type == ESPNOW_TYPE_CMD) {
        DBG_SERIAL.printf("[RX CMD] ");
    } else {
        DBG_SERIAL.printf("[RX ATU] ");
    }
    DBG_SERIAL.write(msg, msg_len);
    if (msg[msg_len - 1] != '\n') DBG_SERIAL.println();

    // Proslijedi na ATU serial port
    ATU_SERIAL.write(msg, msg_len);
}

// ============================================================
// Inicijalizacija ESP-NOW
// ============================================================
static void espnow_init(void) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Postavi fiksni kanal
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    DBG_SERIAL.printf("[OK] WiFi kanal: %d\n", ESPNOW_CHANNEL);

    if (esp_now_init() != ESP_OK) {
        DBG_SERIAL.println("[ERR] ESP-NOW init failed");
        return;
    }

    esp_now_register_send_cb(on_data_sent);
    esp_now_register_recv_cb(on_data_recv);

    // Broadcast peer uvijek prisutan (za announce i fallback)
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcast_mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    DBG_SERIAL.printf("[OK] ESP-NOW init, MAC: %s\n", WiFi.macAddress().c_str());

    // Učitaj pariran MAC iz NVS
    if (nvs_load_mac(paired_mac)) {
        add_unicast_peer(paired_mac);
        is_paired = true;
        DBG_SERIAL.printf("[PAIR] Ucitan MAC iz NVS: %02X:%02X:%02X:%02X:%02X:%02X\n",
                          paired_mac[0], paired_mac[1], paired_mac[2],
                          paired_mac[3], paired_mac[4], paired_mac[5]);
    } else {
        DBG_SERIAL.println("[PAIR] Nema sparenog uredjaja, startuje announce...");
        ann_active   = true;
        ann_start_ms = millis();
        last_ann_ms  = 0;
    }
}

// ============================================================
// Salje paket via ESP-NOW
// Ako je uparen -> unicast na paired_mac; inace -> broadcast
// ============================================================
static void espnow_send(const char *msg, uint8_t len, uint8_t type) {
    uint8_t pkt[MSG_BUF_SIZE + 2];
    pkt[0] = ESPNOW_MAGIC;
    pkt[1] = type;
    memcpy(&pkt[2], msg, len);

    DBG_SERIAL.printf("[TX %s] ", type == ESPNOW_TYPE_CMD ? "CMD" : "ATU");
    DBG_SERIAL.write((const uint8_t *)msg, len);
    if (msg[len - 1] != '\n') DBG_SERIAL.println();

    const uint8_t *dest = is_paired ? paired_mac : broadcast_mac;
    esp_now_send(dest, pkt, len + 2);
}

// ============================================================
// Obrada jednog bajta sa ATU_SERIAL
// ============================================================
static void process_atu_byte(char c) {
    if (!atu_rx_active) {
        if (c == '$') {
            atu_rx_buf[0] = '$';
            atu_rx_len    = 1;
            atu_rx_active = true;
        }
        return;
    }

    if (atu_rx_len < (MSG_BUF_SIZE - 1)) {
        atu_rx_buf[atu_rx_len++] = c;
    } else {
        DBG_SERIAL.println("[WARN] ATU RX overflow, reset");
        atu_rx_len    = 0;
        atu_rx_active = false;
        return;
    }

    if (c == '\n') {
        espnow_send(atu_rx_buf, atu_rx_len, detect_type(atu_rx_buf));
        atu_rx_len    = 0;
        atu_rx_active = false;
    }
}

// ============================================================
// Obrada jednog bajta sa DBG_SERIAL
// ============================================================
static void process_dbg_byte(char c) {
    if (c == '\r' || c == '\n') {
        if (dbg_rx_len == 0) return;
        if (dbg_rx_buf[dbg_rx_len - 1] != '\r') {
            dbg_rx_buf[dbg_rx_len++] = '\r';
        }
        espnow_send(dbg_rx_buf, dbg_rx_len, ESPNOW_TYPE_CMD);
        dbg_rx_len = 0;
        return;
    }

    if (dbg_rx_len < (MSG_BUF_SIZE - 1)) {
        dbg_rx_buf[dbg_rx_len++] = c;
    } else {
        DBG_SERIAL.println("[WARN] DBG RX overflow, reset");
        dbg_rx_len = 0;
    }
}

// ============================================================
void setup() {
    DBG_SERIAL.begin(DBG_BAUD);
    DBG_SERIAL.println("\n[ATU_transiver] boot");
    DBG_SERIAL.println("[INFO] GPIO0 >5s = unpair + reboot");

    pinMode(GPIO_UNPAIR_PIN, INPUT_PULLUP);

    ATU_SERIAL.begin(ATU_BAUD, SERIAL_8N1, ATU_RX_PIN, ATU_TX_PIN);
    DBG_SERIAL.printf("[OK] ATU Serial2 %d baud (RX=%d TX=%d)\n",
                      ATU_BAUD, ATU_RX_PIN, ATU_TX_PIN);

    espnow_init();
}

// ============================================================
void loop() {
    uint32_t now = millis();

    // --- GPIO0 unpair: drzanje >5s -> brisanje NVS + reboot ---
    static uint32_t btn_press_ms = 0;
    static bool     btn_armed    = false;
    if (digitalRead(GPIO_UNPAIR_PIN) == LOW) {
        if (!btn_armed) {
            btn_armed    = true;
            btn_press_ms = now;
        } else if ((now - btn_press_ms) >= UNPAIR_HOLD_MS) {
            DBG_SERIAL.println("[UNPAIR] GPIO0 drzano 5s, brisem NVS i restartujem...");
            nvs_clear_mac();
            delay(200);
            ESP.restart();
        }
    } else {
        btn_armed = false;
    }

    // --- Announce timer: ako nije uparen, svakih 2s, max 2 minute ---
    if (ann_active && !is_paired) {
        if ((now - ann_start_ms) >= ANNOUNCE_TIMEOUT_MS) {
            ann_active = false;
            DBG_SERIAL.println("[ANN] Timeout 2min, announce zaustavljen.");
        } else if ((now - last_ann_ms) >= ANNOUNCE_INTERVAL_MS) {
            last_ann_ms = now;
            send_announce();
        }
    }

    // ATU serial -> ESP-NOW
    while (ATU_SERIAL.available()) {
        process_atu_byte((char)ATU_SERIAL.read());
    }

    // DBG serial -> ESP-NOW
    while (DBG_SERIAL.available()) {
        process_dbg_byte((char)DBG_SERIAL.read());
    }
}
