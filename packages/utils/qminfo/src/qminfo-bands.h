/*
 * qminfo-bands.h
 *
 * QmiNasActiveBand → band number/string mapping tables
 * for LTE, NR5G, UMTS and GSM
 *
 * Part of qminfo — simple monitor for QMI modems
 * by Konstantine Shevlakov (koshev-msk)
 * GPLv3 License. Copyright (c) 2026
 */

#ifndef QMINFO_BANDS_H
#define QMINFO_BANDS_H

#include <libqmi-glib.h>

/* ═══════════════════════════════════════════════════════════════
 * LTE: QmiNasActiveBand → band number (e.g. 7)
 * ═══════════════════════════════════════════════════════════════ */
static inline guint active_band_to_lte_band(QmiNasActiveBand band)
{
    switch (band) {
    case QMI_NAS_ACTIVE_BAND_EUTRAN_1:   return 1;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_2:   return 2;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_3:   return 3;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_4:   return 4;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_5:   return 5;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_6:   return 6;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_7:   return 7;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_8:   return 8;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_9:   return 9;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_10:  return 10;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_11:  return 11;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_12:  return 12;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_13:  return 13;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_14:  return 14;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_17:  return 17;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_18:  return 18;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_19:  return 19;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_20:  return 20;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_21:  return 21;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_23:  return 23;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_24:  return 24;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_25:  return 25;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_26:  return 26;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_27:  return 27;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_28:  return 28;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_29:  return 29;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_30:  return 30;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_31:  return 31;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_32:  return 32;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_33:  return 33;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_34:  return 34;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_35:  return 35;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_36:  return 36;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_37:  return 37;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_38:  return 38;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_39:  return 39;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_40:  return 40;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_41:  return 41;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_42:  return 42;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_43:  return 43;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_46:  return 46;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_47:  return 47;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_48:  return 48;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_66:  return 66;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_71:  return 71;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_125: return 125;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_126: return 126;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_127: return 127;
    case QMI_NAS_ACTIVE_BAND_EUTRAN_250: return 250;
    default:                             return 0;
    }
}

/* ═══════════════════════════════════════════════════════════════
 * LTE EARFCN → band number (based on 3GPP TS 36.101)
 * ═══════════════════════════════════════════════════════════════ */
static inline guint earfcn_to_lte_band(guint16 earfcn)
{
    if (earfcn <= 599) return 1;
    if (earfcn <= 1199) return 2;
    if (earfcn <= 1949) return 3;
    if (earfcn <= 2399) return 4;
    if (earfcn <= 2649) return 5;
    if (earfcn <= 2749) return 6;
    if (earfcn <= 3449) return 7;
    if (earfcn <= 3799) return 8;
    if (earfcn <= 4149) return 9;
    if (earfcn <= 4749) return 10;
    if (earfcn <= 4949) return 11;
    if (earfcn <= 5179) return 12;
    if (earfcn <= 5279) return 13;
    if (earfcn <= 5379) return 14;
    if (earfcn <= 5499) return 17;
    if (earfcn <= 5699) return 17;
    if (earfcn <= 5739) return 18;
    if (earfcn <= 5849) return 19;
    if (earfcn <= 6149) return 20;
    if (earfcn <= 6449) return 21;
    if (earfcn <= 6599) return 22;
    if (earfcn <= 7399) return 23;
    if (earfcn <= 7499) return 24;
    if (earfcn <= 7699) return 25;
    if (earfcn <= 8039) return 26;
    if (earfcn <= 8149) return 27;
    if (earfcn <= 8299) return 28;
    if (earfcn <= 8339) return 29;
    if (earfcn <= 8399) return 30;
    if (earfcn <= 8449) return 31;
    if (earfcn <= 8549) return 32;
    if (earfcn >= 36000 && earfcn <= 36199) return 33;
    if (earfcn >= 36200 && earfcn <= 36349) return 34;
    if (earfcn >= 36350 && earfcn <= 36949) return 35;
    if (earfcn >= 36950 && earfcn <= 37549) return 36;
    if (earfcn >= 37550 && earfcn <= 37749) return 37;
    if (earfcn >= 37750 && earfcn <= 38249) return 38;
    if (earfcn >= 38250 && earfcn <= 38649) return 39;
    if (earfcn >= 38650 && earfcn <= 39649) return 40;
    if (earfcn >= 39650 && earfcn <= 41589) return 41;
    if (earfcn >= 41590 && earfcn <= 43589) return 42;
    if (earfcn >= 43590 && earfcn <= 45589) return 43;
    if (earfcn >= 45590 && earfcn <= 46589) return 44;
    if (earfcn >= 46590 && earfcn <= 46789) return 45;
    if (earfcn >= 46790 && earfcn <= 54539) return 46;
    if (earfcn >= 54540 && earfcn <= 55239) return 47;
    if (earfcn >= 55240 && earfcn <= 56739) return 48;
    if (earfcn >= 56740 && earfcn <= 58239) return 49;
    if (earfcn >= 58240 && earfcn <= 59089) return 50;
    if (earfcn >= 59090 && earfcn <= 59139) return 51;
    if (earfcn >= 59140 && earfcn <= 60139) return 52;
    if (earfcn >= 60140 && earfcn <= 60254) return 53;
    if (earfcn >= 60255 && earfcn <= 60304) return 54;
    if (earfcn >= 65536 && earfcn <= 66435) return 65;
    if (earfcn >= 66436 && earfcn <= 67335) return 66;
    if (earfcn >= 67336 && earfcn <= 67535) return 67;
    if (earfcn >= 67536 && earfcn <= 67835) return 68;
    if (earfcn >= 67836 && earfcn <= 68335) return 69;
    if (earfcn >= 68336 && earfcn <= 68585) return 70;
    if (earfcn >= 68586 && earfcn <= 68935) return 71;
    if (earfcn >= 68936 && earfcn <= 68985) return 72;
    if (earfcn >= 68986 && earfcn <= 69035) return 73;
    if (earfcn >= 69036 && earfcn <= 69465) return 74;
    if (earfcn >= 69466 && earfcn <= 70315) return 75;
    if (earfcn >= 70316 && earfcn <= 70365) return 76;
    if (earfcn >= 70366 && earfcn <= 70545) return 85;

    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 * 5G NR: QmiNasActiveBand → band number (e.g. 78)
 * ═══════════════════════════════════════════════════════════════ */
static inline guint active_band_to_nr_band(QmiNasActiveBand band)
{
    switch (band) {
    case QMI_NAS_ACTIVE_BAND_NR5G_1:   return 1;
    case QMI_NAS_ACTIVE_BAND_NR5G_2:   return 2;
    case QMI_NAS_ACTIVE_BAND_NR5G_3:   return 3;
    case QMI_NAS_ACTIVE_BAND_NR5G_5:   return 5;
    case QMI_NAS_ACTIVE_BAND_NR5G_7:   return 7;
    case QMI_NAS_ACTIVE_BAND_NR5G_8:   return 8;
    case QMI_NAS_ACTIVE_BAND_NR5G_12:  return 12;
    case QMI_NAS_ACTIVE_BAND_NR5G_13:  return 13;
    case QMI_NAS_ACTIVE_BAND_NR5G_14:  return 14;
    case QMI_NAS_ACTIVE_BAND_NR5G_18:  return 18;
    case QMI_NAS_ACTIVE_BAND_NR5G_20:  return 20;
    case QMI_NAS_ACTIVE_BAND_NR5G_25:  return 25;
    case QMI_NAS_ACTIVE_BAND_NR5G_26:  return 26;
    case QMI_NAS_ACTIVE_BAND_NR5G_28:  return 28;
    case QMI_NAS_ACTIVE_BAND_NR5G_29:  return 29;
    case QMI_NAS_ACTIVE_BAND_NR5G_30:  return 30;
    case QMI_NAS_ACTIVE_BAND_NR5G_34:  return 34;
    case QMI_NAS_ACTIVE_BAND_NR5G_38:  return 38;
    case QMI_NAS_ACTIVE_BAND_NR5G_39:  return 39;
    case QMI_NAS_ACTIVE_BAND_NR5G_40:  return 40;
    case QMI_NAS_ACTIVE_BAND_NR5G_41:  return 41;
    case QMI_NAS_ACTIVE_BAND_NR5G_48:  return 48;
    case QMI_NAS_ACTIVE_BAND_NR5G_50:  return 50;
    case QMI_NAS_ACTIVE_BAND_NR5G_51:  return 51;
    case QMI_NAS_ACTIVE_BAND_NR5G_53:  return 53;
    case QMI_NAS_ACTIVE_BAND_NR5G_65:  return 65;
    case QMI_NAS_ACTIVE_BAND_NR5G_66:  return 66;
    case QMI_NAS_ACTIVE_BAND_NR5G_70:  return 70;
    case QMI_NAS_ACTIVE_BAND_NR5G_71:  return 71;
    case QMI_NAS_ACTIVE_BAND_NR5G_74:  return 74;
    case QMI_NAS_ACTIVE_BAND_NR5G_75:  return 75;
    case QMI_NAS_ACTIVE_BAND_NR5G_76:  return 76;
    case QMI_NAS_ACTIVE_BAND_NR5G_77:  return 77;
    case QMI_NAS_ACTIVE_BAND_NR5G_78:  return 78;
    case QMI_NAS_ACTIVE_BAND_NR5G_79:  return 79;
    case QMI_NAS_ACTIVE_BAND_NR5G_80:  return 80;
    case QMI_NAS_ACTIVE_BAND_NR5G_81:  return 81;
    case QMI_NAS_ACTIVE_BAND_NR5G_82:  return 82;
    case QMI_NAS_ACTIVE_BAND_NR5G_83:  return 83;
    case QMI_NAS_ACTIVE_BAND_NR5G_84:  return 84;
    case QMI_NAS_ACTIVE_BAND_NR5G_85:  return 85;
    case QMI_NAS_ACTIVE_BAND_NR5G_86:  return 86;
    case QMI_NAS_ACTIVE_BAND_NR5G_257: return 257;
    case QMI_NAS_ACTIVE_BAND_NR5G_258: return 258;
    case QMI_NAS_ACTIVE_BAND_NR5G_259: return 259;
    case QMI_NAS_ACTIVE_BAND_NR5G_260: return 260;
    case QMI_NAS_ACTIVE_BAND_NR5G_261: return 261;
    default:                            return 0;
    }
}

/* ═══════════════════════════════════════════════════════════════
 * 5G NR (FR1): NR-ARFCN → band number (3GPP TS 38.104)
 * ═══════════════════════════════════════════════════════════════ */
static inline guint nrarfcn_to_nr_band(guint32 nrarfcn)
{
    /* FDD bands */
    if (nrarfcn >= 422000 && nrarfcn <= 434000) return 1;
    if (nrarfcn >= 386000 && nrarfcn <= 398000) return 2;
    if (nrarfcn >= 361000 && nrarfcn <= 376000) return 3;
    if (nrarfcn >= 173800 && nrarfcn <= 178800) return 5;
    if (nrarfcn >= 524000 && nrarfcn <= 538000) return 7;
    if (nrarfcn >= 185000 && nrarfcn <= 192000) return 8;
    if (nrarfcn >= 145800 && nrarfcn <= 149200) return 12;
    if (nrarfcn >= 149200 && nrarfcn <= 151200) return 13;
    if (nrarfcn >= 151600 && nrarfcn <= 153600) return 14;
    if (nrarfcn >= 172000 && nrarfcn <= 175000) return 18;
    if (nrarfcn >= 158200 && nrarfcn <= 164200) return 20;
    if (nrarfcn >= 305000 && nrarfcn <= 311800) return 24;
    if (nrarfcn >= 386000 && nrarfcn <= 399000) return 25;
    if (nrarfcn >= 171800 && nrarfcn <= 178800) return 26;
    if (nrarfcn >= 151600 && nrarfcn <= 160600) return 28;
    if (nrarfcn >= 143400 && nrarfcn <= 145600) return 29;  /* SDL */
    if (nrarfcn >= 470000 && nrarfcn <= 472000) return 30;
    if (nrarfcn >= 92500  && nrarfcn <= 93500)  return 31;
    if (nrarfcn >= 422000 && nrarfcn <= 440000) return 65;
    if (nrarfcn >= 422000 && nrarfcn <= 440000) return 66;  /* same range as 65, need priority */
    if (nrarfcn >= 147600 && nrarfcn <= 151600) return 67;  /* SDL */
    if (nrarfcn >= 150600 && nrarfcn <= 156600) return 68;
    if (nrarfcn >= 399000 && nrarfcn <= 404000) return 70;
    if (nrarfcn >= 123400 && nrarfcn <= 130400) return 71;
    if (nrarfcn >= 92200  && nrarfcn <= 93200)  return 72;
    if (nrarfcn >= 295000 && nrarfcn <= 303600) return 74;
    if (nrarfcn >= 286400 && nrarfcn <= 303400) return 75;  /* SDL */
    if (nrarfcn >= 285400 && nrarfcn <= 286400) return 76;  /* SDL */
    if (nrarfcn >= 145600 && nrarfcn <= 149200) return 85;
    if (nrarfcn >= 84000  && nrarfcn <= 85000)  return 87;
    if (nrarfcn >= 84400  && nrarfcn <= 85400)  return 88;
    if (nrarfcn >= 122400 && nrarfcn <= 130400) return 105;
    if (nrarfcn >= 187000 && nrarfcn <= 188000) return 106;
    if (nrarfcn >= 286400 && nrarfcn <= 303400) return 109;

    /* TDD bands (FR1) */
    if (nrarfcn >= 402000 && nrarfcn <= 405000) return 34;
    if (nrarfcn >= 514000 && nrarfcn <= 524000) return 38;
    if (nrarfcn >= 376000 && nrarfcn <= 384000) return 39;
    if (nrarfcn >= 460000 && nrarfcn <= 480000) return 40;
    if (nrarfcn >= 499200 && nrarfcn <= 538000) return 41;
    if (nrarfcn >= 743334 && nrarfcn <= 795000) return 46;
    if (nrarfcn >= 790334 && nrarfcn <= 795000) return 47;
    if (nrarfcn >= 636667 && nrarfcn <= 646666) return 48;
    if (nrarfcn >= 286400 && nrarfcn <= 303400) return 50;
    if (nrarfcn >= 285400 && nrarfcn <= 286400) return 51;
    if (nrarfcn >= 496700 && nrarfcn <= 499000) return 53;
    if (nrarfcn >= 334000 && nrarfcn <= 335000) return 54;
    if (nrarfcn >= 620000 && nrarfcn <= 680000) return 77;
    if (nrarfcn >= 620000 && nrarfcn <= 653332) return 78;
    if (nrarfcn >= 693334 && nrarfcn <= 733333) return 79;
    if (nrarfcn >= 795000 && nrarfcn <= 875000) return 96;
    if (nrarfcn >= 828334 && nrarfcn <= 875000) return 104;

    /* SUL bands (optional for display) */
    if (nrarfcn >= 342000 && nrarfcn <= 357000) return 80;
    if (nrarfcn >= 176000 && nrarfcn <= 183000) return 81;
    if (nrarfcn >= 166400 && nrarfcn <= 172400) return 82;
    if (nrarfcn >= 140600 && nrarfcn <= 149600) return 83;
    if (nrarfcn >= 384000 && nrarfcn <= 396000) return 84;
    if (nrarfcn >= 342000 && nrarfcn <= 356000) return 86;
    if (nrarfcn >= 164800 && nrarfcn <= 169800) return 89;
    if (nrarfcn >= 499200 && nrarfcn <= 538000) return 90;
    if (nrarfcn >= 285400 && nrarfcn <= 303400) return 91;
    if (nrarfcn >= 286400 && nrarfcn <= 303400) return 92;
    if (nrarfcn >= 285400 && nrarfcn <= 286400) return 93;
    if (nrarfcn >= 286400 && nrarfcn <= 303400) return 94;
    if (nrarfcn >= 402000 && nrarfcn <= 405000) return 95;
    if (nrarfcn >= 460000 && nrarfcn <= 480000) return 97;
    if (nrarfcn >= 376000 && nrarfcn <= 384000) return 98;
    if (nrarfcn >= 325300 && nrarfcn <= 332100) return 99;
    if (nrarfcn >= 380000 && nrarfcn <= 382000) return 101;

    /* FR2 bands (mmWave) – если нужны */
    if (nrarfcn >= 2054166 && nrarfcn <= 2104165) return 257;
    if (nrarfcn >= 2016667 && nrarfcn <= 2070831) return 258;
    if (nrarfcn >= 2270833 && nrarfcn <= 2337499) return 259;
    if (nrarfcn >= 2229166 && nrarfcn <= 2279165) return 260;
    if (nrarfcn >= 2070833 && nrarfcn <= 2084999) return 261;

    return 0;
}

/* ═══════════════════════════════════════════════════════════════
 * UMTS/WCDMA: QmiNasActiveBand → band string (e.g. "B1")
 * ═══════════════════════════════════════════════════════════════ */
static inline const char *active_band_to_umts_band(QmiNasActiveBand band)
{
    switch (band) {
    case QMI_NAS_ACTIVE_BAND_WCDMA_2100:       return "B1";
    case QMI_NAS_ACTIVE_BAND_WCDMA_PCS_1900:   return "B2";
    case QMI_NAS_ACTIVE_BAND_WCDMA_DCS_1800:   return "B3";
    case QMI_NAS_ACTIVE_BAND_WCDMA_1700_US:    return "B4";
    case QMI_NAS_ACTIVE_BAND_WCDMA_850:        return "B5";
    case QMI_NAS_ACTIVE_BAND_WCDMA_800:        return "B6";
    case QMI_NAS_ACTIVE_BAND_WCDMA_2600:       return "B7";
    case QMI_NAS_ACTIVE_BAND_WCDMA_900:        return "B8";
    case QMI_NAS_ACTIVE_BAND_WCDMA_1700_JAPAN: return "B9";
    case QMI_NAS_ACTIVE_BAND_WCDMA_1500_JAPAN: return "B11";
    case QMI_NAS_ACTIVE_BAND_WCDMA_850_JAPAN:  return "B19";
    default:                                    return NULL;
    }
}

/* ═══════════════════════════════════════════════════════════════
 * GSM: QmiNasActiveBand → band string (e.g. "DCS1800")
 * ═══════════════════════════════════════════════════════════════ */
static inline const char *active_band_to_gsm_band(QmiNasActiveBand band)
{
    switch (band) {
    case QMI_NAS_ACTIVE_BAND_GSM_450:          return "GSM450";
    case QMI_NAS_ACTIVE_BAND_GSM_480:          return "GSM480";
    case QMI_NAS_ACTIVE_BAND_GSM_750:          return "GSM750";
    case QMI_NAS_ACTIVE_BAND_GSM_850:          return "GSM850";
    case QMI_NAS_ACTIVE_BAND_GSM_900_EXTENDED: return "EGSM900";
    case QMI_NAS_ACTIVE_BAND_GSM_900_PRIMARY:  return "GSM900";
    case QMI_NAS_ACTIVE_BAND_GSM_900_RAILWAYS: return "RGSM900";
    case QMI_NAS_ACTIVE_BAND_GSM_DCS_1800:     return "DCS1800";
    case QMI_NAS_ACTIVE_BAND_GSM_PCS_1900:     return "PCS1900";
    default:                                    return NULL;
    }
}

#endif /* QMINFO_BANDS_H */
