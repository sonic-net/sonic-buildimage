#include "screaming_eagle.h"
#include "se_device.h"
#include "se_functions.h"

#include "common/common_firmware.h"
#include "common/common_reset.h"
#include "swift/swift_serdes.h"

#include "stringify.h"
#include "utility.h"
#include "sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REFCLK_HZ (0.15625 * 1e9)  // TODO: change to support custom refclocks once firmware supports it

double speed_map[] = {1.25e9,   10.3125e9, 20.625e9, 25.78125e9, 26.5625e9, 51.5625e9,
                      53.125e9, 56.25e9,   90e9,     98.2e9,     106.25e9,  112e9};

Speed_t speed_config[] = {CONFIG_1P25G, CONFIG_10G, CONFIG_20G, CONFIG_25G, CONFIG_26G,  CONFIG_51G,
                          CONFIG_53G,   CONFIG_56G, CONFIG_90G, CONFIG_98G, CONFIG_106G, CONFIG_112G};

flexspeed_config_t se_flexspeed_compute_config(double datarate, CredoLaneMode_t mode) {
    flexspeed_config_t config;
    double baudrate = (mode == CR_LMODE_PAM4) ? datarate / 2 : datarate;
    config.baudrate = baudrate;
    if (baudrate < 1.875e9) {
        config.vcorate = baudrate * 16;
    } else if (baudrate < 3.75e9) {
        config.vcorate = baudrate * 8;
    } else if (baudrate < 7.5e9) {
        config.vcorate = baudrate * 4;
    } else if (baudrate < 15e9) {
        config.vcorate = baudrate * 2;
    } else if (baudrate < 30e9) {
        config.vcorate = baudrate;
    } else {
        config.vcorate = baudrate / 2;
    }
    // find nearest speed for configuration
    Speed_t fw_speed = speed_config[0];
    double fw_full_speed = speed_map[0];
    for (size_t i = 0; i < COUNT_OF(speed_map); i++) {
        if (fabs(speed_map[i] - datarate) < fabs(fw_full_speed - datarate)) {
            fw_speed = speed_config[i];
            fw_full_speed = speed_map[i];
        }
    }
    config.fw_base_speed = fw_speed;

    double pll_n_full = (config.vcorate / (REFCLK_HZ * 4));
    double tmp;
    config.pll_n = (unsigned)(config.vcorate / (REFCLK_HZ * 4));
    config.pll_n_frac = (unsigned)((modf(pll_n_full, &tmp) * (1 << 16)) + 0.5);
    config.tx_target_count = (unsigned)(65535 * baudrate / (32 * REFCLK_HZ));
    config.rx_target_count = config.tx_target_count / 2;
    return config;
}

double se_flexspeed_compute_speed(double base_datarate, unsigned pll_n, unsigned pll_n_frac) {
    double vcorate = 4 * REFCLK_HZ * pll_n;
    vcorate += 4 * REFCLK_HZ * ((double)pll_n_frac) / (1 << 16);

    // compute if any divisor or multiplier
    if ((unsigned)(base_datarate / vcorate) != 0) {
        unsigned multiplier = (unsigned)((base_datarate / vcorate) + 0.5);
        vcorate *= multiplier;
    } else {
        unsigned divisor = (unsigned)((vcorate / base_datarate) + 0.5);
        vcorate /= divisor;
    }

    vcorate = round(vcorate / 1e3) * 1000;
    return vcorate;
}

CredoError_t se_flexspeed_set_registers(CredoSlice_t* slice, CredoSide_t side, const flexspeed_config_t* config,
                                        bool en_tx, bool en_rx) {
    if (side == CR_SIDE_HOST) {
        if (en_tx) {
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_TX_A, config->pll_n));
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_TX_FRAC_A, config->pll_n_frac));
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_TX_TARGET_LSB_A,
                                             config->tx_target_count & 0xFFFF));
            ERR_PROPS(
                hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_TX_TARGET_MSB_A, config->tx_target_count >> 16));
        }
        if (en_rx) {
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_RX_A, config->pll_n));
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_RX_FRAC_A, config->pll_n_frac));

            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_RX_TARGET_LSB_A,
                                             config->rx_target_count & 0xFFFF));
            ERR_PROPS(
                hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_RX_TARGET_MSB_A, config->rx_target_count >> 16));
        }

    } else {
        if (en_tx) {
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_TX_B, config->pll_n));
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_TX_FRAC_B, config->pll_n_frac));
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_TX_TARGET_LSB_B,
                                             config->tx_target_count & 0xFFFF));
            ERR_PROPS(
                hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_TX_TARGET_MSB_B, config->tx_target_count >> 16));
        }
        if (en_rx) {
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_RX_B, config->pll_n));
            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_N_RX_FRAC_B, config->pll_n_frac));

            ERR_PROPS(hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_RX_TARGET_LSB_B,
                                             config->rx_target_count & 0xFFFF));
            ERR_PROPS(
                hal_fw_reg_wr_internal(slice, FWREG_TOP_FLEXSPEED_PLL_RX_TARGET_MSB_B, config->rx_target_count >> 16));
        }
    }
    return CR_OK;
}
