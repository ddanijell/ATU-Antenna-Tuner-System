#pragma once

// ============================================================
// Parsiranje
// ============================================================
static uint16_t parse_dec(const char **p) {
    uint16_t v = 0;
    while (**p >= '0' && **p <= '9') { v = v*10 + (**p-'0'); (*p)++; }
    if (**p == ',') (*p)++;
    return v;
}
static uint8_t parse_hex2(const char *s) {
    auto hv = [](char c)->uint8_t { return c>='A'?c-'A'+10:c-'0'; };
    return (hv(s[0])<<4)|hv(s[1]);
}
static bool parse_pwr(const char *msg) {
    const char *p = msg;
    while (*p && *p!=':') p++; if (!*p) return false; p++;
    g_pwr.pwr      = parse_dec(&p);
    g_pwr.swr      = parse_dec(&p);
    g_pwr.ind      = parse_hex2(p); p+=2; if(*p==',')p++;
    g_pwr.cap      = parse_hex2(p); p+=2; if(*p==',')p++;
    g_pwr.sw       = parse_dec(&p);
    g_pwr.overload = (g_pwr.pwr >= 150);
    return true;
}
static bool parse_cal(const char *msg) {
    const char *p = msg;
    while (*p && *p!=':') p++; if (!*p) return false; p++;
    g_cal.k_mult      = (uint8_t)parse_dec(&p);
    g_cal.min_pwr     = (uint8_t)parse_dec(&p);
    g_cal.div         = parse_dec(&p) + 100;  // ATU šalje raw (0-255), +100 za x100 prikaz
    g_cal.coarse_step = (uint8_t)parse_dec(&p);
    g_cal.relay_ms    = (uint8_t)parse_dec(&p);
    g_cal.retry_wait  = (uint8_t)parse_dec(&p);
    g_cal.debug       = (uint8_t)parse_dec(&p);
    g_cal.adc_swap    = (uint8_t)parse_dec(&p);
    // nova polja: LASTSLOT, IND, CAP, SW
    if (*p) {
        g_cal.last_slot = (uint8_t)parse_dec(&p);
        g_cal.ind       = (uint8_t)parse_dec(&p);
        g_cal.cap       = (uint8_t)parse_dec(&p);
        g_cal.sw        = (uint8_t)parse_dec(&p);
        // sinhronizuj g_band sa slotom iz ATU
        for (uint8_t i = 0; i < BAND_COUNT; i++) {
            if (bands[i].slot == g_cal.last_slot) { g_band = i; break; }
        }
        // sinhronizuj relay state sa ATU vrednostima
        g_pwr.ind = g_cal.ind;
        g_pwr.cap = g_cal.cap;
        g_pwr.sw  = g_cal.sw;
    }
    g_cal_valid = true;
    return true;
}
static uint16_t ind_to_uh100(uint8_t ind) {
    uint16_t v=0;
    if(ind&0x01)v+=5; if(ind&0x02)v+=11; if(ind&0x04)v+=22;
    if(ind&0x08)v+=45; if(ind&0x10)v+=100; if(ind&0x20)v+=220; if(ind&0x40)v+=450;
    return v;
}
static uint16_t cap_to_pf(uint8_t cap) {
    uint16_t v=0;
    if(cap&0x01)v+=10; if(cap&0x02)v+=22; if(cap&0x04)v+=47;
    if(cap&0x08)v+=100; if(cap&0x10)v+=220; if(cap&0x20)v+=470; if(cap&0x40)v+=1000;
    return v;
}
