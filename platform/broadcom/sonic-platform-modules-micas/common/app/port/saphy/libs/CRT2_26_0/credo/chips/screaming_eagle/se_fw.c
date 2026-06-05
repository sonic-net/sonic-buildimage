#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include "common/common_firmware.h"
#include "common/common_reset.h"
#include "swift/swift_serdes.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CredoError_t se_fw_mark_off_lanes(CredoSlice_t* slice, unsigned mask) {
    for (int lane_num = 0; lane_num < slice->desc->lane_count; lane_num++) {
        if (mask & (1 << lane_num)) {
            ERR_PROPS(hal_set_lane_mode(slice, lane_num, CR_LMODE_OFF));
        }
    }
    return CR_OK;
}

static CredoError_t se_fw_get_event_time(CredoSlice_t* slice, int lane, unsigned index, unsigned* sec) {
    unsigned msb = 0, lsb = 0;
    ERR_PROPS(hal_fw_debug_cmd_ex(slice, lane, SE_INFO, index, &msb, &lsb));
    *sec = ((msb << 16) | lsb) * FW_TIME_UNIT;
    return CR_OK;
}

CredoError_t se_fw_get_raw_cmd_address(CredoSlice_t* slice, unsigned* addr) {
    *addr = REG_CMD_RAW;
    return CR_OK;
}

CredoError_t se_fw_get_feature(CredoSlice_t* slice, unsigned* feature) {
    unsigned response = 0, response_param = FW_CMD_LOG_SILENT;
    unsigned feature_en = 0;
    CredoError_t err = hal_fw_cmd(slice, FW_CMD_FEATURE_EN, 0, &response, &response_param);
    feature_en = ((response << 16) + response_param) & 0xffffff;
    if (err != CR_OK || feature_en != FW_FEATURE_ENABLE) {
        *feature = 0;
        return CR_OK;
    }

    ERR_PROPS(hal_fw_cmd(slice, FW_CMD_FEATURE, 0, &response, &response_param));
    *feature = ((response << 16) + response_param) & 0xffffff;

    return CR_OK;
}

CredoError_t se_fw_get_toppll_mode(CredoSlice_t* slice, int* mode) {
    unsigned top_options = 0, toppll_mode = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &top_options));
    toppll_mode = (top_options >> TOP_OPTION_TOPPLL_MODE_OFFSET) & TOP_OPTION_TOPPLL_MODE_MASK;

    switch (toppll_mode) {
        case 1:
            *mode = 0;
            break;
        case 2:
            *mode = 1;
            break;
        case 4:
            *mode = 2;
            break;
        default:
            LOGS_WARN("Firmware toppll mode maybe conflict.");
            *mode = 0xff;
            break;
    }
    return CR_OK;
}

CredoError_t se_fw_set_toppll_mode(CredoSlice_t* slice, int mode) {
    if (mode < 0 || mode > 2) {
        LOGS_ERROR("Firmware toppll mode only support 0, 1 and 2");
        return CR_FAIL;
    }
    unsigned top_options = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &top_options));

    unsigned toppll_mode = ((1 << mode) << TOP_OPTION_TOPPLL_MODE_OFFSET);
    top_options &= ~(TOP_OPTION_TOPPLL_MODE_MASK << TOP_OPTION_TOPPLL_MODE_OFFSET);
    top_options |= toppll_mode;

    ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_OPTIONS, top_options));
    return CR_OK;
}

CredoError_t se_fw_deconfig_cmd(CredoSlice_t* slice, unsigned fw_mode, unsigned detail1) {
    CredoError_t ret = CR_FAIL;
    SeSlice_t* se_sliece = (SeSlice_t*)slice;
    unsigned mask1 = 0, mask2 = 0;
    unsigned cmd = FW_CMD_DESTROY_MODE | fw_mode;

    ERR_PROPS(common_fw_cmd_ex(slice, cmd, detail1, 0, NULL, &mask1, &mask2));

    unsigned destroyed_lane_mask = ((mask1 & 0xFF) << 16) | mask2;
    unsigned destroyed_port = ((mask1 >> 8) & 0xFF);
    LOGS_DEBUG("[Firmware deconfig] lane: 0x%04X, port: 0x%02X", destroyed_lane_mask, destroyed_port);

    unsigned retry_cnt = 0;
    unsigned fw_debug_state[SE_MAX_LANES] = {0};
    unsigned checked_lane = 0;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        if ((destroyed_lane_mask & (1 << lane)) == 0) continue;
        checked_lane = lane;
        break;
    }

    // firmware change all lanes's mode at same time, so check one lane only
    bool fw_deconfig_done = false, fw_state_saved = false;
    do {
        unsigned fw_sel_mode = FW_MODE_OFF;
        ret = se_fw_get_config_mode(slice, checked_lane, &fw_sel_mode);
        if (ret == CR_OK && fw_sel_mode == FW_MODE_OFF) {
            fw_deconfig_done = true;
            break;
        }

        if (fw_deconfig_done == false && retry_cnt == 20) {  // save first failed state after full delay
            for (int lane = 0; lane < slice->desc->lane_count; lane++) {
                if ((destroyed_lane_mask & (1 << lane)) == 0) continue;
                ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE, &fw_debug_state[lane]));
            }
            fw_state_saved = true;
        }

        sleep_ms(FW_DECONFIG_MODE_DELAY / 20);
    } while (fw_deconfig_done != true && retry_cnt++ < 200);

    if (fw_state_saved) {
        LOGS_WARN("[Firmware deconfig] wait mode change over than %d ms", FW_DECONFIG_MODE_DELAY / 20 * retry_cnt);
        for (int lane = 0; lane < slice->desc->lane_count; lane++) {
            LOGS_WARN("[Firmware deconfig] lane %02d, OPT_DEBUG_STATE: %04X", lane, fw_debug_state[lane]);
        }
    }

    if (fw_deconfig_done == false) {
        LOGS_ERROR("[Firmware deconfig] FAIL, destroy lane check mask: %04X", destroyed_lane_mask);
        return CR_FAIL;
    }

    for (int port = 0; port < slice->desc->port_count; port++) {
        if (destroyed_port & (1 << port)) se_sliece->port_info[port].started = false;
    }

    ERR_PROPS(se_fw_mark_off_lanes(slice, destroyed_lane_mask));
    return CR_OK;
}

CredoError_t se_fw_get_info_ver(CredoSlice_t* slice, unsigned* ver) {
    *ver = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_INTERNAL_VER, ver);
    if (err != CR_OK) {
        *ver = 0;
    }

    return CR_OK;
}

CredoError_t se_fw_get_config_mode(CredoSlice_t* slice, int lane, unsigned* config_mode) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_CONFIG_SEL, config_mode));
    return CR_OK;
}

CredoError_t se_fw_get_rate_mode(CredoSlice_t* slice, int lane, unsigned* rate_mode) {
    *rate_mode = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_RATE_MODE, rate_mode);
    if (ret != CR_OK) {
        *rate_mode = FW_RATE_FULL;
    }
    return CR_OK;
}

CredoError_t se_fw_get_tx_rate_mode(CredoSlice_t* slice, int lane, unsigned* rate_mode) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    if (se_slice->uni_tx_port_map[lane] != UNI_PORT_NONE) {
        int source_lane = se_slice->uni_tx_lane_map[lane];
        ERR_PROPS(se_fw_get_rate_mode(slice, source_lane, rate_mode));
        // TODO for uni-bitmux, we can do some conversion
        return CR_OK;
    } else if (se_slice->uni_rx_port_map[lane] != UNI_PORT_NONE) {
        *rate_mode = FW_RATE_FULL;
        return CR_OK;
    }
    return se_fw_get_rate_mode(slice, lane, rate_mode);
}

CredoError_t se_fw_get_cmd_log(CredoSlice_t* slice, unsigned idx, unsigned* cmd) {
    for (int i = 0; i < 7; i++) {
        ERR_PROPS(hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_CMD_LOG + idx * 7 + i, cmd + i));
    }
    return CR_OK;
}

CredoError_t se_fw_get_state_timestamp(CredoSlice_t* slice, int lane, unsigned cnt, unsigned* timestamp) {
    unsigned msb = 0, lsb = 0;
    for (int i = 0; i < cnt; i++) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_TIMESTAMP + i * 2, &msb));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_STATE_TIMESTAMP + i * 2 + 1, &lsb));
        timestamp[i] = (msb << 16) | lsb;
    }
    return CR_OK;
}

CredoError_t se_fw_set_tx_map(CredoSlice_t* slice, unsigned map_a, unsigned map_b) {
    ERR_PROPS(writeTop(slice, REG_DATA + 0, map_a >> 16));
    ERR_PROPS(writeTop(slice, REG_DATA + 1, map_a & 0xFFFF));
    ERR_PROPS(writeTop(slice, REG_DATA + 2, map_b >> 16));
    ERR_PROPS(writeTop(slice, REG_DATA + 3, map_b & 0xFFFF));
    return CR_OK;
}

CredoError_t se_fw_get_tx_map(CredoSlice_t* slice, unsigned* map_a, unsigned* map_b) {
    unsigned reg_data[4] = {0};
    ERR_PROPS(readTop(slice, REG_DATA + 0, reg_data + 0));
    ERR_PROPS(readTop(slice, REG_DATA + 1, reg_data + 1));
    ERR_PROPS(readTop(slice, REG_DATA + 2, reg_data + 2));
    ERR_PROPS(readTop(slice, REG_DATA + 3, reg_data + 3));
    *map_a = (reg_data[0] << 16) | reg_data[1];
    *map_b = (reg_data[2] << 16) | reg_data[3];
    return CR_OK;
}

CredoError_t se_fw_set_tx_map_isc(CredoSlice_t* slice, unsigned map_a, unsigned map_b) {
    ERR_PROPS(writeTop(slice, REG_DATA + 4, map_a >> 16));
    ERR_PROPS(writeTop(slice, REG_DATA + 5, map_a & 0xFFFF));
    ERR_PROPS(writeTop(slice, REG_DATA + 6, map_b >> 16));
    ERR_PROPS(writeTop(slice, REG_DATA + 7, map_b & 0xFFFF));
    return CR_OK;
}

CredoError_t se_fw_get_tx_map_isc(CredoSlice_t* slice, unsigned* map_a, unsigned* map_b) {
    unsigned reg_data[4] = {0};
    ERR_PROPS(readTop(slice, REG_DATA + 4, reg_data + 0));
    ERR_PROPS(readTop(slice, REG_DATA + 5, reg_data + 1));
    ERR_PROPS(readTop(slice, REG_DATA + 6, reg_data + 2));
    ERR_PROPS(readTop(slice, REG_DATA + 7, reg_data + 3));
    *map_a = (reg_data[0] << 16) | reg_data[1];
    *map_b = (reg_data[2] << 16) | reg_data[3];
    return CR_OK;
}

CredoError_t se_fw_get_retimer_state(CredoSlice_t* slice, int lane, unsigned* state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG_RETIMER, RETIMER_DEBUG_STATE, state));
    return CR_OK;
}

CredoError_t se_fw_get_bitmux_state(CredoSlice_t* slice, int lane, unsigned* state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG_BITMUX, BITMUX_DEBUG_STATE, state));
    return CR_OK;
}

CredoError_t se_fw_download(CredoSlice_t* slice, const char* image_file) {
    CredoError_t ret = CR_OK;
    FILE* file = fopen(image_file, "rb");
    if (!file) {
        LOGS_ERROR("[Firmware load] Error opening firmware file %s", image_file);
        return CR_FAIL;
    }

    // se_soft_reset_without_fw_running() include fw_unload()
    ERR_CATCH(ret = se_soft_reset_without_fw_running(slice), goto exit);

    ret = common_fw_load(slice, file);

exit:
    fclose(file);
    return ret;
}

CredoError_t se_fw_phy_ready(CredoSlice_t* slice, unsigned* rdy) {
    ERR_PROPS(readReg(slice, REG_FW_PHY_READY, rdy));
    return CR_OK;
}

CredoError_t se_fw_phy_lane_ready(CredoSlice_t* slice, int lane, unsigned* rdy) {
    unsigned phy_ready = 0;
    ERR_PROPS(se_fw_phy_ready(slice, &phy_ready));
    *rdy = ((phy_ready >> lane) & 0x1);
    return CR_OK;
}

CredoError_t se_fw_tx_ready(CredoSlice_t* slice, unsigned* rdy) {
    ERR_PROP(readReg(slice, REG_FW_PHY_READY, rdy));
    return CR_OK;
}

static CredoError_t se_fw_config_lane_pre_check(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t lane_mode_cur = CR_LMODE_OFF;
    ERR_PROP(hal_get_lane_mode(slice, lane, &lane_mode_cur));
    if (lane_mode_cur == CR_LMODE_DISABLE) {
        LOGS_WARN("[Lane config][%d] Lane already disabled.", lane);
        return CR_FAIL;
    } else if (lane_mode_cur != CR_LMODE_OFF) {
        unsigned speed_cur = 0;
        ERR_PROP(hal_fw_get_lane_speed(slice, lane, &speed_cur));
        LOGS_WARN("[Lane config][%d] Lane already config to speed %u, mode %s.", lane, speed_cur,
                  lane_mode_to_string(lane_mode_cur));
        return CR_FAIL;
    }
    return CR_OK;
}

static CredoError_t se_fw_config_lane_flexspeed(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, double speed,
                                                unsigned* fw_speed) {
    if (speed < 1e9 || speed > 112.5e9) {
        LOGS_ERROR("[Flexspeed][lane=%d] Lane speed must be >=1G and <=112.5G", lane);
        return CR_INVALID_ARGS;
    }
    if (speed < 50e9 && lane_mode != CR_LMODE_NRZ) {
        LOGS_ERROR("[Lane config][%d] Lane mode should be NRZ if speed less than 50G", lane);
        return CR_INVALID_ARGS;
    }
    unsigned fw_feature = 0;
    ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
    if ((fw_feature & FW_FEATURE_SUPPORT_FLEXSPEED) == 0) {
        LOGS_ERROR("Flexspeed unsupported in this firmware");
        return CR_UNSUPPORTED;
    }

    flexspeed_config_t config = se_flexspeed_compute_config(speed, lane_mode);
    LOGS_DEBUG(
        "Flexspeed settings:\ndatarate=%.6fGb/s\nbaudrate=%.6fG\nvcorate=%.6fG\npll_n=%u, "
        "pll_n_frac=%u\ntx_target_count=%u, "
        "rx_target_count=%u",
        speed / 1e9, config.baudrate / 1e9, config.vcorate / 1e9, config.pll_n, config.pll_n_frac,
        config.tx_target_count, config.rx_target_count);
    // unsigned flexspeed_en = 0;
    ERR_PROPS(se_flexspeed_set_registers(slice, lane < HOST_LANES ? CR_SIDE_HOST : CR_SIDE_LINE, &config, true, true));
    *fw_speed = config.fw_base_speed;
    return CR_OK;
}

static CredoError_t se_fw_config_lane_inner(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                                            unsigned detail2) {
    // check current setting
    ERR_PROP(se_fw_config_lane_pre_check(slice, lane));

    // check user setting
    if (speed < CONFIG_50G && lane_mode != CR_LMODE_NRZ) {
        LOGS_ERROR("[Lane config][%d] Lane mode should be NRZ if speed less than 50G", lane);
        return CR_INVALID_ARGS;
    }

    int host_lanes = 0, line_lanes = 0;
    ERR_PROP(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    unsigned detail1 = se_fw_translate_speed(speed);
    if (detail1 == 0xFFFF) {
        LOGS_ERROR("[Lane config][%d] Not supported Speed: %d", lane, speed);
        return CR_INVALID_ARGS;
    }

    unsigned cmd = FW_CMD_CONFIG_MODE;
    cmd |= (FW_MODE_PHY << 4);
    detail1 |= (lane_mode == CR_LMODE_NRZ) ? FW_OPTION_OPT_MODE : 0;
    unsigned tx_map = ((0x8 | ((lane % host_lanes) & 0x7)) << (4 * (lane % host_lanes)));
    unsigned tx_map_a = (lane < host_lanes) ? tx_map : 0;
    unsigned tx_map_b = (lane < host_lanes) ? 0 : tx_map;
    ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
    LOGS_DEBUG("[Lane config] Cmd 0x%04X, detail1 0x%04X, detail2 0x%04X, map_a 0x%08X, map_b 0x%08X", cmd, detail1,
               detail2, tx_map_a, tx_map_b);

    ERR_PROP(common_fw_cmd_ex(slice, cmd, detail1, detail2, NULL, NULL, NULL));
    // sleep_ms(FW_CONFIG_MODE_DELAY);

    se_set_lane_mode(slice, lane, lane_mode);
    return CR_OK;
}

CredoError_t se_fw_config_lane(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed) {
    ERR_PROPS(se_fw_config_lane_inner(slice, lane, lane_mode, speed, 0));
    return CR_OK;
}

static CredoError_t se_fw_config_lane_loopback_inner(CredoSlice_t* slice, int lane, unsigned speed, unsigned detail2) {
    // check current setting
    ERR_PROP(se_fw_config_lane_pre_check(slice, lane));

    unsigned detail1 = 0;
    detail1 = se_fw_translate_speed(speed);
    if (detail1 == 0xFFFF) {
        LOGS_ERROR("Not supported Speed: %d", speed);
        return CR_INVALID_ARGS;
    }

    int host_lanes = 0, line_lanes = 0;
    ERR_PROP(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    unsigned cmd = FW_CMD_CONFIG_MODE;
    cmd |= (FW_MODE_LOOPBACK << 4);
    unsigned tx_map_a = (lane < host_lanes) ? ((0x8 | lane) << (4 * lane)) : 0;
    unsigned tx_map_b = (lane < host_lanes) ? 0 : ((0x8 | (lane & 0x7)) << (4 * (lane % 8)));
    ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
    LOGS_DEBUG("[Lane config] Cmd 0x%04X, detail1 0x%04X, detail2 0x%04X, map_a 0x%08X, map_b 0x%08X", cmd, detail1,
               detail2, tx_map_a, tx_map_b);
    ERR_PROP(common_fw_cmd_ex(slice, cmd, detail1, detail2, NULL, NULL, NULL));
    // sleep_ms(FW_CONFIG_MODE_DELAY);

    CredoLaneMode_t mode = (speed >= CONFIG_50G) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
    ERR_PROPS(hal_set_lane_mode(slice, lane, mode));
    return CR_OK;
}

CredoError_t se_fw_config_lane_loopback(CredoSlice_t* slice, int lane, unsigned speed) {
    ERR_PROPS(se_fw_config_lane_loopback_inner(slice, lane, speed, 0));
    return CR_OK;
}

CredoError_t se_fw_config_phy(CredoSlice_t* slice, int lane, CredoLaneMode_t lane_mode, unsigned speed,
                              uint32_t flags) {
    int host_lanes = 0, line_lanes = 0;
    ERR_PROP(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    unsigned detail2 = 0;
    if (flags & CR_LFLAG_AN) {
        detail2 |= ((lane < host_lanes) ? (FW_OPTION_SYS_SIDE_AN) : (FW_OPTION_LINE_SIDE_AN));
    }
    if (flags & CR_LFLAG_LT) {
        detail2 |= ((lane < host_lanes) ? (FW_OPTION_SYS_SIDE_LT) : (FW_OPTION_LINE_SIDE_LT));
    }

    if (flags & CR_LFLAG_FLEXSPEED) {
        double flexspeed = speed;
        if (flexspeed > 1e6) {
            flexspeed *= 1e3;
        } else if (flexspeed > 1e3) {
            flexspeed *= 1e6;
        }
        ERR_PROPS(se_fw_config_lane_flexspeed(slice, lane, lane_mode, flexspeed, &speed));
    }

    if (flags & CR_LFLAG_LOOPBACK) {
        ERR_PROPS(se_fw_config_lane_loopback_inner(slice, lane, speed, detail2));
    } else {
        ERR_PROPS(se_fw_config_lane_inner(slice, lane, lane_mode, speed, detail2));
    }
    return CR_OK;
}

static CredoError_t se_fw_deconfig_lane_inner(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(se_get_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_OFF) {
        return CR_OK;
    }

    int host_lanes = 0, line_lanes = 0;
    ERR_PROP(hal_get_lane_count(slice, &host_lanes, &line_lanes));

    unsigned fw_mode = FW_MODE_PHY << 4;
    unsigned tx_map_a = (lane < host_lanes) ? ((0x8 | lane) << (4 * lane)) : 0;
    unsigned tx_map_b = (lane < host_lanes) ? 0 : ((0x8 | (lane & 0x7)) << (4 * (lane % 8)));
    ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
    ERR_PROPS(se_fw_deconfig_cmd(slice, fw_mode, 0));

    return CR_OK;
}

CredoError_t se_fw_deconfig_lane(CredoSlice_t* slice, int lane) {
    ERR_PROPS(se_fw_deconfig_lane_inner(slice, lane));
    return CR_OK;
}

static const CredoLaneTxState_t forced[4] = {
    CR_TX_FORCE_DISABLE,
    CR_TX_FORCE_PRBSS_PAM4,
    CR_TX_FORCE_PRBS_NRZ,
    CR_TX_FORCE_TRAFFIC,
};

static const CredoLaneTxState_t normal[4] = {
    CR_TX_SQUELCH,
    CR_TX_PRBS_PAM4,
    CR_TX_PRBS_NRZ,
    CR_TX_TRAFFIC,
};

CredoError_t se_fw_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status) {
    bool tx_pattern_en = false;
    hal_get_tx_test_pattern_enable(slice, lane, &tx_pattern_en);
    if (tx_pattern_en) {
        *status = CR_TX_FORCE_TEST_PATT;
        return CR_OK;
    }

    unsigned tx_status = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_TX_STATUS, &tx_status);
    if (ret != CR_OK) {
        *status = CR_TX_UNKNOWN;
        return CR_OK;
    }

    unsigned state = tx_status & TX_CONTROL_STATE_MASK;
    unsigned forced_state = (tx_status & TX_CONTROL_FORCED_SOURCE_MASK) >> TX_CONTROL_FORCED_SOURCE_SHIFT;
    unsigned normal_state = (tx_status & TX_CONTROL_NORMAL_SOURCE_MASK) >> TX_CONTROL_NORMAL_SOURCE_SHIFT;

    switch (state) {
        case TX_CONTROL_STATE_UNKNOWN:
            *status = CR_TX_UNKNOWN;
            break;
        case TX_CONTROL_STATE_FORCED:
            *status = forced[forced_state];
            break;
        case TX_CONTROL_STATE_NORMAL:
            *status = normal[normal_state];
            break;
        case TX_CONTROL_STATE_LOW_POWER:
            *status = CR_TX_LOWPOWER;
            break;
    }
    return CR_OK;
}

CredoError_t se_fw_tx_control(CredoSlice_t* slice, int lane, FwTxSource_t source) {
    unsigned fw_source_ctl;
    unsigned response;
    if (source == FW_TX_SOURCE_NOFORCE) {
        fw_source_ctl = 0;
    } else {
        fw_source_ctl = ((unsigned)source << 2) | (1 << 0);
    }
    return hal_fw_cmd(slice, FW_CMD_TX_CONTROL + fw_source_ctl, 1 << lane, &response, NULL);
}

static CredoError_t se_fw_rx_control(CredoSlice_t* slice, unsigned lane, FwRxControl_t rx_control) {
    return hal_fw_cmd_ex(slice, FW_CMD_LANE_RESET, rx_control, lane, NULL, NULL, NULL);
}

CredoError_t se_fw_tx_disable(CredoSlice_t* slice, int lane) {
    return se_fw_tx_control(slice, lane, FW_TX_SOURCE_QUIET);
}

CredoError_t se_fw_tx_no_disable(CredoSlice_t* slice, int lane) {
    return se_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
}

CredoError_t se_fw_rx_disable(CredoSlice_t* slice, int lane) {
    return se_fw_rx_control(slice, lane, FW_RX_CONTROL_DISABLE);
}

CredoError_t se_fw_rx_no_disable(CredoSlice_t* slice, int lane) {
    return se_fw_rx_control(slice, lane, FW_RX_CONTROL_ENABLE);
}

CredoError_t se_fw_rx_reset(CredoSlice_t* slice, int lane) {
    return se_fw_rx_control(slice, lane, FW_RX_CONTROL_RESET);
}

CredoError_t se_fw_lane_disable(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t lane_mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (IS_LANE_MODE_PAM4_OR_NRZ(lane_mode)) {
        LOGS_WARN("[Lane config][%d] Lane is using.", lane);
        return CR_FAIL;
    }

    ERR_PROPS(hal_set_lane_mode(slice, lane, CR_LMODE_DISABLE));
    return hal_fw_cmd_ex(slice, FW_CMD_LANE_DISABLE, 0, lane, NULL, NULL, NULL);
}

CredoError_t se_fw_data(CredoSlice_t* slice, int lane, unsigned section, unsigned index, int len, int data[]) {
    unsigned response_param;
    for (int i = 0; i < len; i++) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, section, index + i, &response_param));
        data[i] = response_param & 0x8000 ? (int)response_param - 0x10000 : (int)response_param;
    }
    return CR_OK;
}

CredoError_t se_fw_data_unsigned(CredoSlice_t* slice, int lane, unsigned section, unsigned index, int len,
                                 unsigned data[]) {
    for (int i = 0; i < len; i++) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, section, index + i, &data[i]));
    }
    return CR_OK;
}

CredoError_t se_fw_set_fast_recover_timeout(CredoSlice_t* slice, int lane, int value) {
    ERR_PROP(writeTop(slice, REG_DATA, value));
    ERR_PROPS(
        hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_RECOVER_SD_TIMEOUT, (1 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_get_fast_recover_timeout(CredoSlice_t* slice, int lane, int* value) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_SD_TIMEOUT, (unsigned*)value));
    return CR_OK;
}

CredoError_t se_fw_set_sd_delay(CredoSlice_t* slice, int lane, int value) {
    unsigned dummy = FW_CMD_LOG_SILENT;

    ERR_PROP(writeTop(slice, REG_DATA, value));
    CredoError_t err =
        hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_SD_DELAY, (1 << 8) | lane, NULL, &dummy, NULL);
    if (err != CR_OK) {
        LOGS_WARN("[FW][%d] don't support sd delay setting", lane);
    }
    return CR_OK;
}

CredoError_t se_fw_get_sd_delay(CredoSlice_t* slice, int lane, int* value) {
    unsigned dummy = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_SD_DELAY, &dummy);
    *value = (err == CR_OK) ? dummy : 0;
    return CR_OK;
}

CredoError_t se_fw_set_fec_clk(CredoSlice_t* slice, int lane, int enable) {
    ERR_PROPS(writeTop(slice, REG_DATA, enable));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_TOP, LOAD_SRAM_CLK_EN, (1 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_get_eye_all(CredoSlice_t* slice, int lane, int eyes[]) {
    CredoLaneMode_t mode;
    ERR_PROPS(se_get_lane_mode(slice, lane, &mode));

    int range = 0;
    switch (mode) {
        case CR_LMODE_NRZ:
            range = DSP_EYE_NRZ_COUNT;
            break;
        case CR_LMODE_PAM4:
            range = DSP_EYE_PAM4_COUNT;
            break;
        case CR_LMODE_OFF:
            LOGS_WARN("[Get FW EYE] Lane mode off, return all 0");
            return CR_OK;
        default:
            LOGS_ERROR("[Get FW EYE] Unknown mode %d", mode);
            return CR_FAIL;
    }

    for (int phase = 0; phase < PHASE_NUM; phase++) {
        unsigned eye_idx = phase * range;
        ERR_PROP(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_EYE_ALL + phase * range, range, &eyes[eye_idx]));
    }

    return CR_OK;
}

CredoError_t se_fw_get_eye_phase0(CredoSlice_t* slice, int lane, int eyes[]) {
    CredoLaneMode_t mode;
    ERR_PROPS(se_get_lane_mode(slice, lane, &mode));

    int range = 0;
    switch (mode) {
        case CR_LMODE_NRZ:
            range = DSP_EYE_NRZ_COUNT;
            break;
        case CR_LMODE_PAM4:
            range = DSP_EYE_PAM4_COUNT;
            break;
        case CR_LMODE_OFF:
            LOGS_WARN("[Get FW EYE] Lane mode off, return all 0");
            return CR_OK;
        default:
            LOGS_ERROR("[Get FW EYE] Unknown mode %d", mode);
            return CR_FAIL;
    }

    ERR_PROP(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_EYE_ALL, range, eyes));

    return CR_OK;
}

CredoError_t se_fw_get_eye(CredoSlice_t* slice, int lane, int eyes[3]) {
    CredoLaneMode_t mode;
    eyes[0] = eyes[1] = eyes[2] = 0;

    // return zero for eyes on unconfigured/unready lane
    ERR_PROPS(se_get_lane_mode(slice, lane, &mode));
    if (!IS_LANE_MODE_PAM4_OR_NRZ(mode)) {
        return CR_OK;
    }
    int rdy = 0;
    ERR_PROPS(hal_get_lane_ready(slice, lane, &rdy));

    if (!rdy) {
        return CR_OK;
    }

    if (mode == CR_LMODE_PAM4) {
        ERR_PROP(se_fw_data(slice, lane, OPT_DEBUG_PAM4, OPT_DEBUG_PAM4_EYE, 3, eyes));
    } else if (mode == CR_LMODE_NRZ) {
        ERR_PROP(se_fw_data(slice, lane, OPT_DEBUG_NRZ, OPT_DEBUG_NRZ_EYE, 2, eyes));
    }

    return CR_OK;
}

CredoError_t se_fw_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]) {
    ERR_PROPS(se_fw_data_unsigned(slice, lane, OPT_DEBUG, OPT_DEBUG_FLT_SEL, DSP_FLT_COUNT, flt_sel));
    return CR_OK;
}

CredoError_t se_fw_set_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]) {
    for (int i = 0; i < DSP_FLT_COUNT; i++) {
        ERR_PROPS(writeTop(slice, REG_DATA + 0, i));
        ERR_PROPS(writeTop(slice, REG_DATA + 1, flt_sel[i]));
        ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_FFE_FLT_SEL, (2 << 8) | lane, NULL, NULL, NULL));
    }
    return CR_OK;
}

CredoError_t se_fw_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]) {
    int val[DSP_RX_FFE_COUNT] = {0};
    for (int ph = 0; ph < PHASE_NUM; ph++) {
        ERR_PROPS(se_fw_get_rx_ffe(slice, lane, ph, val));
        memcpy(taps + ph * DSP_RX_FFE_COUNT, val, sizeof(int) * DSP_RX_FFE_COUNT);
    }
    return CR_OK;
}

CredoError_t se_fw_get_rx_ffe(CredoSlice_t* slice, int lane, int phase, int taps[]) {
    int val[8] = {0};

    ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_FFE_CM + phase * 4, 4, val));
    taps[0] = val[3];
    taps[1] = val[2];
    taps[2] = val[1];
    taps[3] = val[0];

    ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_FFE_CP + phase * 8, 8, val));
    memcpy(taps + 4, val, sizeof(int) * 8);

    ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_FFE_CF + phase * 8, 8, val));
    memcpy(taps + 12, val, sizeof(int) * 8);
    return CR_OK;
}

CredoError_t se_fw_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1) {
    ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_FFE_CM, 1, cm1));
    return CR_OK;
}

CredoError_t se_fw_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1) {
    ERR_PROPS(writeTop(slice, REG_DATA + 0, 0));  // 0 means cm1
    ERR_PROPS(writeTop(slice, REG_DATA + 1, cm1));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_FFE_PRE, (2 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_get_isi_all(CredoSlice_t* slice, int lane, double isi[]) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    unsigned phy_ready = 0;
    ERR_PROPS(se_fw_phy_lane_ready(slice, lane, &phy_ready));
    if (phy_ready == 0) {
        LOGS_WARN("[Get FW ISI ALL][%02d] lane not ready.", lane);
        return CR_OK;
    }

    // backward compatible
    unsigned ready_idx, next_idx, isi_idx;
    unsigned dummy = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_ISI_READ_INIT, &dummy);
    if (err != CR_OK) {
        // LOGS_WARN("[Get FW ISI ALL][%02d] firmware version is too old.", lane);
        ready_idx = OPT_DEBUG_ISI_READY_OLD;
        next_idx = OPT_DEBUG_ISI_READY_CLEAR_OLD;
        isi_idx = OPT_DEBUG_ISI_OLD;
    } else {
        ready_idx = OPT_DEBUG_ISI_READY;
        next_idx = OPT_DEBUG_ISI_READY_NEXT_PHS;
        isi_idx = OPT_DEBUG_ISI;
    }

    CredoTime_t start_time = {0};
    int raw_isi[DSP_ISI_COUNT * PHASE_NUM] = {0};
    for (int phase = 0; phase < PHASE_NUM; phase++) {
        unsigned idx = phase * DSP_ISI_COUNT;
        unsigned rdy = 0;
        get_time(&start_time);
        do {
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, ready_idx, &rdy));
            if (rdy != 0) break;
            sleep_ms(150);
        } while (!is_timeout(&start_time, se_slice->fw_isi_timeout * 1000));
        if (rdy == 0) {
            ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, ready_idx, &rdy));
        }
        if (rdy == 0) {
            LOGS_ERROR("[Get FW ISI ALL] phase %d timeout, timeout is %u ms", phase, se_slice->fw_isi_timeout);
            return CR_FAIL;
        }

        ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, isi_idx, DSP_ISI_COUNT, &raw_isi[idx]));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, next_idx, NULL));
    }

    for (int idx = 0; idx < DSP_ISI_COUNT * PHASE_NUM; idx++) {
        isi[idx] = raw_isi[idx] / 100.0;
    }

    return CR_OK;
}

CredoError_t se_fw_get_isi(CredoSlice_t* slice, int lane, double isi[]) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    unsigned phy_ready = 0;
    ERR_PROPS(se_fw_phy_lane_ready(slice, lane, &phy_ready));
    if (phy_ready == 0) {
        LOGS_WARN("[Get FW ISI][%02d] lane not ready.", lane);
        return CR_OK;
    }

    // backward compatible
    unsigned ready_idx, isi_idx;
    unsigned dummy = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_ISI_READ_INIT, &dummy);
    if (err != CR_OK) {
        // LOGS_WARN("[Get FW ISI][%02d] firmware version is too old.", lane);
        ready_idx = OPT_DEBUG_ISI_READY_OLD;
        isi_idx = OPT_DEBUG_ISI_OLD;
    } else {
        ready_idx = OPT_DEBUG_ISI_READY;
        isi_idx = OPT_DEBUG_ISI;
    }

    int raw_isi[DSP_ISI_COUNT] = {0};
    CredoTime_t start_time = {0};
    unsigned rdy = 0;
    get_time(&start_time);
    do {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, ready_idx, &rdy));
        if (rdy != 0) break;
        sleep_ms(150);
    } while (!is_timeout(&start_time, se_slice->fw_isi_timeout * 1000));
    if (rdy == 0) {
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, ready_idx, &rdy));
    }
    if (rdy == 0) {
        LOGS_ERROR("[Get FW ISI] timeout, timeout is %u ms", se_slice->fw_isi_timeout);
        return CR_FAIL;
    }

    ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, isi_idx, DSP_ISI_COUNT, raw_isi));

    for (int idx = 0; idx < DSP_ISI_COUNT; idx++) {
        isi[idx] = raw_isi[idx] / 100.0;
    }
    return CR_OK;
}

CredoError_t se_fw_get_dc_sar(CredoSlice_t* slice, int lane, int dc_sar[]) {
    ERR_PROP(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_DC_SAR, DSP_SAR_COUNT, dc_sar));
    return CR_OK;
}

CredoError_t se_fw_get_gain_sar(CredoSlice_t* slice, int lane, unsigned gain_sar[]) {
    ERR_PROP(se_fw_data_unsigned(slice, lane, OPT_DEBUG, OPT_DEBUG_GAIN_SAR, DSP_SAR_COUNT, gain_sar));
    return CR_OK;
}

CredoError_t se_fw_get_channel_est_psd(CredoSlice_t* slice, int lane, double psd[]) {
    unsigned raw_psd = 0;
    ERR_PROP(se_fw_data_unsigned(slice, lane, OPT_DEBUG, OPT_DEBUG_CHANNEL_LOSS_PSD0, 1, &raw_psd));
    psd[0] = (double)(raw_psd / 1000.0f);

    ERR_PROP(se_fw_data_unsigned(slice, lane, OPT_DEBUG, OPT_DEBUG_CHANNEL_LOSS_PSD1, 1, &raw_psd));
    psd[1] = (double)(raw_psd / 1000.0f);

    return CR_OK;
}

CredoError_t se_fw_get_channel_est(CredoSlice_t* slice, int lane, double* chan_est) {
    unsigned chan_loss = 0;
    ERR_PROP(se_fw_data_unsigned(slice, lane, OPT_DEBUG, OPT_DEBUG_CHANNEL_LOSS, 1, &chan_loss));
    *chan_est = (double)(chan_loss / 1000.0f);

    return CR_OK;
}

CredoError_t se_fw_get_preset_index(CredoSlice_t* slice, int lane, unsigned* preset_index) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_PRESET_INDEX, preset_index));
    return CR_OK;
}

CredoError_t se_fw_get_amp(CredoSlice_t* slice, int lane, unsigned amp[]) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_AMP_P, amp + 0));
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_AMP_N, amp + 1));

    return CR_OK;
}

CredoError_t se_fw_get_rx_state(CredoSlice_t* slice, int lane, unsigned* rx_state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_RX_STATE, rx_state));
    return CR_OK;
}

CredoError_t se_fw_get_error_state(CredoSlice_t* slice, int lane, unsigned* error_state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_ERROR_STATE, error_state));
    return CR_OK;
}

CredoError_t se_fw_get_opt_mode(CredoSlice_t* slice, int lane, unsigned* opt_mode) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_OPT_MODE, opt_mode));
    return CR_OK;
}

CredoError_t se_fw_get_sd(CredoSlice_t* slice, int lane, int* sd) {
    *sd = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_SDT_SIGNAL, (unsigned*)sd);
    if (err != CR_OK) {
        // for old firmware don't support OPT_DEBUG_SDT_SIGNAL
        unsigned sdt_p = 0, sdt_n = 0;
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_SDT_P, &sdt_p));
        ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_SDT_N, &sdt_n));
        *sd = (sdt_p == 1 && sdt_n == 1) ? 1 : 0;
    }

    return CR_OK;
}

CredoError_t se_fw_get_agc_att(CredoSlice_t* slice, int lane, int* agc_att) {
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_AGC_ATT, 1, agc_att));
    return CR_OK;
}

CredoError_t se_fw_get_agc_gain(CredoSlice_t* slice, int lane, int* agc_gain) {
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_AGC_GAIN, 4, agc_gain));
    return CR_OK;
}

CredoError_t se_fw_get_agc_degen(CredoSlice_t* slice, int lane, int* agc_degen) {
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_AGC_DEGEN, 2, agc_degen));
    return CR_OK;
}

CredoError_t se_fw_get_cdfl_env(CredoSlice_t* slice, int lane, int* cdfl_env) {
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_CDFL_ENV, 2, cdfl_env));
    return CR_OK;
}

CredoError_t se_fw_get_dfe_f0(CredoSlice_t* slice, int lane, unsigned* dfe_f0) {
    ERR_PROPS(se_fw_data_unsigned(slice, lane, OPT_DEBUG, OPT_DEBUG_DFE_F0, 1, dfe_f0));
    return CR_OK;
}

CredoError_t se_fw_set_dfe_f0(CredoSlice_t* slice, int lane, unsigned dfe_f0) {
    ERR_PROPS(writeTop(slice, REG_DATA, dfe_f0));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_DFE_F0, (1 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_get_dfe_f1(CredoSlice_t* slice, int lane, int* dfe_f1) {
    ERR_PROPS(se_fw_data(slice, lane, OPT_DEBUG, OPT_DEBUG_DFE_F1, 1, dfe_f1));
    return CR_OK;
}

CredoError_t se_fw_set_dfe_f1(CredoSlice_t* slice, int lane, int dfe_f1) {
    ERR_PROPS(writeTop(slice, REG_DATA, dfe_f1));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_DFE_F1, (1 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_get_dfe(CredoSlice_t* slice, int lane, int dfe[]) {
    ERR_PROPS(se_fw_get_dfe_f0(slice, lane, (unsigned*)(dfe + 0)));
    ERR_PROPS(se_fw_get_dfe_f1(slice, lane, dfe + 1));
    return CR_OK;
}

CredoError_t se_fw_set_dfe(CredoSlice_t* slice, int lane, int dfe[]) {
    ERR_PROPS(se_fw_set_dfe_f0(slice, lane, (unsigned)dfe[0]));
    ERR_PROPS(se_fw_set_dfe_f1(slice, lane, dfe[1]));
    return CR_OK;
}

CredoError_t se_fw_get_loss(CredoSlice_t* slice, int lane, int* loss) {
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_LOSS, 2, loss));
    return CR_OK;
}

CredoError_t se_fw_get_skef(CredoSlice_t* slice, int lane, int* skef) {
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_SKEF, 4, skef));
    return CR_OK;
}

CredoError_t se_fw_get_tx_ffe(CredoSlice_t* slice, int lane, int* tx_ffe) {
    unsigned range = 7;
    ERR_PROPS(se_fw_data(slice, lane, TOP_INFO, TOP_INFO_TX_FFE, range, tx_ffe));
    return CR_OK;
}

CredoError_t se_fw_get_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn) {
    unsigned val = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_DC_CMN, &val);

    if (ret == CR_OK) {
        *dc_cmn = val & 0x8000 ? (int)val - 0x10000 : (int)val;
    } else {
        *dc_cmn = 0;
    }
    return ret;
}

CredoError_t se_fw_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_VGA, vga));
    return CR_OK;
}

CredoError_t se_fw_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga) {
    ERR_PROPS(writeTop(slice, REG_DATA, vga));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_VGA, (1 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_set_rx_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    unsigned val = 0;
    for (int i = 0; i < DSP_AGCGAIN_COUNT; i++) {
        if (agcgain[i] == 63 && i != (DSP_AGCGAIN_COUNT - 1)) continue;
        val = (i << 6) | agcgain[i];
    }
    ERR_PROPS(writeTop(slice, REG_DATA, val));
    ERR_PROPS(hal_fw_cmd_ex(slice, FW_CMD_STATE_LOAD_SERDES, LOAD_AGCGAIN, (1 << 8) | lane, NULL, NULL, NULL));
    return CR_OK;
}

CredoError_t se_fw_get_status(CredoSlice_t* slice, unsigned* status) {
    CredoError_t rc;
    unsigned cmd = FW_CMD_INFO + TOP_INFO;
    unsigned r;
    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL2, 0));
    ERR_PROPS(writeReg(slice, REG_CMD_DETAIL, TOP_INFO_OPT_MODE));
    ERR_PROPS(writeReg(slice, REG_CMD, cmd));
    rc = common_wait_fw_cmd(slice, cmd, &r, slice->data->fw_cmd_timeout);
    *status = 1;
    if (rc == CR_FW_TIMEOUT) *status = 0;
    if ((r & FW_RESPONSE_FREEZE_MASK) == FW_RESPONSE_FREEZE_ERROR) *status = 0;

    return CR_OK;
}

CredoError_t se_fw_get_link_time(CredoSlice_t* slice, int lane, unsigned* up, unsigned* down) {
    ERR_PROPS(se_fw_get_event_time(slice, lane, SE_INFO_LANE_UP_TIME, up));
    ERR_PROPS(se_fw_get_event_time(slice, lane, SE_INFO_LANE_DOWN_TIME, down));
    return CR_OK;
}

CredoError_t se_fw_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    unsigned speed_index;

    ERR_PROPS(common_fw_get_speed_index(slice, lane, &speed_index));
    *speed_kbps = se_fw_speed_kbps(speed_index);

    unsigned fw_feature = 0;
    ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
    if ((fw_feature & FW_FEATURE_SUPPORT_FLEXSPEED) == 0) {
        return CR_OK;
    }
    unsigned flexspeed_lane_map = 0;
    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_FLEXSPEED_EN_RX, &flexspeed_lane_map));

    if ((flexspeed_lane_map & (1 << lane)) != 0) {
        unsigned pll_n, pll_n_frac;
        ERR_PROPS(hal_fw_debug_cmd_ex(slice, lane, OPT_DEBUG, OPT_DEBUG_RX_PLL_CLK, &pll_n_frac, &pll_n));
        double full_speed = se_flexspeed_compute_speed((*speed_kbps) * 1e3, pll_n, pll_n_frac);
        *speed_kbps = (uint32_t)(full_speed / 1000);
    }

    return CR_OK;
}

CredoError_t se_fw_get_lt_state(CredoSlice_t* slice, int lane, unsigned* lt_state) {
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_LT_STATE, lt_state));
    return CR_OK;
}

CredoError_t se_fw_get_an_state(CredoSlice_t* slice, int lane, unsigned* an_state) {
    return common_fw_debug_cmd(slice, lane, ANLT_DEBUG, ANLT_DEBUG_AN_STATE, an_state);
}

CredoError_t se_fw_get_an_pages(CredoSlice_t* slice, int lane, uint64_t* tx_pages, uint64_t* rx_pages) {
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

CredoError_t se_fw_get_anlt(CredoSlice_t* slice, int lane, unsigned* an_on, unsigned* lt_on) {
    unsigned anlt = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_ANLT_ON, &anlt));
    if (an_on) *an_on = (anlt & 0x1);
    if (lt_on) *lt_on = (anlt & 0x2) >> 1;
    return CR_OK;
}

CredoError_t se_fw_TRF_control(CredoSlice_t* slice, unsigned lane, TRF_t source) {
    unsigned trf_ctrl = (source == TRF_NOFORCE) ? 0 : ((unsigned)source << 2) | (1 << 0);
    return hal_fw_cmd_ex(slice, FW_CMD_TRF_CONTROL, trf_ctrl, lane, NULL, NULL, NULL);
}

CredoError_t se_fw_PLL_source_control(CredoSlice_t* slice, unsigned lane, int force) {
    unsigned tx_pi_ctrl = force ? (lane << 2) | (1 << 0) : 0;
    return hal_fw_cmd_ex(slice, FW_CMD_TX_PI_CONTROL, tx_pi_ctrl, lane, NULL, NULL, NULL);
}

CredoError_t se_fw_force_loopback(CredoSlice_t* slice, unsigned lane, unsigned force) {
    return hal_fw_cmd_ex(slice, FW_CMD_FORCE_LOOPBACK, force ? 1 : 0, lane, NULL, NULL, NULL);
}

CredoError_t se_fw_t2r_force_loopback(CredoSlice_t* slice, unsigned lane, unsigned force) {
    return hal_fw_cmd_ex(slice, FW_CMD_T2R_FORCE_LOOPBACK, force ? 1 : 0, lane, NULL, NULL, NULL);
}

CredoError_t se_fw_switchover_switch(CredoSlice_t* slice, unsigned new_active_map, unsigned old_active_map) {
    LOGS_DEBUG("[FW Switchover switch Active Standby lanes] new 0x%04X, old 0x%04X", new_active_map, old_active_map);
    return hal_fw_cmd_ex(slice, FW_CMD_SWITCHOVER_RETIMER_SWITCH, new_active_map, old_active_map, NULL, NULL, NULL);
}

CredoError_t se_fw_testpoint_read(CredoSlice_t* slice, unsigned* vsensor) {
    SeSlice_t* seslice = (SeSlice_t*)slice;
    CredoTestPoint_t* testpoint = &seslice->testpoint;
    unsigned detail1 = 0;

    CredoError_t ret = CR_FAIL;
    if (seslice->testpoint_configured == true) {
        detail1 |= seslice->vsensor_resolution << FW_VSENSOR_RES_OFFSET;
        detail1 |= testpoint->group << FW_VSENSOR_GROUP_OFFSET;
        detail1 |= testpoint->index << FW_VSENSOR_INDEX_OFFSET;
        detail1 |= testpoint->mode << FW_VSENSOR_MODE_OFFSET;
        detail1 |= testpoint->div2 ? 1 : 0;
    } else {
        detail1 = FW_VSENSOR_READ;
    }

    unsigned retry_cnt = 0;
    do {
        ret = hal_fw_cmd_ex(slice, FW_CMD_VSENSOR_TESTPOINT_SEL_READ, detail1, testpoint->lane, NULL, NULL, NULL);
        if (ret == CR_OK) break;
    } while (retry_cnt++ < 5);

    if (ret != CR_OK) {
        LOGS_ERROR("[FW TSTP read] return error: 0x%03x(%s)", ret, fw_errorcodes_to_string(ret));
        return ret;
    }

    ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_VSENSOR, vsensor));
    return CR_OK;
}

CredoError_t se_fw_eye_monitor_range(CredoSlice_t* slice, int lane, int* vstep_side, int* hstep_side) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    CredoLaneMode_t lane_mode;

    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (se_slice->em_vstep_side != 0) {
        *vstep_side = se_slice->em_vstep_side;
    } else if (lane_mode == CR_LMODE_PAM4) {
        *vstep_side = FW_EM_VSTEP_SIDE_PAM4;
    } else if (lane_mode == CR_LMODE_NRZ) {
        *vstep_side = FW_EM_VSTEP_SIDE_NRZ;  // TODO, need check
    } else {
        LOGS_ERROR("[FW EM RANGE][%d] mode error, mode %d", lane, lane_mode);
        return CR_FAIL;
    }

    if (se_slice->em_info[lane].flags & CR_EYE_MONITOR_BATHTUB) {
        *hstep_side = 0;
    } else {
        *hstep_side = FW_EM_HSTEP_SIDE;
    }

    return CR_OK;
}

CredoError_t se_fw_eye_monitor_start(CredoSlice_t* slice, int lane, int ber_exp, int flag) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    if (!IS_LANE_MODE_PAM4_OR_NRZ(mode)) {
        LOGS_ERROR("Lane %d is not in pam4 or nrz mode", lane);
        return CR_FAIL;
    }

    FirmwareEMState_t em_state;
    unsigned cmd, response;
    unsigned ph_sel = (unsigned)se_slice->em_info[lane].phase_base;

    cmd = FW_CMD_EYE_MON_START | ((ber_exp & 0xf) << 4) | (flag & 0x3);
    ERR_PROPS(hal_fw_cmd_ex(slice, cmd, (lane | ((ph_sel & 0xff) << 8)), (ph_sel >> 8), &response, NULL, NULL));

    em_state = (response >> 8) & 0xf;
    if (em_state != FW_EM_SUCCESS) {
        LOGS_WARN("[FW EM START][%d] EM start error, state %d", lane, em_state);
    }

    se_slice->em_info[lane].flags = flag;
    return CR_OK;
}

CredoError_t se_fw_eye_monitor_stop(CredoSlice_t* slice, int lane) {
    FirmwareEMState_t em_state;
    unsigned cmd, response = 0;

    // firmware return error currently, skip error check
    cmd = FW_CMD_EYE_MON_STOP;
    hal_fw_cmd(slice, cmd, lane, &response, NULL);
    em_state = response & 0xf;
    if (em_state != FW_EM_ERROR_CANCELL && em_state != FW_EM_ERROR_NOT_START) {
        LOGS_WARN("[FW EM STOP][%d] EM stop error, state %d", lane, em_state);
    }

    SeSlice_t* se_slice = (SeSlice_t*)slice;
    se_slice->em_info[lane] = (SeEyeMonitorInfo_t){0};
    return CR_OK;
}

CredoError_t se_fw_eye_monitor_progress(CredoSlice_t* slice, int lane, int* percent) {
    FirmwareEMState_t em_state;
    unsigned cmd, response;

    cmd = FW_CMD_EYE_MON_PROG;
    ERR_PROPS(hal_fw_cmd(slice, cmd, lane, &response, NULL));

    em_state = (response >> 8) & 0xf;
    if (em_state != FW_EM_PROG_REPORT) {
        LOGS_WARN("[FW EM PROG][%d] EM seems not start yet, state %d", lane, em_state);
    }

    *percent = response & 0xff;

    return CR_OK;
}

CredoError_t se_fw_eye_monitor_data(CredoSlice_t* slice, int lane, int** data, int* extent_mv) {
    CredoError_t ret;
    unsigned cmd, response, response2;
    int info[16];
    int vstep_side, hstep_side, vstep_full;

    ERR_PROPS(hal_fw_eye_monitor_range(slice, lane, &vstep_side, &hstep_side));
    vstep_full = vstep_side * 2 + 1;

    for (int phase = -hstep_side, pidx = 0; phase <= hstep_side; phase++, pidx++) {
        for (int margin = -vstep_side; margin <= vstep_side; margin += 16) {
            cmd = FW_CMD_EYE_MON_READ | (phase & 0xFF);
            ret = hal_fw_cmd(slice, cmd, margin & 0xFFFF, &response, &response2);
            if (ret != CR_OK) {
                switch (response & 0xFF) {
                    case FW_EM_ERROR_GOING_ON:
                        LOGS_ERROR("[FW EM READ][%d] EM is still running", lane);
                        return CR_FAIL;
                    case FW_EM_ERROR_NOT_START:
                        LOGS_ERROR("[FW EM READ][%d] EM did not start", lane);
                        return CR_FAIL;
                }
                return ret;
            }

            ERR_PROPS(common_fw_info_data(slice, 16, info));
            for (int i = 0; i < 16; i++) {
                int m = margin + vstep_side + i;
                if (m < vstep_full) {
                    data[m][pidx] = info[i];
                }
            }
        }
    }

    *extent_mv = response2;

    return CR_OK;
}

CredoError_t se_fw_eye_monitor_separator(CredoSlice_t* slice, int separator[5]) {
    ERR_PROPS(hal_fw_cmd(slice, FW_CMD_EYE_MON_READ, FW_EM_VSTEP_SEPARATOR, NULL, NULL));
    ERR_PROPS(common_fw_info_data(slice, 5, separator));
    return CR_OK;
}

CredoError_t se_fw_spiflash_status(CredoSlice_t* slice, unsigned* status) {
    ERR_PROPS(hal_fw_cmd(slice, FW_CMD_SPI_STATUS, 0, NULL, status));
    return CR_OK;
}

#define SPI_FLASH_STATUS_READY_TIMEOUT (500)  // ms
static CredoError_t se_fw_spiflash_cmd(CredoSlice_t* slice, unsigned cmd, unsigned addr_lsb, unsigned addr_msb,
                                       unsigned* response, unsigned* response_param1, unsigned* response_param2) {
    unsigned res, res1, res2;
    CredoError_t ret = CR_FAIL;
    CredoTime_t start_time;
    get_time(&start_time);

    do {
        ERR_PROPS(hal_fw_cmd_ex(slice, cmd, addr_lsb, addr_msb, &res, &res1, &res2));
        // 0xff: flash busy
        if ((res & 0xff) != 0xff) {
            ret = CR_OK;
            goto exit;
        }
    } while (us_passed(&start_time) < (SPI_FLASH_STATUS_READY_TIMEOUT * 1000));

    LOGS_ERROR("spiflash timeout after %u msecs", (unsigned)(us_passed(&start_time) / 1000));

exit:
    if (response) *response = res;
    if (response_param1) *response_param1 = res1;
    if (response_param2) *response_param2 = res2;

    return ret;
}

CredoError_t se_fw_spiflash_erase(CredoSlice_t* slice, unsigned addr) {
    ERR_PROPS(se_fw_spiflash_cmd(slice, FW_CMD_SPI_ERASE, addr & 0xFFFF, (addr >> 16) & 0xFFFF, NULL, NULL, NULL));
    sleep_ms(100);
    return CR_OK;
}

#define MAX_SECTION_COUNT (8 / 2)  // FIXME: firmware bug
CredoError_t se_fw_spiflash_read(CredoSlice_t* slice, unsigned addr, unsigned* buffer, unsigned count) {
    unsigned sec_count = 0;
    unsigned buf_idx = 0;
    unsigned char* buf = (unsigned char*)buffer;

    if (addr & 0x3) {
        LOGS_ERROR("addr 0x%08x must be aligned 4", addr);
        return CR_INVALID_ARGS;
    }

    while (count != 0) {
        unsigned val;

        sec_count = (count > MAX_SECTION_COUNT) ? MAX_SECTION_COUNT : count;
        ERR_PROPS(se_fw_spiflash_cmd(slice, FW_CMD_SPI_READ, addr & 0xFFFF, (addr >> 16) & 0xFFFF, NULL, NULL, NULL));

        for (int i = 0; i < sec_count; i++) {
            ERR_PROPS(readTop(slice, REG_DATA + i * 2 + 0, &val));
            buf[buf_idx++] = (val >> 8) & 0xFF;
            buf[buf_idx++] = (val >> 0) & 0xFF;
            ERR_PROPS(readTop(slice, REG_DATA + i * 2 + 1, &val));
            buf[buf_idx++] = (val >> 8) & 0xFF;
            buf[buf_idx++] = (val >> 0) & 0xFF;
        }

        count -= sec_count;
        addr += sec_count * 4;
    }

    return CR_OK;
}

CredoError_t se_fw_spiflash_write(CredoSlice_t* slice, unsigned addr, const unsigned* buffer, unsigned count) {
    unsigned sec_count;
    unsigned buf_idx = 0;
    unsigned char* buf = (unsigned char*)buffer;

    // LOGS_WARN("addr 0x%08X, count %d", addr, count);
    if (addr & 0x3) {
        LOGS_ERROR("addr 0x%08x must be aligned 4", addr);
        return CR_INVALID_ARGS;
    }
    while (count != 0) {
        sec_count = (count > MAX_SECTION_COUNT) ? MAX_SECTION_COUNT : count;

        for (int i = 0; i < sec_count; i++) {
            ERR_PROPS(writeTop(slice, REG_DATA + i * 2 + 0, (buf[buf_idx + 0] << 8) | (buf[buf_idx + 1] << 0)));
            ERR_PROPS(writeTop(slice, REG_DATA + i * 2 + 1, (buf[buf_idx + 2] << 8) | (buf[buf_idx + 3] << 0)));
            buf_idx += 4;
        }

        ERR_PROPS(se_fw_spiflash_cmd(slice, FW_CMD_SPI_WRITE + sec_count - 1, (addr & 0xFFFF), (addr >> 16) & 0xFFFF,
                                     NULL, NULL, NULL));

        count -= sec_count;
        addr += sec_count * 4;
    }

    return CR_OK;
}

static CredoError_t se_fw_spiflash_write_after_erase(CredoSlice_t* slice, unsigned addr, const unsigned* buffer,
                                                     unsigned count) {
    for (int i = 0; i < count; i++) {
        if ((addr + i * 4) % 4096 == 0) ERR_PROPS(se_fw_spiflash_erase(slice, addr + i * 4));
    }

    ERR_PROPS(se_fw_spiflash_write(slice, addr, buffer, count));
    return CR_OK;
}

/*              SPI Flash Layout
 *  0x000000  +-------------------+
 *            | Boot record table |
 *            |   8 KiB           |
 *  0x002000  +-------------------+
 *            | Slot 0 Firmware   |
 *            |   256 KiB         |
 *  0x042000  +-------------------+
 *            | Slot 1 Firmware   |
 *            |   256 KiB         |
 *  0x082000  +-------------------+
 *            | Slot 2 Firmware   |
 *            |   256 KiB         |
 *  0x0C2000  +-------------------+
 *            | Slot 3 Firmware   |
 *            |   256 KiB         |
 *  0x102000  +-------------------+
 *            | Slot 4 Firmware   |
 *            |   256 KiB         |
 *  0x142000  +-------------------+
 */

#define MAX_SLOT_CNT 5
typedef struct {
    uint32_t checksum[8]; /* 32B, SHA256 sum */
    uint8_t revision[4];  /*  4B, firmware revision */
    uint32_t img_src;     /*  4B, firmware image flash load address (flash addressing) */
    uint32_t img_size;    /*  4B, firmware image size in bytes */
    uint32_t rsvd;        /*  4B, reserved */
} SlotInfo_t;             /* total = 48B */

typedef struct {
    uint32_t tag;                  /*  4B, 'CRDO' */
    uint32_t history;              /*  4B - largest history number will be loaded */
    SlotInfo_t slot[MAX_SLOT_CNT]; /* 240B, 48*5 = 240 */
    uint32_t active_slot;          /*  4B, active slot number */
    uint32_t checksum;             /*  4B, checksum of the FwInfo so that the
                                       sum in uint32 is 0 */
} FWInfo_t;

static CredoError_t se_spiflash_update_fwinfo_checksum(FWInfo_t* fwinfo) {
    uint32_t* p = (uint32_t*)fwinfo;
    uint32_t sum = 0;
    for (int i = 0; i < sizeof(FWInfo_t) / 4 - 1; i++) {
        sum += *p++;
    }
    fwinfo->checksum = 0xFFFFFFFF - sum + 1;
    return CR_OK;
}

#define CRDO_MAGIC 0x4352444f /* CRDO */
static CredoError_t se_spiflash_build_fwinfo(FWInfo_t* fwinfo, uint32_t history, uint32_t active_slot) {
    fwinfo->tag = CRDO_MAGIC;
    fwinfo->history = (history == 0xFFFFFFFF) ? 1 : history;
    fwinfo->active_slot = active_slot;
    return CR_OK;
}

static CredoError_t se_spiflash_build_slotinfo(SlotInfo_t* slotinfo, uint32_t sha[8], uint8_t revision[4],
                                               uint32_t img_src, uint32_t img_size) {
    if (sha) memcpy(slotinfo->checksum, sha, sizeof(slotinfo->checksum));
    if (revision) memcpy(slotinfo->revision, revision, sizeof(slotinfo->revision));
    slotinfo->img_src = img_src;
    slotinfo->img_size = img_size;
    return CR_OK;
}

#define SWAP(m, n) \
    do {           \
        m ^= n;    \
        n ^= m;    \
        m ^= n;    \
    } while (0)
#define ENDIAN_CONVERT(x)                       \
    do {                                        \
        size_t bytes = sizeof(x);               \
        uint8_t* m = (uint8_t*)&(x);            \
        uint8_t* n = m + bytes - 1;             \
        for (int i = 0; i < (bytes / 2); i++) { \
            SWAP(*m, *n);                       \
            m += 1;                             \
            n -= 1;                             \
        }                                       \
    } while (0)
#define BIG2LITTLE(x) ENDIAN_CONVERT(x)
#define LITTLE2BIG(x) ENDIAN_CONVERT(x)

static CredoError_t fwinfo_endian_convert(FWInfo_t* fwinfo) {
    unsigned* p = (unsigned*)fwinfo;
    for (int i = 0; i < sizeof(FWInfo_t) / sizeof(unsigned); i++) ENDIAN_CONVERT(p[i]);

    return CR_OK;
}

#define NUM_BOOT_RECORD_ENTRY       32  // (8 KiB / 256 B)
#define BOOT_RECORD_ENTRY_ADDR(idx) (((idx) % NUM_BOOT_RECORD_ENTRY) * sizeof(FWInfo_t))

static CredoError_t se_fw_spiflash_search_boot_record_table(CredoSlice_t* slice, int* entry_index) {
    uint32_t max_history = 0;
    int max_entry = 0;
    for (int i = 0; i < NUM_BOOT_RECORD_ENTRY; i++) {
        uint32_t history;
        ERR_PROPS(se_fw_spiflash_read(slice, BOOT_RECORD_ENTRY_ADDR(i) + 4, (unsigned*)&history, 1));
        ENDIAN_CONVERT(history);
        if ((history != 0xFFFFFFFF) && (history != 0) && (history > max_history)) {
            FWInfo_t fwinfo;
            ERR_PROPS(se_fw_spiflash_read(slice, BOOT_RECORD_ENTRY_ADDR(i), (unsigned*)&fwinfo,
                                          sizeof(FWInfo_t) / sizeof(unsigned)));
            ERR_PROPS(fwinfo_endian_convert(&fwinfo));
            uint32_t checksum = fwinfo.checksum;
            ERR_PROPS(se_spiflash_update_fwinfo_checksum(&fwinfo));
            if ((fwinfo.checksum == checksum) && (fwinfo.tag == CRDO_MAGIC) && (fwinfo.active_slot < MAX_SLOT_CNT)) {
                max_history = history;
                max_entry = i;
            }
        }
    }

    // if not found, flash empty, set entry_index = 0
    *entry_index = max_entry;
    LOGS_TRACE("boot record entry %d", *entry_index);
    return CR_OK;
}

static CredoError_t se_fw_spiflash_read_boot_record_table(CredoSlice_t* slice, FWInfo_t* fwinfo, int* entry) {
    ERR_PROPS(se_fw_spiflash_search_boot_record_table(slice, entry));
    ERR_PROPS(se_fw_spiflash_read(slice, BOOT_RECORD_ENTRY_ADDR(*entry), (unsigned*)fwinfo,
                                  sizeof(FWInfo_t) / sizeof(unsigned)));
    ERR_PROPS(fwinfo_endian_convert(fwinfo));
    return CR_OK;
}

static bool __spiflash_data_erased(unsigned* ptr, int count) {
    for (int i = 0; i < count; i++)
        if (*ptr++ != 0xffffffff) return false;
    return true;
}

static CredoError_t se_fw_spiflash_write_boot_record_table(CredoSlice_t* slice, FWInfo_t* fwinfo, int entry) {
    unsigned flash_start_addr = BOOT_RECORD_ENTRY_ADDR(entry) & 0x1000;

    for (int i = 0; i < NUM_BOOT_RECORD_ENTRY / 2; i++) {
        FWInfo_t tmp;
        ERR_PROPS(se_fw_spiflash_read(slice, BOOT_RECORD_ENTRY_ADDR(entry + i), (unsigned*)&tmp,
                                      sizeof(FWInfo_t) / sizeof(unsigned)));
        if (__spiflash_data_erased((unsigned*)&tmp, sizeof(FWInfo_t) / sizeof(unsigned))) {
            flash_start_addr = BOOT_RECORD_ENTRY_ADDR(entry + i);
            break;
        }
    }

    ERR_PROPS(se_spiflash_update_fwinfo_checksum(fwinfo));
    ERR_PROPS(fwinfo_endian_convert(fwinfo));
    ERR_PROPS(se_fw_spiflash_write_after_erase(slice, flash_start_addr, (unsigned*)fwinfo,
                                               sizeof(FWInfo_t) / sizeof(unsigned)));
    ERR_PROPS(fwinfo_endian_convert(fwinfo));
    return CR_OK;
}

#define SPIFLASH_SECTOR_SIZE  256
#define SPIFLASH_SECTOR_COUNT (SPIFLASH_SECTOR_SIZE / sizeof(unsigned))
#define SLOT_ADDR(i)          (0x40000 * i + 0x2000)  // 256 * i KiB + 8 Kib

CredoError_t se_fw_spiflash_display(CredoSlice_t* slice) {
    int entry;
    FWInfo_t fwinfo = {0};
    ERR_PROPS(se_fw_spiflash_read_boot_record_table(slice, &fwinfo, &entry));
    if (fwinfo.tag != CRDO_MAGIC) {
        LOGS_ERROR("Please format flash first because of incorrect tag 0x%08X", fwinfo.tag);
        return CR_FAIL;
    }
    LOGS_INFO(
        "[boot record] history %u, "
        "active_slot %d, "
        "flash_kb %d",
        fwinfo.history, fwinfo.active_slot, fwinfo.slot[0].rsvd);
    for (int i = 0; i < MAX_SLOT_CNT; i++) {
        LOGS_INFO(
            "Slot [%d] "
            "img_src  0x%08X, "
            "img_size 0x%08X",
            i, fwinfo.slot[i].img_src, fwinfo.slot[i].img_size);
    }
    return CR_OK;
}

CredoError_t se_fw_spiflash_format(CredoSlice_t* slice, unsigned flash_kb_size, int force) {
    CredoError_t ret = CR_OK;

    if (flash_kb_size < 256) {
        LOGS_ERROR("The flash is too small, %d kb", flash_kb_size);
        return CR_FAIL;
    }

    if (force != 1) {
        int entry;
        FWInfo_t fwinfo = {0};
        ERR_PROPS(se_fw_spiflash_read_boot_record_table(slice, &fwinfo, &entry));
        if (fwinfo.tag == CRDO_MAGIC) {
            LOGS_WARN("boot record table exist, format it if set force flag");
            return CR_OK;
        }
    }

    // try to erase the last page of flash, but fw erase cannot return fail if over size
    ERR_PROP_LOG(se_fw_spiflash_erase(slice, (flash_kb_size * 4096 - 1) & 0x1000),
                 LOGS_ERROR("failed to erase the last page of flash"));
    // erase first 8KB of flash
    ERR_PROPS(se_fw_spiflash_erase(slice, 0));     // 0 to 4095
    ERR_PROPS(se_fw_spiflash_erase(slice, 4096));  // 4096 to 8191

    FWInfo_t fwinfo = {0};
    ERR_PROPS(se_spiflash_build_fwinfo(&fwinfo, 0, MAX_SLOT_CNT));
    for (int slot_num = 0; slot_num < MAX_SLOT_CNT; slot_num++) {
        ERR_PROPS(se_spiflash_build_slotinfo(&fwinfo.slot[slot_num], NULL, NULL, SLOT_ADDR(slot_num), 0));
    }
    fwinfo.slot[0].rsvd = flash_kb_size;  // flash_kb_size store in slot[0].rsvd

    ERR_PROPS(se_fw_spiflash_write_boot_record_table(slice, &fwinfo, 0));
    return ret;
}

static CredoError_t se_fw_spiflash_write_firmware_inner(CredoSlice_t* slice, FILE* fp, int slot_num, int force) {
    unsigned image_start_offset;
    FirmwareHeader_t fw_header;
    unsigned shift;
    unsigned fw_length;
    CredoTime_t start_time;
    int entry;
    FWInfo_t fwinfo = {0};

    ERR_PROPS(se_fw_spiflash_read_boot_record_table(slice, &fwinfo, &entry));
    if (fwinfo.tag != CRDO_MAGIC) {
        LOGS_ERROR("Please format flash first because of incorrect tag 0x%08X", fwinfo.tag);
        return CR_FAIL;
    }

    if (force != 1) {
        uint32_t img_size = fwinfo.slot[slot_num].img_size;
        if (img_size != 0 && img_size != 0xFFFFFFFF) {
            LOGS_WARN("Slot[%d] firmware exists. Set force flag to overwrite it.", slot_num);
            return CR_FAIL;
        }
    }

    ERR_PROPS(common_fw_parse_header(slice, fp, &image_start_offset));

    if (fseek(fp, image_start_offset, SEEK_SET)) {
        LOGS_ERROR("fseek fail!");
        return CR_FAIL;
    }

    if (sizeof(fw_header) != fread(&fw_header, sizeof(char), sizeof(fw_header), fp)) {
        LOGS_ERROR("unable to read firmware header");
        return CR_FAIL;
    }

    unsigned flash_start_addr = SLOT_ADDR(slot_num);
    fw_length = common_fw_firmware_length(&fw_header);
    unsigned flash_kb_size = fwinfo.slot[0].rsvd;
    unsigned write_size = fw_length + sizeof(fw_header);

    if ((flash_start_addr + write_size) > (flash_kb_size * 1024)) {
        LOGS_ERROR("over flash_size 0x%X bytes to write firmware from address 0x%X with length 0x%X",
                   flash_kb_size * 1024, flash_start_addr, write_size);
        return CR_FAIL;
    }

    fw_header.SHA_offset = 0;
    fw_header.RSA_offset = 0;

    get_time(&start_time);
    ERR_PROPS(se_fw_spiflash_write_after_erase(slice, flash_start_addr, (unsigned*)&fw_header,
                                               sizeof(fw_header) / sizeof(unsigned)));

    LOGS_INFO("Slot[%02d] Firmware:", slot_num);
    ERR_PROPS(common_fw_header_display(slice, &fw_header));

    LOGS_INFO("Start firmware programming...");

    shift = (common_fw_firmware_type(&fw_header) < COMPRESSED2) ? 20 : 24;
    if (fseek(fp, image_start_offset + shift, SEEK_SET)) {
        LOGS_ERROR("Failed shifting to firmware data start");
        return CR_FAIL;
    }

    flash_start_addr += sizeof(fw_header);
    int fw_count = fw_length / sizeof(unsigned);

    while (fw_count > 0) {
        char buffer[SPIFLASH_SECTOR_SIZE] = {0};
        unsigned write_count = (fw_count > SPIFLASH_SECTOR_COUNT) ? SPIFLASH_SECTOR_COUNT : fw_count;

        if (fread(buffer, sizeof(char), sizeof(unsigned) * write_count, fp) != write_count * sizeof(unsigned)) {
            LOGS_ERROR("unable to read firmware file index");
            return CR_FAIL;
        }

        ERR_PROPS(se_fw_spiflash_write_after_erase(slice, flash_start_addr, (unsigned*)buffer, write_count));

        flash_start_addr += write_count * sizeof(unsigned);
        fw_count -= write_count;
    }

    LOGS_INFO("                             Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));

    ERR_PROPS(se_spiflash_build_slotinfo(&fwinfo.slot[slot_num], NULL, NULL, SLOT_ADDR(slot_num),
                                         fw_length + sizeof(fw_header)));
    if (fwinfo.history == (0xFFFFFFFF - 1)) {
        LOGS_WARN("flash update firmware over 0xFFFFFFFF times, erase boot record table");
        ERR_PROPS(se_fw_spiflash_erase(slice, 0));
        ERR_PROPS(se_fw_spiflash_erase(slice, 4096));
    }
    ERR_PROPS(se_spiflash_build_fwinfo(&fwinfo, fwinfo.history + 1, slot_num));
    get_time(&start_time);
    ERR_PROPS(se_fw_spiflash_write_boot_record_table(slice, &fwinfo, entry + 1));
    LOGS_INFO("Update SlotInfo, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));

    return CR_OK;
}

CredoError_t se_fw_spiflash_write_firmware(CredoSlice_t* slice, const char* fwname, int partition_num, int force) {
    CredoError_t ret;
    FILE* fp = NULL;

    if (fwname == NULL) {
        LOGS_ERROR("Please input fw file\n");
        return CR_FAIL;
    }

    if ((fp = fopen(fwname, "rb")) == NULL) {
        LOGS_ERROR("fw file %s open fail, %s\n", fwname, strerror(errno));
        return CR_FAIL;
    }

    ret = se_fw_spiflash_write_firmware_inner(slice, fp, partition_num, force);

    if (fp) fclose(fp);

    return ret;
}

static CredoError_t se_fw_spiflash_read_firmware_inner(CredoSlice_t* slice, FILE* fp, int slot_num) {
    int entry;
    FWInfo_t fwinfo;
    FirmwareHeader_t fw_header;
    unsigned flash_start_addr;
    unsigned write_count;
    CredoTime_t start_time;

    if ((slot_num < 0) || (slot_num >= MAX_SLOT_CNT)) {
        LOGS_ERROR("Partition number is from 0 to %d", MAX_SLOT_CNT - 1);
        return CR_FAIL;
    }

    get_time(&start_time);
    ERR_PROPS(se_fw_spiflash_read_boot_record_table(slice, &fwinfo, &entry));
    if (fwinfo.tag != CRDO_MAGIC) {
        LOGS_ERROR("Please format flash first because of incorrect tag 0x%08X", fwinfo.tag);
        return CR_FAIL;
    }

    uint32_t img_src = fwinfo.slot[slot_num].img_src;
    uint32_t img_size = fwinfo.slot[slot_num].img_size;
    if ((img_src == 0) || (img_src == 0xFFFFFFFF) || (img_size == 0) || (img_size == 0xFFFFFFFF)) {
        LOGS_ERROR("Slot[%d] image src 0x%08X and size 0x%08X incorrect", slot_num, img_src, img_size);
        return CR_FAIL;
    }

    int remain_length = img_size;
    LOGS_INFO("Start firmware reading...");
    flash_start_addr = img_src;
    ERR_PROPS(
        se_fw_spiflash_read(slice, flash_start_addr, (unsigned*)&fw_header, sizeof(fw_header) / sizeof(unsigned)));
    flash_start_addr += sizeof(fw_header);
    remain_length -= sizeof(fw_header);

    // FW_FORMAT_OLD
    unsigned buffer[4096 / sizeof(unsigned)];
    write_count = sizeof(buffer) / sizeof(unsigned);
    for (int i = 0; i < write_count; i++) buffer[i] = 0xffffffff;
    if (fwrite(buffer, sizeof(unsigned), write_count, fp) != write_count) {
        LOGS_ERROR("unable to write firmware format");
        return CR_FAIL;
    }

    write_count = ((common_fw_firmware_type(&fw_header) < COMPRESSED2) ? 20 : 24) / sizeof(unsigned);
    if (fwrite(&fw_header, sizeof(unsigned), write_count, fp) != write_count) {
        LOGS_ERROR("unable to write firmware header");
        return CR_FAIL;
    }

    while (remain_length > 0) {
        unsigned write_count = (remain_length > sizeof(buffer) ? sizeof(buffer) : remain_length) / sizeof(unsigned);
        ERR_PROPS(se_fw_spiflash_read(slice, flash_start_addr, buffer, write_count));

        if (fwrite(&buffer, sizeof(unsigned), write_count, fp) != write_count) {
            LOGS_ERROR("unable to write firmware file index");
            return CR_FAIL;
        }

        flash_start_addr += write_count * sizeof(unsigned);
        remain_length -= write_count * sizeof(unsigned);
    }

    LOGS_INFO("                         Done, using %u msecs", (unsigned)(us_passed(&start_time) / 1000));
    LOGS_INFO("Slot[%02d] Firmware:", slot_num);
    ERR_PROPS(common_fw_header_display(slice, &fw_header));

    return CR_OK;
}

CredoError_t se_fw_spiflash_read_firmware(CredoSlice_t* slice, const char* fwname, int partition_num) {
    CredoError_t ret;
    FILE* fp = NULL;

    if (fwname == NULL) {
        LOGS_ERROR("Please input fw file\n");
        return CR_FAIL;
    }

    if ((fp = fopen(fwname, "wb")) == NULL) {
        LOGS_ERROR("fw file %s open fail, %s\n", fwname, strerror(errno));
        return CR_FAIL;
    }

    ret = se_fw_spiflash_read_firmware_inner(slice, fp, partition_num);

    if (fp) fclose(fp);

    return ret;
}

static void __recover_slot_img_src(FWInfo_t* fwinfo) {
    for (int i = 0; i < MAX_SLOT_CNT; i++) {
        fwinfo->slot[i].img_src = SLOT_ADDR(i);
    }
}

static void __invalid_slot_img_src(FWInfo_t* fwinfo) {
    for (int i = 0; i < MAX_SLOT_CNT; i++) {
        fwinfo->slot[i].img_src = 0;
    }
}

CredoError_t se_fw_spiflash_set_partition(CredoSlice_t* slice, int partition_num) {
    int entry = 0;
    uint32_t checksum;
    FWInfo_t fwinfo = {0};
    ERR_PROPS(se_fw_spiflash_read_boot_record_table(slice, &fwinfo, &entry));
    checksum = fwinfo.checksum;

    if (0 <= partition_num && partition_num < MAX_SLOT_CNT) {
        fwinfo.active_slot = partition_num;
        __recover_slot_img_src(&fwinfo);
    } else if (partition_num == -1) {
        fwinfo.active_slot = 0;
        __invalid_slot_img_src(&fwinfo);
    } else {
        LOGS_WARN("Disable SPI flash load firmware if set partition_num to -1");
        return CR_OK;
    }
    ERR_PROPS(se_spiflash_update_fwinfo_checksum(&fwinfo));
    if (checksum == fwinfo.checksum) {
        LOGS_WARN("No change because of the same partition_num %d", partition_num);
    } else {
        int count = 1;
        if (partition_num == -1) {
            count = 3;
            LOGS_WARN("Disable SPI flash load firmware");
        }
        for (int i = 0; i < count; i++) {
            fwinfo.history++;
            ERR_PROPS(se_fw_spiflash_write_boot_record_table(slice, &fwinfo, entry + i + 1));
        }
    }

    return CR_OK;
}
