#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include "common/common_firmware.h"
#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <string.h>

CredoError_t se_get_lane_count(CredoSlice_t* slice, int* host_lane, int* line_lane) {
    if (host_lane) *host_lane = HOST_LANES;
    if (line_lane) *line_lane = LINE_LANES;
    return CR_OK;
}

CredoError_t se_get_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    *mode = se_slice->lane_mode[lane];
    return CR_OK;
}
CredoError_t se_get_lane_tx_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t* mode) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    if (se_slice->uni_tx_port_map[lane] != UNI_PORT_NONE) {
        *mode = se_slice->tx_lane_mode[lane];
        return CR_OK;
    } else if (se_slice->uni_rx_port_map[lane] != UNI_PORT_NONE) {
        *mode = CR_LMODE_OFF;
        return CR_OK;
    }
    return se_get_lane_mode(slice, lane, mode);
}

CredoError_t se_set_lane_mode(CredoSlice_t* slice, int lane, CredoLaneMode_t mode) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    se_slice->lane_mode[lane] = mode;

    if (mode == CR_LMODE_OFF || mode == CR_LMODE_DISABLE) {
        // go through and destroy the unidrectional info
        if (se_slice->uni_rx_port_map[lane] != UNI_PORT_NONE) {
            int tx_lane = se_slice->uni_rx_lane_map[lane];
            se_slice->uni_tx_port_map[tx_lane] = UNI_PORT_NONE;
            se_slice->uni_tx_lane_map[tx_lane] = 0;
            se_slice->uni_rx_port_map[lane] = UNI_PORT_NONE;
            se_slice->uni_rx_lane_map[lane] = 0;
            se_slice->tx_lane_mode[tx_lane] = mode;
        }
        return CR_OK;
    }

    se_slice->tx_lane_mode[lane] = mode;

    ERR_PROPS(se_set_tx_taps_internal(slice, lane, false));

    return CR_OK;
}

/*
 * It's a workaround for uni retimer tx taps issue, almost same as se_set_tx_taps_internal function
 * use source lane rate mode to setup destionation lane tx taps
 * NOTE: this function should used in uni-restimer function only, skip parameter sanity check here
 *
 */
CredoError_t se_set_lane_mode_uni_retimer(CredoSlice_t* slice, int src_lane, int dst_lane, CredoLaneMode_t mode) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    se_slice->lane_mode[src_lane] = mode;
    se_slice->tx_lane_mode[dst_lane] = mode;
    se_slice->uni_tx_lane_map[dst_lane] = src_lane;
    se_slice->uni_tx_port_map[dst_lane] = UNI_PORT_REIMER;
    se_slice->uni_rx_port_map[src_lane] = UNI_PORT_REIMER;
    se_slice->uni_rx_lane_map[src_lane] = dst_lane;
    ERR_PROPS(se_set_tx_taps_internal(slice, dst_lane, false));
    return CR_OK;
}

CredoError_t se_update_lane_mode(CredoSlice_t* slice, int lane) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    unsigned fw_status = 0;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status == 1) {
        unsigned opt_mode;
        ERR_PROPS(se_fw_get_opt_mode(slice, lane, &opt_mode));
        switch (opt_mode) {
            case CR_LMODE_PAM4:
            case CR_LMODE_NRZ:
            case CR_LMODE_OFF:
            case CR_LMODE_DISABLE:
                break;
            default:
                opt_mode = CR_LMODE_OFF;  // force to be off for unknown mode
        }

        se_slice->lane_mode[lane] = opt_mode;
    } else {
        unsigned nrz_en = 0;
        ERR_PROPS(readRegLane(slice, lane, REG_RX_DFE_NRZ_MODE, &nrz_en));
        se_slice->lane_mode[lane] = (nrz_en == 0) ? CR_LMODE_PAM4 : CR_LMODE_NRZ;
    }

    return CR_OK;
}

CredoError_t se_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    unsigned fw_status = 0;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status == 1) {
        ERR_PROP(se_fw_get_lane_speed(slice, lane, speed_kbps));
    } else {
        ERR_PROP(swift_get_lane_speed(slice, lane, speed_kbps));
    }
    return CR_OK;
}

CredoError_t se_get_lane_tx_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    if (se_slice->uni_tx_port_map[lane] != UNI_PORT_NONE) {
        int source_lane = se_slice->uni_tx_lane_map[lane];
        ERR_PROPS(se_get_lane_speed(slice, source_lane, speed_kbps));
        // TODO for uni-bitmux, we can do some conversion
        return CR_OK;
    } else if (se_slice->uni_rx_port_map[lane] != UNI_PORT_NONE) {
        *speed_kbps = 0;
        return CR_OK;
    }
    return se_get_lane_speed(slice, lane, speed_kbps);
}

CredoError_t se_get_top_pll_cap(CredoSlice_t* slice, unsigned* caps) {
    return readReg(slice, REG_TOP_PLL_LCVCOCAP, caps);
}

CredoError_t se_logic_reset_lane(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_LOGIC_RESET, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_LOGIC_RESET, 0));
    return CR_OK;
}

CredoError_t se_reg_reset_lane(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_REGISTER_RESET, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_REGISTER_RESET, 0));
    return CR_OK;
}

// allow users to override tx taps
CredoError_t se_set_tx_taps_internal(CredoSlice_t* slice, int lane, bool force) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;

    CredoLaneMode_t lane_mode = CR_LMODE_OFF;
    ERR_PROPS(hal_lane_get_tx_mode(slice, lane, &lane_mode));

    if (!force) {
        unsigned lt_on = 0;
        ERR_PROP(se_fw_get_anlt(slice, lane, NULL, &lt_on));
        if (lt_on) {
            LOGS_DEBUG("[Set TX TAPS][%d] LT is running. Skip it.", lane);
            return CR_OK;
        }
    }

    unsigned fw_rate = FW_RATE_FULL;
    ERR_PROPS(se_fw_get_tx_rate_mode(slice, lane, &fw_rate));

    int tx_taps[7] = {0};
    if (fw_rate != FW_RATE_FULL && lane_mode != CR_LMODE_OFF) {
        if (fw_rate == FW_RATE_HALF) {
            tx_taps[0] = se_slice->tx_taps[lane][1];
            tx_taps[2] = se_slice->tx_taps[lane][2];
            tx_taps[4] = se_slice->tx_taps[lane][3];
            tx_taps[6] = se_slice->tx_taps[lane][4];
        } else if (fw_rate == FW_RATE_QUARTER) {
            tx_taps[0] = se_slice->tx_taps[lane][2];
            tx_taps[4] = se_slice->tx_taps[lane][3];
        } else {
            LOGS_ERROR("[Set TX TAPS][%d] Get unknown fw rate mode(%u).", lane, fw_rate);
            return CR_FAIL;
        }
        LOGS_DEBUG("[Set TX TAPS][%d] %d %d %d %d %d %d %d", lane, tx_taps[0], tx_taps[1], tx_taps[2], tx_taps[3],
                   tx_taps[4], tx_taps[5], tx_taps[6]);
    } else {
        memcpy(tx_taps, se_slice->tx_taps[lane], sizeof(int) * 7);
    }
    return swift_set_tx_taps(slice, lane, tx_taps);
}

CredoError_t se_set_tx_taps(CredoSlice_t* slice, int lane, const int taps[]) {
    SeSlice_t* se_slice = (SeSlice_t*)slice;
    memcpy(se_slice->tx_taps[lane], taps, sizeof(int) * 7);

    unsigned fw_status;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));
    if (fw_status == 0) {
        return swift_set_tx_taps(slice, lane, taps);
    }

    return se_set_tx_taps_internal(slice, lane, true);
}

CredoError_t se_get_tx_taps(CredoSlice_t* slice, int lane, int taps[]) {
    ERR_PROPS(swift_get_tx_taps(slice, lane, taps));

    CredoLaneMode_t lane_mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_tx_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_OFF) return CR_OK;

    unsigned fw_rate = FW_RATE_FULL;
    ERR_PROPS(se_fw_get_tx_rate_mode(slice, lane, &fw_rate));

    if (fw_rate == FW_RATE_HALF) {
        taps[1] = taps[0];
        taps[3] = taps[4];
        taps[4] = taps[6];
        taps[0] = taps[5] = taps[6] = 0;
    } else if (fw_rate == FW_RATE_QUARTER) {
        taps[2] = taps[0];
        taps[3] = taps[4];
        taps[0] = taps[1] = taps[4] = taps[5] = taps[6] = 0;
    }

    return CR_OK;
}

CredoError_t se_get_rx_signal_detect(CredoSlice_t* slice, int lane, int* sd) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_sd(slice, lane, sd));
    } else {
        ERR_PROPS(swift_get_rx_signal_detect(slice, lane, sd));
    }
    return CR_OK;
}

CredoError_t se_get_dfe_f0(CredoSlice_t* slice, int lane, unsigned* f0) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_dfe_f0(slice, lane, f0));
    } else {
        ERR_PROPS(swift_get_rx_dfe_f0(slice, lane, f0));
    }
    return CR_OK;
}

CredoError_t se_set_dfe_f0(CredoSlice_t* slice, int lane, unsigned f0) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_set_dfe_f0(slice, lane, f0));
    } else {
        ERR_PROPS(swift_set_rx_dfe_f0(slice, lane, f0));
    }
    return CR_OK;
}

CredoError_t se_get_dfe_f1(CredoSlice_t* slice, int lane, int* f1) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_dfe_f1(slice, lane, f1));
    } else {
        ERR_PROPS(swift_get_rx_dfe_f1(slice, lane, f1));
    }
    return CR_OK;
}

CredoError_t se_set_dfe_f1(CredoSlice_t* slice, int lane, int f1) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_set_dfe_f1(slice, lane, f1));
    } else {
        ERR_PROPS(swift_set_rx_dfe_f1(slice, lane, f1));
    }
    return CR_OK;
}

CredoError_t se_get_dfe(CredoSlice_t* slice, int lane, int dfe[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_dfe(slice, lane, dfe));
    } else {
        ERR_PROPS(swift_get_rx_dfe(slice, lane, dfe));
    }
    return CR_OK;
}

CredoError_t se_set_dfe(CredoSlice_t* slice, int lane, int dfe[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_set_dfe(slice, lane, dfe));
    } else {
        ERR_PROPS(swift_set_rx_dfe(slice, lane, dfe));
    }
    return CR_OK;
}

CredoError_t se_get_rx_ffe_all(CredoSlice_t* slice, int lane, int taps[]) {
    unsigned fw_rdy = 0;

    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_rx_ffe_all(slice, lane, taps));
    } else {
        ERR_PROPS(swift_get_rx_ffe_all(slice, lane, taps));
    }

    return CR_OK;
}

CredoError_t se_get_rx_ffe(CredoSlice_t* slice, int lane, int taps[]) {
    unsigned fw_rdy = 0;

    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_rx_ffe(slice, lane, 0, taps));
    } else {
        ERR_PROPS(swift_get_rx_ffe(slice, lane, 0, taps));
    }

    return CR_OK;
}

CredoError_t se_get_rx_ffe_cm1(CredoSlice_t* slice, int lane, int* cm1) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_rx_ffe_cm1(slice, lane, cm1));
    } else {
        ERR_PROPS(swift_get_rx_ffe_cm1(slice, lane, cm1));
    }

    return CR_OK;
}

CredoError_t se_set_rx_ffe_cm1(CredoSlice_t* slice, int lane, int cm1) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_set_rx_ffe_cm1(slice, lane, cm1));
    } else {
        ERR_PROPS(swift_set_rx_ffe_cm1(slice, lane, cm1));
    }

    return CR_OK;
}

CredoError_t se_get_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_rx_flt_sel(slice, lane, flt_sel));
    } else {
        ERR_PROPS(swift_get_rx_flt_sel(slice, lane, flt_sel));
    }

    return CR_OK;
}

CredoError_t se_set_rx_flt_sel(CredoSlice_t* slice, int lane, unsigned flt_sel[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_set_rx_flt_sel(slice, lane, flt_sel));
    } else {
        // ERR_PROPS(swift_get_rx_flt_sel(slice, lane, flt_sel));
        return CR_NOTIMPLEMENTED;
    }

    return CR_OK;
}

CredoError_t se_get_rx_flt_loc(CredoSlice_t* slice, int lane, unsigned flt_loc[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    unsigned flt_sel[DSP_FLT_COUNT] = {0};
    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_rx_flt_sel(slice, lane, flt_sel));
    } else {
        ERR_PROPS(swift_get_rx_flt_sel(slice, lane, flt_sel));
    }

    ERR_PROPS(swift_get_rx_flt_location_by_sel(slice, lane, flt_sel, flt_loc));

    return CR_OK;
}

CredoError_t se_get_rx_envelope(CredoSlice_t* slice, int lane, unsigned envelope[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_amp(slice, lane, envelope));
    } else {
        ERR_PROPS(swift_get_rx_envelope(slice, lane, 0, envelope));
    }

    return CR_OK;
}

CredoError_t se_get_dc_sar(CredoSlice_t* slice, int lane, int dc_sar[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_dc_sar(slice, lane, dc_sar));
    } else {
        ERR_PROPS(swift_get_rx_dc_sar(slice, lane, dc_sar));
    }

    return CR_OK;
}

CredoError_t se_get_gain_sar(CredoSlice_t* slice, int lane, unsigned gain_sar[]) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_gain_sar(slice, lane, gain_sar));
    } else {
        ERR_PROPS(swift_get_rx_gain_sar(slice, lane, gain_sar));
    }

    return CR_OK;
}

CredoError_t se_get_dc_cmn(CredoSlice_t* slice, int lane, int* dc_cmn) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    CredoError_t ret = CR_FAIL;
    if (fw_rdy == 1) {
        ret = se_fw_get_dc_cmn(slice, lane, dc_cmn);
    }

    if (ret == CR_FAIL) {
        ret = swift_get_rx_dc_cmn(slice, lane, dc_cmn);
    }
    return ret;
}

CredoError_t se_get_rx_vga(CredoSlice_t* slice, int lane, unsigned* vga) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_get_rx_vga(slice, lane, vga));
    } else {
        ERR_PROPS(swift_get_rx_vga(slice, lane, vga));
    }
    return CR_OK;
}

CredoError_t se_set_rx_vga(CredoSlice_t* slice, int lane, unsigned vga) {
    unsigned fw_rdy = 0;
    ERR_PROPS(se_fw_get_status(slice, &fw_rdy));

    if (fw_rdy == 1) {
        ERR_PROPS(se_fw_set_rx_vga(slice, lane, vga));
    } else {
        ERR_PROPS(swift_set_rx_vga(slice, lane, vga));
    }
    return CR_OK;
}

CredoError_t se_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_tx_lane_mode(slice, lane, &mode));
    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(se_set_tx_prbs_pam4(slice, lane, enable, prbs_mode));
    } else {
        ERR_PROPS(se_set_tx_prbs_nrz(slice, lane, enable, prbs_mode));
    }

    return CR_OK;
}

CredoError_t se_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (enable && mode == CR_LMODE_PAM4) {
        writeRegLane(slice, lane, REG_RX_PRBS_MISMATCH_THRES, 10);
        writeRegLane(slice, lane, REG_RX_PRBS_SYNC_LOSS_THRES, 5);
    }

    ERR_PROPS(swift_set_rx_prbs(slice, lane, enable, prbs_mode));

    return CR_OK;
}

/* This function used for debug only, assume in PAM4 mode */
CredoError_t se_set_tx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = 0;
    patt = common_dsp_tx_prbs_type_credo_to_hw(prbs_mode, CR_LMODE_PAM4);
    if (patt == DSP_PRBS_INVALID) {
        LOGS_ERROR("[Set TX PRBS] Unsupported PRBS mode %d", prbs_mode);
        return CR_FAIL;
    }

    unsigned status = 0;
    ERR_PROPS(se_fw_get_status(slice, &status));
    if (status == 0) {
        LOGS_WARN("Firmware is not running!");
        return swift_set_tx_prbs_pam4(slice, lane, enable, prbs_mode);
    }

    if (!enable) {
        return se_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
    }

    ERR_PROPS(se_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_PAM4));
    ERR_PROPS(common_dsp_set_tx_prbs_mode(slice, lane, prbs_mode));
    return CR_OK;
}

/* This function used for debug only, assume in NRZ mode */
CredoError_t se_set_tx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = 0;
    patt = common_dsp_tx_prbs_type_credo_to_hw(prbs_mode, CR_LMODE_NRZ);
    if (patt == DSP_PRBS_INVALID) {
        LOGS_ERROR("[Set TX PRBS] Unsupported PRBS mode %d", prbs_mode);
        return CR_FAIL;
    }

    unsigned status = 0;
    ERR_PROPS(se_fw_get_status(slice, &status));
    if (status == 0) {
        LOGS_WARN("Firmware is not running!");
        return swift_set_tx_prbs_nrz(slice, lane, enable, prbs_mode);
    }

    if (!enable) {
        return se_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE);
    }
    ERR_PROPS(se_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_NRZ));
    ERR_PROPS(common_dsp_set_tx_prbs_mode(slice, lane, prbs_mode));
    return CR_OK;
}

CredoError_t se_set_tx_test_pattern_enable(CredoSlice_t* slice, int lane, bool enable) {
    unsigned status;

    se_fw_get_status(slice, &status);
    if (status == 1) {
        CredoLaneMode_t mode;
        if (enable) {
            ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
            if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ) {
                mode = CR_LMODE_NRZ;
                LOGS_WARN("[Set TX PRBS] Unknown lane mode %d, treat as NRZ", mode);
            }
            if (mode == CR_LMODE_PAM4) {
                ERR_PROPS(se_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_PAM4));
            } else {
                ERR_PROPS(se_fw_tx_control(slice, lane, FW_TX_SOURCE_PRBS_NRZ));
            }
        } else {
            ERR_PROPS(se_fw_tx_control(slice, lane, FW_TX_SOURCE_NOFORCE));
        }

    } else {
        LOGS_WARN("Firmware is not running!");
        return swift_set_tx_prbs(slice, lane, enable, 0);  // don't care prbs_mode
    }

    return swift_set_tx_test_pattern_enable(slice, lane, enable);
}

CredoError_t se_get_eye_avg_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CredoLaneMode_t lane_mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_PAM4) {
        *count = 3;
    } else if (lane_mode == CR_LMODE_NRZ) {
        *count = 2;
    } else {
        *count = 0;
    }
    return CR_OK;
}

CredoError_t se_get_eye_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CredoLaneMode_t lane_mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_PAM4) {
        *count = 14;
    } else if (lane_mode == CR_LMODE_NRZ) {
        *count = 2;
    } else {
        *count = 0;
    }
    return CR_OK;
}

CredoError_t se_get_eye_all_count(CredoSlice_t* slice, int lane, unsigned* count) {
    ERR_PROPS(se_get_eye_count(slice, lane, count));
    *count = PHASE_NUM * (*count);
    return CR_OK;
}

CredoError_t se_get_agcgain_count(CredoSlice_t* slice, int lane, unsigned* count) {
    *count = DSP_AGCGAIN_COUNT;
    return CR_OK;
}

CredoError_t se_get_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    ERR_PROPS(swift_get_rx_agcgain(slice, lane, agcgain));
    return CR_OK;
}

CredoError_t se_set_agcgain(CredoSlice_t* slice, int lane, unsigned agcgain[]) {
    unsigned fw_status = 0;
    ERR_PROPS(hal_fw_get_status(slice, &fw_status));

    if (fw_status == 1) {
        ERR_PROPS(se_fw_set_rx_agcgain(slice, lane, agcgain));
    } else {
        ERR_PROPS(swift_set_rx_agcgain(slice, lane, agcgain));
    }
    return CR_OK;
}

CredoError_t se_get_ppm(CredoSlice_t* slice, int lane, int* ppm) {
    ERR_PROPS(swift_get_rx_ppm(slice, lane, ppm));

    int ppm_shift = FW_CMD_LOG_SILENT;
    CredoError_t err = hal_fw_debug_cmd(slice, lane, OPT_DEBUG, OPT_DEBUG_RX_PPM_SHIFT, (unsigned*)&ppm_shift);
    if (err == CR_OK) {
        ppm_shift = ppm_shift & 0x8000 ? (int)ppm_shift - 0x10000 : (int)ppm_shift;
        *ppm -= ppm_shift;
    }
    return CR_OK;
}

CredoError_t se_get_skef(CredoSlice_t* slice, int lane, unsigned skef[]) {
    unsigned en = 0, degen = 0, addcap = 0;
    ERR_PROPS(swift_get_rx_skef(slice, lane, &en, &degen, &addcap));
    skef[0] = en;
    skef[1] = degen;
    skef[2] = addcap;

    return CR_OK;
}

CredoError_t se_set_skef(CredoSlice_t* slice, int lane, unsigned skef[]) {
    ERR_PROPS(swift_set_rx_skef(slice, lane, skef[0], skef[1], skef[2]));
    return CR_OK;
}

CredoError_t se_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    unsigned reg_pol;
    ERR_PROPS(readReg(slice, REG_FW_TX_POL, &reg_pol));
    *tx_pol = !((reg_pol >> lane) & 0x1);
    return CR_OK;
}

CredoError_t se_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    unsigned reg_pol;
    ERR_PROPS(readReg(slice, REG_FW_TX_POL, &reg_pol));
    reg_pol &= ~(0x1 << lane);
    reg_pol |= ((!tx_pol) & 0x1) << lane;
    ERR_PROPS(writeReg(slice, REG_FW_TX_POL, reg_pol));
    return swift_set_tx_polarity(slice, lane, tx_pol);
}

CredoError_t se_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    unsigned reg_pol;
    ERR_PROPS(readReg(slice, REG_FW_RX_POL, &reg_pol));
    *rx_pol = (reg_pol >> lane) & 0x1;
    return CR_OK;
}

CredoError_t se_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    unsigned reg_pol;
    ERR_PROPS(readReg(slice, REG_FW_RX_POL, &reg_pol));
    reg_pol &= ~(0x1 << lane);
    reg_pol |= (rx_pol & 0x1) << lane;
    ERR_PROPS(writeReg(slice, REG_FW_RX_POL, reg_pol));
    return swift_set_rx_polarity(slice, lane, rx_pol);
}

static CredoError_t se_set_lane_r2t_loopback_mode(CredoSlice_t* slice, int lane, int enable) {
    if (enable) {
        ERR_PROPS(se_fw_PLL_source_control(slice, lane, 1));
        ERR_PROPS(se_fw_TRF_control(slice, lane, TRF_OnetoOne));
        ERR_PROPS(se_fw_force_loopback(slice, lane, 1));
    } else {
        ERR_PROPS(se_fw_TRF_control(slice, lane, TRF_NOFORCE));
        ERR_PROPS(se_fw_PLL_source_control(slice, lane, 0));
        ERR_PROPS(se_fw_force_loopback(slice, lane, 0));
    }
    return CR_OK;
}

static CredoError_t se_set_lane_t2r_loopback_mode(CredoSlice_t* slice, int lane, int enable) {
    ERR_PROPS(se_fw_t2r_force_loopback(slice, lane, enable));
    return CR_OK;
}

CredoError_t se_set_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t mode) {
    CredoLaneMode_t lane_mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &lane_mode));
    if (lane_mode == CR_LMODE_OFF) {
        LOGS_WARN("Lane %s(%2d) is not configured", stringify_lane_id(slice, lane), lane);
        return CR_FAIL;
    }
    if (lane_mode == CR_LMODE_DISABLE) {
        LOGS_WARN("Lane %s(%2d) is disabled", stringify_lane_id(slice, lane), lane);
        return CR_FAIL;
    }

    /* TODO
    unsigned fw_mode = 0;
    ERR_PROPS(hal_fw_debug_cmd(slice, lane, SE_INFO, SE_INFO_CONFIG_SEL, &fw_mode));
    if (fw_mode == MODE_LOOPBACK) {
        LOGS_WARN("Lane %s(%2d) is in phy loopback mode", stringify_lane_id(slice, lane), lane);
        return CR_FAIL;
    }
    */

    CredoLaneLoopbackMode_t cur_lb_mode = CR_LB_DISABLED;
    ERR_PROPS(hal_get_lane_loopback_mode(slice, lane, &cur_lb_mode));
    if (cur_lb_mode == mode) {
        if (mode != CR_LB_DISABLED)
            LOGS_WARN("Lane %s(%d) skip same loopback mode setting", stringify_lane_id(slice, lane), lane);
        return CR_OK;
    }

    // first disable previous mode
    if (cur_lb_mode == CR_LB_RX_TO_TX) {
        ERR_PROPS(se_set_lane_r2t_loopback_mode(slice, lane, 0));
    } else if (cur_lb_mode == CR_LB_TX_TO_RX) {
        ERR_PROPS(se_set_lane_t2r_loopback_mode(slice, lane, 0));
    }

    // set mode
    if (mode == CR_LB_RX_TO_TX) {
        ERR_PROPS(se_set_lane_r2t_loopback_mode(slice, lane, 1));
    } else if (mode == CR_LB_TX_TO_RX) {
        ERR_PROPS(se_set_lane_t2r_loopback_mode(slice, lane, 1));
    }

    return CR_OK;
}

CredoError_t se_get_lane_loopback_mode(CredoSlice_t* slice, int lane, CredoLaneLoopbackMode_t* mode) {
    unsigned loopback_r2t_en = 0;
    unsigned loopback_t2r_en = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_LOOPBACK_R2T_EN, &loopback_r2t_en));
    ERR_PROPS(readRegLane(slice, lane, REG_LOOPBACK_T2R_EN, &loopback_t2r_en));

    if (loopback_r2t_en && loopback_t2r_en) {
        LOGS_ERROR("Lane %s(%2d) is unknown loopback mode, force to disable", stringify_lane_id(slice, lane), lane);
        ERR_PROPS(se_set_lane_r2t_loopback_mode(slice, lane, 0));
        ERR_PROPS(se_set_lane_t2r_loopback_mode(slice, lane, 0));
        *mode = CR_LB_DISABLED;
        return CR_FAIL;
    } else if (loopback_r2t_en) {
        *mode = CR_LB_RX_TO_TX;
    } else if (loopback_t2r_en) {
        *mode = CR_LB_TX_TO_RX;
    } else {
        *mode = CR_LB_DISABLED;
    }

    return CR_OK;
}

static const int tx_taps_presets_fr[][7] = {
    {0, 5, -11, 40, -2, 0, 0},  // 0-10dB
    {0, 5, -13, 41, -3, 0, 0},  // 10dB-25dB
    {0, 5, -14, 41, -3, 0, 0},  // 25dB-35dB
    {0, 3, -13, 45, -2, 0, 0},  // 35+dB
};

static const int tx_taps_presets_hr[][7] = {
    {0, 0, -12, 51, 0, 0, 0},  // 0-10dB
};

static CredoError_t se_serdes_preset_tx_taps_fr(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    const int* tx_taps = NULL;
    if (desc->channel_loss <= 10) {
        tx_taps = tx_taps_presets_fr[0];
    } else if (desc->channel_loss <= 25) {
        tx_taps = tx_taps_presets_fr[1];
    } else if (desc->channel_loss <= 35) {
        tx_taps = tx_taps_presets_fr[2];
    } else {
        tx_taps = tx_taps_presets_fr[3];
    }
    return hal_set_tx_taps(slice, lane, tx_taps);
}

static CredoError_t se_serdes_preset_tx_taps_hr(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    return hal_set_tx_taps(slice, lane, tx_taps_presets_hr[0]);
}

CredoError_t se_serdes_preset_tx_taps(CredoSlice_t* slice, int lane, const CredoChannelDesc_t* desc) {
    if (desc->speed < 80000) {
        return se_serdes_preset_tx_taps_hr(slice, lane, desc);
    } else {
        return se_serdes_preset_tx_taps_fr(slice, lane, desc);
    }
}
