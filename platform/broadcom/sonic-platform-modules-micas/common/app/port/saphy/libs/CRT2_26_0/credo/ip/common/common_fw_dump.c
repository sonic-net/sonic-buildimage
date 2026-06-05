#include "project.h"

#include "common/common_firmware.h"
#include "common/common_fw_dump.h"

#include "sbs.h"
#include "stringify.h"
#include "sdk.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define BSIZE 512

typedef CredoError_t (*dump_type_fp)(CredoSlice_t* slice, int lane, dump_info_t* dump, const DisplayState_t* D);
typedef CredoError_t (*dump_format_fp)(dump_info_t* dump, unsigned* data, unsigned count, char* buffer);

const char* fw_errorcodes_to_string(unsigned errorcodes) {
#define CASE_FW_ERROR_RETURN_STRING(err) CASE_ENUM_RETURN_STRING_BASIC(ERROR_##err, #err)
    unsigned fw_errorcodes_common = errorcodes & 0xFF;
    switch (fw_errorcodes_common) {
        CASE_FW_ERROR_RETURN_STRING(UNKNOWN_COMMAND);
        CASE_FW_ERROR_RETURN_STRING(INVALID_INPUT);
        CASE_FW_ERROR_RETURN_STRING(PHY_NOT_READY);
        CASE_FW_ERROR_RETURN_STRING(EM_NOT_READY);
        CASE_FW_ERROR_RETURN_STRING(EM_GOING_ON);
        CASE_FW_ERROR_RETURN_STRING(EM_CANCELLED);
        CASE_FW_ERROR_RETURN_STRING(EM_NOT_STARTED);
        CASE_FW_ERROR_RETURN_STRING(OUT_OF_MEMORY);
        CASE_FW_ERROR_RETURN_STRING(FIRMWARE_BUSY);
        CASE_FW_ERROR_RETURN_STRING(INVALID_INDEX);
        CASE_FW_ERROR_RETURN_STRING(INVALID_MODE);
        CASE_FW_ERROR_RETURN_STRING(INVALID_SECTION);
        CASE_FW_ERROR_RETURN_STRING(FW_FREEZE);
        CASE_FW_ERROR_RETURN_STRING(LANE_MAPPING);
        CASE_FW_ERROR_RETURN_STRING(LANE_OUT_OF_RANGE);
        CASE_FW_ERROR_RETURN_STRING(LANE_USED);
        CASE_FW_ERROR_RETURN_STRING(MODE);
        CASE_FW_ERROR_RETURN_STRING(MODE_OUT_OF_RANGE);
        CASE_FW_ERROR_RETURN_STRING(NOT_IMPLEMENTED);
        CASE_FW_ERROR_RETURN_STRING(PORT_USED);
        CASE_FW_ERROR_RETURN_STRING(UNSUPPORT_AN_SPEED);
        CASE_FW_ERROR_RETURN_STRING(UNSUPPORT_LINE_ANLT);
        CASE_FW_ERROR_RETURN_STRING(UNSUPPORT_SYS_ANLT);
        CASE_FW_ERROR_RETURN_STRING(OUT_OF_RANGE);
        CASE_FW_ERROR_RETURN_STRING(UNSUPPORT_NRZ_LTONLY);
        CASE_FW_ERROR_RETURN_STRING(PORT_OUT_OF_RANGE);
        CASE_FW_ERROR_RETURN_STRING(VSENSOR_BUSY);
        CASE_FW_ERROR_RETURN_STRING(CODE_MAX);
        default:
            break;
    }

    return "UNIDENTIFIED_ERROR";
}

static CredoError_t common_print_unsigned(dump_info_t* dump, unsigned* data, unsigned count, char* buffer) {
    sbs* b = SBSFROMBUF("", buffer, BSIZE);
    for (int i = 0; i < count; i++) {
        sbscatprintf(b, "%u ", data[i]);
    }
    return CR_OK;
}

static CredoError_t common_print_unsigned_hex(dump_info_t* dump, unsigned* data, unsigned count, char* buffer) {
    sbs* b = SBSFROMBUF("", buffer, BSIZE);
    for (int i = 0; i < count; i++) {
        sbscatprintf(b, "0x%04X ", data[i]);
    }
    return CR_OK;
}

static CredoError_t common_print_unsigned_split(dump_info_t* dump, unsigned* data, unsigned count, char* buffer) {
    sbs* b = SBSFROMBUF("", buffer, BSIZE);
    int width = dump->fmt_arg1;
    bool reverse = dump->fmt_arg2;

    for (int i = 0; i < count; i++) {
        unsigned v1 = (reverse == true) ? (data[i] & ((1 << width) - 1)) : (data[i] >> width);
        unsigned v2 = (reverse == true) ? (data[i] >> width) : (data[i] & ((1 << width) - 1));
        sbscatprintf(b, "%u,%u  ", v1, v2);
    }

    return CR_OK;
}

static CredoError_t common_print_signed(dump_info_t* dump, unsigned* data, unsigned count, char* buffer) {
    sbs* b = SBSFROMBUF("", buffer, BSIZE);
    int val;

    for (int i = 0; i < count; i++) {
        val = (data[i] & 0x8000) ? (int)data[i] - 0x10000 : data[i];
        sbscatprintf(b, "%d ", val);
    }

    return CR_OK;
}

static CredoError_t common_print_signed_div(dump_info_t* dump, unsigned* data, unsigned count, char* buffer) {
    sbs* b = SBSFROMBUF("", buffer, BSIZE);
    int division = dump->fmt_arg1;
    bool is_float = dump->fmt_arg2;

    for (int i = 0; i < count; i++) {
        int val = (data[i] & 0x8000) ? (int)data[i] - 0x10000 : data[i];

        if (is_float) {
            sbscatprintf(b, "%.2f ", (double)val / division);
        } else {
            sbscatprintf(b, "%d ", val / division);
        }
    }

    return CR_OK;
}

static CredoError_t common_print_float(dump_info_t* dump, unsigned* data, unsigned count, char* buffer) {
    sbs* b = SBSFROMBUF("", buffer, BSIZE);
    double division = (1 << dump->fmt_arg1);

    for (int i = 0; i < count; i++) {
        sbscatprintf(b, "%5.2f ", data[i] / division);
    }

    return CR_OK;
}

static dump_format_fp common_fw_dump_format_table[DUMP_FORMAT_MAX] = {
    common_print_unsigned, common_print_unsigned_hex, common_print_unsigned_split,
    common_print_signed,   common_print_signed_div,   common_print_float,
};

static CredoError_t common_print_list(CredoSlice_t* slice, int lane, dump_info_t* dump, const DisplayState_t* D) {
    char buffer[BSIZE];
    unsigned* param = (unsigned*)malloc(dump->len * sizeof(unsigned));

    for (int i = 0; i < dump->len; i++) {
        if (hal_fw_debug_cmd(slice, lane, dump->section, dump->index + i, param + i) != CR_OK) {
            free(param);
            return CR_FAIL;
        }
    }

    if (common_fw_dump_format_table[dump->fmt] != NULL) {
        common_fw_dump_format_table[dump->fmt](dump, param, dump->len, buffer);
        DISPF(D, "%s: %s\n", dump->name, buffer);
    }

    free(param);
    return CR_OK;
}

static CredoError_t common_print_matrix(CredoSlice_t* slice, int lane, dump_info_t* dump, const DisplayState_t* D) {
    char buffer[BSIZE];
    int row = dump->type_arg1;
    int col = dump->len / row;
    unsigned* param = (unsigned*)malloc(dump->len * sizeof(unsigned));

    DISPF(D, "%s:\n", dump->name);
    for (int i = 0; i < dump->len; i++) {
        if (hal_fw_debug_cmd(slice, lane, dump->section, dump->index + i, param + i) != CR_OK) {
            free(param);
            return CR_FAIL;
        }
    }

    for (int i = 0; i < row; i++) {
        common_fw_dump_format_table[dump->fmt](dump, param + (i * col), col, buffer);
        DISPF(D, "     %s\n", buffer);
    }

    free(param);
    return CR_OK;
}

static CredoError_t common_print_timers(CredoSlice_t* slice, int lane, dump_info_t* dump, const DisplayState_t* D) {
    CredoError_t ret = CR_FAIL;
    unsigned param;
    unsigned last_timer = 0, total = 0;
    const char** timer_name = dump->timer_name;

    DISPF(D, "%s:\n", dump->name);
    for (int i = 0; i < dump->len; i++) {
        ret = hal_fw_debug_cmd(slice, lane, dump->section, dump->index + i, &param);
        if (param == 0 || ret != CR_OK) continue;

        if (strlen(timer_name[i]) != 0 && timer_name[i][0] != '#') {
            total += (param - last_timer);
            DISPF(D, "      %s: %d\n", timer_name[i], param - last_timer);
        }

        last_timer = param;
    }
    DISPF(D, "      total: %d\n", total);

    return CR_OK;
}

static dump_type_fp common_fw_dump_type_table[DUMP_TYPE_MAX] = {
    common_print_list,
    common_print_matrix,
    common_print_timers,
};

CredoError_t common_fw_dump_debug(CredoSlice_t* slice, int lane, dump_info_t* dump_list, unsigned count,
                                  const DisplayState_t* D) {
    for (int i = 0; i < count; i++) {
        dump_info_t* dump = &dump_list[i];
        if (common_fw_dump_type_table[dump->type] != NULL) {
            common_fw_dump_type_table[dump->type](slice, lane, dump, D);
        }
    }
    return CR_OK;
}
