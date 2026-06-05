/*
 * Common DSP PRBS control
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

#define PRBS_MAX 8
// 0 for NRZ, 1 for PAM4
static unsigned tx_prbs_hw_type_table[2][PRBS_MAX] = {
    // NRZ
    {4,    /* CR_PRBS7  */
     0,    /* CR_PRBS9  */
     5,    /* CR_PRBS11 */
     6,    /* CR_PRBS13 */
     1,    /* CR_PRBS15 */
     2,    /* CR_PRBS23 */
     3,    /* CR_PRBS31 */
     0xF}, /* CR_PRBS19 */
    // PAM4
    {4,    /* CR_PRBS7  */
     0,    /* CR_PRBS9  */
     5,    /* CR_PRBS11 */
     1,    /* CR_PRBS13 */
     2,    /* CR_PRBS15 */
     6,    /* CR_PRBS23 */
     3,    /* CR_PRBS31 */
     0xF}, /* CR_PRBS19 */
};

static unsigned rx_prbs_hw_type_table[PRBS_MAX] = {
    0,  /* CR_PRBS7  */
    1,  /* CR_PRBS9  */
    2,  /* CR_PRBS11 */
    3,  /* CR_PRBS13 */
    4,  /* CR_PRBS15 */
    5,  /* CR_PRBS23 */
    6,  /* CR_PRBS31 */
    0xF /* CR_PRBS19 */
};

CredoLanePrbsPattern_t common_dsp_rx_prbs_type_hw_to_credo(unsigned prbs_mode) {
    CredoLanePrbsPattern_t cr_prbs_mode = CR_PRBS_UNKNOWN;
    for (int i = 0; i < PRBS_MAX; i++) {
        if (rx_prbs_hw_type_table[i] == prbs_mode) {
            cr_prbs_mode = i;
            break;
        }
    }
    return cr_prbs_mode;
}

unsigned common_dsp_rx_prbs_type_credo_to_hw(CredoLanePrbsPattern_t prbs_mode) {
    if (prbs_mode >= PRBS_MAX) {
        return 0xF;
    }
    return rx_prbs_hw_type_table[prbs_mode];
}

CredoLanePrbsPattern_t common_dsp_tx_prbs_type_hw_to_credo(unsigned tx_prbs_mode, CredoLaneMode_t mode) {
    CredoLanePrbsPattern_t cr_prbs_mode = CR_PRBS_UNKNOWN;
    unsigned mode_idx = (mode == CR_LMODE_NRZ) ? 0 : 1;
    for (int i = 0; i < PRBS_MAX; i++) {
        if (tx_prbs_hw_type_table[mode_idx][i] == tx_prbs_mode) {
            cr_prbs_mode = i;
            break;
        }
    }
    return cr_prbs_mode;
}

unsigned common_dsp_tx_prbs_type_credo_to_hw(CredoLanePrbsPattern_t prbs_mode, CredoLaneMode_t mode) {
    if (prbs_mode >= PRBS_MAX) {
        return 0xF;
    }
    unsigned mode_idx = (mode == CR_LMODE_NRZ) ? 0 : 1;
    return tx_prbs_hw_type_table[mode_idx][prbs_mode];
}

CredoError_t common_dsp_set_tx_prbs_mode(CredoSlice_t* slice, int lane, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode = CR_LMODE_OFF;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    if (!IS_LANE_MODE_PAM4_OR_NRZ(mode)) {
        LOGS_ERROR("[Set TX prbs][%d] unsupported lane mode, must config mode first.", lane);
        return CR_FAIL;
    }

    unsigned prbs_type = common_dsp_tx_prbs_type_credo_to_hw(prbs_mode, mode);
    if (prbs_type == 0xF) {
        LOGS_ERROR("[Set TX prbs][%d] unsupported prbs type: %d.", lane, prbs_mode);
        return CR_FAIL;
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_MODE_SEL, prbs_type & 0x3));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_MODE_SEL_EXT, (prbs_type >> 2) & 0x1));

    return CR_OK;
}

CredoError_t common_dsp_get_rx_prbs_enable(CredoSlice_t* slice, int lane, int* enable) {
    unsigned en;
    ERR_PROP(readRegLane(slice, lane, REG_RX_PRBS_SYNC_CHECKER_PU, &en));
    *enable = en;
    return CR_OK;
}

CredoError_t common_dsp_set_rx_prbs_enable(CredoSlice_t* slice, int lane, int enable) {
    return writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CHECKER_PU, enable);
}

CredoError_t common_dsp_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned patt = 0;
    ERR_PROP(readRegLane(slice, lane, REG_RX_PRBS_SYNC_CHECKER_PU, (unsigned*)enable));
    ERR_PROP(readRegLane(slice, lane, REG_RX_PRBS_MODE_SEL, &patt));
    *prbs_mode = common_dsp_rx_prbs_type_hw_to_credo(patt);

    return CR_OK;
}

CredoError_t common_dsp_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt;

    patt = common_dsp_rx_prbs_type_credo_to_hw(prbs_mode);
    if (patt == 0xF) {
        LOGS_ERROR("[Set TX PRBS] Unsupported PRBS mode %d", prbs_mode);
        return CR_FAIL;
    }

    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CHECKER_PU, enable));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CNTR_RESET, 1));
    sleep_us(1);
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CNTR_RESET, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PREV_EN, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PREV, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_READOUT_CAPTURE, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_READOUT_SYNC_EN, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_RX_PRBS_AUTO_SYNC_EN, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_RX_PRBS_FORCE_RELOAD, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_CHK_PHASE_EN, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_MODE_SEL, patt));

    return CR_OK;
}

/* TX prbs setting should go through firmware. This should be just a reference when no firmware is available */
CredoError_t common_dsp_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        common_dsp_set_tx_prbs_pam4(slice, lane, enable, prbs_mode);
    } else {
        if (mode != CR_LMODE_NRZ) {
            LOGS_WARN("[GET TX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
        common_dsp_set_tx_prbs_nrz(slice, lane, enable, prbs_mode);
    }

    return CR_OK;
}

CredoError_t common_dsp_set_tx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    /* Programming order:
     * Enable: Pattern, PRBS clock enable, PRBS disable/enable, test pattern=PRBS, test pattern=1
     * Disable: test pattern=0, PRBS disable (all), PRBS clock disable
     */
    if (enable) {  // enable. Set pattern type first
        ERR_PROPS(common_dsp_set_tx_prbs_mode(slice, lane, prbs_mode));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, 0));

    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }

    return CR_OK;
}

CredoError_t common_dsp_set_tx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    /* Programming order:
     * Enable: Pattern, PRBS clock enable, PRBS disable/enable, test pattern=PRBS, test pattern=1
     * Disable: test pattern=0, PRBS disable (all), PRBS clock disable
     */
    if (enable) {  // enable. Set pattern type first
        ERR_PROPS(common_dsp_set_tx_prbs_mode(slice, lane, prbs_mode));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }

    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, enable));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_EN, 0));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, 0));

    if (enable) {
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_PATTERN, enable));
        ERR_PROPS(writeRegLane(slice, lane, REG_TX_TEST_PATT_EN, enable));
    }

    return CR_OK;
}

CredoError_t common_dsp_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    unsigned en = 0, patt = 0, patt_ext = 0, test_patt_sc = 0, test_patt_en = 0, prbs_clk_en = 0;
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));

    if (mode == CR_LMODE_PAM4) {
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN, &prbs_clk_en));
    } else {
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_EN_NRZ, &en));
        ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_GENERATOR_EN_NRZ, &prbs_clk_en));
        if (mode != CR_LMODE_NRZ) {
            LOGS_WARN("[GET TX PRBS][%d] Unknown mode %d, treat as NRZ", lane, mode);
        }
    }

    ERR_PROPS(readRegLane(slice, lane, REG_TX_PATTERN, &test_patt_sc));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_TEST_PATT_EN, &test_patt_en));

    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_MODE_SEL, &patt));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_MODE_SEL_EXT, &patt_ext));
    patt |= (patt_ext << 2);
    *prbs_mode = common_dsp_tx_prbs_type_hw_to_credo(patt, mode);

    *enable = en & test_patt_sc & test_patt_en & prbs_clk_en;
    return CR_OK;
}

CredoError_t common_dsp_get_rx_prbs_error_count(CredoSlice_t* slice, int lane, unsigned* count) {
    unsigned int cnt_lsb = 0, cnt_msb = 0;

    ERR_PROP(writeRegLane(slice, lane, REG_RX_READOUT_SYNC_EN, 0x1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_READOUT_CAPTURE, 0x1));
    ERR_PROP(readRegLane(slice, lane, REG_RX_READ_PRBS_SYNC_ERR_CNTR_LSB, &cnt_lsb));
    ERR_PROP(readRegLane(slice, lane, REG_RX_READ_PRBS_SYNC_ERR_CNTR_MSB, &cnt_msb));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_READOUT_CAPTURE, 0x0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_READOUT_SYNC_EN, 0x0));

    *count = (cnt_msb << 16) + cnt_lsb;
    return CR_OK;
}

CredoError_t common_dsp_rx_prbs_rst(CredoSlice_t* slice, int lane) {
    get_time(&slice->data->prbs_timer[lane]);  // reset prbs timer

    ERR_PROP(writeRegLane(slice, lane, REG_RX_RX_PRBS_AUTO_SYNC_EN, 1));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CNTR_RESET, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CNTR_RESET, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_RX_PRBS_SYNC_CNTR_RESET, 0));

    ERR_PROP(writeRegLane(slice, lane, REG_RX_RX_PRBS_AUTO_SYNC_EN, 0));
    return CR_OK;
}

CredoError_t common_dsp_prbs_get_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled) {
    unsigned auto_sync;
    ERR_PROP(readRegLane(slice, lane, REG_RX_RX_PRBS_AUTO_SYNC_EN, &auto_sync));
    *enabled = (auto_sync != 0);
    return CR_OK;
}

CredoError_t common_dsp_get_rx_prbs_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* lock) {
    int prbs_enable = 0;
    CredoLanePrbsPattern_t prbs_patt;
    ERR_PROPS(common_dsp_get_rx_prbs(slice, lane, &prbs_enable, &prbs_patt));

    // prbs not enabled so lock is invalid
    if (!prbs_enable) {
        *lock = CR_PRBS_LOCK_INVALID;
        return CR_OK;
    }
    unsigned prbs_lock = 0;
    ERR_PROP(readRegLane(slice, lane, REG_RX_PRBS_SYNC, &prbs_lock));
    *lock = prbs_lock ? CR_PRBS_LOCK_YES : CR_PRBS_LOCK_NO;
    return CR_OK;
}

CredoError_t common_dsp_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev) {
    if (prev > 4) return CR_INVALID_ARGS;
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PREV_EN, en));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_PREV, prev));
    return CR_OK;
}

CredoError_t common_dsp_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase) {
    if (phase > 8) return CR_INVALID_ARGS;
    uint8_t val = (phase == 0) ? 0 : ~(0x1 << (phase - 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_CHK_PHASE_EN, val));
    return CR_OK;
}

CredoError_t common_dsp_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]) {
    unsigned lsb = 0, msb = 0;
    for (int i = 0; i < 12; i++) {
        ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_PATTERN_LSB(i), &lsb));
        ERR_PROPS(readRegLane(slice, lane, REG_RX_PRBS_PATTERN_MSB(i), &msb));
        pattern_count[i] = (msb << 16) + lsb;
    }
    return CR_OK;
}

CredoError_t common_dsp_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_CNTR_RESET, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_PRBS_PATTERN_CNTR_RESET, 0));
    return CR_OK;
}

CredoError_t common_dsp_prbs_gen_1error(CredoSlice_t* slice, int lane) {
    CredoLaneMode_t mode;
    ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
    switch (mode) {
        case CR_LMODE_NRZ:
            ERR_PROPS(common_dsp_prbs_gen_1error_nrz(slice, lane));
            break;
        case CR_LMODE_PAM4:
            ERR_PROPS(common_dsp_prbs_gen_1error_pam4(slice, lane));
            break;
        case CR_LMODE_DISABLE:
            break;
        default:
            LOGS_WARN("[PRBS Generate 1 error] Unknown mode %d, skipped", mode);
            break;
    }
    return CR_OK;
}

CredoError_t common_dsp_prbs_gen_1error_nrz(CredoSlice_t* slice, int lane) {
    /* This will show 3 errors in counter. It's designed this way -- there is only 1 bit error. */
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 0));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 1));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR_NRZ, 0));
    return CR_OK;
}

CredoError_t common_dsp_prbs_gen_1error_pam4(CredoSlice_t* slice, int lane) {
    /* PAM4 has synced error counter. This will actually show 1 error. */
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 0));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 1));
    sleep_ms(1);
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_1B_ERR, 0));
    return CR_OK;
}
