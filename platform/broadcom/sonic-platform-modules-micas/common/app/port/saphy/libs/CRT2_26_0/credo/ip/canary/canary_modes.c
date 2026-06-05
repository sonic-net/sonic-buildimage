/*
 * Canary running environments
 *
 * TX/RX polarity set/get
 * RX input mode (CR_COUPLING_AC/CR_COUPLING_DC)
 */
#include "project.h"

#include "sdk.h"

/* Modes */

CredoError_t canary_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    ERR_PROPS(writeRegLane(slice, lane, REG_EAGLE_RX_POL, rx_pol));
    ERR_PROPS(writeRegLane(slice, lane, REG_FLR_RX_POL, rx_pol));
    return CR_OK;
}
/* Set TX polarity. The hardware TX polarity is reversed. */
CredoError_t canary_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_POL, !tx_pol));
    return CR_OK;
}

CredoError_t canary_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    unsigned pam4_pol = 0, nrz_pol = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_FLR_RX_POL, &pam4_pol));
    ERR_PROPS(readRegLane(slice, lane, REG_EAGLE_RX_POL, &nrz_pol));

    if (pam4_pol == nrz_pol) {
        *rx_pol = pam4_pol;
    } else {
        LOGS_WARN("[Get RX polarity] PAM4/NRZ polarity dismatch, return current lane mode polarity");
        CredoLaneMode_t mode = CR_LMODE_OFF;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
        if (mode == CR_LMODE_PAM4 || mode == CR_LMODE_NRZ) {
            *rx_pol = (mode == CR_LMODE_PAM4) ? pam4_pol : nrz_pol;
        } else {
            LOGS_WARN("[Get RX polarity] Unknown mode %d, return NRZ polarity", mode);
            *rx_pol = nrz_pol;
        }
    }

    return CR_OK;
}

/* Get TX polarity. The hardware TX polarity is reversed. */
CredoError_t canary_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    CredoError_t ret = CR_OK;
    unsigned int pol;

    ERR_PROPS(readRegLane(slice, lane, REG_TX_POL, &pol));
    *tx_pol = !pol;
    return ret;
}

CredoError_t canary_set_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_EN_VCOMINBUF, 1));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_VCOMREFSEL_EX, (input_mode == CR_COUPLING_DC) ? 1 : 0));
    return CR_OK;
}

CredoError_t canary_get_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode) {
    unsigned comrefsel;

    ERR_PROPS(readRegLane(slice, lane, REG_RX_VCOMREFSEL_EX, &comrefsel));

    *input_mode = (comrefsel == 1) ? CR_COUPLING_DC : CR_COUPLING_AC;
    return CR_OK;
}
