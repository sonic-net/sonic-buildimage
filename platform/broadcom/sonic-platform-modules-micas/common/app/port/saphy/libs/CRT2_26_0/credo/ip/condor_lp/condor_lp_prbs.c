/*
 *
 * Condor LP PRBS control
 */
#include "project.h"

#include "condor_lp/condor_lp_regmap.h"
#include "condor_lp/condor_lp_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <limits.h>
#include <stdlib.h>

/* Translate HAL PRBS mode into Prbs_t */
CredoLanePrbsPattern_t condor_lp_to_credo(unsigned condor_lp_prbs_mode, CredoLaneMode_t mode) {
    CredoLanePrbsPattern_t drv_prbs_mode;
    if (mode == CR_LMODE_NRZ) {
        switch (condor_lp_prbs_mode) {
            case 0:
                drv_prbs_mode = CR_PRBS9;
                break;
            case 1:
                drv_prbs_mode = CR_PRBS15;
                break;
            case 2:
                drv_prbs_mode = CR_PRBS23;
                break;
            case 3:
                drv_prbs_mode = CR_PRBS31;
                break;
            default:
                drv_prbs_mode = CR_PRBS_UNKNOWN;
        }
    } else if (mode == CR_LMODE_PAM4) {
        switch (condor_lp_prbs_mode) {
            case 0:
                drv_prbs_mode = CR_PRBS9;
                break;
            case 1:
                drv_prbs_mode = CR_PRBS13;
                break;
            case 2:
                drv_prbs_mode = CR_PRBS15;
                break;
            case 3:
                drv_prbs_mode = CR_PRBS31;
                break;
            default:
                drv_prbs_mode = CR_PRBS_UNKNOWN;
        }
    } else {
        drv_prbs_mode = CR_PRBS_UNKNOWN;
    }
    return drv_prbs_mode;
}

unsigned credo_to_condor_lp(CredoLanePrbsPattern_t prbs_mode, CredoLaneMode_t mode) {
    unsigned condor_lp_prbs_mode = CONDOR_LP_PRBS_INVALID;
    if (mode == CR_LMODE_NRZ) {
        switch (prbs_mode) {
            case CR_PRBS9:
                condor_lp_prbs_mode = 0;
                break;
            case CR_PRBS15:
                condor_lp_prbs_mode = 1;
                break;
            case CR_PRBS23:
                condor_lp_prbs_mode = 2;
                break;
            case CR_PRBS31:
                condor_lp_prbs_mode = 3;
                break;
            default:;
        }
    } else if (mode == CR_LMODE_PAM4) {
        switch (prbs_mode) {
            case CR_PRBS9:
                condor_lp_prbs_mode = 0;
                break;
            case CR_PRBS13:
                condor_lp_prbs_mode = 1;
                break;
            case CR_PRBS15:
                condor_lp_prbs_mode = 2;
                break;
            case CR_PRBS31:
                condor_lp_prbs_mode = 3;
                break;
            default:;
        }
    }
    return condor_lp_prbs_mode;
}

CredoError_t condor_lp_get_rx_prbs_enable(CredoSlice_t* slice, int lane, int* enable) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(condor_lp_get_rx_prbs_enable_pam4(slice, lane, enable));
    } else {
        ERR_PROPS(condor_lp_get_rx_prbs_enable_nrz(slice, lane, enable));
        if (mode != CR_LMODE_NRZ && mode != CR_LMODE_DISABLE) {
            LOGS_WARN("[GET RX PRBS ENABLE] Unknown mode %d, treat as NRZ", mode);
        }
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_enable_nrz(CredoSlice_t* slice, int lane, int* enable) {
    unsigned en;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU_NRZ, &en));
    *enable = en;

    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_enable_pam4(CredoSlice_t* slice, int lane, int* enable) {
    unsigned en;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU, &en));
    *enable = en;

    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_enable_nrz(CredoSlice_t* slice, int lane, int enable) {
    return writeRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU_NRZ, enable);
}

CredoError_t condor_lp_set_rx_prbs_enable_pam4(CredoSlice_t* slice, int lane, int enable) {
    return writeRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU, enable);
}

CredoError_t condor_lp_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned en = 0, patt = 0, test_patt_sc = 0, test_patt_en = 0, prbs_clk_en = 0;
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, &prbs_clk_en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, &patt));
    } else {
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, &prbs_clk_en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, &patt));
        if (mode != CR_LMODE_NRZ && mode != CR_LMODE_DISABLE) {
            LOGS_WARN("[GET TX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
    }

    ERR_PROPS(readRegLane(slice, lane, REG_TX_PATTERN, &test_patt_sc));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_TEST_PATT_EN, &test_patt_en));

    *enable = en & test_patt_sc & test_patt_en & prbs_clk_en;
    *prbs_mode = condor_lp_to_credo(patt, mode);
    return CR_OK;
}

/* TX prbs setting should go through firmware. This should be just a reference when no firmware is available */
// sync from canary
CredoError_t condor_lp_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    unsigned patt = credo_to_condor_lp(prbs_mode, mode);
    if (patt == CONDOR_LP_PRBS_INVALID) {
        LOGS_ERROR("[Set TX PRBS][%d] Unsupported PRBS mode %d", lane, prbs_mode);
        return CR_FAIL;
    }

    /* Programming order:
     * Enable: Pattern, PRBS clock enable, PRBS disable/enable, test pattern=PRBS, test pattern=1
     * Disable: test pattern=0, PRBS disable (all), PRBS clock disable
     */
    if (enable) {  // enable. Set pattern type first
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, patt));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }
    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, 0));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, 0));
        if (mode != CR_LMODE_NRZ && mode != CR_LMODE_DISABLE) {
            LOGS_WARN("[GET TX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
    }
    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }

    return CR_OK;
}

// wrapper function
CredoError_t condor_lp_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(condor_lp_get_rx_prbs_enable_pam4(slice, lane, enable));
        ERR_PROPS(condor_lp_get_rx_prbs_pam4(slice, lane, prbs_mode));
    } else {
        ERR_PROPS(condor_lp_get_rx_prbs_enable_nrz(slice, lane, enable));
        ERR_PROPS(condor_lp_get_rx_prbs_nrz(slice, lane, prbs_mode));
        if (mode != CR_LMODE_NRZ && mode != CR_LMODE_DISABLE) {
            LOGS_WARN("[GET RX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
    }

    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_nrz(CredoSlice_t* slice, int lane, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned patt;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_MODE_NRZ, &patt));
    *prbs_mode = condor_lp_to_credo(patt, CR_LMODE_NRZ);

    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_pam4(CredoSlice_t* slice, int lane, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned patt;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_MODE, &patt));
    *prbs_mode = condor_lp_to_credo(patt, CR_LMODE_PAM4);

    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* status) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode != CR_LMODE_PAM4) {
        *status = CR_PRBS_LOCK_INVALID;
        return CR_OK;
    }
    int prbs_enable;
    CredoLanePrbsPattern_t prbs_patt;
    ERR_PROPS(hal_get_rx_prbs(slice, lane, &prbs_enable, &prbs_patt));

    // prbs not enabled so lock is invalid
    if (!prbs_enable) {
        *status = CR_PRBS_LOCK_INVALID;
        return CR_OK;
    }
    unsigned prbs_lock = 0;
    readRegLane(slice, lane, REG_RX_PRBS_SYNC, &prbs_lock);
    *status = prbs_lock ? CR_PRBS_LOCK_YES : CR_PRBS_LOCK_NO;
    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ && mode != CR_LMODE_DISABLE) {
        LOGS_WARN("[Set RX PRBS][%d] Unknown lane mode %d, treat as NRZ", lane, mode);
        mode = CR_LMODE_NRZ;
    }

    if (mode == CR_LMODE_NRZ) {
        ERR_PROPS(condor_lp_set_rx_prbs_nrz(slice, lane, enable, prbs_mode));
    } else {
        ERR_PROPS(condor_lp_set_rx_prbs_pam4(slice, lane, enable, prbs_mode));
    }

    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = credo_to_condor_lp(prbs_mode, CR_LMODE_NRZ);
    if (patt == CONDOR_LP_PRBS_INVALID) {
        LOGS_ERROR("[Set RX PRBS][%d] Unsupported PRBS mode %d", lane, prbs_mode);
        return CR_FAIL;
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU_NRZ, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_MODE_NRZ, patt));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = credo_to_condor_lp(prbs_mode, CR_LMODE_PAM4);
    if (patt == CONDOR_LP_PRBS_INVALID) {
        LOGS_ERROR("[Set RX PRBS][%d] Unsupported PRBS mode %d", lane, prbs_mode);
        return CR_FAIL;
    }
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_MODE, patt));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_error_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_get_rx_prbs_error_count_nrz(slice, lane, count));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_get_rx_prbs_error_count_pam4(slice, lane, count));
            break;
        case CR_LMODE_DISABLE:
            *count = 0;
            break;
        default:
            LOGS_WARN("[Get RX PRBS error count] Unknown mode %d, return 0", mode);
            *count = 0;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_error_count_nrz(CredoSlice_t* slice, int lane, unsigned* count) {
    unsigned int cnt_lsb = 0;
    unsigned int cnt_msb = 0;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_LSB_NRZ, &cnt_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_MSB_NRZ, &cnt_msb));
    *count = (cnt_msb << 16) + cnt_lsb;
    *count /= 3;  // real error count is raw count divided by 3
    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_error_count_pam4(CredoSlice_t* slice, int lane, unsigned* count) {
    unsigned int cnt_lsb = 0;
    unsigned int cnt_msb = 0;

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CNT_ATOMIC_READ, 2));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CNT_ATOMIC_READ, 3));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_LSB, &cnt_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_MSB, &cnt_msb));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CNT_ATOMIC_READ, 0));
    *count = (cnt_msb << 16) + cnt_lsb;
    return CR_OK;
}

CredoError_t condor_lp_rx_prbs_rst(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    get_time(&slice->data->prbs_timer[lane]);  // reset prbs_timer to now
    switch (mode) {
        case CR_LMODE_NRZ:
            return condor_lp_rx_prbs_rst_nrz(slice, lane);
        case CR_LMODE_PAM4:
            return condor_lp_rx_prbs_rst_pam4(slice, lane);
        case CR_LMODE_DISABLE:
            return CR_OK;  // do nothing
        default:
            LOGS_WARN("[RX PRBS counter reset] Unknown mode %d, resetting both", mode);
            ERR_PROPS(condor_lp_rx_prbs_rst_nrz(slice, lane));
            return condor_lp_rx_prbs_rst_pam4(slice, lane);
    }
}

CredoError_t condor_lp_get_prbs_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    unsigned auto_sync;
    switch (mode) {
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_AUTO_SYNC_EN, &auto_sync));
            *enabled = (auto_sync != 0);
            break;
        default:
            *enabled = false;
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_rx_prbs_rst_nrz(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_RST_NRZ, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_RST_NRZ, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_RST_NRZ, 0));
    return CR_OK;
}

CredoError_t condor_lp_rx_prbs_rst_pam4(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_AUTO_SYNC_EN, 1));

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_RST, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_ERR_CNTR_RST, 0));

    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_AUTO_SYNC_EN, 0));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_checker_enable_nrz(CredoSlice_t* slice, int lane, int* enable) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU_NRZ, (unsigned*)enable));

    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_checker_enable_nrz(CredoSlice_t* slice, int lane, int enable) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU_NRZ, enable));

    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_checker_enable_pam4(CredoSlice_t* slice, int lane, int* enable) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU, (unsigned*)enable));

    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_checker_enable_pam4(CredoSlice_t* slice, int lane, int enable) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CHECKER_PU, enable));

    return CR_OK;
}

CredoError_t condor_lp_prbs_gen_1error(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(condor_lp_prbs_gen_1error_nrz(slice, lane));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(condor_lp_prbs_gen_1error_pam4(slice, lane));
            break;
        case CR_LMODE_DISABLE:
            break;
        default:
            LOGS_WARN("[PRBS Generate 1 error] Unknown mode %d, skipped", mode);
            break;
    }
    return CR_OK;
}

CredoError_t condor_lp_prbs_gen_1error_nrz(CredoSlice_t* slice, int lane) {
    /* This will show 3 errors in counter. It's designed this way -- there is only 1 bit error. */
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 0));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 1));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 0));
    return CR_OK;
}

CredoError_t condor_lp_prbs_gen_1error_pam4(CredoSlice_t* slice, int lane) {
    /* PAM4 has synced error counter. This will actually show 1 error. */
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 0));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 1));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 0));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev) {
    if (prev > 4) return CR_INVALID_ARGS;
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PREV_EN, en));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PREV, prev));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase) {
    if (phase > 8) return CR_INVALID_ARGS;
    uint8_t val = (phase == 0) ? 0 : ~(0x1 << (phase - 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PHASE, val));
    return CR_OK;
}

CredoError_t condor_lp_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_CNTR_RESET, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_CNTR_RESET, 0));
    return CR_OK;
}

CredoError_t condor_lp_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]) {
    unsigned lsb = 0, msb = 0;
    for (int i = 0; i < 12; i++) {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_PATTERN_LSB(i), &lsb));
        ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_PATTERN_MSB(i), &msb));
        pattern_count[i] = (msb << 16) + lsb;
    }
    return CR_OK;
}
