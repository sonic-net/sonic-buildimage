/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 */

#ifndef __BARCHETTA_COMMON_H__
#define __BARCHETTA_COMMON_H__

/* Includes */
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#if defined(USE_FTDI)
#include <ftd2xx.h>
#endif
#include <epdm.h>

/* Common defines */
#define CHIP_NAME               "barchetta"
#define LINK_OFF                0
#define LINK_ON                 1
#define LINE_SIDE               0
#define SYS_SIDE                1
#define ENABLE                  1
#define DISABLE                 0
#define INVERT_ON               1
#define INVERT_OFF              0
#define PRBS_LOCK               1
#define PRBS_LOCK_LOSS          0
#define PRBS_CONFIG_TX_RX       0
#define PRBS_CONFIG_RX          1
#define PRBS_CONFIG_TX          2
#define PRBS_NO_LOOPBACK        0
#define PRBS_ERROR_LIMIT        100
#define PMD_LOCK                1
#define PMD_UNLOCK              0
#define CLK_MODE_REC_CLK        0               /* Clock mode: Clock source as Recovered clock (lane-0 is used as clock source) */
#define CLK_MODE_REF_CLK        1               /* Clock mode: Clock source as Reference clock */
#define LL_MODE_DIS_BOTH_PATH   0x0             /* Low-latency mode: Disable at both Ingress and Egress paths */
#define LL_MODE_EN_INGR_PATH    0x1             /* Low-latency mode: Enable at Ingress and disable at Egress path */
#define LL_MODE_EN_EGR_PATH     0x2             /* Low-latency mode: Enable at Egress and disable at Ingress path */
#define LL_MODE_EN_BOTH_PATH    0x3             /* Low-latency mode: Enable at both Ingress and Egress paths */
#define CHIP_ID0                0x1381
#define CHIP_ID1                0x1338
#define CHIP_MAX_LANES          8
#define TEST_SUCCESS            0
#define TEST_FAILURE            (-1)

/* PHY IDs for the test */
#define PHY_ID0                 0x10
#define PHY_ID1                 0x0
#if defined (DUAL_CHIP_SETUP)
#define NUM_OF_PHY              2
#elif defined (SINGLE_CHIP_SETUP)
#define NUM_OF_PHY              1
#endif

/* Barchetta register details */
#define BARCH_PMA_PMD_DEV_ADDR      1
#define BARCH_CTRL_CHIP_ID_REG      0x8500
#define BARCH_CTRL_CHIP_REV_REG     0x8501
#define CHIP_REV_ID_BIT_MASK        0xff

/* Register details to access using FTDI driver */
#define BARCH_FTDI_CHIP_ID_REG      ((BARCH_PMA_PMD_DEV_ADDR << 16) | BARCH_CTRL_CHIP_ID_REG)
#define BARCH_FTDI_CHIP_REV_REG     ((BARCH_PMA_PMD_DEV_ADDR << 16) | BARCH_CTRL_CHIP_REV_REG)

/* USB device serial number */
#define USB_DEV_SERIAL_CHIP0    "FT1OO5YR"
#define USB_DEV_SERIAL_CHIP1    "FT1OO5YR"

/* Get number of elements of an array */
#define NUM_ARR_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

/* Function declarations */
int mdio_read(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data);
int mdio_write(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data);
int device_open(void);
int device_close(void);
int synce_test(char *chip_name, bcm_plp_access_t phy_info, unsigned int clkGenSquelchCfg, unsigned int squelchMonitorLanemap,
    unsigned int recoveredClkLane, unsigned int divider, unsigned int rclk_side);

#ifdef __BARCHETTA_COMMON_C__
    char        *chipname = "barchetta";
    int          p_ctxt = 5;
#else
    extern char *chipname;
    extern int   p_ctxt;
#endif

#endif /* __BARCHETTA_COMMON_H__ */
