#include "sbs.h"
#include "sdk.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CASE_ENUM_RETURN_STRING_BASIC(enum, string) \
    case enum:                                      \
        return string
#define CASE_ENUM_RETURN_STRING(enum) CASE_ENUM_RETURN_STRING_BASIC(enum, #enum)

const char* strify_timestamp_now(char buffer[64]) {
    time_t rawtime;
    time(&rawtime);
    struct tm time;
    localtime_r(&rawtime, &time);
    strftime(buffer, 64, "%Y-%m-%dT%H:%M:%SZ", &time);
    return buffer;
}

const char* strify_time_iso8601(double unix_time, char buffer[64]) {
    time_t rawtime = (time_t)(long)(unix_time);
    double sec, ms;
    ms = modf(unix_time, &sec);
    struct tm time;
    localtime_r(&rawtime, &time);
    size_t n = strftime(buffer, 64, "%H:%M:%S", &time);
    n += snprintf(buffer + n, 64 - n, ".%03u", (unsigned)(ms * 1000));
    strftime(buffer + n, 64 - n, "%z", &time);
    return buffer;
}

const char* strify_timedelta(double seconds, bool show_ms, char buffer[64]) {
    bool is_neg = seconds < 0;
    seconds = fabs(seconds);
    uint64_t sec = (uint64_t)seconds;
    unsigned d = (sec / 86400);
    unsigned h = (sec % 86400) / 3600;
    unsigned m = (sec % 3600) / 60;
    unsigned s = (sec % 60);
    double mili = fmod(seconds, 1.0);
    sbs* t_str = SBS128("");
    if (is_neg) {
        sbscat(t_str, "-");
    }
    if (d) {
        sbscatprintf(t_str, "%ud", d);
    }
    if (d || h) {
        sbscatprintf(t_str, "%u:", h);
    }
    if (d || h || m) {
        sbscatprintf(t_str, "%02u:", m);
    }
    sbscatprintf(t_str, "%02u", s);
    if (show_ms) {
        sbscatprintf(t_str, ".%03u", (unsigned)(mili * 1000));
    }
    snprintf(buffer, 64, "%s", sbsstr(t_str));
    return buffer;
}

const char* strify_fec_error_type(CredoFecErrorType_t error_type) {
    switch (error_type) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_ERROR_BIT, "bit");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_ERROR_FRAME, "frame");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_ERROR_SYMBOL, "symbol");
    }
    return "?";
}

void strify_firmware_version(unsigned version, char version_str[32]) {
    bool test_fw = (version >> 23) & 0x1;
    char branch = (char)((version >> 19) & 0xF);
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
    snprintf(version_str, 32, "%s", sbsstr(fw_version));
}

const char* strify_port_connection_mode(CredoPortConnectionMode_t mode) {
    switch (mode) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_PORT_RETIMER, "retimer");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PORT_BITMUX, "bitmux");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PORT_GEARBOX, "gearbox");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PORT_SWITCHOVER_RETIMER, "switchover_retimer");
    }
    return "?";
}

const char* strify_port_mode(CredoPortMode_t mode) {
    switch (mode) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_PMODE_SERDES, "serdes");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PMODE_PCS, "pcs");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PMODE_MACSEC, "macsec");
    }
    return "?";
}

const char* strify_prbs_training_rx_status(CredoPrbsTrainingStatus_t status) {
    switch (status) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS_TRAINING_ERROR, "error");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS_TRAINING_LINKED, "linked");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS_TRAINING_OPTIMIZING, "optimizing");
        CASE_ENUM_RETURN_STRING_BASIC(CR_PRBS_TRAINING_RELINK, "relink");
    }
    return "?";
}

const char* strify_fec_type(CredoFecType_t fec_type) {
    switch (fec_type) {
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_NONE, "None");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_FIRE_CODE, "fire_code");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_RS_528, "rs_528");
        CASE_ENUM_RETURN_STRING_BASIC(CR_FEC_RS_544, "rs_544");
    }
    return "?";
}
