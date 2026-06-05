/*
 *
 * $Id: aperta2_pm_seq.h Exp $
 *
 *
 *
 *
 * $Copyright: (c) 2022 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 *
 *
 *
 */

#ifndef __APERTA2_PM_SEQ_H__
#define __APERTA2_PM_SEQ_H__
#include <include/portmod_internal.h>
#include <tier1/aperta2_msg_task.h>
#include <tier1/aperta2_cfg_seq.h>
#include <tier1/bcm_aperta2_direct_defs.h>
#include <peregrine5_pc_dependencies.h>
#include <phymod/phymod_util.h>

#define APERTA2_SPEED_800G        800000
#define APERTA2_SPEED_400G        400000
#define APERTA2_SPEED_200G        200000
#define APERTA2_SPEED_100G        100000
#define APERTA2_SPEED_50G          50000
#define APERTA2_SPEED_25G          25000
#define APERTA2_SPEED_10G          10000

#define APERTA2_LD_25G             25781
#define APERTA2_LD_50G             53125
#define APERTA2_LD_100G            106250
#define APERTA2_LD_10G             10312
#define APERTA2_LD_ROFF_25G        25000
#define APERTA2_LD_ROFF_50G        50000
#define APERTA2_LD_ROFF_100G       100000
#define APERTA2_LD_ROFF_10G        10000

#define APERTA2_PM_OCTAL1              1
#define APERTA2_PM_OCTAL2              2
#define APERTA2_MAX_OCTAL              2
#define APERTA2_MSG_CONFIG_PHY         0x1
#define APERTA2_MSG_CLOCK_GEN          0x2
#define APERTA2_MSG_CONFIG_PORT        0x3
#define APERTA2_MSG_PAUSE_PORT         0x4
#define APERTA2_MSG_ENABLE_PORT        0x5
#define APERTA2_MSG_DISABLE_PORT       0x6
#define APERTA2_MSG_FLUSH_PORT         0x7
#define APERTA2_MSG_RESUME_PORT        0x8
#define APERTA2_MAX_PM_INFO            1024
#define APERTA2_MAX_PHY                1024
#define APERTA2_UNINIT_PHYS            0xFFFF
#define APERTA2_LINE_INIT_SPEED        APERTA2_SPEED_100G 
#define APERTA2_SYS_INIT_SPEED         APERTA2_SPEED_100G
#define APERTA2_PORT_DISABLE_PH1       0x10000
#define APERTA2_PORT_DISABLE_PH2       0x20000
#define APERTA2_MAX_PORT               16
#define APERTA2_PORT_ACTIVE_SHIFT      15
#define APERTA2_PORT_SF_SHIFT          14
#define APERTA2_PORT_FC_SHIFT          13
#define APERTA2_PORT_FAULT_SHIFT       12
#define APERTA2_PORT_SPD_SHIFT          0
#define APERTA2_INIT_LANE_CONFIG       0x5040
#define APERTA2_EIGHT_LANE_PORT         8
#define APERTA2_TS_MODE_FLAG_SHIFT      23
#define APERTA2_LANES_PER_OCTAL         8
#define APERTA2_MAX_NO_ABILITY          20
#define APERTA2_FDR_SIZE                8
#define APERTA2_FDR_MAX_ROW             3

#define APERTA2_SWAP_OCTAL(LM)    \
    LM = (LM & 0xFF) ? ((LM & 0xFF) << 8) : ((LM & 0xFF00) >> 8)

/* Define below macro to avoid PM broadcast during init*/
#define APERTA2_OCTAL_NO_BCAST          1

typedef struct aperta2_port_info_s {
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int sys_speed;
    unsigned int line_speed;
    unsigned int sys_fo_map;
    unsigned int line_fo_map;
} aperta2_port_info_t;

typedef struct aperta2_pm_info_s {
    pm_info_t pm_info;
    int phy_id;
    int is_fw_dloaded;
    int init_state;
    int is_db_initialized;
    aperta2_port_info_t port_info[APERTA2_MAX_PORT];
} aperta2_pm_info_t;

typedef struct aperta2_sys_port_config_s {
    plp_aperta2_phymod_phy_inf_config_t sys_port_config;
    aperta2_device_aux_modes_t sys_aux_mode;
    unsigned int lane_mask;
} aperta2_sys_port_config_t;

typedef enum APERTA2_PM_SELECT_E {
    APERTA2_SYS_OCTAL1 = 1,
    APERTA2_SYS_OCTAL2 = 2,
    APERTA2_LIN_OCTAL1 = 4,
    APERTA2_LIN_OCTAL2 = 8
} APERTA2_PM_SELECT_T;

typedef enum aperta2_pm_flow_control_e {
    aperta2FlowcontrolTerminateGenerate = 0,
    aperta2FlowcontrolPassthrough
} aperta2_pm_flow_control_t;

typedef enum aperta2_pm_fault_option_e {
    aperta2FaultoptionTerminateGenerate = 0,
    aperta2FaultoptionPassthrough
} aperta2_pm_fault_option_t;

typedef enum aperta2_pll_vco_e{
    aperta2Vco51G = 0,    /* 51G VCO*/
    aperta2Vco53G = 1     /* 53G VCO*/
}aperta2_pll_vco_t;

typedef struct aperta2_pll_vco_config_s {
    aperta2_pll_vco_t sys_vco;
    aperta2_pll_vco_t line_vco;
}aperta2_pll_vco_config_t;

/*struct for 1588 tx info*/
typedef struct aperta2_pm_ts_tx_info_s {
    uint32_t ts_in_fifo_lo; /**< low 32bit of Timestamp in Fifo */
    uint32_t ts_in_fifo_hi; /**< high 32bit of Timestamp in Fifo */
    uint32_t ts_seq_id; /**< sequence id of tx 1588 packet */
    uint32_t ts_sub_nanosec; /**< sub nanoseconds of tx 1588 packet */
} aperta2_pm_ts_tx_info_t;

#define APERTA2_GET_PORT_UPDATE_PM_INFO_LM(PHY, port)  \
    APERTA2_GET_PORT_FROM_LM(PHY->access.lane_mask, int_port);\
    port = APERTA2_PORT_CONSTRUCTION(PHY->access.addr, int_port);\
    APERTA2_UPDATE_PM_INFO(PHY->access.addr, PHY);

#define APERTA2_CALL_PM_API(P_A, _FUN)                                   \
    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);                       \
    PHYMOD_IF_ERR_RETURN(_FUN);

#define PM_8x100_GEN2_INFO(pm_info) ((pm_info)->pm_data.aperta2_pm8x100_gen2_db)
#define APERTA2_UPDATE_LM(PHY_ID, LM)             \
    {                                               \
        int cnt;                                    \
        for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) { \
            if (_plp_aperta2_pm_info[cnt].phy_id == PHY_ID) { \
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_core_access.access.lane_mask = LM; \
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_phy_access.access.lane_mask = LM; \
               break; \
            } \
    } }

#define APERTA2_UPDATE_PM_INFO(PHY_ID, PHY)             \
    {                                               \
        int cnt;                                    \
        for (cnt = 0; cnt <APERTA2_MAX_PM_INFO; cnt++) { \
            if (_plp_aperta2_pm_info[cnt].phy_id == PHY_ID) { \
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_core_access.access.lane_mask = PHY->access.lane_mask; \
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_phy_access.access.lane_mask = PHY->access.lane_mask; \
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_phy_access.access.tvco_pll_index = 0;\
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_core_access.access.tvco_pll_index = 0;\
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_core_access.port_loc = PHY->port_loc;\
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_phy_access.port_loc = PHY->port_loc;\
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_core_access.access.user_acc = PHY->access.user_acc;\
               PM_8x100_GEN2_INFO(_plp_aperta2_pm_info[cnt].pm_info)->int_phy_access.access.user_acc = PHY->access.user_acc;\
               break; \
            } \
    } }

#define APERTA2_PORT_CONSTRUCTION(PHY_ID, PM_PORT)     (PHY_ID << 16) | (PM_PORT)
#define APERTA2_GET_PHYID_FROM_PM_PORT(PM_PORT)        ((PM_PORT >> 16) & 0xFFFF)

#define APERTA2_GET_PM_SEL(PHY, OCT_SEL, PM_SEL)    \
    if (PHY->port_loc == phymodPortLocDC ||  PHY->port_loc == phymodPortLocLine) { \
        if (OCT_SEL == APERTA2_PM_OCTAL1) {                                        \
            PM_SEL = APERTA2_LIN_OCTAL1;                                           \
        } else {                                                                   \
            PM_SEL = APERTA2_LIN_OCTAL2;                                           \
        }                                                                          \
    } else {                                                                       \
        if (OCT_SEL == APERTA2_PM_OCTAL1) {                                        \
            PM_SEL = APERTA2_SYS_OCTAL1;                                           \
        } else {                                                                   \
            PM_SEL = APERTA2_SYS_OCTAL2;                                           \
        }                                                                          \
    }

#define APERTA2_GET_PLL_MICRO_SEL(PHY, PLL_SEL, MICRO_SEL)   \
    PLL_SEL = 0;                                             \
    MICRO_SEL = plp_aperta2_peregrine5_pc_get_micro_idx((srds_access_t*)PHY);

#define APERTA2_GET_OCTAL(LM)   ((LM & 0xFF) ? (APERTA2_PM_OCTAL1) : (APERTA2_PM_OCTAL2))

#define APERTA2_GET_PORT_SPEED_LDR(phy, PSP, LDR)  {                            \
    int lane_map = 0, port_number = 0;                                          \
    if (plp_aperta2_phymod_count_set_bits(phy->access.lane_mask) == 0) {                    \
        PHYMOD_DEBUG_ERROR(("LaneMap cannot be 0\n"));                          \
        return PHYMOD_E_PARAM; }                                                \
    PHYMOD_IF_ERR_RETURN(                                                       \
        plp_aperta2_pm_info_port_speed_get(phy, port_number, &PSP, &lane_map));     \
    if (lane_map == 0x0) {                                                      \
       /* PHYMOD_DIAG_OUT(("Lanemap cannot be 0:%x--%x\n",reg_addr, phy->access.lane_mask)); */                            \
        PSP = 100000;  /*Set default speed and lane map*/                                     \
        lane_map = phy->access.lane_mask;\
    }                                                                           \
    LDR = (int)PSP/plp_aperta2_phymod_count_set_bits(lane_map);      }

#define APERTA2_GET_PORT_FROM_LM_SP(SP,LD,LM,PRT,LN_SEL,OCT)                              \
    OCT = APERTA2_GET_OCTAL(LM);                                                      \
    if (SP == APERTA2_SPEED_800G ||                                                       \
        ((SP == APERTA2_SPEED_400G) && (LD == APERTA2_LD_50G || LD == APERTA2_LD_ROFF_50G))) {          \
        OCT = APERTA2_GET_OCTAL(LM);                                                      \
        LN_SEL = (OCT == APERTA2_PM_OCTAL1) ? LM : (LM >> 8);                             \
        if (LM == 0x0 || LM == 0xFF) {                                                    \
            PRT=0;                                                            \
        } else if (LM == 0xFF00) {                                                        \
            PRT=8;                                                            \
        } else {                                                                          \
            PRT=0;                                                                        \
        }                                                                                 \
    } else if ((SP == APERTA2_SPEED_10G || SP == APERTA2_SPEED_25G) ||                    \
               ((SP == APERTA2_SPEED_50G) && (LD == APERTA2_LD_50G || LD == APERTA2_LD_ROFF_50G)) ||    \
               ((SP == APERTA2_SPEED_100G) && (LD == APERTA2_LD_100G || LD == APERTA2_LD_ROFF_100G))) {  \
        unsigned int temp_lm = 0;                                                         \
        OCT = APERTA2_GET_OCTAL(LM);                                                      \
        temp_lm = (OCT == APERTA2_PM_OCTAL1) ? LM : (LM >> 8);                            \
        if (temp_lm == 0x0 || temp_lm == 0x1) {                                           \
            LN_SEL = 1; PRT = 0;                                                          \
        } else if (temp_lm == 0x2) {                                                      \
            LN_SEL = temp_lm; PRT = 1;                                                    \
        } else if (temp_lm == 0x4) {                                                      \
            LN_SEL = temp_lm; PRT = 2;                                                    \
        } else if (temp_lm == 0x8) {                                                      \
            LN_SEL = temp_lm; PRT = 3;                                                    \
        } else if (temp_lm == 0x10) {                                                     \
            LN_SEL = temp_lm; PRT = 4;                                                    \
        } else if (temp_lm == 0x20) {                                                     \
            LN_SEL = temp_lm; PRT = 5;                                                    \
        } else if (temp_lm == 0x40) {                                                     \
            LN_SEL = temp_lm; PRT = 6;                                                    \
        } else if (temp_lm == 0x80) {                                                     \
            LN_SEL = temp_lm; PRT = 7;                                                    \
        } else {                                                                          \
            LN_SEL = 1; PRT = 0;                                                          \
        }                                                                                 \
        PRT = (OCT == APERTA2_PM_OCTAL1) ? PRT : (PRT+8);                                 \
    } else if (((SP == APERTA2_SPEED_50G) && (LD == APERTA2_LD_25G || LD == APERTA2_LD_ROFF_25G ))  ||  \
               ((SP == APERTA2_SPEED_100G) && (LD == APERTA2_LD_50G || LD == APERTA2_LD_ROFF_50G)) ||   \
               ((SP == APERTA2_SPEED_200G) && (LD == APERTA2_LD_100G || LD == APERTA2_LD_ROFF_100G))) { \
        unsigned int temp_lm = 0;                                                         \
        OCT = APERTA2_GET_OCTAL(LM);                                                      \
        temp_lm = (OCT == APERTA2_PM_OCTAL1) ? LM : (LM >> 8);                            \
        if (temp_lm == 0x0 || temp_lm & 0x3) {                                            \
            LN_SEL = (temp_lm==0) ? 0x3 : temp_lm; PRT = 0;                               \
        } else if (temp_lm & 0xC) {                                                       \
            LN_SEL = temp_lm; PRT = 2;                                                    \
        } else if (temp_lm & 0x30) {                                                      \
            LN_SEL = temp_lm; PRT = 4;                                                    \
        } else if (temp_lm & 0xC0) {                                                      \
            LN_SEL = temp_lm; PRT = 6;                                                    \
        } else {                                                                          \
            LN_SEL = 1; PRT = 0;                                                          \
        }                                                                                 \
        PRT = (OCT == APERTA2_PM_OCTAL1) ? PRT : (PRT+8);                                 \
    } else if (((SP == APERTA2_SPEED_400G) && (LD == APERTA2_LD_100G || LD == APERTA2_LD_ROFF_100G )) ||\
               ((SP == APERTA2_SPEED_200G) && (LD == APERTA2_LD_50G || LD == APERTA2_LD_ROFF_50G ))  || \
               ((SP == APERTA2_SPEED_100G) && (LD == APERTA2_LD_25G || LD == APERTA2_LD_ROFF_25G))) {  \
        unsigned int temp_lm = 0;                                                         \
        OCT = APERTA2_GET_OCTAL(LM);                                                      \
        temp_lm = (OCT == APERTA2_PM_OCTAL1) ? LM : (LM >> 8);                            \
        if (temp_lm == 0x0 || temp_lm & 0xF) {                                            \
            LN_SEL = (temp_lm == 0) ? 0xF : temp_lm; PRT = 0;                             \
        } else if (temp_lm & 0xF0) {                                                      \
            LN_SEL = temp_lm; PRT = 4;                                                    \
        } else {                                                                          \
            LN_SEL = 1; PRT = 0;                                                          \
        }                                                                                 \
        PRT = (OCT == APERTA2_PM_OCTAL1) ? PRT : (PRT+8);                                 \
    }

#define APERTA2_GET_PORT_FROM_LM(LM,PRT)  PRT = plp_aperta2_phymod_log2n(LM & (-LM))

#define APERTA2_PORT_SPEED(SP,LDR,PRT_SPD)           \
    switch (SP) {                                    \
        case APERTA2_SPEED_800G: {                   \
            PRT_SPD = 0xB; break; }                  \
        case APERTA2_SPEED_400G: {                   \
            if (LDR >= APERTA2_LD_100G) {            \
                PRT_SPD = 0xA; break;                \
            } else {                                 \
                PRT_SPD = 0x9; break;                \
            }}                                       \
        case APERTA2_SPEED_200G: {                   \
            if (LDR >= APERTA2_LD_100G) {            \
                PRT_SPD = 0x8; break;                \
            } else {                                 \
                PRT_SPD = 0x7; break;                \
            }}                                       \
        case APERTA2_SPEED_100G: {                   \
            if (LDR >= APERTA2_LD_100G) {            \
                PRT_SPD = 0x6; break;                \
            } else if (LDR >= APERTA2_LD_50G){       \
                PRT_SPD = 0x5; break;                \
            }  else if (LDR >= APERTA2_LD_25G){      \
                PRT_SPD = 0x4; break;                \
            }}                                       \
        case APERTA2_SPEED_50G: {                    \
            if (LDR >= APERTA2_LD_50G) {             \
                PRT_SPD = 0x3; break;                \
            } else {                                 \
                PRT_SPD = 0x2; break;                \
            }}                                       \
        case APERTA2_SPEED_25G: {                    \
            PRT_SPD = 0x1; break;}                   \
        case APERTA2_SPEED_10G: {                    \
            PRT_SPD = 0x1; break;}                   \
        default :                                    \
            PRT_SPD = 0xB;                           \
    }

#define APERTA2_GET_LM_FROM_PORT(SP,LD,PRT,LM)                               \
    if (SP == APERTA2_SPEED_800G ||                                          \
        ((SP == APERTA2_SPEED_400G) && (LD == APERTA2_LD_50G))) {            \
        if (PRT == 0x0) {                                                    \
            LM = 0xFF;                                                       \
        }                                                                    \
        if (PRT == 0x8) {                                                    \
            LM = 0xFF00;                                                     \
        }                                                                    \
    } else if ((SP == APERTA2_SPEED_10G || SP == APERTA2_SPEED_25G) ||       \
               ((SP == APERTA2_SPEED_50G) && (LD == APERTA2_LD_50G)) ||      \
               ((SP == APERTA2_SPEED_100G) && (LD == APERTA2_LD_100G))) {    \
        unsigned int oct = (PRT > 7) ? 1 : 0;                                \
        unsigned int port = (oct ? (PRT-8) :PRT);                            \
        if (port == 0) {                                                     \
            LM=0x1;                                                          \
        } else if (port == 1) {                                              \
            LM = 0x2;                                                        \
        } else if (port == 2) {                                              \
            LM = 0x4;                                                        \
        } else if (port == 3) {                                              \
            LM = 0x8;                                                        \
        } else if (port == 4) {                                              \
            LM = 0x10;                                                       \
        } else if (port == 5) {                                              \
            LM = 0x20;                                                       \
        } else if (port == 6) {                                              \
            LM = 0x40;                                                       \
        } else if (port == 7) {                                              \
            LM = 0x80;                                                       \
        }                                                                    \
        LM = (oct) ? (LM << 8) : (LM);                                       \
    } else if (((SP == APERTA2_SPEED_50G) && (LD == APERTA2_LD_25G))  ||     \
               ((SP == APERTA2_SPEED_100G) && (LD == APERTA2_LD_50G)) ||     \
               ((SP == APERTA2_SPEED_200G) && (LD == APERTA2_LD_100G))) {    \
        unsigned int oct = (PRT > 7) ? 1 : 0;                                \
        unsigned int port = (oct ? (PRT-8) :PRT);                            \
        if (port == 0 ) {                                                    \
            LM = 0x3;                                                        \
        } else if (port == 0x2) {                                            \
            LM = 0xC;                                                        \
        } else if (port == 0x4) {                                            \
            LM = 0x30;                                                       \
        } else if (port == 0x6) {                                            \
            LM = 0xc0;                                                       \
        }                                                                    \
        LM = (oct) ? (LM << 8) : (LM);                                       \
    } else if (((SP == APERTA2_SPEED_400G) && (LD == APERTA2_LD_100G)) ||    \
               ((SP == APERTA2_SPEED_200G) && (LD == APERTA2_LD_50G))  ||    \
               ((SP == APERTA2_SPEED_100G) && (LD == APERTA2_LD_25G))) {     \
        unsigned int oct = (PRT > 7) ? 1 : 0;                                \
        unsigned int port = (oct ? (PRT-8) :PRT);                            \
        if (port == 0x0) {                                                   \
            LM = 0xF;                                                        \
        } else if (port == 0x4) {                                            \
            LM = 0xF0;                                                       \
        }                                                                    \
        LM = (oct) ? (LM << 8) : (LM);                                       \
    }


#define APERTA2_LM_TO_POSSIBLE_PORT_LIST(lm, list, max_port) \
    max_port = 0; list = 0;                                  \
    for (cnt = 0; cnt < APERTA2_MAX_PORT; cnt++) {           \
        if (lm & (1 << cnt)) {                               \
            list |= (cnt << (max_port*4));                   \
            max_port ++;                                     \
        } }

#define APERTA2_DISABLE_CURRENT_LANE_MAP_PORT(LANEMAP, LOC)                              \
{                                                                                        \
    int prev_speed = 0, prev_lanemap = 0, port = 0;                                      \
    unsigned int temp_lane_map = LANEMAP, retry = 0;                                     \
    max_ports = 1;                                                                       \
    PHYMOD_MEMCPY(temp_acc, phy, sizeof(plp_aperta2_phymod_phy_access_t));                           \
    temp_acc->port_loc = LOC;                                                            \
    while (temp_lane_map) {                                                              \
        temp_acc->access.lane_mask = temp_lane_map;                                      \
        /* Getting Associated ports*/                                                    \
        PHYMOD_IF_ERR_RETURN(                                                            \
            plp_aperta2_pm_info_lane_speed_get(temp_acc, &prev_speed, &prev_lanemap, &port));\
        if ((port == 0xFF) || (retry > APERTA2_MAX_PORT)) {                             \
            break;                                                                       \
        }                                                                                \
        PHYMOD_IF_ERR_RETURN(                                                            \
            BCM_APERTA2_DIRECT_READ_CTRL_SWGPREG0Fr(&phy->access, &disable_port_phase)); \
        COMPILER_64_SET(possible_port_list, 0, port);                                    \
        phy_acc.access.lane_mask = prev_lanemap;                                         \
        APERTA2_DISABLE_PORT(temp_acc);                                                  \
        /* Clean current map based on lane that is disabled*/                            \
        temp_lane_map &= ~phy_acc.access.lane_mask;                                      \
        PHYMOD_IF_ERR_RETURN(                                                            \
            BCM_APERTA2_DIRECT_WRITE_CTRL_SWGPREG0Fr(&phy->access, disable_port_phase)); \
        retry ++;                                                                        \
    }                                                                                    \
}

#define APERTA2_SIMPLE_DISABLE_PORT(P_N)             \
    port = APERTA2_PORT_DISABLE_PH1 | P_N;           \
    PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(&temp_access, 0, NULL, \
                 		       APERTA2_MSG_DISABLE_PORT, &port, 0)); \
    port = APERTA2_PORT_DISABLE_PH2 | P_N;           \
    PHYMOD_IF_ERR_RETURN( plp_aperta2_send_fw_msg(&temp_access, 0, NULL, \
       			       APERTA2_MSG_DISABLE_PORT, &port, 0)); \
    PHYMOD_IF_ERR_RETURN(                                     \
        plp_aperta2_port_active_reset(&temp_access, P_N));

#define APERTA2_OCT_CROSS_CLEAR_USED_LM(P_A)                                                     \
    {                                                                                            \
        int oc_port = 0;                                                                         \
        /* Getting lanes that are associated to other ports */                                   \
        PHYMOD_IF_ERR_RETURN(                                                                    \
            plp_aperta2_pm_info_lane_speed_get(&P_A, &prev_speed, &prev_lanemap, &oc_port));         \
        if (oc_port != 0xFF) {                                                                   \
            P_A.access.lane_mask &= ~(prev_lanemap);                                             \
        }                                                                                        \
        (void)oc_port;                                                                           \
    }


/* If port in allocated State, perform Phase1 Disable and mark the 
 * GPREG as P1 Complete, If Phase1 is completed perform P2 Disable and mark the 
 * Port as deallocated*/
#define APERTA2_DISABLE_PORT(PHY) \
for (port_index = 0; port_index < max_ports; port_index++) {\
            temp = (possible_port_list >> port_index*4) & 0xF;\
            if (enabled_port_list[temp]) {\
                phase &= ~(0xF); \
                phase |= temp &0xF;\
                if ((!(disable_port_phase.v[0] & (1 << (temp&0xF)))) &&\
                        (phase & APERTA2_PORT_DISABLE_PH1)) { \
                    PHYMOD_CRIT_INFO(("##Previous ena-dis port:%x## PH1\n", phase));\
                    PHYMOD_IF_ERR_RETURN(\
                            plp_aperta2_send_fw_msg(PHY, 0, NULL,\
                                APERTA2_MSG_DISABLE_PORT, &phase, 0));\
                    disable_port_phase.v[0] &= ~(1 << (temp&0xF));\
                    disable_port_phase.v[0] |= ( 1 << (temp&0xF));\
                } else if ((disable_port_phase.v[0] & (1 << (temp & 0xF))) && \
                        (phase & APERTA2_PORT_DISABLE_PH2)) { \
                    PHYMOD_CRIT_INFO(("##Previous ena-dis port:%x## PH2\n", phase));\
                    PHYMOD_IF_ERR_RETURN(\
                        plp_aperta2_send_fw_msg(PHY, 0, NULL,\
                           APERTA2_MSG_DISABLE_PORT, &phase, 0));\
                    PHYMOD_IF_ERR_RETURN(\
                        plp_aperta2_port_active_reset(PHY, temp));\
                    disable_port_phase.v[0] &= ~(1 << (temp&0xF));\
                } else { /*Nothing Needed*/ \
                }\
            }\
        }


#define APERTA2_PORT_CTRL_REG     {BCM_APERTA2_DIRECT_CTRL_PORT0_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT1_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT2_CONFIGr,\
         BCM_APERTA2_DIRECT_CTRL_PORT3_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT4_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT5_CONFIGr,\
         BCM_APERTA2_DIRECT_CTRL_PORT6_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT7_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT8_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT9_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT10_CONFIGr,\
         BCM_APERTA2_DIRECT_CTRL_PORT11_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT12_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT13_CONFIGr,\
         BCM_APERTA2_DIRECT_CTRL_PORT14_CONFIGr, BCM_APERTA2_DIRECT_CTRL_PORT15_CONFIGr}

#define APERTA2_PORT_STS_REG     {BCM_APERTA2_DIRECT_CTRL_PORT0_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT1_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT2_STATUSr,\
         BCM_APERTA2_DIRECT_CTRL_PORT3_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT4_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT5_STATUSr,\
         BCM_APERTA2_DIRECT_CTRL_PORT6_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT7_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT8_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT9_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT10_STATUSr,\
         BCM_APERTA2_DIRECT_CTRL_PORT11_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT12_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT13_STATUSr,\
         BCM_APERTA2_DIRECT_CTRL_PORT14_STATUSr, BCM_APERTA2_DIRECT_CTRL_PORT15_STATUSr}

#define APERTA2_TS_PORT_RESUME(ret,pre_cmd,port)                                                                   \
    if (ret != PHYMOD_E_NONE) {                                                                                    \
        PHYMOD_DEBUG_ERROR((pre_cmd "port failed :%d. Resuming port:%d\n", ret, port));                            \
        PHYMOD_IF_ERR_RETURN(                                                                                      \
            plp_aperta2_send_fw_msg(&fw_access, port_config.data_rate , &auxmode, APERTA2_MSG_RESUME_PORT, NULL, 0));  \
        return ret;}

/* This define is used to log the PCS debug register dump into a file */
#define APERTA2_REG_DUMP_INTO_FILE

#ifdef APERTA2_REG_DUMP_INTO_FILE
    #define APERTA2_PCS_INFO_PRINT(fmt, args...) \
    do {                                        \
     fprintf(fp,fmt,##args);                    \
     phymod_log_formatted_message(fmt, ##args); \
    }while(0)
#else
    #define APERTA2_PCS_INFO_PRINT(fmt, args...) phymod_log_formatted_message(fmt, ##args)
#endif


/* Possible Port list*/
#define APERTA2_PT                           0
#define APERTA2_GB                           1
#define APERTA2_RGB                          2

#define APERTA2_MAX_GB_RGB_PORT              36
#define APERTA2_MAX_GB_RGB_PORT_ENTITY       3
#define APERTA2_GB_RGB_PORT_TYPE             0
#define APERTA2_GB_RGB_PORT_SYS_LANE         1
#define APERTA2_GB_RGB_PORT_LINE_LANE        2

#define APERTA2_GET_PORTMODE_FEC(FEC, PM_FEC)           \
    if (FEC == bcmAperta2NoFEC) {                       \
        PM_FEC = PORTMOD_PORT_PHY_FEC_NONE;             \
    } else if(FEC == bcmAperta2RS544) {                 \
       PM_FEC = PORTMOD_PORT_PHY_FEC_RS_544;            \
    } else if(FEC == bcmAperta2RS272) {                 \
       PM_FEC = PORTMOD_PORT_PHY_FEC_RS_272;            \
    } else if( FEC == bcmAperta2RSFEC) {                \
       PM_FEC = PORTMOD_PORT_PHY_FEC_RS_FEC;            \
    } else if(FEC == bcmAperta2RS544_2XN) {             \
       PM_FEC = PORTMOD_PORT_PHY_FEC_RS_544_2XN;        \
    } else if(FEC == bcmAperta2RS272_2XN) {             \
       PM_FEC = PORTMOD_PORT_PHY_FEC_RS_272_2XN;        \
    } else {                                            \
       PM_FEC = PORTMOD_PORT_PHY_FEC_NONE;              \
    }

#define APERTA2_COMMON_REGISTER_START_LIST   {                                  \
                                                0x01008B00, /* CHIP ID       */ \
                                                0x01008215, /* CHIP REV      */ \
                                                0x0100900a, /* LMI STS       */ \
                                                0x01009025, /* LMI DED       */ \
                                                0x01008140, /* CHIP PLL      */ \
                                                0x01008116, /* RESCAL SYS    */ \
                                                0x0100811E, /* RESCAL LINE   */ \
                                                0x01008B82, /* GEN INT       */ \
                                                0x01008B93, /* PORT INT      */ \
                                                0x01008256, /* FW PRT INUSE  */ \
                                                0x010082F0, /* FW BOOT       */ \
                                                0x0100A000, /* PORT REG      */ \
                                                0x0100A300, /* PBIST REG     */ \
                                                0x0100A3E0, /* LMI ERR       */ \
                                            }

#define APERTA2_COMMON_REGISTER_END_LIST    {                                   \
                                                0x01008B01, /* CHIP ID       */ \
                                                0x01008217, /* CHIP REV      */ \
                                                0x0100900a, /* LMI STS       */ \
                                                0x01009025, /* LMI DED       */ \
                                                0x01008140, /* CHIP PLL      */ \
                                                0x01008116, /* RESCAL SYS    */ \
                                                0x0100811E, /* RESCAL LINE   */ \
                                                0x01008B8E, /* GEN INT       */ \
                                                0x01008BC0, /* PORT INT      */ \
                                                0x01008256, /* FW PRT INUSE  */ \
                                                0x010082FF, /* FW BOOT       */ \
                                                0x0100A1FF, /* PORT REG      */ \
                                                0x0100A30F, /* PBIST REG     */ \
                                                0x0100A3FF, /* LMI ERR       */ \
                                            }
#define APERTA2_MAX_CMN_REG                 14

#define APERTA2_PM_IF_ERR_RETURN(A) \
    do {   \
        int loc_err ; \
        if ((loc_err = (A)) != PHYMOD_E_NONE) \
        {  goto APERTA2_ERR ; } \
    } while (0)

typedef enum APERTA2_SW_SIDE_E {
    APERTA2_SW_LINE_SIDE = 0,
    APERTA2_SW_SYS_SIDE  = 1,
    APERTA2_SW_BOTH_SIDE = 2,
    APERTA2_SW_NO_SIDE   = 3,
    APERTA2_SW_MAX_COUNT = 4
}APERTA2_SW_SIDE_T;

#define APERTA2_PORT_NONE  (-2) /* Port selection is don't care */

/*Warmboot Macros*/
#define APERTA2_ISCOREINITIALIZED_SHIFT    0
#define APERTA2_ISACTIVE_SHIFT             1
#define APERTA2_SPEEDIDTABLESTATUS_SHIFT   2
#define APERTA2_FWDLOAD_SHIFT              4
#define APERTA2_FWINITSTATE_SHIFT          6
#define APERTA2_LINE_FO_SHIFT              8
#define APERTA2_LINE_SPD_SHIFT             8

#define APERTA2_ISCOREINITIALIZED          0 
#define APERTA2_ISACTIVE                   1
#define APERTA2_TVCOPLLACTIVELANEBITMAP    2
#define APERTA2_TVCOPLLADVLANEBITMAP       3
#define APERTA2_LANE2PORTMAP               4
#define APERTA2_ANMODE                     5
#define APERTA2_LANE2FECMAP                6
#define APERTA2_TSENABLEPORTCOUNT          7
#define APERTA2_TIMESYNCENABLE             8
#define APERTA2_PORTISPCSBYPASSED          9
#define APERTA2_SPEEDIDTABLESTATUS         10
#define APERTA2_FWDLOAD                    11
#define APERTA2_PORT_SYS_SPEED             12
#define APERTA2_PORT_LINE_SPEED            13
#define APERTA2_PORT_SYS_LM                14
#define APERTA2_PORT_LINE_LM               15
#define APERTA2_PORT_SYS_FO                16
#define APERTA2_PORT_LINE_FO               17
#define APERTA2_FW_INIT_STATE              18
/*!bcm_plp_aperta2_fw_init_params_t
  * This structure is intended to initialize HW configuration after FW download
  * \arg macsec_option\n
  * When bit0 is set to 1, macsec Ingress side on Octal1 is bypassed.
  * When bit1 is set to 1, macsec Egress side on Octal1 is bypassed.
  * When bit2 is set to 1, macsec Ingress side on Octal2 is bypassed.
  * When bit3 is set to 1, macsec Egress side on Octal2 is bypassed.
  * \arg ptp_option\n
  * When bit0 is set to 1, PTP on Octal1 is bypassed.
  * When bit1 is set to 1, PTP on Octal2 is bypassed.
  * When bit2 is set to 1, PTP configured on Sync mode
  * When bit3 is set to 1, PTP external reference clock invert enable
  * 
  */
typedef struct aperta2_fw_init_s {
    unsigned int  macsec_option;
    unsigned int  ptp_option;
    aperta2_pll_vco_config_t octal0;
    aperta2_pll_vco_config_t octal1;
    plp_aperta2_phymod_lane_map_t   sys_lane_map;
    plp_aperta2_phymod_lane_map_t   line_lane_map;
} aperta2_fw_init_t;


void plp_aperta2_plp_aperta2_update_pm_info(int phy_id, uint32_t wb_idx, int val);
void plp_aperta2_plp_aperta2_get_wb_pm_info(int phy_id, uint32_t wb_idx, int *val);
int plp_aperta2_portmod_pm_info_get(int unit, int port, pm_info_t* pm_info);
int plp_aperta2_send_fw_msg(const plp_aperta2_phymod_phy_access_t *phy, 
                       int data_rate, aperta2_device_aux_modes_t *aux,
                       int msg, void *data, int read);

int plp_aperta2_pm_is_fw_dloaded_get(int phy_id, uint32_t *active);
int plp_aperta2_pm_is_fw_dloaded_set(int phy_id, uint32_t active);
int plp_aperta2_pm_init(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_core_init_config_t* init_config, const plp_aperta2_phymod_core_status_t* core_status);
int plp_aperta2_portmod_pm_info_get(int unit, int port, pm_info_t* pm_info);
int plp_aperta2_portmod_port_pm_type_get(int unit, int port, portmod_dispatch_type_t* type);
int plp_aperta2_core_add(const plp_aperta2_phymod_core_access_t* core, const plp_aperta2_phymod_phy_init_config_t* init_config,
                    const plp_aperta2_phymod_core_status_t* core_status, uint8_t octal_start, uint8_t octal_end);

int plp_aperta2_port_speed_set(const plp_aperta2_phymod_phy_access_t* phy, int data_rate, int lane_data_rate);
int plp_aperta2_tsc_clock_sel(const plp_aperta2_phymod_phy_access_t* phy,  aperta2_switch_dpclk_t switch_dp_clk);
int plp_aperta2_pm_store_and_forward_mode_set(const plp_aperta2_phymod_phy_access_t* phy, int enable);
int plp_aperta2_pm_flow_control_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_flow_control_t *flow_control);
int plp_aperta2_pm_flow_control_set(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_flow_control_t flow_control);
int plp_aperta2_pm_fault_option_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_fault_option_t *fault_option);
int plp_aperta2_pm_fault_option_set(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_fault_option_t fault_option);
int plp_aperta2_port_speed_set(const plp_aperta2_phymod_phy_access_t* phy, int data_rate, int lane_data_rate);
int plp_aperta2_port_active_set(const plp_aperta2_phymod_phy_access_t* phy, int data_rate, int lane_data_rate);
int plp_aperta2_port_active_reset(const plp_aperta2_phymod_phy_access_t* phy, int port);
int plp_aperta2_pm_fault_option_set(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_fault_option_t fault_option);
int plp_aperta2_pm_fault_option_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_fault_option_t *fault_option);
int plp_aperta2_pm_flow_control_set(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_flow_control_t flow_option);
int plp_aperta2_pm_flow_control_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_flow_control_t *flow_option);
int plp_aperta2_pm_store_and_forward_mode_set(const plp_aperta2_phymod_phy_access_t* phy, int enable);
int plp_aperta2_pm_store_and_forward_mode_get(const plp_aperta2_phymod_phy_access_t* phy, int *is_enable);
int plp_aperta2_pm_max_pkt_size_set(const plp_aperta2_phymod_phy_access_t* phy, int size);
int plp_aperta2_pm_max_pkt_size_get(const plp_aperta2_phymod_phy_access_t* phy, int *size);
int plp_aperta2_pm_runt_threshold_set(const plp_aperta2_phymod_phy_access_t* phy, int threshold);
int plp_aperta2_pm_runt_threshold_get(const plp_aperta2_phymod_phy_access_t* phy, int *threshold);
int plp_aperta2_pm_pad_size_set(const plp_aperta2_phymod_phy_access_t* phy, int pad_size);
int plp_aperta2_pm_pad_size_get(const plp_aperta2_phymod_phy_access_t* phy, int *pad_size);
int plp_aperta2_pm_tx_mac_sa_set(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta2_pm_tx_mac_sa_get(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta2_pm_rx_mac_sa_set(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta2_pm_rx_mac_sa_get(const plp_aperta2_phymod_phy_access_t* phy, unsigned char mac_sa[6]);
int plp_aperta2_pm_tx_avg_ipg_set(const plp_aperta2_phymod_phy_access_t* phy, int avg_ipg);
int plp_aperta2_pm_tx_avg_ipg_get(const plp_aperta2_phymod_phy_access_t* phy, int *avg_ipg);
int plp_aperta2_pm_tx_preamble_length_set(const plp_aperta2_phymod_phy_access_t* phy, int preamble_length);
int plp_aperta2_pm_tx_preamble_length_get(const plp_aperta2_phymod_phy_access_t* phy, int *preamble_length);
int plp_aperta2_pm_pause_control_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control);
int plp_aperta2_pm_pause_control_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_pause_control_t *pause_control);
int plp_aperta2_pm_pfc_control_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl);
int plp_aperta2_pm_pfc_control_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_control_t *pfc_ctrl);
int plp_aperta2_pm_llfc_control_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl);
int plp_aperta2_pm_llfc_control_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_llfc_control_t *llfc_ctrl);
int plp_aperta2_pm_pfc_config_set(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc);
int plp_aperta2_pm_pfc_config_get(const plp_aperta2_phymod_phy_access_t* phy, portmod_pfc_config_t *pfc);
int plp_aperta2_rx_mac_enable_set(const plp_aperta2_phymod_phy_access_t* phy, int enable);
int plp_aperta2_rx_mac_enable_get(const plp_aperta2_phymod_phy_access_t* phy, int *enable);
int plp_aperta2_tx_mac_enable_set(const plp_aperta2_phymod_phy_access_t* phy, int enable);
int plp_aperta2_tx_mac_enable_get(const plp_aperta2_phymod_phy_access_t* phy, int *enable);

int plp_aperta2_pm_interface_config_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, const plp_aperta2_phymod_phy_inf_config_t* config, uint32_t line_lane_map, const plp_aperta2_phymod_phy_inf_config_t *line_config);
int plp_aperta2_pm_interface_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_phy_inf_config_t* config);
int plp_aperta2_pm_info_port_speed_set(const plp_aperta2_phymod_phy_access_t *phy, int speed, int port_number, int fo_lane_map, aperta2_device_aux_modes_t *aux_mode);
int plp_aperta2_pm_tx_rx_enable_post(const plp_aperta2_phymod_phy_access_t* phy, int enable, int single_port, uint16_t fo_side_lm, uint32_t line_mask);
int plp_aperta2_pm_tx_rx_enable(const plp_aperta2_phymod_phy_access_t* phy, int enable, int single_port, int failover);
int plp_aperta2_disable_port(const plp_aperta2_phymod_phy_access_t *phy, const plp_aperta2_phymod_phy_inf_config_t* config, int prev_port, int phase, uint32_t line_lane_map, const plp_aperta2_phymod_phy_inf_config_t *line_config);
int plp_aperta2_flush_port(const plp_aperta2_phymod_phy_access_t *phy);
int plp_aperta2_pcs_info_dump(const plp_aperta2_phymod_phy_access_t *phy);
int plp_aperta2_pm_diagnostic_dump(const plp_aperta2_phymod_phy_access_t *phy);
int plp_aperta2_pm_mac_mib_stat_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t stat_type, plp_uint64_t *count);
int plp_aperta2_pm_is_fo_enabled(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_phy_inf_config_t* config,
                            unsigned int *is_fo_enabled, unsigned int *pri_port, 
                            unsigned int *primary_lm);
int plp_aperta2_remove_pm_info(int phy_id);
int plp_aperta2_pm_interface_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_phy_inf_config_t* config);

int plp_aperta2_set_init_state(int phy_id, int val);
int plp_aperta2_get_init_state(int phy_id, int *val);
int plp_aperta2_pm_info_port_speed_get(const plp_aperta2_phymod_phy_access_t *phy, int port_number, int *speed, int *lane_map);
int plp_aperta2_lane_swap_set(const plp_aperta2_phymod_phy_access_t *phy, const plp_aperta2_phymod_lane_map_t *sys_lane_map, const plp_aperta2_phymod_lane_map_t *line_lane_map);
int _plp_aperta2_phy_autoneg_remote_ability_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_autoneg_ability_t* an_ability_get_type);
int plp_aperta2_pm_synce_config_set(const plp_aperta2_phymod_phy_access_t* phy, const phymod_synce_cfg_t* synce_cfg);
int plp_aperta2_pm_synce_config_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_synce_cfg_t* synce_cfg);
int plp_aperta2_pm_info_port_lane_map_get(const plp_aperta2_phymod_phy_access_t *phy, int port_number, uint32_t *lane_map);

int plp_aperta2_pm_timesync_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable);
int plp_aperta2_pm_timesync_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable);
int plp_aperta2_pm_timesync_tx_info_get(const plp_aperta2_phymod_phy_access_t* phy, aperta2_pm_ts_tx_info_t* ts_tx_info) ;
int plp_aperta2_pm_info_lane_speed_get(const plp_aperta2_phymod_phy_access_t *phy, int *speed, int *configured_lane, int *port_num);
int plp_aperta2_tx_rx_status(const plp_aperta2_phymod_phy_access_t* phy, int *data);
int plp_aperta2_get_octal_crossing_port(const plp_aperta2_phymod_phy_access_t* phy, aperta2_device_aux_modes_t *sys_aux_mode, unsigned int line_lane_map, int *port, uint8_t *max_port);

int plp_aperta2_configure_octal(const plp_aperta2_phymod_phy_access_t* phy, uint8_t octal_start, uint8_t octal_end,
                            const plp_aperta2_phymod_phy_init_config_t *init_config);
int plp_aperta2_reconfigure_octal_pll(const plp_aperta2_phymod_phy_access_t* phy, portmod_speed_config_t *speed_config, portmod_speed_config_t *line_speed_config);
int plp_aperta2_get_igr_egr_access(const plp_aperta2_phymod_phy_access_t* phy,  plp_aperta2_phymod_phy_access_t *igr_access,  plp_aperta2_phymod_phy_access_t *egr_access);
int plp_aperta2_disable_octal_crossed_port (const plp_aperta2_phymod_phy_access_t *phy, const plp_aperta2_phymod_phy_inf_config_t* config, 
                                        uint32_t line_lane_map, const plp_aperta2_phymod_phy_inf_config_t *line_config); 

int _plp_aperta2_phy_pam4_fec_status_get(const plp_aperta2_phymod_phy_access_t* phy, phymod_phy_fec_dump_status_t* fec_sts);
int _plp_aperta2_fo_get(const plp_aperta2_phymod_phy_access_t* phy, int port_number, plp_aperta2_phymod_failover_mode_t* failover_mode);
int plp_aperta2_pm_warmboot_init(const plp_aperta2_phymod_phy_access_t* phy);
int plp_aperta2_get_phyinfo_from_pminfo(int phy_id, plp_aperta2_phymod_phy_access_t *phy);
#endif /*__APERTA2_PM_SEQ_H__*/
