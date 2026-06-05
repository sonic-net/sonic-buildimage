/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/******************************************************************************
 ******************************************************************************
 *  Revision      :   *
 *                                                                            *
 *  Description   :  Enum types used by Serdes API functions                  *
 *                                                                            *
 ******************************************************************************
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PEREGRINE5_PC_API_ENUM_H
#define PEREGRINE5_PC_API_ENUM_H

#include "peregrine5_pc_ipconfig.h"
#ifdef NON_SDK
#include <stdint.h>
#endif

/*! @file
 *  @brief Enum types used by Serdes API functions
 */

/*! @addtogroup APITag
 * @{
 */

/*! @defgroup SerdesAPIEnumTag Serdes API Enums
 * Enums used throughout Serdes APIs.
 */

/*! @addtogroup SerdesAPIEnumTag
 * @{
 */


/*--------------------------*/
/** Serdes RX PAM Mode Enum */
/*--------------------------*/
enum peregrine5_pc_rx_pam_mode_enum {
    NRZ,
    PAM4_NR,
    PAM4_ER
};

enum peregrine5_pc_cdr_enum {
    OS,
    BR
};

/*! Reference clock frequency enums
 *
 */
enum peregrine5_pc_pll_refclk_enum {
    PEREGRINE5_PC_PLL_REFCLK_UNKNOWN = 0, /*!< Refclk value to be determined by API. */
    PEREGRINE5_PC_PLL_REFCLK_156P25MHZ      = (int)0x00400271, /*!< 156.25 MHz      */
    PEREGRINE5_PC_PLL_REFCLK_161P1328125MHZ = (int)0x08005091, /*!< 161.1328125 MHz */
    PEREGRINE5_PC_PLL_REFCLK_166P67MHZ      = (int)0x0640411B, /*!< 166.67 MHz      */
    PEREGRINE5_PC_PLL_REFCLK_312P5MHZ       = (int)0x00200271  /*!< 312.5 MHz       */
    };

/** @brief Enumeration DIV values used by PLL configuration APIs.
 */
enum peregrine5_pc_pll_div_enum {
    PEREGRINE5_PC_PLL_DIV_UNKNOWN = 0, /*!< Divide value to be determined by API. */
    PEREGRINE5_PC_PLL_DIV_179P2      = (int)0x333340B3, /*!< Divide by 179.2      */
    PEREGRINE5_PC_PLL_DIV_256        = (int)0x00000100, /*!< Divide by 256        */
    PEREGRINE5_PC_PLL_DIV_288        = (int)0x00000120, /*!< Divide by 288        */
    PEREGRINE5_PC_PLL_DIV_294P4      = (int)0x66668126, /*!< Divide by 294.4      */
    PEREGRINE5_PC_PLL_DIV_316P8      = (int)0xCCCCC13C, /*!< Divide by 316.8      */
    PEREGRINE5_PC_PLL_DIV_320        = (int)0x00000140, /*!< Divide by 320        */
    PEREGRINE5_PC_PLL_DIV_336        = (int)0x00000150, /*!< Divide by 336        */
    PEREGRINE5_PC_PLL_DIV_340        = (int)0x00000154, /*!< Divide by 340        */
    PEREGRINE5_PC_PLL_DIV_358        = (int)0x00000166, /*!< Divide by 358        */
    PEREGRINE5_PC_PLL_DIV_358P4      = (int)0x66668166, /*!< Divide by 358.4      */
    PEREGRINE5_PC_PLL_DIV_360        = (int)0x00000168, /*!< Divide by 360        */
    PEREGRINE5_PC_PLL_DIV_368        = (int)0x00000170, /*!< Divide by 368        */
    PEREGRINE5_PC_PLL_DIV_396        = (int)0x0000018C, /*!< Divide by 396        */
    PEREGRINE5_PC_PLL_DIV_400        = (int)0x00000190, /*!< Divide by 400        */
    PEREGRINE5_PC_PLL_DIV_412P5      = (int)0x8000019C, /*!< Divide by 412.5      */
    PEREGRINE5_PC_PLL_DIV_448        = (int)0x000001C0, /*!< Divide by 448        */
    PEREGRINE5_PC_PLL_DIV_480        = (int)0x000001E0, /*!< Divide by 480        */
    PEREGRINE5_PC_PLL_DIV_528        = (int)0x00000210, /*!< Divide by 528        */
    PEREGRINE5_PC_PLL_DIV_560        = (int)0x00000230, /*!< Divide by 560        */
    PEREGRINE5_PC_PLL_DIV_132        = (int)0x00000084, /*!< Divide by 132        */
    PEREGRINE5_PC_PLL_DIV_147P2      = (int)0x33330093, /*!< Divide by 147.2      */
    PEREGRINE5_PC_PLL_DIV_158P4      = (int)0x6666809E, /*!< Divide by 158.4      */
    PEREGRINE5_PC_PLL_DIV_160        = (int)0x000000A0, /*!< Divide by 160        */
    PEREGRINE5_PC_PLL_DIV_165        = (int)0x000000A5, /*!< Divide by 165        */
    PEREGRINE5_PC_PLL_DIV_168        = (int)0x000000A8, /*!< Divide by 168        */
    PEREGRINE5_PC_PLL_DIV_170        = (int)0x000000AA, /*!< Divide by 170        */
    PEREGRINE5_PC_PLL_DIV_175        = (int)0x000000AF, /*!< Divide by 175        */
    PEREGRINE5_PC_PLL_DIV_180        = (int)0x000000B4, /*!< Divide by 180        */
    PEREGRINE5_PC_PLL_DIV_264        = (int)0x00000108, /*!< Divide by 264        */
    PEREGRINE5_PC_PLL_DIV_280        = (int)0x00000118, /*!< Divide by 280        */
    PEREGRINE5_PC_PLL_DIV_330        = (int)0x0000014A, /*!< Divide by 330        */
    PEREGRINE5_PC_PLL_DIV_350        = (int)0x0000015E  /*!< Divide by 350        */
};

/** TX AFE Settings Enum */
enum peregrine5_pc_tx_afe_settings_enum {
    PEREGRINE5_PC_TX_AFE_PRE3,
    PEREGRINE5_PC_TX_AFE_PRE2,
    PEREGRINE5_PC_TX_AFE_PRE1,
    PEREGRINE5_PC_TX_AFE_MAIN,
    PEREGRINE5_PC_TX_AFE_POST1,
    PEREGRINE5_PC_TX_AFE_POST2,
};

/** TXFIR Tap Enable Enum */
enum peregrine5_pc_txfir_tap_enable_enum {
    PEREGRINE5_PC_NRZ_LP_3TAP  = 0,
    PEREGRINE5_PC_NRZ_6TAP     = 1,
    PEREGRINE5_PC_PAM4_LP_3TAP = 2,
    PEREGRINE5_PC_PAM4_6TAP    = 3
};

/*! @def PEREGRINE5_PC_PLL_OPTION_REFCLK_MASK
 * Mask used for REFCLK options within enum peregrine5_pc_pll_option_enum.
 * All REFCLK options should fall within this mask's range.
 */
#define PEREGRINE5_PC_PLL_OPTION_REFCLK_MASK (0xF)

/** PLL Configuration Options Enum used for configure_pll APIs
 *  If using REFCLK options, only one REFCLK option may be used at a time.
 */
enum peregrine5_pc_pll_option_enum {
    PEREGRINE5_PC_PLL_OPTION_NONE              =  0,
    PEREGRINE5_PC_PLL_OPTION_REFCLK_DOUBLER_EN =  1,
    PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV2_EN    =  2,
    PEREGRINE5_PC_PLL_OPTION_REFCLK_DIV4_EN    =  3,
    PEREGRINE5_PC_PLL_OPTION_POWERDOWN         =  1<<4,
    PEREGRINE5_PC_PLL_OPTION_DISABLE_VERIFY    =  1<<5
};


/** Force / Get Force CDR Mode Enum */
enum peregrine5_pc_force_cdr_mode_enum {
    PEREGRINE5_PC_OSCDR_FORCE_ENABLE,     /*!< Force OS-CDR mode */
    PEREGRINE5_PC_OSCDR_FORCE_DISABLE,    /*!< Disable Force OS-CDR mode */
    PEREGRINE5_PC_BRCDR_FORCE_ENABLE,     /*!< Force BR-CDR mode */
    PEREGRINE5_PC_BRCDR_FORCE_DISABLE,    /*!< Disable Force BR-CDR mode */
    PEREGRINE5_PC_CDR_FORCE_DISABLE       /*!< Disable Force CDR modes */
};

/*! PRBS Error Analyzer Aggregate Mode
 *
 */

enum peregrine5_pc_prbs_error_analyzer_aggregate_mode_enum {
    PEREGRINE5_PC_FEC_100GE,              /*!< 100G w/ Bit Muxing 4:1 and no Code Word Interleaving 100GE 112G Peregrine */
    PEREGRINE5_PC_FEC_200GE,              /*!< 200G w/ Bit Muxing 4:1 and 2 Code Word Interleaving 200GE/400GE 112G Peregrine (D1.5 version) */
    PEREGRINE5_PC_FEC_CUSTOM
};


/** Serdes fec code type Enum */
enum peregrine5_pc_fec_code_type_enum {
    PEREGRINE5_PC_RS_544_514_10 = 0,
    PEREGRINE5_PC_RS_528_514_10 = 1
};



/*! @} SerdesAPIEnumTag */
/*! @} APITag */
#endif /*#define PEREGRINE5_PC_API_ENUM_H*/
#ifdef __cplusplus
}
#endif
