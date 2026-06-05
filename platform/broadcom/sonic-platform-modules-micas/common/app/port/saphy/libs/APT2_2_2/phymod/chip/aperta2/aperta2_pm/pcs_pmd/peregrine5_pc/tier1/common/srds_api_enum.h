/********************************************************************************
 *                                                                              *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                              *
 ********************************************************************************/

/******************************************************************************
 ******************************************************************************
 *  Revision      :   *
 *                                                                            *
 *  Description   :  Enum types used by Serdes API functions                  *
 *                                                                            *
 ******************************************************************************
 ******************************************************************************/

#ifndef SRDS_API_ENUM_H
#define SRDS_API_ENUM_H

/*! @file
 *  @brief Enum types used by Serdes API functions
 */

/*! @addtogroup APITag
 * @{
 */

/*! @defgroup SrdsAPIEnumTag Common Enum Types */

/*! @addtogroup SrdsAPIEnumTag
 * @{
 */

/** PRBS Polynomial Enum */
enum plp_aperta2_srds_prbs_polynomial_enum {
    PRBS_7  = 0,
    PRBS_9  = 1,
    PRBS_11 = 2,
    PRBS_15 = 3,
    PRBS_23 = 4,
    PRBS_31 = 5,
    PRBS_58 = 6,
    PRBS_49 = 7,
    PRBS_10 = 8,
    PRBS_20 = 9,
    PRBS_13 = 10,
    USER_40_BIT_REPEAT = 15,
    PRBS_AUTO_DETECT = 20,
    PRBS_58_TR = 40,
    PCS_PRBS_7 = 100,
    PCS_PRBS_15 = 101,
    PCS_PRBS_23 = 102,
    PCS_PRBS_31 = 103,
    PRBS_UNKNOWN = 255
};


/** PRBS Checker Mode Enum */
enum plp_aperta2_srds_prbs_checker_mode_enum {
    PRBS_SELF_SYNC_HYSTERESIS       = 0,
    PRBS_INITIAL_SEED_HYSTERESIS    = 1,
    PRBS_INITIAL_SEED_NO_HYSTERESIS = 2
};


/** TX_PI Fixed Frequency and Jitter Generation Modes Enum */
enum plp_aperta2_srds_tx_pi_freq_jit_gen_enum {
    TX_PI_FIXED_FREQ      = 0,
    TX_PI_SSC_LOW_FREQ    = 1,
    TX_PI_SSC_HIGH_FREQ   = 2,
    TX_PI_SJ              = 3
};

/** RX AFE Settings Enum */
enum plp_aperta2_srds_rx_afe_settings_enum {
    RX_AFE_PF,
    RX_AFE_PF2,
    RX_AFE_VGA,
    RX_AFE_FGA,
    RX_AFE_DFE1,
    RX_AFE_DFE2,
    RX_AFE_DFE3,
    RX_AFE_DFE4,
    RX_AFE_DFE5,
    RX_AFE_DFE6,
    RX_AFE_DFE7,
    RX_AFE_DFE8,
    RX_AFE_DFE9,
    RX_AFE_DFE10,
    RX_AFE_DFE11,
    RX_AFE_DFE12,
    RX_AFE_DFE13,
    RX_AFE_DFE14,
    RX_AFE_DFE15,
    RX_AFE_DFE16,
    RX_AFE_DFE17,
    RX_AFE_DFE18,
    RX_AFE_PF3,
    RX_AFE_FFE_PRE,
    RX_AFE_FFE_POST,
    RX_FFE
};

/** Repeater Mode Enum */
enum plp_aperta2_srds_rptr_mode_enum {
    DATA_IN_SIDE,
    DIG_LPBK_SIDE,
    RMT_LPBK_SIDE
};

/** Core Powerdown Modes Enum */
enum plp_aperta2_srds_core_pwrdn_mode_enum {
    PWR_ON,
    PWRDN,
    PWRDN_DEEP,
    PWRDN_TX,
    PWRDN_RX,
    PWROFF_DEEP,
    PWRDN_TX_DEEP,
    PWRDN_RX_DEEP,
    PWR_ON_TX,
    PWR_ON_RX
};

/** Eye Scan Direction Enum */
enum plp_aperta2_srds_eye_scan_dir_enum {
    EYE_SCAN_HORIZ = 0,
    EYE_SCAN_VERTICAL = 1,
    EYE_SCAN_SLICE = 2,
    EYE_SCAN_INTR  = 3
};

/* Used by #serdes_display_diag_data to select diag_level             */
/* example:                                                           */
/*  diag_level = SRDS_DIAG_LANE | SRDS_DIAG_CORE | SRDS_DIAG_EVENT    */
enum plp_aperta2_srds_diag_level_enum {
    SRDS_DIAG_LANE = 1,
    SRDS_DIAG_CORE = 1<<1,
    SRDS_DIAG_EVENT = 1<<2,
    SRDS_DIAG_EYE = 1<<3,
    SRDS_DIAG_REG_CORE = 1<<4,
    SRDS_DIAG_REG_LANE = 1<<5,
    SRDS_DIAG_UC_CORE = 1<<6,
    SRDS_DIAG_UC_LANE = 1<<7,
    SRDS_DIAG_LANE_DEBUG = 1<<8,
    SRDS_DIAG_BER_VERT = 1<<9,
    SRDS_DIAG_BER_HORZ = 1<<10,
    SRDS_DIAG_EVENT_SAFE = 1<<11,
    SRDS_DIAG_TIMESTAMP = 1<<12,
    SRDS_DIAG_PRBS_PROJ = 1<<13,
    SRDS_DIAG_PRBS_PROJ_30S = 1<<14,
    SRDS_DIAG_INFO_TABLE = 1<<15
};

/* Used by #serdes_diag_access to select type of access */
enum plp_aperta2_srds_diag_access_enum {
    SRDS_REG_READ,
    SRDS_REG_RMW,
    SRDS_CORE_RAM_READ_BYTE,
    SRDS_CORE_RAM_RMW_BYTE,
    SRDS_CORE_RAM_READ_WORD,
    SRDS_CORE_RAM_RMW_WORD,
    SRDS_LANE_RAM_READ_BYTE,
    SRDS_LANE_RAM_RMW_BYTE,
    SRDS_LANE_RAM_READ_WORD,
    SRDS_LANE_RAM_RMW_WORD,
    SRDS_GLOB_RAM_READ_BYTE,
    SRDS_GLOB_RAM_RMW_BYTE,
    SRDS_GLOB_RAM_READ_WORD,
    SRDS_GLOB_RAM_RMW_WORD,
    SRDS_PROG_RAM_READ_BYTE,
    SRDS_PROG_RAM_READ_WORD,
    SRDS_UC_CMD,
    SRDS_EN_BREAKPOINT,
    SRDS_GOTO_BREAKPOINT,
    SRDS_RD_BREAKPOINT,
    SRDS_DIS_BREAKPOINT,
    SRDS_BER_PROJ_DATA
};

/* Used by linktrn_diag to select type of event log */
enum srds_linktrn_diag_log_level_enum {
    SRDS_LINKTRN_DIAG_LOG_INITIAL = 1,
    SRDS_LINKTRN_DIAG_LOG_DETAIL = 1<<1
};

/* Used by error analyzer to select the type of BER report
   Not applicable to 7nm cores. */
enum srds_prbs_ber_rept_enum {
    SRDS_REPT_BER_EQUAL_TO       = 0,
    SRDS_REPT_BER_LESS_THAN      = 1,
    SRDS_REPT_BER_LESS_AND_EQUAL = 2
};
/*! @} SrdsAPIEnumTag */
/*! @} APITag */
#endif
