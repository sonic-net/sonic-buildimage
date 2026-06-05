#include "project.h"
#include "be_device.h"
#include "be_functions.h"

#include "canary/canary_serdes.h"
#include "common/common_firmware.h"
#include "common/params.h"

#include "utility.h"
#include "sdk.h"

#include <string.h>

CredoError_t be_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane) {
    *host_lane = 8;
    *line_lane = 8;
    return CR_OK;
}

unsigned be_is_valid_lane(CredoSlice_t* slice, int lane) {
    int host_lane, line_lane;
    be_get_lane_count(slice, &host_lane, &line_lane);
    return lane >= 0 && lane < host_lane + line_lane;
}

CredoError_t be_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    if (lane < 0 || lane >= slice->desc->lane_count) {
        LOGS_ERROR("Lane %d out of range", lane);
        return CR_FAIL;
    }
    BeSlice_t* be_slice = (BeSlice_t*)slice;
    *mode = be_slice->lane_mode[lane];
    return CR_OK;
}

CredoError_t be_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    if (lane < 0 || lane >= slice->desc->lane_count) {
        LOGS_ERROR("Lane %d out of range", lane);
        return CR_FAIL;
    }
    BeSlice_t* be_slice = (BeSlice_t*)slice;
    be_slice->lane_mode[lane] = mode;
    return CR_OK;
}

CredoError_t be_update_lane_mode(CredoSlice_t* slice, int lane) {
    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));

    if (fw_status == 1) {
        unsigned opt_mode;
        ERR_PROPS(be_fw_get_opt_mode(slice, lane, &opt_mode));
        switch (opt_mode) {
            case CR_LMODE_PAM4:
            case CR_LMODE_NRZ:
            case CR_LMODE_OFF:
            case CR_LMODE_DISABLE:
                break;
            default:
                opt_mode = CR_LMODE_OFF;  // force to be off for unknown mode
        }

        ERR_PROPS(hal_set_lane_mode(slice, lane, opt_mode));
    } else {
        unsigned pam4_en;
        ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PAM4_EN, &pam4_en));
        ERR_PROPS(hal_set_lane_mode(slice, lane, (pam4_en == 1) ? CR_LMODE_PAM4 : CR_LMODE_NRZ));
    }

    return CR_OK;
}

CredoError_t be_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));

    if (fw_status == 1) {
        ERR_PROPS(be_fw_get_lane_speed(slice, lane, speed_kbps));
    } else {
        *speed_kbps = 0;
    }
    return CR_OK;
}

CredoError_t be_disable_lane(CredoSlice_t* slice, int lane) {
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_AGC_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_REFCLKBUF, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_AGC_1_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_AGCDL_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_AGC, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_AGCDL, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_ADC_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_CLKCOMPREG_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_ADC_1_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_INTP_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_PLL_INTP, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_INTP, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PU_ADC, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_CLKCOMP, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_CLKCOMPREG, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_PLL, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_RVDDLOOP, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_RVDD, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_EN_RVDDVCO, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PU_BG, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_TX_PU_PLL_INTP, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_PU_VDRV_MA, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_PU_VDRV, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PLL_PU, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PU_RVDDLOOP, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_EN_RVDDVCO, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PU_RVDD, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PU_BG, 0));

    ERR_PROPS(hal_set_lane_mode(slice, lane, CR_LMODE_DISABLE));
    return CR_OK;
}

CredoError_t be_logic_reset_lane(CredoSlice_t* slice, int lane) {
    unsigned val;
    ERR_PROPS(readTop(slice, REG_LANE_LOGIC_RST, &val));

    val |= (1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_LOGIC_RST, val));

    val &= ~(1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_LOGIC_RST, val));
    return CR_OK;
}

CredoError_t be_reg_reset_lane(CredoSlice_t* slice, int lane) {
    unsigned val;
    ERR_PROPS(readTop(slice, REG_LANE_REGISTER_RST, &val));

    val |= (1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_REGISTER_RST, val));

    val &= ~(1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_REGISTER_RST, val));
    return CR_OK;
}

#define CAL_TIMEOUT       10000  // 10ms
#define CAL_RETRY_CNT     20
#define VCOCAP_MIN        20
#define VCOCAP_MAX        120
#define VCOCAP_WINDOW_MAX 0x4000
#define VCOCAP_WINDOW_MIN 0x3000

CredoError_t be_top_pll_cal(CredoSlice_t* slice, int lane) {
    CredoTime_t start_time;
    int i;

    ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_VCTRL_LOOPEN_SEL, 0x04));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_LO_OPEN, 0x01));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_FCAL_TIMER_WINDOW, 0x3FFF));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_PD_FCAL, 0x00));
    ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_FCAL_START, 0x00));

    for (i = VCOCAP_MIN; i < VCOCAP_MAX; i += 2) {
        unsigned fcal_done, window_readout;
        unsigned retry_cnt = 0;
        writeRegLane(slice, lane, REG_TOP_PLL_LCVCOCAP, i);
        writeRegLane(slice, lane, REG_TOP_PLL_FCAL_START, 1);
        get_time(&start_time);
        while (!is_timeout(&start_time, CAL_TIMEOUT) || retry_cnt < CAL_RETRY_CNT) {
            ERR_PROPS(readRegLane(slice, lane, REG_TOP_PLL_FCAL_DONE, &fcal_done));
            retry_cnt++;
            if (fcal_done == 1) break;
        }
        if (fcal_done == 0) {
            LOGS_ERROR("[Top PLL calibration] Timeout");
            return CR_FAIL;
        }
        ERR_PROPS(readRegLane(slice, lane, REG_TOP_PLL_FCAL_CNT_OP, &window_readout));
        ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_FCAL_START, 0));
        LOGS_DEBUG("cur_vcocap = %d, window_rd = %d", i, window_readout);
        if (window_readout > VCOCAP_WINDOW_MIN && window_readout < VCOCAP_WINDOW_MAX) {
            ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_LCVCOCAP, i - 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_VCTRL_LOOPEN_SEL, 0x00));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_FCAL_TIMER_WINDOW, 0x0000));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_LO_OPEN, 0x00));
            ERR_PROPS(writeRegLane(slice, lane, REG_TOP_PLL_PD_FCAL, 0x01));
            return CR_OK;
        }
    }

    LOGS_ERROR("[Top PLL calibration] Failed");
    return CR_FAIL;
}

CredoError_t be_get_top_pll_cap(CredoSlice_t* slice, unsigned* caps) {
    unsigned value;
    int i;
    *caps = 0;
    for (i = 0; i < 2; i++) {
        ERR_PROPS(readRegLane(slice, i, REG_TOP_PLL_LCVCOCAP, &value));
        *caps = (*caps << 8) | value;
    }
    return CR_OK;
}

CredoError_t be_top_pll_init(CredoSlice_t* slice) {
    int i;
    ERR_PROPS(writeTop(slice, 0x9500, 0x1aa0));
    ERR_PROPS(writeTop(slice, 0x9500, 0x0aa0));
    ERR_PROPS(writeTop(slice, 0x9501, 0x8a5b));
    ERR_PROPS(writeTop(slice, 0x9502, 0x03e6));
    ERR_PROPS(writeTop(slice, 0x9503, 0x6d86));
    ERR_PROPS(writeTop(slice, 0x9504, 0x7180));
    ERR_PROPS(writeTop(slice, 0x9505, 0x9000));
    ERR_PROPS(writeTop(slice, 0x9506, 0x0000));
    ERR_PROPS(writeTop(slice, 0x9507, 0x0280));
    ERR_PROPS(writeTop(slice, 0x950a, 0x4040));
    ERR_PROPS(writeTop(slice, 0x950b, 0x0000));
    ERR_PROPS(writeTop(slice, 0x950d, 0x0000));
    ERR_PROPS(writeTop(slice, 0x9510, 0xb670));
    ERR_PROPS(writeTop(slice, 0x9511, 0x0000));
    ERR_PROPS(writeTop(slice, 0x9512, 0x7de8));
    ERR_PROPS(writeTop(slice, 0x9513, 0x0800));
    ERR_PROPS(writeTop(slice, 0x9512, 0xfde8));
    ERR_PROPS(writeTop(slice, 0x9501, 0x8a5f));
    ERR_PROPS(writeTop(slice, 0x9501, 0x89df));

    ERR_PROPS(writeTop(slice, 0x9600, 0x1aa0));
    ERR_PROPS(writeTop(slice, 0x9600, 0x0aa0));
    ERR_PROPS(writeTop(slice, 0x9601, 0x8a5b));
    ERR_PROPS(writeTop(slice, 0x9602, 0x03e6));
    ERR_PROPS(writeTop(slice, 0x9603, 0x6d86));
    ERR_PROPS(writeTop(slice, 0x9604, 0x7180));
    ERR_PROPS(writeTop(slice, 0x9605, 0x9000));
    ERR_PROPS(writeTop(slice, 0x9606, 0x0000));
    ERR_PROPS(writeTop(slice, 0x9607, 0x0280));
    ERR_PROPS(writeTop(slice, 0x960a, 0x4040));
    ERR_PROPS(writeTop(slice, 0x960b, 0x0000));
    ERR_PROPS(writeTop(slice, 0x960d, 0x0000));
    ERR_PROPS(writeTop(slice, 0x9610, 0xb670));
    ERR_PROPS(writeTop(slice, 0x9611, 0x0000));
    ERR_PROPS(writeTop(slice, 0x9612, 0x7de8));
    ERR_PROPS(writeTop(slice, 0x9613, 0x0800));
    ERR_PROPS(writeTop(slice, 0x9612, 0xfde8));
    ERR_PROPS(writeTop(slice, 0x9601, 0x8a5f));
    ERR_PROPS(writeTop(slice, 0x9601, 0x89df));
    sleep_us(100);
    for (i = 0; i < 2; i++) {
        ERR_PROPS(be_top_pll_cal(slice, i));
    }
    return CR_OK;
}

CredoError_t be_tx_control(CredoSlice_t* slice, int lane, FwTxSource_t source) {
    unsigned fw_source_ctl;
    unsigned response;
    if (source == FW_TX_SOURCE_NOFORCE) {
        fw_source_ctl = 0;
    } else {
        fw_source_ctl = ((unsigned)source << 2) | (1 << 0);
    }
    return common_fw_cmd(slice, FW_CMD_CONFIG_TX + fw_source_ctl, 1 << lane, &response, NULL);
}

CredoError_t be_tx_disable(CredoSlice_t* slice, int lane) {
    return be_tx_control(slice, lane, FW_TX_SOURCE_QUIET);
}

CredoError_t be_tx_no_disable(CredoSlice_t* slice, int lane) {
    return be_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
}

#define TX_CONTROL_STATE_MASK          (3 << 0)
#define TX_CONTROL_FORCED_SOURCE_SHIFT 2
#define TX_CONTROL_FORCED_SOURCE_MASK  (3 << TX_CONTROL_FORCED_SOURCE_SHIFT)
#define TX_CONTROL_NORMAL_SOURCE_SHIFT 4
#define TX_CONTROL_NORMAL_SOURCE_MASK  (3 << TX_CONTROL_NORMAL_SOURCE_SHIFT)

#define TX_CONTROL_SOURCE_QUIET     0
#define TX_CONTROL_SOURCE_PRBS_PAM4 1
#define TX_CONTROL_SOURCE_PRBS_NRZ  2
#define TX_CONTROL_SOURCE_DATA      3

#define TX_CONTROL_STATE_UNKNOWN   0
#define TX_CONTROL_STATE_FORCED    1
#define TX_CONTROL_STATE_NORMAL    2
#define TX_CONTROL_STATE_LOW_POWER 3

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

CredoError_t be_lane_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status) {
    bool tx_pattern_en = false;
    hal_get_tx_test_pattern_enable(slice, lane, &tx_pattern_en);
    if (tx_pattern_en) {
        *status = CR_TX_FORCE_TEST_PATT;
        return CR_OK;
    }

    unsigned tx_status;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_TX_STATUS, &tx_status));

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

CredoError_t be_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned status;

    be_fw_get_status(slice, &status);
    if (status == 1) {
        unsigned patt;
        CredoLaneMode_t mode;
        if (!enable) {
            return be_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
        }
        ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
        if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ) {
            LOGS_WARN("[Set TX PRBS] Unknown lane mode %d, treat as NRZ", mode);
        }
        patt = credo_to_canary(prbs_mode, mode);
        if (patt == CANARY_PRBS_INVALID) {
            LOGS_ERROR("[Set TX PRBS] Unsupported PRBS mode %d", prbs_mode);
            return CR_FAIL;
        }
        if (mode == CR_LMODE_PAM4) {
            ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_PRBS_PAM4));
        } else {
            ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_PRBS_NRZ));
        }
        return writeRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, patt);
    } else {
        LOGS_WARN("Firmware is not running!");
        return canary_set_tx_prbs(slice, lane, enable, prbs_mode);
    }
}

CredoError_t be_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    unsigned status;

    be_fw_get_status(slice, &status);
    if (status == 1) {
        CredoLaneMode_t mode;
        if (enable) {
            ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
            if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ) {
                mode = CR_LMODE_NRZ;
                LOGS_WARN("[Set TX PRBS] Unknown lane mode %d, treat as NRZ", mode);
            }
            if (mode == CR_LMODE_PAM4) {
                ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_PRBS_PAM4));
            } else {
                ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_PRBS_NRZ));
            }
        } else {
            ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE));
        }

    } else {
        LOGS_WARN("Firmware is not running!");
        return canary_set_tx_prbs(slice, lane, enable, 0);  // don't care prbs_mode
    }

    return canary_set_tx_test_pattern_enable(slice, lane, enable);
}

/* RX control */
CredoError_t be_rx_disable(CredoSlice_t* slice, int lane) {
    return common_fw_cmd(slice, FW_CMD_FW_RX_DISABLE + lane, 0, NULL, NULL);
}

CredoError_t be_rx_no_disable(CredoSlice_t* slice, int lane) {
    return common_fw_cmd(slice, FW_CMD_FW_RX_ENABLE + lane, 0, NULL, NULL);
}

CredoError_t be_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    ERR_PROPS(canary_get_rx_ppm(slice, lane, ppm));
    unsigned speed_index, fw_status;
    ERR_PROPS(be_fw_get_status(slice, &fw_status));
    if (fw_status != 1) {
        LOGS_WARN("ppm not calculated as firmware is not running, providing freq_acc");
        return CR_OK;
    }
    ERR_PROPS(common_fw_get_speed_index(slice, lane, &speed_index));

    // need to do some conversion from freq_acc to ppm
    // refer to: https://gitlab.idm.credosemi.com/apps-team/sdk/credo-sdk/-/issues/198
    switch (speed_index) {
        case SPEED_10G:
            *ppm = *ppm * 2;
            break;
        case SPEED_20G:
            *ppm = *ppm * 4;
            break;
        case SPEED_25G:
        case SPEED_26G:
            *ppm = *ppm * 5;
            break;
        default:
            // ppm is just 1:1
            break;
    }

    return CR_OK;
}

/* Set PLL source. Be careful, normally you can only use the other side as source.
 * If set to 8, then it's a loopback. */
CredoError_t be_set_pll_source(CredoSlice_t* slice, int lane, int source) {
    unsigned addr = REG_PLL_SOURCE + ((lane >> 2) ^ 1);
    unsigned shift = (lane & 3) << 2;
    unsigned value;
    ERR_PROPS(readTop(slice, addr, &value));
    value &= ~(0xF << shift);
    value |= source << shift;
    ERR_PROPS(writeTop(slice, addr, value));
    return CR_OK;
}

CredoError_t be_set_trf(CredoSlice_t* slice, int lane, TRF_t trf) {
    switch (trf) {
        case TRF_Off:
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PAM4_TRF_SPEED_SEL, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PAM4_TRF_FREQ, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_NRZ_TRF_SPEED_SEL, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_NRZ_TRF_FREQ, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PI_EN, 0));
            break;
        case TRF_OnetoOne:
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PAM4_TRF_SPEED_SEL, 16));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PAM4_TRF_FREQ, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_NRZ_TRF_SPEED_SEL, 16));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_NRZ_TRF_FREQ, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PI_EN, 1));
            break;
        case TRF_GearBox:
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PAM4_TRF_SPEED_SEL, 20));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PAM4_TRF_FREQ, 2));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_NRZ_TRF_SPEED_SEL, 17));
            ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_NRZ_TRF_FREQ, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PI_EN, 1));
            break;
        default:
            return CR_FAIL;
    }
    return CR_OK;
}

CredoError_t be_set_serdes_loopback(CredoSlice_t* slice, int lane, int enable) {
    unsigned addr = REG_LOOPBACK_ENABLE + (lane % HOST_LANES) * 8;
    unsigned mask = LOOPBACK_MASK >> (lane / HOST_LANES);
    unsigned value;
    ERR_PROPS(readTop(slice, addr, &value));
    if (enable) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    return writeTop(slice, addr, value);
}

#define FIFO_READ_COUNT       16
#define MIN_FIFO_POINTER_DIFF 5

CredoError_t be_adjust_lb_fifo(CredoSlice_t* slice, int lane) {
    unsigned diff[2], difftotal[2] = {0, 0};
    unsigned addr = REG_ADR_DIFF_LB + (lane % HOST_LANES) * 8;
    unsigned value;
    int adjust;

    /* continuous read without delay, FIFO pointer update should be pretty fast */
    for (int i = 0; i < FIFO_READ_COUNT; i++) {
        ERR_PROPS(readTop(slice, addr, &value));
        if (lane >= HOST_LANES) {
            /* B side */
            value >>= ADR_DIFF_B_SHIFT;
        } else {
            /* A side */
            value >>= ADR_DIFF_A_SHIFT;
        }
        value &= 0x3F;
        diff[0] = gray_bin((value)&7);
        diff[1] = gray_bin((value >> 3) & 7);
        difftotal[0] += diff[0];
        difftotal[1] += diff[1];
    }

    /* Adjust */
    addr = REG_W_ADJUST + (lane % HOST_LANES) * 8;
    for (int i = 0; i < 2; i++) {
        adjust = (MIN_FIFO_POINTER_DIFF * FIFO_READ_COUNT - (int)difftotal[i]) / FIFO_READ_COUNT;

        unsigned mask;
        while (adjust) {
            if (adjust < 0) {
                /* Decrease pointer */
                adjust++;
                mask = (lane >= HOST_LANES) ? B0_DEC_MASK : A0_DEC_MASK;
            } else {
                /* Increase pointer */
                adjust--;
                mask = (lane >= HOST_LANES) ? B0_INC_MASK : A0_INC_MASK;
            }
            mask >>= i;
            /* toggle the bit */
            ERR_PROPS(readTop(slice, addr, &value));
            ERR_PROPS(writeTop(slice, addr, value | mask));
            ERR_PROPS(writeTop(slice, addr, value));
        }
    }
    return CR_OK;
}

CredoError_t be_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode) {
    if (mode == CR_LB_TX_TO_RX) {
        LOGS_ERROR("BaldEagle does not support TX to RX loopback mode!");
        return CR_NOTIMPLEMENTED;
    }

    BeSlice_t* be_slice = (BeSlice_t*)slice;
    CredoLaneMode_t lane_mode = be_slice->lane_mode[lane];
    if (lane_mode == CR_LMODE_OFF) {
        LOGS_ERROR("Lane %d is not configured", lane);
        return CR_FAIL;
    }
    if (lane_mode == CR_LMODE_DISABLE) {
        LOGS_ERROR("Lane %d is disabled", lane);
        return CR_FAIL;
    }

    unsigned fw_mode = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_CONFIG_SEL, &fw_mode));
    if (fw_mode == MODE_LOOPBACK_NRZ || fw_mode == MODE_LOOPBACK_PAM4) {
        LOGS_WARN("Lane %2d is in phy loopback mode", lane);
        return CR_FAIL;
    }

    CredoLaneLoopbackMode_t cur_lb_mode = CR_LB_DISABLED;
    ERR_PROPS(hal_get_lane_loopback_mode(slice, lane, &cur_lb_mode));
    if (cur_lb_mode == mode) {
        LOGS_WARN("Lane %d skip same loopback mode setting", lane);
        return CR_OK;
    }

    /* Find out what port does this belong to, we need to disable top firmware */
    int port_start_lane = -1;
    int port_no_of_lane = 0;
    int port = -1;
    const CredoPortConfig_t* port_config = NULL;
    for (int i = 0; i < BE_MAX_PORT; i++) {
        BePortInfo_t* port_info = be_slice->port_info + i;
        if (!port_info->configured) continue;
        port_config = &port_info->port_config;
        if (lane >= port_config->host_start_lane &&
            lane <= port_config->host_start_lane + port_config->host_no_of_lanes - 1) {
            port_start_lane = port_config->host_start_lane;
            port_no_of_lane = port_config->host_no_of_lanes;
            port = i;
            break;
        }
        if (lane >= port_config->line_start_lane &&
            lane <= port_config->line_start_lane + port_config->line_no_of_lanes - 1) {
            port_start_lane = port_config->line_start_lane;
            port_no_of_lane = port_config->line_no_of_lanes;
            port = i;
            break;
        }
    }
    /* sanity check... */
    if (port_no_of_lane) {
        if (port_start_lane + port_no_of_lane > slice->desc->lane_count) {
            LOGS_ERROR("Port %d is using %d lanes starting from %d -- internal error", port, port_no_of_lane,
                       port_start_lane);
            return CR_FAIL;
        }
    }

    unsigned top_fw_enable;
    ERR_PROPS(common_fw_reg_rd_internal(slice, FWREG_TOP_FW_ENABLE, &top_fw_enable));
    if (mode == CR_LB_RX_TO_TX) {
        /* Bookkeeping: loopback enable */
        be_slice->in_loopback[lane] = true;
        if (port_no_of_lane) {
            /* disable port top firmware */
            top_fw_enable &= ~(1 << port_start_lane);
            ERR_PROPS(common_fw_reg_wr_internal(slice, FWREG_TOP_FW_ENABLE, top_fw_enable));
        }
        /*
         * Do actual enable loopback:
         * 1. config pll source
         * 2. enable TRF control
         * 3. enable loopback mode
         * 4. reset and check FIFO
         * 5. Force data mode
         */
        ERR_PROPS(be_set_pll_source(slice, lane, PLL_LOOPBACK_MAGIC));
        ERR_PROPS(be_set_trf(slice, lane, TRF_OnetoOne));
        ERR_PROPS(be_set_serdes_loopback(slice, lane, 1));
        ERR_PROPS(be_adjust_lb_fifo(slice, lane));
        ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_DATA));
    } else {
        /* Do actual disable loopback */
        ERR_PROPS(be_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE));
        ERR_PROPS(be_set_serdes_loopback(slice, lane, 0));
        TRF_t trf = TRF_Off;
        if (port_no_of_lane) {
            switch (port_config->connection_mode) {
                case CR_PORT_RETIMER:
                case CR_PORT_BITMUX:
                case CR_PORT_SWITCHOVER_RETIMER:
                    trf = TRF_OnetoOne;
                    break;
                case CR_PORT_GEARBOX:
                    trf = TRF_GearBox;
                    break;
            }
        }
        ERR_PROPS(be_set_trf(slice, lane, trf));
        int lane_source = 0;
        lane_source = (port_start_lane == port_config->host_start_lane) ? port_config->line_start_lane
                                                                        : port_config->host_start_lane;
        ERR_PROPS(be_set_pll_source(slice, lane, lane_source & 7));
        /* Bookkeeping: loopback disable */
        be_slice->in_loopback[lane] = false;
        if (port_no_of_lane) {
            int have_loopback = false;
            for (int i = 0; i < port_no_of_lane; i++) {
                if (be_slice->in_loopback[port_start_lane + i]) {
                    have_loopback = true;
                    break;
                }
            }
            if (!have_loopback) {
                /* All loopbacks are gone, enable top firmware */
                top_fw_enable |= 1 << port_start_lane;
                ERR_PROPS(common_fw_reg_wr_internal(slice, FWREG_TOP_FW_ENABLE, top_fw_enable));
                /* And trigger a top level restart, on both side */
                ERR_PROPS(common_fw_cmd(slice, FW_CMD_FW_LANE_TOP_RESET + port_config->host_start_lane, 0, NULL, NULL));
                ERR_PROPS(common_fw_cmd(slice, FW_CMD_FW_LANE_TOP_RESET + port_config->line_start_lane, 0, NULL, NULL));
            }
        }
    }

    return CR_OK;
}

CredoError_t be_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode) {
    unsigned addr = REG_LOOPBACK_ENABLE + (lane % HOST_LANES) * 8;
    unsigned mask = (lane < HOST_LANES) ? 0x2 : 0x1;

    unsigned loopback_en = 0;
    ERR_PROPS(readTop(slice, addr, &loopback_en));
    if (loopback_en & mask) {
        *mode = CR_LB_RX_TO_TX;
    } else {
        *mode = CR_LB_DISABLED;
    }
    return CR_OK;
}

static CredoError_t be_sp_get_rx_skef_enable(CredoSlice_t* slice, int lane, int* enable) {
    int degen, addcap, gain;
    return hal_get_rx_skef(slice, lane, enable, &degen, &addcap, &gain);
}
static CredoError_t be_sp_set_rx_skef_enable(CredoSlice_t* slice, int lane, int enable) {
    int old_enable, degen, addcap, gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &old_enable, &degen, &addcap, &gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}
static CredoError_t be_sp_get_rx_skef_degen(CredoSlice_t* slice, int lane, int* degen) {
    int enable, addcap, gain;
    return hal_get_rx_skef(slice, lane, &enable, degen, &addcap, &gain);
}
static CredoError_t be_sp_set_rx_skef_degen(CredoSlice_t* slice, int lane, int degen) {
    int enable, old_degen, addcap, gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &enable, &old_degen, &addcap, &gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}
static CredoError_t be_sp_get_rx_skef_addcap(CredoSlice_t* slice, int lane, int* addcap) {
    int enable, degen, gain;
    return hal_get_rx_skef(slice, lane, &enable, &degen, addcap, &gain);
}
static CredoError_t be_sp_set_rx_skef_addcap(CredoSlice_t* slice, int lane, int addcap) {
    int enable, degen, old_addcap, gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &enable, &degen, &old_addcap, &gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}
static CredoError_t be_sp_get_rx_skef_gain(CredoSlice_t* slice, int lane, int* gain) {
    int enable, degen, addcap;
    return hal_get_rx_skef(slice, lane, &enable, &degen, &addcap, gain);
}
static CredoError_t be_sp_set_rx_skef_gain(CredoSlice_t* slice, int lane, int gain) {
    int enable, degen, addcap, old_gain;
    ERR_PROPS(hal_get_rx_skef(slice, lane, &enable, &degen, &addcap, &old_gain));
    return hal_set_rx_skef(slice, lane, enable, degen, addcap, gain);
}

static const int tx_taps_presets_pam4[][5] = {
    {1, -6, 15, 0, 0},   {1, -6, 15, 0, 0},   {2, -8, 21, 0, 0},   {2, -10, 25, 0, 0},  {2, -12, 24, 0, 0},
    {3, -12, 23, -2, 0}, {3, -14, 22, -2, 0}, {4, -15, 21, -2, 0}, {4, -16, 21, -1, 0},
};

static const int tx_taps_presets_nrz[][5] = {
    {0, -4, 26, 0, 0},  {0, -6, 26, 0, 0},  {0, -8, 26, 0, 0},   {0, -10, 26, 0, 0},
    {0, -12, 25, 0, 0}, {0, -14, 24, 0, 0}, {0, -15, 23, -2, 0},
};

static const int tx_taps_presets_optical[][5] = {
    {0, -6, 26, -5, 0},  {1, -7, 24, -5, 0}, {1, -8, 24, -6, 0}, {0, -2, 23, -2, 0},
    {0, -2, 21, -10, 0}, {0, -6, 21, -4, 0}, {0, -7, 21, -5, 0}, {0, -2, 23, -6, 0},
};

static CredoError_t be_serdes_preset_tx_taps_opt(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    const int* tx_taps = NULL;
    if (desc->opt_vswing > 7) {
        return CR_INVALID_ARGS;
    }
    tx_taps = tx_taps_presets_optical[desc->opt_vswing];
    return hal_set_tx_taps(slice, lane, tx_taps);
}

static CredoError_t be_serdes_preset_tx_taps_pam4(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    const int* tx_taps = NULL;
    if (desc->channel_loss <= 5) {
        tx_taps = tx_taps_presets_pam4[0];
    } else if (desc->channel_loss <= 7) {
        tx_taps = tx_taps_presets_pam4[1];
    } else if (desc->channel_loss <= 10) {
        tx_taps = tx_taps_presets_pam4[2];
    } else if (desc->channel_loss <= 15) {
        tx_taps = tx_taps_presets_pam4[3];
    } else if (desc->channel_loss <= 20) {
        tx_taps = tx_taps_presets_pam4[4];
    } else if (desc->channel_loss <= 25) {
        tx_taps = tx_taps_presets_pam4[5];
    } else if (desc->channel_loss <= 30) {
        tx_taps = tx_taps_presets_pam4[6];
    } else if (desc->channel_loss <= 33) {
        tx_taps = tx_taps_presets_pam4[7];
    } else if (desc->channel_loss <= 40) {
        tx_taps = tx_taps_presets_pam4[8];
    } else {
        LOGS_ERROR("channel loss > 40dB");
        return CR_INVALID_ARGS;
    }
    return hal_set_tx_taps(slice, lane, tx_taps);
}

static CredoError_t be_serdes_preset_tx_taps_nrz(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    const int* tx_taps = NULL;
    if (desc->channel_loss <= 10) {
        tx_taps = tx_taps_presets_nrz[0];
    } else if (desc->channel_loss <= 15) {
        tx_taps = tx_taps_presets_nrz[1];
    } else if (desc->channel_loss <= 20) {
        tx_taps = tx_taps_presets_nrz[2];
    } else if (desc->channel_loss <= 25) {
        tx_taps = tx_taps_presets_nrz[3];
    } else if (desc->channel_loss <= 30) {
        tx_taps = tx_taps_presets_nrz[4];
    } else if (desc->channel_loss <= 35) {
        tx_taps = tx_taps_presets_nrz[5];
    } else if (desc->channel_loss <= 40) {
        tx_taps = tx_taps_presets_nrz[6];
    } else {
        LOGS_ERROR("channel loss > 40dB");
        return CR_INVALID_ARGS;
    }
    return hal_set_tx_taps(slice, lane, tx_taps);
}

CredoError_t be_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    if (desc->optical) {
        return be_serdes_preset_tx_taps_opt(slice, lane, desc);
    }
    if (desc->mode == CR_LMODE_PAM4) {
        return be_serdes_preset_tx_taps_pam4(slice, lane, desc);
    } else if (desc->mode == CR_LMODE_NRZ) {
        return be_serdes_preset_tx_taps_nrz(slice, lane, desc);
    } else {
        return CR_INVALID_ARGS;
    }
}

// make sure to use SP_ name for the parameter, and add it if it doesnt exist. This helps to keep the names
// standardized across chips.
const ParamHandler_t param_serdes_list[] = {
    PARAM_LANE_INTLIST(SP_TX_TAPS, 5, hal_get_tx_taps, hal_set_tx_taps),
    PARAM_LANE_INT(SP_TX_POL, hal_get_tx_polarity, hal_set_tx_polarity),
    PARAM_LANE_INT(SP_RX_POL, hal_get_rx_polarity, hal_set_rx_polarity),
    PARAM_LANE_INT(SP_RX_INPUT_COUPLING, hal_get_rx_input_mode, hal_set_rx_input_mode),
    PARAM_LANE_INT(SP_TX_GRAYCODE, hal_get_tx_gray_code, hal_set_tx_gray_code),
    PARAM_LANE_INT(SP_RX_GRAYCODE, hal_get_rx_gray_code, hal_set_rx_gray_code),
    PARAM_LANE_INT(SP_TX_PRECODER, hal_get_tx_precoder, hal_set_tx_precoder),
    PARAM_LANE_INT(SP_RX_PRECODER, hal_get_rx_precoder, hal_set_rx_precoder),
    PARAM_LANE_INT(SP_TX_MSBLSB, hal_get_tx_msb, hal_set_tx_msb),
    PARAM_LANE_INT(SP_RX_MSBLSB, hal_get_rx_msb, hal_set_rx_msb),
    PARAM_LANE_INT(SP_TX_PLL_CAP, hal_get_tx_cap, hal_set_tx_cap),
    PARAM_LANE_INT(SP_RX_PLL_CAP, hal_get_rx_cap, hal_set_rx_cap),
    PARAM_LANE_INT(SP_RX_PPM, hal_get_rx_ppm, NULL),
    PARAM_LANE_INT(SP_RX_DAC, hal_get_rx_dac, hal_set_rx_dac),
    PARAM_LANE_INTLIST(SP_RX_FFE_TAPS, 7, hal_get_ffe_taps, hal_set_ffe_taps),
    PARAM_LANE_INT(SP_RX_F1OVER3, hal_get_f1over3, hal_set_f1over3),
    PARAM_LANE_INTLIST(SP_RX_AGCGAIN, 2, hal_get_agcgain, hal_set_agcgain),
    PARAM_LANE_INTLIST(SP_RX_CTLE, 2, hal_get_ctle, hal_set_ctle),
    PARAM_LANE_INT(SP_RX_DELTA, hal_get_delta_phase, hal_set_delta_phase),
    PARAM_LANE_INT(SP_RX_EDGE, hal_get_edge, hal_set_edge),
    PARAM_LANE_DBLLIST(SP_RX_DFE, 3, hal_get_dfe, NULL),
    PARAM_LANE_INTLIST(SP_RX_EYE_HEIGHT, 3, hal_fw_get_eye, NULL),
    PARAM_LANE_INT(SP_RX_READY, hal_get_lane_ready, NULL),
    PARAM_LANE_INT(SP_RX_SIGNAL_DETECT, hal_get_signal_detect, NULL),
    PARAM_LANE_INT(SP_RX_SKEF_EN, be_sp_get_rx_skef_enable, be_sp_set_rx_skef_enable),
    PARAM_LANE_INT(SP_RX_SKEF_DEGEN, be_sp_get_rx_skef_degen, be_sp_set_rx_skef_degen),
    PARAM_LANE_INT(SP_RX_SKEF_ADDCAP, be_sp_get_rx_skef_addcap, be_sp_set_rx_skef_addcap),
    PARAM_LANE_INT(SP_RX_SKEF_GAIN, be_sp_get_rx_skef_gain, be_sp_set_rx_skef_gain),
    PARAM_LANE_INT(SP_RX_ADAPT, hal_fw_get_adapt_count, NULL),
    PARAM_LANE_INT(SP_RX_READAPT, hal_fw_get_readapt_count, NULL),
    PARAM_LANE_INT(SP_RX_LINKLOST, hal_fw_get_link_lost_count, NULL),
    PARAM_LANE_DBL(SP_RX_CHANNEL_EST, hal_fw_get_channel_estimate, NULL),
    PARAM_LANE_INT(SP_RX_CHANNEL_OF, hal_fw_get_of, NULL),
    PARAM_LANE_INT(SP_RX_CHANNEL_HF, hal_fw_get_hf, NULL),
};
const int param_serdes_count = COUNT_OF(param_serdes_list);
