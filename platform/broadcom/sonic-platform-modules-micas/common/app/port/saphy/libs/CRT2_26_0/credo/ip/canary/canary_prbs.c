/*
 * Canary PRBS control
 */
#include "project.h"

#include "canary/canary_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <limits.h>
#include <stdlib.h>

/* Translate HAL PRBS mode into Prbs_t */
CredoLanePrbsPattern_t canary_to_credo(unsigned canary_prbs_mode, CredoLaneMode_t mode) {
    CredoLanePrbsPattern_t drv_prbs_mode;
    if (mode == CR_LMODE_NRZ) {
        switch (canary_prbs_mode) {
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
        switch (canary_prbs_mode) {
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

unsigned credo_to_canary(CredoLanePrbsPattern_t prbs_mode, CredoLaneMode_t mode) {
    unsigned canary_prbs_mode = CANARY_PRBS_INVALID;
    if (mode == CR_LMODE_NRZ) {
        switch (prbs_mode) {
            case CR_PRBS9:
                canary_prbs_mode = 0;
                break;
            case CR_PRBS15:
                canary_prbs_mode = 1;
                break;
            case CR_PRBS23:
                canary_prbs_mode = 2;
                break;
            case CR_PRBS31:
                canary_prbs_mode = 3;
                break;
            default:;
        }
    } else if (mode == CR_LMODE_PAM4) {
        switch (prbs_mode) {
            case CR_PRBS9:
                canary_prbs_mode = 0;
                break;
            case CR_PRBS13:
                canary_prbs_mode = 1;
                break;
            case CR_PRBS15:
                canary_prbs_mode = 2;
                break;
            case CR_PRBS31:
                canary_prbs_mode = 3;
                break;
            default:;
        }
    }
    return canary_prbs_mode;
}

CredoError_t canary_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned en = 0, patt = 0, test_patt_sc = 0, test_patt_en = 0, prbs_clk_en = 0;
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, &prbs_clk_en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, &patt));
    } else {
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_CLK_EN_NRZ, &prbs_clk_en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, &patt));
        if (mode != CR_LMODE_NRZ) {
            LOGS_WARN("[GET TX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
        mode = CR_LMODE_NRZ;
    }

    ERR_PROPS(readRegLane(slice, lane, REG_TX_PATTERN, &test_patt_sc));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_TEST_PATT_EN, &test_patt_en));

    *enable = en & test_patt_sc & test_patt_en & prbs_clk_en;
    *prbs_mode = canary_to_credo(patt, mode);
    return CR_OK;
}

CredoError_t canary_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned en, patt;
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PRBS_CHECKER_PU, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PRBS_MODE, &patt));
    } else {
        ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_PRBS_CHECKER_PU, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_PRBS_MODE, &patt));
        if (mode != CR_LMODE_NRZ) {
            LOGS_WARN("[GET RX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
        mode = CR_LMODE_NRZ;
    }
    *enable = en;
    *prbs_mode = canary_to_credo(patt, mode);
    return CR_OK;
}

/* TX prbs setting should go through firmware. This should be just a reference when no firmware is available */
CredoError_t canary_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt;
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ) {
        LOGS_WARN("[Set TX PRBS][%d] Unknown lane mode %d, treat as NRZ", lane, mode);
        mode = CR_LMODE_NRZ;
    }
    patt = credo_to_canary(prbs_mode, mode);
    if (patt == CANARY_PRBS_INVALID) {
        LOGS_ERROR("[Set TX PRBS][%d] Unsupported PRBS mode %d", lane, prbs_mode);
        return CR_FAIL;
    }
    /* Programming order:
     * Enable: Pattern, PRBS clock enable, PRBS disable/enable, test pattern=PRBS, test pattern=1
     * Disable: test pattern=0, PRBS disable (all), PRBS clock disable
     */
    if (!enable) {  // disable. Disable test pattern first
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    } else {  // enable. Set pattern type first
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_PATT_SEL, patt));
    }
    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_CLK_EN_NRZ, 0));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_CLK_EN_NRZ, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, 0));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, 0));
    }
    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }

    return CR_OK;
}

CredoError_t canary_set_rx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = credo_to_canary(prbs_mode, CR_LMODE_NRZ);
    if (patt == CANARY_PRBS_INVALID) {
        LOGS_ERROR("[Set RX PRBS][%d] Unsupported PRBS mode %d", lane, prbs_mode);
        return CR_FAIL;
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_PRBS_CHECKER_PU, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_PRBS_MODE, patt));
    return CR_OK;
}

CredoError_t canary_set_rx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = credo_to_canary(prbs_mode, CR_LMODE_PAM4);
    if (patt == CANARY_PRBS_INVALID) {
        LOGS_ERROR("[Set RX PRBS][%d] Unsupported PRBS mode %d", lane, prbs_mode);
        return CR_FAIL;
    }
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_CHECKER_PU, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_MODE, patt));
    return CR_OK;
}

CredoError_t canary_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (mode != CR_LMODE_PAM4 && mode != CR_LMODE_NRZ) {
        LOGS_WARN("[Set RX PRBS][%d] Unknown lane mode %d, treat as NRZ", lane, mode);
        mode = CR_LMODE_NRZ;
    }
    if (mode == CR_LMODE_NRZ) {
        return canary_set_rx_prbs_nrz(slice, lane, enable, prbs_mode);
    } else {
        return canary_set_rx_prbs_pam4(slice, lane, enable, prbs_mode);
    }
}

CredoError_t canary_get_rx_prbs_count_nrz(CredoSlice_t* slice, int lane, unsigned* count) {
    unsigned int cnt_lsb = 0;
    unsigned int cnt_msb = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_ERR_CNTR_LSB, &cnt_lsb));
    ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_ERR_CNTR_MSB, &cnt_msb));
    *count = (cnt_msb << 16) + cnt_lsb;
    *count /= 3;  // real error count is raw count divided by 3
    return CR_OK;
}

CredoError_t canary_get_rx_prbs_count(CredoSlice_t* slice, int lane, unsigned* count) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(canary_get_rx_prbs_count_nrz(slice, lane, count));
            break;
        case CR_LMODE_PAM4: {
            unsigned int cnt_lsb = 0;
            unsigned int cnt_msb = 0;
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PRBS_CNT_ATOMIC_READ, 2));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PRBS_CNT_ATOMIC_READ, 3));
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_ERR_CNTR_LSB, &cnt_lsb));
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_ERR_CNTR_MSB, &cnt_msb));
            ERR_PROPS(writeRegLane(slice, lane, REG_FLR_PRBS_CNT_ATOMIC_READ, 0));
            *count = (cnt_msb << 16) + cnt_lsb;
            break;
        }
        default:
            LOGS_WARN("[Get RX PRBS error count] Unknown mode %d, return 0", mode);
            break;
    }
    return CR_OK;
}

CredoError_t canary_reset_rx_prbs_count_nrz(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_ERR_CNTR_RST, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_ERR_CNTR_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_ERR_CNTR_RST, 0));
    return CR_OK;
}

CredoError_t canary_reset_rx_prbs_count_pam4(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_AUTO_SYNC_EN, 1));

    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_ERR_CNTR_RST, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_ERR_CNTR_RST, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_ERR_CNTR_RST, 0));

    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_AUTO_SYNC_EN, 0));
    return CR_OK;
}

CredoError_t canary_get_prbs_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    unsigned auto_sync;
    switch (mode) {
        case CR_LMODE_PAM4:
            ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PRBS_AUTO_SYNC_EN, &auto_sync));
            *enabled = (auto_sync != 0);
            break;
        default:
            *enabled = false;
            break;
    }
    return CR_OK;
}

CredoError_t canary_reset_rx_prbs_count(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    get_time(&slice->data->prbs_timer[lane]);
    switch (mode) {
        case CR_LMODE_NRZ:
            return canary_reset_rx_prbs_count_nrz(slice, lane);
        case CR_LMODE_PAM4:
            return canary_reset_rx_prbs_count_pam4(slice, lane);
        default:
            LOGS_WARN("[RX PRBS counter reset] Unknown mode %d, resetting both", mode);
            ERR_PROPS(canary_reset_rx_prbs_count_nrz(slice, lane));
            return canary_reset_rx_prbs_count_pam4(slice, lane);
    }
}

CredoError_t canary_prbs_gen_1error(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    // ERR_PROPS( get_lane_mode(slice, lane, &mode));
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    switch (mode) {
        case CR_LMODE_NRZ:
            /* This will show 3 errors in counter. It's designed this way -- there is only 1 bit error. */
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 0));
            break;
        case CR_LMODE_PAM4:
            /* PAM4 has synced error counter. This will actually show 1 error. */
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 1));
            ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 0));
            break;
        default:
            LOGS_WARN("[PRBS Generate 1 error] Unknown mode %d, skipped", mode);
            break;
    }
    return CR_OK;
}

CredoError_t canary_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev) {
    if (prev > 4) return CR_INVALID_ARGS;
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_PREV_EN, en));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_PREV, prev));
    return CR_OK;
}

CredoError_t canary_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase) {
    if (phase > 8) return CR_INVALID_ARGS;
    uint8_t val = (phase == 0) ? 0 : ~(0x1 << (phase - 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_PHASE, val));
    return CR_OK;
}

CredoError_t canary_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_CNTR_RESET, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_CNTR_RESET, 0));
    return CR_OK;
}

CredoError_t canary_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]) {
    unsigned lsb = 0, msb = 0;
    for (int i = 0; i < 12; i++) {
        ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_LSB(i), &lsb));
        ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_PRBS_PATTERN_MSB(i), &msb));
        pattern_count[i] = (msb << 16) + lsb;
    }
    return CR_OK;
}
