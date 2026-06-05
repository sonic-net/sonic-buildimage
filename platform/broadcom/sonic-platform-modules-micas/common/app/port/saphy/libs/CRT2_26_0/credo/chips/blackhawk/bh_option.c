#include "project.h"
#include "bh_device.h"
#include "bh_functions.h"
#include "bh_option.h"

#include "common/common_firmware.h"
#include "rsfec/rsfec.h"

#include <string.h>

CredoError_t bh_lane_option_get_generic(CredoSlice_t* slice, int lane, const char* option_name, int* value) {
    if (strcmp(option_name, "sd_delay") == 0) {
        ERR_PROPS(bh_fw_get_sd_delay(slice, lane, value));
    } else if (strcmp(option_name, "fast_recover_timeout") == 0) {
        ERR_PROPS(bh_fw_get_fast_recover_timeout(slice, lane, value));
    } else {
        return CR_UNSUPPORTED;
    }

    return CR_OK;
}

CredoError_t bh_lane_option_set_generic(CredoSlice_t* slice, int lane, const char* option_name, int value) {
    if (strcmp(option_name, "sd_delay") == 0) {
        ERR_PROPS(bh_fw_set_sd_delay(slice, lane, value));
    } else if (strcmp(option_name, "fast_recover_timeout") == 0) {
        ERR_PROPS(bh_fw_set_fast_recover_timeout(slice, lane, value));
    } else {
        return CR_UNSUPPORTED;
    }

    return CR_OK;
}

CredoError_t bh_lane_option_get_optical_mode(CredoSlice_t* slice, int lane, const char* name, int* value) {
    unsigned optical_mask = 0;
    ERR_PROP(readReg(slice, REG_FW_OPTICAL, &optical_mask));
    *value = (optical_mask >> lane) & 0x1;
    return CR_OK;
}

CredoError_t bh_lane_option_set_optical_mode(CredoSlice_t* slice, int lane, const char* name, int value) {
    unsigned optical_mask = 0;
    ERR_PROP(readReg(slice, REG_FW_OPTICAL, &optical_mask));

    if (value) {
        optical_mask |= (1 << lane);
    } else {
        optical_mask &= ~(1 << lane);
    }
    return writeReg(slice, REG_FW_OPTICAL, optical_mask);
}

CredoError_t bh_option_get_fifo_auto_recover(CredoSlice_t* slice, const char* name, int* value) {
    bool en = false;
    ERR_PROPS(bh_fw_get_top_option(slice, TOP_OPTION_BG_AUTO_RECOVER, &en));
    *value = en ? 1 : 0;
    return CR_OK;
}

CredoError_t bh_option_set_fifo_auto_recover(CredoSlice_t* slice, const char* name, int value) {
    ERR_PROPS(bh_fw_set_top_option(slice, TOP_OPTION_BG_AUTO_RECOVER, value));
    return CR_OK;
}

CredoError_t bh_option_get_low_latency_1G(CredoSlice_t* slice, const char* name, int* value) {
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_1G_OVERSAMPLE, (unsigned*)value));
    return CR_OK;
}

CredoError_t bh_option_set_low_latency_1G(CredoSlice_t* slice, const char* name, int value) {
    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_1G_OVERSAMPLE, (unsigned)value));
    return CR_OK;
}

CredoError_t bh_option_get_anlt_power_saving(CredoSlice_t* slice, const char* name, int* value) {
    bool en = false;
    ERR_PROPS(bh_fw_get_top_option(slice, TOP_OPTION_ANLT_POWER_SAVING, &en));
    *value = en ? 1 : 0;
    return CR_OK;
}

CredoError_t bh_option_set_anlt_power_saving(CredoSlice_t* slice, const char* name, int value) {
    ERR_PROPS(bh_fw_set_top_option(slice, TOP_OPTION_ANLT_POWER_SAVING, value));
    return CR_OK;
}

CredoError_t bh_option_get_fec_adv_read_info(CredoSlice_t* slice, const char* name, int* value) {
    bool en = false;
    ERR_PROPS(bh_fw_get_top_option(slice, TOP_OPTION_FEC_ADV_INFO, &en));
    *value = en ? 1 : 0;
    return CR_OK;
}

CredoError_t bh_option_set_fec_adv_read_info(CredoSlice_t* slice, const char* name, int value) {
    ERR_PROPS(bh_fw_set_top_option(slice, TOP_OPTION_FEC_ADV_INFO, value));
    return CR_OK;
}

CredoError_t bh_option_get_scratch(CredoSlice_t* slice, const char* name, int* value) {
    int idx = 0;
    sscanf(name, "spare%d", &idx);

    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_SCRATCH + (2 * idx), (unsigned*)value));
    return CR_OK;
}

CredoError_t bh_option_set_scratch(CredoSlice_t* slice, const char* name, int value) {
    int idx = 0;
    sscanf(name, "spare%d", &idx);

    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_SCRATCH + (2 * idx), value));
    return CR_OK;
}

CredoError_t bh_option_get_sd_delay(CredoSlice_t* slice, const char* option_name, int* value) {
    if (strcmp(option_name, "sd_delay_optical_nrz") == 0) {
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_NRZ_SD_DELAY_OPTICAL, (unsigned*)value));
    } else if (strcmp(option_name, "sd_delay_nrz") == 0) {
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_NRZ_SD_DELAY, (unsigned*)value));
    } else if (strcmp(option_name, "sd_delay_optical_pam4") == 0) {
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_PAM4_SD_DELAY_OPTICAL, (unsigned*)value));
    } else if (strcmp(option_name, "sd_delay_pam4") == 0) {
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_PAM4_SD_DELAY, (unsigned*)value));
    } else {
        return CR_INVALID_ARGS;
    }

    return CR_OK;
}

CredoError_t bh_option_set_sd_delay(CredoSlice_t* slice, const char* option_name, int value) {
    if (strcmp(option_name, "sd_delay_optical_nrz") == 0) {
        ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_NRZ_SD_DELAY_OPTICAL, (unsigned)value));
    } else if (strcmp(option_name, "sd_delay_nrz") == 0) {
        ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_NRZ_SD_DELAY, (unsigned)value));
    } else if (strcmp(option_name, "sd_delay_optical_pam4") == 0) {
        ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_PAM4_SD_DELAY_OPTICAL, (unsigned)value));
    } else if (strcmp(option_name, "sd_delay_pam4") == 0) {
        ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_PAM4_SD_DELAY, (unsigned)value));
    } else {
        return CR_INVALID_ARGS;
    }

    return CR_OK;
}

CredoError_t bh_option_get_uncorr_monitor(CredoSlice_t* slice, const char* option_name, int* value) {
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_UNCORR_MONITOR, (unsigned*)value));
    return CR_OK;
}

CredoError_t bh_option_set_uncorr_monitor(CredoSlice_t* slice, const char* option_name, int value) {
    for (int index = 0; index < 8; index++) {
        ERR_PROPS(writeRegLane(slice, index, REG_RSFEC_DIS_CW_BAD_S, value ? 1 : 0));
    }
    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_UNCORR_MONITOR, value));
    return CR_OK;
}

CredoError_t bh_option_get_clk_output_squelch(CredoSlice_t* slice, const char* option_name, int* value) {
    *value = 0;

    unsigned cko_info = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_CKO_INFO, &cko_info));
    *value = (cko_info >> 12) & 0x7;

    return CR_OK;
}

CredoError_t bh_option_set_clk_output_squelch(CredoSlice_t* slice, const char* option_name, int value) {
    for (unsigned clk_out = 0; clk_out < 3; clk_out++) {
        bool en = ((unsigned)value >> clk_out) & 0x1;
        ERR_PROPS(bh_fw_clock_outout_ctrl(slice, clk_out, en));
    }

    return CR_OK;
}

CredoError_t bh_option_set_gearbox_traffic_gen(CredoSlice_t* slice, const char* option_name, int value) {
    return hal_fw_reg_wr(slice, 24, 0, (value != 0) ? 1 : 0);
}

CredoError_t bh_option_get_gearbox_traffic_gen(CredoSlice_t* slice, const char* option_name, int* value) {
    return hal_fw_reg_rd(slice, 24, 0, (unsigned*)value);
}

CredoError_t bh_option_set_acfg_off(CredoSlice_t* slice, const char* option_name, int value) {
    return writeReg(slice, REG_FW_ACFG_OFF, (unsigned)value);
}

CredoError_t bh_option_get_acfg_off(CredoSlice_t* slice, const char* option_name, int* value) {
    ERR_PROP(readReg(slice, REG_FW_ACFG_OFF, (unsigned*)value));
    return CR_OK;
}
