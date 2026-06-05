#include "bh_device.h"
#include "bh_functions.h"
#include "blackhawk.h"

#include "anlt/anlt.h"
#include "common/common_firmware.h"
#include "common/common_misc.h"
#include "common/params.h"
#include "condor_lp/condor_lp_serdes.h"
#include "fec_analyzer/fec_analyzer.h"
#include "rsfec/rsfec.h"

#include "utility.h"
#include "sdk.h"

#include <inttypes.h>
#include <string.h>

CredoError_t bh_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane) {
    *host_lane = HOST_LANES;
    *line_lane = LINE_LANES;
    return CR_OK;
}

unsigned bh_is_valid_lane(CredoSlice_t* slice, int lane) {
    return lane >= 0 && lane < slice->desc->lane_count;
}

CredoError_t bh_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    *mode = bh_slice->lane_mode[lane];
    return CR_OK;
}

CredoError_t bh_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;

    ERR_PROPS(bh_set_tx_taps_internal(slice, lane));

    bh_slice->lane_mode[lane] = mode;
    return CR_OK;
}

CredoError_t bh_get_tx_precoder(CredoSlice_t* slice, int lane, int* pc) {
    *pc = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_TX_PRECODER, (unsigned int*)pc);

    if (ret != CR_OK) {
        // It means firmware don't support tx precoder R/W, read from register
        ret = condor_lp_get_tx_precoder(slice, lane, pc);
    } else {
        // firmware support tx precoder R/W but the real user programmed value is saved in internal reg
        unsigned value = 0;
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_TX_PRECODER, &value));
        *pc = (value >> lane) & 0x1;
    }

    return ret;
}

CredoError_t bh_set_tx_precoder(CredoSlice_t* slice, int lane, int pc) {
    unsigned val = FW_CMD_LOG_SILENT;
    CredoError_t ret = hal_fw_cmd_ex(slice, FW_CMD_TX_PRECODER_CTRL, pc ? 1 : 0, lane, NULL, &val, NULL);
    if (ret != CR_OK) {
        ret = condor_lp_set_tx_precoder(slice, lane, pc);
    }

    return ret;
}

CredoError_t bh_update_lane_mode_ex(CredoSlice_t* slice, int lane, bool warm) {
    CredoLaneMode_t lane_mode_pre = CR_LMODE_OFF, lane_mode_post = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode_pre));

    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));

    if (fw_status == 1) {
        unsigned opt_mode;
        ERR_PROPS(bh_fw_get_opt_mode(slice, lane, &opt_mode));
        switch (opt_mode) {
            case CR_LMODE_PAM4:
            case CR_LMODE_NRZ:
            case CR_LMODE_OFF:
            case CR_LMODE_DISABLE:
                lane_mode_post = opt_mode;
                break;
            default:
                lane_mode_post = CR_LMODE_OFF;  // force to be off for unknown mode
                break;
        }
    } else {
        unsigned pam4_en;
        ERR_PROPS(readRegLane(slice, lane, REG_RX_PAM4_EN, &pam4_en));
        lane_mode_post = (pam4_en == 1) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
    }
    // prevent setting tx taps on a warmboot
    if (warm) {
        BhSlice_t* bh_slice = (BhSlice_t*)slice;
        bh_slice->lane_mode[lane] = lane_mode_post;
    } else if (lane_mode_pre != lane_mode_post) {
        ERR_PROPS(hal_set_lane_mode(slice, lane, lane_mode_post));
    }
    return CR_OK;
}

CredoError_t bh_update_lane_mode(CredoSlice_t* slice, int lane) {
    return bh_update_lane_mode_ex(slice, lane, false);
}

CredoError_t bh_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));

    if (fw_status == 1) {
        ERR_PROPS(bh_fw_get_lane_speed(slice, lane, speed_kbps));
    } else {
        *speed_kbps = 0;
    }
    return CR_OK;
}

CredoError_t bh_logic_reset_lane(CredoSlice_t* slice, int lane) {
    unsigned val;
    ERR_PROPS(readTop(slice, REG_LANE_LOGIC_RST, &val));

    val |= (1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_LOGIC_RST, val));

    val &= ~(1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_LOGIC_RST, val));
    return CR_OK;
}

CredoError_t bh_reg_reset_lane(CredoSlice_t* slice, int lane) {
    unsigned val;
    ERR_PROPS(readTop(slice, REG_LANE_REGISTER_RST, &val));

    val |= (1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_REGISTER_RST, val));

    val &= ~(1 << lane);
    ERR_PROPS(writeTop(slice, REG_LANE_REGISTER_RST, val));
    return CR_OK;
}

CredoError_t bh_fec_analyzer_get_config(CredoSlice_t* slice, int lane, int* enable, CredoFecAnalyzerConfig_t* config) {
    if (enable) {
        CredoLaneMode_t lmode;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lmode));
        if (lmode != CR_LMODE_PAM4) {
            *enable = false;
            return CR_OK;
        }
    }

    return fec_analyzer_get_config(slice, lane, enable, config);
}

CredoError_t bh_fec_analyzer_set_config(CredoSlice_t* slice, int lane, int enable, CredoFecAnalyzerConfig_t* config) {
    if (enable) {
        CredoLaneMode_t lmode;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &lmode));

        if (lmode != CR_LMODE_PAM4) {
            LOGS_WARN("FEC Analyzer only supported in PAM4, doing nothing");
            return CR_OK;
        }
    }

    ERR_PROPS(bh_top_set_fec_analyzer(slice, lane, enable));
    ERR_PROPS(fec_analyzer_set_config(slice, lane, enable, config));
    return CR_OK;
}

CredoError_t bh_top_set_fec_analyzer(CredoSlice_t* slice, int lane, int enable) {
    if (lane < 0 || lane >= slice->desc->lane_count) {
        LOGS_ERROR("Lane %d out of range", lane);
        return CR_FAIL;
    }

    if (lane < HOST_LANES) {
        ERR_PROPS(writeRegLane(slice, lane, REG_FEC_ANA_EN_A, enable));
    } else {
        ERR_PROPS(writeRegLane(slice, lane % 8, REG_FEC_ANA_EN_B, enable));
    }
    return CR_OK;
}

CredoError_t bh_set_tx_taps_internal(CredoSlice_t* slice, int lane) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;

    unsigned fw_speed = SPEED_OFF;
    ERR_PROP(common_fw_get_speed_index(slice, lane, &fw_speed));
    if (fw_speed == SPEED_OFF) return CR_OK;

    unsigned lt_on = 0;
    ERR_PROP(bh_fw_get_anlt(slice, lane, NULL, &lt_on));
    if (lt_on) return CR_OK;

    int tx_taps[7] = {0};
    if (fw_speed == SPEED_10G) {
        LOGS_INFO("[Tx taps][%02d] 10g tx taps have been zero padded per tap", lane);
        // [0, 0, -12, 100, -8, 0 0] to [0, -12, 0 ,100, 0, -8, 0]
        tx_taps[1] = bh_slice->tx_taps[lane][2];
        tx_taps[3] = bh_slice->tx_taps[lane][3];
        tx_taps[5] = bh_slice->tx_taps[lane][4];
    } else {
        memcpy(tx_taps, bh_slice->tx_taps[lane], sizeof(int) * 7);
    }

    return condor_lp_set_tx_taps(slice, lane, tx_taps);
}

CredoError_t bh_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    BhSlice_t* bh_slice = (BhSlice_t*)slice;
    memcpy(bh_slice->tx_taps[lane], taps, sizeof(int) * 7);

    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status == 0) {
        return condor_lp_set_tx_taps(slice, lane, taps);
    }

    unsigned fw_speed = SPEED_OFF;
    ERR_PROP(common_fw_get_speed_index(slice, lane, &fw_speed));
    if (fw_speed == SPEED_OFF) {
        return condor_lp_set_tx_taps(slice, lane, taps);
    }

    unsigned lt_on = 0;
    ERR_PROP(bh_fw_get_anlt(slice, lane, NULL, &lt_on));
    if (lt_on) {
        LOGS_WARN("[Set Tx taps][%02d] link training enabled, skip setting, only save values", lane);
        return CR_OK;
    }

    return bh_set_tx_taps_internal(slice, lane);
}

CredoError_t bh_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    ERR_PROPS(condor_lp_get_tx_taps(slice, lane, taps));

    CredoLaneMode_t lane_mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_OFF) return CR_OK;

    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status == 0) return CR_OK;

    unsigned fw_speed = SPEED_OFF;
    ERR_PROP(common_fw_get_speed_index(slice, lane, &fw_speed));
    if (fw_speed != SPEED_10G) return CR_OK;

    taps[2] = taps[1];
    taps[4] = taps[5];
    taps[1] = taps[5] = 0;

    return CR_OK;
}

CredoError_t bh_tx_disable(CredoSlice_t* slice, int lane) {
    return bh_fw_tx_control(slice, lane, FW_TX_SOURCE_QUIET);
}

CredoError_t bh_tx_no_disable(CredoSlice_t* slice, int lane) {
    return bh_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
}

CredoError_t bh_rx_disable(CredoSlice_t* slice, int lane) {
    return bh_fw_rx_control(slice, lane, FW_RX_CONTROL_DISABLE);
}

CredoError_t bh_rx_no_disable(CredoSlice_t* slice, int lane) {
    return bh_fw_rx_control(slice, lane, FW_RX_CONTROL_ENABLE);
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

CredoError_t bh_lane_tx_status(CredoSlice_t* slice, int lane, CredoLaneTxState_t* status) {
    CredoLaneMode_t lane_mode;

    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (!IS_LANE_MODE_PAM4_OR_NRZ(lane_mode)) {
        *status = CR_TX_LOWPOWER;
        return CR_OK;
    }

    bool tx_pattern_en = false;
    hal_get_tx_test_pattern_enable(slice, lane, &tx_pattern_en);
    if (tx_pattern_en) {
        *status = CR_TX_FORCE_TEST_PATT;
        return CR_OK;
    }

    unsigned tx_status;
    ERR_PROPS(common_fw_debug_cmd(slice, lane, TOP_INFO, TOP_INFO_TX_STATUS, &tx_status));

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

CredoError_t bh_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned status;

    ERR_PROP(bh_fw_get_status(slice, &status));
    if (status == 1) {
        unsigned patt;
        CredoLaneMode_t mode;
        if (!enable) {
            return bh_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
        }
        ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
        patt = credo_to_condor_lp(prbs_mode, mode);
        if (patt == CONDOR_LP_PRBS_INVALID) {
            LOGS_ERROR("[Set TX PRBS] Unsupported PRBS mode %d", prbs_mode);
            return CR_FAIL;
        }
        if (mode == CR_LMODE_PAM4) {
            ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_PAM4));
        } else {
            if (mode != CR_LMODE_NRZ) {
                LOGS_WARN("[Set TX PRBS] Unknown lane mode %d, treat as NRZ", mode);
            }
            ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_NRZ));
        }
        return writeRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, patt);
    } else {
        LOGS_WARN("Firmware is not running!");
        return condor_lp_set_tx_prbs(slice, lane, enable, prbs_mode);
    }
}

CredoError_t bh_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (enable && mode == CR_LMODE_PAM4) {
        writeRegLane(slice, lane, REG_RX_PRBS_MISMATCH_THRES, 10);
        writeRegLane(slice, lane, REG_RX_PRBS_SYNC_LOSS_THRES, 5);
    }

    ERR_PROPS(condor_lp_set_rx_prbs(slice, lane, enable, prbs_mode));

    return CR_OK;
}

CredoError_t bh_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    unsigned status;

    bh_fw_get_status(slice, &status);
    if (status == 1) {
        CredoLaneMode_t mode;
        if (enable) {
            ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
            if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ) {
                mode = CR_LMODE_NRZ;
                LOGS_WARN("[Set TX PRBS] Unknown lane mode %d, treat as NRZ", mode);
            }
            if (mode == CR_LMODE_PAM4) {
                ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_PAM4));
            } else {
                ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_NRZ));
            }
        } else {
            ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE));
        }

    } else {
        LOGS_WARN("Firmware is not running!");
        return condor_lp_set_tx_prbs(slice, lane, enable, 0);  // don't care prbs_mode
    }

    return condor_lp_set_tx_test_pattern_enable(slice, lane, enable);
}

static CredoError_t bh_read_fifo_diff(CredoSlice_t* slice, int lane, unsigned diff[2]) {
    unsigned addr = REG_ADR_DIFF_LB + (lane % HOST_LANES) * 8;
    unsigned value;

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
    return CR_OK;
}

#define FIFO_READ_COUNT       16
#define MIN_FIFO_POINTER_DIFF 5

bool bh_is_lb_fifo_stable(CredoSlice_t* slice, int lane, unsigned* fifo_pos) {
    uint8_t pos[2] = {0};
    for (int i = 0; i < FIFO_READ_COUNT; i++) {
        unsigned diff[2];
        if (bh_read_fifo_diff(slice, lane, diff) != CR_OK) return false;
        pos[0] |= 1 << diff[0];
        pos[1] |= 1 << diff[1];
    }

    if (fifo_pos) *fifo_pos = ((unsigned)pos[1]) << 8 | pos[0];

    LOGS_TRACE("fifo position 0x%X, 0x%X", pos[0], pos[1]);

    for (int i = 0; i < 2; i++) {
        if (pos[i] == 0x81) {
            /* 0 and 7 */
            continue;
        }

        uint8_t pos1 = pos[i] & (pos[i] - 1);
        pos[i] -= pos1;
        if (pos[i] * 2 == pos1 || pos1 == 0) {
            continue;
        }

        return false;
    }

    return true;
}

#define TIMEOUT_LB_FIFO_STABLE (100 * 1000)
static CredoError_t bh_wait_lb_fifo_stable(CredoSlice_t* slice, int lane) {
    CredoTime_t start_time;
    unsigned fifo_pos = 0;
    unsigned count = 1;

    get_time(&start_time);
    do {
        if (bh_is_lb_fifo_stable(slice, lane, &fifo_pos)) {
            LOGS_DEBUG("Lane %2d loopback fifo stable, position 0x%04X, pass %" PRIu64 " usec, count %d", lane,
                       fifo_pos, us_passed(&start_time), count);
            return CR_OK;
        }
        if (count % 2 == 0) {
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PLL_PU_INTP, 0));
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PLL_PU_INTP, 1));
        }
        count++;
    } while (!is_timeout(&start_time, TIMEOUT_LB_FIFO_STABLE) || count < 3);
    LOGS_ERROR("Lane %2d loopback fifo unstable, position 0x%04X", lane, fifo_pos);
    return CR_FAIL;
}

static CredoError_t bh_adjust_lb_fifo(CredoSlice_t* slice, int lane) {
    unsigned addr = REG_W_ADJUST + (lane % HOST_LANES) * 8;

    ERR_PROPS(bh_wait_lb_fifo_stable(slice, lane));

    /* Adjust */
    for (int retry = 0; retry < 2; retry++) {
        unsigned diff[2];
        ERR_PROPS(bh_read_fifo_diff(slice, lane, diff));
        for (int i = 0; i < 2; i++) {
            int adjust = MIN_FIFO_POINTER_DIFF - diff[i];

            unsigned mask;
            unsigned value;
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
                ERR_PROPS(writeTop(slice, addr, (value & ~mask) & 0xFFFF));
            }
        }
    }

    /* Check FIFO position */
    const unsigned expect_fifo_pos = 1 << MIN_FIFO_POINTER_DIFF | 1 << (MIN_FIFO_POINTER_DIFF - 1) |
                                     1 << (MIN_FIFO_POINTER_DIFF + 1) | 1 << (MIN_FIFO_POINTER_DIFF + 8) |
                                     1 << (MIN_FIFO_POINTER_DIFF + 8 - 1) | 1 << (MIN_FIFO_POINTER_DIFF + 8 + 1);
    unsigned fifo_pos = 0;
    if (bh_is_lb_fifo_stable(slice, lane, &fifo_pos) == false) {
        LOGS_ERROR("Lane %2d loopback fifo unstable, position 0x%04X", lane, fifo_pos);
        return CR_FAIL;
    } else if ((fifo_pos & expect_fifo_pos) != fifo_pos) {
        LOGS_WARN("Lane %2d fifo position 0x%04X not in 0x%04X, please set r2t loopback again", lane, fifo_pos,
                  expect_fifo_pos);
    }
    return CR_OK;
}

static CredoError_t bh_set_lane_loopback_mode_inner(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode) {
    if (mode == CR_LB_RX_TO_TX) {
        /*
         * Enable loopback :
         * 1. config pll source
         * 2. enable TRF control
         * 3. enable loopback mode
         * 4. reset and check FIFO
         * 5. Force data mode
         */
        ERR_PROPS(bh_fw_PLL_source_control(slice, lane, PLL_LOOPBACK_MAGIC));
        ERR_PROPS(bh_fw_TRF_control(slice, lane, TRF_OnetoOne));

        ERR_PROPS(bh_fw_force_loopback(slice, lane, 1));

        // ERR_PROPS( writeRegLane(slice, lane, REG_TX_FIFO_RESET, 1));
        // ERR_PROPS( writeRegLane(slice, lane, REG_TX_FIFO_RESET, 0));
        ERR_PROPS(bh_adjust_lb_fifo(slice, lane));

        ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_DATA));
    } else {
        /*
         * Disable loopback :
         * 1. Release TX force
         * 2. Release loopback
         * 3. Release TRF
         * 4. Release PLL source
         * 5. Reset FIFO
         */
        ERR_PROPS(bh_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE));
        ERR_PROPS(bh_fw_TRF_control(slice, lane, TRF_NOFORCE));
        ERR_PROPS(bh_fw_PLL_source_control(slice, lane, -1));
        ERR_PROPS(bh_fw_force_loopback(slice, lane, 0));

        // ERR_PROPS( writeRegLane(slice, lane, REG_TX_FIFO_RESET, 1));
        // ERR_PROPS( writeRegLane(slice, lane, REG_TX_FIFO_RESET, 0));
    }

    return CR_OK;
}

CredoError_t bh_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode) {
    if (mode == CR_LB_TX_TO_RX) {
        LOGS_ERROR("Unsupport Tx to Rx loopback mode!");
        return CR_UNSUPPORTED;
    }

    CredoLaneMode_t lane_mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_OFF) {
        LOGS_WARN("Lane %2d is not configured", lane);
        return CR_FAIL;
    }
    if (lane_mode == CR_LMODE_DISABLE) {
        LOGS_WARN("Lane %2d is disabled", lane);
        return CR_FAIL;
    }

    unsigned fw_mode = 0;
    ERR_PROPS(common_fw_debug_cmd(slice, lane, TOP_DEBUG, TOP_DEBUG_CONFIG_SEL, &fw_mode));
    if (fw_mode == MODE_LOOPBACK) {
        LOGS_WARN("Lane %2d is in phy loopback mode", lane);
        return CR_FAIL;
    }

    CredoLaneLoopbackMode_t cur_lb_mode = CR_LB_DISABLED;
    ERR_PROPS(hal_get_lane_loopback_mode(slice, lane, &cur_lb_mode));
    if (cur_lb_mode == mode) {
        if (mode != CR_LB_DISABLED) LOGS_WARN("Lane %d skip same loopback mode setting", lane);
        return CR_OK;
    }

    if (bh_set_lane_loopback_mode_inner(slice, lane, mode) != CR_OK) {
        ERR_PROPS(bh_set_lane_loopback_mode_inner(slice, lane, CR_LB_DISABLED));
        return CR_FAIL;
    }

    return CR_OK;
}

CredoError_t bh_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode) {
    unsigned addr = REG_LOOPBACK_EN + (lane % HOST_LANES) * 8;
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

CredoError_t bh_set_autoneg_pages(CredoSlice_t* slice, int lane, int page_id, uint64_t page) {
    if (lane < HOST_LANES) return CR_UNSUPPORTED;
    return common_set_autoneg_pages(slice, lane - HOST_LANES, page_id, page);
}

CredoError_t bh_get_autoneg_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                            uint64_t transmitted_pages[9], uint64_t received_pages[9]) {
    if (lane < HOST_LANES) {
        LOGS_ERROR("[Get AN Pages] host lane don't support");
        return CR_UNSUPPORTED;
    }
    unsigned fw_status = 0;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));

    unsigned anlt_pwr_saving_en = 0;
    if (fw_status == 1) {
        ERR_PROPS(hal_fw_reg_rd_internal(slice, FWREG_TOP_OPTIONS, &anlt_pwr_saving_en));
        anlt_pwr_saving_en = (anlt_pwr_saving_en >> 6) & 0x1;
    }

    if (fw_status == 1 && anlt_pwr_saving_en == 1) {
        ERR_PROP(bh_fw_get_an_pages(slice, lane, transmitted_pages, received_pages));
    } else {
        ERR_PROP(common_get_autoneg_exchanged_pages(slice, lane - HOST_LANES, page_count, transmitted_pages,
                                                    received_pages));
    }

    return CR_OK;
}

CredoError_t bh_get_top_pll_cap(CredoSlice_t* slice, unsigned* caps) {
    return readReg(slice, REG_TOP_PLL_LCVCOCAP, caps);
}

CredoError_t bh_get_rx_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    ERR_PROPS(condor_lp_get_rx_ppm(slice, lane, ppm));
    unsigned speed_index, fw_status;
    ERR_PROPS(bh_fw_get_status(slice, &fw_status));
    if (fw_status != 1) {
        LOGS_WARN("ppm not calculated as firmware is not running, providing freq_acc");
        return CR_OK;
    }
    ERR_PROPS(common_fw_get_speed_index(slice, lane, &speed_index));

    // need to do some conversion from freq_acc to ppm
    // refer to: https://gitlab.idm.credosemi.com/apps-team/sdk/credo-sdk/-/issues/198
    switch (speed_index) {
        case SPEED_1G:
        case SPEED_20G:
        case SPEED_25G:
        case SPEED_26G:
        case SPEED_27_9G:
        case SPEED_28G:
            *ppm = *ppm * 4;
            break;
        case SPEED_10G:
            *ppm = *ppm * 2;
            break;
        default:
            // ppm is just 1:1
            break;
    }

    return CR_OK;
}

CredoError_t bh_get_rx_ths(CredoSlice_t* slice, int lane, int ths[]) {
    unsigned fw_status = 0;
    ERR_PROPS(bh_fw_get_status(slice, &fw_status));

    if (fw_status == 1) {
        ERR_PROPS(common_fw_get_ths(slice, lane, ths));
    } else {
        ERR_PROPS(condor_lp_get_rx_ths(slice, lane, ths));
    }

    return CR_OK;
}

CredoError_t bh_rsfec_get_total_codewords(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                          uint64_t* total_bits) {
    CredoError_t ret = CR_FAIL;
    CredoPortConfig_t port_config;
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    if (port_config.port_id == CR_PORT_UNCONFIGURED || port_config.connection_mode != CR_PORT_GEARBOX)
        return CR_INVALID_ARGS;

    ret = bh_fw_get_rsfec_total_codewords(slice, port_id, side, total_bits);
    if (ret != CR_OK) {
        ret = rsfec_get_total_codewords(slice, port_id, side, total_bits);
    }
    return ret;
}

CredoError_t bh_rsfec_get_uncorrected_codeword(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side,
                                               unsigned* uncorr_cw) {
    CredoError_t ret = CR_FAIL;
    CredoPortConfig_t port_config;
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    if (port_config.port_id == CR_PORT_UNCONFIGURED || port_config.connection_mode != CR_PORT_GEARBOX)
        return CR_INVALID_ARGS;

    ret = bh_fw_get_rsfec_uncorrected_codeword(slice, port_id, side, uncorr_cw);
    if (ret != CR_OK) {
        ret = rsfec_get_uncorrected_codeword(slice, port_id, side, uncorr_cw);
    }
    return ret;
}

CredoError_t bh_rsfec_set_count_freeze(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, bool enable) {
    CredoPortConfig_t port_config = {0};
    ERR_PROPS(bh_port_query(slice, port_id, &port_config));

    if (port_config.port_id == CR_PORT_UNCONFIGURED) return CR_INVALID_ARGS;
    int lane = (side == CR_SIDE_HOST) ? port_config.host_start_lane : port_config.line_start_lane;

    unsigned fw_feature = 0;
    bh_fw_get_feature(slice, &fw_feature);

    if (fw_feature & FW_FEATURE_GEARBOX_FEC_CNT_FREEZE) {
        ERR_PROPS(bh_fw_set_rsfec_count_freeze(slice, lane, enable));
    } else {
        if (enable) {
            ERR_PROPS(bh_fw_set_top_option(slice, TOP_OPTION_BG_AUTO_RECOVER, 0));
            ERR_PROPS(bh_fw_set_top_sm_enalbe(slice, lane, 0));
            ERR_PROPS(rsfec_set_count_freeze(slice, port_id, side, enable));
        } else {
            ERR_PROPS(rsfec_set_count_freeze(slice, port_id, side, enable));
            ERR_PROPS(bh_fw_set_top_sm_enalbe(slice, lane, 1));
            ERR_PROPS(bh_fw_set_top_option(slice, TOP_OPTION_BG_AUTO_RECOVER, 1));
        }
    }
    return CR_OK;
}

CredoError_t bh_rsfec_get_fifo(CredoSlice_t* slice, uint32_t port_id, CredoSide_t side, CredoRSFECFifo_t* rsfec_fifo) {
    unsigned ver = 0;
    ERR_PROPS(bh_fw_get_func_ver(slice, &ver));

    if (ver == 0) {
        ERR_PROPS(rsfec_get_fifo(slice, port_id, side, rsfec_fifo));
    } else {
        ERR_PROPS(bh_fw_get_rsfec_fifo(slice, port_id, side, rsfec_fifo));
    }
    return CR_OK;
}

static const int tx_taps_presets_pam4[][7] = {
    {0, 2, -12, 68, 0, 0, 0},  {0, 4, -16, 84, 0, 0, 0},  {0, 4, -20, 100, 0, 0, 0}, {0, 4, -24, 96, 0, 0, 0},
    {0, 6, -24, 92, -4, 0, 0}, {0, 6, -28, 88, -4, 0, 0}, {0, 8, -30, 84, -4, 0, 0}, {0, 8, -32, 84, -2, 0, 0},
};

static const int tx_taps_presets_nrz[][7] = {
    {0, 0, -8, 104, 0, 0, 0},  {0, 0, -12, 104, 0, 0, 0}, {0, 0, -16, 104, 0, 0, 0}, {0, 0, -20, 104, 0, 0, 0},
    {0, 0, -24, 100, 0, 0, 0}, {0, 0, -28, 96, 0, 0, 0},  {0, 0, -30, 92, -4, 0, 0},
};

static const int tx_taps_presets_optical[][7] = {
    {0, 0, -12, 102, -10, 0, 0}, {0, 2, -14, 98, -10, 0, 0}, {0, 2, -16, 96, -12, 0, 0}, {0, 0, -4, 93, -4, 0, 0},
    {0, 0, -8, 84, -6, 0, 0},    {0, 0, -12, 84, -8, 0, 0},  {0, 0, -14, 84, -10, 0, 0}, {0, 2, -16, 82, -10, 0, 0},
};

static CredoError_t bh_serdes_preset_tx_taps_opt(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    const int* tx_taps = NULL;
    if (desc->opt_vswing > 7) {
        return CR_INVALID_ARGS;
    }
    tx_taps = tx_taps_presets_optical[desc->opt_vswing];
    return hal_set_tx_taps(slice, lane, tx_taps);
}

static CredoError_t bh_serdes_preset_tx_taps_pam4(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    const int* tx_taps = NULL;
    if (desc->channel_loss <= 5) {
        tx_taps = tx_taps_presets_pam4[0];
    } else if (desc->channel_loss <= 10) {
        tx_taps = tx_taps_presets_pam4[1];
    } else if (desc->channel_loss <= 15) {
        tx_taps = tx_taps_presets_pam4[2];
    } else if (desc->channel_loss <= 20) {
        tx_taps = tx_taps_presets_pam4[3];
    } else if (desc->channel_loss <= 25) {
        tx_taps = tx_taps_presets_pam4[4];
    } else if (desc->channel_loss <= 30) {
        tx_taps = tx_taps_presets_pam4[5];
    } else if (desc->channel_loss <= 33) {
        tx_taps = tx_taps_presets_pam4[6];
    } else if (desc->channel_loss <= 40) {
        tx_taps = tx_taps_presets_pam4[7];
    } else {
        LOGS_ERROR("channel loss > 40dB");
        return CR_INVALID_ARGS;
    }
    return hal_set_tx_taps(slice, lane, tx_taps);
}

static CredoError_t bh_serdes_preset_tx_taps_nrz(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
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

CredoError_t bh_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    if (desc->optical) {
        return bh_serdes_preset_tx_taps_opt(slice, lane, desc);
    }
    if (desc->mode == CR_LMODE_PAM4) {
        return bh_serdes_preset_tx_taps_pam4(slice, lane, desc);
    } else if (desc->mode == CR_LMODE_NRZ) {
        return bh_serdes_preset_tx_taps_nrz(slice, lane, desc);
    } else {
        return CR_INVALID_ARGS;
    }
}
