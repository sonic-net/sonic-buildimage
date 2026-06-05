/*
 * $Id: $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#ifndef TIMESYNC_H
#define TIMESYNC_H

#include <phymod_custom_config.h>
#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <phymod/phymod_symbols.h>
#include <phymod/phymod_acc.h>
#include <phymod/phymod_dispatch.h>
#if defined(PHYMOD_SHORTFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
#include <phymod/chip/xgbaset_phymod_acc.h>
#endif

#ifdef  PHYMOD_PLP_DRIVER_DEBUG
  #include <phymod/phymod_debug.h>
#endif

#define  TIMESYNC_ENABLE            TRUE
#define  TIMESYNC_DISABLE           FALSE

#if   defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT)
  #include "bcmi_evora_defs.h"
  #include "evora_reg_access.h"
  #include "evora_pm_seq.h"
  #define  HSIP_FWS_FWMSG_TOD     0x0007
  #define  HSIP_FWS_FWREG_000r    0x0001A000
  #define  HSIP_FWS_FWREG_001r    0x0001A001
  #define  HSIP_MST_MSGOUTr       BCMI_EVORA_MST_MSGOUTr  /* 0x00018221 */
  #define  HSIP_MST_MSGINr        BCMI_EVORA_MST_MSGINr   /* 0x00018222 */
  #define  HSIP_MST_MSGIN_TOD     0x1801
  #define  HSIP_MST_MSGOUT_TOD    0x18E3
  #define  HSIP_GET_PORT_FROM_LM  EVORA_GET_PORT_FROM_LM
  #define  PLP_P1588_REG_BASE     EVORA_P1588_BASEADR     /* 0x49007000 */

#elif defined(PHYMOD_MIURA_SUPPORT)
  #include "miura_reg_access.h"
  #define  PLP_P1588_REG_BASE     MIURA_P1588_BASEADR     /* 0x49007000 */

#elif defined(PHYMOD_APERTA_SUPPORT)
  #include "bcmi_aperta_d_defs.h"
  #include "aperta_reg_access.h"
  #include "aperta_pm_seq.h"
  #include "aperta_msg_tasks.h"
  #define  HSIP_FWS_FWMSG_TOD     0x0007
  #define  HSIP_FWS_FWREG_000r    BCMI_APERTA_D_FWS_FWREG_000r
  #define  HSIP_FWS_FWREG_001r    BCMI_APERTA_D_FWS_FWREG_001r
  #define  HSIP_MST_MSGOUTr       BCMI_APERTA_D_GEN_CNTRLS_MST_MSGOUTr /* 0x00018221 */
  #define  HSIP_MST_MSGINr        BCMI_APERTA_D_GEN_CNTRLS_MST_MSGINr  /* 0x00018222 */
  #define  HSIP_MST_MSGIN_TOD     0x1D01
  #define  HSIP_MST_MSGOUT_TOD    0x1DE3
  #define  HSIP_GET_PORT_FROM_LM  APERTA_GET_PORT_FROM_LM
  #define  PLP_P1588_REG_BASE     APERTA_P1588_BASEADR    /* 0x49007000 */

  #define  HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE

#elif defined(PHYMOD_QUADRA28_SUPPORT)
  #include "quadra28_reg_access.h"
  #define  PLP_P1588_REG_BASE     (0xC600 | 0x00010000)   /* DEVAD 1, REGAD 0xC6nn */
  #define  Q28_SERDES_PLL_CTRL4_REG                 0x1D0B4
  #define  Q28_SERDES_PLL_CTRL4_VAL                 0x0420
  #define  Q28_SERDES_RETIMER_9_REG                 0x1C8D9
  #define  Q28_SERDES_RETIMER_9_VAL                 0x0010
  #define  Q28_SERDES_RETIMER_8_REG                 0x1C8D8
  #define  Q28_SERDES_RETIMER_8_VAL0                0x8802
  #define  Q28_SERDES_RETIMER_8_VAL8                0x8882
  #define  Q28_SERDES_RETIMER_8_APPLY_CFG           0x0080
  #define  QUADRA28_PMD_STATUS_REG                  0x1C804
  #define  QUADRA28_PMD_STATUS_VAL                  0x00DD
  #define  QUADRA28_PMD_STATUS_WAIT_TIME_US         500000
  #define  QUADRA28_PMD_STATUS_WAIT_SLEEP_US        10000

#elif defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_PHY848XX_SUPPORT) \
                                      || defined(PHYMOD_MGAUTO_SUPPORT)
  #define  PLP_P1588_REG_BASE     (0xC600 | 0x00010000)   /* DEVAD 1, REGAD 0xC6nn */
  #define  XGP_PF_RW_REG_00        0x001e4110   /* 30.0x4110 Device R/W Access  00 */
  #define  XGP_PF_RW_REG_XFI       0x2004
  #define  XGBASET_IEEE1588_REG_FAST

#else
  #error  IEEE1588 not supported !!
#endif

#define  HSIP_MST_MSGOUT_WAIT_TIME_US               10000
#define  HSIP_MST_MSGOUT_WAIT_SLEEP_US              100

#define  BITMAP32(_v)                               (1U << (_v))
#define  TIMESYNC_HSIP_FWS_FWMSG_MASK               0x60000000

#if defined(PHYMOD_KAUAI_SUPPORT) || defined(PHYMOD_LANAI_SUPPORT)
#define  CRG_CHIP_CLKEN_CTLr                        0x1e8802
#else
#define  CRG_CHIP_CLKEN_CTLr                        0x1e8002
#endif
#define  CRG_CHIP_CLKEN_CTL_CLK_1G_SHIFT            1
#define  CRG_CHIP_CLKEN_CTL_CLK_1G_MASK             (1U << CRG_CHIP_CLKEN_CTL_CLK_1G_SHIFT)

#if defined(PHYMOD_KAUAI_SUPPORT) || defined(PHYMOD_LANAI_SUPPORT)
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL2r              0x1e8911
#else
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL2r              0x1e8111
#endif
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL2_NORMAL_MODE   0x0
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL2_PWRDOWN_MODE  0x1

#if defined(__CHIP_MISC_CFG_XTAL_PTP_CTRL1_REG_SETTING__)
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL1r              0x1e8110
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL1_RESET_VALUE   0x72A1
#define  CHIP_MISC_CFG_XTAL_PTP_CTRL1_PTP_REFCLK    0x21F8
#endif
#define  PTP_REFCLK_EXTERNAL                        TRUE
#define  PTP_REFCLK_INTERNAL                        FALSE

#define  CORE_CFG_HW_STRAPr                         0x1e401c
#define  CORE_CFG_HW_STRAP_COREID_SHIFT             5
#define  CORE_CFG_HW_STRAP_COREID_MASK              (0x7 << CORE_CFG_HW_STRAP_COREID_SHIFT)

#include "p1588_reg_defs.h"

#define  PORTS_PER_PHY_CHIP_MAX   8
#define  TBD32                    0xFFFFFFFFUL
#define  TBD16                    0xFFFFU
#define  TBD8                     0xFFU
#define  INVALID                  0x0U
#define  PENDING                  0x1
#define  FETCHED                  0x3

#ifndef   BIT
  #define BIT(_n)                (0x1U << (_n))
#endif

#if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT)
           /*  Shortfin / Longfin / Blackfin / Broadfin / Whitetip / Eiger */
  #define  TS_IEEE1588_SOPMEM_ARCH_20  /* 1588 engine 2.0: SOPmem supports double VLAN Tags */
  /* Inband_Ctrl2[2:0] to select PTPsrcPort/MacDA/SA/DstIP/SrcIP/MPLS/IPv6 */
  #define  TS_SOPMEM_CAP_PTP_SRC_PORT   0x00U
  #define  TS_SOPMEM_CAP_MAC_DA         0x01U
  #define  TS_SOPMEM_CAP_MAC_SA         0x02U
  #define  TS_SOPMEM_CAP_DST_IPV4       0x03U
  #define  TS_SOPMEM_CAP_SRC_IPV4       0x04U
  #define  TS_SOPMEM_CAP_MPLS_LABEL     0x05U
  #define  TS_SOPMEM_CAP_RESV_2         0x06U
  #define  TS_SOPMEM_CAP_DST_IPV6       0x07U
  #define  TS_SOPMEM_CAP_UNKNOWN        0x0FU
#else      /*  Aperta / Miura / Evora / Europa / Quadra28 / Orca           */
  /* Inband_Ctrl2[7:0] to select PTPsrcPort/MacDA/SA/DstIP/SrcIP/MPLS/IPv6 */
  #define  TS_SOPMEM_CAP_PTP_SRC_PORT   BIT(0)
  #define  TS_SOPMEM_CAP_MAC_DA         BIT(1)
  #define  TS_SOPMEM_CAP_MAC_SA         BIT(2)
  #define  TS_SOPMEM_CAP_DST_IPV4       BIT(3)
  #define  TS_SOPMEM_CAP_SRC_IPV4       BIT(4)
  #define  TS_SOPMEM_CAP_MPLS_LABEL     BIT(5)
  #define  TS_SOPMEM_CAP_RESV_2         BIT(6)
  #define  TS_SOPMEM_CAP_DST_IPV6       BIT(7)
  #define  TS_SOPMEM_CAP_UNKNOWN        0x00U
#endif

#define  UINT20_HI04_SHIFT        16
#define  UINT20_HI04_MASK         0x0000000F
#define  UINT20_LO16_MASK         0x0000FFFF
#define  UINT20_SET(_hi,_lo)      ( (((_hi) & UINT20_HI04_MASK) << UINT20_HI04_SHIFT) | \
                                     ((_lo) & UINT20_LO16_MASK) )
#define  BIT_31_00_MASK           0xFFFFFFFF
#define  BIT_31_28_MASK           0xF0000000
#define  BIT_31_16_MASK           0xFFFF0000
#define  BIT_15_00_MASK           0xFFFF
#define  BIT_15_12_MASK           0xF000
#define  BIT_15_09_MASK           0xFE00
#define  BIT_15_08_MASK           0xFF00
#define  BIT_11_08_MASK           0x0F00
#define  BIT_11_04_MASK           0x0FF0
#define  BIT_11_00_MASK           0x0FFF
#define  BIT_07_00_MASK           0x00FF
#define  BIT_04_00_MASK           0x001F
#define  BIT_03_00_MASK           0x000F
#define  BIT_02_00_MASK           0x0007
#define  BIT_01_00_MASK           0x0003
#define  BIT_13_06_MASK           0x3FC0
#define  P1588_SOPMEM_ENTRY_MAX   32    /* 32 entries in SOPmem */
#define  P1588_SOPMEM_WORDS_MAX   14    /* 14 words for each entry, each word has 16-bit */
#define  P1588_TIMESTAMP_LEN      10    /* 10-byte timestamp */

#define  TS_MPLS_LABEL_COUNT            10
#define  TS_MPLS_LABEL_MAX             (TS_MPLS_LABEL_COUNT - 1)
#define  TS_MPLS_DIRECTION_RX           PHYMOD_TS_MPLS_LABEL_F_IN
#define  TS_MPLS_DIRECTION_TX           PHYMOD_TS_MPLS_LABEL_F_OUT
#define  TS_MPLS_DIRECTION_RXTX        (TS_MPLS_DIRECTION_RX | TS_MPLS_DIRECTION_TX)
#define  TS_INBAND_FILTER_DIRECTION_RX PHYMOD_TS_MPLS_LABEL_F_IN
#define  TS_INBAND_FILTER_DIRECTION_TX PHYMOD_TS_MPLS_LABEL_F_OUT
#define  TS_INBAND_FILTER_DIRECTION_RXTX  (TS_INBAND_FILTER_DIRECTION_RX | TS_INBAND_FILTER_DIRECTION_TX)
#define  IS_TS_MPLS_DIRECTION_RX(_f)   ( (((_f) & TS_MPLS_DIRECTION_RXTX) == 0x0) ||  \
                                          ((_f) & TS_MPLS_DIRECTION_RX) )
#define  IS_TS_MPLS_DIRECTION_TX(_f)   ( (((_f) & TS_MPLS_DIRECTION_RXTX) == 0x0) ||  \
                                          ((_f) & TS_MPLS_DIRECTION_TX) )
#define  IS_TS_INBAND_FILTER_DIRECTION_RX(_f)   ( (((_f) & TS_INBAND_FILTER_DIRECTION_RXTX) == 0x0) ||  \
                                          ((_f) & TS_INBAND_FILTER_DIRECTION_RX) )
#define  IS_TS_INBAND_FILTER_DIRECTION_TX(_f)   ( (((_f) & TS_INBAND_FILTER_DIRECTION_RXTX) == 0x0) ||  \
                                          ((_f) & TS_INBAND_FILTER_DIRECTION_TX) )
#define  TS_DIRECTION_RX_GET(_f)       ((((_f) & PHYMOD_TS_DIRECTION_RX) ? 1 : 0) || !(_f))
#define  TS_DIRECTION_TX_GET(_f)       ((((_f) & PHYMOD_TS_DIRECTION_TX) ? 1 : 0) || !(_f))
#define  NIBBLE_SHIM(_v, _s)           ((((_v) >> UINT20_HI04_SHIFT) & BIT_03_00_MASK) << _s)

#define PKGEN_INDREG_TX_INDEX           2
#define PKGEN_INDREG_RX_INDEX           3
#define PKGEN_EN_INDEX_LOAD             1
#define DIS_PKGEN_ENA_FILTER            0
#define FILTER_FOR_MAC                  (1U << 3)
#define FILTER_ACTION_NONE              0
#define FILTER_ACTION_INBAND36          1
#define FILTER_ACTION_INBAND32          2
#define FILTER_ACTION_MDIO_NOINBAND     3
#define FILTER_ACTION_INBAND32_PTPVER   4

/* indirect register read/write operations for PTP-v2.1 Lookup-Actions */
#define INDIR_REG_READ                  0
#define INDIR_REG_WRITE                 1
#define INDIR_REG_WR(_rw)               ((INDIR_REG_READ == (_rw)) ? 0x0U : 0x1U)
#define INDIR_REG_RD(_rw)               ((INDIR_REG_READ == (_rw)) ? 0x1U : 0x0U)
#define NUM_USER_ACTION_SEL_REG         3  /* # of registers to define user actions */

/* TimeSync 1588 register READ/WRITE functions */
extern int plp_aperta_p1588_reg_read( const plp_aperta_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t *data);
extern int plp_aperta_p1588_reg_write(const plp_aperta_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t  data);
extern int p1588_indir_reg_read( const plp_aperta_phymod_phy_access_t *pa, int addr, uint32_t *data);
extern int p1588_indir_reg_write(const plp_aperta_phymod_phy_access_t *pa, int addr, uint32_t  data);
extern int plp_aperta_p1588_indir_reg_ctrl( const plp_aperta_phymod_phy_access_t *pa, int addr, int rw);

#if defined(PHYMOD_XGBASET_SUPPORT)
  extern uint32_t _xgbaset_phy_speed_interpret(uint16_t speed_code);
  extern int _xgbaset_phy_speed_get(const plp_aperta_phymod_phy_access_t* pacc, uint16_t *speed);
  int __timesync_xgbaset_clock_xtal_1588_enable(const plp_aperta_phymod_phy_access_t *phy, uint32_t port_offset, int en1588, int ptp_refclk);
  #define  _PHY_SPEED_LINE_GET                  _xgbaset_phy_speed_get
  #define  _PHY_SPEED_LINE_INTERPRET            _xgbaset_phy_speed_interpret
#elif defined(PHYMOD_MGAUTO_SUPPORT)
  extern uint32_t _mgauto_phy_speed_decode(uint16_t speed_code);
  extern int _mgauto_phy_speed_get(const plp_aperta_phymod_phy_access_t* pacc, uint16_t *speed);
  int __timesync_xgbaset_clock_xtal_1588_enable(const plp_aperta_phymod_phy_access_t *phy, uint32_t port_offset, int en1588, int ptp_refclk);
  #define  _PHY_SPEED_LINE_GET                  _mgauto_phy_speed_get
  #define  _PHY_SPEED_LINE_INTERPRET            _mgauto_phy_speed_decode
#endif

int _plp_aperta_timesync_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_config_t* config);
int _plp_aperta_timesync_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_config_t* config);

int _plp_aperta_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable);
int _plp_aperta_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable);

int _plp_aperta_timesync_mpls_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t direction, const plp_aperta_phymod_timesync_mpls_ctrl_t *config);
int _plp_aperta_timesync_mpls_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t direction, plp_aperta_phymod_timesync_mpls_ctrl_t *config);

/* no Inband Filter for Whitetip and mGigAuto */
#if (! defined(PHYMOD_WHITETIP_SUPPORT)) && (! defined(PHYMOD_MGAUTO_SUPPORT))
int _plp_aperta_timesync_inband_filter_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t direction, uint32_t index, const phymod_timesync_inband_filter_ctrl_t *config);
int _plp_aperta_timesync_inband_filter_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t direction, uint32_t index, phymod_timesync_inband_filter_ctrl_t *config);
#endif

int _plp_aperta_timesync_nco_addend_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t freq_step);
int _plp_aperta_timesync_nco_addend_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* freq_step);

int _plp_aperta_timesync_framesync_mode_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_framesync_t* framesync);
int _plp_aperta_timesync_framesync_mode_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_framesync_t* framesync);

int _plp_aperta_timesync_local_time_set(const plp_aperta_phymod_phy_access_t* phy, uint64_t local_time);
int _plp_aperta_timesync_local_time_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* local_time);

int _plp_aperta_timesync_load_ctrl_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t load_once, uint32_t load_always);
int _plp_aperta_timesync_load_ctrl_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* load_once, uint32_t* load_always);

int _plp_aperta_timesync_timing_control_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t regaddr, uint64_t  value64);
int _plp_aperta_timesync_timing_control_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t regaddr, uint64_t *value64);

int _plp_aperta_timesync_link_delay_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t  op, uint32_t  msg_type, uint32_t  linkdelay);
int _plp_aperta_timesync_link_delay_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *op, uint32_t *msg_type, uint32_t *linkdelay);

int _plp_aperta_timesync_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, int txrx, uint32_t  op, uint32_t  msg_type, uint32_t  ts_offset);
int _plp_aperta_timesync_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, int txrx, uint32_t *op, uint32_t *msg_type, uint32_t *ts_offset);

int _plp_aperta_timesync_time_code_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_timesync_timespec_t *timecode);
int _plp_aperta_timesync_time_code_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags, plp_aperta_phymod_timesync_timespec_t *timecode);

int _plp_aperta_timesync_capture_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* cap_ts);
int _plp_aperta_timesync_heartbeat_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* hb_ts);
int _plp_aperta_timesync_timestamp_univ_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                       uint32_t type, phymod_timestamp_univ_t *systime);
int _timesync_ptp_action_set(const plp_aperta_phymod_phy_access_t* phy, unsigned int flags,
                                      bcm_plp_timesync_txrx_t             rxtx ,
                                      bcm_plp_timesync_ptp_action_mode_t  mode ,
                                      bcm_plp_timesync_user_action_t     *user_def);
int _timesync_ptp_action_get(const plp_aperta_phymod_phy_access_t *phy, unsigned int flags,
                                      bcm_plp_timesync_txrx_t             rxtx ,
                                      bcm_plp_timesync_ptp_action_mode_t *mode ,
                                      bcm_plp_timesync_user_action_t     *user_def);

int _plp_aperta_timesync_sopmem_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, int entry_id, phymod_timesync_sopmem_t *keys);

int _plp_aperta_timesync_phy_intr_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t en_mask);
int _plp_aperta_timesync_phy_intr_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *en_mask);
int _plp_aperta_timesync_phy_intr_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *intr_status);

#endif /* TIMESYNC_H */

