#include "bald_eagle.h"
#include "be_device.h"
#include "be_functions.h"

#include "canary/canary_serdes.h"
#include "common/common_firmware.h"

#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Firmare debug command (for lanes<=16)
 * Input
 *  Major command   = 0xB
 *  cmd.7:4         = section
 *  cmd.3:0         = lane
 *  detail.15:0     = index
 * Output:
 *  detail          = result
 */

CredoError_t be_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy) {
    unsigned val;

    ERR_PROPS(readReg(slice, REG_FW_PHY_READY, &val));

    *rdy = val;
    return CR_OK;
}

CredoError_t be_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy) {
    unsigned phy_ready;
    if (!be_is_valid_lane(slice, lane)) return CR_INVALID_ARGS;
    ERR_PROPS(be_fw_phy_ready(slice, &phy_ready));
    *rdy = (phy_ready >> lane) & 1;
    return CR_OK;
}

static CredoError_t be_fw_config_lane_inner(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed) {
    unsigned cmd, fw_speed, response;
    switch (speed) {
        case CONFIG_10G:
            fw_speed = DEFSPEED(10G);
            break;
        case CONFIG_20G:
            fw_speed = DEFSPEED(20G);
            break;
        case CONFIG_25G:
            fw_speed = DEFSPEED(25G);
            break;
        case CONFIG_26G:
            fw_speed = DEFSPEED(26G);
            break;
        case CONFIG_51G:
            fw_speed = DEFSPEED(51G);
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            fw_speed = DEFSPEED(53G);
            break;
        case CONFIG_56G:
            fw_speed = DEFSPEED(56G);
            break;
        default:
            LOGS_ERROR("[Lane config] Not supported Speed = %d", speed);
            return CR_NOTIMPLEMENTED;
    }
    BeSlice_t* be_slice = (BeSlice_t*)slice;
    if (speed < CONFIG_50G) {
        cmd = FW_CMD_CONFIG_MODE + ((int)(MODE_PHY_NRZ) << 4) + lane;
    } else {
        cmd = FW_CMD_CONFIG_MODE + ((int)(MODE_PHY_PAM4) << 4) + lane;
    }
    ERR_PROPS(common_fw_cmd(slice, cmd, fw_speed, &response, NULL));

    be_slice->lane_mode[lane] = lane_mode;
    return CR_OK;
}

CredoError_t be_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed) {
    ERR_PROPS(be_fw_config_lane_inner(slice, lane, lane_mode, speed));
    return CR_OK;
}

static CredoError_t be_fw_config_lane_loopback_inner(CredoSlice_t* slice, int lane, uint32_t speed) {
    unsigned cmd, fw_speed, response;
    CredoLaneMode_t lane_mode = CR_LMODE_NRZ;
    switch (speed) {
        case CONFIG_10G:
            fw_speed = DEFSPEED(10G);
            break;
        case CONFIG_20G:
            fw_speed = DEFSPEED(20G);
            break;
        case CONFIG_25G:
            fw_speed = DEFSPEED(25G);
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            fw_speed = DEFSPEED(53G);
            lane_mode = CR_LMODE_PAM4;
            break;
        default:
            return CR_UNSUPPORTED;
    }

    if (speed < CONFIG_50G) {
        cmd = FW_CMD_CONFIG_MODE + ((int)(MODE_LOOPBACK_NRZ) << 4) + lane;
    } else {
        cmd = FW_CMD_CONFIG_MODE + ((int)(MODE_LOOPBACK_PAM4) << 4) + lane;
    }
    ERR_PROPS(common_fw_cmd(slice, cmd, fw_speed, &response, NULL));

    ERR_PROPS(hal_set_lane_mode(slice, lane, lane_mode));
    return CR_OK;
}

CredoError_t be_fw_config_lane_loopback(CredoSlice_t* slice, int lane, uint32_t speed) {
    ERR_PROPS(be_fw_config_lane_loopback_inner(slice, lane, speed));
    return CR_OK;
}

CredoError_t be_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed,
                              uint32_t flags) {
    if (flags & CR_LFLAG_LOOPBACK) {
        ERR_PROPS(be_fw_config_lane_loopback_inner(slice, lane, speed));
    } else {
        ERR_PROPS(be_fw_config_lane_inner(slice, lane, lane_mode, speed));
    }
    return CR_OK;
}

CredoError_t be_fw_set_nrz_optical_mode(CredoSlice_t* slice, unsigned first_lane, unsigned lane_count, int is_optical) {
    unsigned optical_mask;
    unsigned new_mask = ((1 << lane_count) - 1) << first_lane;

    ERR_PROPS(common_fw_reg_rd_internal(slice, FWREG_NRZ_OPTICAL, &optical_mask));

    if (is_optical) {
        optical_mask |= new_mask;
    } else {
        optical_mask &= ~new_mask;
    }
    return common_fw_reg_wr_internal(slice, FWREG_NRZ_OPTICAL, optical_mask);
}

CredoError_t be_fw_get_nrz_optical_mode(CredoSlice_t* slice, unsigned* optical_mode) {
    return common_fw_reg_rd_internal(slice, FWREG_NRZ_OPTICAL, optical_mode);
}

/**
 * @brief Send a deconfig command to BaldEagle device and mark off all destroyed port and lanes
 * @param slice BaldEagle slice handle
 * @param config_mode The config/deconfig command to send, without major command
 * @return Error Code
 */
CredoError_t be_fw_deconfig_cmd(CredoSlice_t* slice, unsigned config_mode) {
    unsigned response, mask;
    BeSlice_t* be_slice = (BeSlice_t*)slice;

    ERR_PROPS(common_fw_cmd(slice, FW_CMD_DESTROY_MODE + config_mode, 0, &response, &mask));

    for (int port = 0; port < BE_MAX_PORT; port++) {
        BePortInfo_t* port_info = &be_slice->port_info[port];
        CredoPortConfig_t* port_config = &port_info->port_config;
        if (port_info->configured == false) continue;
        int lane = port_config->host_start_lane;
        if (mask & (1 << lane)) port_info->configured = false;
    }
    for (int lane = 0; lane < LANES; lane++) {
        if (mask & (1 << lane)) be_slice->lane_mode[lane] = CR_LMODE_OFF;
    }
    return CR_OK;
}

/**
 * @brief Destroy one lane to BaldEagle device and all its associated lanes.
 * @param slice BaldEagle slice handle
 * @param lane The lane to destroy
 * @return Error Code
 */
static CredoError_t be_fw_deconfig_lane_inner(CredoSlice_t* slice, int lane) {
    return be_fw_deconfig_cmd(slice, ((int)(MODE_PHY_NRZ) << 4) + lane);
}

CredoError_t be_fw_deconfig_lane(CredoSlice_t* slice, int lane) {
    ERR_PROPS(be_fw_deconfig_lane_inner(slice, lane));
    return CR_OK;
}

CredoError_t be_fw_get_opt_mode(CredoSlice_t* slice, int lane, unsigned* opt_mode) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_OPT_MODE, opt_mode));
    return CR_OK;
}

CredoError_t be_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state) {
    return common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_AN_STATE, an_state);
}

CredoError_t be_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    unsigned speed_index;
    ERR_PROP(common_fw_get_speed_index(slice, lane, &speed_index));
    *speed_kbps = be_fw_speed_kbps(speed_index);
    return CR_OK;
}

CredoError_t be_fw_get_lane_link_training(CredoSlice_t* slice, int lane, unsigned* lt_on) {
    return hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_LT_ON, lt_on);
}

CredoError_t be_fw_get_status(CredoSlice_t* slice, unsigned* status) {
    CredoError_t rc;
    unsigned cmd = FW_CMD_INFO + (TOP_DEBUG << 4);
    unsigned r;
    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL, TOP_DEBUG_OPT_MODE));
    ERR_PROPS(writeReg(slice, REG_CMD, cmd));
    rc = common_wait_fw_cmd(slice, cmd, &r, slice->data->fw_cmd_timeout);
    *status = 1;
    if (rc == CR_FW_TIMEOUT) *status = 0;
    if ((r & FW_RESPONSE_FREEZE_MASK) == FW_RESPONSE_FREEZE_ERROR) *status = 0;

    return CR_OK;
}

CredoError_t be_fw_download_from_file(CredoSlice_t* slice, const char* image_file) {
    FILE* fw;
    CredoError_t ret;

    fw = fopen(image_file, "rb");

    if (!fw) {
        LOGS_ERROR("[Firmware load] Error opening firmware file %s", image_file);
        return CR_FAIL;
    }

    /* Unload firmware */
    ERR_CATCH((ret = common_fw_unload(slice)), goto exit);

    ERR_CATCH((ret = common_fw_load(slice, fw)), goto exit);

exit:
    fclose(fw);
    return ret;
}

// TODO, fec engine index should get from firmware but not suuport, hard code here
CredoError_t be_fw_get_rsfec_index(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int* index) {
    BeSlice_t* be_slice = (BeSlice_t*)slice;
    BePortInfo_t* port_info = &be_slice->port_info[port_id];

    if (port_info->configured == false) return CR_INVALID_ARGS;

    *index = port_info->port_config.line_start_lane / 2;

    if (side == CR_SIDE_HOST) *index -= 4;

    return CR_OK;
}
