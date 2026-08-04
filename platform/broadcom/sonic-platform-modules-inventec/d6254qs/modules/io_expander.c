/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/i2c.h>
#include "io_expander.h"

static struct ioexp_obj_s *ioexp_head_p = NULL;
static struct ioexp_obj_s *ioexp_tail_p = NULL;


/* ========== Register IOEXP layout ==========
 */
struct ioexp_map_s ioexp_map_magnolia_nab = {

    .chip_amount = 2,
    .data_width  = 2,

    .map_present    = { {0, 0, 4}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {0, 0, 5}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {0, 0, 6}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {0, 0, 7}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {1, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {1, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {1, 0, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {1, 0, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_tx_disable = { {0, 1, 0}, /* map_tx_disable[0] = TXDISABLE_SFP+_P(X)   */
                        {0, 1, 1}, /* map_tx_disable[1] = TXDISABLE_SFP+_P(X+1) */
                        {0, 1, 2}, /* map_tx_disable[2] = TXDISABLE_SFP+_P(X+2) */
                        {0, 1, 3}, /* map_tx_disable[3] = TXDISABLE_SFP+_P(X+3) */
                        {1, 1, 0}, /* map_tx_disable[4] = TXDISABLE_SFP+_P(X+4) */
                        {1, 1, 1}, /* map_tx_disable[5] = TXDISABLE_SFP+_P(X+5) */
                        {1, 1, 2}, /* map_tx_disable[6] = TXDISABLE_SFP+_P(X+6) */
                        {1, 1, 3}, /* map_tx_disable[7] = TXDISABLE_SFP+_P(X+7) */
    },
    .map_tx_fault   = { {0, 0, 0}, /* map_tx_fault[0] = TXFAULT_SFP+_P(X)   */
                        {0, 0, 1}, /* map_tx_fault[1] = TXFAULT_SFP+_P(X+1) */
                        {0, 0, 2}, /* map_tx_fault[2] = TXFAULT_SFP+_P(X+2) */
                        {0, 0, 3}, /* map_tx_fault[3] = TXFAULT_SFP+_P(X+3) */
                        {1, 0, 0}, /* map_tx_fault[4] = TXFAULT_SFP+_P(X+4) */
                        {1, 0, 1}, /* map_tx_fault[5] = TXFAULT_SFP+_P(X+5) */
                        {1, 0, 2}, /* map_tx_fault[6] = TXFAULT_SFP+_P(X+6) */
                        {1, 0, 3}, /* map_tx_fault[7] = TXFAULT_SFP+_P(X+7) */
    },
    .map_rxlos      = { {0, 1, 4}, /* map_rxlos[0] = OPRXLOS_PORT(X)   */
                        {0, 1, 5}, /* map_rxlos[1] = OPRXLOS_PORT(X+1) */
                        {0, 1, 6}, /* map_rxlos[2] = OPRXLOS_PORT(X+2) */
                        {0, 1, 7}, /* map_rxlos[3] = OPRXLOS_PORT(X+3) */
                        {1, 1, 4}, /* map_rxlos[4] = OPRXLOS_PORT(X+4) */
                        {1, 1, 5}, /* map_rxlos[5] = OPRXLOS_PORT(X+5) */
                        {1, 1, 6}, /* map_rxlos[6] = OPRXLOS_PORT(X+6) */
                        {1, 1, 7}, /* map_rxlos[7] = OPRXLOS_PORT(X+7) */
    },
};


struct ioexp_map_s ioexp_map_magnolia_4ab = {

    .chip_amount = 2,
    .data_width  = 2,

    .map_present    = { {0, 0, 4}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {0, 0, 5}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {0, 0, 6}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {0, 0, 7}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {1, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {1, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {1, 0, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {1, 0, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_tx_disable = { {0, 1, 0}, /* map_tx_disable[0] = TXDISABLE_SFP+_P(X)   */
                        {0, 1, 1}, /* map_tx_disable[1] = TXDISABLE_SFP+_P(X+1) */
                        {0, 1, 2}, /* map_tx_disable[2] = TXDISABLE_SFP+_P(X+2) */
                        {0, 1, 3}, /* map_tx_disable[3] = TXDISABLE_SFP+_P(X+3) */
                        {1, 0, 0}, /* map_tx_disable[4] = TXDISABLE_SFP+_P(X+4) */
                        {1, 0, 1}, /* map_tx_disable[5] = TXDISABLE_SFP+_P(X+5) */
                        {1, 0, 2}, /* map_tx_disable[6] = TXDISABLE_SFP+_P(X+6) */
                        {1, 0, 3}, /* map_tx_disable[7] = TXDISABLE_SFP+_P(X+7) */
    },
    .map_tx_fault   = { {0, 0, 0}, /* map_tx_fault[0] = TXFAULT_SFP+_P(X)   */
                        {0, 0, 1}, /* map_tx_fault[1] = TXFAULT_SFP+_P(X+1) */
                        {0, 0, 2}, /* map_tx_fault[2] = TXFAULT_SFP+_P(X+2) */
                        {0, 0, 3}, /* map_tx_fault[3] = TXFAULT_SFP+_P(X+3) */
                        {1, 1, 0}, /* map_tx_fault[4] = TXFAULT_SFP+_P(X+4) */
                        {1, 1, 1}, /* map_tx_fault[5] = TXFAULT_SFP+_P(X+5) */
                        {1, 1, 2}, /* map_tx_fault[6] = TXFAULT_SFP+_P(X+6) */
                        {1, 1, 3}, /* map_tx_fault[7] = TXFAULT_SFP+_P(X+7) */
    },
    .map_rxlos      = { {0, 1, 4}, /* map_rxlos[0] = OPRXLOS_PORT(X)   */
                        {0, 1, 5}, /* map_rxlos[1] = OPRXLOS_PORT(X+1) */
                        {0, 1, 6}, /* map_rxlos[2] = OPRXLOS_PORT(X+2) */
                        {0, 1, 7}, /* map_rxlos[3] = OPRXLOS_PORT(X+3) */
                        {1, 1, 4}, /* map_rxlos[4] = OPRXLOS_PORT(X+4) */
                        {1, 1, 5}, /* map_rxlos[5] = OPRXLOS_PORT(X+5) */
                        {1, 1, 6}, /* map_rxlos[6] = OPRXLOS_PORT(X+6) */
                        {1, 1, 7}, /* map_rxlos[7] = OPRXLOS_PORT(X+7) */
    },
};


struct ioexp_map_s ioexp_map_magnolia_7ab = {

    .chip_amount    = 2,
    .data_width     = 2,

    .map_present    = { {1, 0, 4}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {1, 0, 5}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {1, 0, 6}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {1, 0, 7}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {1, 1, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {1, 1, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP_N_P(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP_N_P(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP_N_P(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP_N_P(X+3) */
                        {1, 0, 0}, /* map_reset[4] = QRESET_QSFP_N_P(X+4) */
                        {1, 0, 1}, /* map_reset[5] = QRESET_QSFP_N_P(X+5) */
    },
    .map_lpmod      = { {0, 0, 4}, /* map_lpmod[0] = LPMODE_QSFP_P(X)   */
                        {0, 0, 5}, /* map_lpmod[1] = LPMODE_QSFP_P(X+1) */
                        {0, 0, 6}, /* map_lpmod[2] = LPMODE_QSFP_P(X+2) */
                        {0, 0, 7}, /* map_lpmod[3] = LPMODE_QSFP_P(X+3) */
                        {1, 0, 2}, /* map_lpmod[4] = LPMODE_QSFP_P(X+4) */
                        {1, 0, 3}, /* map_lpmod[5] = LPMODE_QSFP_P(X+5) */
    },
    .map_modsel     = { {0, 1, 4}, /* map_modsel[0] = MODSEL_QSFP_N_P(X)   */
                        {0, 1, 5}, /* map_modsel[1] = MODSEL_QSFP_N_P(X+1) */
                        {0, 1, 6}, /* map_modsel[2] = MODSEL_QSFP_N_P(X+2) */
                        {0, 1, 7}, /* map_modsel[3] = MODSEL_QSFP_N_P(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP_N_P(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP_N_P(X+5) */
    },
};


struct ioexp_map_s ioexp_map_redwood_p01p08_p17p24 = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 0, 0}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {2, 0, 1}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {2, 0, 2}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {2, 0, 3}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {2, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {2, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {2, 0, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {2, 0, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP28_N_P(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP28_N_P(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP28_N_P(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP28_N_P(X+3) */
                        {1, 0, 0}, /* map_reset[4] = QRESET_QSFP28_N_P(X+4) */
                        {1, 0, 1}, /* map_reset[5] = QRESET_QSFP28_N_P(X+5) */
                        {1, 0, 2}, /* map_reset[6] = QRESET_QSFP28_N_P(X+6) */
                        {1, 0, 3}, /* map_reset[7] = QRESET_QSFP28_N_P(X+7) */
    },
    .map_lpmod      = { {0, 0, 4}, /* map_lpmod[0] = LPMODE_QSFP28_P(X)   */
                        {0, 0, 5}, /* map_lpmod[1] = LPMODE_QSFP28_P(X+1) */
                        {0, 0, 6}, /* map_lpmod[2] = LPMODE_QSFP28_P(X+2) */
                        {0, 0, 7}, /* map_lpmod[3] = LPMODE_QSFP28_P(X+3) */
                        {1, 0, 4}, /* map_lpmod[4] = LPMODE_QSFP28_P(X+4) */
                        {1, 0, 5}, /* map_lpmod[5] = LPMODE_QSFP28_P(X+5) */
                        {1, 0, 6}, /* map_lpmod[6] = LPMODE_QSFP28_P(X+6) */
                        {1, 0, 7}, /* map_lpmod[7] = LPMODE_QSFP28_P(X+7) */
    },
    .map_modsel     = { {0, 1, 4}, /* map_modsel[0] = MODSEL_QSFP28_N_P(X)   */
                        {0, 1, 5}, /* map_modsel[1] = MODSEL_QSFP28_N_P(X+1) */
                        {0, 1, 6}, /* map_modsel[2] = MODSEL_QSFP28_N_P(X+2) */
                        {0, 1, 7}, /* map_modsel[3] = MODSEL_QSFP28_N_P(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP28_N_P(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP28_N_P(X+5) */
                        {1, 1, 6}, /* map_modsel[6] = MODSEL_QSFP28_N_P(X+6) */
                        {1, 1, 7}, /* map_modsel[7] = MODSEL_QSFP28_N_P(X+7) */
    },
};


struct ioexp_map_s ioexp_map_redwood_p09p16_p25p32 = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 1, 0}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {2, 1, 1}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {2, 1, 2}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {2, 1, 3}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {2, 1, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {2, 1, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {2, 1, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {2, 1, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP28_N_P(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP28_N_P(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP28_N_P(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP28_N_P(X+3) */
                        {1, 0, 0}, /* map_reset[4] = QRESET_QSFP28_N_P(X+4) */
                        {1, 0, 1}, /* map_reset[5] = QRESET_QSFP28_N_P(X+5) */
                        {1, 0, 2}, /* map_reset[6] = QRESET_QSFP28_N_P(X+6) */
                        {1, 0, 3}, /* map_reset[7] = QRESET_QSFP28_N_P(X+7) */
    },
    .map_lpmod      = { {0, 0, 4}, /* map_lpmod[0] = LPMODE_QSFP28_P(X)   */
                        {0, 0, 5}, /* map_lpmod[1] = LPMODE_QSFP28_P(X+1) */
                        {0, 0, 6}, /* map_lpmod[2] = LPMODE_QSFP28_P(X+2) */
                        {0, 0, 7}, /* map_lpmod[3] = LPMODE_QSFP28_P(X+3) */
                        {1, 0, 4}, /* map_lpmod[4] = LPMODE_QSFP28_P(X+4) */
                        {1, 0, 5}, /* map_lpmod[5] = LPMODE_QSFP28_P(X+5) */
                        {1, 0, 6}, /* map_lpmod[6] = LPMODE_QSFP28_P(X+6) */
                        {1, 0, 7}, /* map_lpmod[7] = LPMODE_QSFP28_P(X+7) */
    },
    .map_modsel     = { {0, 1, 4}, /* map_modsel[0] = MODSEL_QSFP28_N_P(X)   */
                        {0, 1, 5}, /* map_modsel[1] = MODSEL_QSFP28_N_P(X+1) */
                        {0, 1, 6}, /* map_modsel[2] = MODSEL_QSFP28_N_P(X+2) */
                        {0, 1, 7}, /* map_modsel[3] = MODSEL_QSFP28_N_P(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP28_N_P(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP28_N_P(X+5) */
                        {1, 1, 6}, /* map_modsel[6] = MODSEL_QSFP28_N_P(X+6) */
                        {1, 1, 7}, /* map_modsel[7] = MODSEL_QSFP28_N_P(X+7) */
    },
};


struct ioexp_map_s ioexp_map_hudson32iga_p01p08_p17p24 = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 0, 0}, /* map_present[0] = MODABS_QSFP(X)   */
                        {2, 0, 1}, /* map_present[1] = MODABS_QSFP(X+1) */
                        {2, 0, 2}, /* map_present[2] = MODABS_QSFP(X+2) */
                        {2, 0, 3}, /* map_present[3] = MODABS_QSFP(X+3) */
                        {2, 0, 4}, /* map_present[4] = MODABS_QSFP(X+4) */
                        {2, 0, 5}, /* map_present[5] = MODABS_QSFP(X+5) */
                        {2, 0, 6}, /* map_present[6] = MODABS_QSFP(X+6) */
                        {2, 0, 7}, /* map_present[7] = MODABS_QSFP(X+7) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP(X+3) */
                        {1, 0, 0}, /* map_reset[4] = QRESET_QSFP(X+4) */
                        {1, 0, 1}, /* map_reset[5] = QRESET_QSFP(X+5) */
                        {1, 0, 2}, /* map_reset[6] = QRESET_QSFP(X+6) */
                        {1, 0, 3}, /* map_reset[7] = QRESET_QSFP(X+7) */
    },
    .map_lpmod      = { {0, 0, 4}, /* map_lpmod[0] = LPMODE_QSFP(X)   */
                        {0, 0, 5}, /* map_lpmod[1] = LPMODE_QSFP(X+1) */
                        {0, 0, 6}, /* map_lpmod[2] = LPMODE_QSFP(X+2) */
                        {0, 0, 7}, /* map_lpmod[3] = LPMODE_QSFP(X+3) */
                        {1, 0, 4}, /* map_lpmod[4] = LPMODE_QSFP(X+4) */
                        {1, 0, 5}, /* map_lpmod[5] = LPMODE_QSFP(X+5) */
                        {1, 0, 6}, /* map_lpmod[6] = LPMODE_QSFP(X+6) */
                        {1, 0, 7}, /* map_lpmod[7] = LPMODE_QSFP(X+7) */
    },
    .map_modsel     = { {0, 1, 4}, /* map_modsel[0] = MODSEL_QSFP(X)   */
                        {0, 1, 5}, /* map_modsel[1] = MODSEL_QSFP(X+1) */
                        {0, 1, 6}, /* map_modsel[2] = MODSEL_QSFP(X+2) */
                        {0, 1, 7}, /* map_modsel[3] = MODSEL_QSFP(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP(X+5) */
                        {1, 1, 6}, /* map_modsel[6] = MODSEL_QSFP(X+6) */
                        {1, 1, 7}, /* map_modsel[7] = MODSEL_QSFP(X+7) */
    },
};


struct ioexp_map_s ioexp_map_hudson32iga_p09p16_p25p32 = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 1, 0}, /* map_present[0] = MODABS_QSFP(X)   */
                        {2, 1, 1}, /* map_present[1] = MODABS_QSFP(X+1) */
                        {2, 1, 2}, /* map_present[2] = MODABS_QSFP(X+2) */
                        {2, 1, 3}, /* map_present[3] = MODABS_QSFP(X+3) */
                        {2, 1, 4}, /* map_present[4] = MODABS_QSFP(X+4) */
                        {2, 1, 5}, /* map_present[5] = MODABS_QSFP(X+5) */
                        {2, 1, 6}, /* map_present[6] = MODABS_QSFP(X+6) */
                        {2, 1, 7}, /* map_present[7] = MODABS_QSFP(X+7) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP(X+3) */
                        {1, 0, 0}, /* map_reset[4] = QRESET_QSFP(X+4) */
                        {1, 0, 1}, /* map_reset[5] = QRESET_QSFP(X+5) */
                        {1, 0, 2}, /* map_reset[6] = QRESET_QSFP(X+6) */
                        {1, 0, 3}, /* map_reset[7] = QRESET_QSFP(X+7) */
    },
    .map_lpmod      = { {0, 0, 4}, /* map_lpmod[0] = LPMODE_QSFP(X)   */
                        {0, 0, 5}, /* map_lpmod[1] = LPMODE_QSFP(X+1) */
                        {0, 0, 6}, /* map_lpmod[2] = LPMODE_QSFP(X+2) */
                        {0, 0, 7}, /* map_lpmod[3] = LPMODE_QSFP(X+3) */
                        {1, 0, 4}, /* map_lpmod[4] = LPMODE_QSFP(X+4) */
                        {1, 0, 5}, /* map_lpmod[5] = LPMODE_QSFP(X+5) */
                        {1, 0, 6}, /* map_lpmod[6] = LPMODE_QSFP(X+6) */
                        {1, 0, 7}, /* map_lpmod[7] = LPMODE_QSFP(X+7) */
    },
    .map_modsel     = { {0, 1, 4}, /* map_modsel[0] = MODSEL_QSFP(X)   */
                        {0, 1, 5}, /* map_modsel[1] = MODSEL_QSFP(X+1) */
                        {0, 1, 6}, /* map_modsel[2] = MODSEL_QSFP(X+2) */
                        {0, 1, 7}, /* map_modsel[3] = MODSEL_QSFP(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP(X+5) */
                        {1, 1, 6}, /* map_modsel[6] = MODSEL_QSFP(X+6) */
                        {1, 1, 7}, /* map_modsel[7] = MODSEL_QSFP(X+7) */
    },
};


struct ioexp_map_s ioexp_map_cypress_nabc = {

    .chip_amount = 3,
    .data_width  = 2,

    .map_present    = { {0, 0, 4}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {0, 0, 5}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {0, 0, 6}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {0, 0, 7}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {1, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {1, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {1, 0, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {1, 0, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_tx_disable = { {0, 1, 0}, /* map_tx_disable[0] = TXDISABLE_SFP+_P(X)   */
                        {0, 1, 1}, /* map_tx_disable[1] = TXDISABLE_SFP+_P(X+1) */
                        {0, 1, 2}, /* map_tx_disable[2] = TXDISABLE_SFP+_P(X+2) */
                        {0, 1, 3}, /* map_tx_disable[3] = TXDISABLE_SFP+_P(X+3) */
                        {1, 1, 0}, /* map_tx_disable[4] = TXDISABLE_SFP+_P(X+4) */
                        {1, 1, 1}, /* map_tx_disable[5] = TXDISABLE_SFP+_P(X+5) */
                        {1, 1, 2}, /* map_tx_disable[6] = TXDISABLE_SFP+_P(X+6) */
                        {1, 1, 3}, /* map_tx_disable[7] = TXDISABLE_SFP+_P(X+7) */
    },
    .map_tx_fault   = { {0, 0, 0}, /* map_tx_fault[0] = TXFAULT_SFP+_P(X)   */
                        {0, 0, 1}, /* map_tx_fault[1] = TXFAULT_SFP+_P(X+1) */
                        {0, 0, 2}, /* map_tx_fault[2] = TXFAULT_SFP+_P(X+2) */
                        {0, 0, 3}, /* map_tx_fault[3] = TXFAULT_SFP+_P(X+3) */
                        {1, 0, 0}, /* map_tx_fault[4] = TXFAULT_SFP+_P(X+4) */
                        {1, 0, 1}, /* map_tx_fault[5] = TXFAULT_SFP+_P(X+5) */
                        {1, 0, 2}, /* map_tx_fault[6] = TXFAULT_SFP+_P(X+6) */
                        {1, 0, 3}, /* map_tx_fault[7] = TXFAULT_SFP+_P(X+7) */
    },
    .map_rxlos      = { {0, 1, 4}, /* map_rxlos[0] = OPRXLOS_PORT(X)   */
                        {0, 1, 5}, /* map_rxlos[1] = OPRXLOS_PORT(X+1) */
                        {0, 1, 6}, /* map_rxlos[2] = OPRXLOS_PORT(X+2) */
                        {0, 1, 7}, /* map_rxlos[3] = OPRXLOS_PORT(X+3) */
                        {1, 1, 4}, /* map_rxlos[4] = OPRXLOS_PORT(X+4) */
                        {1, 1, 5}, /* map_rxlos[5] = OPRXLOS_PORT(X+5) */
                        {1, 1, 6}, /* map_rxlos[6] = OPRXLOS_PORT(X+6) */
                        {1, 1, 7}, /* map_rxlos[7] = OPRXLOS_PORT(X+7) */
    },
    .map_hard_rs0   = { {2, 0, 0}, /* map_hard_rs0[0] = RS0_SFP28_P(X)   */
                        {2, 0, 2}, /* map_hard_rs0[1] = RS0_SFP28_P(X+1) */
                        {2, 0, 4}, /* map_hard_rs0[2] = RS0_SFP28_P(X+2) */
                        {2, 0, 6}, /* map_hard_rs0[3] = RS0_SFP28_P(X+3) */
                        {2, 1, 0}, /* map_hard_rs0[4] = RS0_SFP28_P(X+4) */
                        {2, 1, 2}, /* map_hard_rs0[5] = RS0_SFP28_P(X+5) */
                        {2, 1, 4}, /* map_hard_rs0[6] = RS0_SFP28_P(X+6) */
                        {2, 1, 6}, /* map_hard_rs0[7] = RS0_SFP28_P(X+7) */
    },
    .map_hard_rs1   = { {2, 0, 1}, /* map_hard_rs1[0] = RS1_SFP28_P(X)   */
                        {2, 0, 3}, /* map_hard_rs1[1] = RS1_SFP28_P(X+1) */
                        {2, 0, 5}, /* map_hard_rs1[2] = RS1_SFP28_P(X+2) */
                        {2, 0, 7}, /* map_hard_rs1[3] = RS1_SFP28_P(X+3) */
                        {2, 1, 1}, /* map_hard_rs1[4] = RS1_SFP28_P(X+4) */
                        {2, 1, 3}, /* map_hard_rs1[5] = RS1_SFP28_P(X+5) */
                        {2, 1, 5}, /* map_hard_rs1[6] = RS1_SFP28_P(X+6) */
                        {2, 1, 7}, /* map_hard_rs1[7] = RS1_SFP28_P(X+7) */
    },
};


struct ioexp_map_s ioexp_map_cypress_7abc = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 0, 0}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {2, 0, 1}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {2, 0, 2}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {2, 0, 3}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {2, 0, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {2, 0, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
    },
    .map_reset      = { {0, 0, 0}, /* map_reset[0] = QRESET_QSFP_N_P(X)   */
                        {0, 0, 1}, /* map_reset[1] = QRESET_QSFP_N_P(X+1) */
                        {0, 0, 2}, /* map_reset[2] = QRESET_QSFP_N_P(X+2) */
                        {0, 0, 3}, /* map_reset[3] = QRESET_QSFP_N_P(X+3) */
                        {0, 0, 4}, /* map_reset[4] = QRESET_QSFP_N_P(X+4) */
                        {0, 0, 5}, /* map_reset[5] = QRESET_QSFP_N_P(X+5) */
    },
    .map_lpmod      = { {0, 1, 0}, /* map_lpmod[0] = LPMODE_QSFP_P(X)   */
                        {0, 1, 1}, /* map_lpmod[1] = LPMODE_QSFP_P(X+1) */
                        {0, 1, 2}, /* map_lpmod[2] = LPMODE_QSFP_P(X+2) */
                        {0, 1, 3}, /* map_lpmod[3] = LPMODE_QSFP_P(X+3) */
                        {0, 1, 4}, /* map_lpmod[4] = LPMODE_QSFP_P(X+4) */
                        {0, 1, 5}, /* map_lpmod[5] = LPMODE_QSFP_P(X+5) */
    },
    .map_modsel     = { {1, 1, 0}, /* map_modsel[0] = MODSEL_QSFP_N_P(X)   */
                        {1, 1, 1}, /* map_modsel[1] = MODSEL_QSFP_N_P(X+1) */
                        {1, 1, 2}, /* map_modsel[2] = MODSEL_QSFP_N_P(X+2) */
                        {1, 1, 3}, /* map_modsel[3] = MODSEL_QSFP_N_P(X+3) */
                        {1, 1, 4}, /* map_modsel[4] = MODSEL_QSFP_N_P(X+4) */
                        {1, 1, 5}, /* map_modsel[5] = MODSEL_QSFP_N_P(X+5) */
    },
};


struct ioexp_map_s ioexp_map_tahoe_5a = {

    .chip_amount    = 1,
    .data_width     = 2,

    .map_present    = { {0, 0, 3}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {0, 1, 0}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {0, 1, 5}, /* map_present[2] = MOD_ABS_PORT(X+2) */
    },
    .map_reset      = { {0, 0, 1}, /* map_reset[0] = QRESET_QSFP_N_P(X)   */
                        {0, 0, 6}, /* map_reset[1] = QRESET_QSFP_N_P(X+1) */
                        {0, 1, 3}, /* map_reset[2] = QRESET_QSFP_N_P(X+2) */
    },
    .map_lpmod      = { {0, 0, 2}, /* map_lpmod[0] = LPMODE_QSFP_P(X)   */
                        {0, 0, 7}, /* map_lpmod[1] = LPMODE_QSFP_P(X+1) */
                        {0, 1, 4}, /* map_lpmod[2] = LPMODE_QSFP_P(X+2) */
    },
    .map_modsel     = { {0, 0, 0}, /* map_modsel[0] = MODSEL_QSFP_N_P(X)   */
                        {0, 0, 5}, /* map_modsel[1] = MODSEL_QSFP_N_P(X+1) */
                        {0, 1, 2}, /* map_modsel[2] = MODSEL_QSFP_N_P(X+2) */
    },
};


struct ioexp_map_s ioexp_map_tahoe_6abc = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {0, 0, 3}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {0, 1, 0}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {0, 1, 5}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {1, 0, 3}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {1, 1, 0}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {1, 1, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {2, 0, 3}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {2, 1, 0}, /* map_present[7] = MOD_ABS_PORT(X+7) */
                        {2, 1, 5}, /* map_present[8] = MOD_ABS_PORT(X+8) */
    },
    .map_reset      = { {0, 0, 1}, /* map_reset[0] = QRESET_QSFP28_N_P(X)   */
                        {0, 0, 6}, /* map_reset[1] = QRESET_QSFP28_N_P(X+1) */
                        {0, 1, 3}, /* map_reset[2] = QRESET_QSFP28_N_P(X+2) */
                        {1, 0, 1}, /* map_reset[3] = QRESET_QSFP28_N_P(X+3) */
                        {1, 0, 6}, /* map_reset[4] = QRESET_QSFP28_N_P(X+4) */
                        {1, 1, 3}, /* map_reset[5] = QRESET_QSFP28_N_P(X+5) */
                        {2, 0, 1}, /* map_reset[6] = QRESET_QSFP28_N_P(X+6) */
                        {2, 0, 6}, /* map_reset[7] = QRESET_QSFP28_N_P(X+7) */
                        {2, 1, 3}, /* map_reset[7] = QRESET_QSFP28_N_P(X+7) */
    },
    .map_lpmod      = { {0, 0, 2}, /* map_lpmod[0] = LPMODE_QSFP28_P(X)   */
                        {0, 0, 7}, /* map_lpmod[1] = LPMODE_QSFP28_P(X+1) */
                        {0, 1, 4}, /* map_lpmod[2] = LPMODE_QSFP28_P(X+2) */
                        {1, 0, 2}, /* map_lpmod[3] = LPMODE_QSFP28_P(X+3) */
                        {1, 0, 7}, /* map_lpmod[4] = LPMODE_QSFP28_P(X+4) */
                        {1, 1, 4}, /* map_lpmod[5] = LPMODE_QSFP28_P(X+5) */
                        {2, 0, 2}, /* map_lpmod[6] = LPMODE_QSFP28_P(X+6) */
                        {2, 0, 7}, /* map_lpmod[7] = LPMODE_QSFP28_P(X+7) */
                        {2, 1, 4}, /* map_lpmod[7] = LPMODE_QSFP28_P(X+8) */
    },
    .map_modsel     = { {0, 0, 0}, /* map_modsel[0] = MODSEL_QSFP28_N_P(X)   */
                        {0, 0, 5}, /* map_modsel[1] = MODSEL_QSFP28_N_P(X+1) */
                        {0, 1, 2}, /* map_modsel[2] = MODSEL_QSFP28_N_P(X+2) */
                        {1, 0, 0}, /* map_modsel[3] = MODSEL_QSFP28_N_P(X+3) */
                        {1, 0, 5}, /* map_modsel[4] = MODSEL_QSFP28_N_P(X+4) */
                        {1, 1, 2}, /* map_modsel[5] = MODSEL_QSFP28_N_P(X+5) */
                        {2, 0, 0}, /* map_modsel[6] = MODSEL_QSFP28_N_P(X+6) */
                        {2, 0, 5}, /* map_modsel[7] = MODSEL_QSFP28_N_P(X+7) */
                        {2, 1, 2}, /* map_modsel[7] = MODSEL_QSFP28_N_P(X+7) */
    },
};


struct ioexp_map_s ioexp_map_sequoia_nabc = {

    .chip_amount    = 3,
    .data_width     = 2,

    .map_present    = { {2, 1, 0}, /* map_present[0] = MOD_ABS_PORT(X)   */
                        {2, 1, 1}, /* map_present[1] = MOD_ABS_PORT(X+1) */
                        {2, 1, 2}, /* map_present[2] = MOD_ABS_PORT(X+2) */
                        {2, 1, 3}, /* map_present[3] = MOD_ABS_PORT(X+3) */
                        {2, 1, 4}, /* map_present[4] = MOD_ABS_PORT(X+4) */
                        {2, 1, 5}, /* map_present[5] = MOD_ABS_PORT(X+5) */
                        {2, 1, 6}, /* map_present[6] = MOD_ABS_PORT(X+6) */
                        {2, 1, 7}, /* map_present[7] = MOD_ABS_PORT(X+7) */
    },
    .map_reset      = { {0, 1, 0}, /* map_reset[0] = QRESET_QSFP28_N_P(X)   */
                        {0, 1, 1}, /* map_reset[1] = QRESET_QSFP28_N_P(X+1) */
                        {0, 1, 2}, /* map_reset[2] = QRESET_QSFP28_N_P(X+2) */
                        {0, 1, 3}, /* map_reset[3] = QRESET_QSFP28_N_P(X+3) */
                        {0, 1, 4}, /* map_reset[4] = QRESET_QSFP28_N_P(X+4) */
                        {0, 1, 5}, /* map_reset[5] = QRESET_QSFP28_N_P(X+5) */
                        {0, 1, 6}, /* map_reset[6] = QRESET_QSFP28_N_P(X+6) */
                        {0, 1, 7}, /* map_reset[7] = QRESET_QSFP28_N_P(X+7) */
    },
    .map_lpmod      = { {1, 0, 0}, /* map_lpmod[0] = LPMODE_QSFP28_P(X)   */
                        {1, 0, 1}, /* map_lpmod[1] = LPMODE_QSFP28_P(X+1) */
                        {1, 0, 2}, /* map_lpmod[2] = LPMODE_QSFP28_P(X+2) */
                        {1, 0, 3}, /* map_lpmod[3] = LPMODE_QSFP28_P(X+3) */
                        {1, 0, 4}, /* map_lpmod[4] = LPMODE_QSFP28_P(X+4) */
                        {1, 0, 5}, /* map_lpmod[5] = LPMODE_QSFP28_P(X+5) */
                        {1, 0, 6}, /* map_lpmod[6] = LPMODE_QSFP28_P(X+6) */
                        {1, 0, 7}, /* map_lpmod[7] = LPMODE_QSFP28_P(X+7) */
    },
    .map_modsel     = { {0, 0, 0}, /* map_modsel[0] = MODSEL_QSFP28_N_P(X)   */
                        {0, 0, 1}, /* map_modsel[1] = MODSEL_QSFP28_N_P(X+1) */
                        {0, 0, 2}, /* map_modsel[2] = MODSEL_QSFP28_N_P(X+2) */
                        {0, 0, 3}, /* map_modsel[3] = MODSEL_QSFP28_N_P(X+3) */
                        {0, 0, 4}, /* map_modsel[4] = MODSEL_QSFP28_N_P(X+4) */
                        {0, 0, 5}, /* map_modsel[5] = MODSEL_QSFP28_N_P(X+5) */
                        {0, 0, 6}, /* map_modsel[6] = MODSEL_QSFP28_N_P(X+6) */
                        {0, 0, 7}, /* map_modsel[7] = MODSEL_QSFP28_N_P(X+7) */
    },
};


struct ioexp_map_s ioexp_map_lavender_p65 = {

    .chip_amount    = 1,
    .data_width     = 1,

    .map_present    = { {0, 0, 4}, }, /* map_present[0] = MOD_ABS_PORT(X)      */
    .map_reset      = { {0, 0, 1}, }, /* map_reset[0]   = QRESET_QSFP28_N_P(X) */
    .map_lpmod      = { {0, 0, 2}, }, /* map_lpmod[0]   = LPMODE_QSFP28_P(X)   */
    .map_modsel     = { {0, 0, 0}, }, /* map_modsel[0]  = MODSEL_QSFP28_N_P(X) */
};


/* ========== Private functions ==========
 */
int check_channel_tier_1(void);

struct i2c_client *
_get_i2c_client(struct ioexp_obj_s *self,
                int chip_id){

    struct ioexp_i2c_s *i2c_curr_p = self->i2c_head_p;

    if (!(i2c_curr_p)){
        SWPS_ERR("%s: i2c_curr_p is NULL\n", __func__);
        return NULL;
    }
    while (i2c_curr_p){
        if ((i2c_curr_p->chip_id) == chip_id){
            return i2c_curr_p->i2c_client_p;
        }
        i2c_curr_p = i2c_curr_p->next;
    }
    SWPS_ERR("%s: not exist! <chip_id>:%d\n", __func__, chip_id);
    return NULL;
}


static int
_common_ioexp_update_one(struct ioexp_obj_s *self,
                         struct ioexp_addr_s *ioexp_addr,
                         int chip_id,
                         int data_width,
                         int show_err,
                         char *caller_name) {
    int buf      = 0;
    int err      = 0;
    int data_id  = 0;
    int r_offset = 0;

    for(data_id=0; data_id<data_width; data_id++){
        /* Read from IOEXP */
        r_offset = ioexp_addr->read_offset[data_id];
        buf = i2c_smbus_read_byte_data(_get_i2c_client(self, chip_id), r_offset);
        /* Check error */
        if (buf < 0) {
            err = 1;
            if (show_err) {
                SWPS_INFO("IOEXP-%d read fail! <err>:%d \n", self->ioexp_id, buf);
                SWPS_INFO("Dump: <chan>:%d <addr>:0x%02x <offset>:%d, <caller>:%s\n",
                          ioexp_addr->chan_id, ioexp_addr->chip_addr,
                          ioexp_addr->read_offset[data_id], caller_name);
            }
            continue;
        }
        /* Update IOEXP object */
        self->chip_data[chip_id].data[data_id] = (uint8_t)buf;
    }
    if (err) {
        return ERR_IOEXP_UNEXCPT;
    }
    return 0;
}


static int
common_ioexp_update_all(struct ioexp_obj_s *self,
                        int show_err,
                        char *caller_name){

    int err     = 0;
    int chip_id = 0;
    int chip_amount = self->ioexp_map_p->chip_amount;

    for (chip_id=0; chip_id<chip_amount; chip_id++){
        if (_common_ioexp_update_one(self,
                                     &(self->ioexp_map_p->map_addr[chip_id]),
                                     chip_id,
                                     self->ioexp_map_p->data_width,
                                     show_err,
                                     caller_name) < 0) {
            err = 1;
        }
    }
    if (err) {
        return ERR_IOEXP_UNEXCPT;
    }
    return 0;
}


static int
_common_check_by_mode(struct ioexp_obj_s *self){

    switch (self->mode){
        case IOEXP_MODE_DIRECT:
            return self->fsm_4_direct(self);

        case IOEXP_MODE_POLLING:
            if (self->state >= 0){
                return 0;
            }
            switch (self->state){
                case STATE_IOEXP_INIT:
                    return ERR_IOEXP_UNINIT;
                case STATE_IOEXP_ABNORMAL:
                    return ERR_IOEXP_ABNORMAL;
                default:
                    return ERR_IOEXP_NOSTATE;
            }
            break;

        default:
            break;
    }
    SWPS_ERR("%s: Exception occurs. <mode>:%d \n", __func__, self->mode);
    return ERR_IOEXP_UNEXCPT;
}


static int
_common_get_bit(struct ioexp_obj_s *self,
                struct ioexp_bitmap_s *bitmap_obj_p,
                char *func_mane){
    uint8_t buf;
    int err_code;

    /* Check and get address */
    err_code = _common_check_by_mode(self);
    if (err_code < 0){
        return err_code;
    }
    if (!bitmap_obj_p){
        SWPS_ERR("Layout config incorrect! <ioexp_id>:%d <func>:%s\n",
                 self->ioexp_id, func_mane);
        return ERR_IOEXP_BADCONF;
    }
    /* Get data form cache */
    buf = self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset];
    return (int)(buf >> bitmap_obj_p->bit_shift & 0x01);
}


static int
_common_set_bit(struct ioexp_obj_s *self,
                struct ioexp_bitmap_s *bitmap_obj_p,
                int input_val,
                char *func_mane){
    int err_code, target_offset;
    uint8_t origin_byte;
    uint8_t modify_byte;

    /* Check and get address */
    err_code = _common_check_by_mode(self);
    if (err_code < 0){
        return err_code;
    }
    if (!bitmap_obj_p){
        SWPS_ERR("Layout config incorrect! <ioexp>:%d <func>:%s\n",
                 self->ioexp_id, func_mane);
        return ERR_IOEXP_BADCONF;
    }
    /* Prepare write date */
    origin_byte = self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset];
    switch (input_val) {
        case 0:
            modify_byte = origin_byte;
            SWP_BIT_CLEAR(modify_byte, bitmap_obj_p->bit_shift);
            break;
        case 1:
            modify_byte = origin_byte;
            SWP_BIT_SET(modify_byte, bitmap_obj_p->bit_shift);
            break;
        default:
            SWPS_ERR("Input value incorrect! <val>:%d <ioexp>:%d <func>:%s\n",
                     input_val, self->ioexp_id, func_mane);
            return ERR_IOEXP_BADINPUT;
    }
    /* Setup i2c client */
    target_offset = self->ioexp_map_p->map_addr[bitmap_obj_p->chip_id].write_offset[bitmap_obj_p->ioexp_voffset];
    /* Write byte to chip via I2C */
    err_code = i2c_smbus_write_byte_data(_get_i2c_client(self, bitmap_obj_p->chip_id),
                                         target_offset,
                                         modify_byte);
    /* Update or bollback object */
    if (err_code < 0){
        self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset] = origin_byte;
        SWPS_ERR("I2C write fail! <input>:%d <ioexp>:%d <func>:%s <err>:%d\n",
                 input_val, self->ioexp_id, func_mane, err_code);
        return err_code;
    }
    self->chip_data[bitmap_obj_p->chip_id].data[bitmap_obj_p->ioexp_voffset] = modify_byte;
    return 0;
}


/* ========== Object public functions ==========
 */
int
common_get_present(struct ioexp_obj_s *self,
                   int virt_offset){

    int UNPLUG = 1;
    int retval = ERR_IOEXP_UNEXCPT;

    retval = _common_get_bit(self,
                             &(self->ioexp_map_p->map_present[virt_offset]),
                             "common_get_present");
    if (retval < 0) {
        /* [Note]
         * => Transceiver object does not need to handle IOEXP layer issues.
         */
        return UNPLUG;
    }
    return retval;
}


int
common_get_tx_fault(struct ioexp_obj_s *self,
                    int virt_offset){
    /* [Transmit Fault (Tx_Fault)]
     * A catastrophic laser fault will activate the transmitter signal,
     * TX_FAULT, and disable the laser. This signal is an open collector
     * output (pull-up required on the host board). A low signal indicates
     * normal laser operation and a high signal indicates a fault. The
     * TX_FAULT will be latched high when a laser fault occurs and is
     * cleared by toggling the TX_DISABLE input or power cycling the
     * transceiver. The transmitter fault condition can also be monitored
     * via the two-wire serial interface.
     * (address A2, byte 110, bit 2).
     *
     * 0: Normal
     * 1: Abnormal
     */
    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_tx_fault[virt_offset]),
                           "common_get_tx_fault");
}


int
common_get_rxlos(struct ioexp_obj_s *self,
                 int virt_offset){
    /* [Receiver Loss of Signal (Rx_LOS)]
     * The post-amplification IC also includes transition detection circuitry
     * which monitors the ac level of incoming optical signals and provides a
     * TTL/CMOS compatible status signal to the host (pin 8). An adequate optical
     * input results in a low Rx_LOS output while a high Rx_LOS output indicates
     * an unusable optical input. The Rx_LOS thresholds are factory set so that
     * a high output indicates a definite optical fault has occurred. Rx_LOS can
     * also be monitored via the two-wire serial interface
     * (address A2h, byte 110, bit 1).
     *
     * 0: Normal
     * 1: Abnormal
     */
    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_rxlos[virt_offset]),
                           "common_get_rxlos");
}


int
common_get_tx_disable(struct ioexp_obj_s *self,
                      int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_tx_disable[virt_offset]),
                           "common_get_tx_disable");
}


int
common_get_reset(struct ioexp_obj_s *self,
                 int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_reset[virt_offset]),
                           "common_get_reset");
}


int
common_get_lpmod(struct ioexp_obj_s *self,
                 int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_lpmod[virt_offset]),
                           "common_get_lpmod");
}


int
common_get_modsel(struct ioexp_obj_s *self,
                  int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_modsel[virt_offset]),
                           "common_get_modsel");
}


int
common_get_hard_rs0(struct ioexp_obj_s *self,
                    int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_hard_rs0[virt_offset]),
                           "common_get_hard_rs0");
}


int
common_get_hard_rs1(struct ioexp_obj_s *self,
                   int virt_offset){

    return _common_get_bit(self,
                           &(self->ioexp_map_p->map_hard_rs1[virt_offset]),
                           "common_get_hard_rs1");
}


int
common_set_tx_disable(struct ioexp_obj_s *self,
                      int virt_offset,
                      int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_tx_disable[virt_offset]),
                           input_val,
                           "common_set_tx_disable");
}


int
common_set_reset(struct ioexp_obj_s *self,
                 int virt_offset,
                 int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_reset[virt_offset]),
                           input_val,
                           "common_set_reset");
}


int
common_set_lpmod(struct ioexp_obj_s *self,
                 int virt_offset,
                 int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_lpmod[virt_offset]),
                           input_val,
                           "common_set_lpmod");
}


int
common_set_modsel(struct ioexp_obj_s *self,
                  int virt_offset,
                  int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_modsel[virt_offset]),
                           input_val,
                           "common_set_modsel");
}


int
common_set_hard_rs0(struct ioexp_obj_s *self,
        int virt_offset,
        int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_hard_rs0[virt_offset]),
                           input_val,
                           "common_set_hard_rs0");
}


int
common_set_hard_rs1(struct ioexp_obj_s *self,
        int virt_offset,
        int input_val){

    return _common_set_bit(self,
                           &(self->ioexp_map_p->map_hard_rs1[virt_offset]),
                           input_val,
                           "common_set_hard_rs1");
}


int
ioexp_get_not_support(struct ioexp_obj_s *self,
                      int virt_offset){
    return ERR_IOEXP_NOTSUPPORT;
}


int
ioexp_set_not_support(struct ioexp_obj_s *self,
                      int virt_offset,
                      int input_val){
    return ERR_IOEXP_NOTSUPPORT;
}


int
fake_ioexp_init(struct ioexp_obj_s *self){
    return 1;
}

int
fake_ioexp_update(struct ioexp_obj_s *self){
    return 1;
}


int
fake_update_func(struct ioexp_obj_s *self){
    return 1;
}

int
fake_get_func(struct ioexp_obj_s *self,
              int virt_offset){
    SWPS_WARN("Someone called fake_get_func\n");
    return -1;
}

int
fake_set_func(struct ioexp_obj_s *self,
              int virt_offset,
              int input_val){
    SWPS_WARN("Someone called fake_set_func\n");
    return -1;
}


/* ========== Initial functions for IO Expander ==========
 */
int
common_ioexp_init(struct ioexp_obj_s *self) {

    int chip_id, offset, err_code;
    struct ioexp_addr_s *addr_p;
    
    if (self->mode == IOEXP_MODE_DIRECT) {
        goto update_common_ioexp_init;
    }
    /* Setup default value to each physical IO Expander */
    for (chip_id=0; chip_id<(self->ioexp_map_p->chip_amount); chip_id++){
        /* Get address mapping */
        addr_p = &(self->ioexp_map_p->map_addr[chip_id]);
        if (!addr_p){
            SWPS_ERR("%s: IOEXP config incorrect! <chip_id>:%d \n",
                     __func__, chip_id);
            return -1;
        }
        /* Setup default value */
        for (offset=0; offset<(self->ioexp_map_p->data_width); offset++){
            err_code = i2c_smbus_write_byte_data(_get_i2c_client(self, chip_id),
                                                 addr_p->write_offset[offset],
                                                 addr_p->data_default[offset]);
            if (err_code < 0){
                SWPS_ERR("%s: set default fail! <error>:%d \n",
                         __func__, err_code);
                return ERR_IOEXP_UNEXCPT;
            }
        }
    }
update_common_ioexp_init:
    /* Check and update info to object */
    err_code = self->update_all(self, 1, "common_ioexp_init");
    if (err_code < 0) {
        SWPS_ERR("%s: update_all() fail! <error>:%d \n",
                __func__, err_code);
        return ERR_IOEXP_UNEXCPT;
    }
    return 0;
}


/* ========== Object functions for Final State Machine ==========
 */
int
_is_channel_ready(struct ioexp_obj_s *self){

    int buf     = 0;
    int chip_id = 0;  /* Use first chip which be registered */
    int data_id = 0;  /* Use first byte which be registered */
    struct ioexp_addr_s *ioexp_addr = NULL;

    ioexp_addr = &(self->ioexp_map_p->map_addr[chip_id]);
    if (!ioexp_addr){
        SWPS_ERR("%s: config incorrect!\n", __func__);
        return ERR_IOEXP_UNEXCPT;
    }
    buf = i2c_smbus_read_byte_data(_get_i2c_client(self, chip_id),
                                   ioexp_addr->read_offset[data_id]);
    if (buf >= 0){
        return 1;
    }
    return 0;
}

int
_ioexp_init_handler(struct ioexp_obj_s *self){

    int return_val;

    switch (self->mode) {
        case IOEXP_MODE_DIRECT:
            return_val = self->init(self);
            if (return_val < 0){
                self->state = STATE_IOEXP_ABNORMAL;
            } else {
                self->state = STATE_IOEXP_NORMAL;
            }
            return return_val;

        case IOEXP_MODE_POLLING:
            /* Check system and channel is ready */
            if (self->state == STATE_IOEXP_INIT){
                if (!_is_channel_ready(self)){
                    self->state = STATE_IOEXP_INIT;
                    SWPS_WARN("%s: IOEXP:%d channel not ready.\n",
                            __func__, self->ioexp_id);
                    return 0;
                }
            }
            /* Execute initial callback */
            return_val = self->init(self);
            if (return_val < 0){
                self->state = STATE_IOEXP_ABNORMAL;
            } else {
                self->state = STATE_IOEXP_NORMAL;
            }
            return return_val;

        default:
            break;
    }
    SWPS_ERR("%s: exception occur <mode>:%d\n", __func__, self->mode);
    return ERR_IOEXP_UNEXCPT;
}


int
common_ioexp_fsm_4_direct(struct ioexp_obj_s *self){

    int result_val;
    int show_err = 1;
    char *func_mane = "common_ioexp_fsm_4_direct";

    switch (self->state){
        case STATE_IOEXP_INIT:
            result_val = _ioexp_init_handler(self);
            /* Exception case: terminate initial procedure */
            if(result_val < 0){
                /* Initial fail */
                return result_val;
            }
            if(self->state == STATE_IOEXP_INIT){
                /* Keep in INIT state, and return error */
                return ERR_IOEXP_UNINIT;
            }
            /* Case: Initial done */
            return 0;

        case STATE_IOEXP_NORMAL:
            result_val = self->update_all(self, show_err, func_mane);
            if (result_val < 0){
                SWPS_INFO("%s: NORMAL -> ABNORMAL <err>:%d\n",
                           __func__, result_val);
                self->state = STATE_IOEXP_ABNORMAL;
                return result_val;
            }
            self->state = STATE_IOEXP_NORMAL;
            return 0;

        case STATE_IOEXP_ABNORMAL:
            result_val = self->update_all(self, show_err, func_mane);
            if (result_val < 0){
                self->state = STATE_IOEXP_ABNORMAL;
                return result_val;
            }
            SWPS_DEBUG("%s: ABNORMAL -> NORMAL <err>:%d\n",
                       __func__, result_val);
            self->state = STATE_IOEXP_NORMAL;
            return 0;

        default:
            break;
    }
    SWPS_ERR("%s: Exception occurs <state>:%d\n",
            __func__, self->state);
    return ERR_IOEXP_UNEXCPT;
}


int
common_ioexp_fsm_4_polling(struct ioexp_obj_s *self){

    int result_val, i, show_e;
    int fail_retry = 3;
    char *func_name = "common_ioexp_fsm_4_polling";

#ifdef DEBUG_SWPS
    show_e = 1;
#else
    show_e = 0;
#endif

    switch (self->state){
        case STATE_IOEXP_INIT:
            result_val = _ioexp_init_handler(self);
            /* Exception case: terminate initial procedure */
            if(result_val < 0){
                /* Initial fail */
                return result_val;
            }
            /* Case: System (Channel) not ready */
            if(self->state == STATE_IOEXP_INIT){
                /* Keep in INIT state, wait and retry */
                return 0;
            }
            /* Case: Initial done */
            SWPS_INFO("IOEXP-%d: initial done. <type>:%d\n",
                      self->ioexp_id, self->ioexp_type);
            return result_val;

        case STATE_IOEXP_NORMAL:
            /* Retry mechanism for case of i2c topology not stable */
            for (i=0; i<fail_retry; i++) {
                result_val = self->update_all(self, show_e, func_name);
                if (result_val >= 0) {
                    self->state = STATE_IOEXP_NORMAL;
                    return 0;
                }
                if (check_channel_tier_1() < 0) {
                    SWPS_INFO("%s: detect I2C crash <ioexp>:%d\n",
                              __func__, self->ioexp_id);
                    break;
                }
                SWPS_DEBUG("IOEXP-%d: unstable <retry>:%d\n",
                           self->ioexp_id, result_val);
            }
            SWPS_INFO("IOEXP-%d: NORMAL -> ABNORMAL <err>:%d\n",
                      self->ioexp_id, result_val);
            self->state = STATE_IOEXP_ABNORMAL;
            return result_val;

        case STATE_IOEXP_ABNORMAL:
            result_val = self->update_all(self, show_e, func_name);
            if (result_val < 0){
                self->state = STATE_IOEXP_ABNORMAL;
                return result_val;
            }
            SWPS_INFO("IOEXP-%d: ABNORMAL -> NORMAL <err>:%d\n",
                      self->ioexp_id, result_val);
            self->state = STATE_IOEXP_NORMAL;
            return 0;

        default:
            break;
    }
    SWPS_ERR("IOEXP-%d: Exception occurs <state>:%d\n",
             self->ioexp_id, self->state);
    return ERR_IOEXP_UNEXCPT;
}


/* ========== Object private functions for check & update ==========
 */
int
common_ioexp_check(struct ioexp_obj_s *self){

    int result;

    if (self->mode != IOEXP_MODE_POLLING){
        SWPS_ERR("%s: not polling mode <mode>:%d\n",
                 __func__, self->mode);
        return ERR_IOEXP_NOTSUPPORT;
    }
    mutex_lock(&self->lock);
    result = self->fsm_4_polling(self);
    mutex_unlock(&self->lock);
    return result;
}


/* ========== Functions for Factory pattern ==========
 */
static struct ioexp_map_s *
get_ioexp_map(int ioexp_type){
    switch (ioexp_type){
        case IOEXP_TYPE_MAGINOLIA_NAB:
            return &ioexp_map_magnolia_nab;
        case IOEXP_TYPE_MAGINOLIA_4AB:
            return &ioexp_map_magnolia_4ab;
        case IOEXP_TYPE_MAGINOLIA_7AB:
        case IOEXP_TYPE_SPRUCE_7AB:
            return &ioexp_map_magnolia_7ab;
        case IOEXP_TYPE_REDWOOD_P01P08:
            return &ioexp_map_redwood_p01p08_p17p24;
        case IOEXP_TYPE_REDWOOD_P09P16:
            return &ioexp_map_redwood_p09p16_p25p32;
        case IOEXP_TYPE_HUDSON32IGA_P01P08:
            return &ioexp_map_hudson32iga_p01p08_p17p24;
        case IOEXP_TYPE_HUDSON32IGA_P09P16:
            return &ioexp_map_hudson32iga_p09p16_p25p32;
        case IOEXP_TYPE_CYPRESS_NABC:
            return &ioexp_map_cypress_nabc;
        case IOEXP_TYPE_CYPRESS_7ABC:
            return &ioexp_map_cypress_7abc;
        case IOEXP_TYPE_TAHOE_5A:
            return &ioexp_map_tahoe_5a;
        case IOEXP_TYPE_TAHOE_6ABC:
            return &ioexp_map_tahoe_6abc;
        case IOEXP_TYPE_SEQUOIA_NABC:
            return &ioexp_map_sequoia_nabc;
        case IOEXP_TYPE_LAVENDER_P65:
            return &ioexp_map_lavender_p65;
        default:
            return NULL;
    }
}


int
setup_ioexp_ssize_attr(struct ioexp_obj_s *self,
                       struct ioexp_map_s *ioexp_map_p,
                       int ioexp_id,
                       int ioexp_type,
                       int run_mode){
    switch (run_mode){
        case IOEXP_MODE_POLLING:  /* Direct access device mode */
        case IOEXP_MODE_DIRECT:   /* Polling mode, read from cache */
            self->mode = run_mode;
            break;
        default:
            SWPS_ERR("%s: non-defined run_mode:%d\n",
                     __func__, run_mode);
            self->mode = ERR_IOEXP_UNEXCPT;
            return ERR_IOEXP_UNEXCPT;
    }
    /* Setup mapping structure */
    self->ioexp_map_p = kzalloc(sizeof(*ioexp_map_p), GFP_KERNEL);
    if (!(self->ioexp_map_p)) {
        SWPS_ERR("%s: kzalloc ioexp_map_p fail\n", __func__);
        return -1;
    }
    memcpy(self->ioexp_map_p, ioexp_map_p, sizeof(*ioexp_map_p));
    /* Setup attributes */
    self->ioexp_id   = ioexp_id;
    self->ioexp_type = ioexp_type;
    self->state      = STATE_IOEXP_INIT;
    mutex_init(&self->lock);
    return 0;
}


static int
setup_addr_mapping(struct ioexp_obj_s *self,
                   struct ioexp_addr_s *addr_map_p,
                   int chip_amount){
    struct ioexp_addr_s *tmp_p;
    if (!addr_map_p){
        SWPS_ERR("%s: map is null\n", __func__);
        return -1;
    }
    tmp_p = kzalloc((sizeof(*addr_map_p) * chip_amount), GFP_KERNEL);
    if (!tmp_p){
        SWPS_ERR("%s: kzalloc fail.\n", __func__);
        return -1;
    }
    memcpy(tmp_p, addr_map_p, (sizeof(*addr_map_p) * chip_amount));
    self->ioexp_map_p->map_addr = tmp_p;

    return 0;
}


static int
setup_ioexp_public_cb(struct ioexp_obj_s *self,
                      int ioexp_type){

    switch (ioexp_type){
        case IOEXP_TYPE_MAGINOLIA_NAB:
        case IOEXP_TYPE_MAGINOLIA_4AB:
            self->get_present    = common_get_present;
            self->get_tx_fault   = common_get_tx_fault;
            self->get_rxlos      = common_get_rxlos;
            self->get_tx_disable = common_get_tx_disable;
            self->get_reset      = ioexp_get_not_support;
            self->get_lpmod      = ioexp_get_not_support;
            self->get_modsel     = ioexp_get_not_support;
            self->get_hard_rs0   = ioexp_get_not_support;
            self->get_hard_rs1   = ioexp_get_not_support;
            self->set_tx_disable = common_set_tx_disable;
            self->set_reset      = ioexp_set_not_support;
            self->set_lpmod      = ioexp_set_not_support;
            self->set_modsel     = ioexp_set_not_support;
            self->set_hard_rs0   = ioexp_set_not_support;
            self->set_hard_rs1   = ioexp_set_not_support;
            return 0;
        case IOEXP_TYPE_CYPRESS_NABC:
            self->get_present    = common_get_present;
            self->get_tx_fault   = common_get_tx_fault;
            self->get_rxlos      = common_get_rxlos;
            self->get_tx_disable = common_get_tx_disable;
            self->get_reset      = ioexp_get_not_support;
            self->get_lpmod      = ioexp_get_not_support;
            self->get_modsel     = ioexp_get_not_support;
            self->get_hard_rs0   = common_get_hard_rs0;
            self->get_hard_rs1   = common_get_hard_rs1;
            self->set_tx_disable = common_set_tx_disable;
            self->set_reset      = ioexp_set_not_support;
            self->set_lpmod      = ioexp_set_not_support;
            self->set_modsel     = ioexp_set_not_support;
            self->set_hard_rs0   = common_set_hard_rs0;
            self->set_hard_rs1   = common_set_hard_rs1;
            return 0;
        case IOEXP_TYPE_MAGINOLIA_7AB:
        case IOEXP_TYPE_SPRUCE_7AB:
        case IOEXP_TYPE_REDWOOD_P01P08:
        case IOEXP_TYPE_REDWOOD_P09P16:
        case IOEXP_TYPE_HUDSON32IGA_P01P08:
        case IOEXP_TYPE_HUDSON32IGA_P09P16:
        case IOEXP_TYPE_CYPRESS_7ABC:
        case IOEXP_TYPE_TAHOE_5A:
        case IOEXP_TYPE_TAHOE_6ABC:
        case IOEXP_TYPE_SEQUOIA_NABC:
        case IOEXP_TYPE_LAVENDER_P65:
            self->get_present    = common_get_present;
            self->get_tx_fault   = ioexp_get_not_support;
            self->get_rxlos      = ioexp_get_not_support;
            self->get_tx_disable = ioexp_get_not_support;
            self->get_reset      = common_get_reset;
            self->get_lpmod      = common_get_lpmod;
            self->get_modsel     = common_get_modsel;
            self->get_hard_rs0   = ioexp_get_not_support;
            self->get_hard_rs1   = ioexp_get_not_support;
            self->set_tx_disable = ioexp_set_not_support;
            self->set_reset      = common_set_reset;
            self->set_lpmod      = common_set_lpmod;
            self->set_modsel     = common_set_modsel;
            self->set_hard_rs0   = ioexp_set_not_support;
            self->set_hard_rs1   = ioexp_set_not_support;
            return 0;

        default:
            SWPS_ERR("%s: type:%d incorrect!\n", __func__, ioexp_type);
            break;
    }
    return ERR_IOEXP_UNEXCPT;
}


static int
setup_ioexp_private_cb(struct ioexp_obj_s *self,
                       int ioexp_type){

    switch (ioexp_type){
        case IOEXP_TYPE_MAGINOLIA_NAB:
        case IOEXP_TYPE_MAGINOLIA_4AB:
        case IOEXP_TYPE_MAGINOLIA_7AB:
        case IOEXP_TYPE_SPRUCE_7AB:
        case IOEXP_TYPE_REDWOOD_P01P08:
        case IOEXP_TYPE_REDWOOD_P09P16:
        case IOEXP_TYPE_HUDSON32IGA_P01P08:
        case IOEXP_TYPE_HUDSON32IGA_P09P16:
        case IOEXP_TYPE_CYPRESS_NABC:
        case IOEXP_TYPE_CYPRESS_7ABC:
        case IOEXP_TYPE_TAHOE_5A:
        case IOEXP_TYPE_TAHOE_6ABC:
        case IOEXP_TYPE_SEQUOIA_NABC:
        case IOEXP_TYPE_LAVENDER_P65:
            self->init           = common_ioexp_init;
            self->check          = common_ioexp_check;
            self->update_all     = common_ioexp_update_all;
            self->fsm_4_direct   = common_ioexp_fsm_4_direct;
            self->fsm_4_polling  = common_ioexp_fsm_4_polling;
            return 0;

        default:
            SWPS_ERR("%s: type:%d incorrect!\n", __func__, ioexp_type);
            break;
    }
    return ERR_IOEXP_UNEXCPT;
}


static int
setup_i2c_client_one(struct ioexp_obj_s *self,
                     int chip_id){

    char *err_msg = "ERROR";
    struct i2c_adapter *adap    = NULL;
    struct i2c_client  *client  = NULL;
    struct ioexp_i2c_s *i2c_obj_p  = NULL;
    struct ioexp_i2c_s *i2c_curr_p = NULL;

    int chan_id = self->ioexp_map_p->map_addr[chip_id].chan_id;
    adap = i2c_get_adapter(chan_id);
    if(!adap){
        err_msg = "Can not get adap!";
        goto err_ioexp_setup_i2c_1;
    }
    client = kzalloc(sizeof(*client), GFP_KERNEL);
    if (!client){
        err_msg = "Can not kzalloc client!";
        goto err_ioexp_setup_i2c_1;
    }
    i2c_obj_p = kzalloc(sizeof(*i2c_obj_p), GFP_KERNEL);
    if (!i2c_obj_p){
        err_msg = "Can not kzalloc i2c_obj_p!";
        goto err_ioexp_setup_i2c_2;
    }
    client->adapter = adap;
    client->addr = self->ioexp_map_p->map_addr[chip_id].chip_addr;
    i2c_obj_p->i2c_client_p = client;
    i2c_obj_p->chip_id = chip_id;
    i2c_obj_p->next = NULL;
    if (!self->i2c_head_p){
        self->i2c_head_p = i2c_obj_p;
    } else {
        i2c_curr_p = self->i2c_head_p;
        while (i2c_curr_p->next){
            i2c_curr_p = i2c_curr_p->next;
        }
        i2c_curr_p->next = i2c_obj_p;
    }
    return 0;

err_ioexp_setup_i2c_2:
    kfree(client);
err_ioexp_setup_i2c_1:
    SWPS_ERR("%s: %s <chanID>:%d\n", __func__, err_msg, chan_id);
    return -1;
}


static int
setup_i2c_client(struct ioexp_obj_s* self){

    int result;
    int chip_id = 0;

    for (chip_id=0; chip_id<(self->ioexp_map_p->chip_amount); chip_id++){
        result  = setup_i2c_client_one(self, chip_id);
        if (result < 0){
            SWPS_ERR("%s fail! <chip_id>:%d\n", __func__, chip_id);
            return -1;
        }
    }
    return 0;
}


static int
setup_ioexp_config(struct ioexp_obj_s *self) {

    int chip_id, offset, err_code;
    struct ioexp_addr_s *addr_p;

    for (chip_id=0; chip_id<(self->ioexp_map_p->chip_amount); chip_id++){
        addr_p = &(self->ioexp_map_p->map_addr[chip_id]);
        if (!addr_p){
            SWPS_ERR("IOEXP config incorrect! <chip_id>:%d \n",chip_id);
            return -1;
        }
        for (offset=0; offset<(self->ioexp_map_p->data_width); offset++){

            err_code = i2c_smbus_write_byte_data(_get_i2c_client(self, chip_id),
                                                 addr_p->conf_offset[offset],
                                                 addr_p->conf_default[offset]);

            if (err_code < 0){
                SWPS_INFO("%s: set conf fail! <err>:%d \n", __func__, err_code);
                return -2;
            }
        }
    }
    return 0;
}


struct ioexp_obj_s *
_create_ioexp_obj(int ioexp_id,
                  int ioexp_type,
                  struct ioexp_addr_s *addr_map_p,
                  int run_mode){

    struct ioexp_map_s* ioexp_map_p;
    struct ioexp_obj_s* result_p;
    struct ioexp_i2c_s *i2c_curr_p;
    struct ioexp_i2c_s *i2c_next_p;

    /* Get layout */
    ioexp_map_p = get_ioexp_map(ioexp_type);
    if (!ioexp_map_p){
        SWPS_ERR("%s: Invalid ioexp_type\n", __func__);
        goto err_create_ioexp_fail;
    }
    /* Prepare IOEXP object */
    result_p = kzalloc(sizeof(*result_p), GFP_KERNEL);
    if (!result_p){
        SWPS_ERR("%s: kzalloc failure!\n", __func__);
        goto err_create_ioexp_fail;
    }
    /* Prepare static size attributes */
    if (setup_ioexp_ssize_attr(result_p,
                               ioexp_map_p,
                               ioexp_id,
                               ioexp_type,
                               run_mode) < 0){
        goto err_create_ioexp_setup_attr_fail;
    }
    /* Prepare address mapping */
    if (setup_addr_mapping(result_p, addr_map_p, ioexp_map_p->chip_amount) < 0){
        goto err_create_ioexp_setup_attr_fail;
    }
    if (setup_i2c_client(result_p) < 0){
        goto err_create_ioexp_setup_i2c_fail;
    }
    /* Prepare call back functions of object */
    if (setup_ioexp_public_cb(result_p, ioexp_type) < 0){
        goto err_create_ioexp_setup_i2c_fail;
    }
    if (setup_ioexp_private_cb(result_p, ioexp_type) < 0){
        goto err_create_ioexp_setup_i2c_fail;
    }
    return result_p;

err_create_ioexp_setup_i2c_fail:
    i2c_curr_p = result_p->i2c_head_p;
    i2c_next_p = result_p->i2c_head_p;
    while (i2c_curr_p){
        i2c_next_p = i2c_curr_p->next;
        kfree(i2c_curr_p->i2c_client_p);
        kfree(i2c_curr_p);
        i2c_curr_p = i2c_next_p;
    }
err_create_ioexp_setup_attr_fail:
    kfree(result_p);
err_create_ioexp_fail:
    SWPS_ERR("%s: fail! <id>:%d <type>:%d \n",
             __func__, ioexp_id, ioexp_type);
    return NULL;
}


int
create_ioexp_obj(int ioexp_id,
                 int ioexp_type,
                 struct ioexp_addr_s *addr_map_p,
                 int run_mode){

    struct ioexp_obj_s *ioexp_p  = NULL;

    ioexp_p = _create_ioexp_obj(ioexp_id, ioexp_type,
                                addr_map_p, run_mode);
    if (!ioexp_p){
        return -1;
    }
    if (ioexp_head_p == NULL){
        ioexp_head_p = ioexp_p;
        ioexp_tail_p = ioexp_p;
        return 0;
    }
    ioexp_tail_p->next = ioexp_p;
    ioexp_tail_p = ioexp_p;
    return 0;
}


static int
_init_ioexp_obj(struct ioexp_obj_s* self) {

    char *err_msg   = "ERR";
    char *func_name = "_init_ioexp_obj";

    /* Setup IOEXP configure byte */
    if (setup_ioexp_config(self) < 0){
        err_msg = "setup_ioexp_config fail";
        goto err_init_ioexp_obj;
    }
    /* Setup default data */
    if (_ioexp_init_handler(self) < 0){
        err_msg = "_ioexp_init_handler fail";
        goto err_init_ioexp_obj;
    }
    /* Update all */
    if (self->state == STATE_IOEXP_NORMAL){
        if (self->update_all(self, 1, func_name) < 0){
            err_msg = "update_all() fail";
            goto err_init_ioexp_obj;
        }
    }
    return 0;

err_init_ioexp_obj:
    SWPS_DEBUG("%s: %s\n", __func__, err_msg);
    return -1;
}


int
init_ioexp_objs(void){
    /* Return value:
     *   0: Success
     *  -1: Detect topology error
     *  -2: SWPS internal error
     */

    struct ioexp_obj_s *curr_p  = ioexp_head_p;

    if (!curr_p) {
        SWPS_ERR("%s: ioexp_head_p is NULL\n", __func__);
        return -2;
    }
    while (curr_p) {
        if (_init_ioexp_obj(curr_p) < 0) {
            SWPS_DEBUG("%s: _init_ioexp_obj() fail\n", __func__);
            return -1;
        }
        curr_p = curr_p->next;
    }
    SWPS_DEBUG("%s: done.\n", __func__);
    return 0;
}


void
clean_ioexp_objs(void){

    struct ioexp_i2c_s *i2c_curr_p   = NULL;
    struct ioexp_i2c_s *i2c_next_p   = NULL;
    struct ioexp_obj_s *ioexp_next_p = NULL;
    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    if (ioexp_head_p == NULL){
        ioexp_tail_p = NULL;
        return;
    }
    while(ioexp_curr_p){
        ioexp_next_p = ioexp_curr_p->next;
        if (ioexp_curr_p->ioexp_map_p) {
            if (ioexp_curr_p->ioexp_map_p->map_addr) {
                kfree(ioexp_curr_p->ioexp_map_p->map_addr);
            }
            kfree(ioexp_curr_p->ioexp_map_p);
        }

        i2c_curr_p = ioexp_curr_p->i2c_head_p;
        while (i2c_curr_p) {
            i2c_next_p = i2c_curr_p->next;
            kfree(i2c_curr_p->i2c_client_p);
            kfree(i2c_curr_p);
            i2c_curr_p = i2c_next_p;
        }
        kfree(ioexp_curr_p);
        ioexp_curr_p = ioexp_next_p;
    }
    ioexp_tail_p = NULL;
    SWPS_DEBUG("%s: done.\n", __func__);
}


int
check_ioexp_objs(void){

    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    while (ioexp_curr_p){
        if ( (ioexp_curr_p->check(ioexp_curr_p)) < 0){
            SWPS_INFO("check IOEXP-%d fail! <type>:%d\n",
                     ioexp_curr_p->ioexp_id, ioexp_curr_p->ioexp_type);
            return -1;
        }
        ioexp_curr_p = ioexp_curr_p->next;
    }
    return 0;
}


struct ioexp_obj_s *
get_ioexp_obj(int ioexp_id){

    struct ioexp_obj_s *result_p = NULL;
    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    while(ioexp_curr_p){
        if (ioexp_curr_p->ioexp_id == ioexp_id){
            result_p = ioexp_curr_p;
            break;
        }
        ioexp_curr_p = ioexp_curr_p->next;
    }
    return result_p;
}


void
unlock_ioexp_all(void) {

    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    while(ioexp_curr_p){
        mutex_unlock(&ioexp_curr_p->lock);
        ioexp_curr_p = ioexp_curr_p->next;
    }
}


int
lock_ioexp_all(void) {

    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;

    while(ioexp_curr_p){
        mutex_lock(&ioexp_curr_p->lock);
        ioexp_curr_p = ioexp_curr_p->next;
    }
    return 0;
}


int
check_channel_tier_1(void) {

    if ( (!_is_channel_ready(ioexp_head_p)) &&
         (!_is_channel_ready(ioexp_tail_p)) ){
        return -1;
    }
    return 0;
}


static int
_scan_channel_tier_1(int force,
                     int show_err) {

    struct ioexp_obj_s *ioexp_curr_p = ioexp_head_p;
    int ready = 0;

    if (!ioexp_curr_p) {
        goto err_scan_tier_1_channel;
    }
    while(ioexp_curr_p) {
        ready = _is_channel_ready(ioexp_curr_p);
        if ((!ready) && (!force)) {
            goto err_scan_tier_1_channel;
        }
        ioexp_curr_p = ioexp_curr_p->next;
    }
    return 0;

err_scan_tier_1_channel:
    if (show_err) {
        if (ioexp_curr_p) {
            SWPS_INFO("%s: IOEXP-%d fail\n", __func__, ioexp_curr_p->ioexp_id);
        } else {
            SWPS_INFO("%s: IOEXP is null.\n", __func__);
        }
    }
    return -1;
}


static int
_scan_channel_tier_1_single(void) {

    int ret       = 0;
    int chan_id   = 0;
    int fake_cid  = 0;
    int fake_offs = 0;
    int fake_addr = 0;
    struct i2c_adapter *adap   = NULL;
    struct i2c_client  *client = NULL;

    if (ioexp_head_p->ioexp_id != ioexp_tail_p->ioexp_id) {
        return 0;
    }
    /* Setup i2c_client */
    chan_id   = ioexp_head_p->ioexp_map_p->map_addr[fake_cid].chan_id;
    fake_addr = ioexp_head_p->ioexp_map_p->map_addr[fake_cid].chip_addr;
    adap = i2c_get_adapter((chan_id + 1));
    if(!adap){
        SWPS_INFO("%s: Can not get adap!\n", __func__);
        return 0;
    }
    client = kzalloc(sizeof(*client), GFP_KERNEL);
    if (!client){
        SWPS_INFO("%s: Can not kzalloc client!\n", __func__);
        return 0;
    }
    client->adapter = adap;
    client->addr = fake_addr;
    /* Fouce move ioexp ptr to next */
    ret = i2c_smbus_read_byte_data(client, fake_offs);
    SWPS_DEBUG("%s: move ioexp_ptr done. <ret>:%d\n", __func__, ret);
    kfree(client);
    return 1;
}


int
resync_channel_tier_1(void) {

    char *emsg = "ERR";

    if (!ioexp_head_p) {
        emsg = "ioexp_head_p is NULL";
        goto err_resync_ioexp_status_1;
    }
    /* Run all */
    if (ioexp_head_p->ioexp_id == ioexp_tail_p->ioexp_id) {
        _scan_channel_tier_1_single();
    } else {
        _scan_channel_tier_1(1, 0);
    }
    /* Check all */
    if (_scan_channel_tier_1(0, 1) < 0) {
        emsg = "resync tier-1 channel fail";
        goto err_resync_ioexp_status_1;
    }
    return 0;

err_resync_ioexp_status_1:
    SWPS_ERR("%s: %s\n", __func__, emsg);
    return -1;
}







