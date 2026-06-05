/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 */

#ifndef __BARCHETTA_CONFIG_H__
#define __BARCHETTA_CONFIG_H__

/* Tx/Rx logical lane-map details */
#if defined (SINGLE_CHIP_SETUP)
                                                                    /* L0  L1  L2  L3  L4  L5  L6  L7 */
static const bcm_plp_logical_lane_map_t sys_logical_lane_map[] =  {
                                                                    {
                                                                      CHIP_MAX_LANES,                       /* Total number of lanes */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 },    /* Logical lane-map for Sys side Rx of PHY-0 */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 }     /* Logical lane-map for Sys side Tx of PHY-0 */
                                                                    }
                                                                  };
static const bcm_plp_logical_lane_map_t line_logical_lane_map[] = {
                                                                    {
                                                                      CHIP_MAX_LANES,                       /* Total number of lanes */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 },    /* Logical lane-map for Line side Rx of PHY-0 */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 }     /* Logical lane-map for Line side Tx of PHY-0 */
                                                                    }
                                                                  };
#elif defined (DUAL_CHIP_SETUP)
static const bcm_plp_logical_lane_map_t sys_logical_lane_map[] =  {
                                                                    {
                                                                      CHIP_MAX_LANES,                       /* Total number of lanes */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 },    /* Logical lane-map for Sys side Rx of PHY-0 */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 }     /* Logical lane-map for Sys side Tx of PHY-0 */
                                                                    },
                                                                    {
                                                                      CHIP_MAX_LANES,                       /* Total number of lanes */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 },    /* Logical lane-map for Sys side Rx of PHY-1 */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 }     /* Logical lane-map for Sys side Tx of PHY-1 */
                                                                    }
                                                                  };
static const bcm_plp_logical_lane_map_t line_logical_lane_map[] = {
                                                                    {
                                                                      CHIP_MAX_LANES,                       /* Total number of lanes */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 },    /* Logical lane-map for Line side Rx of PHY-0 */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 }     /* Logical lane-map for Line side Tx of PHY-0 */
                                                                    },
                                                                    {
                                                                      CHIP_MAX_LANES,                       /* Total number of lanes */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 },    /* Logical lane-map for Line side Rx of PHY-1 */
                                                                      { 0,  1,  2,  3,  4,  5,  6,  7 }     /* Logical lane-map for Line side Tx of PHY-1 */
                                                                    }
                                                                  };
#endif

/* Default TAP parameters for PAM4 */
#define TX_SET_PAM4_DEF_PRE     (-20)
#define TX_SET_PAM4_DEF_MAIN    144
#define TX_SET_PAM4_DEF_POST    (-4)
#define TX_SET_PAM4_TAP_MODE    bcmplpTapModePAM4_LP_3TAP

/* Tx/Rx polarity values */
#define SYS_TX_POLARITY         0x00
#define SYS_RX_POLARITY         0x00
#define LINE_TX_POLARITY        0x00
#define LINE_RX_POLARITY        0x00

/* Mode configuration parameters */

#define REF_CLOCK               bcm_pm_RefClk156Mhz
#define PRBS_PATTERN            bcm_pm_PrbsPoly31
#define LANE_MAP                0xff

#endif /* __BARCHETTA_CONFIG_H__ */

