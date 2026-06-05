/*----------------------------------------------------------------------
 * $Id: tscpmod.h,
 * $Copyright: $
 *
 * $Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.$
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: define enumerators
 *----------------------------------------------------------------------*/
/*
 * $Id: $
 * $Copyright:
 * All Rights Reserved.$
 */

#ifndef _tscpmod_H_
#define _tscpmod_H_

#include <phymod/phymod.h>
#include <phymod/phymod_debug.h>
#include <phymod/phymod_diagnostics.h>

#include "tscpmod_spd_ctrl.h"
/*! 
 * @brief Timesync adjust flags for special cases. 
 */ 
#define TSCP_TIMESYNC_F_SOP 0x10 /**< Enable SOP timestamp mode, ignore MAC_DA bit. If disable, check MAC_DA mode bit. */
#define TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE 0x40 /**< Port is in reduced preamble mode. */
#define TSCP_TIMESYNC_F_MAC_DA 0x20 /**< Enable MAC_DA timestamp mode. Disabled, SFD timestamp mode will be used. */
#define TSCP_TIMESYNC_F_802_3_CX 0x8 /**< Enable -  802.3CX mode. Disabled - Legacy Broadcom mode will be used. */

#define TSCP_TIMESYNC_F_SOP_SET(flags) (flags |= TSCP_TIMESYNC_F_SOP)
#define TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE_SET(flags) (flags |= TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE)
#define TSCP_TIMESYNC_F_MAC_DA_SET(flags) (flags |= TSCP_TIMESYNC_F_MAC_DA)
#define TSCP_TIMESYNC_F_802_3_CX_SET(flags) (flags |= TSCP_TIMESYNC_F_802_3_CX)

#define TSCP_TIMESYNC_F_SOP_CLR(flags) (flags &= ~TSCP_TIMESYNC_F_SOP)
#define TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE_CLR(flags) (flags &= ~TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE)
#define TSCP_TIMESYNC_F_MAC_DA_CLR(flags) (flags &= ~TSCP_TIMESYNC_F_MAC_DA)
#define TSCP_TIMESYNC_F_802_3_CX_CLR(flags) (flags &= ~TSCP_TIMESYNC_F_802_3_CX)

#define TSCP_TIMESYNC_F_SOP_GET(flags) (flags & TSCP_TIMESYNC_F_SOP ? 1 : 0)
#define TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE_GET(flags) (flags & TSCP_TIMESYNC_F_REDUCED_PREAMBLE_MODE ? 1 : 0)
#define TSCP_TIMESYNC_F_MAC_DA_GET(flags) (flags & TSCP_TIMESYNC_F_MAC_DA ? 1 : 0)
#define TSCP_TIMESYNC_F_802_3_CX_GET(flags) (flags & TSCP_TIMESYNC_F_802_3_CX ? 1 : 0)


#define PHYMOD_APERTA2_TSCP_GET_OCTAL(LM)     (LM & 0xFF00) ? 1 : 0;
#define PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(OCT)  (OCT*8)
#define PHYMOD_ST plp_aperta2_phymod_phy_access_t

#define TSCPMOD_NOF_LANES_IN_CORE  8
#define TSCPMOD_AUTONEG_SPEED_ID_COUNT 48

#define TSCPMOD_FEC_NOT_SUPRTD         0
#define TSCPMOD_FEC_SUPRTD_NOT_REQSTD  1
#define TSCPMOD_FEC_CL74_SUPRTD_REQSTD 4
#define TSCPMOD_FEC_CL91_SUPRTD_REQSTD 8

#define TSCPMOD_CL73_ABILITY_50G_KR1_CR1_POSITION    13
#define TSCPMOD_CL73_ABILITY_100G_KR2_CR2_POSITION   14
#define TSCPMOD_CL73_ABILITY_200G_KR4_CR4_POSITION   15

#define TSCPMOD_VCO_NONE         0x0
#define TSCPMOD_VCO_41G          0x1
#define TSCPMOD_VCO_51G          0x2
#define TSCPMOD_VCO_53G          0x4
#define TSCPMOD_VCO_INVALID      0x8

#define TSCPMOD_HW_SPEED_ID_TABLE_SIZE   64
#define TSCPMOD_HW_AM_TABLE_SIZE    64
#define TSCPMOD_HW_UM_TABLE_SIZE    64


#define TSCPMOD_ID0                     0x600d
#define TSCPMOD_ID1                     0x8770
#define TSCPMOD_PHY_ALL_LANES           0xff
#define TSCPMOD_TX_TAP_NUM              12
#define TSCPMOD_FORCED_SPEED_ID_OFFSET  56

#define TSCPMOD_PMD_CRC_UCODE_VERIFY 1

#define TSCPMOD_SYNCE_SDM_DIVISOR_10G_PER_LANE        5280
#define TSCPMOD_SYNCE_SDM_DIVISOR_20G_PER_LANE        10560
#define TSCPMOD_SYNCE_SDM_DIVISOR_25G_PER_LANE        13200
#define TSCPMOD_SYNCE_SDM_DIVISOR_51G_VCO_PAM4        13200
#define TSCPMOD_SYNCE_SDM_DIVISOR_53G_VCO_PAM4        13600

#define TSCPMOD_FEC_OVERRIDE_BIT_SHIFT 0
#define TSCPMOD_FEC_OVERRIDE_MASK  0x1
#define TSCPMOD_PORT_AN_ENABLE_BIT_SHIFT 1
#define TSCPMOD_PORT_AN_ENABLE_MASK  0x2
#define TSCPMOD_PORT_ENABLE_BIT_SHIFT 2
#define TSCPMOD_PORT_ENABLE_MASK  0x4

#define TSCPMOD_IEEE_CL22_REG_ADDR        0x0003
#define TSCPMOD_PCS_REG_START_ADDR        0x9000
#define TSCPMOD_PCS_ONE_COPY_REG          0x9270
#define TSCPMOD_PCS_FOUR_COPY_REG_BLOCK1  0xc010
#define TSCPMOD_PCS_FOUR_COPY_REG_BLOCK2  0xc170
#define TSCPMOD_PCS_FOUR_COPY_REG_BLOCK3  0xc210

#define TSCPMOD_NOF_LANES_IN_MPP        4
#define TSCPMOD_MPP_NUM(lane)           (((lane / TSCPMOD_NOF_LANES_IN_MPP) < 2) ?  (lane / TSCPMOD_NOF_LANES_IN_MPP) : ((lane / TSCPMOD_NOF_LANES_IN_MPP) - 2))
#define TSCPMOD_MPP_LANE(lane)          (lane % TSCPMOD_NOF_LANES_IN_MPP)
#define TSCPMOD_MPP_LANE_MASK(lane)     (0x01 << (TSCPMOD_MPP_NUM(lane) * TSCPMOD_NOF_LANES_IN_MPP))

#define TSCP_TS_OFFSET_SOP          0
#define TSCP_TS_OFFSET_SFD          7
#define TSCP_TS_OFFSET_MAC_DA       8
#define TSCP_TS_RPM_OFFSET          4

#define TSCP_TS_RX_1588_TABLE_100G_2XN_START_LOCATION       160
#define TSCP_TS_TX_1588_TABLE_100G_2XN_START_LOCATION       0

#define TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM                  40


/* Rx Use the native addr. */
#define TSCP_TS_RX_1588_TABLE_100G_2XN_BASE_ADDRESS         80
#define TSCP_TS_TX_1588_TABLE_100G_2XN_BASE_ADDRESS         176

#define TSCPMOD_TS_RX_MPP_EVEN_BANK_NATIVE_TO_ADDR(na)      (na * 2)
#define TSCPMOD_TS_RX_MPP_ODD_BANK_NATIVE_TO_ADDR(na)       ((na * 2)+1)

#define TSCPMOD_TS_RX_MPP_MEM_ADDR_TO_NATIVE_ADDR(na)       (na / 2)

#define TSCPMOD_CORE_TO_PHY_ACCESS(_phy_access, _core_access) \
    do{\
        PHYMOD_MEMCPY(&(_phy_access)->access, &(_core_access)->access, sizeof((_phy_access)->access));\
        (_phy_access)->type           = (_core_access)->type; \
        (_phy_access)->port_loc       = (_core_access)->port_loc; \
        (_phy_access)->device_op_mode = (_core_access)->device_op_mode; \
    }while(0)

/* So far 4 bits debug mask are used by TSCPMOD */
#define TSCPMOD_DBG_MEM        (1L << 4) /* allocation/object */
#define TSCPMOD_DBG_REGACC     (1L << 3) /* Print all register accesses */
#define TSCPMOD_DBG_FUNCVALOUT (1L << 2) /* All values returned by Tier1*/
#define TSCPMOD_DBG_FUNCVALIN  (1L << 1) /* All values pumped into Tier1*/
#define TSCPMOD_DBG_FUNC       (1L << 0) /* Every time we enter a  Tier1*/

/* MACROs for AN PAGE definition*/
#define TSCPMOD_BRCM_OUI        0xAF7
#define TSCPMOD_BRCM_BAM_CODE   0x3
#define TSCPMOD_MSA_OUI         0x6A737D /* OUI defined for consortium 25G */
#define TSCPMOD_MSA_OUI_13to23  0x353
#define TSCPMOD_MSA_OUI_2to12   0x4DF
#define TSCPMOD_MSA_OUI_0to1    0x1


/* defines for the PMD over sample value */
#define TSCPMOD_OS_MODE_1                   0x0
#define TSCPMOD_OS_MODE_2                   0x1
#define TSCPMOD_OS_MODE_5                   0x3
#define TSCPMOD_OS_MODE_33                  0x11
#define TSCPMOD_OS_MODE_41p25               0x19
#define TSCPMOD_OS_MODE_42p5                0x21


/* Below macros are defined based on IEEE and MSA 25G spec
 * for detailed information Please refer to the
 * IEEE 802.3by section 73.6 Link codeword encoding
 * MSA Spec section 3.2.5 Auto-negotiation Figure 10
 */

/* 50GBASE_KR_CR, 100GBASE_KR2_CR2, 200GBASE_KR4_CR4
 * bit position is proposed in 802.3cb, but not finilized.
 * The corresponding Macro value may need revisit.
 */

/* Base page definitions */
#define TSCPMOD_AN_BASE0_PAGE_PAUSE_MASK   0x3
#define TSCPMOD_AN_BASE0_PAGE_PAUSE_OFFSET 0xA

#define TSCPMOD_AN_BASE0_PAGE_NP_MASK   0x1
#define TSCPMOD_AN_BASE0_PAGE_NP_OFFSET 0xF

#define TSCPMOD_AN_BASE1_TECH_ABILITY_10G_KR1_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_10G_KR1_OFFSET 0x7

#define TSCPMOD_AN_BASE1_TECH_ABILITY_40G_KR4_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_40G_KR4_OFFSET 0x8

#define TSCPMOD_AN_BASE1_TECH_ABILITY_40G_CR4_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_40G_CR4_OFFSET 0x9

#define TSCPMOD_AN_BASE1_TECH_ABILITY_100G_KR4_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_100G_KR4_OFFSET 0xC

#define TSCPMOD_AN_BASE1_TECH_ABILITY_100G_CR4_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_100G_CR4_OFFSET 0xD

#define TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_OFFSET 0xE

#define TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KR1_CR1_MASK   0x1
#define TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KR1_CR1_OFFSET 0xF

#define TSCPMOD_AN_BASE2_TECH_ABILITY_50G_KR1_CR1_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_50G_KR1_CR1_OFFSET 0x2

#define TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR2_CR2_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR2_CR2_OFFSET 0x3

#define TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR4_CR4_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR4_CR4_OFFSET 0x4

#define TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR1_CR1_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR1_CR1_OFFSET 0x5

#define TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR2_CR2_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR2_CR2_OFFSET 0x6

#define TSCPMOD_AN_BASE2_TECH_ABILITY_400G_KR4_CR4_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_400G_KR4_CR4_OFFSET 0x7

#define TSCPMOD_AN_BASE2_TECH_ABILITY_800G_KR8_CR8_MASK   0x1
#define TSCPMOD_AN_BASE2_TECH_ABILITY_800G_KR8_CR8_OFFSET 0x8
#define TSCPMOD_AN_BASE2_25G_RS_FEC_ABILITY_REQ_MASK   0x1
#define TSCPMOD_AN_BASE2_25G_RS_FEC_ABILITY_REQ_OFFSET 0xC

#define TSCPMOD_AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_MASK   0x1
#define TSCPMOD_AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_OFFSET 0xD

#define TSCPMOD_AN_BASE2_CL74_ABILITY_REQ_SUP_MASK   0x3
#define TSCPMOD_AN_BASE2_CL74_ABILITY_REQ_SUP_OFFSET 0xE

#define TSCPMOD_AN_BASE3_RS_FEC_544_2XN_MASK   0x1
#define TSCPMOD_AN_BASE3_RS_FEC_544_2XN_OFFSET 0xb

/* Message Page definitions */

#define TSCPMOD_AN_MSG_PAGE1_OUI_13to23_MASK 0x7FF
#define TSCPMOD_AN_MSG_PAGE1_OUI_13to23_OFFSET 0x0

#define TSCPMOD_AN_MSG_PAGE2_OUI_2to12_MASK   0x7FF
#define TSCPMOD_AN_MSG_PAGE2_OUI_2to12_OFFSET 0x0

/* Unformatted Page definitions */
#define TSCPMOD_AN_UF_PAGE0_UD_0to8_MASK 0x1FF
#define TSCPMOD_AN_UF_PAGE0_UD_0to8_OFFSET 0x0

#define TSCPMOD_AN_UF_PAGE0_OUI_MASK   0x3
#define TSCPMOD_AN_UF_PAGE0_OUI_OFFSET 0x9

/* Unformatted Page for CL73BAM only*/
#define TSCPMOD_AN_UF_PAGE1_BAM_20G_KR1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_20G_KR1_OFFSET 0x2

#define TSCPMOD_AN_UF_PAGE1_BAM_20G_CR1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_20G_CR1_OFFSET 0x3

#define TSCPMOD_AN_UF_PAGE1_BAM_25G_KR1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_25G_KR1_OFFSET 0x4

#define TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_OFFSET 0x5

#define TSCPMOD_AN_UF_PAGE1_BAM_40G_KR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_40G_KR2_OFFSET 0x6

#define TSCPMOD_AN_UF_PAGE1_BAM_40G_CR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_40G_CR2_OFFSET 0x7

#define TSCPMOD_AN_UF_PAGE1_BAM_50G_KR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_50G_KR2_OFFSET 0x8

#define TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_OFFSET 0x9

#define TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_OFFSET 0xA

#define TSCPMOD_AN_UF_PAGE1_BAM_100G_BRCM_CR1_KR1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_100G_BRCM_CR1_KR1_OFFSET 0xD

#define TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_OFFSET 0xE

#define TSCPMOD_AN_UF_PAGE1_BAM_800G_BRCM_CR8_KR8_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_BAM_800G_BRCM_CR8_KR8_OFFSET 0xF
#define TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_OFFSET 0x3

#define TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_OFFSET 0x4

#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_OFFSET 0x5

#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_OFFSET 0x7

#define TSCPMOD_AN_UF_PAGE2_BAM_CL91_REQ_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_CL91_REQ_OFFSET 0xB

#define TSCPMOD_AN_UF_PAGE2_BAM_CL74_REQ_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_CL74_REQ_OFFSET 0xC

#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_OFFSET 0xD

#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_OFFSET 0xE

/* Unformatted Page for MSA only*/
#define TSCPMOD_AN_UF_PAGE1_MSA_25G_KR1_ABILITY_MASK   0x1
#define TSCPMOD_AN_UF_PAGE1_MSA_25G_KR1_ABILITY_OFFSET 0x4

#define TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK   0x1
#define TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET 0x5

#define TSCPMOD_AN_UF_PAGE1_MSA_50G_KR2_ABILITY_MASK   0x1
#define TSCPMOD_AN_UF_PAGE1_MSA_50G_KR2_ABILITY_OFFSET 0x8

#define TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK   0x1
#define TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET 0x9

#define TSCPMOD_AN_UF_PAGE1_MSA_800G_CR8_KR8_MASK 0x1
#define TSCPMOD_AN_UF_PAGE1_MSA_800G_CR8_KR8_OFFSET 0xF
#define TSCPMOD_AN_UF_PAGE2_MSA_400G_ABILITY_MASK   0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_400G_ABILITY_OFFSET 0x2

#define TSCPMOD_AN_UF_PAGE2_MSA_LF1_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_LF1_OFFSET 0x5

#define TSCPMOD_AN_UF_PAGE2_MSA_LF2_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_LF2_OFFSET 0x6

#define TSCPMOD_AN_UF_PAGE2_MSA_LF3_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_LF3_OFFSET 0x7

#define TSCPMOD_AN_UF_PAGE2_MSA_CL91_SUPPORT_MASK   0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_CL91_SUPPORT_OFFSET 0x8

#define TSCPMOD_AN_UF_PAGE2_MSA_CL74_SUPPORT_MASK   0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_CL74_SUPPORT_OFFSET 0x9

#define TSCPMOD_AN_UF_PAGE2_MSA_CL91_REQ_MASK   0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_CL91_REQ_OFFSET 0xA

#define TSCPMOD_AN_UF_PAGE2_MSA_CL74_REQ_MASK   0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_CL74_REQ_OFFSET 0xB

#define TSCPMOD_AN_UF_PAGE2_MSA_LFR_MASK 0x1
#define TSCPMOD_AN_UF_PAGE2_MSA_LFR_OFFSET 0xC

/* UI macros definition. */
#define TSCPMOD_UI_41G_NRZ_OSX2         0x0C698000
#define TSCPMOD_UI_41G_NRZ_OSX4         0x18D30000

#define TSCPMOD_UI_51G_PAM4_OSX2        0x04F70000
#define TSCPMOD_UI_51G_PAM4_OSX1        0x027B8000
#define TSCPMOD_UI_51G_NRZ_OSX5         0x18D30000
#define TSCPMOD_UI_51G_NRZ_OSX2         0x09EE0000
#define TSCPMOD_UI_51G_NRZ_OSX1         0x04F70000

#define TSCPMOD_UI_53G_PAM4_OSX2        0x04D19E00
#define TSCPMOD_UI_53G_PAM4_OSX1        0x0268CF00
#define TSCPMOD_UI_53G_NRZ_OSX2         0x09A33C00
#define TSCPMOD_UI_53G_NRZ_OSX1         0x04D19E00

/* FCLK period macros definition. */
#define TSCPMOD_FCLK_PERIOD_41G_DIV8    0x634c
#define TSCPMOD_FCLK_PERIOD_41G_DIV6    0x4a79
#define TSCPMOD_FCLK_PERIOD_51G         0x4f70
#define TSCPMOD_FCLK_PERIOD_53G         0x4d19

/* PMD datapath latency. From Perigrine user spec v.0.83  datapath latency table. */
#define TSCPMOD_PMD_TX_DP_LATENCY                           344

#define TSCPMOD_PMD_RX_DP_LATENCY_NRZ_OS1                   808
#define TSCPMOD_PMD_RX_DP_LATENCY_NRZ_NON_OS1               448
#define TSCPMOD_PMD_RX_DP_LATENCY_PAM4_OS1                  808
#define TSCPMOD_PMD_RX_DP_LATENCY_PAM4_NON_OS1              448
#define TSCPMOD_FEC_MAX_CW_BLOCK_544_528   80
#define TSCPMOD_FEC_MAX_CW_BLOCK_272       40
#define TSCPMOD_FEC_MAX_CONTIGUOUS_BLOCK   31

typedef enum {
    TSCPMOD_AN_MODE_CL73 = 0,
    TSCPMOD_AN_MODE_CL73_BAM,
    TSCPMOD_AN_MODE_NONE,
    TSCPMOD_AN_MODE_CL73_MSA,
    TSCPMOD_AN_MODE_MSA,
    TSCPMOD_AN_MODE_TYPE_COUNT
}tscpmod_an_type_t;

typedef enum {
    TSCPMOD_NO_PAUSE = 0,
    TSCPMOD_SYMM_PAUSE,
    TSCPMOD_ASYM_PAUSE,
    TSCPMOD_ASYM_SYMM_PAUSE,
    TSCPMOD_AN_PAUSE_COUNT
}tscpmod_an_pause_t;

typedef enum {
    TSCPMOD_AN_TIMER_NONE = 0,
    TSCPMOD_AN_FAIL_INHIBIT_TIMER_LT_PAM4
}tscpmod_an_timer_t;

typedef enum {
    TSCPMOD_REF_CLK_156P25MHZ    = 1,
    TSCPMOD_REF_CLK_312P5MHZ     = 2,
    TSCPMOD_REF_CLK_COUNT
} tscpmod_refclk_t;

typedef struct tscpmod_an_control_s {
    tscpmod_an_type_t an_type;
    uint16_t num_lane_adv;
    uint16_t enable;
} tscpmod_an_control_t;

typedef enum {
    TSCPMOD_SPD_ZERO            = 0   ,  /*!< Illegal value (enum boundary)   */
    TSCPMOD_SPD_10G_IEEE_KR1          ,  /*!< 10Gb IEEE KR1                   */
    TSCPMOD_SPD_25G_IEEE_KS1_CS1      ,  /*!< 25Gb IEEE KS1/CS1               */
    TSCPMOD_SPD_25G_IEEE_KR1_CR1      ,  /*!< 25Gb IEEE KR1/CR1               */
    TSCPMOD_SPD_25G_BRCM_CR1          ,  /*!< 25Gb BRCM CR1                   */
    TSCPMOD_SPD_25G_BRCM_KR1          ,  /*!< 25Gb BRCM KR1                   */
    TSCPMOD_SPD_25G_BRCM_NO_FEC_KR1_CR1      ,  /*!< 25Gb BRCM NO FEC  KR1/CR1       */
    TSCPMOD_SPD_25G_BRCM_FEC_528_KR1_CR1     ,  /*!< 25Gb BRCM RS FEC  KR1/CR1       */
    TSCPMOD_SPD_50G_IEEE_KR1_CR1      ,  /*!< 50Gb IEEE KR1/CR1               */
    TSCPMOD_SPD_50G_BRCM_FEC_528_CR1_KR1      ,  /*!< 50Gb BRCM KR1/CR1               */
    TSCPMOD_SPD_50G_BRCM_FEC_272_CR1_KR1      ,  /*!< 50Gb BRCM KR1/CR1               */
    TSCPMOD_SPD_50G_BRCM_CR2_KR2_NO_FEC       ,  /*!< 50Gb BRCM KR2/CR2               */
    TSCPMOD_SPD_50G_BRCM_CR2_KR2_RS_FEC       ,  /*!< 50Gb BRCM RS528 KR2/Cr2         */
    TSCPMOD_SPD_50G_BRCM_FEC_544_CR2_KR2      ,  /*!< 50Gb BRCM RS544 KR2/Cr2         */
    TSCPMOD_SPD_100G_IEEE_KR4         ,  /*!< 100Gb serial XFI FEC RS528      */
    TSCPMOD_SPD_100G_IEEE_CR4         ,  /*!< 100Gb serial XFI FEC RS528      */
    TSCPMOD_SPD_100G_BRCM_NO_FEC_X4   ,  /*!< 100Gb serial XFI                */
    TSCPMOD_SPD_100G_BRCM_KR4_CR4     ,  /*!< 100Gb serial XFI                */
    TSCPMOD_SPD_CL73_IEEE_41G         ,  /*!< 1G CL73 Auto-neg                */
    TSCPMOD_SPD_CL73_IEEE_51G         ,  /*!< 1G CL73 Auto-neg                */
    TSCPMOD_SPD_CL73_IEEE_53G         ,  /*!< 1G CL73 Auto-neg                */
    TSCPMOD_SPD_100G_IEEE_KR2_CR2     ,  /*!< 100Gb serial IEEE 100G KR2/CR2  */
    TSCPMOD_SPD_100G_BRCM_NO_FEC_KR2_CR2       ,  /*!< 100Gb serial BRCM NO FEC 100G KR2/CR2  */
    TSCPMOD_SPD_100G_BRCM_FEC_528_KR2_CR2      ,  /*!< 100Gb serial BRCM RS FEC 100G KR2/CR2  */
    TSCPMOD_SPD_100G_BRCM_FEC_272_CR2_KR2      ,  /*!< 100Gb serial BRCM FEC 272 100G KR2/CR2 */
    TSCPMOD_SPD_100G_IEEE_KR1_CR1_OPT  ,  /*!< 100G PAM4 FEC                   */
    TSCPMOD_SPD_100G_IEEE_KR1_CR1      ,  /*!< 100G PAM4 FEC 544 2XN           */
    TSCPMOD_SPD_100G_BRCM_KR1_CR1      ,  /*!< 100G PAM4 FEC 544 1XN           */
    TSCPMOD_SPD_100G_BRCM_FEC_272_KR1_CR1      ,  /*!< 100G PAM4 FEC 272 1XN           */
    TSCPMOD_SPD_200G_IEEE_KR4_CR4      ,  /*!< 200G KR4/CR4                    */
    TSCPMOD_SPD_200G_BRCM_FEC_272_N4   ,  /*!< 200G KR4/CR4 BRCM FEC RS272 1xN */
    TSCPMOD_SPD_200G_BRCM_FEC_272_CR4_KR4           ,  /*!< 200G KR4/CR4 BRCM FEC RS272 2xN */
    TSCPMOD_SPD_200G_BRCM_FEC_544_CR8_KR8           ,  /*!< 200G KR8/CR8 BRCM FEC RS544 2xN */
    TSCPMOD_SPD_200G_BRCM_NO_FEC_KR4_CR4            ,  /*!< 200G KR4/CR4 with NO FEC  */
    TSCPMOD_SPD_200G_BRCM_KR4_CR4      ,  /*!< 200G KR4/CR4 with FEC544 1xN  */
    TSCPMOD_SPD_200G_IEEE_KR2_CR2      ,  /*!< 200G IEEE FEC 544 2XN         */
    TSCPMOD_SPD_200G_BRCM_FEC_272_KR2_CR2  ,  /*!< 200G FEC 272 2XN              */
    TSCPMOD_SPD_200G_BRCM_FEC_544_KR2_CR2  ,  /*!< 200G FEC 544 1XN              */
    TSCPMOD_SPD_200G_BRCM_FEC_272_N2       ,  /*!< 200G FEC 272 1XN              */
    TSCPMOD_SPD_400G_IEEE_X8           ,  /*!< 400G X8 IEEE FEC RS544 2xN    */
    TSCPMOD_SPD_400G_BRCM_FEC_KR8_CR8               ,  /*!< 400G X8 IEEE FEC RS544 2xN  */
    TSCPMOD_SPD_400G_BRCM_FEC_272_N8                ,  /*!< 400G X8 IEEE FEC RS272 2xN  */
    TSCPMOD_SPD_400G_IEEE_KR4_CR4                   ,  /*!< 400G X4 IEEE FEC RS544 2xN  */
    TSCPMOD_SPD_400G_BRCM_FEC_272_KR4_CR4           ,  /*!< 400G X4 BRCM FEC RS272 2xN  */
    TSCPMOD_SPD_800G_BRCM_KR8_CR8                   ,  /*!< 800G X8 BRCM FEC RS544 2xN  */
    TSCPMOD_SPD_800G_IEEE_KR8_CR8                   ,  /*!< 800G X8 IEEE FEC RS544 2xN  */
    TSCPMOD_SPD_CUSTOM_ENTRY_56                     ,  /*!< Custom Entry at 56          */
    TSCPMOD_SPD_CUSTOM_ENTRY_57                     ,  /*!< Custom Entry at 57          */
    TSCPMOD_SPD_CUSTOM_ENTRY_58                     ,  /*!< Custom Entry at 58          */
    TSCPMOD_SPD_CUSTOM_ENTRY_59                     ,  /*!< Custom Entry at 59          */
    TSCPMOD_SPD_CUSTOM_ENTRY_60                     ,  /*!< Custom Entry at 60          */
    TSCPMOD_SPD_CUSTOM_ENTRY_61                     ,  /*!< Custom Entry at 61          */
    TSCPMOD_SPD_CUSTOM_ENTRY_62                     ,  /*!< Custom Entry at 62          */
    TSCPMOD_SPD_CUSTOM_ENTRY_63                     ,  /*!< Custom Entry at 63          */
    TSCPMOD_SPD_ILLEGAL                                /*!< Illegal value (enum boundary)*/
} tscpmod_spd_intfc_type_t;

typedef enum {
    TSCPMOD_PLL_MODE_DIV_ZERO = 0, /* Divide value to be determined by API. */
    TSCPMOD_PLL_MODE_DIV_132        =          (int)0x00000084, /* Divide by 132       */
    TSCPMOD_PLL_MODE_DIV_165        =          (int)0x000000A5, /* Divide by 165       */
    TSCPMOD_PLL_MODE_DIV_170        =          (int)0x000000AA  /* Divide by 170       */
} tscpmod_pll_mode_type;

typedef enum phymod_interrupt_type_e {
    phymodIntrNone = 0x0,
    phymodIntrEccRx1588400g, /**< RX 1588 400G ecc error */
    phymodIntrEccRx1588Mpp1, /**< RX 1588 Mpp1 ecc error */
    phymodIntrEccRx1588Mpp0, /**< RX 1588 Mpp0 ecc error */
    phymodIntrEccTx1588400g, /**< TX 1588 400G ecc error */
    phymodIntrEccTx1588Mpp1, /**< TX 1588 Mpp1 ecc error */
    phymodIntrEccTx1588Mpp0, /**< TX 1588 Mpp0 ecc error */
    phymodIntrEccUMTable, /**< Unique Marker table ecc error */
    phymodIntrEccAMTable, /**< Alignment Marker table ecc error */
    phymodIntrEccSpeedTable, /**< Speed table ecc error */
    phymodIntrEccDeskew, /**< Deskew ecc error */
    phymodIntrEccRsFECRs400gMpp1, /**< RsFEC_Rs400g Mpp1 ecc error */
    phymodIntrEccRsFECRs400gMpp0, /**< RsFEC_Rs400g Mpp0 ecc error */
    phymodIntrEccRsFECRbufMpp1, /**< RsFEC_RBUF MPP1 ecc error */
    phymodIntrEccRsFECRbufMpp0, /**< RsFEC_RBUF MPP0 ecc error */
    phymodIntrEccBaseRFEC, /**< BaseR FEC ecc error */
    phymodIntrEccRsFECMpp1, /**< Mpp1 RsFEC memory ecc error */
    phymodIntrEccRsFECMpp0, /**< Mpp0 RsFEC memory ecc error */
    phymodIntrRsFecFdr, /**< RsFEC Flight Data Recorder. */
    phymodIntrEccDslDataFifo, /**< Dsl Data Fifo ecc error */
    phymodIntrEccTx1588_2Mpp0, /**< TX 1588 2  Mpp0 ecc error */
    phymodIntrEccRx1588_2Mpp1, /**< RX 1588 2 Mpp1 ecc error */
    phymodIntrEccRx1588_2Mpp0, /**< RX 1588 2 Mpp0 ecc error */
    phymodIntrCount
} phymod_interrupt_type_t;


typedef struct tscpmod_intr_status_s {
    phymod_interrupt_type_t type;
    uint8_t is_1b_err;
    uint8_t is_2b_err;
    int err_addr;
    /* Indicates interrupt is set for non-ECC interrupt type. */
    uint8_t non_ecc_intr_set;
    /* SW AN status */
    uint8_t lp_page_rdy;
} tscpmod_intr_status_t;

typedef enum {
    TSCPMOD_RS_FEC_CW_TYPE_544  = 0,        /* CW type 544  */
    TSCPMOD_RS_FEC_CW_TYPE_272,             /* CW type 272 */
    TSCPMOD_RS_FEC_CW_TYPE_COUNT
} tscpmod_rs_fec_cw_type;

typedef enum {
  TSCPMOD_QUAD_PORT          = 0    ,  /*!< Each channel is one logical port */
  TSCPMOD_TRI1_PORT                 ,  /*!< 3 ports, one of them paird as follows (0,1,2-3) */
  TSCPMOD_TRI2_PORT                 ,  /*!< 3 ports, one of them paird as follows (0-1,2,3) */
  TSCPMOD_DUAL_PORT                 ,  /*!< Each paired channel(0-1, 2-3) is one logical port */
  TSCPMOD_SINGLE_PORT               ,  /*!< single port mode: this is also true for 400G 8 lane */
  TSCPMOD_MULTI_MPP_PORT            ,  /*!< multi Mpp port mode: 8 channels as one logical port */
  TSCPMOD_PORT_MODE_ILLEGAL           /*!< Illegal value (enum boundary) */
} tscpmod_port_type_t;

/*
 * Default prbs13 seed value
 * The default value of these register bits for each lane is unique
 */
#define LINKTRN_DEFAULT_SEED_0        0x1AA0
#define LINKTRN_DEFAULT_SEED_1        0x105C
#define LINKTRN_DEFAULT_SEED_2        0x0689
#define LINKTRN_DEFAULT_SEED_3        0x0822
#define LINKTRN_DEFAULT_SEED_4        0x0CBF
#define LINKTRN_DEFAULT_SEED_5        0x0EE3
#define LINKTRN_DEFAULT_SEED_6        0x02C0
#define LINKTRN_DEFAULT_SEED_7        0x1C8C

/* Bit error mask to generate FEC error. */
typedef struct tscpmod_fec_error_mask_s {
    uint32_t error_mask_bit_31_0;     /* Error mask bit 31-0. */
    uint32_t error_mask_bit_63_32;    /* Error mask bit 63-32. */
    uint8_t error_mask_bit_67_64;    /* Error mask bit 67-64. */
} tscpmod_fec_error_mask_t;

/*!
 * @enum phymod_tscbh_pll_multiplier_e
 * @brief tscbh_pll_multiplier  
 */ 
typedef enum phymod_tscbh_pll_multiplier_e {
    phymod_TSCBH_PLL_DIVNONE = (int32_t) 0xffffffff, /**< PLL is not used */
    phymod_TSCBH_PLL_DIV66 = 0x00000042, /**< Multiply ref. clk by 66 */
    phymod_TSCBH_PLL_DIV67 = 0x00000043, /**< Multiply ref. clk by 67 */
    phymod_TSCBH_PLL_DIV70 = 0x00000046, /**< Multiply ref. clk by 70 */
    phymod_TSCBH_PLL_DIV72 = 0x00000048, /**< Multiply ref. clk by 72 */
    phymod_TSCBH_PLL_DIV73P6 = (int32_t) 0x99998049, /**< Multiply ref. clk by 73.6 */
    phymod_TSCBH_PLL_DIV79P2 = 0x3333304F, /**< Multiply ref. clk by 79.2 */
    phymod_TSCBH_PLL_DIV80 = 0x00000050, /**< Multiply ref. clk by 80 */
    phymod_TSCBH_PLL_DIV82P5 = (int32_t) 0x80000052, /**< Multiply ref. clk by 82.5 */
    phymod_TSCBH_PLL_DIV84 = 0x00000054, /**< Multiply ref. clk by 84 */
    phymod_TSCBH_PLL_DIV85 = 0x00000055, /**< Multiply ref. clk by 85 */
    phymod_TSCBH_PLL_DIV87P5 = (int32_t) 0x80000057, /**< Multiply ref. clk by 87.5 */
    phymod_TSCBH_PLL_DIV89P6 = (int32_t) 0x9999A059, /**< Multiply ref. clk by 89.6 */
    phymod_TSCBH_PLL_DIV90 = 0x0000005A, /**< Multiply ref. clk by 90 */
    phymod_TSCBH_PLL_DIV96 = 0x00000060, /**< Multiply ref. clk by 92 */
    phymod_TSCBH_PLL_DIV100 = 0x00000064, /**< Multiply ref. clk by 100 */
    phymod_TSCBH_PLL_DIV120 = 0x00000078, /**< Multiply ref. clk by 120 */
    phymod_TSCBH_PLL_DIV127P4 = 0x66E8707F, /**< Multiply ref. clk by 127.4 */
    phymod_TSCBH_PLL_DIV128 = 0x00000080, /**< Multiply ref. clk by 128 */
    phymod_TSCBH_PLL_DIV132 = 0x00000084, /**< Multiply ref. clk by 132 */
    phymod_TSCBH_PLL_DIV140 = 0x0000008C, /**< Multiply ref. clk by 140 */
    phymod_TSCBH_PLL_DIV144 = 0x00000090, /**< Multiply ref. clk by 144 */
    phymod_TSCBH_PLL_DIV147P2 = 0x33330093, /**< Multiply ref. clk by 147.2 */
    phymod_TSCBH_PLL_DIV158P4 = 0x6666609E, /**< Multiply ref. clk by 158.4 */
    phymod_TSCBH_PLL_DIV160 = 0x000000A0, /**< Multiply ref. clk by 160 */
    phymod_TSCBH_PLL_DIV165 = 0x000000A5, /**< Multiply ref. clk by 165 */
    phymod_TSCBH_PLL_DIV168 = 0x000000A8, /**< Multiply ref. clk by 168 */
    phymod_TSCBH_PLL_DIV170 = 0x000000AA, /**< Multiply ref. clk by 170 */
    phymod_TSCBH_PLL_DIV175 = 0x000000AF, /**< Multiply ref. clk by 175 */
    phymod_TSCBH_PLL_DIV180 = 0x000000B4, /**< Multiply ref. clk by 180 */
    phymod_TSCBH_PLL_DIV184 = 0x000000B8, /**< Multiply ref. clk by 184 */
    phymod_TSCBH_PLL_DIV192 = 0x000000C0, /**< Multiply ref. clk by 192 */
    phymod_TSCBH_PLL_DIV198 = 0x000000C6, /**< Multiply ref. clk by 198 */
    phymod_TSCBH_PLL_DIV200 = 0x000000C8, /**< Multiply ref. clk by 200 */
    phymod_TSCBH_PLL_DIV224 = 0x000000E0, /**< Multiply ref. clk by 224 */
    phymod_TSCBH_PLL_DIV240 = 0x000000F0, /**< Multiply ref. clk by 240 */
    phymod_TSCBH_PLL_DIV264 = 0x00000108, /**< Multiply ref. clk by 264 */
    phymod_TSCBH_PLL_DIV280 = 0x00000118, /**< Multiply ref. clk by 280 */
    phymod_TSCBH_PLL_DIV179P6875327 = (int32_t) 0xB00240B3, /**< Multiply ref. clk by 179.6875327 */
    phymod_TSCBH_PLL_DIV141P4285714 = (int32_t) 0x6DB6C08D, /**< Multiply ref. clk by 141.4285714 */
    phymod_TSCBH_PLL_DIV171P347605 = (int32_t) 0x58FCC0AB, /**< Multiply ref. clk by 171.347605 */
    phymod_TSCBH_PLL_DIV176P7857143 = (int32_t) 0xC92480B0, /**< Multiply ref. clk by 176.7857143 */
    phymod_TSCBH_PLL_DIV177P5316456 = (int32_t) 0x881A00B1, /**< Multiply ref. clk by 177.5316456 */
    phymod_TSCBH_PLL_DIV178P8955584 = (int32_t) 0xE54340B2, /**< Multiply ref. clk by 178.8955584 */
    phymod_TSCBH_PLL_Count
} phymod_tscbh_pll_multiplier_t;

typedef struct tscpmod_ts_tx_info_s {
    uint32_t ts_in_fifo_lo; /**< low 32bit of Timestamp in Fifo */
    uint32_t ts_in_fifo_hi; /**< high 32bit of Timestamp in Fifo */
    uint32_t ts_seq_id; /**< sequence id of tx 1588 packet */
    uint32_t ts_sub_nanosec; /**< sub nanoseconds of tx 1588 packet */
} tscpmod_ts_tx_info_t;

extern int plp_aperta2_tscpmod_model_num_get(PHYMOD_ST* pc, uint32_t* model_num);
extern int plp_aperta2_tscpmod_speed_id_get(PHYMOD_ST* pc, int *speed_id);
extern int plp_aperta2_tscpmod_enable_get(PHYMOD_ST* pc, uint32_t* enable);
extern int plp_aperta2_tscpmod_disable_set(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_enable_set(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_set_an_timers(PHYMOD_ST* pc, plp_aperta2_phymod_ref_clk_t refclk, uint32_t *pam4_an_timer_value);
extern int plp_aperta2_tscpmod_get_pcs_latched_link_status(PHYMOD_ST* pc, uint32_t *link);
extern int plp_aperta2_tscpmod_rsfec_cw_type_set(PHYMOD_ST* pc, tscpmod_rs_fec_cw_type cw_type, int fec_bypass_correction);
extern int plp_aperta2_tscpmod_osts_pipeline(PHYMOD_ST* pc, uint32_t en);
extern int plp_aperta2_tscpmod_osts_pipeline_get(PHYMOD_ST* pc, uint32_t* en);
extern int plp_aperta2_tscpmod_pcs_rx_lane_swap(PHYMOD_ST* pc, int rx_lane_swap);
extern int plp_aperta2_tscpmod_pcs_tx_lane_swap(PHYMOD_ST* pc, int tx_lane_swap);
extern int plp_aperta2_tscpmod_pcs_rx_scramble_idle_en(PHYMOD_ST* pc, int en);
extern int plp_aperta2_tscpmod_synce_mode_set(PHYMOD_ST* pc, int stage0_mode, int stage1_mode);
extern int plp_aperta2_tscpmod_synce_mode_get(PHYMOD_ST* pc, int* stage0_mode, int* stage1_mode);
extern int plp_aperta2_tscpmod_synce_clk_ctrl_set(PHYMOD_ST* pc, uint32_t val);
extern int plp_aperta2_tscpmod_synce_clk_ctrl_get(PHYMOD_ST* pc, uint32_t* val);
extern int plp_aperta2_tscpmod_resolved_port_mode_get(PHYMOD_ST* pc, uint32_t* port_mode);
extern int plp_aperta2_tscpmod_revid_get(PHYMOD_ST* pc, uint32_t* rev_id);
extern int plp_aperta2_tscpmod_fec_arch_decode_get(int fec_arch, phymod_fec_type_t* fec_type);
extern int plp_aperta2_tscpmod_fec_align_status_get(PHYMOD_ST* pc, uint32_t* fec_align_live);
extern int plp_aperta2_tscpmod_fec_override_set(PHYMOD_ST* pc, uint32_t enable);
extern int plp_aperta2_tscpmod_fec_override_get(PHYMOD_ST* pc, uint32_t* enable);
extern int plp_aperta2_tscpmod_fec_correctable_counter_get(PHYMOD_ST* pc, int speed, uint32_t* count);
extern int plp_aperta2_tscpmod_fec_uncorrectable_counter_get(PHYMOD_ST* pc, int speed, uint32_t* count);
extern int plp_aperta2_tscpmod_pmd_reset_seq(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_set_an_port_mode(PHYMOD_ST* pc, int starting_lane);
extern int plp_aperta2_tscpmod_update_port_mode(PHYMOD_ST *pc, uint32_t data_rate);
extern int plp_aperta2_tscpmod_refclk_set(PHYMOD_ST* pc, tscpmod_refclk_t ref_clk);
extern int plp_aperta2_tscpmod_refclk_get(PHYMOD_ST* pc, tscpmod_refclk_t* ref_clk);
extern int plp_aperta2_tscpmod_pmd_x4_reset(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_set_sc_speed(PHYMOD_ST* pc, int mapped_speed, int set_sw_speed_change);
extern int plp_aperta2_tscpmod_autoneg_control(PHYMOD_ST* pc, tscpmod_an_control_t *an_control);
extern int plp_aperta2_tscpmod_autoneg_control_get(PHYMOD_ST* pc, tscpmod_an_control_t *an_control, int *an_complete);
extern int plp_aperta2_tscpmod_autoneg_status_get(PHYMOD_ST* pc, int *an_en, int *an_done);
extern int plp_aperta2_tscpmod_autoneg_fec_status_get(PHYMOD_ST* pc, uint8_t *fec_status);
extern int plp_aperta2_tscpmod_autoneg_ability_set(PHYMOD_ST* pc,
                               const phymod_autoneg_advert_abilities_t* autoneg_abilities);
extern int plp_aperta2_tscpmod_autoneg_ability_base_100g_fec_sel_set(PHYMOD_ST* pc,
                               const phymod_autoneg_advert_abilities_t* autoneg_abilities);
extern int plp_aperta2_tscpmod_autoneg_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t* autoneg_abilities);
extern int plp_aperta2_tscpmod_autoneg_remote_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t *autoneg_abilities);
extern int plp_aperta2_tscpmod_pll_to_vco_get(tscpmod_refclk_t ref_clock, uint32_t pll, uint32_t *vco);
extern int tscpmod_pcs_reset_sw_war(const PHYMOD_ST *pc);
extern int plp_aperta2_tscpmod_plldiv_lkup_get(PHYMOD_ST* pc, int mapped_speed_id, tscpmod_refclk_t refclk,  uint32_t *plldiv);
extern int plp_aperta2_tscpmod_pmd_rx_lock_override_enable(PHYMOD_ST* pc, uint32_t enable);
extern int plp_aperta2_tscpmod_polling_for_sc_done(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_read_sc_done(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_read_sc_fsm_status(PHYMOD_ST* pc);
extern int plp_aperta2_tscpmod_pcs_clk_blk_en(const PHYMOD_ST* pc, uint32_t en);
extern int plp_aperta2_tscpmod_port_start_lane_get(PHYMOD_ST *pc, int *port_starting_lane, int *port_num_lane);
extern int plp_aperta2_tscpmod_fec_bypass_indication_set(PHYMOD_ST* pc, uint32_t rsfec_bypass_indication);
extern int plp_aperta2_tscpmod_fec_bypass_indication_get(PHYMOD_ST *pc, uint32_t *rsfec_bypass_indication);
extern int plp_aperta2_tscpmod_vco_to_pll_lkup(uint32_t vco, tscpmod_refclk_t refclk, uint32_t* pll_div);
extern int plp_aperta2_tscpmod_fec_cobra_enable(PHYMOD_ST *pc, uint32_t enable);
extern int plp_aperta2_tscpmod_pcs_ts_config(PHYMOD_ST *pc, int ts_offset,
                           int rx_ts_base_addr, int tx_ts_base_addr);
extern int plp_aperta2_tscpmod_pcs_rx_ts_en(PHYMOD_ST* pc, uint32_t en);
extern int plp_aperta2_tscpmod_pcs_rx_ts_en_get(PHYMOD_ST* pc, uint32_t* en);
extern int plp_aperta2_tscpmod_pcs_1588_ts_offset_set(PHYMOD_ST *pc, uint32_t ns_offset, uint32_t sub_ns_offset);
extern int plp_aperta2_tscpmod_pcs_1588_ts_offset_get(PHYMOD_ST *pc, uint32_t *ns_offset, uint32_t *sub_ns_offset);
extern int plp_aperta2_tscpmod_ts_offset_rx_set(PHYMOD_ST* pc, int tbl_ln, uint32_t *table);
extern int plp_aperta2_tscpmod_pcs_rx_deskew_en(PHYMOD_ST *pc, int en);
#ifdef SERDES_API_FLOATING_POINT
extern int tscpmod_set_pmd_timer_offset(PHYMOD_ST *pc, float ts_clk_period);
#endif
extern int plp_aperta2_tscpmod_set_fclk_period (PHYMOD_ST *pc, uint32_t vco, int clk4sync_div);
extern int plp_aperta2_tscpmod_pcs_set_1588_ui(PHYMOD_ST *pc, uint32_t vco, int os_mode, int pam4);
extern int plp_aperta2_tscpmod_tx_pmd_latency_get(PHYMOD_ST *pc, int *tx_latency);
extern int plp_aperta2_tscpmod_tx_pmd_latency_set(PHYMOD_ST *pc, uint32_t vco, int os_mode, int pam4);
extern int plp_aperta2_tscpmod_pcs_set_tx_lane_skew_capture(PHYMOD_ST *pc, int tx_skew_en);
extern int plp_aperta2_tscpmod_pcs_measure_tx_lane_skew (PHYMOD_ST *pc, uint32_t vco, int os_mode, int pam4,
                                             int pma_width_multiplier, int normalize_to_latest, int *tx_max_skew);
extern int plp_aperta2_tscpmod_measure_n_normalize_tx_lane_skew(PHYMOD_ST *pc, uint32_t vco, int os_mode, int is_pam4, int fixed_lane, int *tx_max_skew);
extern int plp_aperta2_tscpmod_mod_rx_1588_tbl_val(PHYMOD_ST *pc, int bit_mux_mode, uint32_t vco, int os_mode, int is_pam4, int normalize_to_latest, uint32_t *rx_max_skew, uint32_t *rx_min_skew, uint32_t *skew_per_vl, uint32_t *rx_dsl_sel, uint32_t *rx_psll_sel);
extern int plp_aperta2_tscpmod_timesync_rx_deskew_info_dump(PHYMOD_ST *pc, int bit_mux_mode, uint32_t vco, int os_mode, int is_pam4, int normalize_to_latest);
extern int plp_aperta2_tscpmod_chk_rx_ts_deskew_valid(PHYMOD_ST *pc, int bit_mux_mode, int *rx_ts_deskew_valid);
extern int plp_aperta2_tscpmod_rsfec_symbol_error_counter_get(PHYMOD_ST* pc,
                                          int bit_mux_mode,
                                          int max_count,
                                          int* actual_count,
                                          uint32_t* symb_err_cnt);
extern int plp_aperta2_tscpmod_intr_status_get(PHYMOD_ST* pc, tscpmod_intr_status_t* intr_status);
extern int plp_aperta2_tscpmod_tx_ts_info_unpack_tx_ts_tbl_entry(uint32_t *tx_ts_tbl_entry, tscpmod_ts_tx_info_t *tx_ts_info);
extern int plp_aperta2_tscpmod_rs_fec_hi_ser_get(PHYMOD_ST* pc, uint32_t* hi_ser_lh, uint32_t* hi_ser_live);
extern int plp_aperta2_tscpmod_pmd_osmode_set(PHYMOD_ST* pc, int mapped_speed_id, tscpmod_refclk_t refclk);
extern int plp_aperta2_tscpmod_port_an_mode_enable_set(PHYMOD_ST* pc,int enable);
extern int plp_aperta2_tscpmod_port_an_mode_enable_get(PHYMOD_ST* pc, int* enable);
extern int plp_aperta2_tscpmod_port_cl73_enable_set(PHYMOD_ST* pc, int enable);
extern int plp_aperta2_tscpmod_port_cl73_enable_get(PHYMOD_ST* pc, int* enable);
extern int plp_aperta2_tscpmod_port_enable_set(PHYMOD_ST* pc, int enable);
extern int plp_aperta2_tscpmod_port_enable_get(PHYMOD_ST* pc, int* enable);
extern int tscpmod_pcs_reg_num_copy_get(uint32_t reg_addr, int *num_copy);
#ifdef APERTA2_PM_UNSUPPORTED_API
extern int tscpmod_pmd_override_enable_set(PHYMOD_ST* pc,
                                        phymod_override_type_t pmd_override_type,
                                        uint32_t override_enable,
                                        uint32_t override_val);
#endif
extern int plp_aperta2_tscpmod_pmd_tx_pcs_delay_cnt_set(PHYMOD_ST* pc, uint32_t delay_cnt);
extern int
plp_aperta2_tscpmod_interrupt_enable_set(PHYMOD_ST *pc,
                             phymod_interrupt_type_t intr_type,
                             uint32_t enable);
extern int
plp_aperta2_tscpmod_interrupt_enable_get(PHYMOD_ST *pc,
                             phymod_interrupt_type_t intr_type,
                             uint32_t *enable);
extern int plp_aperta2_tscpmod_1588_ts_valid_get(PHYMOD_ST* pc, uint16_t* ts_valid);
#if 0
extern int tscpmod_fec_error_inject_config_set(PHYMOD_ST *pc,
                                               const phymod_fec_error_injection_config_t *error_injection_config);
extern int tscpmod_fec_error_inject_config_get(PHYMOD_ST *pc,
                                               phymod_fec_error_injection_config_t *error_injection_config);
#endif
extern int plp_aperta2_tscpmod_autoneg_ability_400g_8lane_get(PHYMOD_ST* pc, uint32_t *enabled);
extern int plp_aperta2_tscpmod_autoneg_ability_400g_4lane_get(PHYMOD_ST* pc, uint32_t *enabled);

extern int plp_aperta2_tscpmod_an_link_fail_inhibit_timer_set(PHYMOD_ST* pc,
                                                  tscpmod_an_timer_t timer_type,
                                                  uint32_t period);
extern int plp_aperta2_tscpmod_an_link_fail_inhibit_timer_get(PHYMOD_ST* pc,
                                                  tscpmod_an_timer_t timer_type,
                                                  uint32_t* period);
extern int plp_aperta2_tscpmod_fec_error_bits_counter_get(PHYMOD_ST* pc, uint32_t speed, uint32_t* count);
#if 0
extern int tscpmod_fec_error_inject_enable_set(PHYMOD_ST* pc, const phymod_fec_error_injection_enable_t *error_injection_enable);
extern int tscpmod_fec_error_inject_enable_get(PHYMOD_ST* pc, phymod_fec_error_injection_enable_t *error_injection_enable);
#endif
extern int plp_aperta2_tscpmod_bip_error_counter_get(PHYMOD_ST* pc, uint32_t* count);
extern int plp_aperta2_tscpmod_cl49_ber_counter_get(PHYMOD_ST* pc, uint32_t* count);
extern int plp_aperta2_tscpmod_cl82_ber_counter_get(PHYMOD_ST* pc, uint32_t* count);
#if 0
extern int tscpmod_rsfec_symbol_error_mem_get(PHYMOD_ST* pc,
                                              int bit_mux_mode,
                                              phymod_rsfec_symbol_err_mem_info_t* sym_err_mem_info);
#endif
#endif  /*  _tscpmod_H_ */
