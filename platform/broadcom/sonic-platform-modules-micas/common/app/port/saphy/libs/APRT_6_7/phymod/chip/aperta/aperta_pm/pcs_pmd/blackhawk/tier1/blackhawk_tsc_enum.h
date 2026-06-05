/******************************************************************************
 ******************************************************************************
 *                                                                            *
 *  Description   :  Enum types used by Serdes API functions                  *
 *                                                                            *
 * $Copyright: (c) 2020 Broadcom.                                             *
 * Broadcom Proprietary and Confidential. All rights reserved.$               *
 *                                                                            *
 ******************************************************************************
 ******************************************************************************/

/** @file blackhawk_tsc_enum.h
 * Enum types used by Serdes API functions
 */

#ifndef BLACKHAWK_TSC_API_ENUM_H
#define BLACKHAWK_TSC_API_ENUM_H

#include "blackhawk_tsc_ipconfig.h"
#ifdef NON_SDK
#include <stdint.h>
#else
#include <phymod/phymod.h>
#endif


enum blackhawk_tsc_pll_refclk_enum {
    BLACKHAWK_TSC_PLL_REFCLK_UNKNOWN = 0, /* Refclk value to be determined by API.  */
    BLACKHAWK_TSC_PLL_REFCLK_100MHZ          = (int)0x00100064, /* 100 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_106P25MHZ       = (int)0x004001A9, /* 106.25 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_122P88MHZ       = (int)0x01900C00, /* 122.88 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_125MHZ          = (int)0x0010007D, /* 125 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_145P947802MHZ   = (int)0x16C0CF85, /* 145.947802 MHz   */
    BLACKHAWK_TSC_PLL_REFCLK_155P52MHZ       = (int)0x01900F30, /* 155.52 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_156P25MHZ       = (int)0x00400271, /* 156.25 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_159P375MHZ      = (int)0x008004FB, /* 159.375 MHz      */
    BLACKHAWK_TSC_PLL_REFCLK_161P1328125MHZ  = (int)0x08005091, /* 161.1328125 MHz  */
    BLACKHAWK_TSC_PLL_REFCLK_166P67MHZ       = (int)0x0640411B, /* 166.67 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_166P7974882MHZ  = (int)0x27D19F0A, /* 166.7974882 MHz  */
    BLACKHAWK_TSC_PLL_REFCLK_167P38MHZ       = (int)0x06404162, /* 167.38 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_167P4107143MHZ  = (int)0x0380249F, /* 167.4107143 MHz  */
    BLACKHAWK_TSC_PLL_REFCLK_174P703125MHZ   = (int)0x04002BAD, /* 174.703125 MHz   */
    BLACKHAWK_TSC_PLL_REFCLK_212P5MHZ        = (int)0x002001A9, /* 212.5 MHz        */
    BLACKHAWK_TSC_PLL_REFCLK_311p04          = (int)0x01901E60, /* 311.04 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_312P5MHZ        = (int)0x00200271, /* 312.5 MHz        */
    BLACKHAWK_TSC_PLL_REFCLK_425MHZ          = (int)0x001001A9, /* 425 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_534MHZ          = (int)0x00100216, /* 534 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_705MHZ          = (int)0x001002C1, /* 705 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_175MHZ          = (int)0x001000AF, /* 175 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_400MHZ          = (int)0x00100190, /* 400 MHz          */
    BLACKHAWK_TSC_PLL_REFCLK_266P67MHZ       = (int)0x0640682B, /* 266.67 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_156P637MHZ      = (int)0x3E8263DD, /* 156.637 MHz      */
    BLACKHAWK_TSC_PLL_REFCLK_157P844MHZ      = (int)0x0FA09A25, /* 157.844 MHz      */
    BLACKHAWK_TSC_PLL_REFCLK_158P51MHZ       = (int)0x06403DEB, /* 158.51 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_162P9485MHZ     = (int)0x7D04F909, /* 162.9485 MHz     */
    BLACKHAWK_TSC_PLL_REFCLK_167P331MHZ      = (int)0x3E828DA3, /* 167.331 MHz      */
    BLACKHAWK_TSC_PLL_REFCLK_168P04MHZ       = (int)0x01901069, /* 168.04 MHz       */
    BLACKHAWK_TSC_PLL_REFCLK_169P40905086MHZ = (int)0xD4B8CBF3, /* 169.40905086 MHz */
    BLACKHAWK_TSC_PLL_REFCLK_173P37MHZ       = (int)0x064043B9  /* 173.37 MHz       */
    };


enum blackhawk_tsc_pll_div_enum {
    BLACKHAWK_TSC_PLL_DIV_UNKNOWN = 0, /* Divide value to be determined by API. */
    BLACKHAWK_TSC_PLL_DIV_66         = (int)0x00000042, /* Divide by 66         */
    BLACKHAWK_TSC_PLL_DIV_67         = (int)0x00000043, /* Divide by 67         */
    BLACKHAWK_TSC_PLL_DIV_70         = (int)0x00000046, /* Divide by 70         */
    BLACKHAWK_TSC_PLL_DIV_72         = (int)0x00000048, /* Divide by 72         */
    BLACKHAWK_TSC_PLL_DIV_76         = (int)0x0000004C, /* Divide by 76         */
    BLACKHAWK_TSC_PLL_DIV_73P6       = (int)0x99998049, /* Divide by 73.6       */
    BLACKHAWK_TSC_PLL_DIV_79P2       = (int)0x3333004F, /* Divide by 79.2       */
    BLACKHAWK_TSC_PLL_DIV_80         = (int)0x00000050, /* Divide by 80         */
    BLACKHAWK_TSC_PLL_DIV_82P5       = (int)0x80000052, /* Divide by 82.5       */
    BLACKHAWK_TSC_PLL_DIV_84         = (int)0x00000054, /* Divide by 84         */
    BLACKHAWK_TSC_PLL_DIV_85         = (int)0x00000055, /* Divide by 85         */
    BLACKHAWK_TSC_PLL_DIV_87P5       = (int)0x80000057, /* Divide by 87.5       */
    BLACKHAWK_TSC_PLL_DIV_89P6       = (int)0x99998059, /* Divide by 89.6       */
    BLACKHAWK_TSC_PLL_DIV_90         = (int)0x0000005A, /* Divide by 90         */
    BLACKHAWK_TSC_PLL_DIV_96         = (int)0x00000060, /* Divide by 96         */
    BLACKHAWK_TSC_PLL_DIV_100        = (int)0x00000064, /* Divide by 100        */
    BLACKHAWK_TSC_PLL_DIV_120        = (int)0x00000078, /* Divide by 120        */
    BLACKHAWK_TSC_PLL_DIV_127P401984 = (int)0x66E8807F, /* Divide by 127.401984 */
    BLACKHAWK_TSC_PLL_DIV_128        = (int)0x00000080, /* Divide by 128        */
    BLACKHAWK_TSC_PLL_DIV_132        = (int)0x00000084, /* Divide by 132        */
    BLACKHAWK_TSC_PLL_DIV_134        = (int)0x00000086, /* Divide by 134        */
    BLACKHAWK_TSC_PLL_DIV_140        = (int)0x0000008C, /* Divide by 140        */
    BLACKHAWK_TSC_PLL_DIV_144        = (int)0x00000090, /* Divide by 144        */
    BLACKHAWK_TSC_PLL_DIV_147P2      = (int)0x33330093, /* Divide by 147.2      */
    BLACKHAWK_TSC_PLL_DIV_148        = (int)0x00000094, /* Divide by 148        */
    BLACKHAWK_TSC_PLL_DIV_152        = (int)0x00000098, /* Divide by 152        */
    BLACKHAWK_TSC_PLL_DIV_158P4      = (int)0x6666809E, /* Divide by 158.4      */
    BLACKHAWK_TSC_PLL_DIV_160        = (int)0x000000A0, /* Divide by 160        */
    BLACKHAWK_TSC_PLL_DIV_165        = (int)0x000000A5, /* Divide by 165        */
    BLACKHAWK_TSC_PLL_DIV_168        = (int)0x000000A8, /* Divide by 168        */
    BLACKHAWK_TSC_PLL_DIV_170        = (int)0x000000AA, /* Divide by 170        */
    BLACKHAWK_TSC_PLL_DIV_175        = (int)0x000000AF, /* Divide by 175        */
    BLACKHAWK_TSC_PLL_DIV_178P895958 = (int)0xE55D80B2, /* Divide by 178.895958 */
    BLACKHAWK_TSC_PLL_DIV_180        = (int)0x000000B4, /* Divide by 180        */
    BLACKHAWK_TSC_PLL_DIV_184        = (int)0x000000B8, /* Divide by 184        */
    BLACKHAWK_TSC_PLL_DIV_192        = (int)0x000000C0, /* Divide by 192        */
    BLACKHAWK_TSC_PLL_DIV_198        = (int)0x000000C6, /* Divide by 198        */
    BLACKHAWK_TSC_PLL_DIV_200        = (int)0x000000C8, /* Divide by 200        */
    BLACKHAWK_TSC_PLL_DIV_224        = (int)0x000000E0, /* Divide by 224        */
    BLACKHAWK_TSC_PLL_DIV_240        = (int)0x000000F0, /* Divide by 240        */
    BLACKHAWK_TSC_PLL_DIV_264        = (int)0x00000108, /* Divide by 264        */
    BLACKHAWK_TSC_PLL_DIV_280        = (int)0x00000118, /* Divide by 280        */
    BLACKHAWK_TSC_PLL_DIV_330        = (int)0x0000014A, /* Divide by 330        */
    BLACKHAWK_TSC_PLL_DIV_350        = (int)0x0000015E  /* Divide by 350        */
};

/** TX AFE Settings Enum */
enum blackhawk_tsc_tx_afe_settings_enum {
    TX_AFE_TAP0,
    TX_AFE_TAP1,
    TX_AFE_TAP2,
    TX_AFE_TAP3,
    TX_AFE_TAP4,
    TX_AFE_TAP5,
    TX_AFE_TAP6,
    TX_AFE_TAP7,
    TX_AFE_TAP8,
    TX_AFE_TAP9,
    TX_AFE_TAP10,
    TX_AFE_TAP11
};

/** TXFIR Tap Enable Enum */
enum blackhawk_tsc_txfir_tap_enable_enum {
    NRZ_LP_3TAP  = 0,
    NRZ_6TAP     = 1,
    PAM4_LP_3TAP = 2,
    PAM4_6TAP    = 3
};

#define BLACKHAWK_TSC_PLL_OPTION_REFCLK_MASK 0xF

/** PLL Configuration Options Enum */
enum blackhawk_tsc_pll_option_enum {
    BLACKHAWK_TSC_PLL_OPTION_NONE              =  0,
    BLACKHAWK_TSC_PLL_OPTION_REFCLK_DOUBLER_EN =  1,
    BLACKHAWK_TSC_PLL_OPTION_REFCLK_DIV2_EN    =  2,
    BLACKHAWK_TSC_PLL_OPTION_REFCLK_DIV4_EN    =  3,
    BLACKHAWK_TSC_PLL_OPTION_DISABLE_VERIFY    = 16
};

#endif
