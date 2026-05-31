/*
 * qminfo-helpers.h
 *
 * Pure helper functions for qminfo:
 *   - unit converters (BW, CSQ, TA)
 *   - QMI enum to string mappers
 *   - JSON string escaper
 *
 * Part of qminfo — simple monitor for QMI modems
 * by Konstantine Shevlakov (koshev-msk)
 * GPLv3 License. Copyright (c) 2026
 */

#ifndef QMINFO_HELPERS_H
#define QMINFO_HELPERS_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libqmi-glib.h>

/* ═══════════════════════════════════════════════════════════════
 * Unit converters
 * ═══════════════════════════════════════════════════════════════ */

/* LTE DL bandwidth index (0-5) → kHz */
static inline guint32 bw_index_to_khz(guint8 bw)
{
    static const guint32 t[] = { 1400, 3000, 5000, 10000, 15000, 20000 };
    return (bw < 6) ? t[bw] : 0;
}

/* CSQ (0-31) → percent (0-100), -1 if invalid */
static inline int csq_to_percent(int csq)
{
    if (csq < 0 || csq > 31) return -1;
    return (csq * 100) / 31;
}

/* LTE Timing Advance (μs) → distance in km */
static inline float ta_to_km(guint32 ta)
{
    /* d = c * t / 2 = 299792458 * (ta * 10^-6) / 2 / 1000 (km) */
    float d = (299792458.0f * (float)ta * 1e-6f) / 2.0f / 1000.0f;
    return roundf(d * 100.0f) / 100.0f;
}

/* ═══════════════════════════════════════════════════════════════
 * QMI enum → string mappers
 * ═══════════════════════════════════════════════════════════════ */

static inline const char *radio_to_mode(QmiNasRadioInterface iface)
{
    switch (iface) {
    case QMI_NAS_RADIO_INTERFACE_GSM:         return "GSM";
    case QMI_NAS_RADIO_INTERFACE_UMTS:        return "UMTS";
    case QMI_NAS_RADIO_INTERFACE_CDMA_1X:     return "CDMA";
    case QMI_NAS_RADIO_INTERFACE_CDMA_1XEVDO: return "EVDO";
    case QMI_NAS_RADIO_INTERFACE_LTE:         return "LTE";
    case QMI_NAS_RADIO_INTERFACE_5GNR:        return "5G NR";
    default:                                   return "Unknown";
    }
}

static inline const char *reg_state_to_string(QmiNasRegistrationState state)
{
    switch (state) {
    case QMI_NAS_REGISTRATION_STATE_NOT_REGISTERED:           return "Not Registered";
    case QMI_NAS_REGISTRATION_STATE_REGISTERED:               return "Registered";
    case QMI_NAS_REGISTRATION_STATE_NOT_REGISTERED_SEARCHING: return "Searching";
    case QMI_NAS_REGISTRATION_STATE_REGISTRATION_DENIED:      return "Denied";
    case QMI_NAS_REGISTRATION_STATE_UNKNOWN:                  return "Unknown";
    default:                                                   return "Unknown";
    }
}

/* ═══════════════════════════════════════════════════════════════
 * JSON string escaper
 * ═══════════════════════════════════════════════════════════════ */

static inline void jstr(char *dst, size_t sz, const char *src)
{
    if (!src) { dst[0] = '\0'; return; }
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 2 < sz; i++) {
        if (src[i] == '"' || src[i] == '\\') dst[j++] = '\\';
        dst[j++] = src[i];
    }
    dst[j] = '\0';
}

#endif /* QMINFO_HELPERS_H */
