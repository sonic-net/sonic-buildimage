/*
 * Swift PRBS control
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"
#include "swift/swift_serdes.h"

#include "utility.h"
#include "sdk.h"

#include <stdlib.h>

CredoError_t swift_get_rx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    ERR_PROP(common_dsp_get_rx_prbs(slice, lane, enable, prbs_mode));
    return CR_OK;
}

CredoError_t swift_set_rx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    ERR_PROP(common_dsp_set_rx_prbs(slice, lane, enable, prbs_mode));
    return CR_OK;
}

/* TX prbs setting should go through firmware. This should be just a reference when no firmware is available */
CredoError_t swift_set_tx_prbs(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    ERR_PROP(common_dsp_set_tx_prbs(slice, lane, enable, prbs_mode));
    return CR_OK;
}

CredoError_t swift_set_tx_prbs_checker(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    unsigned patt = common_dsp_rx_prbs_type_credo_to_hw(prbs_mode);  // based off 3-bit pattern selector (rx)
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_CHECKER_MODE_SEL, patt));
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_PRBS_CHECKER_EN, enable ? 1 : 0));
    if (enable) {
        ERR_PROPS(swift_tx_prbs_rst(slice, lane));
    }
    return CR_OK;
}

CredoError_t swift_get_tx_prbs_checker(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    *prbs_mode = CR_PRBS_UNKNOWN;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_CHECKER_EN, (unsigned*)enable));
    if (!*enable) {
        return CR_OK;
    }
    unsigned patt;
    ERR_PROPS(readRegLane(slice, lane, REG_TX_PRBS_CHECKER_MODE_SEL, &patt));
    *prbs_mode = common_dsp_rx_prbs_type_hw_to_credo(patt);  // based off 3-bit pattern selector
    return CR_OK;
}

CredoError_t swift_set_tx_prbs_pam4(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    ERR_PROP(common_dsp_set_tx_prbs_pam4(slice, lane, enable, prbs_mode));
    return CR_OK;
}

CredoError_t swift_set_tx_prbs_nrz(CredoSlice_t* slice, int lane, int enable, CredoLanePrbsPattern_t prbs_mode) {
    ERR_PROP(common_dsp_set_tx_prbs_nrz(slice, lane, enable, prbs_mode));
    return CR_OK;
}

CredoError_t swift_get_tx_prbs(CredoSlice_t* slice, int lane, int* enable, CredoLanePrbsPattern_t* prbs_mode) {
    ERR_PROP(common_dsp_get_tx_prbs(slice, lane, enable, prbs_mode));
    return CR_OK;
}

CredoError_t swift_get_rx_prbs_error_count(CredoSlice_t* slice, int lane, unsigned* count) {
    ERR_PROP(common_dsp_get_rx_prbs_error_count(slice, lane, count));
    return CR_OK;
}

CredoError_t swift_get_tx_prbs_error_count(CredoSlice_t* slice, int lane, unsigned count[2]) {
    unsigned int cnt_lsb = 0, cnt_msb = 0;

    ERR_PROP(readRegLane(slice, lane, REG_TX_PRBS_CHECKER_ERR_CNTR1_LSB, &cnt_lsb));
    ERR_PROP(readRegLane(slice, lane, REG_TX_PRBS_CHECKER_ERR_CNTR1_MSB, &cnt_msb));
    count[0] = ((cnt_msb << 16) + cnt_lsb) / 3;  // divide by 3 for correct counter
    ERR_PROP(readRegLane(slice, lane, REG_TX_PRBS_CHECKER_ERR_CNTR2_LSB, &cnt_lsb));
    ERR_PROP(readRegLane(slice, lane, REG_TX_PRBS_CHECKER_ERR_CNTR2_MSB, &cnt_msb));
    count[1] = ((cnt_msb << 16) + cnt_lsb) / 3;
    return CR_OK;
}

CredoError_t swift_rx_prbs_rst(CredoSlice_t* slice, int lane) {
    ERR_PROP(common_dsp_rx_prbs_rst(slice, lane));
    return CR_OK;
}

CredoError_t swift_tx_prbs_rst(CredoSlice_t* slice, int lane) {
    get_time(&slice->data->tx_prbs_timer[lane]);  // reset prbs timer
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PRBS_CHECKER_CNTR_RESET, 0));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PRBS_CHECKER_CNTR_RESET, 1));
    ERR_PROP(writeRegLane(slice, lane, REG_TX_PRBS_CHECKER_CNTR_RESET, 0));
    return CR_OK;
}

CredoError_t swift_prbs_get_rx_autosync(CredoSlice_t* slice, int lane, bool* enabled) {
    ERR_PROP(common_dsp_prbs_get_rx_autosync(slice, lane, enabled));
    return CR_OK;
}

CredoError_t swift_get_rx_prbs_lock(CredoSlice_t* slice, int lane, CredoPrbsLockStatus_t* lock) {
    ERR_PROP(common_dsp_get_rx_prbs_lock(slice, lane, lock));
    return CR_OK;
}

CredoError_t swift_set_rx_prbs_prev(CredoSlice_t* slice, int lane, bool en, unsigned prev) {
    ERR_PROP(common_dsp_set_rx_prbs_prev(slice, lane, en, prev));
    return CR_OK;
}

CredoError_t swift_set_rx_prbs_pattern_phase(CredoSlice_t* slice, int lane, unsigned phase) {
    ERR_PROP(common_dsp_set_rx_prbs_pattern_phase(slice, lane, phase));
    return CR_OK;
}

CredoError_t swift_get_rx_prbs_pattern_count(CredoSlice_t* slice, int lane, unsigned pattern_count[12]) {
    ERR_PROP(common_dsp_get_rx_prbs_pattern_count(slice, lane, pattern_count));
    return CR_OK;
}

CredoError_t swift_reset_rx_prbs_pattern_count(CredoSlice_t* slice, int lane) {
    ERR_PROP(common_dsp_reset_rx_prbs_pattern_count(slice, lane));
    return CR_OK;
}

CredoError_t swift_prbs_gen_1error(CredoSlice_t* slice, int lane) {
    ERR_PROP(common_dsp_prbs_gen_1error(slice, lane));
    return CR_OK;
}
