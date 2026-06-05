#include "stringify.h"

#include "sbs.h"
#include "sdk.h"

#include <stdio.h>
#include <string.h>

// NOTE: we should mainly stick to only converting enum to str, not the other way around. We can use the var list to
// store string to enum conversion.

const char* errorcodes_to_string(CredoError_t err) {
    switch (err) {
        CASE_ENUM_RETURN_STRING(CR_OK);
        CASE_ENUM_RETURN_STRING(CR_FAIL);
        CASE_ENUM_RETURN_STRING(CR_PHY_FW_DOWNLOAD_FAIL);
        CASE_ENUM_RETURN_STRING(CR_FW_TIMEOUT);
        CASE_ENUM_RETURN_STRING(CR_NOT_READY);
        CASE_ENUM_RETURN_STRING(CR_OUT_OF_MEMORY);
        CASE_ENUM_RETURN_STRING(CR_HAL_LOAD_FAIL);
        CASE_ENUM_RETURN_STRING(CR_HAL_NOT_FOUND);
        CASE_ENUM_RETURN_STRING(CR_INVALID_ARGS);
        CASE_ENUM_RETURN_STRING(CR_MUTEX_TIMEOUT);
        CASE_ENUM_RETURN_STRING(CR_UNSUPPORTED);
        CASE_ENUM_RETURN_STRING(CR_NOTIMPLEMENTED);
        default:
            return "UNKNOWN_ERROR";
    }
}

const char* prbs_pattern_to_string(CredoLanePrbsPattern_t mode) {
    switch (mode) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS7, "PRBS7");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS9, "PRBS9");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS11, "PRBS11");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS13, "PRBS13");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS15, "PRBS15");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS23, "PRBS23");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS31, "PRBS31");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS19, "PRBS19");
        default:
            CASE_ENUM_RETURN_STRING(CR_PRBS_UNKNOWN);
    }
}

const char* lane_mode_to_string(CredoLaneMode_t mode) {
    switch (mode) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_LMODE_OFF, "OFF");
        CASE_ENUM_RETURN_STRING_BASIC(CR_LMODE_PAM4, "PAM4");
        CASE_ENUM_RETURN_STRING_BASIC(CR_LMODE_NRZ, "NRZ");
        CASE_ENUM_RETURN_STRING_BASIC(CR_LMODE_AN, "AN");
        CASE_ENUM_RETURN_STRING_BASIC(CR_LMODE_PAM3, "PAM3");
        CASE_ENUM_RETURN_STRING_BASIC(CR_LMODE_DISABLE, "DISABLE");
        default:
            return "-";
    }
}

const char* fec_to_string(CredoFecType_t fec) {
    switch (fec) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_NONE, "OFF");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_FIRE_CODE, "Fire Code");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_RS_528, "RS(528)");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_RS_544, "RS(544)");
        default:
            return "-";
    }
}

CredoError_t port_flags_to_string(uint32_t flags, char* buf, uint32_t size) {
    char* b = buf;
    b[0] = '\0';
    if (flags & CR_PFLAG_LINE_SIDE_OPTICAL) {
        b += snprintf(b, size - (b - buf), "LINE_SIDE_OPTICAL,");
    }
    if (flags & CR_PFLAG_SYS_SIDE_OPTICAL) {
        b += snprintf(b, size - (b - buf), "SYS_SIDE_OPTICAL,");
    }

    if ((flags & CR_PFLAG_LINE_SIDE_LT) == CR_PFLAG_LINE_SIDE_LT) {
        b += snprintf(b, size - (b - buf), "LINE_SIDE_LT,");
    } else if (flags & CR_PFLAG_LINE_SIDE_AN) {
        b += snprintf(b, size - (b - buf), "LINE_SIDE_AN,");
    } else if (flags & CR_PFLAG_LINE_SIDE_ANLT) {
        b += snprintf(b, size - (b - buf), "LINE_SIDE_ANLT,");
    } else {
    }

    if (flags & CR_PFLAG_SYS_SIDE_LT) {
        b += snprintf(b, size - (b - buf), "SYS_SIDE_LT,");
    }
    if (flags & CR_PFLAG_AUTONEG_OVERRIDE) {
        b += snprintf(b, size - (b - buf), "AUTONEG_OVERRIDE,");
    }
    if (flags & CR_PFLAG_ENABLE_DOUBLE_CRC) {
        b += snprintf(b, size - (b - buf), "ENABLE_DOUBLE_CRC,");
    }

    if (strlen(buf) != 0) {
        buf[strlen(buf) - 1] = '\0';
    }

    return CR_OK;
}

void ppm_to_format_string(int32_t ppm, char ppm_str[8]) {
    if (ppm > 999) {
        snprintf(ppm_str, 8, "%s", "+###");
    } else if (ppm < -999) {
        snprintf(ppm_str, 8, "%s", "-###");
    } else {
        snprintf(ppm_str, 8, "%d", ppm);
    }
}

void seconds_to_format_string(unsigned sec, char time_str[16]) {
    if (sec == 0) {
        time_str[0] = '-';
        time_str[1] = '\0';
        return;
    }

    unsigned h = sec / 3600;
    unsigned m = (sec % 3600) / 60;
    unsigned s = (sec % 3600) % 60;

    sbs* t_str = SBS64("");

    if (h) {
        sbscatprintf(t_str, "%u:", h);
    }
    if (h || m) {
        sbscatprintf(t_str, "%02u:", m);
    }
    sbscatprintf(t_str, "%02u", s);
    snprintf(time_str, 15, "%s", sbsstr(t_str));
}

void stringify_firmware_version(unsigned version, char version_str[32]) {
    bool test_fw = (version >> 23) & 0x1;
    char branch = ((version >> 19) & 0xF);
    unsigned major = (version >> 16) & 0x7;
    unsigned minor = (version >> 8) & 0xFF;
    unsigned subpatch = (version >> 4) & 0xF;
    unsigned patch = (version >> 0) & 0xF;

    // make it backwards compatible with older chip firmwares
    if (branch == 0) {
        patch = (subpatch << 4) | patch;
    }

    sbs* fw_version = SBS64("v");

    if (test_fw) {
        sbscat(fw_version, "t");
    }
    sbscatprintf(fw_version, "%u.%u.%u", major, minor, patch);
    if (branch > 0) {
        sbscatprintf(fw_version, ".%c%u", 'A' + branch - 1, subpatch);
    }
    snprintf(version_str, 31, "%s", sbsstr(fw_version));
}

const char* stringify_tx_state(CredoLaneTxState_t state) {
    switch (state) {
        case CR_TX_LOWPOWER:
            return "LOW-POWER";
        case CR_TX_TRAFFIC:
            return "TRAFFIC";
        case CR_TX_SQUELCH:
            return "SQUELCH";
        case CR_TX_PRBS_PAM4:
            return "PRBS-PAM4";
        case CR_TX_PRBS_NRZ:
            return "PRBS-NRZ";
        case CR_TX_FORCE_DISABLE:
            return "fDISABLE";
        case CR_TX_FORCE_PRBSS_PAM4:
            return "fPRBS-PAM4";
        case CR_TX_FORCE_PRBS_NRZ:
            return "fPRBS-NRZ";
        case CR_TX_FORCE_TRAFFIC:
            return "fTRAFFIC";
        case CR_TX_FORCE_TEST_PATT:
            return "fTEST-PATT";
        default:
            return "?";
    }
}

const char* stringify_tx_loopback_mode(CredoLaneLoopbackMode_t mode) {
    switch (mode) {
        case CR_LB_DISABLED:
            return "DISABLED";
        case CR_LB_TX_TO_RX:
            return "TX -> RX";
        case CR_LB_RX_TO_TX:
            return "RX -> TX";
        default:
            return "?";
    }
}

static const char* host_lanes[MAX_CREDO_LANES] = {
    "A0",  "A1",  "A2",  "A3",  "A4",  "A5",  "A6",  "A7",  "A8",  "A9",  "A10", "A11", "A12", "A13", "A14", "A15",
    "A16", "A17", "A18", "A19", "A20", "A21", "A22", "A23", "A24", "A25", "A26", "A27", "A28", "A29", "A30", "A31",
};

static const char* line_lanes[MAX_CREDO_LANES] = {
    "B0",  "B1",  "B2",  "B3",  "B4",  "B5",  "B6",  "B7",  "B8",  "B9",  "B10", "B11", "B12", "B13", "B14", "B15",
    "B16", "B17", "B18", "B19", "B20", "B21", "B22", "B23", "B24", "B25", "B26", "B27", "B28", "B29", "B30", "B31",
};

const char* stringify_lane_id(CredoSlice_t* slice, int lane) {
    if (lane < 0 || lane >= MAX_CREDO_LANES) return "--";
    CredoDeviceType_t devtype = slice->device->desc->device_type;
    switch (devtype) {
        case CREDO_OWL_800:
        case CREDO_OSPREY_800:
            if (lane < 0 || lane > 19 || (lane > 7 && lane < 12)) return "--";
            return (lane < 8) ? host_lanes[lane] : line_lanes[lane - 12];
        case CREDO_HERON_1P0:
        case CREDO_HERON_MR: {
            int max_host_lane = slice->desc->lane_count >> 1;
            if (lane < 0 || lane >= slice->desc->lane_count) return "--";
            return (lane < max_host_lane) ? line_lanes[lane] : host_lanes[lane - max_host_lane];
        }

        default: {
            int max_host_lane = slice->desc->lane_count >> 1;
            int max_line_lane;

            hal_get_lane_count(slice, &max_host_lane, &max_line_lane);
            if (lane < 0 || lane >= slice->desc->lane_count) return "--";
            return (lane < max_host_lane) ? host_lanes[lane] : line_lanes[lane - max_host_lane];
        }
    }
}

// assume lane list is valid and contain 1 element at least
void stringify_lane_list(int* lane_list, unsigned count, char lane_list_str[32]) {
    bool is_counting = false;
    int counting_val = -1;

    sbs* buf = SBS64("");
    sbscatprintf(buf, "%d", lane_list[0]);
    for (int i = 1; i < count; i++) {
        if (lane_list[i] < 0) {
            is_counting = false;
            break;
        }

        if (lane_list[i] == lane_list[i - 1] + 1 || lane_list[i] == lane_list[i - 1] - 1) {
            is_counting = true;
            counting_val = lane_list[i];
            continue;
        }

        if (is_counting) {
            sbscatprintf(buf, "-%d", counting_val);
        }

        sbscatprintf(buf, ",%d", lane_list[i]);
        is_counting = false;
    }

    if (is_counting) {
        sbscatprintf(buf, "-%d", counting_val);
    }

    snprintf(lane_list_str, 31, "%s", sbsstr(buf));
}
