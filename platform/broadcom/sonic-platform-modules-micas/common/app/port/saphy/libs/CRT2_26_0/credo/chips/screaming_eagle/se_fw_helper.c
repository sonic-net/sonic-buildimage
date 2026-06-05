#include "se_fw_helper.h"
#include "se_fw_state.h"

#include "sdk.h"

#include <stdlib.h>

const char* se_fw_speed_string(FirmwareSpeed_t speed) {
    switch (speed) {
        case SPEED_OFF:
            return "OFF";
        case SPEED_1G:
            return "1G";
        case SPEED_10G:
            return "10G";
        case SPEED_20G:
            return "20G";
        case SPEED_25G:
            return "25G";
        case SPEED_26G:
            return "26G";
        case SPEED_51G:
            return "51G";
        case SPEED_53G:
            return "53G";
        case SPEED_56G:
            return "56G";
        case SPEED_90G:
            return "90G";
        case SPEED_98G:
            return "98G";
        case SPEED_106G:
            return "106G";
        case SPEED_112G:
            return "112G";
        default:
            return "???";
    }
}

const char* se_fw_config_mode_string(unsigned firmware_mode) {
    switch (firmware_mode) {
        case FW_MODE_RETIMER:
            return "RETIMER";
        case FW_MODE_UNI_RETIMER:
            return "UNI_RETIMER";
        case FW_MODE_SWITCHOVER_RETIMER:
            return "SWITCHOVER_RETIMER";
        case FW_MODE_BITMUX_A1B2:
        case FW_MODE_BITMUX_A2B1:
            return "BITMUX";
        case FW_MODE_PHY:
            return "PHY";
        case FW_MODE_LOOPBACK:
            return "LOOPBACK";
        case FW_MODE_OFF:
            return "OFF";
        default:
            return "???";
    }
}

const char* se_fw_port_direction_string(unsigned direction) {
    switch (direction) {
        case CR_PORT_DIR_BIDIRECTIONAL:
            return "<->";
        case CR_PORT_DIR_HOST_TO_LINE:
            return "-->";
        case CR_PORT_DIR_LINE_TO_HOST:
            return "<--";
        default:
            return "???";
    }
}

#define FW_STATE(X)    \
    case FW_STATE_##X: \
        return #X;

const char* se_fw_state_mode_string(unsigned state_mode) {
    switch (state_mode) {
        case FW_STATE_MODE_PAM4:
            return "PAM4";
        case FW_STATE_MODE_NRZ:
            return "NRZ";
        case 0:
            return "";
        default:
            return "Unknown";
    }
}

const char* se_fw_state_major_string(unsigned state_major) {
    switch (state_major) {
        FW_STATE(TOP)
        FW_STATE(OPT_STARTUP)
        FW_STATE(OPT_PRESET)
        FW_STATE(OPT_FFE_FLT)
        FW_STATE(OPT_CTLE_CP2)
        FW_STATE(OPT_F1_EYE)
        FW_STATE(OPT_CM1_EYE)
        FW_STATE(OPT_F0_EYE)
        FW_STATE(OPT_SKEF_EYE)
        FW_STATE(OPT_GAIN)
        FW_STATE(ANLT)
        FW_STATE(LT)
        FW_STATE(THDLY_ADAPT)
        FW_STATE(OPT_LINKUP)
        default:
            return "Unknown";
    }
}

const char* se_fw_state_minor_string(unsigned state_minor) {
    switch (state_minor) {
        FW_STATE(MODE_QUIET)
        FW_STATE(MODE_SQUELCH)
        FW_STATE(INIT_MODE)
        FW_STATE(INIT_NRZ)
        FW_STATE(INIT_PAM4)
        FW_STATE(INIT_AN)
        FW_STATE(START_PLL_CAL)
        FW_STATE(RX_DISABLE)
        FW_STATE(PLL_CAL_DONE)
        FW_STATE(DESTROYING)

        FW_STATE(OPT_START_NRZ)
        FW_STATE(OPT_START_PAM4)
        FW_STATE(OPT_START_AN)
        FW_STATE(OPT_START_NRZ_LT)
        FW_STATE(OPT_START_PAM4_LT)
        FW_STATE(OPT_SINGAL_DETECT)
        FW_STATE(OPT_CHECK_CDR)
        FW_STATE(OPT_SD_RESET)

        FW_STATE(OPT_PRESET_CH_EST)
        FW_STATE(OPT_PRESET_INIT)
        FW_STATE(OPT_PRESET_CAL_TABLE_RANGE)
        FW_STATE(OPT_PRESET_ATTN_SEARCH)
        FW_STATE(OPT_PRESET_DSP_STARTUP)
        FW_STATE(OPT_PRESET_OPT_F0_ENV)
        FW_STATE(OPT_PRESET_OPT_F0_VGA)
        FW_STATE(OPT_PRESET_THDLY_ADAPT)
        FW_STATE(OPT_PRESET_OPT_CM1)
        FW_STATE(OPT_PRESET_CHECK_EYE_BALC)
        FW_STATE(OPT_PRESET_CHECK_CDR)
        FW_STATE(OPT_PRESET_CHECK_ANA_FE)
        FW_STATE(OPT_PRESET_CHECK_GAIN)
        FW_STATE(OPT_PRESET_GET_CANDIDATE)
        FW_STATE(OPT_PRESET_BEST_INDEX_BROKEN)
        FW_STATE(OPT_PRESET_OW)

        FW_STATE(OPT_PRESET_ENTRY_SHORT_CH)
        FW_STATE(OPT_PRESET_ENTRY_MIDDLE_CH)
        FW_STATE(OPT_PRESET_ENTRY_LONG_CH)
        FW_STATE(OPT_PRESET_CS_ADJ_PLUS)
        FW_STATE(OPT_PRESET_CS_ADJ_MINUS)
        FW_STATE(OPT_PRESET_THDLY_OW)
        FW_STATE(OPT_PRESET_CS_GRP_SHRT_2_LNG)
        FW_STATE(OPT_PRESET_CS_GRP_LNG_2_SHRT)
        FW_STATE(OPT_PRESET_PPM_PATCH)
        FW_STATE(OPT_PRESET_PPM_PATCH_CDR_UNLOCK)
        FW_STATE(OPT_PRESET_INIT_DONE)
        FW_STATE(OPT_PRESET_OFF_CTEL_IND)
        FW_STATE(OPT_PRESET_DO_LOAD)

        FW_STATE(OPT_CTLE_FIND_CUR_IDX)
        FW_STATE(OPT_CTLE_START_OPT)
        FW_STATE(OPT_CTLE_OPT_F1_BY_CP1)
        FW_STATE(OPT_CTLE_CP2_HIT_BOUNDARY)
        FW_STATE(OPT_CTLE_CP2_DIR_CHANGE)
        FW_STATE(OPT_CTLE_CP2_OUT_OF_RANGE)
        FW_STATE(OPT_CTLE_F1_SATURATED)
        FW_STATE(OPT_CTLE_GET_CANDIDATE)
        FW_STATE(OPT_PRESET_PPM_PATCH_DONE)
        FW_STATE(OPT_PRESET_PPM_PATCH_SIGNAL_LOST)

        FW_STATE(OPT_CM1_EYE_CHECK_RANGE)
        FW_STATE(OPT_CM1_EYE_START)
        FW_STATE(OPT_CM1_EYE_CAL_AVG)
        FW_STATE(OPT_CM1_EYE_CAL_MOVING_WINDOW)
        FW_STATE(OPT_CM1_EYE_DATA_ERROR)
        FW_STATE(OPT_CM1_EYE_BAD_RESULT)

        FW_STATE(OPT_F0_EYE_ENV_ADJ_1)
        FW_STATE(OPT_F0_EYE_ENV_ADJ_2)
        FW_STATE(OPT_F0_EYE_VGA_PROTECTION)
        FW_STATE(OPT_F0_EYE_CS_BY_ENV)
        FW_STATE(OPT_F0_EYE_FIN_ADJ)

        FW_STATE(OPT_BG_INIT)
        FW_STATE(OPT_BG_READ_EYE)
        FW_STATE(OPT_BG_READ_ISI)
        FW_STATE(OPT_BG_GET_FE_INFO)
        FW_STATE(OPT_BG_CHECK_SMALL_EYE)
        FW_STATE(OPT_BG_CHECK_EYE)
        FW_STATE(OPT_BG_TOP_RESET)
        FW_STATE(OPT_BG_COMP_VGA_CS_ATT_ICTRL)
        FW_STATE(OPT_BG_COMP_CS_VGA_REF_CTL)
        FW_STATE(OPT_BG_COMP_GAIN_PROTECT)

        FW_STATE(RETIMER_FIFO_ADJUST)
        FW_STATE(RETIMER_TX_EANBLE)
        case 0:
            return "";
        default:
            return "Unknown";
    }
}

unsigned se_fw_speed_kbps(FirmwareSpeed_t speed_index) {
    switch (speed_index) {
        case SPEED_OFF:
            return 0;
        case SPEED_1G:
            return 1250000;
        case SPEED_10G:
            return 10312500;  // 10G * (33/32)
        case SPEED_20G:
            return 20625000;  // 20G * (33/32)
        case SPEED_25G:
            return 25781250;  // 25G * (33/32)
        case SPEED_26G:
            return 26562500;  // 25G * (34/32)
        case SPEED_51G:
            return 51562500;  // 50G * (33/32)
        case SPEED_53G:
            return 53125000;  // 50G * (34/32)
        case SPEED_56G:
            return 56250000;
        case SPEED_90G:
            return 90000000;
        case SPEED_98G:
            return 98200000;
        case SPEED_106G:
            return 106250000;  // 50G * (34/32) * 2
        case SPEED_112G:
            return 112000000;
        default:
            return 0;
    }
}

unsigned se_fw_translate_speed(unsigned speed) {
    unsigned cmd = 0xFFFF;
    switch (speed) {
        case CONFIG_1G:
        case CONFIG_1P25G:
            cmd = DEFSPEED(1G);
            break;
        case CONFIG_10G:
            cmd = DEFSPEED(10G);
            break;
        case CONFIG_20G:
            cmd = DEFSPEED(20G);
            break;
        case CONFIG_25G:
            cmd = DEFSPEED(25G);
            break;
        case CONFIG_26G:
            cmd = DEFSPEED(26G);
            break;
        case CONFIG_51G:
            cmd = DEFSPEED(51G);
            break;
        case CONFIG_50G:
        case CONFIG_53G:
            cmd = DEFSPEED(53G);
            break;
        case CONFIG_56G:
            cmd = DEFSPEED(56G);
            break;
        case CONFIG_90G:
            cmd = DEFSPEED(90G);
            break;
        case CONFIG_98G:
        case 98200:
            cmd = DEFSPEED(98G);
            break;
        case CONFIG_100G:
        case CONFIG_106G:
            cmd = DEFSPEED(106G);
            break;
        case CONFIG_112G:
            cmd = DEFSPEED(112G);
            break;
    }
    return cmd;
}

CredoError_t se_fw_translate_multilane(unsigned multilane, unsigned* multilane_code) {
    switch (multilane) {
        case 1:
            *multilane_code = 0;
            break;
        case 2:
            *multilane_code = 1;
            break;
        case 4:
            *multilane_code = 2;
            break;
        case 8:
            *multilane_code = 3;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t se_fw_translate_multilane_reverse(unsigned multilane_code, unsigned* multilane) {
    switch (multilane_code) {
        case 0:
            *multilane = 1;
            break;
        case 1:
            *multilane = 2;
            break;
        case 2:
            *multilane = 4;
            break;
        case 3:
            *multilane = 8;
            break;
        default:
            return CR_NOTIMPLEMENTED;
    }
    return CR_OK;
}

CredoError_t se_fw_translate_config_mode_reverse(unsigned firmware_mode, CredoPortConnectionMode_t* mode) {
    switch (firmware_mode) {
        case FW_MODE_RETIMER:
        case FW_MODE_UNI_RETIMER:
            *mode = CR_PORT_RETIMER;
            break;
        case FW_MODE_SWITCHOVER_RETIMER:
            *mode = CR_PORT_SWITCHOVER_RETIMER;
            break;
        case FW_MODE_BITMUX_A1B2:
        case FW_MODE_BITMUX_A2B1:
        case FW_MODE_UNI_BITMUX_A1B2:
        case FW_MODE_UNI_BITMUX_A2B1:
            *mode = CR_PORT_BITMUX;
            break;
        default:
            return CR_UNSUPPORTED;
    }
    return CR_OK;
}

CredoError_t se_fw_translate_lane_speed_reverse(FirmwareSpeed_t fw_speed, unsigned* speed) {
    switch (fw_speed) {
        case SPEED_112G:
            *speed = CONFIG_112G;
            break;
        case SPEED_106G:
            *speed = CONFIG_106G;
            break;
        case SPEED_98G:
            *speed = CONFIG_98G;
            break;
        case SPEED_90G:
            *speed = CONFIG_90G;
            break;
        case SPEED_56G:
            *speed = CONFIG_56G;
            break;
        case SPEED_53G:
            *speed = CONFIG_53G;
            break;
        case SPEED_51G:
            *speed = CONFIG_51G;
            break;
        case SPEED_26G:
            *speed = CONFIG_26G;
            break;
        case SPEED_25G:
            *speed = CONFIG_25G;
            break;
        case SPEED_20G:
            *speed = CONFIG_20G;
            break;
        case SPEED_10G:
            *speed = CONFIG_10G;
            break;
        case SPEED_1G:
            *speed = CONFIG_1G;
            break;
        case SPEED_OFF:
            *speed = 0;
            break;
        default:
            return CR_UNSUPPORTED;
    }
    return CR_OK;
}

CredoError_t se_fw_translate_state_reverse(unsigned fw_lt_state, CredoLinkTrainingState_t* lt_state) {
    switch (fw_lt_state) {
        case FW_STATE_LT_IDLE:
            *lt_state = CR_LT_STATE_IDLE;
            break;
        case FW_STATE_LT_START:
            *lt_state = CR_LT_STATE_START;
            break;
        case FW_STATE_LT_WAIT_FRAME_LOCK:
            *lt_state = CR_LT_STATE_PAM2_WAIT_FRAME_LOCK;
            break;
        case FW_STATE_LT_WAIT_REMOTE_LOCK:
            *lt_state = CR_LT_STATE_PAM2_WAIT_REMOTE_LOCK;
            break;
        case FW_STATE_LT_WAIT_INITIAL_CMD:
            *lt_state = CR_LT_STATE_PAM2_WAIT_INITIAL_CMD;
            break;
        case FW_STATE_LT_TX_ADJUST:
            *lt_state = CR_LT_STATE_PAM2_TX_ADJUST;
            break;
        case FW_STATE_LT_SWITCH_TO_PAM4:
            *lt_state = CR_LT_STATE_PAM4_START;
            break;
        case FW_STATE_LT_PAM4_WAIT_FRAME_LOCK:
            *lt_state = CR_LT_STATE_PAM4_WAIT_FRAME_LOCK;
            break;
        case FW_STATE_LT_PAM4_TX_ADJUST:
            *lt_state = CR_LT_STATE_PAM4_TX_ADJUST;
            break;
        case FW_STATE_LT_END:
            *lt_state = CR_LT_STATE_LINK_UP;
            break;
        default:
            *lt_state = CR_LT_STATE_UNKNOWN;
            break;
    }
    return CR_OK;
}

CredoError_t se_fw_translate_flags(const SePortInfo_t* info, unsigned* detail2) {
    /* LT flag must check first */
    if (info->line_lt) {
        *detail2 |= FW_OPTION_LINE_SIDE_LT;
    }
    if (info->line_an) {
        *detail2 |= (FW_OPTION_LINE_SIDE_AN);
    }
    if (info->host_lt) {
        *detail2 |= FW_OPTION_SYS_SIDE_LT;
    }
    return CR_OK;
}
