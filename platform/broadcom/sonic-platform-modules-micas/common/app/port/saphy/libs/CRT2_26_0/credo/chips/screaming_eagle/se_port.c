#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"
#include "se_fw_config.h"
#include "se_option.h"

#include "common/common_firmware.h"

#include "sbs.h"
#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CredoError_t se_fw_config_switchover_mode(CredoSlice_t* slice, uint32_t port_id, SePortInfo_t* port_config) {
    unsigned fw_feature = 0;
    ERR_PROP(se_fw_get_feature(slice, &fw_feature));
    bool is_fw_support_switchover_mode = (fw_feature & FW_FEATURE_SUPPORT_SWITCHOVER_RETIMER_MODE) ? true : false;
    if (is_fw_support_switchover_mode == false) {
        LOGS_ERROR("[Port config][Switchover] firmware doesn't support.");
        return CR_UNSUPPORTED;
    }

    if (port_config->setup.host_count != (port_config->setup.line_count / 2) &&
        (port_config->setup.host_count / 2) != port_config->setup.line_count) {
        LOGS_ERROR("[Port config][Switchover] Host/Line lane count error.");
        return CR_INVALID_ARGS;
    }
    bool is_a_side_bcst = (port_config->setup.host_count < port_config->setup.line_count) ? true : false;
    unsigned active_count = (is_a_side_bcst) ? port_config->setup.host_count : port_config->setup.line_count;
    unsigned multilane_code = active_count - 1;
    unsigned speed = port_config->setup.speed / active_count;
    unsigned detail1 = se_fw_translate_speed(speed);
    if (detail1 == 0xFFFF) {
        LOGS_ERROR("[Port config][Retimer] Not supported Speed: %d", speed);
        return CR_INVALID_ARGS;
    }

    CredoLaneMode_t lane_mode;
    if (speed > CONFIG_56G || (speed >= CONFIG_50G && port_config->is_50g_nrz_mode == false)) {
        lane_mode = CR_LMODE_PAM4;
    } else {
        lane_mode = CR_LMODE_NRZ;
    }
    // force host and line main lane to 0th index
    port_config->host_main_lane = port_config->setup.host_lanes[0];
    port_config->line_main_lane = port_config->setup.line_lanes[0];

    if (port_config->is_50g_nrz_mode == true) {
        detail1 |= FW_OPTION_OPT_MODE;
    }
    detail1 |= (multilane_code << 8);

    unsigned detail2 = 0;
    ERR_PROPS(se_fw_translate_flags(port_config, &detail2));
    detail2 |= ((port_id + 1) << 12);
    if (is_a_side_bcst) {
        detail2 |= (FW_OPTION_SYS_ARBITRARY_MAIN | ((port_config->host_main_lane % 8) << FW_SYS_ARBITRARY_MAIN_OFFSET));
    } else {
        detail2 |=
            (FW_OPTION_LINE_ARBITRARY_MAIN | ((port_config->line_main_lane % 8) << FW_LINE_ARBITRARY_MAIN_OFFSET));
    }

    unsigned tx_map_a = 0, tx_map_b = 0;
    unsigned active_lane, standby_lane, bcst_lane, dummy;
    for (unsigned i = 0; i < active_count; i++) {
        if (is_a_side_bcst) {
            bcst_lane = port_config->setup.host_lanes[i];
            active_lane = port_config->setup.line_lanes[i * 2];
            dummy = (port_config->setup.host_lanes[i] % 2) ? port_config->setup.host_lanes[i] - 1
                                                           : port_config->setup.host_lanes[i] + 1;
            standby_lane = port_config->setup.line_lanes[i * 2 + 1];

            tx_map_a |= ((0x8 | (active_lane & 0x7)) << (4 * (bcst_lane % 8)));
            tx_map_a |= ((0x8 | (standby_lane & 0x7)) << (4 * (dummy % 8)));

            tx_map_b |= ((0x8 | (bcst_lane & 0x7)) << (4 * (active_lane % 8)));
            tx_map_b |= ((0x8 | (bcst_lane & 0x7)) << (4 * (standby_lane % 8)));
        } else {
            bcst_lane = port_config->setup.line_lanes[i];
            active_lane = port_config->setup.host_lanes[i * 2];
            dummy = (port_config->setup.line_lanes[i] % 2) ? port_config->setup.line_lanes[i] - 1
                                                           : port_config->setup.line_lanes[i] + 1;
            standby_lane = port_config->setup.host_lanes[i * 2 + 1];

            tx_map_a |= ((0x8 | (bcst_lane & 0x7)) << (4 * (active_lane % 8)));
            tx_map_a |= ((0x8 | (bcst_lane & 0x7)) << (4 * (standby_lane % 8)));

            tx_map_b |= ((0x8 | (active_lane & 0x7)) << (4 * (bcst_lane % 8)));
            tx_map_b |= ((0x8 | (standby_lane & 0x7)) << (4 * (dummy % 8)));
        }
    }

    unsigned active_lane_map = 0, bcst_lane_map = 0;
    for (unsigned i = 0; i < active_count; i++) {
        if (is_a_side_bcst) {
            active_lane_map |= (1 << port_config->setup.line_lanes[i * 2]);
            bcst_lane_map |= (1 << port_config->setup.host_lanes[i]);
        } else {
            active_lane_map |= (1 << port_config->setup.host_lanes[i * 2]);
            bcst_lane_map |= (1 << port_config->setup.line_lanes[i]);
        }
    }

    if (port_config->flexspeed_kbps != 0) {
        unsigned fw_feature = 0;
        ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
        if ((fw_feature & FW_FEATURE_SUPPORT_FLEXSPEED) == 0) {
            LOGS_ERROR("Flexspeed unsupported in this firmware");
            return CR_UNSUPPORTED;
        }
        double port_flexspeed = ((double)port_config->flexspeed_kbps) * 1000;
        double lane_speed = port_flexspeed / port_config->setup.host_count;
        flexspeed_config_t flex_config = se_flexspeed_compute_config(lane_speed, lane_mode);
        LOGS_DEBUG(
            "Flexspeed settings:\ndatarate=%.6fGb/s\nbaudrate=%.6fG\nvcorate=%.6fG\npll_n=%u, "
            "pll_n_frac=%u\ntx_target_count=%u, "
            "rx_target_count=%u",
            lane_speed / 1e9, flex_config.baudrate / 1e9, flex_config.vcorate / 1e9, flex_config.pll_n,
            flex_config.pll_n_frac, flex_config.tx_target_count, flex_config.rx_target_count);
        ERR_PROPS(se_flexspeed_set_registers(slice, CR_SIDE_HOST, &flex_config, true, true));
        ERR_PROPS(se_flexspeed_set_registers(slice, CR_SIDE_LINE, &flex_config, true, true));
    }

    ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
    ERR_PROPS(writeTop(slice, REG_DATA + 8, active_lane_map));
    ERR_PROPS(writeTop(slice, REG_DATA + 9, bcst_lane_map));

    unsigned cmd = FW_CMD_CONFIG_MODE | (FW_MODE_SWITCHOVER_RETIMER << 4);
    unsigned response = 0;
    LOGS_DEBUG(
        "[Port config][%u] Cmd 0x%04X, detail1 0x%04X, detail2 0x%04X, map_a 0x%08X, map_b 0x%08X, active_map 0x%04X, "
        "bcst_map 0x%04X",
        port_id, cmd, detail1, detail2, tx_map_a, tx_map_b, active_lane_map, bcst_lane_map);
    CredoError_t err = common_fw_cmd_ex(slice, cmd, detail1, detail2, &response, NULL, NULL);
    if (err != CR_OK) {
        if ((response & 0xFF) != ERROR_LANE_USED) {
            LOGS_INFO("[Port Config][%u] Cleaning up invalid port setup", port_id);
            ERR_PROPS(se_fw_deconfig_cmd(slice, cmd, detail1));
        }
        return err;
    }

    port_config->active_lane_map = active_lane_map;
    port_config->switchover_select = 0;
    for (unsigned i = 0; i < port_config->setup.host_count; i++) {
        se_set_lane_mode(slice, port_config->setup.host_lanes[i], lane_mode);
    }
    for (unsigned i = 0; i < port_config->setup.line_count; i++) {
        se_set_lane_mode(slice, port_config->setup.line_lanes[i], lane_mode);
    }

    return CR_OK;
}

static CredoError_t se_fw_config_retimer(CredoSlice_t* slice, uint32_t port_id, const SePortInfo_t* port_config) {
    if (port_config->setup.host_count != port_config->setup.line_count) {
        LOGS_ERROR("[Port config][Retimer] Host/Line lane number mismatch.");
        return CR_INVALID_ARGS;
    }

    CredoPortSetup_t port_setup = port_config->setup;
    unsigned host_count = port_setup.host_count;
    unsigned speed, detail1, response;
    CredoLaneMode_t A_mode, B_mode;

    speed = port_config->setup.speed / host_count;
    detail1 = se_fw_translate_speed(speed);
    if (detail1 == 0xFFFF) {
        LOGS_ERROR("[Port config][Retimer] Not supported Speed: %d", speed);
        return CR_INVALID_ARGS;
    }

    if (speed > CONFIG_56G || (speed >= CONFIG_50G && port_config->is_50g_nrz_mode == false)) {
        A_mode = B_mode = CR_LMODE_PAM4;
    } else {
        A_mode = B_mode = CR_LMODE_NRZ;
    }

    if (port_config->is_50g_nrz_mode == true) {
        detail1 |= FW_OPTION_OPT_MODE;
    }
    if (port_config->an_override) {
        detail1 |= FW_OPTION_AN_OVERRIDE;
    }

    /* translate multilane */
    unsigned multilane_code = port_config->setup.line_count;
    ERR_PROPS(se_fw_translate_multilane(multilane_code, &multilane_code));
    detail1 |= multilane_code << 8;

    unsigned detail2 = 0;
    ERR_PROPS(se_fw_translate_flags(port_config, &detail2));
    detail2 |= (FW_OPTION_LINE_ARBITRARY_MAIN | ((port_config->line_main_lane % 8) << FW_LINE_ARBITRARY_MAIN_OFFSET));
    detail2 |= (FW_OPTION_SYS_ARBITRARY_MAIN | ((port_config->host_main_lane % 8) << FW_SYS_ARBITRARY_MAIN_OFFSET));
    detail2 |= ((port_id + 1) << 12);

    unsigned tx_map_a = 0, tx_map_b = 0;
    for (unsigned i = 0; i < host_count; i++) {
        unsigned host_lane = port_config->setup.host_lanes[i];
        unsigned line_lane = port_config->setup.line_lanes[i];
        tx_map_a |= ((0x8 | (line_lane & 0x7)) << (4 * host_lane));
        tx_map_b |= ((0x8 | host_lane) << (4 * (line_lane % 8)));
    }

    unsigned cmd = FW_CMD_CONFIG_MODE;
    if (port_config->isc.enable) {
        unsigned detail3 = 0;
        int isc_slice_id;
        ERR_PROPS(se_option_get_isc_slice_id(slice, NULL, &isc_slice_id));
        if (isc_slice_id == 0xFFFF) {
            LOGS_ERROR("Please set isc_slice_id");
            return CR_FAIL;
        }
        cmd |= ((FW_MODE_RETIMER << 4) | (isc_slice_id & 0xF));
        detail1 |= FW_OPTION_ISC_ENABLE;
        detail1 |= (port_config->isc.an_agent) ? FW_OPTION_ISC_AN_AGENT : 0;
        detail2 &= (port_config->isc.an_agent) ? ~FW_OPTION_LINE_SIDE_AN : ~0;
        ERR_PROPS(se_fw_set_tx_map_isc(slice, port_config->isc.lane_map_a, port_config->isc.lane_map_b));
        detail3 = ((port_config->isc.host_main_lane & 0x7) << FW_OPTION_ISC_SYS_MAIN_OFFSET) |
                  ((port_config->isc.line_main_lane & 0x7) << FW_OPTION_ISC_LINE_MAIN_OFFSET);
        ERR_PROPS(writeReg(slice, REG_CMD_DETAIL3, detail3));
        LOGS_DEBUG("[Port config] ISC, detail3 0x%04X, map_a 0x%08X, map_b 0x%08X", detail3,
                   port_config->isc.lane_map_a, port_config->isc.lane_map_b);
    } else {
        unsigned rt_mode =
            (port_config->direction != CR_PORT_DIR_BIDIRECTIONAL) ? FW_MODE_UNI_RETIMER : FW_MODE_RETIMER;
        cmd |= (rt_mode << 4);
        if (port_config->direction == CR_PORT_DIR_HOST_TO_LINE) tx_map_b = 0;
        if (port_config->direction == CR_PORT_DIR_LINE_TO_HOST) tx_map_a = 0;
        ERR_PROPS(se_fw_set_tx_map_isc(slice, 0, 0));
        ERR_PROPS(writeReg(slice, REG_CMD_DETAIL3, 0));
    }

    if (port_config->flexspeed_kbps != 0) {
        unsigned fw_feature = 0;
        ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
        if ((fw_feature & FW_FEATURE_SUPPORT_FLEXSPEED) == 0) {
            LOGS_ERROR("Flexspeed unsupported in this firmware");
            return CR_UNSUPPORTED;
        }
        double port_flexspeed = ((double)port_config->flexspeed_kbps) * 1000;
        double lane_speed = port_flexspeed / host_count;
        flexspeed_config_t flex_config = se_flexspeed_compute_config(lane_speed, A_mode);
        LOGS_DEBUG(
            "Flexspeed settings:\ndatarate=%.6fGb/s\nbaudrate=%.6fG\nvcorate=%.6fG\npll_n=%u, "
            "pll_n_frac=%u\ntx_target_count=%u, "
            "rx_target_count=%u",
            lane_speed / 1e9, flex_config.baudrate / 1e9, flex_config.vcorate / 1e9, flex_config.pll_n,
            flex_config.pll_n_frac, flex_config.tx_target_count, flex_config.rx_target_count);
        ERR_PROPS(se_flexspeed_set_registers(
            slice, CR_SIDE_HOST, &flex_config,
            port_config->direction == CR_PORT_DIR_BIDIRECTIONAL || port_config->direction == CR_PORT_DIR_LINE_TO_HOST,
            port_config->direction == CR_PORT_DIR_BIDIRECTIONAL || port_config->direction == CR_PORT_DIR_HOST_TO_LINE));
        ERR_PROPS(se_flexspeed_set_registers(
            slice, CR_SIDE_LINE, &flex_config,
            port_config->direction == CR_PORT_DIR_BIDIRECTIONAL || port_config->direction == CR_PORT_DIR_HOST_TO_LINE,
            port_config->direction == CR_PORT_DIR_BIDIRECTIONAL || port_config->direction == CR_PORT_DIR_LINE_TO_HOST));
    }

    ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
    LOGS_DEBUG("[Port config][%u] Cmd 0x%04X, detail1 0x%04X, detail2 0x%04X, map_a 0x%08X, map_b 0x%08X", port_id, cmd,
               detail1, detail2, tx_map_a, tx_map_b);
    CredoError_t err = common_fw_cmd_ex(slice, cmd, detail1, detail2, &response, NULL, NULL);

    if (err != CR_OK) {
        if ((response & 0xFF) != ERROR_LANE_USED) {
            LOGS_INFO("[Retimer Config][%d] Cleaning up invalid port setup", port_id);
            ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
            ERR_PROPS(se_fw_deconfig_cmd(slice, cmd, detail1));
        }
        return err;
    }

    for (int i = 0; i < host_count; i++) {
        unsigned host_lane = port_config->setup.host_lanes[i];
        unsigned line_lane = port_config->setup.line_lanes[i];
        if (port_config->direction == CR_PORT_DIR_LINE_TO_HOST) {
            se_set_lane_mode_uni_retimer(slice, line_lane, host_lane, B_mode);
        } else if (port_config->direction == CR_PORT_DIR_HOST_TO_LINE) {
            se_set_lane_mode_uni_retimer(slice, host_lane, line_lane, A_mode);
        } else {
            se_set_lane_mode(slice, host_lane, A_mode);
            se_set_lane_mode(slice, line_lane, B_mode);
        }
    }

    return CR_OK;
}

static CredoError_t se_fw_config_bitmux(CredoSlice_t* slice, uint32_t port_id, const SePortInfo_t* port_config) {
    if (((port_config->setup.host_count << 1) != port_config->setup.line_count) &&
        (port_config->setup.host_count != (port_config->setup.line_count << 1))) {
        LOGS_ERROR("[Port config][Bitmux] Host/Line lane number mismatch.");
        return CR_INVALID_ARGS;
    }

    CredoPortSetup_t port_setup = port_config->setup;

    unsigned host_count = port_setup.host_count;
    unsigned line_count = port_setup.line_count;
    unsigned fw_mode, multilane = 1, multilane_code = 0;
    unsigned detail1, response;
    CredoLaneMode_t A_mode, B_mode;

    A_mode = B_mode = CR_LMODE_PAM4;
    unsigned speed = port_config->setup.speed;
    switch (speed) {
        case (CONFIG_50G):
        case (CONFIG_53G):
            if (!((host_count == 1 && line_count == 2) || (host_count == 2 && line_count == 1))) {
                LOGS_ERROR("[Port config][Bitmux] Speed/Lane mismatch.");
                return CR_INVALID_ARGS;
            }
            fw_mode = (host_count == 1) ? FW_MODE_BITMUX_A1B2 : FW_MODE_BITMUX_A2B1;
            detail1 = (host_count == 1) ? (DEFSPEED(53G, 26G)) : (DEFSPEED(26G, 53G));
            A_mode = (host_count == 1) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
            B_mode = (host_count == 1) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
            multilane = 1;
            break;
        case (CONFIG_53G * 2):
        case (CONFIG_100G):
            if ((host_count == 1 && line_count == 2) || (host_count == 2 && line_count == 1)) {
                fw_mode = (host_count == 1) ? FW_MODE_BITMUX_A1B2 : FW_MODE_BITMUX_A2B1;
                detail1 = (host_count == 1) ? (DEFSPEED(106G, 53G)) : (DEFSPEED(53G, 106G));
                multilane = 1;
            } else if ((host_count == 2 && line_count == 4) || (host_count == 4 && line_count == 2)) {
                fw_mode = (host_count == 2) ? FW_MODE_BITMUX_A1B2 : FW_MODE_BITMUX_A2B1;
                detail1 = (host_count == 2) ? (DEFSPEED(53G, 26G)) : (DEFSPEED(26G, 53G));
                A_mode = (host_count == 2) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
                B_mode = (host_count == 2) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
                multilane = 2;
            } else {
                LOGS_ERROR("[Port config][Bitmux] Speed/Lane mismatch.");
                return CR_INVALID_ARGS;
            }
            break;
        case (CONFIG_53G * 4):
        case (CONFIG_200G):
            if ((host_count == 2 && line_count == 4) || (host_count == 4 && line_count == 2)) {
                fw_mode = (host_count == 2) ? FW_MODE_BITMUX_A1B2 : FW_MODE_BITMUX_A2B1;
                detail1 = (host_count == 2) ? (DEFSPEED(106G, 53G)) : (DEFSPEED(53G, 106G));
                multilane = 2;
            } else if ((host_count == 4 && line_count == 8) || (host_count == 8 && line_count == 4)) {
                fw_mode = (host_count == 4) ? FW_MODE_BITMUX_A1B2 : FW_MODE_BITMUX_A2B1;
                detail1 = (host_count == 4) ? (DEFSPEED(53G, 26G)) : (DEFSPEED(26G, 53G));
                A_mode = (host_count == 4) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
                B_mode = (host_count == 4) ? CR_LMODE_NRZ : CR_LMODE_PAM4;
                multilane = 4;
            } else {
                LOGS_ERROR("[Port config][Bitmux] Speed/Lane mismatch.");
                return CR_INVALID_ARGS;
            }
            break;
        case (CONFIG_53G * 8):
        case (CONFIG_400G):
            if (!((host_count == 4 && line_count == 8) || (host_count == 8 && line_count == 4))) {
                LOGS_ERROR("[Port config][Bitmux] Speed/Lane mismatch.");
                return CR_INVALID_ARGS;
            }
            fw_mode = (host_count == 4) ? FW_MODE_BITMUX_A1B2 : FW_MODE_BITMUX_A2B1;
            detail1 = (host_count == 4) ? (DEFSPEED(106G, 53G)) : (DEFSPEED(53G, 106G));
            multilane = 4;
            break;
        default:
            LOGS_ERROR("[Port config][Bitmux] Not supported Speed: %d", speed);
            return CR_UNSUPPORTED;
    }

    if (port_config->is_50g_nrz_mode == true) {
        detail1 |= FW_OPTION_OPT_MODE;
        if (fw_mode == FW_MODE_BITMUX_A1B2) B_mode = CR_LMODE_NRZ;
        if (fw_mode == FW_MODE_BITMUX_A2B1) A_mode = CR_LMODE_NRZ;

        if (speed == CONFIG_50G || speed == CONFIG_53G) A_mode = B_mode = CR_LMODE_NRZ;
    }
    if (port_config->an_override) {
        detail1 |= FW_OPTION_AN_OVERRIDE;
    }

    /* translate multilane */
    ERR_PROPS(se_fw_translate_multilane(multilane, &multilane_code));
    detail1 |= multilane_code << 8;

    unsigned detail2 = 0;
    ERR_PROPS(se_fw_translate_flags(port_config, &detail2));
    detail2 |= (FW_OPTION_LINE_ARBITRARY_MAIN | ((port_config->line_main_lane % 8) << FW_LINE_ARBITRARY_MAIN_OFFSET));
    detail2 |= (FW_OPTION_SYS_ARBITRARY_MAIN | ((port_config->host_main_lane % 8) << FW_SYS_ARBITRARY_MAIN_OFFSET));
    detail2 |= ((port_id + 1) << 12);

    unsigned tx_map_a = 0, tx_map_b = 0;
    if (fw_mode == FW_MODE_BITMUX_A1B2) {
        for (int i = 0; i < multilane; i++) {
            unsigned host_lane = port_setup.host_lanes[i] & 0x7;
            unsigned line_lane0 = port_setup.line_lanes[(i * 2)] % 0x8;
            unsigned line_lane1 = port_setup.line_lanes[(i * 2) + 1] % 0x8;
            tx_map_a |= ((0x8 | line_lane0) << (4 * host_lane));
            tx_map_b |= ((0x8 | host_lane) << (4 * line_lane0));
            tx_map_b |= ((0x8 | host_lane) << (4 * line_lane1));
        }
    } else {
        for (int i = 0; i < multilane; i++) {
            unsigned host_lane0 = port_setup.host_lanes[(i * 2)] & 0x7;
            unsigned host_lane1 = port_setup.host_lanes[(i * 2) + 1] & 0x7;
            unsigned line_lane = port_setup.line_lanes[i] % 0x8;
            tx_map_a |= ((0x8 | line_lane) << (4 * host_lane0));
            tx_map_a |= ((0x8 | line_lane) << (4 * host_lane1));
            tx_map_b |= ((0x8 | host_lane0) << (4 * line_lane));
        }
    }
    // tx map maybe modified after fw deconfig...set again
    ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));

    unsigned cmd = FW_CMD_CONFIG_MODE | ((int)(fw_mode) << 4);
    if (port_config->isc.enable) {
        unsigned detail3 = 0;
        int isc_slice_id;
        ERR_PROPS(se_option_get_isc_slice_id(slice, NULL, &isc_slice_id));
        if (isc_slice_id == 0xFFFF) {
            LOGS_ERROR("Please set isc_slice_id");
            return CR_FAIL;
        }
        cmd |= isc_slice_id & 0xF;  // remove once firmware no longer uses this isc_slice_id
        detail1 |= FW_OPTION_ISC_ENABLE;
        detail1 |= (port_config->isc.an_agent) ? FW_OPTION_ISC_AN_AGENT : 0;
        detail2 &= (port_config->isc.an_agent) ? ~FW_OPTION_LINE_SIDE_AN : ~0;
        ERR_PROPS(se_fw_set_tx_map_isc(slice, port_config->isc.lane_map_a, port_config->isc.lane_map_b));
        detail3 = ((port_config->isc.host_main_lane & 0x7) << FW_OPTION_ISC_SYS_MAIN_OFFSET) |
                  ((port_config->isc.line_main_lane & 0x7) << FW_OPTION_ISC_LINE_MAIN_OFFSET);
        ERR_PROPS(writeReg(slice, REG_CMD_DETAIL3, detail3));
        LOGS_DEBUG("[Port config] ISC, detail3 0x%04X, map_a 0x%08X, map_b 0x%08X", detail3,
                   port_config->isc.lane_map_a, port_config->isc.lane_map_b);
    } else {
        ERR_PROPS(se_fw_set_tx_map_isc(slice, 0, 0));
        ERR_PROPS(writeReg(slice, REG_CMD_DETAIL3, 0));
    }

    if (port_config->flexspeed_kbps != 0) {
        unsigned fw_feature = 0;
        ERR_PROPS(se_fw_get_feature(slice, &fw_feature));
        if ((fw_feature & FW_FEATURE_SUPPORT_FLEXSPEED) == 0) {
            LOGS_ERROR("Flexspeed unsupported in this firmware");
            return CR_UNSUPPORTED;
        }
        double port_flexspeed = ((double)port_config->flexspeed_kbps) * 1000;
        double lane_speed_host = port_flexspeed / host_count;
        double lane_speed_line = port_flexspeed / line_count;
        flexspeed_config_t flex_config_host = se_flexspeed_compute_config(lane_speed_host, A_mode);
        flexspeed_config_t flex_config_line = se_flexspeed_compute_config(lane_speed_line, B_mode);
        LOGS_DEBUG(
            "Flexspeed Host settings:\ndatarate=%.6fGb/s\nbaudrate=%.6fG\nvcorate=%.6fG\npll_n=%u, "
            "pll_n_frac=%u\ntx_target_count=%u, "
            "rx_target_count=%u",
            lane_speed_host / 1e9, flex_config_host.baudrate / 1e9, flex_config_host.vcorate / 1e9,
            flex_config_host.pll_n, flex_config_host.pll_n_frac, flex_config_host.tx_target_count,
            flex_config_host.rx_target_count);
        LOGS_DEBUG(
            "Flexspeed Line settings:\ndatarate=%.6fGb/s\nbaudrate=%.6fG\nvcorate=%.6fG\npll_n=%u, "
            "pll_n_frac=%u\ntx_target_count=%u, "
            "rx_target_count=%u",
            lane_speed_line / 1e9, flex_config_line.baudrate / 1e9, flex_config_line.vcorate / 1e9,
            flex_config_line.pll_n, flex_config_line.pll_n_frac, flex_config_line.tx_target_count,
            flex_config_line.rx_target_count);

        // may need to change if supporting unidirectional bitmux
        ERR_PROPS(se_flexspeed_set_registers(slice, CR_SIDE_HOST, &flex_config_host, true, true));
        ERR_PROPS(se_flexspeed_set_registers(slice, CR_SIDE_LINE, &flex_config_line, true, true));
    }

    LOGS_DEBUG("[Port config][%u] Cmd 0x%04X, detail1 0x%04X, detail2 0x%04X, map_a 0x%08X, map_b 0x%08X", port_id, cmd,
               detail1, detail2, tx_map_a, tx_map_b);

    CredoError_t err = common_fw_cmd_ex(slice, cmd, detail1, detail2, &response, NULL, NULL);

    // we need to clean up the lanes because firmware seems to leave them
    if (err != CR_OK) {
        if ((response & 0xFF) != ERROR_LANE_USED) {
            LOGS_INFO("[Bitmux Config][%u] Cleaning up invalid port setup", port_id);
            ERR_PROPS(se_fw_set_tx_map(slice, tx_map_a, tx_map_b));
            ERR_PROPS(se_fw_deconfig_cmd(slice, cmd, detail1));
        }
        return err;
    }

    for (unsigned i = 0; i < host_count; i++) {
        se_set_lane_mode(slice, port_setup.host_lanes[i], A_mode);
    }
    for (unsigned i = 0; i < line_count; i++) {
        se_set_lane_mode(slice, port_setup.line_lanes[i], B_mode);
    }

    return CR_OK;
}

CredoError_t se_fw_teardown_port(CredoSlice_t* slice, unsigned port_id) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    if (se_slice->port_info[port_id].started == false) return CR_OK;

    int main_lane = 0;
    if (se_slice->port_info[port_id].direction == CR_PORT_DIR_LINE_TO_HOST) {
        main_lane = se_slice->port_info[port_id].setup.line_lanes[0];
    } else {
        main_lane = se_slice->port_info[port_id].setup.host_lanes[0];
    }
    ERR_PROPS(se_fw_deconfig_lane(slice, main_lane));

    se_slice->port_info[port_id].started = false;
    se_slice->port_info[port_id].built = false;
    return CR_OK;
}

CredoError_t se_fw_teardown_port_all(CredoSlice_t* slice) {
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        ERR_PROPS(se_fw_teardown_port(slice, port_id));
    }
    return CR_OK;
}

static int cmpfunc(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

/*
 * Query port
 * Input:
 *  cmd[3:0]  port_id
 * Returns:
 *  cmd[15:8]   response
 *  cmd[7:4]    mode
 *  cmd[3:2]    A side FEC
 *  cmd[1:0]    B side FEC
 *
 *  detail1[11:8]   port speed or multilane code
 *  detail1[7:4]    A side speed code
 *  detail1[3:0]    B side speed code
 *
 *  detail2[15:0]   reserved
 */
CredoError_t se_port_capture_info(CredoSlice_t* slice, unsigned port_id, SePortInfo_t* port_info) {
    CredoPortSetup_t* port_config = &(port_info->setup);
    unsigned detail1 = 0, detail2 = 0, detail3 = 0, response = 0;
    unsigned tx_map_a = 0, tx_map_b = 0, isc_tx_map_a = 0, isc_tx_map_b = 0;
    ERR_PROPS(common_fw_cmd_ex(slice, FW_CMD_PORT_QUERY + port_id, 0, 0, &response, &detail1, &detail2));
    ERR_PROPS(readReg(slice, REG_CMD_DETAIL3, &detail3));
    ERR_PROPS(se_fw_get_tx_map(slice, &tx_map_a, &tx_map_b));
    ERR_PROPS(se_fw_get_tx_map_isc(slice, &isc_tx_map_a, &isc_tx_map_b));
    LOGS_DEBUG(
        "[Port query] Response 0x%04X, detail1 0x%04X, detail2 0x%04X, detail3 0x%04X, map_a 0x%08X, map_b 0x%08X, "
        "isc_map_a 0x%08X isc_map_b 0x%08X",
        response, detail1, detail2, detail3, tx_map_a, tx_map_b, isc_tx_map_a, isc_tx_map_b);

    /* Extract the fields of port query */
    unsigned fw_mode = (response >> 4) & 0xF;
    // unsigned char fec = (response >> 0) & 0xF;
    unsigned multilane = (detail1 >> 8) & 0xF;
    unsigned char host_fw_speed = (detail1 >> 4) & 0xF;
    unsigned char line_fw_speed = (detail1 >> 0) & 0xF;
    unsigned host_speed = 0, line_speed = 0;

    bool host_lt = detail2 & FW_OPTION_SYS_SIDE_LT;
    bool line_lt = detail2 & FW_OPTION_LINE_SIDE_LT;
    bool line_an = detail2 & FW_OPTION_LINE_SIDE_AN;
    int host_main_lane = (int)((detail2 >> FW_SYS_ARBITRARY_MAIN_OFFSET) & 0x7);
    int line_main_lane = (int)((detail2 >> FW_LINE_ARBITRARY_MAIN_OFFSET) & 0x7) + HOST_LANES;

    // isc info
    int isc_host_main_lane = 0, isc_line_main_lane = 0, isc_slice_id = 0xFFFF;
    bool isc_an_agent = false;
    bool isc_enable = detail1 & FW_OPTION_ISC_ENABLE;
    if (isc_enable) {
        isc_host_main_lane = (int)((detail3 >> FW_OPTION_ISC_SYS_MAIN_OFFSET) & 0x7);
        isc_line_main_lane = (int)((detail3 >> FW_OPTION_ISC_LINE_MAIN_OFFSET) & 0x7) + HOST_LANES;
        isc_an_agent = detail1 & FW_OPTION_ISC_AN_AGENT;
    }

    /* unconfigured */
    if (multilane == 0xF) {
        port_info->started = false;
        return CR_OK;
    }
    port_info->started = true;
    port_info->built = true;

    ERR_PROP_LOG(se_fw_translate_config_mode_reverse(fw_mode, &port_config->mode),
                 LOGS_ERROR("[Port query] Unknown fw mode %d", fw_mode));
    ERR_PROP_LOG(se_fw_translate_lane_speed_reverse((FirmwareSpeed_t)host_fw_speed, &host_speed),
                 LOGS_ERROR("[Port query] Unknown host side fw lane speed %d", host_fw_speed));
    ERR_PROP_LOG(se_fw_translate_lane_speed_reverse((FirmwareSpeed_t)line_fw_speed, &line_speed),
                 LOGS_ERROR("[Port query] Unknown line side fw lane speed %d", line_fw_speed));
    ERR_PROP_LOG(se_fw_translate_multilane_reverse(multilane, &multilane),
                 LOGS_ERROR("[Port query] Unknown fw multilane code %d", multilane));

    port_info->direction = (tx_map_a != 0 && tx_map_b != 0) ? CR_PORT_DIR_BIDIRECTIONAL
                           : (tx_map_b == 0)                ? CR_PORT_DIR_HOST_TO_LINE
                                                            : CR_PORT_DIR_LINE_TO_HOST;
    port_info->host_lt = host_lt;
    port_info->line_lt = line_lt;
    port_info->line_an = line_an;
    port_info->host_main_lane = host_main_lane;
    port_info->line_main_lane = line_main_lane;
    port_config->host_count = 0;
    port_config->line_count = 0;

    port_info->isc.enable = isc_enable;
    port_info->isc.an_agent = isc_an_agent;
    port_info->isc.lane_map_a = isc_tx_map_a;
    port_info->isc.lane_map_b = isc_tx_map_b;
    port_info->isc.host_main_lane = isc_host_main_lane;
    port_info->isc.line_main_lane = isc_line_main_lane;

    // isc is enabled, we can cache the isc_slice_id for the user
    if (isc_enable) {
        ERR_PROPS(se_option_get_isc_slice_id(slice, NULL, &isc_slice_id));
    }

    int map_a_host_lanes[32] = {0};
    unsigned map_a_host_count = 0;
    int map_a_line_lanes[32] = {0};
    unsigned map_a_line_count = 0;

    int map_b_host_lanes[32] = {0};
    unsigned map_b_host_count = 0;
    int map_b_line_lanes[32] = {0};
    unsigned map_b_line_count = 0;

    for (unsigned i = 0; i < HOST_LANES; i++) {
        if (((tx_map_a >> (4 * i)) & 0x8) == 0x8) {
            map_a_host_lanes[map_a_host_count] = i;
            map_a_host_count += 1;

            bool duplicate = false;
            int line_lane = ((tx_map_a >> (4 * i)) & 0x7) + HOST_LANES;
            // if duplicate of existing lane, dont add. Will happen with bitmux
            for (unsigned j = 0; j < map_a_line_count; j++) {
                if (map_a_line_lanes[j] == line_lane) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                map_a_line_lanes[map_a_line_count] = line_lane;
                map_a_line_count += 1;
            }
        }

        if (((tx_map_b >> (4 * i)) & 0x8) == 0x8) {
            map_b_line_lanes[map_b_line_count] = i + HOST_LANES;
            map_b_line_count += 1;

            bool duplicate = false;
            int host_lane = (tx_map_b >> (4 * i)) & 0x7;
            // if duplicate of existing lane, dont add. Will happen with bitmux
            for (unsigned j = 0; j < map_b_host_count; j++) {
                if (map_b_host_lanes[j] == host_lane) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                map_b_host_lanes[map_b_host_count] = host_lane;
                map_b_host_count += 1;
            }
        }
    }
    // the mapping that has more lanes in it is the one that is complete
    // this comes into play with bitmux, in retimer they are the same
    if (map_a_host_count + map_a_line_count >= map_b_host_count + map_b_line_count) {
        // a2b1 bitmux or retimer
        memcpy(port_config->host_lanes, map_a_host_lanes, sizeof(int) * map_a_host_count);
        port_config->host_count = map_a_host_count;
        memcpy(port_config->line_lanes, map_a_line_lanes, sizeof(int) * map_a_line_count);
        port_config->line_count = map_a_line_count;
    } else {
        // a1b2 bitmux
        memcpy(port_config->host_lanes, map_b_host_lanes, sizeof(int) * map_b_host_count);
        port_config->host_count = map_b_host_count;
        memcpy(port_config->line_lanes, map_b_line_lanes, sizeof(int) * map_b_line_count);
        port_config->line_count = map_b_line_count;
    }

    port_config->speed = host_speed * port_config->host_count;

    if (port_config->mode == CR_PORT_SWITCHOVER_RETIMER) {
        unsigned full_active_lane_map = 0;
        CredoError_t err = hal_fw_debug_cmd(slice, 0, SE_INFO, SE_INFO_TOP_AP_ACTIVE_LANES, &full_active_lane_map);
        if (err != CR_OK) {
            full_active_lane_map = 0;
        }
        port_info->active_lane_map = 0;
        bool is_aside_bcst = map_a_host_count + map_a_line_count >= map_b_host_count + map_b_line_count;
        if (is_aside_bcst) {
            // determine broadcast lanes, map_b_host_lanes has the broadcast lanes, but not in order
            memcpy(port_config->host_lanes, map_b_host_lanes, sizeof(int) * map_b_host_count);
            port_config->host_count = map_b_host_count;
            qsort(port_config->host_lanes, port_config->host_count, sizeof(int), cmpfunc);
            // all line lanes are captured, but dummy lane and active lane may be switched
            for (size_t i = 0; i < port_config->host_count; i++) {
                if (port_config->host_lanes[i] % 2 == 0) continue;
                int tmp_lane = port_config->line_lanes[i * 2];
                port_config->line_lanes[i * 2] = port_config->line_lanes[(i * 2) + 1];
                port_config->line_lanes[(i * 2) + 1] = tmp_lane;
            }
            // line side is active-standby, determine active map
            for (size_t i = 0; i < port_config->line_count; i++) {
                port_info->active_lane_map |= (1 << port_config->line_lanes[i]) & full_active_lane_map;
            }
            // check if mux group 0 is active
            port_info->switchover_select = (port_info->active_lane_map & (1 << port_config->line_lanes[0])) == 0;
            // need to properly select line side main lane, based off which switchover group is active
            port_info->line_main_lane = port_config->line_lanes[(port_info->switchover_select == 0) ? 0 : 1];
        } else {
            // determine broadcast lanes, map_a_line_lanes has the broadcast lanes, but not in order
            memcpy(port_config->line_lanes, map_a_line_lanes, sizeof(int) * map_a_line_count);
            port_config->line_count = map_a_line_count;
            qsort(port_config->line_lanes, port_config->line_count, sizeof(int), cmpfunc);
            // all host lanes are captured, but dummy lane and active lane may be switched
            for (size_t i = 0; i < port_config->line_count; i++) {
                if (port_config->line_lanes[i] % 2 == 0) continue;
                int tmp_lane = port_config->host_lanes[i * 2];
                port_config->host_lanes[i * 2] = port_config->host_lanes[(i * 2) + 1];
                port_config->host_lanes[(i * 2) + 1] = tmp_lane;
            }
            // host side is active-standby, determine active map
            for (size_t i = 0; i < port_config->host_count; i++) {
                port_info->active_lane_map |= (1 << port_config->host_lanes[i]) & full_active_lane_map;
            }
            // check if mux group 0 is active
            port_info->switchover_select = (port_info->active_lane_map & (1 << port_config->host_lanes[0])) == 0;
            // need to properly select line side main lane, based off which switchover group is active
            port_info->host_main_lane = port_config->host_lanes[(port_info->switchover_select == 0) ? 0 : 1];
        }
        port_config->speed = host_speed * MIN(port_config->host_count, port_config->line_count);
    }

    // now map unidirectional information to tx
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    if (port_info->direction != CR_PORT_DIR_BIDIRECTIONAL) {
        if (port_config->mode == CR_PORT_RETIMER) {
            int* tx_lanes =
                (port_info->direction == CR_PORT_DIR_HOST_TO_LINE) ? port_config->line_lanes : port_config->host_lanes;
            int* rx_lanes =
                (port_info->direction == CR_PORT_DIR_HOST_TO_LINE) ? port_config->host_lanes : port_config->line_lanes;

            size_t num_lanes = port_config->host_count;
            for (size_t i = 0; i < num_lanes; i++) {
                se_slice->tx_lane_mode[tx_lanes[i]] = se_slice->lane_mode[rx_lanes[i]];
                se_slice->uni_tx_lane_map[tx_lanes[i]] = rx_lanes[i];
                se_slice->uni_rx_lane_map[rx_lanes[i]] = tx_lanes[i];
                se_slice->uni_tx_port_map[tx_lanes[i]] = UNI_PORT_REIMER;
                se_slice->uni_rx_port_map[rx_lanes[i]] = UNI_PORT_REIMER;
            }
        }
    }

    return CR_OK;
}

CredoError_t se_port_link_internal(CredoSlice_t* slice, unsigned port_id, unsigned phy_rdy, CredoSide_t side,
                                   int* link) {
    *link = 1;
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    SePortInfo_t port_info = se_slice->port_info[port_id];
    CredoPortSetup_t port_setup = port_info.setup;

    if (!port_info.started) {
        *link = 0;
        return CR_OK;
    }

    // check port state
    unsigned state = 0;
    unsigned main_lane = (side == CR_SIDE_HOST) ? port_info.host_main_lane : port_info.line_main_lane;
    if (port_setup.mode == CR_PORT_RETIMER || port_setup.mode == CR_PORT_SWITCHOVER_RETIMER) {
        ERR_PROPS(se_fw_get_retimer_state(slice, main_lane, &state));
        if (state != FW_RT_STATE_TRACK) *link = 0;
    } else if (port_setup.mode == CR_PORT_BITMUX) {
        ERR_PROPS(se_fw_get_bitmux_state(slice, main_lane, &state));
        if (state != FW_BM_STATE_DONE) *link = 0;
    } else {
        *link = 0;
        LOGS_ERROR("[Port link] Internal ERROR");
        return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t se_port_link(CredoSlice_t* slice, unsigned port_id, int link[]) {
    unsigned rdy = 0;

    ERR_PROPS(se_port_link_internal(slice, port_id, rdy, CR_SIDE_HOST, link + 0));
    ERR_PROPS(se_port_link_internal(slice, port_id, rdy, CR_SIDE_LINE, link + 1));

    return CR_OK;
}

CredoError_t se_port_link_host(CredoSlice_t* slice, unsigned port_id, int* link) {
    unsigned rdy = 0;
    ERR_PROPS(se_port_link_internal(slice, port_id, rdy, CR_SIDE_HOST, link));

    return CR_OK;
}

CredoError_t se_port_link_line(CredoSlice_t* slice, unsigned port_id, int* link) {
    unsigned rdy = 0;
    ERR_PROPS(se_port_link_internal(slice, port_id, rdy, CR_SIDE_LINE, link));

    return CR_OK;
}

CredoError_t se_port_is_link_up(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool* up) {
    int link;
    if (side == CR_SIDE_HOST) {
        ERR_PROPS(se_port_link_host(slice, port_id, &link));
        *up = (link != 0);
    } else {
        ERR_PROPS(se_port_link_line(slice, port_id, &link));
        *up = (link != 0);
    }
    return CR_OK;
}

CredoError_t se_port_assign_id(CredoSlice_t* slice, unsigned* port_id) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    for (unsigned i = 0; i < slice->desc->port_count; i++) {
        if (!se_slice->port_info[i].started) {
            *port_id = i;
            LOGS_INFO("[Port Build] Auto assigning port id %u", i);
            return CR_OK;
        }
    }
    LOGS_ERROR("No free ports available to assign");
    return CR_FAIL;
}

CredoError_t se_port_build(CredoSlice_t* slice, uint32_t port_id, const CredoPortSetup_t* setup) {
    if (port_id == CR_PORT_AUTO_ASSIGN_ID) {
        LOGS_ERROR("[Port Build] Use auto assign function");
        return CR_INVALID_ARGS;
    }

    if (port_id > slice->desc->port_count) {
        return CR_INVALID_ARGS;
    }

    // do as much validation as we reasonably can

    unsigned host_lane_mask = 0;
    unsigned line_lane_mask = 0;

    unsigned lane_count = slice->desc->lane_count;
    unsigned side_lane_count = lane_count >> 1;

    // check for a valid number of lanes
    if (setup->host_count == 0 || setup->host_count > side_lane_count) {
        LOGS_ERROR("[Port Build] Invalid host lane count");
        return CR_INVALID_ARGS;
    }

    if (setup->line_count == 0 || setup->line_count > side_lane_count) {
        LOGS_ERROR("[Port Build] Invalid line lane count");
        return CR_INVALID_ARGS;
    }

    // check for duplicates and incorrect lane side

    // host side
    for (unsigned i = 0; i < setup->host_count; i++) {
        int lane = setup->host_lanes[i];
        if (lane < 0 || lane >= side_lane_count) {
            LOGS_ERROR("[Port Build] Invalid host lane");
            return CR_INVALID_ARGS;
        }

        unsigned old_mask = host_lane_mask;
        host_lane_mask |= 1 << setup->host_lanes[i];
        if (old_mask == host_lane_mask) {
            LOGS_ERROR("[Port Build] Duplicate host lanes");
            return CR_INVALID_ARGS;
        }
    }

    // line side
    for (unsigned i = 0; i < setup->line_count; i++) {
        int lane = setup->line_lanes[i];
        if (lane < side_lane_count || lane >= lane_count) {
            LOGS_ERROR("[Port Build] Invalid line lane");
            return CR_INVALID_ARGS;
        }

        unsigned old_mask = line_lane_mask;
        line_lane_mask |= 1 << setup->line_lanes[i];
        if (old_mask == line_lane_mask) {
            LOGS_ERROR("[Port Build] Duplicate line lanes");
            return CR_INVALID_ARGS;
        }
    }

    SeSlice_t* se_slice = (SeSlice_t*)slice;

    if (se_slice->port_info[port_id].started) {
        LOGS_ERROR("[Port Build] Port already started");
        return CR_FAIL;
    }

    memset(&se_slice->port_info[port_id], 0, sizeof(SePortInfo_t));
    se_slice->port_info[port_id].setup = *setup;
    se_slice->port_info[port_id].built = true;
    se_slice->port_info[port_id].started = false;  // not started
    se_slice->port_info[port_id].direction = CR_PORT_DIR_BIDIRECTIONAL;
    se_slice->port_info[port_id].host_lt = false;
    se_slice->port_info[port_id].line_lt = false;
    se_slice->port_info[port_id].line_an = false;
    se_slice->port_info[port_id].an_override = false;
    se_slice->port_info[port_id].is_50g_nrz_mode = false;
    se_slice->port_info[port_id].active_lane_map = 0;
    se_slice->port_info[port_id].host_main_lane = setup->host_lanes[0];
    se_slice->port_info[port_id].line_main_lane = setup->line_lanes[0];
    se_slice->port_info[port_id].flexspeed_kbps = 0;
    // isc
    se_slice->port_info[port_id].isc.enable = false;
    se_slice->port_info[port_id].isc.an_agent = false;
    se_slice->port_info[port_id].isc.lane_map_a = 0;
    se_slice->port_info[port_id].isc.lane_map_b = 0;

    return CR_OK;
}

CredoError_t se_port_start(CredoSlice_t* slice, uint32_t port_id, bool force) {
    if (force) {
        LOGS_ERROR("force feature is removed in screaming eagle.");
        return CR_INVALID_ARGS;
    }
    if (port_id > slice->desc->port_count) {
        return CR_INVALID_ARGS;
    }
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    SePortInfo_t* port_setup = &se_slice->port_info[port_id];
    if (!port_setup->built) {
        LOGS_ERROR("[Port Start] Port for id=%u not built", port_id);
        return CR_INVALID_ARGS;
    }

    if (port_setup->started) {
        LOGS_ERROR("[Port Start] Port is already started");
        return CR_INVALID_ARGS;
    }

    switch (port_setup->setup.mode) {
        case CR_PORT_SWITCHOVER_RETIMER:
            ERR_PROPS(se_fw_config_switchover_mode(slice, port_id, port_setup));
            break;
        case CR_PORT_RETIMER:
            ERR_PROPS(se_fw_config_retimer(slice, port_id, port_setup));
            break;
        case CR_PORT_BITMUX:
            ERR_PROPS(se_fw_config_bitmux(slice, port_id, port_setup));
            break;
        default:
            return CR_UNSUPPORTED;
    }
    port_setup->started = true;

    // sleep_ms(FW_CONFIG_MODE_DELAY);
    return CR_OK;
}

CredoError_t se_port_get_setup(CredoSlice_t* slice, uint32_t port_id, bool* started, CredoPortSetup_t* setup) {
    if (port_id > slice->desc->port_count) {
        return CR_INVALID_ARGS;
    }

    SeSlice_t* se_slice = (SeSlice_t*)slice;
    if (!se_slice->port_info[port_id].started) {
        *started = false;
        return CR_OK;
    }
    *started = true;
    *setup = se_slice->port_info[port_id].setup;
    return CR_OK;
}

void se_port_options_to_string(CredoSlice_t* slice, const SePortInfo_t* port_info, char* buf, uint32_t size) {
    sbs* opt_str = sbsnew(&(sbs){0}, "", buf, size);

    if (port_info->line_an && port_info->line_lt) {
        sbscat(opt_str, "LINE_ANLT,");
    } else if (port_info->line_an) {
        sbscat(opt_str, "LINE_AN,");
    } else if (port_info->line_lt) {
        sbscat(opt_str, "LINE_LT,");
    }

    if (port_info->host_lt) {
        sbscat(opt_str, "HOST_LT,");
    }
    sbstrim(opt_str, ",");
}

CredoError_t se_port_switchover_switch(CredoSlice_t* slice, uint32_t port_id, int lane) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    SePortInfo_t* port_info = &se_slice->port_info[port_id];
    bool is_a_side_bcst = (port_info->setup.host_count < port_info->setup.line_count) ? true : false;
    unsigned new_active_map = port_info->active_lane_map;
    unsigned active_count = (is_a_side_bcst) ? port_info->setup.host_count : port_info->setup.line_count;

    for (unsigned i = 0; i < active_count; i++) {
        int lane_1 = -1, lane_2 = -1;
        if (is_a_side_bcst) {
            lane_1 = port_info->setup.line_lanes[i * 2];
            lane_2 = port_info->setup.line_lanes[i * 2 + 1];
        } else {
            lane_1 = port_info->setup.host_lanes[i * 2];
            lane_2 = port_info->setup.host_lanes[i * 2 + 1];
        }

        if (lane == lane_1 || lane == lane_2 || lane == 0xFF) {
            new_active_map ^= (1 << lane_1);
            new_active_map ^= (1 << lane_2);

            if (i == 0 && (new_active_map & (1 << lane_1))) {
                if (lane_1 >= 8) {
                    port_info->line_main_lane = lane_1;
                } else {
                    port_info->host_main_lane = lane_1;
                }
            } else if (i == 0 && (new_active_map & (1 << lane_2))) {
                if (lane_1 >= 8) {
                    port_info->line_main_lane = lane_2;
                } else {
                    port_info->host_main_lane = lane_2;
                }
            }
        }
    }

    ERR_PROPS(se_fw_switchover_switch(slice, new_active_map, port_info->active_lane_map));
    port_info->active_lane_map = new_active_map;
    return CR_OK;
}

CredoError_t se_port_get_isc_enable(CredoSlice_t* slice, uint32_t port_id, int* value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    int lane = se_slice->port_info[port_id].host_main_lane;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_ISC_ENABLE, (unsigned*)value));
    return CR_OK;
}

CredoError_t se_port_get_active_lane_list(CredoSlice_t* slice, uint32_t port_id, int active_lane_list[32]) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    int cnt = 0;
    int active_lane_map = se_slice->port_info[port_id].active_lane_map;
    for (int lane = 0; lane < slice->desc->lane_count; lane++) {
        if (active_lane_map & (1 << lane)) {
            active_lane_list[cnt++] = lane;
        }
    }

    active_lane_list[cnt] = -1;
    return CR_OK;
}

CredoError_t se_port_get_switchover_configured(CredoSlice_t* slice, bool* value) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    *value = false;
    for (int port_id = 0; port_id < slice->desc->port_count; port_id++) {
        if (se_slice->port_info[port_id].setup.mode == CR_PORT_SWITCHOVER_RETIMER) {
            *value = true;
            break;
        }
    }
    return CR_OK;
}
