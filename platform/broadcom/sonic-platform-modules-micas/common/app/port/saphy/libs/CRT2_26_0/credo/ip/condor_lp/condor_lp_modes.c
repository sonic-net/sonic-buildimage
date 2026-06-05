/*
 * Condor LP running environments
 *
 * TX/RX polarity set/get
 * RX input mode (CR_COUPLING_AC/CR_COUPLING_DC)
 */
#include "project.h"

#include "condor_lp/condor_lp_regmap.h"
#include "condor_lp/condor_lp_serdes.h"

#include "sdk.h"

/* Modes */
/* Set TX polarity. The hardware TX polarity is reversed. */
CredoError_t condor_lp_set_tx_polarity(CredoSlice_t* slice, int lane, int tx_pol) {
    ERR_PROPS(writeRegLane(slice, lane, REG_TX_POL, !tx_pol));
    return CR_OK;
}

/* Get TX polarity. The hardware TX polarity is reversed. */
CredoError_t condor_lp_get_tx_polarity(CredoSlice_t* slice, int lane, int* tx_pol) {
    CredoError_t ret = CR_OK;
    unsigned int pol;

    ERR_PROPS(readRegLane(slice, lane, REG_TX_POL, &pol));
    *tx_pol = !pol;
    return ret;
}

CredoError_t condor_lp_set_rx_polarity(CredoSlice_t* slice, int lane, int rx_pol) {
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_REG_PRBS_FLIP, rx_pol));
    ERR_PROPS(writeRegLane(slice, lane, REG_RX_DATA_CUS_FLIP, rx_pol));
    return CR_OK;
}

CredoError_t condor_lp_set_rx_polarity_nrz(CredoSlice_t* slice, int lane, int rx_pol) {
    return writeRegLane(slice, lane, REG_RX_REG_PRBS_FLIP, rx_pol);
}

CredoError_t condor_lp_set_rx_polarity_pam4(CredoSlice_t* slice, int lane, int rx_pol) {
    return writeRegLane(slice, lane, REG_RX_DATA_CUS_FLIP, rx_pol);
}

CredoError_t condor_lp_get_rx_polarity(CredoSlice_t* slice, int lane, int* rx_pol) {
    int pam4_pol = 0, nrz_pol = 0;
    ERR_PROPS(condor_lp_get_rx_polarity_pam4(slice, lane, &pam4_pol));
    ERR_PROPS(condor_lp_get_rx_polarity_nrz(slice, lane, &nrz_pol));

    if (pam4_pol == nrz_pol) {
        *rx_pol = pam4_pol;
    } else {
        LOGS_WARN("[Get RX polarity] PAM4/NRZ polarity dismatch, return current lane mode polarity");
        CredoLaneMode_t mode = CR_LMODE_OFF;
        ERR_PROPS(hal_get_lane_mode(slice, lane, &mode));
        if (mode == CR_LMODE_PAM4 || mode == CR_LMODE_NRZ) {
            *rx_pol = (mode == CR_LMODE_PAM4) ? pam4_pol : nrz_pol;
        } else if (mode == CR_LMODE_DISABLE) {
            *rx_pol = nrz_pol;
        } else {
            LOGS_WARN("[Get RX polarity] Unknown mode %d, return NRZ polarity", mode);
            *rx_pol = nrz_pol;
        }
    }

    return CR_OK;
}

CredoError_t condor_lp_get_rx_polarity_nrz(CredoSlice_t* slice, int lane, int* rx_pol) {
    return readRegLane(slice, lane, REG_RX_REG_PRBS_FLIP, (unsigned*)rx_pol);
}

CredoError_t condor_lp_get_rx_polarity_pam4(CredoSlice_t* slice, int lane, int* rx_pol) {
    return readRegLane(slice, lane, REG_RX_DATA_CUS_FLIP, (unsigned*)rx_pol);
}

CredoError_t condor_lp_set_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t input_mode) {
    if (input_mode == CR_COUPLING_AC) {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_EN_VCOMINBUF, 1));
    } else {
        ERR_PROPS(writeRegLane(slice, lane, REG_RX_EN_VCOMINBUF, 0));
    }
    return CR_OK;
}

CredoError_t condor_lp_get_rx_input_mode(CredoSlice_t* slice, int lane, CredoLaneCoupling_t* input_mode) {
    ERR_PROPS(readRegLane(slice, lane, REG_RX_EN_VCOMINBUF, input_mode));
    return CR_OK;
}
