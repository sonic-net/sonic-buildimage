/*
 * $Id: $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#ifndef __TIMESYNC_H__
#define __TIMESYNC_H__

#include <phymod_custom_config.h>
#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <phymod/phymod_symbols.h>
#include <phymod/phymod_acc.h>
#include <phymod/phymod_dispatch.h>
#if defined (PHYMOD_PLP_DRIVER_DEBUG)
  #include <phymod/phymod_debug.h>
#endif

#include "p1588_reg_defs.h"

#if defined (PHYMOD_APERTA2_SUPPORT)
  #include "bcm_aperta2_direct_defs.h"
  #include "aperta2_reg_access.h"
  #include "aperta2_pm_seq.h"

  #define  PLP_P1588_REG_BASE     APERTA2_CHIP_IND_BASEADR
#endif

#define  TIMESYNC_OPERATOR_SUB           2
#define  TS_OFFSET_CTRL_SHIFT            4
#define  TS_OFFSET_MSG_TYPE_MASK         0x00F0   /* bit[7:4] */
#define  TS_OFFSET_OPERATOR_MASK         0x0100   /* bit[8]   */

#define  BIT_31_16_MASK                  0xFFFF0000
#define  BIT_15_00_MASK                  0xFFFF
#define  TS_MPLS_LABEL_COUNT             10
#define  TS_MPLS_LABEL_MAX               (TS_MPLS_LABEL_COUNT - 1)
#define  TS_MPLS_DIRECTION_RX            PHYMOD_TS_MPLS_LABEL_F_IN
#define  TS_MPLS_DIRECTION_TX            PHYMOD_TS_MPLS_LABEL_F_OUT
#define  TS_MPLS_DIRECTION_RXTX          (TS_MPLS_DIRECTION_RX | TS_MPLS_DIRECTION_TX)
#define  IS_TS_MPLS_DIRECTION_RX(_f)     ( (((_f) & TS_MPLS_DIRECTION_RXTX) == 0x0) || ((_f) & TS_MPLS_DIRECTION_RX) )
#define  IS_TS_MPLS_DIRECTION_TX(_f)     ( (((_f) & TS_MPLS_DIRECTION_RXTX) == 0x0) || ((_f) & TS_MPLS_DIRECTION_TX) )
#define  TS_MPLS_EGR_LABEL0_ADDR         P1588_EGR_PARSER_MPLS_LABEL0_0_15r
#define  TS_MPLS_EGR_MASK0_ADDR          P1588_EGR_PARSER_MPLS_MASK0_0_15r
#define  TS_MPLS_IGR_LABEL0_ADDR         P1588_ING_PARSER_MPLS_LABEL0_0_15r
#define  TS_MPLS_IGR_MASK0_ADDR          P1588_ING_PARSER_MPLS_MASK0_0_15r
#define  TS_MPLS_EGR_LABEL_MASK_MSB_ADDR P1588_EGR_PARSER_MPLS_LABEL03_MSBr
#define  TS_MPLS_IGR_LABEL_MASK_MSB_ADDR P1588_ING_PARSER_MPLS_LABEL03_MSBr
#define  TS_SOPMEM_TS0_EGR_ADDR          P1588_EGR_TS_SOPMEM_TS0r
#define  TS_SOPMEM_TS0_IGR_ADDR          P1588_ING_TS_SOPMEM_TS0r
#define  TS_SOPMEM_SRC_PRT0_IGR_ADDR     P1588_ING_TS_SOPMEM_SRC_PORTID0r
#define  TS_ENCR_SEQID_MEM_EGR_CTRL_ADDR        0x00004100
#define  TS_ENCR_SEQID_MEM_EGR_STS_ADDR         0x00004101
#define  TS_ENCR_SEQID_MEM_EGR_SEQID_ADDR       0x00004102
#define  TS_ENCR_SEQID_MEM_EGR_SOPMEM_WPTR_ADDR 0x00004103
#define  TS_ENCR_SEQID_MEM_EGR_BLK_CTRL_ADDR    0x00004180
#define  TS_ONE_SEC_IN_NS                       0x3B9ACA00

#define  TS_FILTER_FOR_MAC  (1U << 3)
#define  TS_FILTER_FOR_IPV6 (1U << 4)

extern int plp_aperta2_p1588_reg_read( const plp_aperta2_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t *data);
extern int plp_aperta2_p1588_reg_write(const plp_aperta2_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t  data);

int _plp_aperta2_timesync_config_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_timesync_config_t* config);
int _plp_aperta2_timesync_config_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_timesync_config_t* config);

int _plp_aperta2_timesync_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t enable);
int _plp_aperta2_timesync_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable);

int _plp_aperta2_timesync_framesync_mode_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_timesync_framesync_t* framesync);
int _plp_aperta2_timesync_framesync_mode_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_timesync_framesync_t* framesync);

int _plp_aperta2_timesync_timestamp_offset_set(const plp_aperta2_phymod_phy_access_t* phy, int txrx, uint32_t  op, uint32_t  msg_type, uint32_t  ts_offset);
int _plp_aperta2_timesync_timestamp_offset_get(const plp_aperta2_phymod_phy_access_t* phy, int txrx, uint32_t *op, uint32_t *msg_type, uint32_t *ts_offset);

int _plp_aperta2_timesync_time_code_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t flags, const plp_aperta2_phymod_timesync_timespec_t *timecode);
int _plp_aperta2_timesync_time_code_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t flags, plp_aperta2_phymod_timesync_timespec_t *timecode);

int _plp_aperta2_timesync_sopmem_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, int entry_id, phymod_timesync_sopmem_t *keys);

int _plp_aperta2_timesync_mpls_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, const plp_aperta2_phymod_timesync_mpls_ctrl_t *config);
int _plp_aperta2_timesync_mpls_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, plp_aperta2_phymod_timesync_mpls_ctrl_t *config);

int _plp_aperta2_timesync_inband_filter_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, uint32_t index, const phymod_timesync_inband_filter_ctrl_t *config);
int _plp_aperta2_timesync_inband_filter_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, uint32_t index, phymod_timesync_inband_filter_ctrl_t *config);

#endif /* __TIMESYNC_H__ */

