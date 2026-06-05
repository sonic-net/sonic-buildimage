#include "project.h"

#include "common/common_firmware.h"

#include "sdk.h"

CredoError_t common_fw_ver(CredoSlice_t* slice, unsigned* version) {
    unsigned cmd;
    unsigned response;
    unsigned response_param;

    cmd = FW_CMD_VER_READ;
    // ERR_PROPS( firmware_cmd(slice, cmd, 0, &response, &response_param));
    ERR_PROPS(hal_fw_cmd(slice, cmd, 0, &response, &response_param));
    *version = ((response << 16) + response_param) & 0xffffff;

    return CR_OK;
}

CredoError_t common_fw_hash(CredoSlice_t* slice, unsigned* hash) {
    unsigned cmd;
    unsigned response;
    unsigned response_param;

    cmd = FW_CMD_HASH_READ;
    // ERR_PROPS( firmware_cmd(slice, cmd, 0, &response, &response_param));
    ERR_PROPS(hal_fw_cmd(slice, cmd, 0, &response, &response_param));
    *hash = ((response << 16) + response_param) & 0xffffff;

    return CR_OK;
}

CredoError_t common_fw_magic(CredoSlice_t* slice, unsigned* magic) {
    ERR_PROPS(readReg(slice, REG_MAGIC, magic));

    if (!is_fw_load_magic(*magic)) log_if_invalid_fw_load_magic(slice, *magic);

    return CR_OK;
}

CredoError_t common_fw_crc(CredoSlice_t* slice, unsigned* crc) {
    unsigned cmd;
    unsigned response;

    cmd = FW_CMD_CRC_READ;
    // return firmware_cmd(slice, cmd, 0, &response, crc);
    return hal_fw_cmd(slice, cmd, 0, &response, crc);
}

CredoError_t common_fw_date(CredoSlice_t* slice, unsigned* date) {
    unsigned cmd;
    unsigned response;

    cmd = FW_CMD_DATE_READ;
    // return firmware_cmd(slice, cmd, 0, &response, date);
    return hal_fw_cmd(slice, cmd, 0, &response, date);
}
