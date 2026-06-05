/*
 * $Copyright: (c) 2024 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * BCM APIs. This program might not work for all environments.
 */

#ifndef __APERTA2_COMMON_H__
#define __APERTA2_COMMON_H__

/* Includes */
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include <smbus_access.h>
#include <pmbus_regulator.h>

#if defined(USE_FTDI)
#include <ftd2xx.h>
#endif
#include <epdm.h>
#include <epdm_sec.h>

typedef struct timespec hr_time_t;

#define uint32_t unsigned int
#define uint8_t  unsigned short int

/* Common defines */
#define CHIP_NAME               "aperta2"
#define CHIP_MAX_LANES          16
#define LINE_SIDE               0
#define SYS_SIDE                1
#define LINK_OFF                0
#define LINK_ON                 1
#define AN_GOOD                 1
#define CHIP_ID                 0x5343
#define TEST_SUCCESS            0
#define TEST_FAILURE            (-1)
#define ENABLE                  1
#define DISABLE                 0

/* Lane Map and Port Speed */
#define ALL_LANE_MAP            0xFFFF
#define OCTAL0_LANE_MAP         0xFF
#define OCTAL1_LANE_MAP         0xFF00
#define REF_CLOCK               bcm_pm_RefClk312Mhz

/* PHY IDs for the test */
#define PHY_ID                  0x0
#define NUM_OF_PHY              1
#define LANE_MAP                ALL_LANE_MAP
#define MAX_NUM_OF_LANES        16
/* Default TAP parameters */
#define TX_SET_NRZ_PRE3         0
#define TX_SET_NRZ_PRE2         0
#define TX_SET_NRZ_PRE          0
#define TX_SET_NRZ_MAIN         95
#define TX_SET_NRZ_POST         0
#define TX_SET_NRZ_POST2        0
#define TX_SET_NRZ_TAP_MODE     bcmplpTapModeNRZ_6TAP

/* Default TAP parameters */
#define TX_SET_PAM4_PRE3        0
#define TX_SET_PAM4_PRE2        0
#define TX_SET_PAM4_PRE         0
#define TX_SET_PAM4_MAIN        148
#define TX_SET_PAM4_POST        0
#define TX_SET_PAM4_POST2       0
#define TX_SET_PAM4_TAP_MODE    bcmplpTapModePAM4_6TAP

/* Register details to access using FTDI driver */
#define APRT2_PMA_PMD_DEV_ADDR      1
#define APRT2_CTRL_CHIP_ID_REG      0x8B00
#define APRT2_CTRL_CHIP_REV_REG     0x8B01
#define CHIP_REV_ID_BIT_MASK        0xFF
#define APRT2_FTDI_CHIP_ID_REG      ((APRT2_PMA_PMD_DEV_ADDR << 16) | APRT2_CTRL_CHIP_ID_REG)
#define APRT2_FTDI_CHIP_REV_REG     ((APRT2_PMA_PMD_DEV_ADDR << 16) | APRT2_CTRL_CHIP_REV_REG)

/* Get number of elements of an array */
#define NUM_ARR_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

/* USB device serial number */
#define USB_DEV_SERIAL_CHIP    "FT7AB30T"   /* Board Serial Number */

#ifdef _APERTA2_COMMON_C_
    char        *chipname = "aperta2";
    int          p_ctxt = 5;
#else
    extern char *chipname;
    extern int   p_ctxt;
#endif

/* PLL Defines */
#define APERTA2_VCO_53G               bcmplpVco53G
#define APERTA2_VCO_51G               bcmplpVco51G

#define TRAINED                       1
#define UNTRAINED                     0
#define TRAINING_COMPLETE             0
#define TRAINING_FAILURE              1

#define AVS_FW_control_Adr            (0xA3A0) /*FW_REGS_fwreg_3A0_Adr*/
#define AVS_FW_status_Adr             (0xA3A1) /*FW_REGS_fwreg_3A1_Adr*/
#define AVS_regulator0_config_Adr     (0xA3A2) /*FW_REGS_fwreg_3A2_Adr*/
#define AVS_regulator1_config_Adr     (0xA3A3) /*FW_REGS_fwreg_3A3_Adr*/
#define AVS_I2C_address0_Adr          (0xA3A4) /*FW_REGS_fwreg_3A4_Adr*/
#define AVS_I2C_address1_Adr          (0xA3A5) /*FW_REGS_fwreg_3A5_Adr*/
#define AVS_Vout_control_Adr          (0xA3A6) /*FW_REGS_fwreg_3A6_Adr*/
#define AVS_Vout_mV_status_Adr        (0x8269) /*GEN_CNTRLS_GPREeg_19_Adr*/
#define AVS_Vout_response_Adr         (0x8267) /*GEN_CNTRLS_GPREeg_17_Adr*/

#define CONFIRM_AFTER_WAIT

#define REGULATOR_I2C_ADDR            0x41
#define REGULATOR_RAIL_ADDR           0x40
#define AVS_I2C_RESP_ADDR             0

typedef struct {
    char tag[8];
    bcm_plp_access_t phy_info;
    uint16_t cnt, state;
    uint16_t info[4];
} actor_t;

actor_t aSTA, aRGC;

/* Function declarations */
int mdio_i2c_read(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data);
int mdio_i2c_write(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data);
int device_sn_open(char *board_sn);
int device_open(void);
int device_close(void);
int polarity_swap(void);

hr_time_t hr_time(hr_time_t* p);
double hr_difftime(hr_time_t *end, hr_time_t *begin);
uint16_t get_avs_vrequest(char *chip_name, bcm_plp_access_t plp_info, uint8_t avs_i2c_responder_addr);
void put_avs_vresponse(char *chip_name, bcm_plp_access_t plp_info, uint8_t avs_i2c_responder_addr, uint16_t resp);
uint16_t get_dvdd_with_limit(uint16_t vout_req, uint16_t lim_lo, uint16_t lim_hi);
actor_t * get_STA(void);
actor_t * get_RGC(void);
uint16_t run_STA(char *chip_name, actor_t *sta, double tmd);
uint16_t run_RGC(char *chip_name, actor_t *rgc, double tmd);
uint16_t show_AVS_status_from_STA(char *chip_name, bcm_plp_access_t plp_info);
void config_regulators(char *chip_name, bcm_plp_access_t plp_info, uint8_t rail_addr);
void test_close_loop_ext_regu_controller(char *chip_name, bcm_plp_access_t plp_info, uint8_t regu_rail_addr, uint8_t avs_i2c_responder_addr);
void config_dut_as_avs_requestor(char *chip_name, bcm_plp_access_t plp_info, uint8_t addr);

#endif /* __APERTA2_COMMON_H__ */
