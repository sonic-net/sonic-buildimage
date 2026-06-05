/*
 *
 * $Id: barchetta_cfg_seq.h Exp $
 *
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 *
 *
 *
 */

#ifndef __BARCHETTA_CFG_SEQ_H__
#define __BARCHETTA_CFG_SEQ_H__

#include <phymod/phymod.h>
#include "barchetta_msg_interface.h"
#include "blackhawk_barchetta_interface.h"
#include "blackhawk_barchetta_functions.h"
#include "blackhawk_barchetta_internal.h"

/* Enable Quad based PLL configuration
   In Quad based PLL configuration,
    - Lanes 0 to 3 will use PLL0 and lanes 4 to 7 will use PLL1
    - Mixed speed on the same quad is not allowed
*/
/* #define QUAD_PLL_CONFIG */

#define BARCHETTA_NUM_OF_PORTS  BARCHETTA_MAX_NUM_DUPLEX_PORTS
#define BARCHETTA_NUM_OF_LANES  BARCHETTA_MAX_NUM_DUPLEX_LANES

#define BARCHETTA_NUM_OF_PHY     1024

#define m_BARCHETTA_SET_BIT(num, bp) (num |= (1<<bp))
#define m_BARCHETTA_MAX(x,y) ( (x > y)? x : y )

#define BARCHETTA_CHIP_81337        0x81337
#define BARCHETTA_CHIP_81338        0x81338
#define BARCHETTA_CHIP_81381        0x81381
#define BARCHETTA_CHIP_81321        0x81321
#define BARCHETTA_CHIP_81764        0x81764
#define BARCHETTA_CHIP_REV_A0       0xA0
#define BARCHETTA_CHIP_REV_INTERNAL 0xB0

#define BARCHETTA_PLL_INDEX_0        0x0
#define BARCHETTA_PLL_INDEX_1        0x1
#define BARCHETTA_PLL_DIV_66         0x1
#define BARCHETTA_PLL_DIV_70         0x2
#define BARCHETTA_PLL_DIV_80         0x3
#define BARCHETTA_PLL_DIV_82P5       0x4
#define BARCHETTA_PLL_DIV_85         0x5
#define BARCHETTA_PLL_DIV_87P5       0x6
#define BARCHETTA_PLL_DIV_90         0x7
#define BARCHETTA_PLL_DIV_128        0x8
#define BARCHETTA_PLL_DIV_132        0x9
#define BARCHETTA_PLL_DIV_140        0xa
#define BARCHETTA_PLL_DIV_160        0xb
#define BARCHETTA_PLL_DIV_165        0xc
#define BARCHETTA_PLL_DIV_170        0xd
#define BARCHETTA_PLL_DIV_175        0xE
#define BARCHETTA_PLL_DIV_180        0xF
#define BARCHETTA_PLL_DIV_264        0x10
#define BARCHETTA_PLL_DIV_280        0x11
#define BARCHETTA_PLL_DIV_330        0x12
#define BARCHETTA_PLL_DIV_350        0x13
#define BARCHETTA_PLL_DIV_96         0x14
#define BARCHETTA_PLL_DIV_144        0x15
#define BARCHETTA_PLL_DIV_168        0x16
#define BARCHETTA_PLL_DIV_184        0x17
#define BARCHETTA_PLL_DIV_198        0x18
#define BARCHETTA_PLL_DIV_200        0x19
#define BARCHETTA_PLL_DIV_120        0x1A
#define BARCHETTA_PLL_DIV_148        0x1B
#define BARC_REG_DUMP_MOD_CNT        7

/* The following SWGPREG are used to save software database status information */
#define BARCHETTA_SW_DB_LANES_CONFIG_SYS_STATUS_REG_ADDR        BCMI_BARCHETTA_CTRL_SWGPREGBr /* 0x0001859b */
#define BARCHETTA_SW_DB_PORT_CONFIG_SYS_STATUS_REG_ADDR         BCMI_BARCHETTA_CTRL_SWGPREGCr /* 0x0001859c */
#define BARCHETTA_SW_DB_PMD_CONFIG_SYS_STATUS_REG_ADDR          BCMI_BARCHETTA_CTRL_SWGPREGDr /* 0x0001859d */
#define BARCHETTA_SW_DB_PORT_STATUS_REG_ADDR                    BCMI_BARCHETTA_CTRL_SWGPREGEr /* 0x0001859e */

/* The following defines are used to save lane data rate in GPREG */

/* Note : line_lane_0_1_data_rate @ 0x0001b000
 *        line_lane_2_3_data_rate @ 0x0001b001
 *        line_lane_4_5_data_rate @ 0x0001b002
 *        line_lane_6_7_data_rate @ 0x0001b003
 *        sys_lane_0_1_data_rate  @ 0x0001b004
 *        sys_lane_2_3_data_rate  @ 0x0001b005
 *        sys_lane_4_5_data_rate  @ 0x0001b006
 *        sys_lane_6_7_data_rate  @ 0x0001b007
 */
#define BARCHETTA_LANE_DATA_RATE_SAVE_GPREG_BASE_ADDR        0x0001b000
#define BARCHETTA_LANE_DATA_RATE_STORAGE_MASK                0xFF
#define BARCHETTA_LINE_SYS_LANE_DATA_RATE_SAVE_GPREG_OFFSET  4


/* The following defines are used to save interface type in SWGPREG */

/* Note : line_lane_0_to_3_if_type @ 0x18590 [BCMI_BARCHETTA_CTRL_SWGPREG0r]
 *        line_lane_4_to_7_if_type @ 0x18591 [BCMI_BARCHETTA_CTRL_SWGPREG1r]
 *        sys_lane_0_to_3_if_type  @ 0x18592 [BCMI_BARCHETTA_CTRL_SWGPREG2r]
 *        sys_lane_4_to_7_if_type  @ 0x18593 [BCMI_BARCHETTA_CTRL_SWGPREG3r]
 */
#define BARCHETTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR        BCMI_BARCHETTA_CTRL_SWGPREG0r
#define BARCHETTA_IF_TYPE_PER_LANE_STORAGE_MASK         0xF
#define BARCHETTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET  2

/* For CR/KR and KRS/CRS*/
#define BARCHETTA_25G_CRS              0
#define BARCHETTA_25G_KRS              1
#define BARCHETTA_25G_IEEE_KRS_SHIFT   8
#define BARCHETTA_25G_CR               0
#define BARCHETTA_25G_KR               1

typedef enum BARCHETTA_REGISTER_SELECT_E {
    BarchettaRegisterSelectNone = 0,
    BarchettaRegisterSelectIngress,
    BarchettaRegisterSelectEgress
} BARCHETTA_REGISTER_SELECT_T;

typedef enum BARCHETTA_REGISTER_TYPE_E {
    BarchettaRegisterTypePMD = 0,
    BarchettaRegisterTypeAN
} BARCHETTA_REGISTER_TYPE_T;

typedef enum BARCHETTA_PORT_OPERATION_E {
    BarchettaRegisterTx = 0,
    BarchettaRegisterRx
} BARCHETTA_PORT_OPERATION_T;

typedef enum BARCHETTA_LANE_MAP_DIR_E {
    BarchettaLaneMapOpTx = 0,
    BarchettaLaneMapOpRx,
    BarchettaLaneMapOpNone
} BARCHETTA_LANE_MAP_DIR_T;

#define BARCHETTA_MAX_LANE                        8

#define BARCHETTA_FW_ALREADY_DOWNLOADED           0xFAD
#define BARCHETTA_MICRO_RETRY_COUNT               0x250
#define BARCHETTA_HEADER_WORDS                    64

#define BARCHETTA_SPEED_10G    10000
#define BARCHETTA_SPEED_20G    20000
#define BARCHETTA_SPEED_25G    25000
#define BARCHETTA_SPEED_50G    50000

#define BARCHETTA_IS_SYSTEM_SIDE(PHY)             (PHY->port_loc == phymodPortLocSys)
#define BARCHETTA_IS_LINE_SIDE(PHY)               (PHY->port_loc == phymodPortLocLine || PHY->port_loc == phymodPortLocDC)
#define BARCHETTA_GET_REG_SEL_FOR_RX_OP(PHY)      ((PHY->port_loc == phymodPortLocLine) ? BarchettaRegisterSelectIngress : BarchettaRegisterSelectEgress)
#define BARCHETTA_GET_REG_SEL_FOR_TX_OP(PHY)      ((PHY->port_loc == phymodPortLocLine) ? BarchettaRegisterSelectEgress : BarchettaRegisterSelectIngress)

#define BARCHETTA_GET_PORT_AND_PORT_LANE(PHY, PRT_NO, PRT_LN, PRT_OP)                \

typedef enum {
    BARCHETTA_PARAM_GET_CNT       = 0x8888,
    BARCHETTA_PARAM_GET_LSB       = 0xABCD,
    BARCHETTA_PARAM_GET_MSB       = 0x4321,
    BARCHETTA_PARAM_GET_2B        = 0xEEEE,
    BARCHETTA_PARAM_GET_B         = 0xF00D,
    BARCHETTA_PARAM_ERR           = 0x0BAD,
    BARCHETTA_PARAM_NEXT          = 0x2222,
    BARCHETTA_PARAM_NOT_DWNLD     = 0x0101,
    BARCHETTA_PARAM_DWNLD_ALREADY = 0x0202,
    BARCHETTA_PARAM_DWNLD_DONE    = 0x0303,
    BARCHETTA_PARAM_PRGRM_DONE    = 0x0404,
    BARCHETTA_PARAM_HDR_ERR       = 0x0E0E,
    BARCHETTA_PARAM_FLASH         = 0xF1AC,
    BARCHETTA_PARAM_HEAD          = 0x0EAD
} BARCHETTA_MESSAGE_TYPE_T;

typedef enum barchetta_interface_mode_e {
    BARCHETTA_INTERFACE_MODE_ETHERNET = 0,
    BARCHETTA_INTERFACE_MODE_FIBER = 3
} barchetta_interface_mode_t;

/*!
 * @enum barchetta_package_die_info_e
 * Constants to specify number of dies available in barchetta package
 */
typedef enum barchetta_package_die_info_e {
    BARCHETTA_SINGLE_DIE_PACKAGE = (1), /* Single Die package */
    BARCHETTA_DUAL_DIE_PACKAGE   = (2) /* Dual die package   */

} barchetta_package_die_info_t;

/*!
 * @enum barchetta_port_status_e
 * Constants to specify barchetta port status (unallocated/allocated)
 */
typedef enum barchetta_port_status_e {
    BARCHETTA_PORT_UNALLOCATED = (0), /* Port status unallocated */
    BARCHETTA_PORT_ALLOCATED   = (1) /* Port status allocated   */

} barchetta_port_status_t;

/*!
 * @enum barchetta_lane_data_rate_e
 * Constants to specify barchetta lane data rates.
 * Expected to be in ascending order always
 */
typedef enum barchetta_lane_data_rate_e {
    BARCHETTA_LANE_DATA_RATE_NONE        = (0),
    BARCHETTA_LANE_DATA_RATE_1P0625G     = (1062),
    BARCHETTA_LANE_DATA_RATE_1P25G       = (1250),
    BARCHETTA_LANE_DATA_RATE_2P125G      = (2125),
    BARCHETTA_LANE_DATA_RATE_2P4576G     = (2457),
    BARCHETTA_LANE_DATA_RATE_4P25G       = (4250),
    BARCHETTA_LANE_DATA_RATE_4P9152G     = (4915),
    BARCHETTA_LANE_DATA_RATE_6P144G      = (6144),
    BARCHETTA_LANE_DATA_RATE_6P25G       = (6250),
    BARCHETTA_LANE_DATA_RATE_7P5G        = (7500),
    BARCHETTA_LANE_DATA_RATE_8P5G        = (8500),
    BARCHETTA_LANE_DATA_RATE_9P8304G     = (9830),
    BARCHETTA_LANE_DATA_RATE_9P95328G    = (9953),
    BARCHETTA_LANE_DATA_RATE_10P1376G    = (10137),
    BARCHETTA_LANE_DATA_RATE_10P3125G    = (10312),
    BARCHETTA_LANE_DATA_RATE_10P51875G   = (10518),
    BARCHETTA_LANE_DATA_RATE_10P52581G   = (10525),
    BARCHETTA_LANE_DATA_RATE_10P70922G   = (10709),
    BARCHETTA_LANE_DATA_RATE_10P75460G   = (10754),
    BARCHETTA_LANE_DATA_RATE_10P9375G    = (10937),
    BARCHETTA_LANE_DATA_RATE_11P04908G   = (11049),
    BARCHETTA_LANE_DATA_RATE_11P09568G   = (11095),
    BARCHETTA_LANE_DATA_RATE_11P181G     = (11181),
    BARCHETTA_LANE_DATA_RATE_11P197G     = (11197),
    BARCHETTA_LANE_DATA_RATE_11P25G      = (11250),
    BARCHETTA_LANE_DATA_RATE_11P5G       = (11500),
    BARCHETTA_LANE_DATA_RATE_12P16512G   = (12165),
    BARCHETTA_LANE_DATA_RATE_12P5G       = (12500),
    BARCHETTA_LANE_DATA_RATE_14P025G     = (14025),
    BARCHETTA_LANE_DATA_RATE_15G         = (15000),
    BARCHETTA_LANE_DATA_RATE_20P625G     = (20625),
    BARCHETTA_LANE_DATA_RATE_21P875G     = (21875),
    BARCHETTA_LANE_DATA_RATE_22P5G       = (22500),
    BARCHETTA_LANE_DATA_RATE_23G         = (23000),
    BARCHETTA_LANE_DATA_RATE_24P33024G   = (24330),
    BARCHETTA_LANE_DATA_RATE_25G         = (25000),
    BARCHETTA_LANE_DATA_RATE_25P6608G    = (25660),
    BARCHETTA_LANE_DATA_RATE_25P78125G   = (25781),
    BARCHETTA_LANE_DATA_RATE_26P25G      = (26250),
    BARCHETTA_LANE_DATA_RATE_26P5625G    = (26562),
    BARCHETTA_LANE_DATA_RATE_27P34375G   = (27343),
    BARCHETTA_LANE_DATA_RATE_27P9525G    = (27952),
    BARCHETTA_LANE_DATA_RATE_28P05G      = (28050),
    BARCHETTA_LANE_DATA_RATE_28P125G     = (28125),
    BARCHETTA_LANE_DATA_RATE_32P5G       = (32500),
    BARCHETTA_LANE_DATA_RATE_33P75G      = (33750),
    BARCHETTA_LANE_DATA_RATE_46P25G      = (46250),
    BARCHETTA_LANE_DATA_RATE_50G         = (50000),
    BARCHETTA_LANE_DATA_RATE_51P5625G    = (51562),
    BARCHETTA_LANE_DATA_RATE_53P125G     = (53125),
    BARCHETTA_LANE_DATA_RATE_56P1G       = (56100),
    BARCHETTA_LANE_DATA_RATE_56P25G      = (56250)
} barchetta_lane_data_rate_t;

#define BARCHETTA_LANE_DATARATE_LIST_ELEMENTS   {                                       \
                                                    BARCHETTA_LANE_DATA_RATE_1P0625G,   \
                                                    BARCHETTA_LANE_DATA_RATE_1P25G,     \
                                                    BARCHETTA_LANE_DATA_RATE_1P25G,     \
                                                    BARCHETTA_LANE_DATA_RATE_1P25G,     \
                                                    BARCHETTA_LANE_DATA_RATE_1P25G,     \
                                                    BARCHETTA_LANE_DATA_RATE_2P125G,    \
                                                    BARCHETTA_LANE_DATA_RATE_2P4576G,   \
                                                    BARCHETTA_LANE_DATA_RATE_4P25G,     \
                                                    BARCHETTA_LANE_DATA_RATE_4P9152G,   \
                                                    BARCHETTA_LANE_DATA_RATE_6P144G,    \
                                                    BARCHETTA_LANE_DATA_RATE_6P144G,    \
                                                    BARCHETTA_LANE_DATA_RATE_6P25G,     \
                                                    BARCHETTA_LANE_DATA_RATE_7P5G,      \
                                                    BARCHETTA_LANE_DATA_RATE_7P5G,      \
                                                    BARCHETTA_LANE_DATA_RATE_8P5G,      \
                                                    BARCHETTA_LANE_DATA_RATE_9P8304G,   \
                                                    BARCHETTA_LANE_DATA_RATE_9P95328G,  \
                                                    BARCHETTA_LANE_DATA_RATE_10P1376G,  \
                                                    BARCHETTA_LANE_DATA_RATE_10P3125G,  \
                                                    BARCHETTA_LANE_DATA_RATE_10P3125G,  \
                                                    BARCHETTA_LANE_DATA_RATE_10P51875G, \
                                                    BARCHETTA_LANE_DATA_RATE_10P52581G, \
                                                    BARCHETTA_LANE_DATA_RATE_10P70922G, \
                                                    BARCHETTA_LANE_DATA_RATE_10P75460G, \
                                                    BARCHETTA_LANE_DATA_RATE_10P75460G, \
                                                    BARCHETTA_LANE_DATA_RATE_10P75460G, \
                                                    BARCHETTA_LANE_DATA_RATE_10P9375G,  \
                                                    BARCHETTA_LANE_DATA_RATE_11P04908G, \
                                                    BARCHETTA_LANE_DATA_RATE_11P09568G, \
                                                    BARCHETTA_LANE_DATA_RATE_11P09568G, \
                                                    BARCHETTA_LANE_DATA_RATE_11P181G,   \
                                                    BARCHETTA_LANE_DATA_RATE_11P181G,   \
                                                    BARCHETTA_LANE_DATA_RATE_11P197G,   \
                                                    BARCHETTA_LANE_DATA_RATE_11P25G,    \
                                                    BARCHETTA_LANE_DATA_RATE_11P5G,     \
                                                    BARCHETTA_LANE_DATA_RATE_12P16512G, \
                                                    BARCHETTA_LANE_DATA_RATE_12P5G,     \
                                                    BARCHETTA_LANE_DATA_RATE_12P5G,     \
                                                    BARCHETTA_LANE_DATA_RATE_14P025G,   \
                                                    BARCHETTA_LANE_DATA_RATE_15G,       \
                                                    BARCHETTA_LANE_DATA_RATE_15G,       \
                                                    BARCHETTA_LANE_DATA_RATE_20P625G,   \
                                                    BARCHETTA_LANE_DATA_RATE_20P625G,   \
                                                    BARCHETTA_LANE_DATA_RATE_21P875G,   \
                                                    BARCHETTA_LANE_DATA_RATE_22P5G,     \
                                                    BARCHETTA_LANE_DATA_RATE_23G,       \
                                                    BARCHETTA_LANE_DATA_RATE_24P33024G, \
                                                    BARCHETTA_LANE_DATA_RATE_24P33024G, \
                                                    BARCHETTA_LANE_DATA_RATE_25G,       \
                                                    BARCHETTA_LANE_DATA_RATE_25G,       \
                                                    BARCHETTA_LANE_DATA_RATE_25P6608G,  \
                                                    BARCHETTA_LANE_DATA_RATE_25P6608G,  \
                                                    BARCHETTA_LANE_DATA_RATE_25P78125G, \
                                                    BARCHETTA_LANE_DATA_RATE_25P78125G, \
                                                    BARCHETTA_LANE_DATA_RATE_25P78125G, \
                                                    BARCHETTA_LANE_DATA_RATE_26P25G,    \
                                                    BARCHETTA_LANE_DATA_RATE_26P5625G,  \
                                                    BARCHETTA_LANE_DATA_RATE_26P5625G,  \
                                                    BARCHETTA_LANE_DATA_RATE_27P34375G, \
                                                    BARCHETTA_LANE_DATA_RATE_27P34375G, \
                                                    BARCHETTA_LANE_DATA_RATE_27P9525G,  \
                                                    BARCHETTA_LANE_DATA_RATE_27P9525G,  \
                                                    BARCHETTA_LANE_DATA_RATE_28P05G,    \
                                                    BARCHETTA_LANE_DATA_RATE_28P125G,   \
                                                    BARCHETTA_LANE_DATA_RATE_32P5G,     \
                                                    BARCHETTA_LANE_DATA_RATE_33P75G,    \
                                                    BARCHETTA_LANE_DATA_RATE_46P25G,    \
                                                    BARCHETTA_LANE_DATA_RATE_50G,       \
                                                    BARCHETTA_LANE_DATA_RATE_50G,       \
                                                    BARCHETTA_LANE_DATA_RATE_51P5625G,  \
                                                    BARCHETTA_LANE_DATA_RATE_51P5625G,  \
                                                    BARCHETTA_LANE_DATA_RATE_51P5625G,  \
                                                    BARCHETTA_LANE_DATA_RATE_53P125G,   \
                                                    BARCHETTA_LANE_DATA_RATE_53P125G,   \
                                                    BARCHETTA_LANE_DATA_RATE_56P1G,     \
                                                    BARCHETTA_LANE_DATA_RATE_56P25G,    \
                                                    BARCHETTA_LANE_DATA_RATE_56P25G     \
                                                }

#define BARCHETTA_IF_TYPE_LIST_ELEMENTS         {                                       \
                                                    phymodInterfaceSR,                  \
                                                    phymodInterfaceKR,                  \
                                                    phymodInterfaceCR,                  \
                                                    phymodInterfaceXFI,                 \
                                                    phymodInterfaceSFI,                 \
                                                    phymodInterfaceXLAUI,               \
                                                    phymodInterfaceLR,                  \
                                                    phymodInterfaceER,                  \
                                                    phymodInterfaceVSR,                 \
                                                    phymodInterfaceCAUI4_C2C,           \
                                                    phymodInterfaceAUI_C2C,             \
                                                    phymodInterfaceAUI_C2M,             \
                                                    phymodInterfaceKR4,                 \
                                                    phymodInterfaceCR4,                 \
                                                    phymodInterfaceCAUI4_C2M,           \
                                                    phymodInterfaceCAUI                 \
                                                }
#define BARCHETTA_MAX_NO_DR                   78
#define BARCHETTA_MAX_NO_DR_COL               5
#define BARCHETTA_LN_DR_IDX                   0
#define BARCHETTA_REF_CLK_IDX                 1
#define BARCHETTA_PLL_DIV_IDX                 2
#define BARCHETTA_OSR_IDX                     3
#define BARCHETTA_BH_REF_CLK_IDX              4

#define BARCHETTA_MAX_REF_CLK_FOR_DR          4


/* Barchetta Interrupt type*/
#define BARCHETTA_MAX_INTR_REG                   5
#define BARCHETTA_INTR_M0_MST_MISC               (0x1 << 0)
#define BARCHETTA_INTR_M0_MST_MSGOUT             (0x1 << 1)
#define BARCHETTA_INTR_PLL_LOCK_FOUND            (0x1 << 2)
#define BARCHETTA_INTR_PLL_LOCK_LOST             (0x1 << 3)
#define BARCHETTA_INTR_CL73_AN_COMPLETE          (0x1 << 4)
#define BARCHETTA_INTR_CL73_AN_RESTARTED         (0x1 << 5)
#define BARCHETTA_INTR_PMD_RX_SIGDET_FOUND       (0x1 << 6)
#define BARCHETTA_INTR_PMD_RX_SIGDET_LOST        (0x1 << 7)
#define BARCHETTA_INTR_PMD_RX_LOCK_FOUND         (0x1 << 8)
#define BARCHETTA_INTR_PMD_RX_LOCK_LOST          (0x1 << 9)
#define BARCHETTA_INTR_PMD_MOD_ABS               (0x1 << 10)

/* Barchetta interrupt registers address */
#define BARCHETTA_CTRL_TOP_EIER_ADDR            BCMI_BARCHETTA_CTRL_TOP_EIERr
#define BARCHETTA_CTRL_TOP_EISR_ADDR            BCMI_BARCHETTA_CTRL_TOP_EISRr
#define BARCHETTA_CTRL_TOP_EIPR_ADDR            BCMI_BARCHETTA_CTRL_TOP_EIPRr

#define BARCHETTA_CTRL_M0_EIER_ADDR             BCMI_BARCHETTA_CTRL_M0_EIERr
#define BARCHETTA_CTRL_M0_EISR_ADDR             BCMI_BARCHETTA_CTRL_M0_EISRr
#define BARCHETTA_CTRL_M0_EIPR_ADDR             BCMI_BARCHETTA_CTRL_M0_EIPRr

#define BARCHETTA_CTRL_COMMON_MISC_EIER_ADDR    BCMI_BARCHETTA_CTRL_COMMON_MISC_EIERr
#define BARCHETTA_CTRL_COMMON_MISC_EISR_ADDR    BCMI_BARCHETTA_CTRL_COMMON_MISC_EISRr
#define BARCHETTA_CTRL_COMMON_MISC_EIPR_ADDR    BCMI_BARCHETTA_CTRL_COMMON_MISC_EIPRr

#define BARCHETTA_CTRL_Px_A_EIER_BASE_ADDR      BCMI_BARCHETTA_CTRL_P0_A_EIERr
#define BARCHETTA_CTRL_Px_B_EIER_BASE_ADDR      BCMI_BARCHETTA_CTRL_P0_B_EIERr
#define BARCHETTA_CTRL_Px_A_EISR_BASE_ADDR      BCMI_BARCHETTA_CTRL_P0_A_EISRr
#define BARCHETTA_CTRL_Px_B_EISR_BASE_ADDR      BCMI_BARCHETTA_CTRL_P0_B_EISRr
#define BARCHETTA_CTRL_Px_A_EIPR_BASE_ADDR      BCMI_BARCHETTA_CTRL_P0_A_EIPRr
#define BARCHETTA_CTRL_Px_B_EIPR_BASE_ADDR      BCMI_BARCHETTA_CTRL_P0_B_EIPRr

#define BARCHETTA_CTRL_MST_M0_EIER_ADDR         BCMI_BARCHETTA_CTRL_MST_M0_INTR_EIERr
#define BARCHETTA_CTRL_MST_M0_EISR_ADDR         BCMI_BARCHETTA_CTRL_MST_M0_INTR_EISRr
#define BARCHETTA_CTRL_MST_M0_EIPR_ADDR         BCMI_BARCHETTA_CTRL_MST_M0_INTR_EIPRr

/* Barchetta interrupt type bit description */
#define TOP_M0_ENABLE_BIT_POS                   9
#define TOP_M0_ENABLE_MASK                      (0x1 << TOP_M0_ENABLE_BIT_POS)
#define TOP_M0_ENABLE_COMMON_MISC_BIT_POS       8
#define TOP_M0_ENABLE_COMMON_MISC_MASK          (0x1 << TOP_M0_ENABLE_COMMON_MISC_BIT_POS)

#define M0_ENABLE_MST_MISC_INTR_BIT_POS         3
#define M0_ENABLE_MST_MISC_INTR_MASK            (0x1 << M0_ENABLE_MST_MISC_INTR_BIT_POS)
#define M0_ENABLE_MST_MSGOUT_INTR_BIT_POS       2
#define M0_ENABLE_MST_MSGOUT_INTR_MASK          (0x1 << M0_ENABLE_MST_MSGOUT_INTR_BIT_POS)

#define LIN_SYS_INT_PLL_DIFF                    4
#define LIN_PMD_PLL0_LOCK_FOUND_BIT_POS         0
#define LIN_PMD_PLL0_LOCK_LOST_BIT_POS          1

#define LIN_SYS_INT_AN_DIFF                     4
#define LIN_CL73_AN_COMPLETE_BIT_POS            0
#define LIN_CL73_AN_RESTARTED_BIT_POS           1

#define LIN_SYS_INT_PMD_DIFF                    2
#define LIN_PMD_RX_SIGDET_FOUND_BIT_POS         0
#define LIN_PMD_RX_SIGDET_LOST_BIT_POS          1
#define LIN_PMD_RX_LOCK_FOUND_BIT_POS           4
#define LIN_PMD_RX_LOCK_LOST_BIT_POS            5

#define MOD_ABS_0_BIT_POS                       0
#define MOD_ABS_0_MASK                          (0x1 << MOD_ABS_0_BIT_POS)
#define MOD_ABS_1_BIT_POS                       1
#define MOD_ABS_1_MASK                          (0x1 << MOD_ABS_1_BIT_POS)
#define MOD_ABS_2_BIT_POS                       2
#define MOD_ABS_2_MASK                          (0x1 << MOD_ABS_2_BIT_POS)
#define MOD_ABS_3_BIT_POS                       3
#define MOD_ABS_3_MASK                          (0x1 << MOD_ABS_3_BIT_POS)
#define MOD_ABS_4_BIT_POS                       4
#define MOD_ABS_4_MASK                          (0x1 << MOD_ABS_4_BIT_POS)
#define MOD_ABS_5_BIT_POS                       5
#define MOD_ABS_5_MASK                          (0x1 << MOD_ABS_5_BIT_POS)
#define MOD_ABS_6_BIT_POS                       6
#define MOD_ABS_6_MASK                          (0x1 << MOD_ABS_6_BIT_POS)
#define MOD_ABS_7_BIT_POS                       7
#define MOD_ABS_7_MASK                          (0x1 << MOD_ABS_7_BIT_POS)

/* GPIO Configuration defines */
#define BARCHETTA_MAX_GPIO_PIN   31

#define BARCHETTA_PAD_CTRL_GPIO_X_CTRL_BASE_ADDR    BCMI_BARCHETTA_PAD_CNTRL_GPIO_0_CONTROLr
#define GPIO_X_CTRL_GPIO_X_OEBF_BIT_POS             0
#define GPIO_X_CTRL_GPIO_X_OEBF_MASK                (0x1 << GPIO_X_CTRL_GPIO_X_OEBF_BIT_POS)
#define GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_BIT_POS     1
#define GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_MASK        (0x3 << GPIO_X_CTRL_GPIO_X_PULL_UP_DOWN_BIT_POS)
#define GPIO_X_CTRL_GPIO_X_IBOF_BIT_POS             9
#define GPIO_X_CTRL_GPIO_X_IBOF_MASK                (0x1 << GPIO_X_CTRL_GPIO_X_IBOF_BIT_POS)
#define GPIO_X_CTRL_GPIO_X_OUT_FRCVAL_BIT_POS       11
#define GPIO_X_CTRL_GPIO_X_OUT_FRCVAL_MASK          (0x1 << GPIO_X_CTRL_GPIO_X_OUT_FRCVAL_BIT_POS)

#define BARCHETTA_PAD_CTRL_GPIO_X_STATUS_BASE_ADDR  BCMI_BARCHETTA_PAD_CNTRL_GPIO_0_STATUSr
#define GPIO_X_STATUS_GPIO_X_DIN_BIT_POS            2
#define GPIO_X_STATUS_GPIO_X_DIN_MASK               (0x1 << GPIO_X_STATUS_GPIO_X_DIN_BIT_POS)

/* SyncE Configurations */
#define BARCHETTA_AMS_PLL_COM_PLL_INTCTRL         0x1D11B
#define BARCHETTA_PLL_INTCTRL_OFFSET              2
#define BARCHETTA_PLL_INTCTRL_MASK                (0x1 << BARCHETTA_PLL_INTCTRL_OFFSET)

#define BARCHETTA_AMS_PLL_COM_PLL_CONTROL_3        0x1D113
#define BARCHETTA_PLL_CTRL_OFFSET                  12
#define BARCHETTA_PLL_CTRL_MASK                    (0xF << BARCHETTA_PLL_CTRL_OFFSET)

#define BARCHETTA_AMS_RX_CONTROL_5                 0x1D0C5
#define BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_OFFSET   14
#define BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_MASK     (0x1 << BARCHETTA_AMS_RX_CTRL_TESTCLK_MUX_OFFSET)

#define BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_OFFSET   12
#define BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_MASK     (0x3 << BARCHETTA_AMS_RX_CTRL_TESTCLK_DIV_OFFSET)
#define BARCHETTA_TESTCLK_DIV1                     0x0
#define BARCHETTA_TESTCLK_DIV2                     0x1
#define BARCHETTA_TESTCLK_DIV4                     0x2
#define BARCHETTA_TESTCLK_DIV8                     0x3

#define BARCHETTA_AMS_RX_CONTROL_0                 0x1D0C0
#define BARCHETTA_AMS_RX_CTRL_TPORT_EN_OFFSET      8
#define BARCHETTA_AMS_RX_CTRL_TPORT_EN_MASK        (0x1 << BARCHETTA_AMS_RX_CTRL_TPORT_EN_OFFSET)
#define BARCHETTA_TPORT_ENA                        1
#define BARCHETTA_TPORT_DIS                        0

/* Recovered Clock Output data structures */
/*
   Mux clock out of the selected Rx lane
   testclk_mux_lanes 0 to 3 and 4 to 7.
   0  - Select local clock
   1  - Select adjacent lane's clock
   2-D Array. [Recovered Clock Output lanes][lanes 0-3 or 4-7]
*/
static const int plp_barchetta_testclk_mux [BARCHETTA_MAX_LANE][BARCHETTA_MAX_LANE/2] = {
                                                                      { 0, 1, 1, 1}, /* Selecting adjacent lanes clock 1, 2, 3 to output recovered clock from lane 0.*/
                                                                      { 1, 0, 1, 1}, /* Selecting adjacent lanes clock 2, 3 to output recovered clock from lane 1.*/
                                                                      { 1, 1, 0, 1}, /* Selecting adjacent lane clock 3 to output recovered clock from lane 2.*/
                                                                      { 1, 1, 1, 0}, /* Selecting local clock to output recovered clock from lane 3.*/
                                                                      { 0, 1, 1, 1}, /* Selecting adjacent lanes clock 5, 6, 7 to output recovered clock from lane 4.*/
                                                                      { 1, 0, 1, 1}, /* Selecting adjacent lanes clock 6, 7 to output recovered clock from lane 5.*/
                                                                      { 1, 1, 0, 1}, /* Selecting adjacent lane clock 7 to output recovered clock from lane 6.*/
                                                                      { 1, 1, 1, 0}  /* Selecting local clock to output recovered clock from lane 7.*/
                                                             };
/*
   PLL Control for 81381
*/
static const int plp_barchetta_pll_ctrl_81381 [2][2] = {
                                /* PLL0, PLL1 */
                                { 0x8,   0x0}, /* Selecting PLL0, Disabling PLL1 to output recovered clock from lanes 0 to 3.*/
                                { 0x0,   0x8}, /* Selecting PLL0, Disabling PLL1 to output recovered clock from lanes 4 to 7.*/
                            };
/*
   PLL Control for 81338 SYS side
*/
static const int plp_barchetta_pll_ctrl_81338_sys [2][BARCHETTA_MAX_LANE/2] = {
                                /* SYS PLL0, SYS PLL1, LINE PLL0, LINE PLL1 */
                                {0x8,        0x0,      0x0,       0x0}, /* Recovered clk output from SYS lanes 0 to 3*/
                                {0x0,        0x8,      0x0,       0x0}, /* Recovered clk output from SYS lanes 4 to 7*/
                            };
/*
   PLL Control for 81338 LINE side
*/
static const int plp_barchetta_pll_ctrl_81338_line [2][BARCHETTA_MAX_LANE/2] = {
                                /* SYS PLL0, SYS PLL1, LINE PLL0, LINE PLL1 */
                                {0x0,        0x0,      0x8,       0x0}, /* Recovered clk output from LINE lanes 0 to 3*/
                                {0x0,        0x0,      0x0,       0x8}  /* Recovered clk output from LINE lanes 4 to 7*/
                            };

/* Serdes debug dump levels */
#define SRDS_MIN_DUMP   0x0

#define PHY_INTERNAL_DUMP 0x8
/* Serdes dump level corresponding to PHY_INTERNAL_DUMP */
#define SRDS_INTERNAL_DUMP 0x5

/*
 * This displays the core state, the lane state, the extended
 * lane state and the event log
 */
#define PHY_DUMP_L1 0x200
/* Serdes dump level corresponding to PHY_DUMP_L1 */
#define SRDS_DUMP_L1 0x7

/*
 * This displays the core state, the lane state, the extended
 * lane state, the event log, lane and core registers as well
 * as lane and core uc variables.
 */
#define PHY_DUMP_L2 0x400
/* Serdes dump level corresponding to PHY_DUMP_L2 */
#define SRDS_DUMP_L2 0xF7

/*
 * This displays the core state, the lane state, the extended
 * lane state, the event log, fast eye scan, lane and core
 * registers as well as lane and core uc variables.
 */
#define PHY_DUMP_L3 0x800
/* Serdes dump level corresponding to PHY_DUMP_L3 */
#define SRDS_DUMP_L3 0xFF

/* Module controller I2C master commands */
typedef enum barchetta_i2c_module_cmd_e{
  BARCHETTA_FLUSH = (0),
  BARCHETTA_RANDOM_ADDRESS_READ,
  BARCHETTA_CURRENT_ADDRESS_READ,
  BARCHETTA_I2C_WRITE

} barchetta_i2c_module_cmd_t;

/* Module controller specific defines */
#define BARCHETTA_MODULE_CNTRL_RAM_NVR0_ADR    0x8800
#define BARCHETTA_MOD_ABS_0_GPIO_PIN            10
#define BARCHETTA_MOD_ABS_1_GPIO_PIN            11

/*!
 * @enum barchetta_pll_select_e
 * Constants to select Blackhawk PLL
 */
typedef enum barchetta_pll_select_e {
    BARCHETTA_PLL0 = 0x00, /* select PLL 0 */
    BARCHETTA_PLL1 = 0x01 /* select PLL 1 */

} barchetta_pll_select_t;

/*!
 * @enum barchetta_port_mode_t
 * Constants to specify barchetta port mode
 */
typedef enum barchetta_port_mode_e {
    BARCHETTA_PORT_MODE_REGULAR = 0x00,
    BARCHETTA_PORT_MODE_FAILOVER = 0x01

} barchetta_port_mode_t;

/*!
 * Structure to hold package information
 */
typedef struct barchetta_package_info_s {
    uint8_t pkg_type;       /* Package type, Like DUPLEX or SIMPLEX                               */
    uint8_t pkg_lanes;      /* number of lanes in the package, Like 8 lanes or 16 lanes package   */
    uint8_t no_of_dies;     /* Number of dies in the package, Like dual die or single die package */
    uint8_t no_of_max_ports;/* Number of maximum ports can be configured with respective package  */

} barchetta_package_info_t;

/*!
 * Structure to hold duplex port logical lane maps
 */
typedef struct barchetta_duplex_lane_map_s {
    uint8_t sys_lane_map;  /* System side logical lane map  */
    uint8_t line_lane_map; /* Line side logical lane map    */

} barchetta_duplex_lane_map_t;

/*!
 * Structure to hold simplex port (SR2LT) logical lane maps
 * NOTE : Simplex design subject to modify
 */
typedef struct barchetta_sr2lt_lane_map_s {
    uint8_t sys_lane_map;  /* System side logical lane map  */
    uint8_t line_lane_map; /* Line side logical lane map    */

} barchetta_sr2lt_lane_map_t;

/*!
 * Structure to hold simplex port (LR2ST) logical lane maps
 * NOTE : Simplex design subject to modify
 */
typedef struct barchetta_lr2st_lane_map_s {
    uint8_t sys_lane_map;  /* System side logical lane map  */
    uint8_t line_lane_map; /* Line side logical lane map    */

} barchetta_lr2st_lane_map_t;

/*!
 * Structure to hold lane maps of a port
 */
typedef struct barchetta_lane_map_s {
    union {
        barchetta_duplex_lane_map_t duplex_lane_map; /* Duplex lane config parameters structure */
        barchetta_sr2lt_lane_map_t sr2lt_lane_map;   /* SR2LT lane config parameters structure  */
        barchetta_lr2st_lane_map_t lr2st_lane_map;   /* LR2ST lane config parameter structure   */

    } lane_map;

} barchetta_lane_map_t;

/*!
 * @enum barchetta_fec_mode_e
 * Constants to specify port status (unallocated/allocated)
 */
typedef enum barchetta_fec_mode_e {
    BARCHETTA_NO_FEC = 0 /* No fec Default */

} barchetta_fec_mode_t;

/*!
 * @enum barchetta_modulation_mode_e
 * Constants to specify barchetta modulation mode
 */
typedef enum barchetta_modulation_mode_e {
    BARCHETTA_MODULATION_NONE = (0), /* No modulation   */
    BARCHETTA_MODULATION_NRZ = (1),  /* NRZ modulation  */
    BARCHETTA_MODULATION_PAM4 = (2)  /* PAM4 modulation */

} barchetta_modulation_mode_t;

typedef enum barchetta_phy_mode_e {
    BARCHETTA_PHY_MODE_GENERAL   = 0,   /* PHY mode: General PHY */
    BARCHETTA_PHY_MODE_LOCAL     = 1,   /* PHY mode: Local PHY (Remote-PHY app)*/
    BARCHETTA_PHY_MODE_REMOTE    = 2   /* PHY mode: Remote PHY (Remote-PHY app)*/
}barchetta_phy_mode_t;

typedef enum barchetta_rphy_mode_e {
   BARCHETTA_RPHY_10G       = 0,   /* RPHY Mode: IEEE 10G*/
   BARCHETTA_RPHY_25G       = 1,   /* RPHY Mode: Consortium 25G*/
   BARCHETTA_RPHY_50G       = 2    /* RPHY Mode: Consortium 50G/25G*/
}barchetta_rphy_mode_t;

typedef enum barchetta_port_speed_e
{
   BARCHETTA_PORT_NULL      = 0,   /*Port Speed Not Configured*/
   BARCHETTA_PORT_10G       = 1,   /*Port Speed: 10G*/
   BARCHETTA_PORT_25G       = 2,   /*Port Speed: 25G*/
   BARCHETTA_PORT_50G       = 3   /*Port Speed: 50G*/
}barchetta_port_speed_t;

/****************************************************************************
 * Remote-PHY Registers
 ****************************************************************************/
#define GPREG_FW_AN_CONTROL_ADR            BCMI_BARCHETTA_GEN_CNTRLS_GPREG_09r
#define GPREG_FW_PHY_CONTROL_ADR           BCMI_BARCHETTA_GEN_CNTRLS_GPREG_10r
#define GPREG_FW_PHY_STATUS_ADR            BCMI_BARCHETTA_GEN_CNTRLS_GPREG_11r
#define GPREG_FW_PHY_ERROR_ADR             BCMI_BARCHETTA_GEN_CNTRLS_GPREG_12r
#define GPREG_FW_TOMAHAWK_SPEED_ADR        BCMI_BARCHETTA_GEN_CNTRLS_GPREG_13r
#define GPREG_FW_PHY_PORT_SPEED_ADR        BCMI_BARCHETTA_GEN_CNTRLS_GPREG_14r
#define GPREG_FW_PHY_PORT_SYS_STATUS_ADR   BCMI_BARCHETTA_GEN_CNTRLS_GPREG_15r
#define GPREG_FW_PHY_PORT_LINE_STATUS_ADR  BCMI_BARCHETTA_GEN_CNTRLS_GPREG_16r
#define GPREG_AN_CL72_ENA_STS              BCMI_BARCHETTA_CTRL_SWGPREG9r /* 0x00018599 */
#define BARCHETTA_PORT_SPEED_10G           10000
#define BARCHETTA_PORT_SPEED_25G           25000
#define BARCHETTA_PORT_SPEED_50G           50000
#define BARCHETTA_RMT_DISABLE_PRBS_AN      0x202
#define BARCHETTA_LOC_DISABLE_PRBS_AN      0x200
#define GPREG_FOR_REF_CLK_OFFLINE          BCMI_BARCHETTA_CTRL_SWGPREGAr /* 0x0001859a */

typedef enum bcm_plp_barchetta_tx_driv_mode_e {
    bcmplptxdrivebarchettaHWdefaults = 0,
    bcmplptxdrivebarchetta0P8Volt = 1,
    bcmplptxdrivebarchetta1P2Volt = 2,
    bcmplptxdrivebarchetta1P00Volt = 3,
    bcmplptxdrivebarchetta1P25Volt = 4
}bcm_plp_barchetta_tx_driv_mode_t;

typedef enum bcm_plp_barchetta_low_latency_variation_e {
    bcmplpLowLatencyVariationDisable = 0, /**< Low latency disable; Disable both ingress and egress */
    bcmplpLowLatencyVariationIngressEnable, /**< Low latency ingress */
    bcmplpLowLatencyVariationEgressEnable, /**< Low latency egress enable */
    bcmplpLowLatencyVariationEnable /**< Low latency enable; Enable both ingress and egress */
} bcm_plp_barchetta_low_latency_variation_t;

/*!
 * Structure to hold the barchetta aux mode structure
 * This is used to store system side information in global database.
 * needed for future.
 */
typedef struct barchetta_aux_modes_s {
    barchetta_lane_data_rate_t lane_data_rate;
    barchetta_modulation_mode_t modulation_mode;
    uint8_t clock_mode;
    uint8_t ll_mode;
    uint32_t failover_lane_map; /* Lane map of secondary port*/
    uint32_t phy_mode;
    bcm_plp_barchetta_tx_driv_mode_t  tx_driver_mode;
} barchetta_aux_modes_t;

/*!
 * Structure to hold the port information required for software port database management
 */
typedef struct barchetta_sw_db_s {
    barchetta_config_lanes_t lanes_cfg_sys; /* System side lane configuration information     */
    barchetta_port_config_t port_cfg_info;  /* Port configuration information                 */
    barchetta_lane_map_t lane_map_info;     /* Port lane map information                      */
    barchetta_aux_modes_t port_cfg_aux_sys; /* System Side Port Config aux mode Information   */
    uint32_t sys_primary_port_lane_map;     /* System Side primary port                       */
    barchetta_pmd_config_t pmd_cfg_sys;     /* PMD Config Sys Side Information                */
    uint8_t lanes_cfg_sys_status;           /* System side lane config status                 */
    uint8_t port_cfg_aux_sys_status;        /* System Side Port configuration status          */
    uint8_t pmd_cfg_sys_status;             /* PMD COnfig System Side configuration status    */
    uint8_t port_status;                    /* Port status (allocated / unallocated)          */

} barchetta_sw_db_t;

/*!
 * Structure to hold the barchetta serdes configuration parameters
 */
typedef struct barchetta_serdes_config_info_s {
    enum blackhawk_barchetta_pll_refclk_enum refclk;
    enum blackhawk_barchetta_pll_div_enum pll_div;
    enum blackhawk_barchetta_osr_mode_enum osr;

} barchetta_serdes_config_info_t;

/***************************************************************************//**
 \brief    To retrieve chip ID
 \param    pa        [In]  Pointer to phymod access structure
 \param    chip_id   [Out] Pointer to chip id
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_chip_id(
    const plp_barchetta_phymod_access_t *pa,
    int *chip_id
);

/***************************************************************************//**
 \brief    To retrieve chip revision
 \param    pa        [In]  Pointer to phymod access structure
 \param    chip_rev  [Out] Pointer to chip revision
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_chip_rev(
    const plp_barchetta_phymod_access_t *pa,
    uint32_t *chip_rev
);

/***************************************************************************//**
 \brief    To retrieve chip revision from internal register
 \param    pa        [In]  Pointer to phymod access structure
 \param    chip_rev  [Out] Pointer to chip revision
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_internal_chip_rev(
    const plp_barchetta_phymod_access_t *pa,
    uint32_t *chip_rev
);

/***************************************************************************//**
 \brief    To read the content specified register address.
 \param    pa        [In]  Pointer to phymod access structure
 \param    address   [In]  Register address to read
 \param    data      [Out] Pointer to the retrieved register content
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_reg_read(
    const plp_barchetta_phymod_access_t *pa,
    uint32_t address,
    uint32_t *data
);
/***************************************************************************//**
 \brief    To write the data into specified register address.
 \param    pa        [In]  Pointer to phymod access structure
 \param    address   [In]  Register address to write data
 \param    data      [In]  Data to be written into the specified register address
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_reg_write(
    const plp_barchetta_phymod_access_t *pa,
    uint32_t address,
    uint32_t data
);

/***************************************************************************//**
 \brief    To perform the hard or soft reset functionality
 \param    core        [In] Pointer to core access structure
 \param    reset_mode  [In] HARD_RESET(0), SOFT_RESET(1)
 \param    direction   [In] Reserved for future use
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_UNAVAIL(-12)
 *******************************************************************************/
int _plp_barchetta_core_reset_set(
    const plp_barchetta_phymod_core_access_t* core,
    plp_barchetta_phymod_reset_mode_t reset_mode,
    plp_barchetta_phymod_reset_direction_t direction
);

/***************************************************************************//**
 \brief    To download the firmware
 \param    core        [In] Pointer to core access structure
 \param    init_config [In] Pointer to init config structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4), PHYMOD_E_FAIL(-8)
 *******************************************************************************/
int _plp_barchetta_dload_fw(
    const plp_barchetta_phymod_core_access_t* core,
    const plp_barchetta_phymod_core_init_config_t *init_config
);

/***************************************************************************//**
 \brief    To check the firmware download status
 \param    core_access [In] Pointer to core access structure
 \param    load_method [In] Firmware download methods
           0 : Don't load FW
           1 : Load FW internaly
           2 : Load FW externally by a given function
           3 : Program EEPROM
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_check_fw_download_status(
    const plp_barchetta_phymod_core_access_t *core_access,
    plp_barchetta_phymod_firmware_load_method_t load_method
);

/***************************************************************************//**
 \brief    To retrive the firmware information like fw version and fw crc
 \param    core        [In]  Pointer to core access structure
 \param    fw_info     [Out] Pointer to retrived fw information
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_core_firmware_info_get(
    const plp_barchetta_phymod_core_access_t* core,
    plp_barchetta_phymod_core_firmware_info_t* fw_info
);

/***************************************************************************//**
 \brief    To get the Rx PMD locked status
 \param    phy            [In]  Pointer to phymod phy access structure
 \param    rx_pmd_locked  [Out] Pointer to rx pmd locked status
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_rx_pmd_locked_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t* rx_pmd_locked
);

/***************************************************************************//**
 \brief    To set the Tx tap values
 \param    phy    [In] Pointer to phymod phy access structure
 \param    tx     [In] Pointer to tx tap parameters structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_pam4_tx_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const phymod_pam4_tx_t* tx
);

/***************************************************************************//**
 \brief    To get the Tx tap values
 \param    phy    [In]  Pointer to phymod phy access structure
 \param    tx     [Out] Pointer to tx tap parameters structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_pam4_tx_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    phymod_pam4_tx_t* tx
);

/***************************************************************************//**
 \brief    To set the Rx tap values
 \param    phy    [In] Pointer to phymod phy access structure
 \param    rx     [In] Pointer to rx tap parameters structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_rx_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_rx_t* rx
);

/***************************************************************************//**
 \brief    To get the Rx tap values
 \param    phy    [In]  Pointer to phymod phy access structure
 \param    rx     [Out] Pointer to rx tap parameters structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_rx_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_rx_t* rx
);

/***************************************************************************//**
 \brief    To set the polarity of the specified lane
 \param    phy      [In] Pointer to phymod phy access structure
 \param    polarity [In] Pointer to (Tx, Rx) polarity structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_phy_polarity_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_polarity_t* polarity
);

/***************************************************************************//**
 \brief    To get the polarity of the specified lane
 \param    phy      [In]  Pointer to phymod phy access structure
 \param    polarity [Out] Pointer to (Tx, Rx) polarity structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_phy_polarity_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_polarity_t* polarity
);

/***************************************************************************//**
 \brief    To reset the Tx/Rx datapath
 \param    phy      [In] Pointer to phymod phy access structure
 \param    reset    [In] Pointer to (Tx, Rx) reset parameter structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_reset_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_phy_reset_t* reset
);

/***************************************************************************//**
 \brief    To get the reset status of Tx/Rx datapath
 \param    phy      [In]  Pointer to phymod phy access structure
 \param    reset    [Out] Pointer to (Tx, Rx) reset parameter structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_reset_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_reset_t* reset
);

/***************************************************************************//**
 \brief    To set the power of transmitter or receiver of the specified lane
 \param    phy      [In]  Pointer to phymod phy access structure
 \param    power    [In]  Pointer to (Tx, Rx) power parameter structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_power_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_phy_power_t* power
);

/***************************************************************************//**
 \brief    To get the power of transmitter or receiver of the specified lane
 \param    phy      [In]  Pointer to phymod phy access structure
 \param    power    [Out] Pointer to (Tx, Rx) power parameter structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_power_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_power_t* power
);

/***************************************************************************//**
 \brief    To perform Tx datapath reset/Traffic disable/Squelch for a specified lane
 \param    phy        [In] Pointer to phymod phy access structure
 \param    tx_control [In] Tx datapath control information
                       2: Tx datapath reset
                       3: Tx Squelch On
                       4: Tx Squelch Off
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_UNAVAIL(-12)
 *******************************************************************************/
int _plp_barchetta_phy_tx_lane_control_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_tx_lane_control_t tx_control
);

/***************************************************************************//**
 \brief    To get Tx datapath reset/Traffic disable/Squelch for a specified lane
 \param    phy        [In] Pointer to phymod phy access structure
 \param    tx_control [Out] Pointer to Tx datapath control information
                       2: Tx datapath reset
                       3: Tx Squelch On
                       4: Tx Squelch Off
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_tx_lane_control_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_tx_lane_control_t* tx_control
);

/***************************************************************************//**
 \brief    To perform Rx datapath reset/Traffic disable/Squelch for a specified lane
 \param    phy        [In] Pointer to phymod phy access structure
 \param    rx_control [In] Rx datapath control information
                       2: Rx datapath reset
                       3: Rx Squelch On
                       4: Rx Squelch Off
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_rx_lane_control_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_rx_lane_control_t rx_control
);

/***************************************************************************//**
 \brief    To get Rx datapath reset/Traffic disable/Squelch for a specified lane
 \param    phy        [In] Pointer to phymod phy access structure
 \param    rx_control [Out] Pointer to Rx datapath control information
                       2: Rx datapath reset
                       3: Rx Squelch On
                       4: Rx Squelch Off
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_rx_lane_control_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_rx_lane_control_t* rx_control
);

/***************************************************************************//**
 \brief    To map logical lanes to physical lane Tx and Rx
 \param    phy        [In] Pointer to phymod phy access structure
 \param    no_of_lanes[In] Number of lanes to be mapped
 \param    tx_list    [In] Tx lane list
 \param    rx_list    [In] Rx lane list
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_logical_lane_list_set(
    const plp_barchetta_phymod_phy_access_t *phy,
    const uint8_t no_of_lanes,
    const uint8_t *tx_list,
    const uint8_t *rx_list
);

/***************************************************************************//**
 \brief    To get logical lane mapping information
 \param    phy        [In] Pointer to phymod phy access structure
 \param    no_of_lanes[In] Pointer to number of lanes got mapped
 \param    tx_list    [In] Pointer to tx lane list
 \param    rx_list    [In] Pointer to rx lane list
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_logical_lane_list_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint8_t *no_of_lanes,
    uint8_t *tx_list,
    uint8_t *rx_list
);

/***************************************************************************//**
 \brief    To perform the interface mode configuration
 \param    phy        [In] Pointer to phymod phy access structure
 \param    flags      [In] Reserved for future use
 \param    config     [In] Pointer phy interface configuration structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_phy_interface_config_set(
    const plp_barchetta_phymod_phy_access_t *phy,
    uint32_t flags,
    const plp_barchetta_phymod_phy_inf_config_t *config
);

/***************************************************************************//**
 \brief    To get the interface mode configuration parameters
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    flags      [In]  Reserved for future use
 \param    config     [Out] Pointer phy interface configuration structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
*******************************************************************************/
int _plp_barchetta_phy_interface_config_get(
    const plp_barchetta_phymod_phy_access_t *phy,
    uint32_t flags,
    plp_barchetta_phymod_phy_inf_config_t *config
);

/***************************************************************************//**
 \brief    To set the Remote or Digital loopback
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    loopback   [In]  Loopback mode. 1: Digital Loopback, 2: Remote Loopback
 \param    enable     [In]  1: Enable, 0: Disable
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1), PHYMOD_E_PARAM(-4), PHYMOD_E_CONFIG(-11)
 *******************************************************************************/
int _plp_barchetta_phy_loopback_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_loopback_mode_t loopback,
    uint32_t enable
);

/***************************************************************************//**
 \brief    To get loopback configuration status
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    loopback   [In]  Loopback mode. 1: Digital Loopback, 2: Remote Loopback
 \param    enable     [Out] Pointer to loopback enable/disable status
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_loopback_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_loopback_mode_t loopback,
    uint32_t *enable
);

/***************************************************************************//**
 \brief    To perform phy status dump
 \param    phy        [In]  Pointer to phymod phy access structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_status_dump(
    const plp_barchetta_phymod_phy_access_t* phy
);

/***************************************************************************//**
 \brief    To enable/disable FEC
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    enable     [In]  Enable or disable CL74/CL91 FEC
                            0x00000: disable CL91 FEC
                            0x00001: enable CL91 FEC
                            0x10000: disable CL74 FEC
                            0x10001: enable CL74 FEC
                            0x20000: disable CL108 FEC
                            0x20001: enable CL108 FEC
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4), PHYMOD_E_UNAVAIL(-12)
 *******************************************************************************/
int _plp_barchetta_phy_fec_enable_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t enable
);

/***************************************************************************//**
 \brief    To get FEC enable/disable status
 \param    phy        [In]     Pointer to phymod phy access structure
 \param    enable     [In/Out] Represent enabled status of CL74/CL91
                      bit 16-31 act as input and bit 0 to 16 act as output.
                      0x0xxxx : Get FEC enabled status for CL91
                      0x1xxxx : Get FEC enabled status for CL74
                      0x2xxxx : Get FEC enabled status for CL108
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_fec_enable_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t *enable
);

/***************************************************************************//**
 \brief    To enable or disable force tx training
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    cl72_en    [In]  1: Enable, 0: Disable
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_cl72_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t cl72_en
);

/***************************************************************************//**
 \brief    To get force tx training enable/disable status
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    cl72_en    [In]  Pointer to force Tx training enable/disable status
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_cl72_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t* cl72_en
);

/***************************************************************************//**
 \brief    To get force tx training status
 \param    phy       [In]  Pointer to phymod phy access structure
 \param    status    [In]  Pointer to force Tx training status structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_cl72_status_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_cl72_status_t* status
);

/***************************************************************************//**
 \brief    To set the autoneg ability parameters
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    an_ability [In]  Pointer to an_ability parameter structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_autoneg_ability_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_autoneg_ability_t* an_ability
);

/***************************************************************************//**
 \brief    To get the autoneg ability parameters
 \param    phy        [In]  Pointer to phymod phy access structure
 \param    an_ability [Out] Pointer to an_ability get parameter structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_autoneg_ability_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_autoneg_ability_t* an_ability_get_type
);

/***************************************************************************//**
 \brief    To enable the autoneg
 \param    phy   [In] Pointer to phymod phy access structure
 \param    an    [In] Pointer to AN control structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_autoneg_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_autoneg_control_t* an
);

/***************************************************************************//**
 \brief    To get the autoneg status
 \param    phy   [In] Pointer to phymod phy access structure
 \param    an    [Out]Pointer to AN control structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_autoneg_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_autoneg_control_t* an,
    uint32_t *an_done
);

/***************************************************************************//**
 \brief    To set the firmware lane configuration
 \param    phy            [In] Pointer to phymod phy access structure
 \param    fw_lane_config [In] Firmware lane configuration parameter structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_phy_firmware_lane_config_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    phymod_firmware_lane_config_t fw_lane_config
);

/***************************************************************************//**
 \brief    To get the firmware lane configuration
 \param    phy            [In] Pointer to phymod phy access structure
 \param    fw_lane_config [Out]Pointer to firmware lane configuration parameter structure
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_firmware_lane_config_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    phymod_firmware_lane_config_t* fw_lane_config
);

/***************************************************************************//**
 \brief    To enable the specified interrupt
 \param    phy         [In] Pointer to phy access structure
 \param    enable      [In] To enable specfied interrupt
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_intr_enable_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t enable
);

/***************************************************************************//**
 \brief    To get the specified interrupt enable status
 \param    phy         [In] Pointer to phy access structure
 \param    enable      [Out]Pointer to interrupt enable status
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_intr_enable_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t* enable
);

/***************************************************************************//**
 \brief    To get the interrupt status
 \param    phy         [In] Pointer to phy access structure
 \param    intr_status [Out]Pointer to interrupt status
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_intr_status_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t* intr_status
);

/***************************************************************************//**
 \brief    To clear the interrupt status
 \param    phy         [In] Pointer to phy access structure
 \param    intr_clr    [In] interrupt clear information
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_intr_status_clear(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t intr_clr
);

/***************************************************************************//**
 \brief    To write data through I2C interface
 \param    phy           [In] Pointer to phy access structure
 \param    slv_dev_addr  [In] Slave device address
 \param    start_addr    [In] Start address
 \param    no_of_bytes   [In] Number of bytes to write
 \param    write_data    [In] Pointer to write data
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_phy_i2c_write(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t slv_dev_addr,
    uint32_t start_addr,
    uint32_t no_of_bytes,
    const uint8_t* write_data
);

/***************************************************************************//**
 \brief    To read data through I2C interface
 \param    phy           [In] Pointer to phy access structure
 \param    slv_dev_addr  [In] Slave device address
 \param    start_addr    [In] Start address
 \param    no_of_bytes   [In] Number of bytes to read
 \param    read_data     [In] Pointer to read data
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_INTERNAL(-1)
 *******************************************************************************/
int _plp_barchetta_phy_i2c_read(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t slv_dev_addr,
    uint32_t start_addr,
    uint32_t no_of_bytes,
    uint8_t* read_data
);

/***************************************************************************//**
 \brief    To set the GPIO configuration
 \param    phy           [In] Pointer to phy access structure
 \param    pin_no        [In] GPIO pin number
 \param    gpio_mode     [In] gpio_mode structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_gpio_config_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    int pin_no, plp_barchetta_phymod_gpio_mode_t gpio_mode
);

/***************************************************************************//**
 \brief    To get the GPIO configuration
 \param    phy           [In] Pointer to phy access structure
 \param    pin_no        [In] GPIO pin number
 \param    gpio_mode     [Out]Pointer to gpio_mode structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_gpio_config_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    int pin_no, plp_barchetta_phymod_gpio_mode_t* gpio_mode
);

/***************************************************************************//**
 \brief    To set the gpio pin value
 \param    phy           [In] Pointer to phy access structure
 \param    pin_no        [In] GPIO pin number
 \param    value         [In] GPIO pin value
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_gpio_pin_value_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    int pin_no,
    int value
);

/***************************************************************************//**
 \brief    To get the gpio pin value
 \param    phy           [In]  Pointer to phy access structure
 \param    pin_no        [In]  GPIO pin number
 \param    value         [Out] Pointer to GPIO pin value
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_gpio_pin_value_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    int pin_no,
    int* value
);

/***************************************************************************//**
 \brief    To perform warmboot initialization
 \param    core           [In] Pointer to phymod core access structure
 \return   init_data      [In] Pointer to init_data
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_warmboot_init(
    const plp_barchetta_phymod_core_access_t* core,
    void* init_data
);

/***************************************************************************//**
 \brief    To set the status of failover mode configuration
 \param    phy           [In] Pointer to phy access structure
 \param    failover_mode [In] failover mode settings. 1: Failover, 0: No Failover
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_failover_mode_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    unsigned int failover_mode
);

/***************************************************************************//**
 \brief    To get the status of failover mode configuration
 \param    phy           [In]  Pointer to phy access structure
 \param    failover_mode [Out] Pointer to failover mode get
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_failover_mode_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    unsigned int *failover_mode
);

/*****************************************************************************
 \brief    To get Link partners AN ability
 \param    phy                  [In] Pointer to phy access structure
 \param    an_ability_get_type  [Out] Pointer to AN ability get parameters types
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4), PHYMOD_E_UNAVAIL(-12)
 *******************************************************************************/
int _plp_barchetta_phy_autoneg_remote_ability_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_autoneg_ability_t* an_ability_get_type
);

/*****************************************************************************
 \brief    To reset SW database, allocated/unallocated port list and used GPREG information
 \param    phy          [In] Pointer to phy access structure
 \param    init_config  [In] Pointer to init config structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_init(
    const plp_barchetta_phymod_phy_access_t* phy,
    const plp_barchetta_phymod_phy_init_config_t* init_config
);

/*****************************************************************************/
/* Following API's used by both barchetta_cfg_seq.c and barchetta_diag_seq.c */
/*****************************************************************************/

/***************************************************************************//**
 \brief    To set the slice register
 \param    phy            [In] Pointer to phymod phy access structure
 \param    port           [In] Port number
 \param    port_lane      [In] port lane
 \param    reg_sel        [In] INGRESS/EGRESS register select
 \param    reg_type       [In] PMD/AN register type
 \param    lane_based     [In] lane based
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_set_slice(
    const plp_barchetta_phymod_phy_access_t *phy,
    int port,
    int port_lane,
    BARCHETTA_REGISTER_SELECT_T reg_sel,
    BARCHETTA_REGISTER_TYPE_T reg_type,
    int reserved
);

/***************************************************************************//**
 \brief    To get the logical lane from lane map
 \param    phy            [In] Pointer to phymod phy access structure
 \param    lane_map_index [In] Specified lane index in the lane map
 \param    logical_lane   [Out]Pointer to logical lane
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_logical_lane_from_lane_map(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint8_t lane_map_index,
    uint8_t *logical_lane
);

/***************************************************************************//**
 \brief    To retrive hardware port number from software database
 \param    phy         [In] Pointer to phymod phy access structure
 \param    pkg_info    [In] Package information
 \param    port_number [Out] Port number
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_retrieve_hardware_port_number_from_sw_database(
    const plp_barchetta_phymod_phy_access_t *phy,
    barchetta_package_info_t pkg_info,
    int *port_number
);

/***************************************************************************//**
 \brief    To get the package information
 \param    pa         [In] Pointer to phymod access structure
 \param    pkg_info   [Out]Pointer to package information
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_package_info(
    const plp_barchetta_phymod_access_t *pa,
    barchetta_package_info_t *pkg_info
);

/***************************************************************************//**
 \brief    To set the PLL index
 \param    phy         [In] Pointer to phymod phy access structure
 \param    pll_index   [In] PLL index
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_set_slice_pll_index(
    const plp_barchetta_phymod_phy_access_t* phy,
    int pll_index
);

/***************************************************************************//**
 \brief    To get the PLL index
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    pll_index   [Out] Pointer to PLL index
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_pll_index(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint8_t *pll_index
);

/***************************************************************************//**
 \brief    To configure PLL
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    ref_clock   [In]  Reference clock information
 \param    serdes_cfg  [In]  Serdes config information like refclk, plldiv, osr
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_CONFIG(-11)
 *******************************************************************************/
int _plp_barchetta_configure_pll(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_ref_clk_t ref_clock,
    barchetta_serdes_config_info_t serdes_cfg
);

/***************************************************************************//**
 \brief    To restore the lane data rate
 \param    phy            [In]  Pointer to phymod phy access structure
 \param    lane_index     [In]  Lane index information
 \param    lane_data_rate [Out] Pointer to lane data rate
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_restore_lane_data_rate(
    const plp_barchetta_phymod_phy_access_t *phy,
    uint8_t lane_index,
    barchetta_lane_data_rate_t *lane_data_rate
);

/*****************************************************************************
 \brief    To get the port lane information
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    max_lane    [In]  Maximum number of lane
 \param    lane_map    [In]  Lane map information
 \param    port_number [In]  Port number
 \param    port_lane   [Out] Pointer to port lane
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_get_port_lane(
    const plp_barchetta_phymod_phy_access_t* phy,
    int max_lane,
    int lane_map,
    int port_number,
    int *port_lane
);

/*****************************************************************************
 \brief    To detect wheather current mode is PAM4 or not.
 \param    phy      [In]  Pointer to phymod phy access structure
 \retval   1 if curent mode is PAM4, otherwise 0
 *******************************************************************************/
uint8_t _plp_barchetta_is_pam4_mode(const plp_barchetta_phymod_phy_access_t* phy) ;

/*******************************************************************************
 PURPOSE: Enable/Disable SyncE with user provided configuration.

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_synce_config_set(const plp_barchetta_phymod_phy_access_t* phy,
    const phymod_synce_cfg_t* synce_cfg);

/*******************************************************************************
 PURPOSE: Retrieve current SyncE Configuration and return to the user.

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_synce_config_get(const plp_barchetta_phymod_phy_access_t* phy,
    phymod_synce_cfg_t* synce_cfg);

/*******************************************************************************
 PURPOSE: Enables serdes diagnostics access for debugging

 COMMENT:
 *******************************************************************************/
int _plp_barchetta_srds_diag_access_enable(const plp_barchetta_phymod_phy_access_t* phy,
    const phymod_srds_diag_access_cfg_t* diag_access_cfg);

#endif
