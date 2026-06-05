#ifndef SWIFT_REGMAP_H
#define SWIFT_REGMAP_H

#include "dsp_series/common_dsp_regmap.h"

#include "sdk.h"

/* tx_56g_digital_top */
#define REG_TX_TAPS_SUM_LIMIT 63.5  // TODO, need check
#define REG_TX_TAP1           REGBITR(TX, 0x0A6, 4, 0)
#define REG_TX_TAP2           REGBITR(TX, 0x0A8, 5, 0)
#define REG_TX_TAP3           REGBITR(TX, 0x0AA, 6, 0)
#define REG_TX_TAP4           REGBITR(TX, 0x0AB, 14, 8)
#define REG_TX_TAP5           REGBITR(TX, 0x0B1, 6, 0)
#define REG_TX_TAP6           REGBITR(TX, 0x0B5, 5, 0)
#define REG_TX_TAP7           REGBITR(TX, 0x0B7, 5, 0)
#define REG_TX_TAP8           REGBITR(TX, 0x0B8, 15, 12)
#define REG_TX_TAP9           REGBITR(TX, 0x0B8, 11, 8)
#define REG_TX_TAP10          REGBITR(TX, 0x0B8, 7, 4)
#define REG_TX_TAP11          REGBITR(TX, 0x0B8, 3, 0)

/* ANA_TOP_REG */
#define REG_RX_EN_SKC    REGBITR(ANA_TOP, 0x0D8, 8)
#define REG_RX_CAP_SKC   REGBITR(ANA_TOP, 0x0BE, 3, 1)
#define REG_RX_SKC_DEGEN REGBITR(ANA_TOP, 0x0B4, 3, 0)

#define REG_RX_REF_CTL REGBITR(ANA_TOP, 0x0D1, 3, 0)
#define REG_RX_ADJ_CS2 REGBITR(ANA_TOP, 0x0B9, 15, 12)
#define REG_RX_ADJ_CS1 REGBITR(ANA_TOP, 0x0B9, 11, 8)

/* reg_quail_one_lane */
// TX PRBS checker
#define REG_TX_PRBS_CHECKER_EN            REGBITR(ONELANE, 0x43, 3)
#define REG_TX_PRBS_CHECKER_MODE_SEL      REGBITR(ONELANE, 0x43, 6, 4)
#define REG_TX_PRBS_CHECKER_CNTR_RESET    REGBITR(ONELANE, 0x43, 7)
#define REG_TX_PRBS_CHECKER_ERR_CNTR1_MSB REGBITR(ONELANE, 0x44, 15, 0)
#define REG_TX_PRBS_CHECKER_ERR_CNTR1_LSB REGBITR(ONELANE, 0x45, 15, 0)
#define REG_TX_PRBS_CHECKER_ERR_CNTR2_MSB REGBITR(ONELANE, 0x46, 15, 0)
#define REG_TX_PRBS_CHECKER_ERR_CNTR2_LSB REGBITR(ONELANE, 0x47, 15, 0)

#endif
