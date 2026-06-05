#include "bh_device.h"
#include "bh_functions.h"
#include "blackhawk.h"

#include "common/common_firmware.h"
#include "common/common_reset.h"

#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>

static CredoError_t bh_fw_mark_off_lanes(CredoSlice_t* slice, unsigned mask) {
    for (int lane_num = 0; lane_num < slice->desc->lane_count; lane_num++) {
        if (mask & (1 << lane_num)) {
            ERR_PROPS(hal_set_lane_mode(slice, lane_num, CR_LMODE_OFF));
        }
    }
    return CR_OK;
}

CredoError_t bh_fw_set_optical_mode(CredoSlice_t* slice, unsigned first_lane, unsigned lane_count, int is_optical) {
    unsigned optical_mask;
    unsigned new_mask = ((1 << lane_count) - 1) << first_lane;

    ERR_PROP(readReg(slice, REG_FW_OPTICAL, &optical_mask));

    if (is_optical) {
        optical_mask |= new_mask;
    } else {
        optical_mask &= ~new_mask;
    }

    return writeReg(slice, REG_FW_OPTICAL, optical_mask);
}

CredoError_t bh_fw_deconfig_cmd(CredoSlice_t* slice, FirmwareMode_t fw_mode, unsigned detail1, unsigned detail2) {
    unsigned mask1 = 0, mask2 = 0;
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    unsigned cmd = FW_CMD_DESTROY_MODE + ((int)fw_mode << 4);

    ERR_PROPS(common_fw_cmd_ex(slice, cmd, detail1, detail2, NULL, &mask1, &mask2));

    unsigned destroyed_lane = ((mask1 & 0xFF) << 16) | mask2;
    unsigned destroyed_port = ((mask1 >> 8) & 0xFF);

    for (int port = 0; port < BH_MAX_PORT; port++) {
        BhPortInfo_t* port_info = &bh_slice->port_info[port];
        CredoPortConfig_t* port_config = &port_info->port_config;
        if (port_info->configured == false) continue;
        int lane = port_config->host_start_lane;
        if (destroyed_lane & (1 << lane)) port_info->configured = false;
    }

    ERR_PROPS(bh_fw_mark_off_lanes(slice, destroyed_lane));

    LOGS_DEBUG("[Firmware deconfig] lane: 0x%04X, port: 0x%02X", destroyed_lane, destroyed_port);
    return CR_OK;
}

static CredoError_t bh_fw_get_feature_inner(CredoSlice_t* slice, unsigned* response_output, unsigned* feature) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    unsigned response = 0, response_param = FW_CMD_LOG_SILENT;
    unsigned feature_en = 0;
    CredoError_t err = hal_fw_cmd(slice, FW_CMD_FEATURE_EN, 0, &response, &response_param);
    err = (response & (FW_CMD_FEATURE_EN >> 4)) ? err : CR_FW_TIMEOUT;
    if (response_output) *response_output = response;
    feature_en = ((response << 16) + response_param) & 0xffffff;
    if (err != CR_SUCCESS || feature_en != FW_FEATURE_ENABLE) {
        bh_slice->fw_feature = *feature = 0;
        return err;
    }

    ERR_PROP(hal_fw_cmd(slice, FW_CMD_FEATURE, 0, &response, &response_param));
    bh_slice->fw_feature = *feature = ((response << 16) + response_param) & 0xffffff;

    return CR_OK;
}

CredoError_t bh_fw_get_feature(CredoSlice_t* slice, unsigned* feature) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    if (bh_slice->fw_feature_captured) {
        *feature = bh_slice->fw_feature;
        return CR_OK;
    }
    bh_fw_get_feature_inner(slice, NULL, feature);
    bh_slice->fw_feature = *feature;
    bh_slice->fw_feature_captured = true;

    return CR_SUCCESS;
}

CredoError_t bh_fw_download(CredoSlice_t* slice, const char* image_file) {
    CredoError_t ret = CR_FAIL;

    FILE* file = fopen(image_file, "rb");
    if (!file) {
        LOGS_ERROR("[Firmware load] Error opening firmware file %s", image_file);
        return CR_FAIL;
    }

    ERR_CATCH((ret = common_fw_unload(slice)), goto exit);

#if 0
    ERR_CATCH((ret = common_soft_reset(slice)), goto exit);
#else
    // using reg_reset and logic_reset instead of soft_reset, because soft_reset will trigger spiflash booting
    ERR_CATCH(hal_reg_reset(slice), goto exit);
    ERR_CATCH(hal_logic_reset(slice), goto exit);
#endif

    ERR_CATCH((ret = common_fw_load(slice, file)), goto exit);

exit:
    fclose(file);
    return ret;
}

CredoError_t bh_fw_force_loopback(CredoSlice_t* slice, unsigned lane, unsigned force) {
    return common_fw_cmd_ex(slice, FW_CMD_FORCE_LOOPBACK, force ? 1 : 0, lane, NULL, NULL, NULL);
}

/* CDR Clk forward control:
 * bit3:0 = pin assignment(0~2)
 * bit 4 = config type(0: fw_ctrl_clk, 1: pin_en)
 * bit 5 = enable/disable
 **/
CredoError_t bh_fw_clock_outout_notify(CredoSlice_t* slice, unsigned lane, unsigned clk_output, bool en) {
    unsigned dummy = FW_CMD_LOG_SILENT;
    unsigned param = 0x10 | (en << 5) | clk_output;
    CredoError_t ret = common_fw_cmd_ex(slice, FW_CMD_CLK_OUTPUT_CTRL, param, lane, NULL, &dummy, NULL);
    if (ret != CR_OK) {
        LOGS_WARN("firmware don't support output clock ctrl feature");
    }
    return CR_OK;
}

CredoError_t bh_fw_clock_outout_ctrl(CredoSlice_t* slice, unsigned clk_output, bool en) {
    unsigned dummy = FW_CMD_LOG_SILENT;
    unsigned param = (en << 5) | clk_output;
    CredoError_t ret = common_fw_cmd_ex(slice, FW_CMD_CLK_OUTPUT_CTRL, param, 0, NULL, &dummy, NULL);
    if (ret != CR_OK) {
        LOGS_WARN("firmware don't support output clock ctrl feature");
    }
    return CR_OK;
}

CredoError_t bh_fw_tx_control(CredoSlice_t* slice, unsigned lane, FwTxSource_t source) {
    unsigned source_ctrl = (source == FW_TX_SOURCE_NOFORCE) ? 0 : ((unsigned)source << 2) | (1 << 0);
    return common_fw_cmd_ex(slice, FW_CMD_CONFIG_TX, source_ctrl, lane, NULL, NULL, NULL);
}

CredoError_t bh_fw_rx_control(CredoSlice_t* slice, unsigned lane, FwRxControl_t rx_control) {
    return common_fw_cmd_ex(slice, FW_CMD_LANE_RESET, rx_control, lane, NULL, NULL, NULL);
}

CredoError_t bh_fw_TRF_control(CredoSlice_t* slice, unsigned lane, TRF_t source) {
    unsigned trf_ctrl = (source == TRF_NOFORCE) ? 0 : ((unsigned)source << 2) | (1 << 0);
    return common_fw_cmd_ex(slice, FW_CMD_TRF_CONTROL, trf_ctrl, lane, NULL, NULL, NULL);
}

CredoError_t bh_fw_PLL_source_control(CredoSlice_t* slice, unsigned lane, int source) {
    unsigned tx_pi_ctrl = (source < 0) ? 0 : ((unsigned)source << 2) | (1 << 0);
    return common_fw_cmd_ex(slice, FW_CMD_TX_PI_CONTROL, tx_pi_ctrl, lane, NULL, NULL, NULL);
}

CredoError_t bh_fw_port_config_mask(CredoSlice_t* slice, unsigned* config_mask) {
    *config_mask = FW_CMD_LOG_SILENT;
    CredoError_t ret = common_fw_cmd(slice, FW_CMD_PORT_QUERY_ALL, 0, NULL, config_mask);
    if (ret != CR_OK) {
        *config_mask = 0xff;  // backport compatibility, make sure caller will query each port
    }
    return CR_OK;
}

CredoError_t bh_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy) {
    return readReg(slice, REG_FW_PHY_READY, rdy);
}

CredoError_t bh_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy) {
    unsigned phy_ready;
    if (!bh_is_valid_lane(slice, lane)) return CR_INVALID_ARGS;
    ERR_PROPS(bh_fw_phy_ready(slice, &phy_ready));
    *rdy = (phy_ready >> lane) & 1;
    return CR_OK;
}

CredoError_t bh_fw_optical_mode(CredoSlice_t* slice, unsigned* optical_mode) {
    return readReg(slice, REG_FW_OPTICAL, optical_mode);
}

static CredoError_t bh_fw_config_lane_inner(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed) {
    unsigned fw_speed;
    unsigned cmd = FW_CMD_CONFIG_MODE + ((int)(MODE_PHY) << 4);
    CredoLaneMode_t mode = CR_LMODE_NRZ;

    lane &= 0x3f;
    /* For phy mode, only B side lane number in detail2 */
    switch (speed) {
        case CONFIG_1G:
            fw_speed = DEFSPEED(1G);
            break;
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
        case CONFIG_28G:
            fw_speed = DEFSPEED(28G);
            break;
        case CONFIG_28_125G:
            fw_speed = DEFSPEED(28_125G);
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            fw_speed = DEFSPEED(53G);
            mode = CR_LMODE_PAM4;
            break;
        case CONFIG_55G:
            fw_speed = DEFSPEED(55G);
            mode = CR_LMODE_PAM4;
            break;
        case CONFIG_56_15G:
            fw_speed = DEFSPEED(56_15G);
            mode = CR_LMODE_PAM4;
            break;
        case CONFIG_56G:
            LOGS_WARN("Set to 56.25G, please use %u instead.", CONFIG_56_25G);
        case CONFIG_56_25G:
            fw_speed = DEFSPEED(56G);
            mode = CR_LMODE_PAM4;
            break;
        default:
            // FIXME: need refactor
            if (speed >= CONFIG_55_9G && speed < CONFIG_56G) {
                fw_speed = DEFSPEED(55_9G);
                mode = CR_LMODE_PAM4;
            } else if (speed >= CONFIG_27G && speed < 28000) {
                fw_speed = DEFSPEED(27_9G);
            } else {
                LOGS_ERROR("Not supported Speed: %d", speed);
                return CR_UNSUPPORTED;
            }
    }

    // need check
    if (lane_mode != mode) return CR_FAIL;

    ERR_PROPS(common_fw_cmd_ex(slice, cmd, fw_speed, lane, NULL, NULL, NULL));

    ERR_PROPS(bh_set_lane_mode(slice, lane, lane_mode));

    return CR_OK;
}

CredoError_t bh_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed) {
    ERR_PROPS(bh_fw_config_lane_inner(slice, lane, lane_mode, speed));
    return CR_OK;
}

static CredoError_t bh_fw_config_lane_loopback_inner(CredoSlice_t* slice, int lane, unsigned speed) {
    unsigned fw_speed;
    unsigned cmd = FW_CMD_CONFIG_MODE + ((int)(MODE_LOOPBACK) << 4);

    CredoLaneMode_t mode = CR_LMODE_NRZ;
    /* For phy mode, only B side lane number in detail2 */
    switch (speed) {
        case CONFIG_1G:
            fw_speed = DEFSPEED(1G);
            break;
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
        case CONFIG_28_125G:
            fw_speed = DEFSPEED(28_125G);
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            fw_speed = DEFSPEED(53G);
            mode = CR_LMODE_PAM4;
            break;
        case CONFIG_55G:
            fw_speed = DEFSPEED(55G);
            mode = CR_LMODE_PAM4;
            break;
        case CONFIG_56G:
            fw_speed = DEFSPEED(56G);
            mode = CR_LMODE_PAM4;
            break;
        default:
            // FIXME: need refactor
            if (speed >= CONFIG_55_9G && speed < CONFIG_56G) {
                fw_speed = DEFSPEED(55_9G);
                mode = CR_LMODE_PAM4;
            } else if (speed >= CONFIG_27G && speed < 28000) {
                fw_speed = DEFSPEED(27_9G);
            } else {
                LOGS_ERROR("Not supported Speed: %d", speed);
                return CR_UNSUPPORTED;
            }
    }

    ERR_PROPS(common_fw_cmd_ex(slice, cmd, fw_speed, lane, NULL, NULL, NULL));

    ERR_PROPS(bh_set_lane_mode(slice, lane, mode));
    return CR_OK;
}

CredoError_t bh_fw_config_lane_loopback(CredoSlice_t* slice, int lane, unsigned speed) {
    ERR_PROPS(bh_fw_config_lane_loopback_inner(slice, lane, speed));
    return CR_OK;
}

CredoError_t bh_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, uint32_t speed,
                              uint32_t flags) {
    if (flags & CR_LFLAG_LOOPBACK) {
        ERR_PROPS(bh_fw_config_lane_loopback_inner(slice, lane, speed));
    } else {
        ERR_PROPS(bh_fw_config_lane_inner(slice, lane, lane_mode, speed));
    }
    return CR_OK;
}

static CredoError_t bh_fw_deconfig_lane_inner(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(bh_get_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_OFF) {
        return CR_OK;
    }
    return bh_fw_deconfig_cmd(slice, MODE_PHY, 0, (lane & 0x3f));
}

CredoError_t bh_fw_deconfig_lane(CredoSlice_t* slice, int lane) {
    ERR_PROPS(bh_fw_deconfig_lane_inner(slice, lane));
    return CR_OK;
}

CredoError_t bh_fw_lane_disable(CredoSlice_t* slice, int lane) {
    return common_fw_cmd_ex(slice, FW_CMD_LANE_DISABLE, 0, lane, NULL, NULL, NULL);
}

CredoError_t bh_fw_atomic_read(CredoSlice_t* slice, unsigned addr, int len, unsigned* data) {
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_DUMP_MDIO, addr, len, NULL, NULL, NULL));
    for (int i = 0; i < len; i++) {
        ERR_PROPS(readTop(slice, REG_DATA + i, data + i));
    }
    return CR_OK;
}

CredoError_t bh_fw_get_top_option(CredoSlice_t* slice, unsigned option, bool* en) {
    unsigned top_options = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &top_options));
    *en = (top_options & option) ? true : false;
    return CR_OK;
}

CredoError_t bh_fw_set_top_option(CredoSlice_t* slice, unsigned option, bool en) {
    unsigned top_options = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &top_options));

    if (en) {
        top_options |= option;
    } else {
        top_options &= ~(option);
    }

    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_OPTIONS, top_options));
    return CR_OK;
}

CredoError_t bh_fw_set_top_sm_enalbe(CredoSlice_t* slice, int lane, bool en) {
    unsigned sm_en = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_SM_ENABLE, &sm_en));

    if (en) {
        sm_en |= (1 << lane);
    } else {
        sm_en &= ~(1 << lane);
    }

    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_SM_ENABLE, sm_en));
    return CR_OK;
}

CredoError_t bh_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    unsigned speed_index;
    ERR_PROP(common_fw_get_speed_index(slice, lane, &speed_index));
    *speed_kbps = bh_fw_speed_kbps(speed_index);
    return CR_OK;
}

CredoError_t bh_fw_get_opt_mode(CredoSlice_t* slice, int lane, unsigned* opt_mode) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_OPT_MODE, opt_mode));
    return CR_OK;
}

CredoError_t bh_fw_get_retimer_debug(CredoSlice_t* slice, int lane, int index, unsigned* val) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, RETIMER_DEBUG, index, val));
    return CR_OK;
}

CredoError_t bh_fw_get_retimer_state(CredoSlice_t* slice, int lane, unsigned* state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, RETIMER_DEBUG, RETIMER_DEBUG_STATE, state));
    return CR_OK;
}

CredoError_t bh_fw_get_bitmux_debug(CredoSlice_t* slice, int lane, int index, unsigned* val) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, BITMUX_DEBUG, index, val));
    return CR_OK;
}

CredoError_t bh_fw_get_bitmux_state(CredoSlice_t* slice, int lane, unsigned* state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, BITMUX_DEBUG, BITMUX_DEBUG_STATE, state));
    return CR_OK;
}

CredoError_t bh_fw_get_bitmux_fifo(CredoSlice_t* slice, unsigned fifo_idx, unsigned buffer_type, unsigned* fifo) {
    ERR_PROP_SLICE(slice, writeTop(slice, REG_DATA, (buffer_type << 8) | (fifo_idx & 0xff)));
    ERR_PROP_SLICE(slice, hal_fw_debug_cmd(slice, 0, BITMUX_DEBUG, BITMUX_DEBUG_RD_DIFF_PT, fifo));
    return CR_SUCCESS;
}

CredoError_t bh_fw_get_gearbox_debug(CredoSlice_t* slice, int lane, int index, unsigned* val) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, index, val));
    return CR_OK;
}

CredoError_t bh_fw_get_gearbox_state(CredoSlice_t* slice, int lane, unsigned* state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_STATE, state));
    return CR_OK;
}

CredoError_t bh_fw_set_sd_delay(CredoSlice_t* slice, int lane, int value) {
    ERR_PROP(writeReg(slice, REG_FW_REG_VALUE, value));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_TOP, TOP_LOAD_SD_DELAY, lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t bh_fw_get_sd_delay(CredoSlice_t* slice, int lane, int* value) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_SD_DELAY, (unsigned*)value));
    return CR_OK;
}

CredoError_t bh_fw_set_fast_recover_timeout(CredoSlice_t* slice, int lane, int value) {
    ERR_PROP(writeReg(slice, REG_FW_REG_VALUE, value));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_TOP, TOP_LOAD_RECOVER_SD_TIMEOUT, lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t bh_fw_get_fast_recover_timeout(CredoSlice_t* slice, int lane, int* value) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_SD_TIMEOUT, (unsigned*)value));
    return CR_OK;
}

// FIXME, update when firmware support ANLT
CredoError_t bh_fw_get_anlt(CredoSlice_t* slice, int lane, unsigned* an_on, unsigned* lt_on) {
    unsigned cmd = FW_CMD_INFO + TOP_DEBUG;
    unsigned r;
    ERR_PROP(writeReg(slice, REG_CMD_DETAIL2, lane));
    ERR_PROP(writeReg(slice, REG_CMD_DETAIL, TOP_DEBUG_ANLT_ON));
    ERR_PROP(writeReg(slice, REG_CMD, cmd));
    common_wait_fw_cmd(slice, cmd, &r, slice->data->fw_cmd_timeout);
    if ((r & FW_RESPONSE_MASK) != FW_RESPONSE_ERROR) {
        ERR_PROP(readReg(slice, REG_CMD_DETAIL, &r));
        switch (r & 0x7) {
            case (0x0):
                if (an_on) *an_on = 0;
                if (lt_on) *lt_on = 0;
                break;
            case (0x7):
            case (0x1):
                if (an_on) *an_on = 1;
                if (lt_on) *lt_on = 1;
                break;
            case (0x2):
                if (an_on) *an_on = 1;
                break;
            case (0x4):
                if (lt_on) *lt_on = 1;
                break;
            default:
                LOGS_ERROR("[GET FW ANLT] Internal error, get wrong flags, %03x", (r & 0x3));
                return CR_FAIL;
        }
    }
    return CR_OK;
    // return common_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_ANLT_ON, lt_on);
}

CredoError_t bh_fw_get_an_mode(CredoSlice_t* slice, int lane, unsigned* an_mode) {
    return common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_AN_MODE, an_mode);
}

CredoError_t bh_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state) {
    return common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_AN_STATE, an_state);
}

CredoError_t bh_fw_get_an_pages(CredoSlice_t* slice, int lane, uint64_t* tx_pages, uint64_t* rx_pages) {
    unsigned tx[3] = {0}, rx[3] = {0};
    for (int i = 0; i < 3; i++) {
        ERR_PROPS(common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LD_PAGES + (i * 3) + 0, tx + 0));
        ERR_PROPS(common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LD_PAGES + (i * 3) + 1, tx + 1));
        ERR_PROPS(common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LD_PAGES + (i * 3) + 2, tx + 2));
        tx_pages[i] = ((uint64_t)tx[2] << 32) | ((uint64_t)tx[1] << 16) | tx[0];

        ERR_PROPS(common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LP_PAGES + (i * 3) + 0, rx + 0));
        ERR_PROPS(common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LP_PAGES + (i * 3) + 1, rx + 1));
        ERR_PROPS(common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LP_PAGES + (i * 3) + 2, rx + 2));
        rx_pages[i] = ((uint64_t)rx[2] << 32) | ((uint64_t)rx[1] << 16) | rx[0];
    }
    return CR_OK;
}

CredoError_t bh_fw_get_rx_ffe_weighting_table(CredoSlice_t* slice, int lane, double** wt_table) {
    CredoLaneMode_t mode;
    ERR_PROPS(bh_get_lane_mode(slice, lane, &mode));

    int data[FW_FFE_WT_ROW * FW_FFE_WT_COL] = {0};
    switch (mode) {
        case CR_LMODE_NRZ:
            LOGS_WARN("[Get FW FFE WT TABLE] NRZ mode not implement");
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(common_fw_debug_cmd(slice, lane, PAM4_INFO, PAM4_INFO_FFE_WT0, NULL));
            ERR_PROPS(common_fw_info_data(slice, FW_FFE_WT_COL, data));

            ERR_PROPS(common_fw_debug_cmd(slice, lane, PAM4_INFO, PAM4_INFO_FFE_WT1, NULL));
            ERR_PROPS(common_fw_info_data(slice, FW_FFE_WT_COL, data + FW_FFE_WT_COL));

            ERR_PROPS(common_fw_debug_cmd(slice, lane, PAM4_INFO, PAM4_INFO_FFE_WT2, NULL));
            ERR_PROPS(common_fw_info_data(slice, FW_FFE_WT_COL, data + FW_FFE_WT_COL * 2));

            ERR_PROPS(common_fw_debug_cmd(slice, lane, PAM4_INFO, PAM4_INFO_FFE_WT3, NULL));
            ERR_PROPS(common_fw_info_data(slice, FW_FFE_WT_COL, data + FW_FFE_WT_COL * 3));

            ERR_PROPS(common_fw_debug_cmd(slice, lane, PAM4_INFO, PAM4_INFO_FFE_WT4, NULL));
            ERR_PROPS(common_fw_info_data(slice, FW_FFE_WT_COL, data + FW_FFE_WT_COL * 4));

            for (int i = 0; i < FW_FFE_WT_ROW * FW_FFE_WT_COL; i++) {
                wt_table[i / FW_FFE_WT_COL][i % FW_FFE_WT_COL] = data[i] / 128.0f;
            }
            break;
        default:
            LOGS_WARN("[Get FW FFE WT TABLE] Unknown mode %d", mode);
            break;
    }

    return CR_OK;
}

CredoError_t bh_fw_get_func_ver(CredoSlice_t* slice, unsigned* ver) {
    *ver = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, 0, TOP_DEBUG, TOP_DEBUG_INTERNAL_VER, ver);
    if (err != CR_OK) {
        *ver = 0;
    }

    return CR_OK;
}

CredoError_t bh_fw_get_status(CredoSlice_t* slice, unsigned* status) {
    unsigned r, feature = 0;
    CredoError_t rc = bh_fw_get_feature_inner(slice, &r, &feature);
    *status = (rc == CR_FW_TIMEOUT) || ((r & FW_RESPONSE_FREEZE_MASK) == FW_RESPONSE_FREEZE_ERROR) ? 0 : 1;
    return CR_OK;
}

// TODO, fec engine index should get from firmware but not support, hard code here
CredoError_t bh_fw_get_rsfec_index(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, int* index) {
    CredoPortConfig_t port_config;
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    if (port_config.port_id == CR_PORT_UNCONFIGURED) return CR_INVALID_ARGS;

    *index = port_config.line_start_lane / 2;

    if (side == CR_SIDE_HOST) *index -= 4;

    return CR_OK;
}

CredoError_t bh_fw_get_rsfec_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                  CredoRSFECFifo_t* rsfec_fifo) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    int lane = (side == CR_SIDE_HOST) ? port_config.host_start_lane : port_config.line_start_lane;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_PDIFF_MIN, &rsfec_fifo->rx_min));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_PDIFF, &rsfec_fifo->rx_cur));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_PDIFF_MAX, &rsfec_fifo->rx_max));

    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_PDIFF_MIN, &rsfec_fifo->tx_min));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_PDIFF, &rsfec_fifo->tx_cur));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_PDIFF_MAX, &rsfec_fifo->tx_max));

    return CR_OK;
}

CredoError_t bh_fw_get_gearbox_info(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                    fw_gearbox_info_t* gb_info) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    ERR_PROPS(bh_fw_get_rsfec_fifo(slice, port_id, side, &gb_info->fifo));

    unsigned msb = 0, lsb = 0;
    int lane = (side == CR_SIDE_HOST) ? port_config.host_start_lane : port_config.line_start_lane;

    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_PDIFF_DEV, &gb_info->cfg_pdiff_dev_rx));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_CFG_PDIFF, &gb_info->cfg_pdiff_rx));
    ERR_PROPS(
        hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_CFG_PDIFF_HW_MIN, &gb_info->cfg_pdiff_hw_rx_min));
    ERR_PROPS(
        hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_CFG_PDIFF_HW_MAX, &gb_info->cfg_pdiff_hw_rx_max));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_NCW_AVG_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_NCW_AVG_LSB, &lsb));
    gb_info->ncw_avg_rx = (msb << 16) | lsb;
    if (side == CR_SIDE_LINE) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FEC_BIP_MSB, &msb));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FEC_BIP_LSB, &lsb));
        gb_info->bip_sum_rx = (msb << 16) | lsb;
    }

    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_PDIFF_DEV, &gb_info->cfg_pdiff_dev_tx));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_CFG_PDIFF, &gb_info->cfg_pdiff_tx));
    ERR_PROPS(
        hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_CFG_PDIFF_HW_MIN, &gb_info->cfg_pdiff_hw_tx_min));
    ERR_PROPS(
        hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_CFG_PDIFF_HW_MAX, &gb_info->cfg_pdiff_hw_tx_max));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_NCW_AVG_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_NCW_AVG_LSB, &lsb));
    gb_info->ncw_avg_tx = (msb << 16) | lsb;

    return CR_OK;
}

CredoError_t bh_fw_get_gearbox_error_info(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                          fw_gearbox_error_info_t* gb_err_info) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    unsigned msb = 0, lsb = 0;
    int lane = (side == CR_SIDE_HOST) ? port_config.host_start_lane : port_config.line_start_lane;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_ERROR_CODE, &gb_err_info->gb_error_code));
    ERR_PROPS(
        hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF, &gb_err_info->fail_pdiff_tx));
    ERR_PROPS(
        hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF, &gb_err_info->fail_pdiff_rx));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MIN,
                               &gb_err_info->fail_pdiff_tx_min));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MAX,
                               &gb_err_info->fail_pdiff_tx_max));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MIN,
                               &gb_err_info->fail_pdiff_rx_min));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MAX,
                               &gb_err_info->fail_pdiff_rx_max));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_DEV,
                               &gb_err_info->fail_pdiff_tx_dev));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_DEV,
                               &gb_err_info->fail_pdiff_rx_dev));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_LSB, &lsb));
    gb_err_info->fail_ncw_avg_tx = (msb << 16) | lsb;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_LSB, &lsb));
    gb_err_info->fail_ncw_avg_rx = (msb << 16) | lsb;

    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_FIRST,
                               &gb_err_info->first_fail_pdiff_tx));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_FIRST,
                               &gb_err_info->first_fail_pdiff_rx));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MIN_FIRST,
                               &gb_err_info->first_fail_pdiff_tx_min));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_MAX_FIRST,
                               &gb_err_info->first_fail_pdiff_tx_max));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MIN_FIRST,
                               &gb_err_info->first_fail_pdiff_rx_min));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_MAX_FIRST,
                               &gb_err_info->first_fail_pdiff_rx_max));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_PDIFF_DEV_FIRST,
                               &gb_err_info->first_fail_pdiff_tx_dev));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_PDIFF_DEV_FIRST,
                               &gb_err_info->first_fail_pdiff_rx_dev));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_FIRST_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TX_BG_FAIL_NCW_AVG_FIRST_LSB, &lsb));
    gb_err_info->first_fail_ncw_avg_tx = (msb << 16) | lsb;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_FIRST_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_RX_BG_FAIL_NCW_AVG_FIRST_LSB, &lsb));
    gb_err_info->first_fail_ncw_avg_rx = (msb << 16) | lsb;

    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_BG_FAIL_BIP_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_BG_FAIL_BIP_LSB, &lsb));
    gb_err_info->fail_bip = (msb << 16) | lsb;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_BG_FAIL_BIP_FIRST_MSB, &msb));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_BG_FAIL_BIP_FIRST_LSB, &lsb));
    gb_err_info->first_fail_bip = (msb << 16) | lsb;
    return CR_OK;
}

CredoError_t bh_fw_get_rsfec_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                                  unsigned* uncorr_cw) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    unsigned msb = 0, lsb = 0;
    int lane = (side == CR_SIDE_HOST) ? port_config.host_start_lane : port_config.line_start_lane;

    lsb = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_debug_cmd_ex(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_UNCORR_CW, &lsb, &msb);
    if (ret != CR_OK) {
        return ret;
    }

    *uncorr_cw = (msb << 16) | lsb;

    return CR_OK;
}

CredoError_t bh_fw_get_rsfec_total_codewords(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                             uint64_t* total_bits) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    unsigned msb = 0, mid = 0, lsb = 0;
    int lane = (side == CR_SIDE_HOST) ? port_config.host_start_lane : port_config.line_start_lane;

    lsb = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_debug_cmd_ex(slice, lane, GEARBOX_DEBUG, GEARBOX_DEBUG_TOTAL_BLK_CNTR, &lsb, &mid);
    if (ret != CR_OK) {
        return ret;
    }

    ERR_PROPS(readTop(slice, REG_DATA, &msb));
    *total_bits = ((uint64_t)msb << 32) | (mid << 16) | lsb;

    return CR_OK;
}

CredoError_t bh_fw_set_rsfec_count_freeze(CredoSlice_t* slice, int lane, bool en) {
    unsigned dummy = FW_CMD_LOG_SILENT;
    CredoError_t ret = common_fw_cmd_ex(slice, FW_CMD_RSFEC_FREEZE_CTRL, (en) ? 1 : 0, lane, NULL, &dummy, NULL);
    if (ret != CR_OK) {
        LOGS_WARN("firmware don't support rsfec counter freeze.");
    }
    return CR_OK;
}

CredoError_t bh_fw_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;

    ERR_PROPS(common_fw_em_start(slice, lane, ber_exp, flag));

    bh_slice->em_flags[lane] = flag;

    return CR_OK;
}

CredoError_t bh_fw_eye_monitor_stop(CredoSlice_t* slice, int lane) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;

    ERR_PROPS(common_fw_em_stop(slice, lane));

    bh_slice->em_flags[lane] = 0;
    return CR_OK;
}

CredoError_t bh_fw_eye_monitor_range(CredoSlice_t* slice, int lane, int* vstep_side, int* hstep_side) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    CredoLaneMode_t lane_mode;

    ERR_PROPS(bh_get_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_PAM4) {
        *vstep_side = FW_EM_VSTEP_SIDE_PAM4;
    } else if (lane_mode == CR_LMODE_NRZ) {
        *vstep_side = FW_EM_VSTEP_SIDE_NRZ;
    } else {
        LOGS_ERROR("[FW EM RANGE][%d] mode error, mode %d", lane, lane_mode);
        return CR_FAIL;
    }

    if (bh_slice->em_flags[lane] & CR_EYE_MONITOR_BATHTUB) {
        *hstep_side = 0;
    } else {
        *hstep_side = FW_EM_HSTEP_SIDE;
    }

    return CR_OK;
}

#define IS_FW_FEATURE(bit) ((fw_feature)&bit)
CredoError_t bh_fw_get_raw_cmd_address(CredoSlice_t* slice, unsigned* addr) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    if (!IS_FW_FEATURE(FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG)) return CR_UNSUPPORTED;
    *addr = REG_CMD_RAW_FB4;
    return CR_OK;
}
unsigned bh_fw_cmd_spi_read(CredoSlice_t* slice) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    return IS_FW_FEATURE(FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG) ? FW_CMD_SPI_READ_FB4 : FW_CMD_SPI_READ_OLD;
}
unsigned bh_fw_cmd_spi_write(CredoSlice_t* slice) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    return IS_FW_FEATURE(FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG) ? FW_CMD_SPI_WRITE_FB4 : FW_CMD_SPI_WRITE_OLD;
}
unsigned bh_fw_cmd_spi_gpi(CredoSlice_t* slice) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    return IS_FW_FEATURE(FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG) ? FW_CMD_SPI_GPI_FB4 : FW_CMD_SPI_GPI_OLD;
}
unsigned bh_fw_cmd_spi_status(CredoSlice_t* slice) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    return IS_FW_FEATURE(FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG) ? FW_CMD_SPI_STATUS_FB4 : FW_CMD_SPI_STATUS_OLD;
}
unsigned bh_fw_cmd_spi_erase(CredoSlice_t* slice) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    return IS_FW_FEATURE(FW_FEATURE_NEW_SPI_FW_CMD_AND_ECHO_REG) ? FW_CMD_SPI_ERASE_FB4 : FW_CMD_SPI_ERASE_OLD;
}

CredoError_t bh_fw_get_acfg_status(CredoSlice_t* slice, int* state, int* error_code) {
    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);
    if ((fw_feature & FW_FEATURE_ACFG) == 0) return CR_UNSUPPORTED;

    if (state) ERR_PROPS(hal_fw_debug_cmd(slice, 0, TOP_DEBUG, TOP_DEBUG_ACFG_STATE, (unsigned*)state));
    if (error_code) {
        ERR_PROPS(hal_fw_debug_cmd(slice, 0, TOP_DEBUG, TOP_DEBUG_ACFG_ERROR_CODE, (unsigned*)error_code));
        *error_code = (short)*error_code;
    }
    return CR_OK;
}

CredoError_t bh_fw_ready_post_actions(CredoSlice_t* slice) {
    int state;
    if (bh_fw_get_acfg_status(slice, &state, NULL) == CR_UNSUPPORTED) return CR_OK;

    if (state == TOP_DEBUG_ACFG_STATE_OK) {
        LOGS_INFO("[ACFG] Trigger warm init");
        ERR_PROPS(bh_slice_warm_init(slice));
    }

    return CR_OK;
}
