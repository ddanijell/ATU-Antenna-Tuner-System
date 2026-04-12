#pragma once
// ota_update.h
// OTA firmware update iz SD kartice
//
// SD kartica: VSPI  (GPIO 23/19/18/5)
// TFT display: HSPI (GPIO 13/12/14/15) - nezavisan od SD, nema konflikta
//
// Tok:
//   check_and_ota() -> TFT init -> discovery ekran (2s) -> progress ekran
//   do_ota_from_sd() -> TFT progress bar + Serial -> success/error ekran
//   Uspjeh: firmware.bin -> firmware.bak, ESP.restart()
//   Greska: poruka na ekranu 5s, nastavlja normalan boot

#include <SD.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

// --- Pomocne funkcije za crtanje ---

static void ota_text(TFT_eSPI &tft, int y, uint16_t color, uint8_t sz, const char *txt) {
    tft.setTextFont(1);
    tft.setTextSize(sz);
    tft.setTextColor(color, TFT_BLACK);
    tft.drawCentreString(txt, tft.width() / 2, y, 1);
}

static void ota_progress(TFT_eSPI &tft, int pct) {
    tft.fillRect(12, 102, (296 * pct) / 100, 26, 0x07E0);
    char pbuf[8];
    snprintf(pbuf, sizeof(pbuf), "%3d%%", pct);
    ota_text(tft, 142, TFT_WHITE, 2, pbuf);
}

// --- OTA write ---
// Vraca opis greske ili NULL i restartuje na uspjehu

static const char* do_ota_from_sd(TFT_eSPI &tft) {
    File f = SD.open("/firmware.bin", FILE_READ);
    if (!f) return "Cannot open firmware.bin";

    size_t fileSize = f.size();
    if (fileSize == 0) { f.close(); return "firmware.bin is empty"; }

    const esp_partition_t *upd = esp_ota_get_next_update_partition(NULL);
    if (!upd) { f.close(); return "No OTA partition found"; }

    esp_ota_handle_t hdl = 0;
    if (esp_ota_begin(upd, fileSize, &hdl) != ESP_OK) {
        f.close(); return "esp_ota_begin failed";
    }

    // --- Progress ekran ---
    tft.fillScreen(TFT_BLACK);
    ota_text(tft, 12,  TFT_WHITE, 2, "UPDATING FIRMWARE");
    char sbuf[40];
    snprintf(sbuf, sizeof(sbuf), "Size: %u KB", (unsigned)(fileSize / 1024));
    ota_text(tft, 52,  TFT_CYAN,  1, sbuf);
    tft.drawRect(10, 100, 300, 30, TFT_WHITE);   // progress bar outline
    ota_text(tft, 185, 0xFFE0,    1, "Do not power off!");
    ota_progress(tft, 0);

    Serial.printf("[OTA] Size: %u KB\n", (unsigned)(fileSize / 1024));

    static uint8_t wbuf[4096];
    size_t written = 0;
    int last_pct = -1;
    bool err_flag = false;

    while (written < fileSize) {
        size_t toRead = fileSize - written;
        if (toRead > sizeof(wbuf)) toRead = sizeof(wbuf);
        size_t rd = f.read(wbuf, toRead);
        if (rd == 0) { err_flag = true; break; }
        if (esp_ota_write(hdl, wbuf, rd) != ESP_OK) { err_flag = true; break; }
        written += rd;
        int pct = (int)((written * 100UL) / fileSize);
        if (pct != last_pct) {
            last_pct = pct;
            ota_progress(tft, pct);
            Serial.printf("[OTA] %d%%\n", pct);
        }
    }
    f.close();

    if (err_flag || esp_ota_end(hdl) != ESP_OK) return "Write error";
    if (esp_ota_set_boot_partition(upd) != ESP_OK) return "Set boot partition failed";

    // --- Success ekran ---
    tft.fillScreen(TFT_BLACK);
    ota_text(tft, 80,  0x07E0,    2, "UPDATE COMPLETE!");
    ota_text(tft, 125, TFT_WHITE, 1, "Restarting into new firmware...");
    Serial.println("[OTA] Complete! Restarting...");

    SD.remove("/firmware.bak");
    SD.rename("/firmware.bin", "/firmware.bak");
    delay(2000);
    ESP.restart();
    return NULL;
}

// --- Entry point ---

static void check_and_ota(void) {
    if (!SD.exists("/firmware.bin")) return;

    Serial.println("[OTA] firmware.bin found - starting upgrade...");

    // TFT init (HSPI) - ne dira SD (VSPI), nema konflikta
    TFT_eSPI tft;
    tft.init();
    tft.setRotation(3);   // landscape 320x240 (180° od rotation 1)
    tft.fillScreen(TFT_BLACK);
    bl_init();
    bl_set(200);

    // --- Discovery ekran ---
    ota_text(tft, 60,  0xFFE0,    2, "FIRMWARE UPDATE");
    ota_text(tft, 110, TFT_WHITE, 1, "firmware.bin found on SD card");
    ota_text(tft, 135, TFT_CYAN,  1, "Starting update...");
    Serial.println("[OTA] Discovery screen, waiting 2s...");
    delay(2000);

    // --- OTA write + progress ekran ---
    const char *err = do_ota_from_sd(tft);

    // Greska - uspjeh = restart, nikad ne stize ovdje
    tft.fillScreen(TFT_BLACK);
    ota_text(tft, 70,  TFT_RED,   2, "UPDATE FAILED");
    ota_text(tft, 120, 0xFFE0,    1, err ? err : "Unknown error");
    ota_text(tft, 160, TFT_WHITE, 1, "Continuing normal boot...");
    Serial.printf("[OTA] ERR: %s\n", err ? err : "unknown");
    delay(5000);
}
