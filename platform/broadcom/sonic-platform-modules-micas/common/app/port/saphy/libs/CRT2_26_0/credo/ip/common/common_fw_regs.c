#include "project.h"

#include "common/common_firmware.h"

#include "utility.h"
#include "sdk.h"

CredoError_t common_fw_reg_rd(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned* value) {
    unsigned cmd;
    unsigned response;
    unsigned response_param = value ? *value : 0;

    cmd = FW_CMD_INTERNAL_REG_READ + (section & 0xF);

    // ERR_PROPS( firmware_cmd(slice, cmd, addr, &response, NULL));
    ERR_PROPS(hal_fw_cmd(slice, cmd, addr, &response, &response_param));
    ERR_PROPS(readReg(slice, REG_FW_REG_VALUE, value));

    return CR_OK;
}

CredoError_t common_fw_reg_wr(CredoSlice_t* slice, unsigned addr, unsigned section, unsigned value) {
    unsigned cmd;
    unsigned response;

    cmd = FW_CMD_INTERNAL_REG_WRITE + (section & 0xF);
    ERR_PROPS(writeReg(slice, REG_FW_REG_VALUE, value));
    // ERR_PROPS( firmware_cmd(slice, cmd, addr, &response, NULL));
    ERR_PROPS(hal_fw_cmd(slice, cmd, addr, &response, NULL));

    return CR_OK;
}

CredoError_t common_fw_reg_rd_internal(CredoSlice_t* slice, unsigned combined_addr, unsigned* value) {
    unsigned section = combined_addr >> 16;
    unsigned addr = combined_addr & 0xffff;
    int is_32b = 0;
    if (section & (1 << 15)) {
        is_32b = 1;
        section &= 0x7fff;
    }
    if (is_32b) {
        unsigned value1;
        ERR_PROPS(hal_fw_reg_rd(slice, addr + 1, section, &value1));
        ERR_PROPS(hal_fw_reg_rd(slice, addr, section, value));
        *value = (*value << 16) + value1;
        return CR_OK;
    } else {
        return hal_fw_reg_rd(slice, addr, section, value);
    }
}

CredoError_t common_fw_reg_wr_internal(CredoSlice_t* slice, unsigned combined_addr, unsigned value) {
    unsigned section = combined_addr >> 16;
    unsigned addr = combined_addr & 0xffff;
    int is_32b = 0;
    if (section & (1 << 15)) {
        is_32b = 1;
        section &= 0x7fff;
    }
    if (is_32b) {
        ERR_PROPS(hal_fw_reg_wr(slice, addr + 1, section, value & 0xffff));
        return hal_fw_reg_wr(slice, addr, section, value >> 16);
    } else {
        return hal_fw_reg_wr(slice, addr, section, value);
    }
}
