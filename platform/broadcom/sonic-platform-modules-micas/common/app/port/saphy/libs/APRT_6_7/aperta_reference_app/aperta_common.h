/*
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 */

#ifndef __APERTA_COMMON_H__
#define __APERTA_COMMON_H__

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
#include <epdm_sec.h>
#define uint32_t unsigned int
#define uint8_t  unsigned short int
typedef enum {
    POLICY_BYPASS_NONE,
    POLICY_BYPASS_CONTROL_PACKET,
    POLICY_BYPASS_DATA_PACKET,
    POLICY_BYPASS_OPTION_MAX
} macsec_policy_bypass_option_t ;

/* Common defines */
#define CHIP_NAME               "aperta"
#define CHIP_MAX_LANES          8
#define LINE_SIDE               0
#define SYS_SIDE                1
#define LINK_OFF                0
#define LINK_ON                 1
#define CHIP_ID0                0x1343
#define CHIP_ID1                0x1384
#define TEST_SUCCESS            0
#define TEST_FAILURE            (-1)
#define ENABLE                  1
#define DISABLE                 0

#define FILENAME_MAX_LENGTH     30
#define EGRESS                  0
#define INGRESS                 1
#define POLICY_BYPASS           1

#define MAX_VPORT 512
#define MAX_SA    1024
#define MAX_SA_PER_VPORT 4
#define TRANSREC_INGRESS_SIZE 20
#define TRANSREC_EGRESS_SIZE 24

/* PHY IDs for the test */
#define PHY_ID0                 0x0
#define PHY_ID1                 0x1
#define NUM_OF_PHY              2
#define LANE_MAP                ALL_LANE_MAP

/* Default TAP parameters for PAM4 */
#define TX_SET_PAM4_DEF_PRE     (-16)
#define TX_SET_PAM4_DEF_MAIN    138
#define TX_SET_PAM4_DEF_POST    (-4)
#define TX_SET_PAM4_TAP_MODE    bcmplpTapModePAM4_LP_3TAP

/* Lane Map and Port Speed */
#define ALL_LANE_MAP            0xFF
#define LANE_MAP_P0_100G        0x0F
#define LANE_MAP_P1_100G        0xF0
#define PORT_SPEED_100G         100000
#define PORT_SPEED_400G         400000
#define REF_CLOCK               bcm_pm_RefClk156Mhz

/* Register details to access using FTDI driver */
#define APRT_PMA_PMD_DEV_ADDR      1
#define APRT_CTRL_CHIP_ID_REG      0x8B00
#define APRT_CTRL_CHIP_REV_REG     0x8B01
#define CHIP_REV_ID_BIT_MASK       0xFF
#define APRT_FTDI_CHIP_ID_REG      ((APRT_PMA_PMD_DEV_ADDR << 16) | APRT_CTRL_CHIP_ID_REG)
#define APRT_FTDI_CHIP_REV_REG     ((APRT_PMA_PMD_DEV_ADDR << 16) | APRT_CTRL_CHIP_REV_REG)

/* Get number of elements of an array */
#define NUM_ARR_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

/* USB device serial number */
#define USB_DEV_SERIAL_CHIP0    "FT2SE1SQ"   /* Board Serial Number */
#define USB_DEV_SERIAL_CHIP1    "FT2QZ9CU"   /* Board Serial Number */

#define WARMBOOT_FILE_PREFIX "/tmp/warmboot"
#define WARMBOOT_MASTER_FILE "/tmp/warmbootmaster"

#ifdef _APERTA_COMMON_C_
    char        *chipname = "aperta";
    int          p_ctxt = 5;
#else
    extern char *chipname;
    extern int   p_ctxt;
#endif

/* Function declarations */
int mdio_read(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data);
int mdio_write(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data);
int device_sn_open(char *board_sn);
int device_open(void);
int device_close(void);
int polarity_swap(void);
int macsec_build_tranform_record(bcm_plp_sec_phy_access_t sec_info, int sa_num, int secy_num);
void gen_macsec_configs();
int macsec_initialize(bcm_plp_sec_phy_access_t sec_info, int bypass);
int macsec_uninitialize(bcm_plp_sec_phy_access_t sec_info);
int macsec_install_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy);
int macsec_add_rule_policy(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy);
int macsec_mtu_update(bcm_plp_sec_phy_access_t sec_info, int mtusize);
int macsec_update_rule(bcm_plp_sec_phy_access_t sec_info);
int macsec_print_statistics(unsigned int lane_map);
int macsec_sa_allowdrop_tagged_pkts(bcm_plp_sec_phy_access_t sec_info, int allow_untagged);
int macsec_sa_windowsize_update(bcm_plp_sec_phy_access_t sec_info, unsigned int winsize);
int macsec_traffic_verify(char *dm, int lane_map, char *expect);
int macsec_lanemap_to_portindex(unsigned int lane_map);
void* discard_const(const void * Ptr_p);
int synce_test(bcm_plp_access_t phy_info, unsigned int clkGenSquelchCfg, unsigned int squelchMonitorLanemap,
    unsigned int recoveredClkLane, unsigned int divider, unsigned int rclk_side, unsigned int rclk_out_pin_sel);

#if defined(BCM_PLP_TIMESYNC_SUPPORT)
int  scallop_main(int argc, char *argv[]);
#endif

#endif /* __APERTA_COMMON_H__ */
