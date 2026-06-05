/*----------------------------------------------------------------------
 * $Id: aperta_tbhmod.h,
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *----------------------------------------------------------------------
 *  Description: define enumerators
 *----------------------------------------------------------------------*/
/*
 * $Id: $
 * $Copyright:
 * All Rights Reserved.$
 */

#ifndef _aperta_tbhmod_H_
#define _aperta_tbhmod_H_

#ifndef _DV_TB_
 #define _SDK_APERTA_TBHMOD_ 1
#endif

#ifdef _DV_TB_
#ifdef LINUX
#include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"
#endif

#ifdef _SDK_APERTA_TBHMOD_
#include <phymod/phymod.h>
#include <phymod/phymod_debug.h>
#include "aperta_tbhmod_enum_defines.h"
#endif

#ifdef _SDK_APERTA_TBHMOD_
#define PHYMOD_ST plp_aperta_phymod_phy_access_t
#else
#define PHYMOD_ST aperta_tbhmod_dv_st
#endif

#include "aperta_tbhmod_spd_ctrl.h"

#define APERTA_TSCBH_NOF_LANES_IN_CORE       8

#define APERTA_TBHMOD_FEC_NOT_SUPRTD         0
#define APERTA_TBHMOD_FEC_SUPRTD_NOT_REQSTD  1
#define APERTA_TBHMOD_FEC_CL74_SUPRTD_REQSTD 4
#define APERTA_TBHMOD_FEC_CL91_SUPRTD_REQSTD 8

#define APERTA_TBHMOD_CL73_ABILITY_50G_KR1_CR1_POSITION    13
#define APERTA_TBHMOD_CL73_ABILITY_100G_KR2_CR2_POSITION   14
#define APERTA_TBHMOD_CL73_ABILITY_200G_KR4_CR4_POSITION   15
#define TSCBH_FEC_OVERRIDE_BIT_SHIFT 0
#define TSCBH_FEC_OVERRIDE_MASK  0x1
#define TSCBH_PORT_AN_ENABLE_BIT_SHIFT 1
#define TSCBH_PORT_AN_ENABLE_MASK  0x2
#define TSCBH_PORT_ENABLE_BIT_SHIFT 2
#define TSCBH_PORT_ENABLE_MASK  0x4

#define APERTA_TBHMOD_VCO_NONE 0x0
#define APERTA_TBHMOD_VCO_20G 0x1
#define APERTA_TBHMOD_VCO_25G 0x2
#define APERTA_TBHMOD_VCO_26G 0x4
#define APERTA_TBHMOD_VCO_PCS_BYPASS 0x8
#define APERTA_TBHMOD_VCO_25P00G 0x10

/* So far 4 bits debug mask are used by APERTA_TBHMOD */
#define APERTA_TBHMOD_DBG_MEM        (1L << 4) /* allocation/object */
#define APERTA_TBHMOD_DBG_REGACC     (1L << 3) /* Print all register accesses */
#define APERTA_TBHMOD_DBG_FUNCVALOUT (1L << 2) /* All values returned by Tier1*/
#define APERTA_TBHMOD_DBG_FUNCVALIN  (1L << 1) /* All values pumped into Tier1*/
#define APERTA_TBHMOD_DBG_FUNC       (1L << 0) /* Every time we enter a  Tier1*/

typedef enum {
    APERTA_TBHMOD_AN_MODE_CL73 = 0,
    APERTA_TBHMOD_AN_MODE_CL73_BAM,
    APERTA_TBHMOD_AN_MODE_NONE,
    APERTA_TBHMOD_AN_MODE_CL73_MSA,
    APERTA_TBHMOD_AN_MODE_MSA,
    APERTA_TBHMOD_AN_MODE_TYPE_COUNT
}aperta_tbhmod_an_type_t;

typedef enum {
    APERTA_TBHMOD_NO_PAUSE = 0,
    APERTA_TBHMOD_SYMM_PAUSE,
    APERTA_TBHMOD_ASYM_PAUSE,
    APERTA_TBHMOD_ASYM_SYMM_PAUSE,
    APERTA_TBHMOD_AN_PAUSE_COUNT
}aperta_tbhmod_an_pause_t;

typedef enum {
  OVERRIDE_CLEAR                  =  0x0000,
  OVERRIDE_NUM_LANES              =  0x0001,
  OVERRIDE_OS_MODE                =  0x0002,
  OVERRIDE_T_FIFO_MODE            =  0x0004,
  OVERRIDE_T_ENC_MODE             =  0x0008,
  OVERRIDE_T_HG2_ENABLE           =  0x0010,
  OVERRIDE_T_PMA_BTMX_MODE_OEN    =  0x0020,
  OVERRIDE_SCR_MODE               =  0x0040,
  OVERRIDE_DESCR_MODE_OEN         =  0x0100,
  OVERRIDE_DEC_TL_MODE            =  0x0200,
  OVERRIDE_DESKEW_MODE            =  0x0400,
  OVERRIDE_DEC_FSM_MODE           =  0x0800,
  OVERRIDE_R_HG2_ENABLE           =  0x8001,
  OVERRIDE_BS_SM_SYNC_MODE_OEN    =  0x1000,
  OVERRIDE_BS_SYNC_EN_OEN         =  0x1001,
  OVERRIDE_BS_DIST_MODE_OEN       =  0x1002,
  OVERRIDE_BS_BTMX_MODE_OEN       =  0x1004,
  OVERRIDE_CL72_EN                =  0x1008,
  OVERRIDE_CLOCKCNT0              =  0x2000,
  OVERRIDE_CLOCKCNT1              =  0x2001,
  OVERRIDE_LOOPCNT0               =  0x2002,
  OVERRIDE_LOOPCNT1               =  0x2004,
  OVERRIDE_MAC_CREDITGENCNT       =  0x2008,
  OVERRIDE_T_CL91_CW_SCR          =  0x2009,
  OVERRIDE_T_5BIT_XOR_EN          =  0x200a,
  OVERRIDE_T_PMA_CL91_MUX_SEL     =  0x200b,
  OVERRIDE_T_PMA_WM               =  0x200c,
  OVERRIDE_T_PMA_BTMX_DELAY       =  0x200d,
  OVERRIDE_AM_SPACING_MUL         =  0x200e,
  OVERRIDE_CL91_BLKSYNC_MODE      =  0x200f,
  OVERRIDE_R_MERGE_MODE           =  0x3000,
  OVERRIDE_R_CL91_CW_SCR          =  0x3001,
  OVERRIDE_R_TC_IN_MODE           =  0x3002,
  OVERRIDE_R_TC_MODE              =  0x3003,
  OVERRIDE_R_5BIT_XOR_EN          =  0x3004,
  OVERRIDE_R_TC_OUT_MODE          =  0x3005,
  OVERRIDE_R_HG2_EN               =  0x3006,

  OVERRIDE_NUM_LANES_DIS          =  0x00f0,
  OVERRIDE_OS_MODE_DIS            =  0x00f1,
  OVERRIDE_T_FIFO_MODE_DIS        =  0x00f2,
  OVERRIDE_T_ENC_MODE_DIS         =  0x00f3,
  OVERRIDE_T_HG2_ENABLE_DIS       =  0x00f4,
  OVERRIDE_T_PMA_BTMX_MODE_OEN_DIS=  0x00f5,
  OVERRIDE_SCR_MODE_DIS           =  0x00f6,
  OVERRIDE_DESCR_MODE_OEN_DIS     =  0x00f7,
  OVERRIDE_DEC_TL_MODE_DIS        =  0x00f8,
  OVERRIDE_DESKEW_MODE_DIS        =  0x00f9,
  OVERRIDE_DEC_FSM_MODE_DIS       =  0x00fa,
  OVERRIDE_R_HG2_ENABLE_DIS       =  0x00fb,
  OVERRIDE_BS_SM_SYNC_MODE_OEN_DIS=  0x00fc,
  OVERRIDE_BS_SYNC_EN_OEN_DIS     =  0x00fd,
  OVERRIDE_BS_DIST_MODE_OEN_DIS   =  0x00fe,
  OVERRIDE_BS_BTMX_MODE_OEN_DIS   =  0x00e0,
  OVERRIDE_CL72_EN_DIS            =  0x00e1,
  OVERRIDE_CLOCKCNT0_DIS          =  0x00e2,
  OVERRIDE_CLOCKCNT1_DIS          =  0x00e3,
  OVERRIDE_LOOPCNT0_DIS           =  0x00e4,
  OVERRIDE_LOOPCNT1_DIS           =  0x00e5,
  OVERRIDE_MAC_CREDITGENCNT_DIS   =  0x00e6,
  OVERRIDE_T_CL91_CW_SCR_DIS      =  0x00e7,
  OVERRIDE_T_5BIT_XOR_EN_DIS      =  0x00e8,
  OVERRIDE_T_PMA_CL91_MUX_SEL_DIS =  0x00e9,
  OVERRIDE_T_PMA_WM_DIS           =  0x00ea,
  OVERRIDE_T_PMA_BTMX_DELAY_DIS   =  0x00eb,
  OVERRIDE_AM_SPACING_MUL_DIS     =  0x00ec,
  OVERRIDE_CL91_BLKSYNC_MODE_DIS  =  0x00ed,
  OVERRIDE_R_MERGE_MODE_DIS       =  0x00ee,
  OVERRIDE_R_CL91_CW_SCR_DIS      =  0x00d0,
  OVERRIDE_R_TC_IN_MODE_DIS       =  0x00d1,
  OVERRIDE_R_TC_MODE_DIS          =  0x00d2,
  OVERRIDE_R_5BIT_XOR_EN_DIS      =  0x00d3,
  OVERRIDE_R_TC_OUT_MODE_DIS      =  0x00d4,
  OVERRIDE_R_HG2_EN_DIS           =  0x00d5
} override_type_t;

typedef enum {
  APERTA_TBHMOD_REF_CLK_156P25MHZ    = 1,
  APERTA_TBHMOD_REF_CLK_312P5MHZ     = 2
}aperta_tbhmod_refclk_t;

/**
\struct aperta_tbhmod_an_adv_ability_s
TBD
*/

typedef struct aperta_tbhmod_an_control_s {
  aperta_tbhmod_an_type_t an_type;
  uint16_t num_lane_adv;
  uint16_t enable;
  aperta_tbhmod_an_property_enable  aperta_tbhmod_an_property_type;
} aperta_tbhmod_an_control_t;

/**
\struct aperta_tbhmod_an_init_s

This embodies all parameters required to program autonegotiation features in the
PHY for both CL37 and CL73 type autonegotiation and the BAM variants.
For details of individual fields, please refer to microarchitectural doc.
*/
typedef struct aperta_tbhmod_an_init_s{
  uint16_t  an_fail_cnt;
  uint16_t  linkfailtimer_dis;
  uint16_t  linkfailtimerqua_en;
  uint16_t  an_good_check_trap;
  uint16_t  an_good_trap;
  uint16_t  bam73_adv_oui;
  uint16_t  bam73_det_oui;
  uint16_t  hpam_adv_oui;
  uint16_t  hpam_det_oui;
  uint16_t  disable_rf_report;
  uint16_t  cl73_remote_fault;
  uint16_t  cl73_nonce_match_over;
  uint16_t  cl73_nonce_match_val;
  uint16_t  cl73_transmit_nonce;
  uint16_t  base_selector;
} aperta_tbhmod_an_init_t;


/**
\struct aperta_tbhmod_an_timers_s
TBD
*/
typedef struct aperta_tbhmod_an_timers_s{
  uint16_t  value;
} aperta_tbhmod_an_timers_t;

/*!
 * @enum phymod_interrupt_type_e
 * @brief Phymod interrupt types. 
 */ 
typedef enum aperta_interrupt_type_e {
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
    phymodIntrCount
} aperta_interrupt_type_t;

/* struct for interrupts status */
typedef struct tbhmod_intr_status_s {
    aperta_interrupt_type_t type;
    uint8_t is_1b_err;
    uint8_t is_2b_err;
    int err_addr;
} aperta_tbhmod_intr_status_t;

/*struct for 1588 tx info*/
typedef struct aperta_tbhmod_ts_tx_info_s {
    uint16_t ts_val_lo;
    uint16_t ts_val_mid;
    uint16_t ts_val_hi;
    uint16_t ts_seq_id;
    uint16_t ts_sub_nanosec;
} aperta_tbhmod_ts_tx_info_t;

/* MACROs for AN PAGE definition*/
#define BRCM_OUI 0xAF7
#define BRCM_BAM_CODE 0x3
#define MSA_OUI 0x6A737D /* OUI defined for consortium 25G */
#define MSA_OUI_13to23 0x353
#define MSA_OUI_2to12  0x4DF
#define MSA_OUI_0to1   0x1

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
#define AN_BASE0_PAGE_PAUSE_MASK   0x3
#define AN_BASE0_PAGE_PAUSE_OFFSET 0xA

#define AN_BASE0_PAGE_NP_MASK   0x1
#define AN_BASE0_PAGE_NP_OFFSET 0xF

#define AN_BASE1_TECH_ABILITY_10G_KR1_MASK   0x1
#define AN_BASE1_TECH_ABILITY_10G_KR1_OFFSET 0x7

#define AN_BASE1_TECH_ABILITY_40G_KR4_MASK   0x1
#define AN_BASE1_TECH_ABILITY_40G_KR4_OFFSET 0x8

#define AN_BASE1_TECH_ABILITY_40G_CR4_MASK   0x1
#define AN_BASE1_TECH_ABILITY_40G_CR4_OFFSET 0x9

#define AN_BASE1_TECH_ABILITY_100G_KR4_MASK   0x1
#define AN_BASE1_TECH_ABILITY_100G_KR4_OFFSET 0xC

#define AN_BASE1_TECH_ABILITY_100G_CR4_MASK   0x1
#define AN_BASE1_TECH_ABILITY_100G_CR4_OFFSET 0xD

#define AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_MASK   0x1
#define AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_OFFSET 0xE

#define AN_BASE1_TECH_ABILITY_25G_KR1_CR1_MASK   0x1
#define AN_BASE1_TECH_ABILITY_25G_KR1_CR1_OFFSET 0xF

#define AN_BASE2_TECH_ABILITY_50G_KR1_CR1_MASK   0x1
#define AN_BASE2_TECH_ABILITY_50G_KR1_CR1_OFFSET 0x2

#define AN_BASE2_TECH_ABILITY_100G_KR2_CR2_MASK   0x1
#define AN_BASE2_TECH_ABILITY_100G_KR2_CR2_OFFSET 0x3

#define AN_BASE2_TECH_ABILITY_200G_KR4_CR4_MASK   0x1
#define AN_BASE2_TECH_ABILITY_200G_KR4_CR4_OFFSET 0x4

#define AN_BASE2_25G_RS_FEC_ABILITY_REQ_MASK   0x1
#define AN_BASE2_25G_RS_FEC_ABILITY_REQ_OFFSET 0xC

#define AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_MASK   0x1
#define AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_OFFSET 0xD

#define AN_BASE2_CL74_ABILITY_REQ_SUP_MASK   0x3
#define AN_BASE2_CL74_ABILITY_REQ_SUP_OFFSET 0xE

/* Message Page definitions */

#define AN_MSG_PAGE1_OUI_13to23_MASK 0x7FF
#define AN_MSG_PAGE1_OUI_13to23_OFFSET 0x0

#define AN_MSG_PAGE2_OUI_2to12_MASK   0x7FF
#define AN_MSG_PAGE2_OUI_2to12_OFFSET 0x0

/* Unformatted Page definitions */
#define AN_UF_PAGE0_UD_0to8_MASK 0x1FF
#define AN_UF_PAGE0_UD_0to8_OFFSET 0x0

#define AN_UF_PAGE0_OUI_MASK   0x3
#define AN_UF_PAGE0_OUI_OFFSET 0x9

/* Unformatted Page for CL73BAM only*/
#define AN_UF_PAGE1_BAM_20G_KR1_MASK 0x1
#define AN_UF_PAGE1_BAM_20G_KR1_OFFSET 0x2

#define AN_UF_PAGE1_BAM_20G_CR1_MASK 0x1
#define AN_UF_PAGE1_BAM_20G_CR1_OFFSET 0x3

#define AN_UF_PAGE1_BAM_25G_KR1_MASK 0x1
#define AN_UF_PAGE1_BAM_25G_KR1_OFFSET 0x4

#define AN_UF_PAGE1_BAM_25G_CR1_MASK 0x1
#define AN_UF_PAGE1_BAM_25G_CR1_OFFSET 0x5

#define AN_UF_PAGE1_BAM_40G_KR2_MASK 0x1
#define AN_UF_PAGE1_BAM_40G_KR2_OFFSET 0x6

#define AN_UF_PAGE1_BAM_40G_CR2_MASK 0x1
#define AN_UF_PAGE1_BAM_40G_CR2_OFFSET 0x7

#define AN_UF_PAGE1_BAM_50G_KR2_MASK 0x1
#define AN_UF_PAGE1_BAM_50G_KR2_OFFSET 0x8

#define AN_UF_PAGE1_BAM_50G_CR2_MASK 0x1
#define AN_UF_PAGE1_BAM_50G_CR2_OFFSET 0x9

#define AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_MASK 0x1
#define AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_OFFSET 0xA

#define AN_UF_PAGE1_BAM_50G_BRCM_NO_FEC_CR1_KR1_MASK 0x1
#define AN_UF_PAGE1_BAM_50G_BRCM_NO_FEC_CR1_KR1_OFFSET 0xD

#define AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_MASK 0x1
#define AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_OFFSET 0xE

#define AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_MASK 0x1
#define AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_OFFSET 0x3

#define AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_MASK 0x1
#define AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_OFFSET 0x4

#define AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_MASK 0x1
#define AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_OFFSET 0x5

#define AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_MASK 0x1
#define AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_OFFSET 0x7

#define AN_UF_PAGE2_BAM_CL91_REQ_MASK 0x1
#define AN_UF_PAGE2_BAM_CL91_REQ_OFFSET 0xB

#define AN_UF_PAGE2_BAM_CL74_REQ_MASK 0x1
#define AN_UF_PAGE2_BAM_CL74_REQ_OFFSET 0xC

#define AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_MASK 0x1
#define AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_OFFSET 0xD

#define AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_MASK 0x1
#define AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_OFFSET 0xE

/* Unformatted Page for MSA only*/
#define AN_UF_PAGE1_MSA_25G_KR1_ABILITY_MASK   0x1
#define AN_UF_PAGE1_MSA_25G_KR1_ABILITY_OFFSET 0x4

#define AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK   0x1
#define AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET 0x5

#define AN_UF_PAGE1_MSA_50G_KR2_ABILITY_MASK   0x1
#define AN_UF_PAGE1_MSA_50G_KR2_ABILITY_OFFSET 0x8

#define AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK   0x1
#define AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET 0x9

#define AN_UF_PAGE2_MSA_CL91_SUPPORT_MASK   0x1
#define AN_UF_PAGE2_MSA_CL91_SUPPORT_OFFSET 0x8

#define AN_UF_PAGE2_MSA_CL74_SUPPORT_MASK   0x1
#define AN_UF_PAGE2_MSA_CL74_SUPPORT_OFFSET 0x9

#define AN_UF_PAGE2_MSA_CL91_REQ_MASK   0x1
#define AN_UF_PAGE2_MSA_CL91_REQ_OFFSET 0xA

#define AN_UF_PAGE2_MSA_CL74_REQ_MASK   0x1
#define AN_UF_PAGE2_MSA_CL74_REQ_OFFSET 0xB

#ifdef _DV_TB_
extern int plp_aperta_phymod_debug_check(uint32_t flags, aperta_tbhmod_st *pc);
#endif
/*extern int configure_st_entry(int st_entry_num, int st_entry_speed, aperta_tbhmod_st* pc); */
extern int aperta_tbhmod_pmd_reset_seq(PHYMOD_ST* pc, int pmd_touched); /* PMD_RESET_SEQ */
extern int plp_aperta_tbhmod_pmd_x4_reset(PHYMOD_ST* pc);
extern int plp_aperta_tbhmod_update_port_mode_select(PHYMOD_ST* pc,aperta_tbhmod_port_type_t port_type);
extern int plp_aperta_tbhmod_tx_lane_control_get(PHYMOD_ST* pc,  int *reset, int *enable);
extern int plp_aperta_tbhmod_get_pcs_latched_link_status(PHYMOD_ST* pc, uint32_t *link);


/* extern int aperta_tbhmod_clause72_control(aperta_tbhmod_st* pc, int per_lane_control); */
/*extern int aperta_tbhmod_FEC_control(aperta_tbhmod_st* pc, int fec_en, int fec_dis, int cl74or91);
 */
extern int plp_aperta_tbhmod_power_control(PHYMOD_ST* pc, int tx, int rx);
extern int aperta_tbhmod_disable_pcs_blackhawk(PHYMOD_ST* pc);
extern int aperta_tbhmod_pmd_reset_remove(PHYMOD_ST *dv_st, int pmd_touched);
extern int plp_aperta_tbhmod_set_sc_speed(PHYMOD_ST* pc,int mapped_speed, int set_sw_speed_change);

extern int plp_aperta_tbhmod_get_mapped_speed(aperta_tbhmod_spd_intfc_type_t spd_intf, int *speed);

#ifdef _DV_TB_
/*extern int aperta_tbhmod_autoneg_set(aperta_tbhmod_st* pc); */
extern int aperta_tbhmod_sw_autoneg_set(PHYMOD_ST* pc, sw_autoneg_st* an_st);
extern int aperta_tbhmod_autoneg_set(PHYMOD_ST* pc, autoneg_set_st* an_st);
extern int aperta_tbhmod_autoneg_get(aperta_tbhmod_st* pc);
extern int plp_aperta_tbhmod_autoneg_control(PHYMOD_ST* pc, autoneg_control_st* an_st);
extern int aperta_tbhmod_set_spd_intf(aperta_tbhmod_st* pc);
extern int plp_aperta_tbhmod_set_an_port_mode(PHYMOD_ST* pc);
#endif /* _DV_TB_ */
extern int plp_aperta_tbhmod_set_an_port_mode(PHYMOD_ST* pc, int starting_lane);


extern int plp_aperta_tbhmod_autoneg_ability_set(PHYMOD_ST* pc, const phymod_autoneg_advert_abilities_t* autoneg_abilities);
extern int plp_aperta_tbhmod_autoneg_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t* autoneg_abilities);
extern int plp_aperta_tbhmod_autoneg_remote_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t *autoneg_abilities);
extern int plp_aperta_tbhmod_autoneg_control(PHYMOD_ST* pc, aperta_tbhmod_an_control_t *an_control);
extern int plp_aperta_tbhmod_autoneg_control_get(PHYMOD_ST* pc, aperta_tbhmod_an_control_t *an_control, int *an_complete);
extern int plp_aperta_tbhmod_autoneg_status_get(PHYMOD_ST* pc, int *an_en, int *an_done);
extern int plp_aperta_tbhmod_set_an_timers(PHYMOD_ST* pc, plp_aperta_phymod_ref_clk_t refclk, uint32_t *pam4_an_timer_value);    /* SET_AN_TIMERS */

#ifdef DV_AN_SUPPORT
#ifdef _SDK_APERTA_TBHMOD_
extern int aperta_tbhmod_set_spd_intf(aperta_tbhmod_st *pc, aperta_tbhmod_spd_intfc_type_t spd_intf);
extern int aperta_tbhmod_autoneg_set_init(aperta_tbhmod_st* pc, aperta_tbhmod_an_init_t *an_init_st);
extern int aperta_tbhmod_autoneg_timer_init(aperta_tbhmod_st* pc);
extern int aperta_tbhmod_diag(aperta_tbhmod_st *ws, aperta_tbhmod_diag_type_t diag_type);
extern int aperta_tbhmod_port_enable_set(aperta_tbhmod_st *pc, int enable);
extern int aperta_tbhmod_tx_squelch_set(aperta_tbhmod_st *pc, int enable);
extern int aperta_tbhmod_rx_squelch_set(aperta_tbhmod_st *pc, int enable);
extern int aperta_tbhmod_rx_squelch_get(aperta_tbhmod_st *pc, int *enable);
#endif /* _SDK_APERTA_TBHMOD_ */
#endif /* DV_AN_SUPPORT */

extern int plp_aperta_tbhmod_speed_id_get(PHYMOD_ST* pc, int *speed_id);
extern int plp_aperta_tbhmod_enable_get(PHYMOD_ST* pc, uint32_t* enable);
extern int plp_aperta_tbhmod_disable_set(PHYMOD_ST* pc);
extern int plp_aperta_tbhmod_enable_set(PHYMOD_ST* pc);
extern int plp_aperta_tbhmod_rx_lane_control_get(PHYMOD_ST* pc, int *enable);
extern int plp_aperta_tbhmod_update_port_mode( PHYMOD_ST *pc);

extern int plp_aperta_tbhmod_tx_lane_control( PHYMOD_ST* pc, int en, aperta_tbhmod_tx_disable_enum_t dis);
extern int plp_aperta_tbhmod_rx_lane_control(PHYMOD_ST* pc, int per_lane_control);

extern int plp_aperta_tbhmod_pcs_rx_lane_swap(PHYMOD_ST* pc, int rx_lane_swap);
extern int plp_aperta_tbhmod_pcs_tx_lane_swap(PHYMOD_ST* pc, int tx_lane_swap);

extern int plp_aperta_tbhmod_rs_fec_rx_control_0(PHYMOD_ST* pc, int cl91_bypass_indication);
extern int plp_aperta_tbhmod_rsfec_cw_type(PHYMOD_ST* pc,int en,int fec_bypass_correction);

extern int plp_aperta_tbhmod_low_latency_mode_en(PHYMOD_ST* pc,int low_latency_mode_en);
extern int plp_aperta_tbhmod_cl74_control_fec_0(PHYMOD_ST* pc, int cl74_good_parity_cnt, int cl74_invalid_parity_cnt, int cl74_dec_max_pm, int cl74_burst_err_status_mode, int cl74_dbg_err_mode);
extern int plp_aperta_tbhmod_cl74_control_fec_1(PHYMOD_ST* pc,int cl74_dec_pm_mode, int cl74_dec_19b_burst_gap_count, int cl74_dec_18b_burst_gap_count, int cl74_dec_17b_burst_gap_count, int cl74_dec_gap_count_mode);
extern int plp_aperta_tbhmod_cl74_control_fec_2(PHYMOD_ST* pc, int cl74_fec_err_enable_per_stream_mpp0, int cl74_dbg_enable_per_stream_mpp0, int cl74_fec_error_code_all_per_stream_mpp0, int cl74_fec_err_enable_per_stream_mpp1, int cl74_dbg_enable_per_stream_mpp1, int cl74_fec_error_code_all_per_stream_mpp1, aperta_tbhmod_port_type_t port_type);
extern int plp_aperta_tbhmod_pcs_rx_scramble_idle_en(PHYMOD_ST* pc, int en);
/* 1588 APIs */
extern int plp_aperta_tbhmod_osts_pipeline(PHYMOD_ST* pc,uint32_t en);
extern int plp_aperta_tbhmod_osts_pipeline_get(PHYMOD_ST* pc,uint32_t* en);
extern int plp_aperta_tbhmod_pcs_rx_ts_en(PHYMOD_ST* pc, int en, int sfd);
extern int plp_aperta_tbhmod_pcs_rx_ts_en_get(PHYMOD_ST* pc, uint32_t* en);
extern int plp_aperta_tbhmod_pcs_set_1588_ui (PHYMOD_ST* pc, uint32_t vco, uint32_t vco1, int os_mode, int clk4sync_div, int pam4);
extern int plp_aperta_tbhmod_1588_pmd_latency (PHYMOD_ST* pc, uint32_t vco, int os_mode, int pam4);
extern int plp_aperta_tbhmod_pcs_rx_deskew_en(PHYMOD_ST* pc, int en);
extern int plp_aperta_tbhmod_pcs_set_1588_xgmii (PHYMOD_ST* pc, int sfd, uint32_t vco, int os_mode);
extern int plp_aperta_tbhmod_pcs_ts_deskew_valid(PHYMOD_ST* pc, int bit_mux_mode, int *rx_ts_deskew_valid);
extern int plp_aperta_tbhmod_pcs_mod_rx_1588_tbl_val(PHYMOD_ST* pc, int bit_mux_mode, uint32_t vco, int os_mode, int pam4, int tbl_ln, uint32_t *table);

extern int plp_aperta_tbhmod_pack_spd_id_tbl_entry (spd_id_tbl_entry_t *entry, uint32_t *packed_entry);
extern int plp_aperta_tbhmod_unpack_spd_id_tbl_entry (uint32_t *packed_entry, spd_id_tbl_entry_t *entry);
extern int plp_aperta_tbhmod_pack_am_tbl_entry (am_tbl_entry_t *entry, uint32_t *packed_entry);
extern int plp_aperta_tbhmod_unpack_am_tbl_entry (uint32_t *packed_entry, am_tbl_entry_t *entry);
extern int plp_aperta_tbhmod_pack_um_tbl_entry (um_tbl_entry_t *entry, uint32_t *packed_entry);
extern int plp_aperta_tbhmod_unpack_um_tbl_entry (uint32_t *packed_entry, um_tbl_entry_t *entry);
extern int plp_aperta_tbhmod_rx_lock_ovrd(PHYMOD_ST* pc, uint16_t ovrd);
extern int plp_aperta_tbhmod_rx_sigdet_ovrd(PHYMOD_ST* pc, uint16_t ovrd);
extern int plp_aperta_tbhmod_tx_lane_disable (PHYMOD_ST* pc, uint16_t ovrr, uint16_t value);
extern int plp_aperta_tbhmod_pcs_set_ilkn_wm(PHYMOD_ST* pc,int ilkn_wm);
extern int plp_aperta_tbhmod_init_pcs_ilkn(PHYMOD_ST* pc);
extern int plp_aperta_tbhmod_plldiv_lkup_get(PHYMOD_ST* pc, int mapped_speed, aperta_tbhmod_refclk_t refclk,  uint32_t *plldiv);
extern int plp_aperta_tbhmod_pcs_ilkn_chk(PHYMOD_ST* pc, int *ilkn_set);
extern int plp_aperta_tbhmod_refclk_set(PHYMOD_ST* pc, aperta_tbhmod_refclk_t ref_clk);
extern int plp_aperta_tbhmod_refclk_get(PHYMOD_ST* pc, aperta_tbhmod_refclk_t* ref_clk);
extern int plp_aperta_tbhmod_pmd_osmode_set(PHYMOD_ST* pc, int mapped_speed, aperta_tbhmod_refclk_t refclk);
extern int plp_aperta_tbhmod_fec_correctable_counter_get(PHYMOD_ST* pc,phymod_fec_type_t fec_type, uint32_t* count);
extern int plp_aperta_tbhmod_fec_uncorrectable_counter_get(PHYMOD_ST* pc, phymod_fec_type_t fec_type, uint32_t* count);
extern int plp_aperta_tbhmod_autoneg_fec_status_get(PHYMOD_ST* pc, uint8_t *fec_status);
extern int plp_aperta_tbhmod_pcs_ilkn_enable(PHYMOD_ST* pc, int enable);

extern int plp_aperta_tbhmod_synce_mode_set(PHYMOD_ST* pc, int stage0_mode, int stage1_mode);
extern int plp_aperta_tbhmod_synce_mode_get(PHYMOD_ST* pc, int* stage0_mode, int* stage1_mode);
extern int plp_aperta_tbhmod_synce_clk_ctrl_set(PHYMOD_ST* pc, uint32_t val);
extern int plp_aperta_tbhmod_synce_clk_ctrl_get(PHYMOD_ST* pc, uint32_t* val);
extern int plp_aperta_tbhmod_1588_tx_info_get(PHYMOD_ST* pc, aperta_tbhmod_ts_tx_info_t* ts_tx_info);
#ifdef FIXME
extern int print_aperta_tbhmod_st(aperta_tbhmod_st* pc);
#endif

int plp_aperta_tbhmod_pmd_rx_lock_override_enable(PHYMOD_ST* pc, uint32_t enable);
int plp_aperta_tbhmod_read_sc_fsm_status(PHYMOD_ST* pc);
int plp_aperta_tbhmod_read_sc_done(PHYMOD_ST* pc);
int plp_aperta_tbhmod_polling_for_sc_done(PHYMOD_ST* pc);
extern int plp_aperta_tbhmod_pcs_reset_sw_war(const PHYMOD_ST* pc);
extern int plp_aperta_tbhmod_ts_offset_rx_set(PHYMOD_ST* pc, uint32_t vco, int os_mode, int tbl_ln, uint32_t *table);
int plp_aperta_tbhmod_intr_status_get(PHYMOD_ST* pc, aperta_tbhmod_intr_status_t* intr_status);
int plp_aperta_tbhmod_pcs_rx_ts_update_en(PHYMOD_ST* pc, int en);
extern int plp_aperta_tbhmod_port_enable_get(PHYMOD_ST* pc, uint32_t* enable);
int plp_aperta_tbhmod_port_start_lane_get(PHYMOD_ST *pc, int *port_starting_lane, int *port_num_lane);
int plp_aperta_tbhmod_fec_override_get(PHYMOD_ST* pc, uint32_t* enable);
int plp_aperta_tbhmod_400g_autoneg_timer_disable(PHYMOD_ST* pc, int timer_disable);
#endif  /*  _aperta_tbhmod_H_ */

