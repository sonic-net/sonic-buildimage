#include "project.h"

#include "common/common_firmware.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

CredoError_t common_fw_notepad(CredoSlice_t* slice, unsigned* notepad_address) {
    *notepad_address = REG_DATA;
    return CR_OK;
}

static CredoError_t common_fw_cmd_safty_dump_and_retry(CredoSlice_t* slice, unsigned cmd, unsigned param,
                                                       unsigned* response) {
    if (slice->sdk->max_log_level < CR_LOG_TRACE) return CR_FW_TIMEOUT;

    unsigned val = 0;
    ERR_PROPS(readReg(slice, REG_CMD, &val));
    if (val != cmd) {
        LOGS_ERROR("[Firmware command] cmd read back dismatch 0x%04x", val);
    }
    ERR_PROPS(readReg(slice, REG_CMD_DETAIL, &val));
    if (val != param) {
        LOGS_ERROR("[Firmware command] detail read back dismatch 0x%04x", val);
    }

    unsigned fw_cmd = 0, fw_detail_1 = 0, fw_detail_2 = 0;
    ERR_PROPS(readTop(slice, REG_DATA + 0, &fw_cmd));
    ERR_PROPS(readTop(slice, REG_DATA + 1, &fw_detail_1));
    ERR_PROPS(readTop(slice, REG_DATA + 2, &fw_detail_2));
    LOGS_ERROR("[Firmware command] fw raw info: 0x%04x, 0x%04x, 0x%04x", fw_cmd, fw_detail_1, fw_detail_2);

    val = 0;
    hal_get_top_pll_cap(slice, &val);
    LOGS_ERROR("[Firmware command] Top pll cap: 0x%04x", val);

#ifdef REG_FW_WATCHDOG_TIMER
    unsigned watchdog_prev = 0, watchdog = 0;
    ERR_PROPS(readReg(slice, REG_FW_WATCHDOG_TIMER, &watchdog_prev));
    LOGS_ERROR("[Firmware command] watchdog value before sleep 5s: %u", watchdog_prev);
    sleep_ms(5000);
    ERR_PROPS(readReg(slice, REG_FW_WATCHDOG_TIMER, &watchdog));
    LOGS_ERROR("[Firmware command] watchdog value after sleep 5s: %u", watchdog);
    if (watchdog_prev == watchdog) {
        LOGS_ERROR("[Firmware command] firmware DIED");
        return CR_FW_TIMEOUT;
    }
#endif

    // if firmware is alive, try again with longer timeout
    unsigned new_timeout = slice->data->fw_cmd_timeout * 2;
    unsigned r;
    LOGS_ERROR("[Firmware command] firmware alive, trying double cmd timeout to %u us.", new_timeout);
    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL, param));
    ERR_PROPS(writeReg(slice, REG_CMD, cmd));
    if (common_wait_fw_cmd(slice, cmd, &r, new_timeout) != CR_FW_TIMEOUT) {
        LOGS_ERROR("[Firmware command] Retry succeeded, please change timeout to %u us.", new_timeout);
        return CR_OK;
    }

    // still fail, wait long period of time then retry again
    LOGS_ERROR("[Firmware command] still timeout, wait 10s...and try again with timeout 10s");
    sleep_ms(10000);
    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL, param));
    ERR_PROPS(writeReg(slice, REG_CMD, cmd));
    if (common_wait_fw_cmd(slice, cmd, &r, 10000000) == CR_FW_TIMEOUT) {
        return CR_FW_TIMEOUT;
    }

    *response = r;
    return CR_OK;
}

CredoError_t common_wait_fw_cmd(CredoSlice_t* slice, unsigned cmd, unsigned* response, unsigned cmd_timeout) {
    CredoTime_t start_time;
    get_time(&start_time);
    while (!is_timeout(&start_time, cmd_timeout)) {
        ERR_PROPS(readReg(slice, REG_CMD, response));
        if (*response != cmd) break;
    }
    // try 1 more time in case some contention happens in driver code or driver code is quite slow to read
    if (*response == 0xFFFF || *response == cmd) {
        ERR_PROPS(readReg(slice, REG_CMD, response));
    }

    return (*response == 0xFFFF || *response == cmd) ? CR_FW_TIMEOUT : CR_OK;
}

CredoError_t common_fw_cmd_send(CredoSlice_t* slice, unsigned cmd, unsigned param) {
    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL, param));
    ERR_PROPS(writeReg(slice, REG_CMD, cmd | FW_RESPONSE_MASK));
    return CR_OK;
}

CredoError_t common_fw_cmd_response(CredoSlice_t* slice, unsigned cmd, unsigned param, unsigned* response,
                                    unsigned* response_param) {
    unsigned r = 0;
    if (common_wait_fw_cmd(slice, cmd, &r, slice->data->fw_cmd_timeout) == CR_FW_TIMEOUT) {
        LOGS_ERROR("[Firmware command] Timeout, command: 0x%04x, param: 0x%04x", cmd, param);
        return CR_FW_TIMEOUT;
    }

    if (response) *response = r;
    if (response_param) {
        ERR_PROPS(readReg(slice, REG_CMD_DETAIL, response_param));
    }

    if ((r & FW_RESPONSE_MASK) == FW_RESPONSE_ERROR) {
        LOGS_ERROR("[Firmware command] error: 0x%03x(%s), command: 0x%04x, param: 0x%04x", r,
                   fw_errorcodes_to_string(r), cmd, param);
    }

    return CR_OK;
}

CredoError_t common_fw_cmd(CredoSlice_t* slice, unsigned cmd, unsigned param, unsigned* response,
                           unsigned* response_param) {
    bool is_log_silent = response_param && (*response_param & 0xFFFF0000) == FW_CMD_LOG_SILENT ? true : false;
    unsigned r = (is_log_silent) ? FW_CMD_LOG_SILENT : 0;

    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL, param));
    ERR_PROPS(writeReg(slice, REG_CMD, cmd));
    if (common_wait_fw_cmd(slice, cmd, &r, slice->data->fw_cmd_timeout) == CR_FW_TIMEOUT) {
        if (is_log_silent) {
            return CR_FW_TIMEOUT;
        } else {
            LOGS_ERROR("[Firmware command] Timeout, command: 0x%04x, param: 0x%04x", cmd, param);
            if (common_fw_cmd_safty_dump_and_retry(slice, cmd, param, &r) == CR_FW_TIMEOUT) {
                return CR_FW_TIMEOUT;
            }
        }
    }

    if (response) *response = r;
    if (response_param) {
        ERR_PROPS(readReg(slice, REG_CMD_DETAIL, response_param));
    }

    if ((r & FW_RESPONSE_MASK) == FW_RESPONSE_ERROR) {
        if (!is_log_silent) {
            unsigned fw_cmd_addr = 0, fw_cmd = 0, fw_detail_1 = 0, fw_detail_2 = 0;
            if (hal_fw_get_raw_cmd_address(slice, &fw_cmd_addr) != CR_OK) {
                fw_cmd_addr = REG_DATA + 15;
            }
            ERR_PROPS(readTop(slice, fw_cmd_addr, &fw_cmd));
            ERR_PROPS(readReg(slice, REG_CMD_DETAIL, &fw_detail_1));
#ifdef REG_CMD_DETAIL2
            ERR_PROPS(readReg(slice, REG_CMD_DETAIL2, &fw_detail_2));
#endif
            LOGS_ERROR(
                "[Firmware command] error: 0x%03x(%s), command: 0x%04x, param: 0x%04x, fw raw info: 0x%04x, "
                "0x%04x, 0x%04x",
                r, fw_errorcodes_to_string(r), cmd, param, fw_cmd, fw_detail_1, fw_detail_2);
        }
        return CR_FAIL;
    }
    return CR_OK;
}

CredoError_t common_fw_cmd_ex(CredoSlice_t* slice, unsigned cmd, unsigned param1, unsigned param2, unsigned* response,
                              unsigned* response_param1, unsigned* response_param2) {
#ifdef REG_CMD_DETAIL2
    CredoError_t ret;
    bool is_log_silent = response_param1 && (*response_param1 & 0xFFFF0000) == FW_CMD_LOG_SILENT ? true : false;

    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL2, param2));
    if ((ret = hal_fw_cmd(slice, cmd, param1, response, response_param1)) != CR_OK) {
        if (!is_log_silent) LOGS_ERROR("[Firmware command] param2: 0x%04x", param2);
        return ret;
    }
    if (response_param2) {
        ERR_PROPS(readReg(slice, REG_CMD_DETAIL2, response_param2));
    }
    return CR_OK;
#else
    return CR_NOTIMPLEMENTED;
#endif
}

/* For internal use */
CredoError_t common_fw_info_data(CredoSlice_t* slice, int length, int data[]) {
    unsigned temp[16] = {0};
    unsigned reg_data_base = REG_DATA;

    if (length > 16) {
        LOGS_ERROR("[fw_info_data] Incorrect reading length: %d>16", length);
        return CR_FAIL;
    }
    ERR_PROPS(cr_slice_burst_read(slice, reg_data_base, temp, length));
    for (int i = 0; i < length; i++) {
        data[i] = temp[i] & 0x8000 ? (int)temp[i] - 0x10000 : (int)temp[i];
    }
    return CR_OK;
}

CredoError_t common_fw_info_data_unsigned(CredoSlice_t* slice, int length, unsigned data[]) {
    unsigned reg_data_base = REG_DATA;

    if (length > 16) {
        LOGS_ERROR("[fw_info_data] Incorrect reading length: %d>16", length);
        return CR_FAIL;
    }
    return cr_slice_burst_read(slice, reg_data_base, data, length);
}
