/*
 * qminfo.c
 *
 * simple monitor for QMI modems with json output
 *       by Konstantine Shevlakov (koshev-msk)
 *		 GPLv3 License. Copyrihgt (c) 2026
 *
 * compile:
 *   gcc $(pkg-config --cflags --libs qmi-glib) -lm -o qminfo qminfo.c
 *
 * run:
 *   sudo ./qminfo /dev/cdc-wdm0              # auto-detect (QMI first)
 *   sudo ./qminfo /dev/cdc-wdm0 --json       # JSON output
 *   sudo ./qminfo /dev/cdc-wdm0 --qmi        # force native QMI
 *   sudo ./qminfo /dev/cdc-wdm0 --mbim       # force QMI over MBIM
 *   sudo ./qminfo /dev/cdc-wdm0 --mbim --json
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libqmi-glib.h>

/* ═══════════════════════════════════════════════════════════════
 * main data
 * ═══════════════════════════════════════════════════════════════ */
typedef struct {
    char    imei[32];
    char    model[128];
    char    firmware[128];
    char    iccid[32];
    char    imsi[32];
    gint    chip_temp;

    gint    reg_state;
    char    cops[64];
    char    mode[32];
    guint32 lac;
    guint64 cid;

    gint8   rssi;
    gint16  rsrp;       /* *0.1 dBm */
    gint8   rsrq;       /* *0.1 dB  */
    gint16  sinr;       /* *0.1 dB  */
    gint    csq;        /* 0-31, -1 if not exist */

    guint16 arfcn;
    guint16 pci;
    guint32 enb_id;
    guint8  cell_sector;
    guint32 ta;
    guint8  bw_dl;      /* index 0-5 */

    gint    lte_ca;
    char    scc_bands[128];
	char    scc_bands_json[128];
    guint32 bw_ca_total; /* kHz */

    gboolean has_rsrp;
    gboolean has_rsrq;
    gboolean has_sinr;
    gboolean has_ta;
    gboolean has_bw_dl;
    gboolean has_pci;
    gboolean has_arfcn;
    gboolean has_ca;
    gboolean has_temp;
    gboolean has_iccid;
    gboolean has_imsi;
    gboolean has_imei;
    gboolean has_model;
    gboolean has_firmware;
    gboolean has_csq;
    gboolean ta_valid;
    
    gboolean is_umts;   /* UMTS flag */
} ModemInfo;

/* ═══════════════════════════════════════════════════════════════
 * main state
 * ═══════════════════════════════════════════════════════════════ */
static GMainLoop    *loop      = NULL;
static QmiDevice    *device    = NULL;
static QmiClientNas *nas       = NULL;
static QmiClientDms *dms       = NULL;
static ModemInfo     info;
static gboolean      json_mode = FALSE;
static const char   *dev_path  = NULL;
static gint          pending   = 0;
typedef enum {
    MODE_AUTO,   /* try QMI first, fallback to MBIM */
    MODE_QMI,    /* force native QMI */
    MODE_MBIM,   /* force QMI over MBIM */
} DeviceMode;

static DeviceMode    device_mode = MODE_AUTO;
static gint          open_retries = 0;  /* retry counter for auto mode */

/* ═══════════════════════════════════════════════════════════════
 * helpers
 * ═══════════════════════════════════════════════════════════════ */

static guint32 bw_index_to_khz(guint8 bw)
{
    static const guint32 t[] = { 1400, 3000, 5000, 10000, 15000, 20000 };
    return (bw < 6) ? t[bw] : 0;
}

static int csq_to_percent(int csq)
{
    if (csq < 0 || csq > 31) return -1;
    return (csq * 100) / 31;
}

static float ta_to_km(guint32 ta)
{
    /* QMI TA μsec.
     * d = c * t / 2 = 299792458 * (ta * 10^-6) / 2 / 1000 (in km) */
    float d = (299792458.0f * (float)ta * 1e-6f) / 2.0f / 1000.0f;
    return roundf(d * 100.0f) / 100.0f;
}

static const char *radio_to_mode(QmiNasRadioInterface iface)
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

static const char *reg_state_to_string(QmiNasRegistrationState state)
{
    switch (state) {
    case QMI_NAS_REGISTRATION_STATE_NOT_REGISTERED:          return "Not Registered";
    case QMI_NAS_REGISTRATION_STATE_REGISTERED:              return "Registered";
    case QMI_NAS_REGISTRATION_STATE_NOT_REGISTERED_SEARCHING: return "Searching";
    case QMI_NAS_REGISTRATION_STATE_REGISTRATION_DENIED:     return "Denied";
    case QMI_NAS_REGISTRATION_STATE_UNKNOWN:                 return "Unknown";
    default:                                                  return "Unknown";
    }
}

static guint active_band_to_lte_band(QmiNasActiveBand band)
{
    /* QmiNasActiveBand enum to LTE band */
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

static void jstr(char *dst, size_t sz, const char *src)
{
    if (!src) { dst[0] = '\0'; return; }
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 2 < sz; i++) {
        if (src[i] == '"' || src[i] == '\\') dst[j++] = '\\';
        dst[j++] = src[i];
    }
    dst[j] = '\0';
}

/* ═══════════════════════════════════════════════════════════════
 * Read USB ID from sysfs
 * ═══════════════════════════════════════════════════════════════ */
static void read_usb_id(const char *dev_name, char *manufacturer, size_t man_size, char *product, size_t prod_size)
{
    char path[512];
    
    manufacturer[0] = '\0';
    product[0] = '\0';
    
    if (!dev_name || !dev_name[0]) return;
    
    /* Read manufacturer */
    snprintf(path, sizeof(path), "/sys/class/usbmisc/%s/../../../manufacturer", dev_name);
    
    FILE *f = fopen(path, "r");
    if (f) {
        if (fgets(manufacturer, man_size, f)) {
            size_t l = strlen(manufacturer);
            if (l > 0 && manufacturer[l-1] == '\n')
                manufacturer[l-1] = '\0';
        }
        fclose(f);
    }
    
    /* Read product */
    snprintf(path, sizeof(path), "/sys/class/usbmisc/%s/../../../product", dev_name);
    f = fopen(path, "r");
    if (f) {
        if (fgets(product, prod_size, f)) {
            size_t l = strlen(product);
            if (l > 0 && product[l-1] == '\n')
                product[l-1] = '\0';
        }
        fclose(f);
    }
}

/* ═══════════════════════════════════════════════════════════════
 * release DMS -> release NAS > close device -> quit loop
 * ═══════════════════════════════════════════════════════════════ */

static void on_device_closed(QmiDevice *dev, GAsyncResult *res, gpointer ud)
{
    (void)dev; (void)ud;
    GError *err = NULL;
    qmi_device_close_finish(device, res, &err);
    g_clear_error(&err);
    g_main_loop_quit(loop);
}

static void do_close_device(void)
{
    qmi_device_close_async(device, 5, NULL,
        (GAsyncReadyCallback)on_device_closed, NULL);
}

static void on_nas_released(QmiDevice *dev, GAsyncResult *res, gpointer ud)
{
    (void)dev; (void)ud;
    GError *err = NULL;
    qmi_device_release_client_finish(device, res, &err);
    g_clear_error(&err);
	if (nas){
		g_object_unref(nas);
		nas = NULL;
	}
    do_close_device();
}

static void on_dms_released(QmiDevice *dev, GAsyncResult *res, gpointer ud)
{
    (void)dev; (void)ud;
    GError *err = NULL;
    qmi_device_release_client_finish(device, res, &err);
    g_clear_error(&err);
	if (dms){
		g_object_unref(dms);
		dms = NULL;
	}

    if (nas) {
        qmi_device_release_client(device, QMI_CLIENT(nas),
            QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
            5, NULL,
            (GAsyncReadyCallback)on_nas_released, NULL);
    } else {
        do_close_device();
    }
}

static void do_release_clients(void)
{
    if (dms) {
        qmi_device_release_client(device, QMI_CLIENT(dms),
            QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
            5, NULL,
            (GAsyncReadyCallback)on_dms_released, NULL);
    } else if (nas) {
        qmi_device_release_client(device, QMI_CLIENT(nas),
            QMI_DEVICE_RELEASE_CLIENT_FLAGS_RELEASE_CID,
            5, NULL,
            (GAsyncReadyCallback)on_nas_released, NULL);
    } else {
        do_close_device();
    }
}

/* ═══════════════════════════════════════════════════════════════
 * Results
 * ═══════════════════════════════════════════════════════════════ */
static void emit_results(void)
{
    int   csq_pct = info.has_csq ? csq_to_percent(info.csq) : -1;
    float dist_km = -1.0f;
    const char* siname;

    if (info.has_ta && info.ta_valid && info.ta > 0 && info.ta != 0xFFFFFFFF)
        dist_km = ta_to_km(info.ta);

    if (info.cid > 0) {
        info.enb_id      = (guint32)(info.cid >> 8);
        info.cell_sector = (guint8)(info.cid & 0xFF);
    }

    if (!json_mode) {
        siname = info.is_umts ? "EcIO" : "SINR";
        printf("\n=================================================\n");
        printf("  ModemInfo-QMI — %s\n", dev_path);
        printf("=================================================\n");
        printf("  Model      : %s\n",  info.has_model    ? info.model    : "N/A");
        printf("  Firmware   : %s\n",  info.has_firmware ? info.firmware : "N/A");
        printf("  IMEI       : %s\n",  info.has_imei     ? info.imei     : "N/A");
        if (info.has_iccid) printf("  ICCID      : %s\n", info.iccid);
        if (info.has_imsi)  printf("  IMSI       : %s\n", info.imsi);
        if (info.has_temp)  printf("  Temp       : %d C\n", info.chip_temp);
        printf("-------------------------------------------------\n");
        printf("  Operator   : %s\n",  info.cops[0] ? info.cops : "N/A");
        printf("  Mode       : %s\n",  info.mode[0] ? info.mode : "N/A");
        printf("  Reg.State  : %s\n",  reg_state_to_string((QmiNasRegistrationState)info.reg_state));
        printf("  LAC/TAC    : %u\n",  info.lac);
        printf("  CID        : %llu\n",(unsigned long long)info.cid);
        printf("  eNB ID     : %u\n",  info.enb_id);
        printf("  Sector     : %u\n",  info.cell_sector);
        if (info.has_arfcn) printf("  RF Chan.   : %u\n",  info.arfcn);
        if (info.has_pci)   printf("  PCI        : %u\n",  info.pci);
        if (dist_km >= 0)   printf("  Distance   : ~%.2f km (TA=%u)\n", dist_km, info.ta);
		if (info.has_bw_dl && strcmp(info.mode, "LTE") == 0) 
			printf("  BW DL      : %.1f MHz\n", bw_index_to_khz(info.bw_dl) / 1000.0f);
        printf("-------------------------------------------------\n");
	if (info.has_csq)  printf("  Strength   : %d%% (CSQ %d)\n", csq_pct, info.csq);
        printf("  RSSI       : %d dBm\n", info.rssi);
        if (info.has_rsrp) printf("  RSRP       : %d dBm\n", (int)roundf(info.rsrp));
        if (info.has_rsrq) printf("  RSRQ       : %d dB\n",  (int)roundf(info.rsrq));
        if (info.has_sinr) printf("  %s       : %d dB\n", siname, (int)roundf(info.sinr / 10.0f));
        if (info.has_ca) {
            printf("  LTE-A SCC  : %d — %s\n", info.lte_ca, info.scc_bands);
            printf("  BW CA      : %.1f MHz\n",   info.bw_ca_total / 1000.0f);
        }
        printf("=================================================\n");
        return;
    }

    /* ── JSON ── */
    char b_model[256], b_cops[128], b_fw[256];
    char b_imei[64], b_iccid[64], b_imsi[64], b_scc[128];
    jstr(b_model, sizeof(b_model), info.has_model    ? info.model    : "");
    jstr(b_cops,  sizeof(b_cops),  info.cops[0]      ? info.cops     : "");
    jstr(b_fw,    sizeof(b_fw),    info.has_firmware ? info.firmware : "");
    jstr(b_imei,  sizeof(b_imei),  info.has_imei     ? info.imei     : "");
    jstr(b_iccid, sizeof(b_iccid), info.has_iccid    ? info.iccid    : "");
    jstr(b_imsi,  sizeof(b_imsi),  info.has_imsi     ? info.imsi     : "");
    jstr(b_scc,   sizeof(b_scc),   info.has_ca       ? info.scc_bands_json : "");

    const char *csq_col = "";
    if (info.has_csq && csq_pct >= 0)
        csq_col = (info.csq > 20) ? "green" : (info.csq > 10) ? "orange" : "red";

    char csq_per_str[16] = "";
    if (csq_pct >= 0) snprintf(csq_per_str, sizeof(csq_per_str), "%d", csq_pct);

    printf("{\n");
    printf("  \"device\"   : \"%s\",\n",  b_model);
    printf("  \"cops\"     : \"%s\",\n",  b_cops);
    printf("  \"mode\"     : \"%s\",\n",  info.mode[0] ? info.mode : "");
    printf("  \"csq_per\"  : \"%s\",\n",  csq_per_str);
    printf("  \"lac\"      : \"%u\",\n",  info.lac);
    printf("  \"cid\"      : \"%llu\",\n",(unsigned long long)info.cid);
    printf("  \"rssi\"     : \"%d\",\n",  info.rssi);
    if (info.has_sinr)
        printf("  \"sinr\"     : \"%d\",\n", (int)roundf(info.sinr / 10.0f));
    else
        printf("  \"sinr\"     : \"\",\n");
    if (info.has_rsrp)
        printf("  \"rsrp\"     : \"%d\",\n", (int)roundf(info.rsrp));
    else
        printf("  \"rsrp\"     : \"\",\n");
    if (info.has_rsrq)
        printf("  \"rsrq\"     : \"%d\",\n", (int)roundf(info.rsrq));
    else
        printf("  \"rsrq\"     : \"\",\n");
    printf("  \"imei\"     : \"%s\",\n",  b_imei);
    printf("  \"reg\"      : \"%d\",\n",  info.reg_state);
    printf("  \"csq_col\"  : \"%s\",\n",  csq_col);
    if (info.has_arfcn)
        printf("  \"arfcn\"    : \"%u\",\n", info.arfcn);
    else
        printf("  \"arfcn\"    : \"\",\n");
    if (info.has_temp)
        printf("  \"chiptemp\" : \"%d\",\n", info.chip_temp);
    else
        printf("  \"chiptemp\" : \"\",\n");
    printf("  \"firmware\" : \"%s\",\n",  b_fw);
	if (info.has_bw_dl && strcmp(info.mode, "LTE") == 0)
		printf("  \"bwdl\"     : \"%u\",\n",  info.has_bw_dl ? (guint32)info.bw_dl : 0);
	else
		printf("  \"bwdl\"     : \"\",\n");
    printf("  \"lteca\"    : \"%d\",\n",  info.has_ca ? info.lte_ca : 0);
    printf("  \"enbid\"    : \"%u\",\n",  info.enb_id);
    if (dist_km >= 0.0f)
        printf("  \"distance\" : \"%.2f\",\n", dist_km);
    else
        printf("  \"distance\" : \"\",\n");
    printf("  \"cell\"     : \"%u\",\n",  info.cell_sector);
    printf("  \"scc\"      : \"%s\",\n",  b_scc);
    printf("  \"bwca\"     : \"%u\",\n",  info.has_ca ? info.bw_ca_total / 1000 : 0);
    printf("  \"iccid\"    : \"%s\",\n",  b_iccid);
    printf("  \"imsi\"     : \"%s\",\n",  b_imsi);
    if (info.has_pci)
        printf("  \"pci\"      : \"%u\"\n", info.pci);
    else
        printf("  \"pci\"      : \"\"\n");
    printf("}\n");
}

/* ═══════════════════════════════════════════════════════════════
 * check_done: release all queries
 * ═══════════════════════════════════════════════════════════════ */
static void check_done(void)
{
    pending--;
    if (pending <= 0) {
        emit_results();
        do_release_clients();
    }
}

/* ═══════════════════════════════════════════════════════════════
 * NAS — Get Signal Strength (CSQ)
 * ═══════════════════════════════════════════════════════════════ */
static void on_signal_strength(QmiClientNas *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageNasGetSignalStrengthOutput *out =
        qmi_client_nas_get_signal_strength_finish(nas, res, &err);

    if (out && qmi_message_nas_get_signal_strength_output_get_result(out, NULL)) {
        gint8 rssi = 0;
        QmiNasRadioInterface iface = QMI_NAS_RADIO_INTERFACE_NONE;
        if (qmi_message_nas_get_signal_strength_output_get_signal_strength(
                out, &rssi, &iface, NULL)) {
            int csq = (rssi + 113) / 2;
            if (csq < 0)  csq = 0;
            if (csq > 31) csq = 31;
            info.csq     = csq;
            info.has_csq = TRUE;
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_nas_get_signal_strength_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * NAS — Get Cell Location Info
 * ═══════════════════════════════════════════════════════════════ */
static void on_cell_location(QmiClientNas *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageNasGetCellLocationInfoOutput *out =
        qmi_client_nas_get_cell_location_info_finish(nas, res, &err);

    if (out && qmi_message_nas_get_cell_location_info_output_get_result(out, NULL)) {

        /* 5G NR ARFCN */
        guint32 nr_arfcn = 0;
        if (qmi_message_nas_get_cell_location_info_output_get_nr5g_arfcn(
                out, &nr_arfcn, NULL) && nr_arfcn) {
            info.arfcn     = (guint16)nr_arfcn;
            info.has_arfcn = TRUE;
        }

        /* LTE intra-frequency v2: EARFCN=guint16, PCI=guint16 */
        guint16 earfcn = 0, pci16 = 0;
        GArray *cells  = NULL;
        if (qmi_message_nas_get_cell_location_info_output_get_intrafrequency_lte_info_v2(
                out,
                NULL, NULL, NULL, NULL,
                &earfcn,
                &pci16,
                NULL, NULL, NULL, NULL,
                &cells,
                NULL)) {
            if (earfcn) {
                info.arfcn     = earfcn;
                info.has_arfcn = TRUE;
            }
            info.pci     = pci16;
            info.has_pci = TRUE;
        }

        /* TA */
        guint32 ta32 = 0xFFFFFFFF;
        if (qmi_message_nas_get_cell_location_info_output_get_lte_info_timing_advance(
                out, &ta32, NULL)) {
            info.ta       = ta32;
            info.has_ta   = TRUE;
            info.ta_valid = (ta32 != 0xFFFFFFFF && ta32 != 0x7FFFFFFF && ta32 < 1000);
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_nas_get_cell_location_info_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * NAS — Get LTE phy CA Info
 * ═══════════════════════════════════════════════════════════════ */
static void on_lte_ca(QmiClientNas *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageNasGetLteCphyCaInfoOutput *out =
        qmi_client_nas_get_lte_cphy_ca_info_finish(nas, res, &err);

    if (out && qmi_message_nas_get_lte_cphy_ca_info_output_get_result(out, NULL)) {
        guint16           pcc_pci  = 0;
        guint16           pcc_rx   = 0;
        QmiNasDLBandwidth pcc_bw   = (QmiNasDLBandwidth)0;
        QmiNasActiveBand  pcc_band = (QmiNasActiveBand)0;
        if (qmi_message_nas_get_lte_cphy_ca_info_output_get_phy_ca_agg_pcell_info(
                out, &pcc_pci, &pcc_rx, &pcc_bw, &pcc_band, NULL)) {
            guint8 bw = (guint8)pcc_bw;
            if (bw < 6) {
                info.bw_dl     = bw;
                info.has_bw_dl = TRUE;
            }
        }

        /* SCC — probe scell_info (secondary cell) */
        {
            guint16           scc_pci   = 0;
            guint16           scc_rx    = 0;
            QmiNasDLBandwidth scc_bw    = (QmiNasDLBandwidth)0;
            QmiNasActiveBand  scc_band  = (QmiNasActiveBand)0;
            QmiNasScellState  scc_state = QMI_NAS_SCELL_STATE_DECONFIGURED;

            if (qmi_message_nas_get_lte_cphy_ca_info_output_get_phy_ca_agg_scell_info(
                    out, &scc_pci, &scc_rx, &scc_bw, &scc_band, &scc_state, NULL)
                    && scc_state == QMI_NAS_SCELL_STATE_ACTIVATED) {
                info.has_ca       = TRUE;
                info.lte_ca       = 1;
                info.scc_bands[0] = '\0';
				info.scc_bands_json[0] = '\0';
                guint32 total     = info.has_bw_dl ? bw_index_to_khz(info.bw_dl) : 0;
                char tmp[16];
                
                /* For text mode: B40 */
                snprintf(tmp, sizeof(tmp), "B%u", active_band_to_lte_band(scc_band));
                g_strlcat(info.scc_bands, tmp, sizeof(info.scc_bands));
				/* For json mode: +40 */
				snprintf(tmp, sizeof(tmp), "+%u", active_band_to_lte_band(scc_band));
                g_strlcat(info.scc_bands_json, tmp, sizeof(info.scc_bands_json));
                
                total += bw_index_to_khz((guint8)scc_bw);
                info.bw_ca_total = total;
            } else {
                /* Fallback: massive secondary cells */
                GArray *scc_arr = NULL;
                if (qmi_message_nas_get_lte_cphy_ca_info_output_get_phy_ca_agg_secondary_cells(
                        out, &scc_arr, NULL) && scc_arr && scc_arr->len > 0) {
                    info.has_ca       = TRUE;
                    info.lte_ca       = (gint)scc_arr->len;
                    info.scc_bands[0] = '\0';
					info.scc_bands_json[0] = '\0';
                    guint32 total     = info.has_bw_dl ? bw_index_to_khz(info.bw_dl) : 0;
                    
                    for (guint i = 0; i < scc_arr->len; i++) {
                        QmiMessageNasGetLteCphyCaInfoOutputPhyCaAggSecondaryCellsSsc *el =
                            &g_array_index(scc_arr,
                                QmiMessageNasGetLteCphyCaInfoOutputPhyCaAggSecondaryCellsSsc, i);
                        char tmp[16];
                        
                        if (i > 0) g_strlcat(info.scc_bands, ",", sizeof(info.scc_bands));
						/* For text mode: B40 */
                        snprintf(tmp, sizeof(tmp), "B%u", active_band_to_lte_band(el->lte_band));
                        g_strlcat(info.scc_bands, tmp, sizeof(info.scc_bands));
						/* For json mode: +40 */
                        snprintf(tmp, sizeof(tmp), "+%u", active_band_to_lte_band(el->lte_band));
                        g_strlcat(info.scc_bands_json, tmp, sizeof(info.scc_bands_json));						
                        
                        total += bw_index_to_khz((guint8)el->dl_bandwidth);
                    }
                    info.bw_ca_total = total;
                }
            }
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_nas_get_lte_cphy_ca_info_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * NAS — Get Signal Info (RSRP, RSRQ, SINR)
 * ═══════════════════════════════════════════════════════════════ */
static void on_signal_info(QmiClientNas *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageNasGetSignalInfoOutput *out =
        qmi_client_nas_get_signal_info_finish(nas, res, &err);

    if (out && qmi_message_nas_get_signal_info_output_get_result(out, NULL)) {
        /* LTE: rssi=gint8, rsrq=gint8, rsrp=gint16, snr=gint16 (все *0.1) */
        gint8  lrssi = 0, lrsrq = 0;
        gint16 lrsrp = 0, lsnr  = 0;
        if (qmi_message_nas_get_signal_info_output_get_lte_signal_strength(
                out, &lrssi, &lrsrq, &lrsrp, &lsnr, NULL)) {
            info.rssi     = lrssi;
            info.rsrq     = lrsrq;
            info.rsrp     = lrsrp;
            info.sinr     = lsnr;
            info.has_rsrp = TRUE;
            info.has_rsrq = TRUE;
            info.has_sinr = TRUE;
            info.is_umts  = FALSE;
        }

        /* WCDMA: rssi=gint8, ecio=gint16 */
        gint8  wrssi = 0;
        gint16 wecio = 0;
        if (qmi_message_nas_get_signal_info_output_get_wcdma_signal_strength(
                out, &wrssi, &wecio, NULL)) {
            info.rssi     = wrssi;
            info.sinr     = wecio;
            info.has_sinr = TRUE;
            info.is_umts  = TRUE;
        }

        /* GSM */
        gint8 grssi = 0;
        if (!info.has_rsrp && !info.has_sinr &&
            qmi_message_nas_get_signal_info_output_get_gsm_signal_strength(
                out, &grssi, NULL)) {
            info.rssi = grssi;
            info.is_umts = FALSE;
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_nas_get_signal_info_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * NAS — Get RF Band Information (для ARFCN в 3G/UMTS)
 * ═══════════════════════════════════════════════════════════════ */
static void on_rf_band_info(QmiClientNas *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageNasGetRfBandInformationOutput *out =
        qmi_client_nas_get_rf_band_information_finish(nas, res, &err);

    if (out && qmi_message_nas_get_rf_band_information_output_get_result(out, NULL)) {
        GArray *list = NULL;
        
        /* get main list band information */
        if (qmi_message_nas_get_rf_band_information_output_get_list(
                out, &list, NULL) && list && list->len > 0) {
            
            QmiMessageNasGetRfBandInformationOutputListElement *el =
                &g_array_index(list, QmiMessageNasGetRfBandInformationOutputListElement, 0);
            
            /* if non LTE/5GNR (UMTS/GSM) */
            if (el->radio_interface != QMI_NAS_RADIO_INTERFACE_LTE && 
                el->radio_interface != QMI_NAS_RADIO_INTERFACE_5GNR &&
                el->active_channel != 0) {
                
                /* ARFCN non LTE */
                if (!info.has_arfcn) {
                    info.arfcn = el->active_channel;
                    info.has_arfcn = TRUE;
                }
            }
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_nas_get_rf_band_information_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * NAS — Get Serving System
 * ═══════════════════════════════════════════════════════════════ */
static void on_serving_system(QmiClientNas *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageNasGetServingSystemOutput *out =
        qmi_client_nas_get_serving_system_finish(nas, res, &err);

    if (out && qmi_message_nas_get_serving_system_output_get_result(out, NULL)) {
        QmiNasRegistrationState reg = 0;
        QmiNasAttachState cs = 0, ps = 0;
        QmiNasNetworkType net = 0;
        GArray *ifaces = NULL;

        if (qmi_message_nas_get_serving_system_output_get_serving_system(
                out, &reg, &cs, &ps, &net, &ifaces, NULL)) {
            info.reg_state = (gint)reg;
            if (ifaces && ifaces->len > 0) {
                QmiNasRadioInterface pri =
                    g_array_index(ifaces, QmiNasRadioInterface, 0);
                strncpy(info.mode, radio_to_mode(pri), sizeof(info.mode) - 1);
            }
        }

        const gchar *opname = NULL;
        guint16 mcc = 0, mnc = 0;
        if (qmi_message_nas_get_serving_system_output_get_current_plmn(
                out, &mcc, &mnc, &opname, NULL) && opname)
            strncpy(info.cops, opname, sizeof(info.cops) - 1);

        /* LTE TAC */
        guint16 tac_tmp = 0;
        if (qmi_message_nas_get_serving_system_output_get_lte_tac(
                out, &tac_tmp, NULL) && tac_tmp != 0xFFFF && tac_tmp != 0xFFFE) {
            info.lac = tac_tmp;
        } else {
            /* Fallback: LAC 3GPP */
            guint16 lac_tmp = 0;
            if (qmi_message_nas_get_serving_system_output_get_lac_3gpp(
                    out, &lac_tmp, NULL) && lac_tmp != 0xFFFE && lac_tmp != 0xFFFF) {
                info.lac = lac_tmp;
            }
        }

        guint32 cid32 = 0;
        if (qmi_message_nas_get_serving_system_output_get_cid_3gpp(out, &cid32, NULL)
                && cid32 != 0xFFFFFFFF)
            info.cid = cid32;
    }
    g_clear_error(&err);
    if (out) qmi_message_nas_get_serving_system_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Get IMSI
 * ═══════════════════════════════════════════════════════════════ */
static void on_imsi(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsUimGetImsiOutput *out =
        qmi_client_dms_uim_get_imsi_finish(dms, res, &err);

    if (out && qmi_message_dms_uim_get_imsi_output_get_result(out, NULL)) {
        const gchar *v = NULL;
        if (qmi_message_dms_uim_get_imsi_output_get_imsi(out, &v, NULL) && v) {
            strncpy(info.imsi, v, sizeof(info.imsi) - 1);
            info.has_imsi = TRUE;
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_dms_uim_get_imsi_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Get ICCID
 * ═══════════════════════════════════════════════════════════════ */
static void on_iccid(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsUimGetIccidOutput *out =
        qmi_client_dms_uim_get_iccid_finish(dms, res, &err);

    if (out && qmi_message_dms_uim_get_iccid_output_get_result(out, NULL)) {
        const gchar *v = NULL;
        if (qmi_message_dms_uim_get_iccid_output_get_iccid(out, &v, NULL) && v) {
            strncpy(info.iccid, v, sizeof(info.iccid) - 1);
            info.has_iccid = TRUE;
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_dms_uim_get_iccid_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Chip Temperature (sysfs)
 * ═══════════════════════════════════════════════════════════════ */
static void on_temp(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsGetPowerStateOutput *out =
        qmi_client_dms_get_power_state_finish(dms, res, &err);
    g_clear_error(&err);
    if (out) qmi_message_dms_get_power_state_output_unref(out);

    static const char *paths[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/class/thermal/thermal_zone2/temp",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        FILE *f = fopen(paths[i], "r");
        if (!f) continue;
        int val = 0;
        if (fscanf(f, "%d", &val) == 1 && val > 0) {
            info.chip_temp = (val > 1000) ? val / 1000 : val;
            info.has_temp  = TRUE;
            fclose(f);
            break;
        }
        fclose(f);
    }
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Get Firmware Version
 * ═══════════════════════════════════════════════════════════════ */
static void on_firmware(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsGetSoftwareVersionOutput *out =
        qmi_client_dms_get_software_version_finish(dms, res, &err);

    if (out && qmi_message_dms_get_software_version_output_get_result(out, NULL)) {
        const gchar *v = NULL;
        if (qmi_message_dms_get_software_version_output_get_version(out, &v, NULL) && v) {
            strncpy(info.firmware, v, sizeof(info.firmware) - 1);
            info.has_firmware = TRUE;
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_dms_get_software_version_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * run processes if DMS and NAS ready. Model recieved
 * ═══════════════════════════════════════════════════════════════ */
/* forward declaration */
static void on_manufacturer(QmiClientDms *client, GAsyncResult *res, gpointer ud);

static void fire_all_requests(void)
{
    pending = 11;

    qmi_client_dms_get_manufacturer(dms, NULL, 10, NULL,
        (GAsyncReadyCallback)on_manufacturer, NULL);
    qmi_client_dms_get_software_version(dms, NULL, 10, NULL,
        (GAsyncReadyCallback)on_firmware, NULL);
    qmi_client_dms_uim_get_iccid(dms, NULL, 10, NULL,
        (GAsyncReadyCallback)on_iccid, NULL);
    qmi_client_dms_uim_get_imsi(dms, NULL, 10, NULL,
        (GAsyncReadyCallback)on_imsi, NULL);
    qmi_client_dms_get_power_state(dms, NULL, 10, NULL,
        (GAsyncReadyCallback)on_temp, NULL);

    qmi_client_nas_get_serving_system(nas, NULL, 10, NULL,
        (GAsyncReadyCallback)on_serving_system, NULL);
    qmi_client_nas_get_signal_info(nas, NULL, 10, NULL,
        (GAsyncReadyCallback)on_signal_info, NULL);
    qmi_client_nas_get_signal_strength(nas, NULL, 10, NULL,
        (GAsyncReadyCallback)on_signal_strength, NULL);
    qmi_client_nas_get_cell_location_info(nas, NULL, 10, NULL,
        (GAsyncReadyCallback)on_cell_location, NULL);
    qmi_client_nas_get_lte_cphy_ca_info(nas, NULL, 10, NULL,
        (GAsyncReadyCallback)on_lte_ca, NULL);
    qmi_client_nas_get_rf_band_information(nas, NULL, 10, NULL,
        (GAsyncReadyCallback)on_rf_band_info, NULL);
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Get Manufacturer (merge of USB ID)
 * ═══════════════════════════════════════════════════════════════ */
static void on_manufacturer(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsGetManufacturerOutput *out =
        qmi_client_dms_get_manufacturer_finish(dms, res, &err);

    /* if no USB data use libQMI  */
    if (!info.has_model && out && qmi_message_dms_get_manufacturer_output_get_result(out, NULL)) {
        const gchar *v = NULL;
        if (qmi_message_dms_get_manufacturer_output_get_manufacturer(out, &v, NULL) && v) {
            /* Skip Qualcomm name */
            if (g_ascii_strncasecmp(v, "QUALCOMM", 8) != 0) {
                snprintf(info.model, sizeof(info.model), "%s", v);
                info.has_model = TRUE;
            }
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_dms_get_manufacturer_output_unref(out);
    check_done();
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Get Model: save model in USB ID and libQMI
 * ═══════════════════════════════════════════════════════════════ */
static void on_model(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsGetModelOutput *out =
        qmi_client_dms_get_model_finish(dms, res, &err);

    if (out && qmi_message_dms_get_model_output_get_result(out, NULL)) {
        const gchar *v = NULL;
        if (qmi_message_dms_get_model_output_get_model(out, &v, NULL) && v)
            strncpy(info.model, v, sizeof(info.model) - 1);
    }
    g_clear_error(&err);
    if (out) qmi_message_dms_get_model_output_unref(out);

    /* Read USB ID manufacturer and product */
    char usb_manufacturer[128] = {0};
    char usb_product[128] = {0};
    
    const char *dev_name = strrchr(dev_path, '/');
    if (dev_name)
        dev_name++;
    else
        dev_name = dev_path;
    
    read_usb_id(dev_name, usb_manufacturer, sizeof(usb_manufacturer), 
                usb_product, sizeof(usb_product));
    
    /* Former model at USB ID */
    if (usb_manufacturer[0] && usb_product[0]) {
        /* Check duplicate words manufacturer and product */
        if (g_ascii_strncasecmp(usb_product, usb_manufacturer, strlen(usb_manufacturer)) == 0) {
            /*  Only product, if manufacturer exist */
            snprintf(info.model, sizeof(info.model), "%s", usb_product);
        } else {
            snprintf(info.model, sizeof(info.model), "%s %s", 
                    usb_manufacturer, usb_product);
        }
        info.has_model = TRUE;
    } else if (usb_product[0]) {
        g_strlcpy(info.model, usb_product, sizeof(info.model));
        info.has_model = TRUE;
    } else if (usb_manufacturer[0]) {
        g_strlcpy(info.model, usb_manufacturer, sizeof(info.model));
        info.has_model = TRUE;
    }

    fire_all_requests();
}

/* ═══════════════════════════════════════════════════════════════
 * DMS — Get IDs (IMEI)
 * ═══════════════════════════════════════════════════════════════ */
static void on_ids(QmiClientDms *client, GAsyncResult *res, gpointer ud)
{
    (void)client; (void)ud;
    GError *err = NULL;
    QmiMessageDmsGetIdsOutput *out =
        qmi_client_dms_get_ids_finish(dms, res, &err);

    if (out && qmi_message_dms_get_ids_output_get_result(out, NULL)) {
        const gchar *v = NULL;
        if (qmi_message_dms_get_ids_output_get_imei(out, &v, NULL) && v) {
            strncpy(info.imei, v, sizeof(info.imei) - 1);
            info.has_imei = TRUE;
        }
    }
    g_clear_error(&err);
    if (out) qmi_message_dms_get_ids_output_unref(out);

    qmi_client_dms_get_model(dms, NULL, 10, NULL,
        (GAsyncReadyCallback)on_model, NULL);
}

/* ═══════════════════════════════════════════════════════════════
 * Open clients
 * ═══════════════════════════════════════════════════════════════ */
/* clients_ready: timer — if all (DMS+NAS) open */
static gint clients_ready = 0;

static void start_dms_chain(void)
{
    /* run DMS if all clients ready */
    clients_ready++;
    if (clients_ready == 2)
        qmi_client_dms_get_ids(dms, NULL, 10, NULL,
            (GAsyncReadyCallback)on_ids, NULL);
}

static void on_nas_client(QmiDevice *dev, GAsyncResult *res, gpointer ud)
{
    (void)dev; (void)ud;
    GError *err = NULL;
    QmiClient *c = qmi_device_allocate_client_finish(device, res, &err);
    if (!c) {
        fprintf(stderr, "NAS client error: %s\n", err ? err->message : "?");
        g_clear_error(&err);
        g_main_loop_quit(loop);
        return;
    }
    nas = QMI_CLIENT_NAS(c);
    start_dms_chain();
}

static void on_dms_client(QmiDevice *dev, GAsyncResult *res, gpointer ud)
{
    (void)dev; (void)ud;
    GError *err = NULL;
    QmiClient *c = qmi_device_allocate_client_finish(device, res, &err);
    if (!c) {
        fprintf(stderr, "DMS client error: %s\n", err ? err->message : "?");
        g_clear_error(&err);
        g_main_loop_quit(loop);
        return;
    }
    dms = QMI_CLIENT_DMS(c);

    /* open NAS */
    qmi_device_allocate_client(device, QMI_SERVICE_NAS,
        QMI_CID_NONE, 10, NULL,
        (GAsyncReadyCallback)on_nas_client, NULL);

    start_dms_chain();
}

static void try_open_device(void);  /* forward */

static void on_device_open(QmiDevice *dev, GAsyncResult *res, gpointer ud)
{
    (void)dev; (void)ud;
    GError *err = NULL;
    if (!qmi_device_open_finish(device, res, &err)) {
        if (device_mode == MODE_AUTO && open_retries == 0) {
            /* QMI failed in auto mode — retry with MBIM */
            g_clear_error(&err);
            open_retries++;
            try_open_device();
            return;
        }
        fprintf(stderr, "Open error: %s\n", err ? err->message : "?");
        g_clear_error(&err);
        g_main_loop_quit(loop);
        return;
    }

    /* Restore normal log handler after successful open in auto mode */
    if (device_mode == MODE_AUTO)
        g_log_set_handler("Qmi",
            G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
            g_log_default_handler, NULL);

    qmi_device_allocate_client(device, QMI_SERVICE_DMS,
        QMI_CID_NONE, 10, NULL,
        (GAsyncReadyCallback)on_dms_client, NULL);
}

/* Open device with flags depending on mode / retry state */
static void try_open_device(void)
{
    QmiDeviceOpenFlags flags = QMI_DEVICE_OPEN_FLAGS_PROXY;
    gboolean mbim;

    switch (device_mode) {
    case MODE_MBIM:
        mbim = TRUE;
        break;
    case MODE_QMI:
        mbim = FALSE;
        break;
    case MODE_AUTO:
    default:
        /* First attempt: QMI; after retry: MBIM */
        mbim = (open_retries > 0);
        break;
    }

    if (mbim) {
        flags |= QMI_DEVICE_OPEN_FLAGS_MBIM;
        //fprintf(stderr, "Opening as QMI over MBIM\n");
    } else {
        flags |= QMI_DEVICE_OPEN_FLAGS_NET_802_3 |
                 QMI_DEVICE_OPEN_FLAGS_NET_NO_QOS_HEADER;
    }

    qmi_device_open(device, flags, 15, NULL,
        (GAsyncReadyCallback)on_device_open, NULL);
}

static void on_device_new(GObject *src, GAsyncResult *res, gpointer ud)
{
    (void)src; (void)ud;
    GError *err = NULL;
    device = qmi_device_new_finish(res, &err);
    if (!device) {
        fprintf(stderr, "Device error: %s\n", err ? err->message : "?");
        g_clear_error(&err);
        g_main_loop_quit(loop);
        return;
    }
    try_open_device();
}

/* ═══════════════════════════════════════════════════════════════
 * Silent log handler — suppresses GLib/QMI warnings
 * ═══════════════════════════════════════════════════════════════ */
static void log_silent(const gchar    *domain,
                       GLogLevelFlags  level,
                       const gchar    *message,
                       gpointer        user_data)
{
    (void)domain; (void)level; (void)message; (void)user_data;
    /* intentionally empty */
}

/* ═══════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device> [--json] [--qmi|--mbim]\n", argv[0]);
        fprintf(stderr, "  --json   JSON output\n");
        fprintf(stderr, "  --qmi    force native QMI mode\n");
        fprintf(stderr, "  --mbim   force QMI over MBIM mode\n");
        fprintf(stderr, "  (default: auto-detect, try QMI first)\n");
        fprintf(stderr, "Example: %s /dev/cdc-wdm0 --json\n", argv[0]);
        fprintf(stderr, "         %s /dev/cdc-wdm0 --mbim --json\n", argv[0]);
        return EXIT_FAILURE;
    }

    dev_path    = argv[1];
    json_mode   = FALSE;
    device_mode = MODE_AUTO;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0)
            json_mode = TRUE;
        else if (strcmp(argv[i], "--mbim") == 0)
            device_mode = MODE_MBIM;
        else if (strcmp(argv[i], "--qmi") == 0)
            device_mode = MODE_QMI;
    }

    memset(&info, 0, sizeof(info));
    info.chip_temp = -999;
    info.csq       = -1;
    info.is_umts   = FALSE;

    /* In auto-detect mode, suppress QMI warnings that are expected
     * when probing QMI on an MBIM-only device:
     *   "requested QMI mode but unexpected transport type found"
     *   "Cannot read from istream: connection broken" */
    if (device_mode == MODE_AUTO) {
        g_log_set_handler("Qmi",
            G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL,
            log_silent, NULL);
    }

    loop = g_main_loop_new(NULL, FALSE);

    GFile *f = g_file_new_for_path(dev_path);
    qmi_device_new(f, NULL, (GAsyncReadyCallback)on_device_new, NULL);
    g_object_unref(f);

    g_main_loop_run(loop);

    /* destroy sesion do_release_clients() */
    if (device) {
        g_object_unref(device);
        device = NULL;
    }
    g_main_loop_unref(loop);

    return EXIT_SUCCESS;
}
