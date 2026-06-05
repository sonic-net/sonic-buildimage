/*
 *
 * $Id: aperta_pm_seq.h Exp $
 *
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

#ifndef __APERTA_PM_SEQ_H__
#define __APERTA_PM_SEQ_H__

#include "../aperta_pm/include/portmod_internal.h"
#include <tier1/aperta_cfg_seq.h>
#include <blackhawk/tier1/common/srds_api_types.h>
#include <blackhawk/tier1/blackhawk_tsc_interface.h>
#include <blackhawk/tier1/blackhawk_tsc_dependencies.h>

/* This define is used to log the PCS debug register dump into a file */
#define APERTA_REG_DUMP_INTO_FILE

#ifdef APERTA_REG_DUMP_INTO_FILE
    #define APERTA_PCS_INFO_PRINT(fmt, args...) \
    do {                                        \
     fprintf(fp,fmt,##args);                    \
     phymod_log_formatted_message(fmt, ##args); \
    }while(0)
#else
    #define APERTA_PCS_INFO_PRINT(fmt, args...) phymod_log_formatted_message(fmt, ##args)
#endif

#define APERTA_PM_IF_ERR_RETURN(A) \
    do {   \
        int loc_err ; \
        if ((loc_err = (A)) != PHYMOD_E_NONE) \
        {  goto APERTA_ERR ; } \
    } while (0)

#define APERTA_TOD_SET(arr, max_size, tod_lw, tod_hi)             \
{                                                                 \
    for (index=0; index < max_size; index ++) {                   \
        if (index < 7) {                                          \
            arr[index] = (tod_lw >> (index << 3)) & 0xFF;         \
        } else {                                                  \
            arr[index] = (tod_hi >> (index << 3)) & 0xFF;         \
        }                                                         \
    } }

#define APERTA_PORT_NONE  (-2) /* Port selection is don't care */
#define APERTA_COMMON_REGISTER_START_LIST   {                                   \
                                                0x01008100, /* OTP           */ \
                                                0x01008110, /* RESCAL        */ \
                                                0x01008120, /* VTMON         */ \
                                                0x01008150, /* PLLCAL        */ \
                                                0x01008200, /* GCT           */ \
                                                0x01008250, /* FW GPREG      */ \
                                                0x01008270, /* GCT           */ \
                                                0x010082F0, /* BOOT          */ \
                                                0x01008B00, /* CTRL          */ \
                                                0x01008B70, /* CTRL          */ \
                                                0x01008B80, /* EXT_INTR      */ \
                                                0x01009000, /* LMI           */ \
                                                0x0100A100, /* FW_REGS       */ \
                                                0x0100A180, /* FW_REGS RCLK  */ \
                                                0x0100A3F0, /* FW_REGS DBG   */ \
                                                0x49001000, /* LINE_PMIF     */ \
                                                0x49002000, /* SYS_PMIF      */ \
                                                0x49003000, /* EIP163IF_ING  */ \
                                                0x49003040, /* EIP164IF_ING  */ \
                                                0x49004000, /* EIP163IF_EGR  */ \
                                                0x49004040, /* EIP164IF_EGR  */ \
                                                0x4900e000, /* ING_RX_PORTID */ \
                                                0x4900e100, /* ING_TX_PORTID */ \
                                                0x4900f000, /* EGR_RX_PORTID */ \
                                                0x4900f100, /* EGR_TX_PORTID */ \
                                            }

#define APERTA_COMMON_REGISTER_END_LIST     {                                   \
                                                0x0100810C, /* OTP           */ \
                                                0x0100811A, /* RESCAL        */ \
                                                0x01008129, /* VTMON         */ \
                                                0x0100815E, /* PLLCAL        */ \
                                                0x01008215, /* GCT           */ \
                                                0x0100825F, /* FW GPREG      */ \
                                                0x01008273, /* GCT           */ \
                                                0x010082FF, /* BOOT          */ \
                                                0x01008B22, /* CTRL          */ \
                                                0x01008B78, /* CTRL          */ \
                                                0x01008BAF, /* EXT_INTR      */ \
                                                0x01009025, /* LMI           */ \
                                                0x0100A17F, /* FW_REGS       */ \
                                                0x0100A18F, /* FW_REGS RCLK  */ \
                                                0x0100A3FF, /* FW_REGS DBG   */ \
                                                0x49001011, /* LINE_PMIF     */ \
                                                0x49002011, /* SYS_PMIF      */ \
                                                0x49003021, /* EIP163IF_ING  */ \
                                                0x4900307b, /* EIP164IF_ING  */ \
                                                0x49004021, /* EIP163IF_EGR  */ \
                                                0x4900407b, /* EIP164IF_EGR  */ \
                                                0x4900e005, /* ING_RX_PORTID */ \
                                                0x4900e104, /* ING_TX_PORTID */ \
                                                0x4900f005, /* EGR_RX_PORTID */ \
                                                0x4900f104, /* EGR_TX_PORTID */ \
                                            }

#define APERTA_UNUSED_VAR(x)              (void) x

#define APERTA_PM_NUM_LANES            8
#define APERTA_MAX_PORT                    8
#define PM8X50_LANE_MASK                   0xFF
#define PM8X50_MAX_PM                      32
#define APERTA_MAX_PM_INFO                 1024
#define APERTA_UNINIT_PHYS                 0xFFFF
#define APERTA_FAULT_SHIFT              12
#define APERTA_FLOW_CTRL_SHIFT          13
#define APERTA_S_F_SHIFT                14
#define APERTA_PRT_ACTIVE_SHIFT         15
#define APERTA_PRT_SPEED_SHIFT          0
#define APERTA_SPEED_PLL1_DEF 0xFF
#define APERTA_SPEED_400G    400000
#define APERTA_SPEED_200G    200000
#define APERTA_SPEED_100G    100000
#define APERTA_SPEED_10G     10000
#define APERTA_SPEED_25G     25000
#define APERTA_SPEED_26G     26000
#define APERTA_SPEED_20G     20000
#define APERTA_SPEED_40G     40000
#define APERTA_SPEED_50G     50000
#define APERTA_LD_10G        10312
#define APERTA_LD_20G        20625
#define APERTA_LD_25G        25781
#define APERTA_LD_50G        53125
#define APERTA_DIAG_MAX_REG_ADDR           20
#define APERTA_MAC_REG_COUNTER             104
#define APERTA_MAC_MEM_IDX_COUNT           13
#define APERTA_MAC_MEM_INDEX_MAX_COUNT     8
#define APERTA_FW_MSG_CONFIG_PHY           1
#define APERTA_FW_MSG_CLOCK_GEN            2
#define APERTA_FW_MSG_CONFIG_PORT          3
#define APERTA_FW_MSG_PAUSE_PORT           4
#define APERTA_FW_MSG_RESUME_PORT          5
#define APERTA_FW_MSG_ENABLE_PORT          6
#define APERTA_FW_MSG_DISABLE_PORT         7
#define APERTA_FW_MSG_FLUSH_PORT           8
#define APERTA_SC_FC_PRINT                 37
#define APERTA_DEBUG_PM_REG                18
#define APERTA_PORT_TYPE_REPEATER          0
#define APERTA_PORT_TYPE_GEARBOX           1
#define APERTA_PORT_TYPE_R_GEARBOX         2
#define APERTA_DIAG_FW_NUM_REG             10
#define APERTA_DIAG_DIR_REG                18
#define APERTA_DIAG_INDIR_REG              15

#define APERTA_FW_SP_10G               0x00
#define APERTA_FW_SP_25G               0x01
#define APERTA_FW_SP_40G               0x02
#define APERTA_FW_SP_50G_NRZ           0x03
#define APERTA_FW_SP_50G_PAM4          0x04
#define APERTA_FW_SP_100G_NRZ          0x05
#define APERTA_FW_SP_100G_PAM4         0x06
#define APERTA_FW_SP_200G_PAM4         0x07
#define APERTA_FW_SP_400G_PAM4         0x08
#define APERTA_INIT_SPD                400000
#define APERTA_CHIP_81343              0x81343
#define APERTA_CHIP_81384              0x81384
#define APERTA_CHIP_81385              0x81385
#define APERTA_CHIP_81394              0x81394
#define APERTA_CHIP_81388              0x81388
#define APERTA_CHIP_81398              0x81398
#define APERTA_CHIP_81392              0x81392
#define APERTA_REV_B0                  0xB0
#define APERTA_MAX_NO_ABILITY          20
#define APERTA_26GVCO_VALUE            170
#define APERTA_25GVCO_VALUE            165
#define APERTA_DEF_GB_Q2_LANEMAP          0x30
#define APERTA_DEF_GB_Q1_LANEMAP          0x3


#define APERTA_CALL_PM_API(P_A, _FUN)                                   \
    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);                       \
    PHYMOD_IF_ERR_RETURN(_FUN);

#define PM_8x50_INFO(pm_info) ((pm_info)->pm_data.pm8x50_db)

#define APERTA_UPDATE_LM(PHY_ID, LM)             \
    {                                               \
        int cnt;                                    \
        for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) { \
            if (_plp_aperta_pm_info[cnt].phy_id == PHY_ID) { \
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_core_access.access.lane_mask = LM; \
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_phy_access.access.lane_mask = LM; \
               break; \
            } \
    } }

#define APERTA_UPDATE_PM_INFO(PHY_ID, PHY)             \
    {                                               \
        int cnt;                                    \
        for (cnt = 0; cnt <APERTA_MAX_PM_INFO; cnt++) { \
            if (_plp_aperta_pm_info[cnt].phy_id == PHY_ID) { \
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_core_access.access.lane_mask = PHY->access.lane_mask; \
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_phy_access.access.lane_mask = PHY->access.lane_mask; \
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_phy_access.access.tvco_pll_index = APERTA_TVCO_PLL_INDEX;\
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_core_access.access.tvco_pll_index = APERTA_TVCO_PLL_INDEX;\
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_core_access.port_loc = PHY->port_loc;\
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_phy_access.port_loc = PHY->port_loc;\
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_core_access.access.user_acc = PHY->access.user_acc;\
               PM_8x50_INFO(_plp_aperta_pm_info[cnt].pm_info)->int_phy_access.access.user_acc = PHY->access.user_acc;\
               break; \
            } \
    } }
#define APERTA_PORT_CONSTRUCTION(PHY_ID, PM_PORT)     (PHY_ID << 8) | (PM_PORT)


#define APERTA_GET_PORT_FROM_LM_SP(SP,LD,LM,PRT,LN_SEL)                 \
    if ((SP == APERTA_SPEED_100G || SP == APERTA_SPEED_40G) &&          \
         (LD == APERTA_LD_25G || LD == 25000 || LD == APERTA_LD_10G || LD == 10000)) {   \
        if (LM == 0x0 || LM == 0xF) {                                   \
            PRT = 0; LN_SEL = 0xF;                                      \
        } else if (LM == 0xF0) {                                        \
            PRT = 4; LN_SEL = 0xF0;                                                      \
        } else {                                                        \
            if ( LM & 0xF) { PRT = 0; } else { PRT = 4;}                                 \
            LN_SEL = LM;                                                                 \
        }                                                               \
    } else if ((SP == APERTA_SPEED_10G || SP == APERTA_SPEED_25G)       \
        || ((SP == APERTA_SPEED_50G) && (LD == APERTA_LD_50G || LD == 50000))) {         \
        if (LM == 0x0 || LM == 0x1) {                                   \
            LN_SEL = 1; PRT = 0;                                        \
        } else if (LM == 0x2) {                                         \
            LN_SEL = LM; PRT = 1;                                       \
        } else if (LM == 0x4) {                                         \
            LN_SEL = LM; PRT = 2;                                       \
        } else if (LM == 0x8) {                                         \
            LN_SEL = LM; PRT = 3;                                       \
        } else if (LM == 0x10) {                                        \
            LN_SEL = LM; PRT = 4;                                       \
        } else if (LM == 0x20) {                                        \
            LN_SEL = LM; PRT = 5;                                       \
        } else if (LM == 0x40) {                                        \
            LN_SEL = LM; PRT = 6;                                       \
        } else if (LM == 0x80) {                                        \
            LN_SEL = LM; PRT = 7;                                       \
        } else {                                                        \
            LN_SEL = 1; PRT = 0;                                        \
        }                                                               \
    } else if (SP == APERTA_SPEED_400G) {                               \
        LN_SEL = LM; PRT = 0;                                           \
    } else if (SP == APERTA_SPEED_200G) {                               \
        if (LM == 0x0 || LM == 0xF) {                                   \
            LN_SEL = 0xF; PRT = 0;                                      \
        } else if (LM == 0xF0) {                                        \
            LN_SEL = LM; PRT = 4;                                       \
        } else {                                                        \
            LN_SEL = LM; PRT = 0;                                       \
        }                                                               \
    } else if ((SP == APERTA_SPEED_50G && (LD == APERTA_LD_25G || LD == 25000))          \
           || (SP == APERTA_SPEED_100G && (LD == APERTA_LD_50G || LD == 50000))          \
           || (SP == APERTA_SPEED_40G && (LD == APERTA_LD_20G || LD == 20000))) {        \
        if (LM == 0x3 || LM == 0) {                                     \
            PRT = 0; LN_SEL = 0x3;                                      \
        } else if (LM == 0xc) {                                         \
            LN_SEL = LM; PRT = 2;                                       \
        } else if (LM == 0x30) {                                        \
            LN_SEL = LM; PRT = 4;                                       \
        } else if (LM == 0xC0) {                                        \
            LN_SEL = LM; PRT = 6;                                       \
        } else {                                                        \
            LN_SEL = LM;                                                \
            PRT = (LM & 0x3) ? 0 : ((LM & 0xC) ? 2: (LM & 0x30) ? 4 : 6);               \
        }                                                               \
    }

#define APERTA_PORT_SPEED(SP,LDR,PRT_SPD)          \
    if (SP == APERTA_SPEED_400G) {                 \
        PRT_SPD = 0xC;                             \
    } else if (SP == APERTA_SPEED_200G) {          \
        if (LDR >= APERTA_LD_50G) {                \
            PRT_SPD = 0xB;                         \
        } else {                                   \
            PRT_SPD = 0xA;                         \
        }                                          \
    } else if (SP == APERTA_SPEED_100G) {          \
        if (LDR >= APERTA_LD_50G) {                \
            PRT_SPD = 0x9;                         \
        } else {                                   \
            PRT_SPD = 0x8;                         \
        }                                          \
    } else if (SP == APERTA_SPEED_50G) {           \
        if (LDR >= APERTA_LD_50G) {                \
            PRT_SPD = 0x7;                         \
        } else {                                   \
            PRT_SPD = 0x6;                         \
        }                                          \
    } else if (SP == APERTA_SPEED_40G) {           \
        if (LDR >= APERTA_LD_20G) {                \
            PRT_SPD = 0x5;                         \
        } else {                                   \
            PRT_SPD = 0x4;                         \
        }                                          \
    } else if (SP == APERTA_SPEED_25G) {           \
            PRT_SPD = 0x3;                         \
    } else if (SP == APERTA_SPEED_20G) {           \
        if (LDR >= APERTA_LD_20G) {                \
            PRT_SPD = 0x2;                         \
        } else {                                   \
            PRT_SPD = 0x1;                         \
        }                                          \
    } else if (SP == APERTA_SPEED_10G) {           \
            PRT_SPD = 0x0;                         \
    } else {                                       \
            PRT_SPD = 0x8;                         \
    }

#define APERTA_GET_LM_FROM_PORT(SP,LD,PRT,LM)                           \
    if ((SP == APERTA_SPEED_100G || SP == APERTA_SPEED_40G) &&          \
         (LD == APERTA_LD_25G || LD == 25000 || LD == APERTA_LD_10G || LD == 10000)) {   \
        if (PRT == 0) {                                                 \
            LM=0xF;                                                     \
        } else if (PRT == 4) {                                          \
            LM=0xF0;                                                    \
        }                                                               \
    } else if ((SP == APERTA_SPEED_10G || SP == APERTA_SPEED_25G)       \
        || ((SP == APERTA_SPEED_50G) && (LD == APERTA_LD_50G || LD == 50000))) {         \
        if (PRT == 0) {                                                 \
            LM = 1;                                                     \
        } else if (PRT == 1) {                                          \
            LM = 0x2;                                                   \
        } else if (PRT == 2) {                                          \
            LM = 0x4;                                                   \
        } else if (PRT == 0x3) {                                        \
            LM = 0x8;                                                   \
        } else if (PRT == 4) {                                          \
            LM = 0x10;                                                  \
        } else if (PRT == 5) {                                          \
            LM = 0x20;                                                  \
        } else if (PRT == 6) {                                          \
            LM = 0x40;                                                  \
        } else if (PRT == 0x7) {                                        \
            LM = 0x80;                                                  \
        }                                                               \
    } else if (SP == APERTA_SPEED_400G) {                               \
        LM = 0xFF;                                                      \
    } else if (SP == APERTA_SPEED_200G) {                               \
        if (PRT == 0) {                                                 \
            LM = 0xF;                                                   \
        } else if (PRT == 4) {                                          \
            LM = 0xF0;                                                  \
        }                                                               \
    } else if ((SP == APERTA_SPEED_50G && (LD == APERTA_LD_25G || LD == 25000))          \
           || (SP == APERTA_SPEED_100G && (LD == APERTA_LD_50G || LD == 50000))          \
           || (SP == APERTA_SPEED_40G && (LD == APERTA_LD_20G || LD == 20000))) {        \
        if (PRT == 0) {                                                 \
            LM = 0x3;                                                   \
        } else if (PRT == 2) {                                          \
            LM = 0xC;                                                   \
        } else if (PRT == 4) {                                          \
            LM = 0x30;                                                  \
        } else if (PRT == 6) {                                          \
            LM = 0xc0;                                                  \
        }                                                               \
    }

#define APERTA_GET_PORT_FROM_LM(LM,PRT)     \
    if (LM == 0xF || LM == 0xFF) {         \
        PRT = 0x0;                         \
    } else if (LM == 0xF0) {               \
        PRT = 0x4;                         \
    } else if(LM == 0x3) {                 \
        PRT = 0x0;                         \
    } else if(LM == 0xC) {                 \
        PRT = 0x2;                         \
    } else if(LM == 0x30) {                \
        PRT = 0x4;                         \
    } else if(LM == 0xC0) {                \
        PRT = 0x6;                          \
    } else if (LM == 0x1) {                \
        PRT = 0x0;                         \
    }  else if (LM == 0x2) {               \
        PRT = 0x1;                         \
    }  else if (LM == 0x4) {               \
        PRT = 0x2;                         \
    }  else if (LM == 0x8) {               \
        PRT = 0x3;                         \
    } else if (LM == 0x10) {               \
        PRT = 0x4;                         \
    }  else if (LM == 0x20) {              \
        PRT = 0x5;                         \
    }  else if (LM == 0x40) {              \
        PRT = 0x6;                         \
    }  else if (LM == 0x80) {              \
        PRT = 0x7;                         \
    } else {                               \
        PRT = 0;                           \
    }

#define APERTA_LM_TO_POSSIBLE_PORT_LIST(lm, list, max_port) \
    if(lm == 0xF) {                                         \
        list = 0x0123;                                      \
        max_port = 4;                                       \
    } else if(lm == 0xF0) {                                 \
        /*list = 0x4567;*/                                      \
        list = 0x7654;                                      \
        max_port = 4;                                       \
    } else if(lm == 0x3) {                                  \
        list = 0x01;                                        \
        max_port = 2;                                       \
    } else if(lm == 0xc) {                                  \
        list = 0x23;                                        \
        max_port = 2;                                       \
    } else if(lm == 0x30) {                                 \
        list = 0x45;                                        \
        max_port = 2;                                       \
    } else if(lm == 0xc0) {                                 \
        list = 0x67;                                        \
        max_port = 2;                                       \
    } else {\
        if (plp_aperta_phymod_count_set_bits(lm) == 1) {\
            /*Just to handle single lane*/\
            list = plp_aperta_log2n(lm);\
            max_port=1;\
        }\
    }

#define APERTA_FW_DR_USER_DR(fw_sp, user_speed, user_ldr)      \
    if (fw_sp == APERTA_FW_SP_10G) {                           \
        user_speed = APERTA_SPEED_10G;                         \
        user_ldr = APERTA_LD_10G;                              \
    } else if (fw_sp == APERTA_FW_SP_25G) {                    \
        user_speed = APERTA_SPEED_25G;                         \
        user_ldr = APERTA_LD_25G;                              \
    } else if (fw_sp == APERTA_FW_SP_40G) {                    \
        user_speed = APERTA_SPEED_40G;                         \
        user_ldr = APERTA_LD_10G;                              \
    } else if (fw_sp == APERTA_FW_SP_50G_NRZ) {                \
        user_speed = APERTA_SPEED_50G;                         \
        user_ldr = APERTA_LD_25G;                              \
    } else if (fw_sp == APERTA_FW_SP_50G_PAM4) {               \
        user_speed = APERTA_SPEED_50G;                         \
        user_ldr = APERTA_LD_50G;                              \
    } else if (fw_sp == APERTA_FW_SP_100G_NRZ) {               \
        user_speed = APERTA_SPEED_100G;                        \
        user_ldr = APERTA_LD_25G;                              \
    } else if (fw_sp == APERTA_FW_SP_100G_PAM4) {              \
        user_speed = APERTA_SPEED_100G;                        \
        user_ldr = APERTA_LD_50G;                              \
    } else if (fw_sp == APERTA_FW_SP_200G_PAM4) {              \
        user_speed = APERTA_SPEED_200G;                        \
        user_ldr = APERTA_LD_50G;                              \
    } else if (fw_sp == APERTA_FW_SP_400G_PAM4) {              \
        user_speed = APERTA_SPEED_400G;                        \
        user_ldr = APERTA_LD_50G;                              \
    }


#define APERTA_LMAP_TO_NUM(LMAP, NUM)                       \
    switch(LMAP) {                                          \
        case 0x1: NUM=0; break;                             \
        case 0x2: NUM=1; break;                             \
        case 0x4: NUM=2; break;                             \
        case 0x8: NUM=3; break;                             \
        case 0x3: NUM=4; break;                             \
        case 0xc: NUM=5; break;                             \
        case 0xF: NUM=6; break;                             \
        case 0x10: NUM=7; break;                            \
        case 0x20: NUM=8; break;                            \
        case 0x40: NUM=9; break;                            \
        case 0x80: NUM=0xa; break;                          \
        case 0x30: NUM=0xb; break;                          \
        case 0xc0: NUM=0xc; break;                          \
        case 0xF0: NUM=0xd; break;                          \
        case 0xFF: NUM=0xe; break;                          \
        default: NUM=0x1; break;}

#define APERTA_NUM_TO_LMAP(NUM, LMAP)                       \
    switch(NUM) {                                           \
        case 0: LMAP=0x1; break;                            \
        case 1: LMAP=0x2; break;                            \
        case 2: LMAP=0x4; break;                            \
        case 3: LMAP=0x8; break;                            \
        case 4: LMAP=0x3; break;                            \
        case 5: LMAP=0xc; break;                            \
        case 6: LMAP=0xF; break;                            \
        case 0x7: LMAP=0x10; break;                         \
        case 0x8: LMAP=0x20; break;                         \
        case 0x9: LMAP=0x40; break;                         \
        case 0xa: LMAP=0x80; break;                         \
        case 0xb: LMAP=0x30; break;                         \
        case 0xc: LMAP=0xc0; break;                         \
        case 0xd: LMAP=0xF0; break;                         \
        case 0xe: LMAP=0xFF; break;                         \
        default: LMAP=0x1; break;}

#define APERTA_GET_PLL_MICRO_SEL(PHY, PLL_SEL, MICRO_SEL)   \
    PLL_SEL = PHY->access.pll_idx;

#define APERTA_GET_PORT_SPEED_LDR(phy, PSP, LDR)  {     \
    int lane_map;                                       \
    PHYMOD_IF_ERR_RETURN(                               \
        plp_aperta_pm_info_speed_get(phy, &PSP, &lane_map)); \
    if (lane_map == 0x0) {                               \
        PHYMOD_DIAG_OUT(("Lanemap cannot be 0, Checkit\n"));        \
        lane_map = 0xf;                                  \
    }                                                    \
    LDR = (int)PSP/plp_aperta_count_no_bits(lane_map);      }


#define APERTA_GET_PORT_UPDATE_PM_INFO_LM(PHY, port)  \
    APERTA_GET_PORT_FROM_LM(PHY->access.lane_mask, int_port);\
    port = APERTA_PORT_CONSTRUCTION(PHY->access.addr, int_port);\
    APERTA_UPDATE_PM_INFO(PHY->access.addr, PHY);

#define APERTA_TS_PORT_RESUME(ret,pre_cmd,port)            \
    if (ret != PHYMOD_E_NONE) {                              \
        PHYMOD_DEBUG_ERROR((pre_cmd "port failed :%d. Resuming port:%d\n", ret, port)); \
        PHYMOD_IF_ERR_RETURN(                                                                \
            plp_aperta_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA_FW_MSG_RESUME_PORT, NULL, 0)); \
        return ret;}


#define APERTA_TVCO_PLL_INDEX            1

#define APERTA_ISCOREINITIALIZED         0
#define APERTA_ISACTIVE                  1
#define APERTA_ISBYPASSED                2
#define APERTA_PLL0ACTIVELANEBITMAP      3
#define APERTA_PLL1ACTIVELANEBITMAP      4
#define APERTA_SYS_PLL0ACTIVELANEBITMAP  5
#define APERTA_SYS_PLL1ACTIVELANEBITMAP  6
#define APERTA_PLL0ADVLANEBITMAP         7
#define APERTA_PLL1ADVLANEBITMAP         8
#define APERTA_SYS_PLL0ADVLANEBITMAP     9
#define APERTA_SYS_PLL1ADVLANEBITMAP     10
#define APERTA_SPEED                     11
#define APERTA_FWDLOAD                   12
#define APERTA_SYS_SPEED                 13
#define APERTA_TS_MODE_FLAG_SHIFT        23

#define APERTA_PCS_UPDATE(PHY_ACC, LINE_LANE_MAP)            \
    PHYMOD_IF_ERR_RETURN(                                    \
        plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 0));               \
    /* Updated  WAR*/                                        \
    PHYMOD_IF_ERR_RETURN(                                    \
        plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 4000));            \
    fo_lane_map = plp_aperta_is_fo_enabled(PHY_ACC, &side);    \
    if ((fo_lane_map != 0xFFFF) && (side == phy_copy.port_loc)) {                                  \
        phy_copy.access.lane_mask =  fo_lane_map;              \
        phy_copy.port_loc = side;                              \
        PHYMOD_IF_ERR_RETURN(                                           \
        plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 0));           \
        /* Updated  WAR*/                                               \
        PHYMOD_IF_ERR_RETURN(                                           \
            plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 4000));        \
    }                                                                   \
    phy_copy.access.lane_mask = LINE_LANE_MAP;               \
    phy_copy.port_loc = phymodPortLocLine ;                  \
    /* Disable PCS*/                                         \
    PHYMOD_IF_ERR_RETURN(                                    \
       plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 0));                \
    /* Updated  WAR*/                                        \
    PHYMOD_IF_ERR_RETURN(                                    \
       plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 4000));  \
    fo_lane_map = plp_aperta_is_fo_enabled(PHY_ACC, &side);    \
    if ((fo_lane_map != 0xFFFF) && (side == phy_copy.port_loc)) {                                  \
        phy_copy.access.lane_mask =  fo_lane_map;              \
        phy_copy.port_loc = side;                              \
        PHYMOD_IF_ERR_RETURN(                                           \
           plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 0));           \
        /* Updated  WAR*/                                               \
        PHYMOD_IF_ERR_RETURN(                                           \
           plp_aperta_tscbh_phy_pcs_enable_set(PHY_ACC, 4000));        \
    }


#define APERTA_OUTPUT_BUFFER_READ                           \
        addr = MSG_OUT_BUFFER_ADDR;                         \
         PHYMOD_IF_ERR_RETURN(                              \
            PHYMOD_BUS_READ(&phy->access,  addr, &length)); \
        plp_aperta_put_half_word (&rx_msg, length);             \
        addr++;                                             \
        length = (length + 1) / 2;                          \
        while (length) {                                    \
              PHYMOD_IF_ERR_RETURN(                            \
                PHYMOD_BUS_READ(&phy->access,  addr, &value)); \
            plp_aperta_put_half_word (&rx_msg, value);             \
            addr++;                                            \
            length--;                                          \
        }

#define APERTA_FW_FUN_SUPPORT_START_RESULT(FUN)    \
     (FUN == APERTA_FUNC_ENABLE_PORT  ||      \
      FUN == APERTA_FUNC_DISABLE_PORT ||      \
      FUN == APERTA_FUNC_FLUSH_PORT   ||      \
      FUN == APERTA_FUNC_MACSEC_INIT_EIP164 ||\
      FUN == APERTA_FUNC_SWITCH_MUX )

#define APERTA_PORT_DISABLE_PH1          0x10000
#define APERTA_PORT_DISABLE_PH2          0x20000

/* Possible Port list*/
#define APERTA_PT          0
#define APERTA_GB          1
#define APERTA_RGB         2

#define APERTA_MAX_GB_RGB_PORT             15
#define APERTA_MAX_GB_RGB_PORT_ENTITY      3
#define APERTA_GB_RGB_PORT_TYPE        0
#define APERTA_GB_RGB_PORT_SYS_LANE    1
#define APERTA_GB_RGB_PORT_LINE_LANE   2

/* If port in allocated State, perform Phase1 Disable and mark the 
 * GPREG as P1 Complete, If Phase1 is completed perform P2 Disable and mark the 
 * Port as deallocated*/

#define APERTA_DISABLE_PORT \
for (port_index = 0; port_index < max_ports; port_index++) {\
            temp = (possible_port_list >> port_index*4) & 0xF;\
            if (enabled_port_list[temp] && (temp != prev_port)) {\
                phase &= ~(0xF); \
                phase |= temp &0xF;\
                if ((!(disable_port_phase.v[0] & (1 << (temp&0xF)))) &&\
                        (phase & APERTA_PORT_DISABLE_PH1)) { \
                    PHYMOD_CRIT_INFO(("##Previous ena-dis port:%x## PH1\n", phase));\
                    PHYMOD_IF_ERR_RETURN(\
                            plp_aperta_send_fw_msg(phy, 0, NULL,\
                                APERTA_FW_MSG_DISABLE_PORT, &phase, 0));\
                    disable_port_phase.v[0] &= ~(1 << (temp&0xF));\
                    disable_port_phase.v[0] |= ( 1 << (temp&0xF));\
                } else if ((disable_port_phase.v[0] & (1 << (temp & 0xF))) && \
                        (phase & APERTA_PORT_DISABLE_PH2)) { \
                    PHYMOD_CRIT_INFO(("##Previous ena-dis port:%x## PH2\n", phase));\
                    PHYMOD_IF_ERR_RETURN(\
                        plp_aperta_send_fw_msg(phy, 0, NULL,\
                           APERTA_FW_MSG_DISABLE_PORT, &phase, 0));\
                    PHYMOD_IF_ERR_RETURN(\
                        plp_aperta_port_active_reset(phy, temp));\
                    disable_port_phase.v[0] &= ~(1 << (temp&0xF));\
                } else { /*Nothing Needed*/ \
                }\
            }\
        }

typedef struct aperta_pm_info_s {
    pm_info_t pm_info;
    int phy_id;
    int is_fw_dloaded;
    int speed[8]; /* Speed associated to lane*/
    int sys_speed[8]; /* Speed associated to lane*/
} aperta_pm_info_t;

typedef enum aperta_pm_flow_control_e {
    apertaFlowcontrolTerminateGenerate = 0,
    apertaFlowcontrolPassthrough
} aperta_pm_flow_control_t;

typedef enum aperta_pm_fault_option_e {
    apertaFaultoptionTerminateGenerate = 0,
    apertaFaultoptionPassthrough
} aperta_pm_fault_option_t;

typedef enum bcm_plp_aperta_pll_vco_e{
    bcmplpapertaVco20p625G = 0,
    bcmplpapertaVco25p781G = 1,
    bcmplpapertaVco26p562G = 2
}bcm_plp_aperta_pll_vco_t;

/*struct for 1588 tx info*/
typedef struct aperta_pm_ts_tx_info_s {
    uint32_t ts_in_fifo_lo; /**< low 32bit of Timestamp in Fifo */
    uint32_t ts_in_fifo_hi; /**< high 32bit of Timestamp in Fifo */
    uint32_t ts_seq_id; /**< sequence id of tx 1588 packet */
    uint32_t ts_sub_nanosec; /**< sub nanoseconds of tx 1588 packet */
} aperta_pm_ts_tx_info_t;

/*!bcm_plp_aperta_fw_init_params_t
  * This structure is intended to initialize HW configuration after FW download
  * \arg macsec_bypass\n
  * When set to 1, macsec(EIP 163 & EIP 164) is bypassed otherwise user needs to 
  * initialize MACSEC.
  * \arg pll1_vco_rate\n
  * Used to choose the max vco rate for PLL1.
  *
  * \arg tx_drv_supply\n
  * Forced value for tx driver supply.
  * 1 : TVDD1P25 supply is set to 1.0 V 
  * 0 : TVDD1P25 supply is set to 1.25 V 
  * 
  */
typedef struct aperta_fw_init_s {
    unsigned int  macsec_static_bypass;
    bcm_plp_aperta_pll_vco_t  pll1_vco_rate;
    unsigned int  tx_drv_supply;
} aperta_fw_init_t;

int plp_aperta_count_no_bits(int data);
int plp_aperta_add_pm_info(int phy_id, pm_info_t pm_info);
int plp_aperta_remove_pm_info(int phy_id);
void plp_aperta_plp_aperta_update_pm_info(int phy_id, uint32_t wb_idx, int val);
void plp_aperta_plp_aperta_get_wb_pm_info(int phy_id, uint32_t wb_idx, int *val);
int plp_aperta_get_pm_info(int phy_id, pm_info_t pm_info);
int plp_aperta_portmod_pm_info_get(int unit, int port, pm_info_t* pm_info);
int plp_aperta_portmod_port_pm_type_get(int unit, int port, portmod_dispatch_type_t* type);
int plp_aperta_portmod_port_chain_phy_access_get(int unit, int port, pm_info_t pm_info, plp_aperta_phymod_phy_access_t* core_access_arr, int max_buf, int* nof_cores);
int plp_aperta_portmod_port_chain_core_access_get(int unit, int port, pm_info_t pm_info, plp_aperta_phymod_core_access_t* core_access_arr, int max_buf, int* nof_cores);
int plp_aperta_port_speed_set(const plp_aperta_phymod_phy_access_t* phy, int data_rate, int lane_data_rate);
int aperta_chip_config(const plp_aperta_phymod_phy_access_t* phy, int data_rate);
int plp_aperta_port_active_set(const plp_aperta_phymod_phy_access_t* phy, int data_rate, int lane_data_rate);
int plp_aperta_send_fw_msg(const plp_aperta_phymod_phy_access_t *phy,
                       int data_rate, aperta_device_aux_modes_t *aux_mode,
                       int msg, void *data, int read);
int plp_aperta_pm_is_fw_dloaded_set(int phy_id, uint32_t active);
int plp_aperta_pm_is_fw_dloaded_get(int phy_id, uint32_t *active);
int plp_aperta_pm_info_speed_set(const plp_aperta_phymod_phy_access_t* phy, int speed, aperta_port_type_t port_type);
int plp_aperta_pm_info_speed_get(const plp_aperta_phymod_phy_access_t* phy, int *speed, int *);
int plp_aperta_pm_warmboot_init(const plp_aperta_phymod_phy_access_t* phy);
int plp_aperta_pm_init(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_core_init_config_t* init_config, const plp_aperta_phymod_core_status_t* core_status);
int plp_aperta_core_add(const plp_aperta_phymod_core_access_t* core, const plp_aperta_phymod_phy_init_config_t* init_config,
                    const plp_aperta_phymod_core_status_t* core_status);
int plp_aperta_port_attach(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_phy_init_config_t* init_config) ;
int plp_aperta_pm_interface_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta_phymod_phy_inf_config_t* config);
int plp_aperta_pm_interface_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_phy_inf_config_t* config);
int plp_aperta_pm_link_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *link_status);
int plp_aperta_pm_loopback_set(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t enable);
int plp_aperta_pm_loopback_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_loopback_mode_t loopback, uint32_t *enable);
int plp_aperta_pm_fault_option_set(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_fault_option_t fault_option);
int plp_aperta_pm_fault_option_get(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_fault_option_t *fault_option);
int plp_aperta_pm_flow_control_set(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_flow_control_t flow_option);
int plp_aperta_pm_flow_control_get(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_flow_control_t *flow_option);
int plp_aperta_pm_store_and_forward_mode_set(const plp_aperta_phymod_phy_access_t* phy, int enable);
int plp_aperta_pm_store_and_forward_mode_get(const plp_aperta_phymod_phy_access_t* phy, int *is_enable);
int plp_aperta_pm_max_pkt_size_set(const plp_aperta_phymod_phy_access_t* phy, int size);
int plp_aperta_pm_max_pkt_size_get(const plp_aperta_phymod_phy_access_t* phy, int *size);
int plp_aperta_pm_runt_threshold_set(const plp_aperta_phymod_phy_access_t* phy, int threshold);
int plp_aperta_pm_runt_threshold_get(const plp_aperta_phymod_phy_access_t* phy, int *threshold);
int plp_aperta_pm_pad_size_set(const plp_aperta_phymod_phy_access_t* phy, int pad_size);
int plp_aperta_pm_pad_size_get(const plp_aperta_phymod_phy_access_t* phy, int *pad_size);
int plp_aperta_pm_tx_mac_sa_set(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta_pm_tx_mac_sa_get(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta_pm_rx_mac_sa_set(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta_pm_rx_mac_sa_get(const plp_aperta_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta_pm_tx_avg_ipg_set(const plp_aperta_phymod_phy_access_t* phy, int avg_ipg);
int plp_aperta_pm_tx_avg_ipg_get(const plp_aperta_phymod_phy_access_t* phy, int *avg_ipg);
int plp_aperta_pm_tx_preamble_length_set(const plp_aperta_phymod_phy_access_t* phy, int preamble_length);
int plp_aperta_pm_tx_preamble_length_get(const plp_aperta_phymod_phy_access_t* phy, int *preamble_length);
int plp_aperta_pm_pause_control_set(const plp_aperta_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control);
int plp_aperta_pm_pause_control_get(const plp_aperta_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control);
int plp_aperta_pm_pfc_control_set(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl);
int plp_aperta_pm_pfc_control_get(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl);
int plp_aperta_pm_llfc_control_set(const plp_aperta_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl);
int plp_aperta_pm_llfc_control_get(const plp_aperta_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl);
int plp_aperta_pm_pfc_config_set(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc);
int plp_aperta_pm_pfc_config_get(const plp_aperta_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc);
int plp_aperta_pm_diagnostic_dump(const plp_aperta_phymod_phy_access_t* phy);
int plp_aperta_get_speed_from_bits(uint32_t speed_in_bits);
int plp_aperta_restore_pm_info(const plp_aperta_phymod_phy_access_t* phy );
int aperta_warmboot_init(const plp_aperta_phymod_phy_access_t* phy);
int plp_aperta_convert_speed_to_bits(uint32_t speed);
int plp_aperta_write_warmboot_reg( int phy_id , uint32_t type , int value,int lane_index,int lane_map);
int aperta_pm_is_pminitialized_set(int phy_id, uint32_t active);
int aperta_pm_is_pminitialized_get(int phy_id, uint32_t *active);
int plp_aperta_rx_mac_enable_set(const plp_aperta_phymod_phy_access_t* phy, int enable);
int plp_aperta_tx_mac_enable_set(const plp_aperta_phymod_phy_access_t* phy, int enable);
int plp_aperta_rx_mac_enable_get(const plp_aperta_phymod_phy_access_t* phy, int *enable);
int plp_aperta_tx_mac_enable_get(const plp_aperta_phymod_phy_access_t* phy, int *enable);
int plp_aperta_pcs_info_dump(const plp_aperta_phymod_phy_access_t* phy);
int plp_aperta_pm_tx_rx_enable(const plp_aperta_phymod_phy_access_t* phy, int enable, int single_port, int failover) ;
int plp_aperta_pm_tx_rx_enable_post(const plp_aperta_phymod_phy_access_t* phy, int enable, int single_port, uint16_t fo_side_lm) ;
int plp_aperta_disable_port(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t* config, int prev_port, int phase);
int plp_aperta_pm_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable);
int plp_aperta_pm_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable); 
int plp_aperta_pm_timesync_tx_info_get(const plp_aperta_phymod_phy_access_t* phy, aperta_pm_ts_tx_info_t* ts_tx_info) ;
int plp_aperta_update_vco(plp_aperta_phymod_phy_access_t *phy, int pll, int val) ;
int plp_aperta_pm_synce_config_get(const plp_aperta_phymod_phy_access_t* phy, phymod_synce_cfg_t* synce_cfg);
int plp_aperta_pm_synce_config_set(const plp_aperta_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg);
int plp_aperta_get_enabled_port(const plp_aperta_phymod_phy_access_t *phy, uint8_t *enabled_port_list) ;
int plp_aperta_pm_reset_pcs(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t* config);
int plp_aperta_fec_status_get(const plp_aperta_phymod_phy_access_t* phy, phymod_phy_fec_dump_status_t* fec_sts);
int plp_aperta_pm_mac_mib_stat_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t stat_type, uint64_t *count);
int plp_aperta_pm_is_synce_enabled(plp_aperta_phymod_phy_access_t *phy, unsigned int pll_idx, int *is_synce_enabled);
int plp_aperta_port_active_reset(const plp_aperta_phymod_phy_access_t* phy, int port);

int plp_aperta_flush_port(const plp_aperta_phymod_phy_access_t* phy) ;
int plp_aperta_update_port_config(const plp_aperta_phymod_phy_access_t *phy, aperta_update_port_config_t *port_lat_config); 
uint8_t plp_aperta_log2n(uint32_t n);
int plp_aperta_get_pll1_div (const plp_aperta_phymod_phy_access_t *phy, int *div);
int plp_aperta_pm_is_fo_enabled(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_phy_inf_config_t* config,
                            unsigned int *is_fo_enabled, unsigned int *pri_port, 
                            unsigned int *primary_lm); 
int plp_aperta_is_fo_enabled(const plp_aperta_phymod_phy_access_t *phy, int *side);
int _plp_aperta_phy_autoneg_remote_ability_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_autoneg_ability_t* an_ability_get_type);
int _plp_aperta_pcs_status_get(const plp_aperta_phymod_phy_access_t* phy, phymod_pcs_status_t* pcs_status);
#endif


