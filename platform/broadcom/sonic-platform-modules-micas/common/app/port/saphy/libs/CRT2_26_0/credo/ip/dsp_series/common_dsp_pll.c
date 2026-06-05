/*
 * Common DSP PLL cap codes
 */
#include "project.h"

#include "dsp_series/common_dsp_functions.h"

#include "sdk.h"

#include <math.h>

CredoError_t common_dsp_set_rx_cap(CredoSlice_t* slice, int lane, int rx_cap) {
    return writeRegLane(slice, lane, REG_RX_LCVCOCAP, rx_cap);
}

CredoError_t common_dsp_get_rx_cap(CredoSlice_t* slice, int lane, int* rx_cap) {
    return readRegLane(slice, lane, REG_RX_LCVCOCAP, (unsigned*)rx_cap);
}

CredoError_t common_dsp_set_tx_cap(CredoSlice_t* slice, int lane, int tx_cap) {
    return writeRegLane(slice, lane, REG_TX_PLL_LVCOCAP, tx_cap);
}

CredoError_t common_dsp_get_tx_cap(CredoSlice_t* slice, int lane, int* tx_cap) {
    return readRegLane(slice, lane, REG_TX_PLL_LVCOCAP, (unsigned*)tx_cap);
}

// XXX: formula maybe different each IP
CredoError_t common_dsp_get_lane_speed(CredoSlice_t* slice, int lane, uint32_t* speed_kbps) {
    double refclk = 0.0, fix_div2 = 0.5;
    // int refclk_hz = 0;
    // refclk = (double)refclk_hz / 1000000;
    refclk = 195.3125f;

    unsigned pam4_en = 0, reg_pam4_en = 0, nrz_mode = 0, mode10g_en;
    ERR_PROPS(readRegLane(slice, lane, REG_PAM4_EN_TX, &reg_pam4_en));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_NRZ_MODE, &nrz_mode));
    ERR_PROPS(readRegLane(slice, lane, REG_TX_NRZ_10G_MODE_EN, &mode10g_en));
    pam4_en = (nrz_mode == 0 && reg_pam4_en == 1) ? 1 : 0;

    double data_rate_to_fvco_ratio = 0;
    if (pam4_en) {
        data_rate_to_fvco_ratio = (mode10g_en) ? 1.0 : 2.0;
    } else {
        data_rate_to_fvco_ratio = (mode10g_en) ? 0.5 : 1.0;
    }

    unsigned rx_pll_n = 0, rx_pll_n_en = 0, rx_pll_frac_n = 0, rx_div2_bypass = 0, speed_mode_sel = 0;
    ERR_PROPS(readRegLane(slice, lane, REG_RX_PLL_N, &rx_pll_n));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_SDM_EN, &rx_pll_n_en));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_REG_PLL_N_FRAC, &rx_pll_frac_n));
    ERR_PROPS(readRegLane(slice, lane, REG_RX_BYPASS_DIV2PI, &rx_div2_bypass));
    ERR_PROPS(readRegLane(slice, lane, REG_SPEED_MODE_SEL, &speed_mode_sel));

    unsigned rx_mul_by_2 = (rx_div2_bypass) ? 1 : 2;
    double pll_n = (rx_pll_n_en) ? rx_pll_n + ((double)rx_pll_frac_n / 65535.0) : rx_pll_n;

    double rx_fvco = (refclk * pll_n * 2 * rx_mul_by_2) / 1000.0;

    // LOGS_DEBUG("rx_fvco: %.4lf, pll_n: %.4lf, rx_pll_n: %u, rx_div2_bypass: %u, rx_pll_frac_en: %u, rx_pll_frac_n:
    // %u", rx_fvco, pll_n, rx_pll_n, rx_div2_bypass, rx_pll_n_en, rx_pll_frac_n); LOGS_DEBUG("data_rate_to_fvco_ratio:
    // %.3lf, speed_mode_sel: %u", data_rate_to_fvco_ratio, speed_mode_sel);
    *speed_kbps = (rx_fvco * data_rate_to_fvco_ratio / pow(2, speed_mode_sel) / fix_div2) * 1000000;
    return CR_OK;
}
