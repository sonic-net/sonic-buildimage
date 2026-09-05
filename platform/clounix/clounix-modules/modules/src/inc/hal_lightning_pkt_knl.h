/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  hal_lightning_pkt_knl.h
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 */

#ifndef HAL_LIGHTNING_PKT_KNL_H
#define HAL_LIGHTNING_PKT_KNL_H

#include <clx_error.h>
#include <netif_knl.h>

/* CP_COMMON */
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_EN_HI                 (0x000FC000)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_EN_LO                 (0x000FC004)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_LVL_HI                (0x000FC008)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_LVL_LO                (0x000FC00C)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_MASK_SET_HI           (0x000FC010)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_MASK_SET_LO           (0x000FC014)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_MASK_CLR_HI           (0x000FC018)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_MASK_CLR_LO           (0x000FC034)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_MASK_VAL_HI           (0x000FC020)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_MASK_VAL_LO           (0x000FC024)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_STAT_HI               (0x000FC028)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_STAT_LO               (0x000FC02C)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_CLR_HI                (0x000FC030)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_CLR_LO                (0x000FC034)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_SET_HI                (0x000FC038)
#define HAL_LIGHTNING_PKT_CP_COMMON_INT_SET_LO                (0x000FC03C)

/* PDMA */
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_STAT                   (0x003F1000)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_CLR                    (0x003F1004)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_EN                     (0x003F1010)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_LVL                    (0x003F1014)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_MASK_SET               (0x003F1018)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_MASK_CLR               (0x003F101C)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_MASK_VAL               (0x003F1020)
#define HAL_LIGHTNING_PKT_PDMA_ERR_INT_SET                    (0x003F1024)
#define HAL_LIGHTNING_PKT_PDMA_CREDIT_CFG                     (0x003F1100)

/* Rx */
#define HAL_LIGHTNING_PKT_PDMA_RCH_GPD_RING_START_ADDR_LO     (0x003F12E4)
#define HAL_LIGHTNING_PKT_PDMA_RCH_GPD_RING_START_ADDR_HI     (0x003F12E8)
#define HAL_LIGHTNING_PKT_PDMA_RCH_GPD_RING_SIZE              (0x003F12EC)
#define HAL_LIGHTNING_PKT_PDMA_RCH_CMD                        (0x003F1300)
#define HAL_LIGHTNING_PKT_PDMA_RCH_INT_EN                     (0x003F1360)
#define HAL_LIGHTNING_PKT_PDMA_RCH_INT_LVL                    (0x003F1364)
#define HAL_LIGHTNING_PKT_PDMA_RCH_INT_MASK                   (0x003F1368)
#define HAL_LIGHTNING_PKT_PDMA_RCH_INT_SET                    (0x003F1374)
#define HAL_LIGHTNING_PKT_PDMA_RCH_INT_CLR                    (0x003F1378)
#define HAL_LIGHTNING_PKT_PDMA_RCH_INT_STAT                   (0x003F1370)

/* Tx */
#define HAL_LIGHTNING_PKT_PDMA_TCH_GPD_RING_START_ADDR_LO     (0x003F1A00)
#define HAL_LIGHTNING_PKT_PDMA_TCH_GPD_RING_START_ADDR_HI     (0x003F1A04)
#define HAL_LIGHTNING_PKT_PDMA_TCH_GPD_RING_SIZE              (0x003F1A08)
#define HAL_LIGHTNING_PKT_PDMA_TCH_CMD                        (0x003F1A20)
#define HAL_LIGHTNING_PKT_PDMA_TCH_INT_EN                     (0x003F1A40)
#define HAL_LIGHTNING_PKT_PDMA_TCH_INT_LVL                    (0x003F1A44)
#define HAL_LIGHTNING_PKT_PDMA_TCH_INT_MASK                   (0x003F1A48)
#define HAL_LIGHTNING_PKT_PDMA_TCH_INT_SET                    (0x003F1A54)
#define HAL_LIGHTNING_PKT_PDMA_TCH_INT_CLR                    (0x003F1A58)
#define HAL_LIGHTNING_PKT_PDMA_TCH_INT_STAT                   (0x003F1A50)

#define HAL_LIGHTNING_PKT_GET_MMIO(__tbl__)                   (0x00FFFFFF & (__tbl__))
#define HAL_LIGHTNING_PKT_GET_PDMA_RCH_REG(__tbl__, __channel__)  ((__tbl__) + (0x200 * (__channel__)))
#define HAL_LIGHTNING_PKT_GET_PDMA_TCH_REG(__tbl__, __channel__)  ((__tbl__) + (0x100 * (__channel__)))

#define HAL_LIGHTNING_PORT_NUM                      (256)
#define HAL_LIGHTNING_PLANE_NUM                     (8)
#define HAL_LIGHTNING_PLANE_BITS                    (3)
#define HAL_LIGHTNING_PLANE_MASK                    (0x7)

/* NAMING DECLARATIONS
 */
/* PKT definitions */
#define HAL_LIGHTNING_PKT_TX_MAX_LEN              (9216)
#define HAL_LIGHTNING_PKT_RX_MAX_LEN              (9216 + 86) /* EPP tunnel header */
#define HAL_LIGHTNING_PKT_MIN_LEN                 (64)        /* Ethernet definition */
#define HAL_LIGHTNING_PKT_TMH_HDR_SZ              (20)
#define HAL_LIGHTNING_PKT_PPH_HDR_SZ              (20)
#define HAL_LIGHTNING_PKT_CRC_LEN                 (4)

/* CH */
#define HAL_LIGHTNING_PKT_CH_LAST_GPD             (0)
#define HAL_LIGHTNING_PKT_CH_MIDDLE_GPD           (1)

/* PRG */
#define HAL_LIGHTNING_PKT_PRG_PROCESS_GPD         (0) /* Normal   */
#define HAL_LIGHTNING_PKT_PRG_SKIP_GPD            (1) /* Skip     */

/* CRCC */
#define HAL_LIGHTNING_PKT_CRCC_SUM_BY_HW          (0) /* calculated by HW */
#define HAL_LIGHTNING_PKT_CRCC_SUM_BY_SW          (1) /* calculated by SW */

/* IOC */
#define HAL_LIGHTNING_PKT_IOC_NO_INTR             (0) /* trigger interrupt each GPD */
#define HAL_LIGHTNING_PKT_IOC_HAS_INTR            (1) /* trigger interrupt when ch=0, default setting */

/* HWO */
#define HAL_LIGHTNING_PKT_HWO_SW_OWN              (0)
#define HAL_LIGHTNING_PKT_HWO_HW_OWN              (1)

/* ECC */
#define HAL_LIGHTNING_PKT_ECC_ERROR_OCCUR         (1)

/* CPU, CPI queue number */
#define HAL_LIGHTNING_PKT_CPU_QUE_NUM             (48)
#define HAL_LIGHTNING_PKT_CPI_QUE_NUM             (8)

/* PDMA Definitions */
#define HAL_LIGHTNING_PKT_PDMA_MAX_GPD_PER_PKT    (10)   /* <= 256   */
#define HAL_LIGHTNING_PKT_PDMA_TX_INTR_TIMEOUT    (10 * 1000) /* us */
#define HAL_LIGHTNING_PKT_PDMA_TX_POLL_MAX_LOOP   (10 * 1000) /* int */

/* Mode */
#define HAL_LIGHTNING_PKT_TX_WAIT_MODE            (HAL_LIGHTNING_PKT_TX_WAIT_ASYNC)

/* TX Queue */
#define HAL_LIGHTNING_PKT_TX_TASK_MAX_LOOP        (HAL_DFLT_CFG_PKT_TX_QUEUE_LEN)

/* RX Queue */
#define HAL_LIGHTNING_PKT_RX_QUEUE_NUM            (HAL_LIGHTNING_PKT_RX_CHANNEL_LAST)
#define HAL_LIGHTNING_PKT_RX_TASK_MAX_LOOP        (HAL_DFLT_CFG_PKT_RX_QUEUE_LEN)

/* MACRO FUNCTION DECLARATIONS
 */
/*---------------------------------------------------------------------------*/
/* [CL8570] Alignment to 64-bytes */
#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define HAL_LIGHTNING_PKT_PDMA_ALIGN_ADDR(pdma_addr, align_sz) (((pdma_addr) + (align_sz)) & 0xFFFFFFFFFFFFFFC0)
#else
#define HAL_LIGHTNING_PKT_PDMA_ALIGN_ADDR(pdma_addr, align_sz) (((pdma_addr) + (align_sz)) & 0xFFFFFFC0)
#endif
/*---------------------------------------------------------------------------*/
#if defined(CLX_EN_BIG_ENDIAN)
#define HAL_LIGHTNING_PKT_ENDIAN_SWAP32(val)  (val)
#else
#define HAL_LIGHTNING_PKT_ENDIAN_SWAP32(val)  CMLIB_UTIL_ENDIAN_SWAP32(val)
#endif
/*---------------------------------------------------------------------------*/
#define HAL_LIGHTNING_PKT_GET_BIT(flags, bit)             ((((flags) & (bit)) > 0)? 1 : 0)
/*---------------------------------------------------------------------------*/
#define HAL_LIGHTNING_PKT_SET_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) | (mask_bitmap)))
#define HAL_LIGHTNING_PKT_CLR_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) & (~(mask_bitmap))))
/*---------------------------------------------------------------------------*/
#define HAL_LIGHTNING_PKT_GET_TX_INTR_TYPE(channel)       (HAL_INTR_TX_CH0 + channel)
#define HAL_LIGHTNING_PKT_GET_RX_INTR_TYPE(channel)       (HAL_INTR_RX_CH0 + channel)

/* DATA TYPE DECLARATIONS
 */
typedef enum
{
    HAL_LIGHTNING_PKT_TX_WAIT_ASYNC     = 0,
    HAL_LIGHTNING_PKT_TX_WAIT_SYNC_INTR = 1,
    HAL_LIGHTNING_PKT_TX_WAIT_SYNC_POLL = 2

} HAL_LIGHTNING_PKT_TX_WAIT_T;

typedef enum
{
    HAL_LIGHTNING_PKT_RX_SCHED_RR       = 0,
    HAL_LIGHTNING_PKT_RX_SCHED_WRR      = 1

} HAL_LIGHTNING_PKT_RX_SCHED_T;

/* GPD and Packet Strucutre Definition */
#if defined(CLX_EN_BIG_ENDIAN)

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  typ                           : 2;
    UI32_T  tc                            : 4;
    UI32_T  color                         : 2;
    UI32_T  srv                           : 3;
    UI32_T  trig                          : 1;
    UI32_T  igr_phy_port                  :12;
    UI32_T  hsh_val_w0                    : 8;
    /* CLX DWORD 1 */
    UI32_T  hsh_val_w1                    : 2;
    UI32_T  dst_idx                       :15;
    UI32_T  src_idx                       :15;
    /* CLX DWORD 2 */
    UI32_T  intf_fdid                     :14;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T                                : 2;
    UI32_T  ecn                           : 2;
    UI32_T  store_and_forward             : 1;
    UI32_T  lag_epoch                     : 1;
    UI32_T  src_supp_tag                  : 5;
    UI32_T                                : 2;
    UI32_T  skip_ipp                      : 1;
    UI32_T  igr_fab_port_grp              : 1;
    /* CLX DWORD 3 */
    UI32_T                                :32;
    /* CLX DWORD 4 */
    UI32_T                                :20;
    UI32_T  had_uturn                     : 1;
    UI32_T  fab_stacking_supp             : 2;
    UI32_T  chip_igr_lag_port             : 8;
    UI32_T  exp_dscp_mrkd                 : 1;
} HAL_LIGHTNING_PKT_ITMH_FAB_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  typ                           : 2;
    UI32_T  tc                            : 4;
    UI32_T  color                         : 2;
    UI32_T  srv                           : 3;
    UI32_T  trig                          : 1;
    UI32_T  igr_phy_port                  :12;
    UI32_T  hsh_val_w0                    : 8;
    /* CLX DWORD 1 */
    UI32_T  hsh_val_w1                    : 2;
    UI32_T  dst_idx                       :15;
    UI32_T  src_idx                       :15;
    /* CLX DWORD 2 */
    UI32_T  intf_fdid                     :14;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T  nvo3_ip_tnl_decap_prop_ttl    : 1;
    UI32_T  nvo3_mpls_uhp_prop_ttl        : 1;
    UI32_T  ecn                           : 2;
    UI32_T  store_and_forward             : 1;
    UI32_T  lag_epoch                     : 1;
    UI32_T  src_supp_tag                  : 5;
    UI32_T  one_arm_rte_srv_fdid          : 1;
    UI32_T  fab_one_arm_rte               : 1;
    UI32_T  skip_ipp                      : 1;
    UI32_T  igr_fab_port_grp              : 1;
    /* CLX DWORD 3 */
    UI32_T                                : 2;
    UI32_T  nvo3_mgid                     :15;
    UI32_T  nvo3_intf                     :14;
    UI32_T  nvo3_src_supp_tag_w0          : 1;
    /* CLX DWORD 4 */
    UI32_T  nvo3_src_supp_tag_w1          : 4;
    UI32_T  mir_bmap                      : 8;
    UI32_T  cp_to_cpu_code                : 4;
    UI32_T  cp_to_cpu_bmap                :16;
} HAL_LIGHTNING_PKT_ITMH_ETH_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  typ                           : 2;
    UI32_T  tc                            : 4;
    UI32_T  color                         : 2;
    UI32_T  srv                           : 3;
    UI32_T  trig                          : 1;
    UI32_T  igr_phy_port                  :12;
    UI32_T  hsh_val_w0                    : 8;
    /* CLX DWORD 1 */
    UI32_T  hsh_val_w1                    : 2;
    UI32_T  dst_idx                       :15;
    UI32_T  src_idx                       :15;
    /* CLX DWORD 2 */
    UI32_T  intf_fdid                     :14;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T  nvo3_ip_tnl_decap_prop_ttl    : 1;
    UI32_T  nvo3_mpls_uhp_prop_ttl        : 1;
    UI32_T  ecn                           : 2;
    UI32_T  store_and_forward             : 1;
    UI32_T  lag_epoch                     : 1;
    UI32_T  src_supp_tag                  : 5;
    UI32_T  one_arm_rte_srv_fdid          : 1;
    UI32_T  fab_one_arm_rte               : 1;
    UI32_T                                : 2;
    /* CLX DWORD 3 */
    UI32_T                                :32;
    /* CLX DWORD 4 */
    UI32_T                                :20;
    UI32_T  had_uturn                     : 1;
    UI32_T                                : 2;
    UI32_T  excpt_code                    : 8;
    UI32_T  exp_dscp_mrkd                 : 1;
} HAL_LIGHTNING_PKT_ETMH_FAB_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  typ                           : 2;
    UI32_T  tc                            : 4;
    UI32_T  color                         : 2;
    UI32_T  srv                           : 3;
    UI32_T  trig                          : 1;
    UI32_T  igr_phy_port                  :12;
    UI32_T  hsh_val_w0                    : 8;
    /* CLX DWORD 1 */
    UI32_T  hsh_val_w1                    : 2;
    UI32_T  dst_idx                       :15;
    UI32_T  src_idx                       :15;
    /* CLX DWORD 2 */
    UI32_T  intf_fdid                     :14;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T  nvo3_ip_tnl_decap_prop_ttl    : 1;
    UI32_T  nvo3_mpls_uhp_prop_ttl        : 1;
    UI32_T  ecn                           : 2;
    UI32_T  igr_fab_port_grp              : 1;
    UI32_T  redir                         : 1;
    UI32_T  excpt_code_mir_bmap           : 8;
    UI32_T  cp_to_cpu_bmap_w0             : 1;
    /* CLX DWORD 3 */
    UI32_T  cp_to_cpu_bmap_w1             : 7;
    UI32_T  egr_phy_port                  :12;
    UI32_T  src_supp_pnd                  : 1;
    UI32_T  mc_vid_ctl                    : 3;
    UI32_T  mc_vid_1st_w0                 : 9;
    /* CLX DWORD 4 */
    UI32_T  mc_vid_1st_w1                 : 3;
    UI32_T  mc_vid_2nd                    :12;
    UI32_T  mc_decr_ttl                   : 1;
    UI32_T  mc_is_routed                  : 1;
    UI32_T  mc_mel_vld                    : 1;
    UI32_T  mc_cp_idx                     :13;
    UI32_T  exp_dscp_mrkd                 : 1;
} HAL_LIGHTNING_PKT_ETMH_ETH_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  decap_act                     : 3;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  mpls_pw_cw_vld                : 1;
    UI32_T  hit_idx_w0                    :12;
    /* CLX DWORD 1 */
    UI32_T  hit_idx_w1                    : 7;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  seg_vmid_w0                   :17;
    /* CLX DWORD 2 */
    UI32_T  seg_vmid_w1                   : 7;
    UI32_T                                : 1;
    UI32_T  l2_sa_lrn_en_hw_cvs           : 1;
    UI32_T  l2_sa_lrn_en_hw               : 1;
    UI32_T  vid_ctl                       : 3;
    UI32_T  vid_1st                       :12;
    UI32_T  vid_2nd_w0                    : 7;
    /* CLX DWORD 3 */
    UI32_T  vid_2nd_w1                    : 5;
    UI32_T  flw_lbl                       :10;
    UI32_T  rewr_idx_ctl                  : 2;
    UI32_T  rewr_idx_0                    :13;
    UI32_T  rewr_idx_1_w0                 : 2;
    /* CLX DWORD 4 */
    UI32_T  rewr_idx_1_w1                 :11;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  ts_0_7                        : 8;
    UI32_T  ts_8_15                       : 8;
} HAL_LIGHTNING_PKT_PPH_L2_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  decap_act                     : 3;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  mpls_pw_cw_vld                : 1;
    UI32_T  hit_idx_w0                    :12;
    /* CLX DWORD 1 */
    UI32_T  hit_idx_w1                    : 7;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  seg_vmid_w0                   :17;
    /* CLX DWORD 2 */
    UI32_T  seg_vmid_w1                   : 7;
    UI32_T                                : 1;
    UI32_T  rpf_pnd                       : 1;
    UI32_T  adj_idx                       :18;
    UI32_T  is_mc                         : 1;
    UI32_T  decr_ttl                      : 1;
    UI32_T  decap_prop_ttl                : 1;
    UI32_T  mrk_dscp_en                   : 1;
    UI32_T  mrk_dscp_val_w0               : 1;
    /* CLX DWORD 3 */
    UI32_T  mrk_dscp_val_w1               : 5;
    UI32_T  flw_lbl                       :10;
    UI32_T  rewr_idx_ctl                  : 2;
    UI32_T  rewr_idx_0                    :13;
    UI32_T  rewr_idx_1_w0                 : 2;
    /* CLX DWORD 4 */
    UI32_T  rewr_idx_1_w1                 :11;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  ts_0_7                        : 8;
    UI32_T  ts_8_15                       : 8;
} HAL_LIGHTNING_PKT_PPH_L3UC_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  decap_act                     : 3;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  mpls_pw_cw_vld                : 1;
    UI32_T  hit_idx_w0                    :12;
    /* CLX DWORD 1 */
    UI32_T  hit_idx_w1                    : 7;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  vid_1st                       :12;
    UI32_T  vid_2nd_w0                    : 5;
    /* CLX DWORD 2 */
    UI32_T  vid_2nd_w1                    : 7;
    UI32_T                                :15;
    UI32_T  l2_sa_lrn_en_hw_cvs           : 1;
    UI32_T  l2_sa_lrn_en_hw               : 1;
    UI32_T  vid_ctl                       : 3;
    UI32_T  is_mc                         : 1;
    UI32_T                                : 1;
    UI32_T  decap_prop_ttl                : 1;
    UI32_T  mrk_dscp_en                   : 1;
    UI32_T  mrk_dscp_val_w0               : 1;
    /* CLX DWORD 3 */
    UI32_T  mrk_dscp_val_w1               : 5;
    UI32_T  flw_lbl                       :10;
    UI32_T  rewr_idx_ctl                  : 2;
    UI32_T  rewr_idx_0                    :13;
    UI32_T  rewr_idx_1_w0                 : 2;
    /* CLX DWORD 4 */
    UI32_T  rewr_idx_1_w1                 :11;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  ts_0_7                        : 8;
    UI32_T  ts_8_15                       : 8;
} HAL_LIGHTNING_PKT_PPH_L3MC_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  decap_act                     : 3;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T                                : 1;
    UI32_T  hit_idx_w0                    :12;
    /* CLX DWORD 1 */
    UI32_T  hit_idx_w1                    : 7;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  seg_vmid_w0                   :17;
    /* CLX DWORD 2 */
    UI32_T  seg_vmid_w1                   : 7;
    UI32_T                                : 2;
    UI32_T  adj_idx                       :18;
    UI32_T                                : 1;
    UI32_T  decr_ttl                      : 1;
    UI32_T  decap_prop_ttl                : 1;
    UI32_T  mrk_exp_en                    : 1;
    UI32_T                                : 1;
    /* CLX DWORD 3 */
    UI32_T                                : 2;
    UI32_T  mrk_exp_val                   : 3;
    UI32_T  php_pop_keep_inner_qos        : 1;
    UI32_T                                :26;
    /* CLX DWORD 4 */
    UI32_T                                :11;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  ts_0_7                        : 8;
    UI32_T  ts_8_15                       : 8;
} HAL_LIGHTNING_PKT_PPH_L25_T;

#elif defined(CLX_EN_LITTLE_ENDIAN)

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hsh_val_w0                    : 8;
    UI32_T  igr_phy_port                  :12;
    UI32_T  trig                          : 1;
    UI32_T  srv                           : 3;
    UI32_T  color                         : 2;
    UI32_T  tc                            : 4;
    UI32_T  typ                           : 2;
    /* CLX DWORD 1 */
    UI32_T  src_idx                       :15;
    UI32_T  dst_idx                       :15;
    UI32_T  hsh_val_w1                    : 2;
    /* CLX DWORD 2 */
    UI32_T  igr_fab_port_grp              : 1;
    UI32_T  skip_ipp                      : 1;
    UI32_T                                : 2;
    UI32_T  src_supp_tag                  : 5;
    UI32_T  lag_epoch                     : 1;
    UI32_T  store_and_forward             : 1;
    UI32_T  ecn                           : 2;
    UI32_T                                : 2;
    UI32_T  steer_applied                 : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  intf_fdid                     :14;
    /* CLX DWORD 3 */
    UI32_T                                :32;
    /* CLX DWORD 4 */
    UI32_T  exp_dscp_mrkd                 : 1;
    UI32_T  chip_igr_lag_port             : 8;
    UI32_T  fab_stacking_supp             : 2;
    UI32_T  had_uturn                     : 1;
    UI32_T                                :20;
} HAL_LIGHTNING_PKT_ITMH_FAB_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hsh_val_w0                    : 8;
    UI32_T  igr_phy_port                  :12;
    UI32_T  trig                          : 1;
    UI32_T  srv                           : 3;
    UI32_T  color                         : 2;
    UI32_T  tc                            : 4;
    UI32_T  typ                           : 2;
    /* CLX DWORD 1 */
    UI32_T  src_idx                       :15;
    UI32_T  dst_idx                       :15;
    UI32_T  hsh_val_w1                    : 2;
    /* CLX DWORD 2 */
    UI32_T  igr_fab_port_grp              : 1;
    UI32_T  skip_ipp                      : 1;
    UI32_T  fab_one_arm_rte               : 1;
    UI32_T  one_arm_rte_srv_fdid          : 1;
    UI32_T  src_supp_tag                  : 5;
    UI32_T  lag_epoch                     : 1;
    UI32_T  store_and_forward             : 1;
    UI32_T  ecn                           : 2;
    UI32_T  nvo3_mpls_uhp_prop_ttl        : 1;
    UI32_T  nvo3_ip_tnl_decap_prop_ttl    : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  intf_fdid                     :14;
    /* CLX DWORD 3 */
    UI32_T  nvo3_src_supp_tag_w0          : 1;
    UI32_T  nvo3_intf                     :14;
    UI32_T  nvo3_mgid                     :15;
    UI32_T                                : 2;
    /* CLX DWORD 4 */
    UI32_T  cp_to_cpu_bmap                :16;
    UI32_T  cp_to_cpu_code                : 4;
    UI32_T  mir_bmap                      : 8;
    UI32_T  nvo3_src_supp_tag_w1          : 4;
} HAL_LIGHTNING_PKT_ITMH_ETH_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hsh_val_w0                    : 8;
    UI32_T  igr_phy_port                  :12;
    UI32_T  trig                          : 1;
    UI32_T  srv                           : 3;
    UI32_T  color                         : 2;
    UI32_T  tc                            : 4;
    UI32_T  typ                           : 2;
    /* CLX DWORD 1 */
    UI32_T  src_idx                       :15;
    UI32_T  dst_idx                       :15;
    UI32_T  hsh_val_w1                    : 2;
    /* CLX DWORD 2 */
    UI32_T                                : 2;
    UI32_T  fab_one_arm_rte               : 1;
    UI32_T  one_arm_rte_srv_fdid          : 1;
    UI32_T  src_supp_tag                  : 5;
    UI32_T  lag_epoch                     : 1;
    UI32_T  store_and_forward             : 1;
    UI32_T  ecn                           : 2;
    UI32_T  nvo3_mpls_uhp_prop_ttl        : 1;
    UI32_T  nvo3_ip_tnl_decap_prop_ttl    : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  intf_fdid                     :14;
    /* CLX DWORD 3 */
    UI32_T                                :32;
    /* CLX DWORD 4 */
    UI32_T  exp_dscp_mrkd                 : 1;
    UI32_T  excpt_code                    : 8;
    UI32_T                                : 2;
    UI32_T  had_uturn                     : 1;
    UI32_T                                :20;
} HAL_LIGHTNING_PKT_ETMH_FAB_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hsh_val_w0                    : 8;
    UI32_T  igr_phy_port                  :12;
    UI32_T  trig                          : 1;
    UI32_T  srv                           : 3;
    UI32_T  color                         : 2;
    UI32_T  tc                            : 4;
    UI32_T  typ                           : 2;
    /* CLX DWORD 1 */
    UI32_T  src_idx                       :15;
    UI32_T  dst_idx                       :15;
    UI32_T  hsh_val_w1                    : 2;
    /* CLX DWORD 2 */
    UI32_T  cp_to_cpu_bmap_w0             : 1;
    UI32_T  excpt_code_mir_bmap           : 8;
    UI32_T  redir                         : 1;
    UI32_T  igr_fab_port_grp              : 1;
    UI32_T  ecn                           : 2;
    UI32_T  nvo3_mpls_uhp_prop_ttl        : 1;
    UI32_T  nvo3_ip_tnl_decap_prop_ttl    : 1;
    UI32_T  steer_applied                 : 1;
    UI32_T  skip_epp                      : 1;
    UI32_T  nvo3_mgid_is_transit          : 1;
    UI32_T  intf_fdid                     :14;
    /* CLX DWORD 3 */
    UI32_T  mc_vid_1st_w0                 : 9;
    UI32_T  mc_vid_ctl                    : 3;
    UI32_T  src_supp_pnd                  : 1;
    UI32_T  egr_phy_port                  :12;
    UI32_T  cp_to_cpu_bmap_w1             : 7;
    /* CLX DWORD 4 */
    UI32_T  exp_dscp_mrkd                 : 1;
    UI32_T  mc_cp_idx                     :13;
    UI32_T  mc_mel_vld                    : 1;
    UI32_T  mc_is_routed                  : 1;
    UI32_T  mc_decr_ttl                   : 1;
    UI32_T  mc_vid_2nd                    :12;
    UI32_T  mc_vid_1st_w1                 : 3;
} HAL_LIGHTNING_PKT_ETMH_ETH_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hit_idx_w0                    :12;
    UI32_T  mpls_pw_cw_vld                : 1;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  decap_act                     : 3;
    /* CLX DWORD 1 */
    UI32_T  seg_vmid_w0                   :17;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  hit_idx_w1                    : 7;
    /* CLX DWORD 2 */
    UI32_T  vid_2nd_w0                    : 7;
    UI32_T  vid_1st                       :12;
    UI32_T  vid_ctl                       : 3;
    UI32_T  l2_sa_lrn_en_hw               : 1;
    UI32_T  l2_sa_lrn_en_hw_cvs           : 1;
    UI32_T                                : 1;
    UI32_T  seg_vmid_w1                   : 7;
    /* CLX DWORD 3 */
    UI32_T  rewr_idx_1_w0                 : 2;
    UI32_T  rewr_idx_0                    :13;
    UI32_T  rewr_idx_ctl                  : 2;
    UI32_T  flw_lbl                       :10;
    UI32_T  vid_2nd_w1                    : 5;
    /* CLX DWORD 4 */
    UI32_T  ts_8_15                       : 8;
    UI32_T  ts_0_7                        : 8;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  rewr_idx_1_w1                 :11;
} HAL_LIGHTNING_PKT_PPH_L2_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hit_idx_w0                    :12;
    UI32_T  mpls_pw_cw_vld                : 1;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  decap_act                     : 3;
    /* CLX DWORD 1 */
    UI32_T  seg_vmid_w0                   :17;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  hit_idx_w1                    : 7;
    /* CLX DWORD 2 */
    UI32_T  mrk_dscp_val_w0               : 1;
    UI32_T  mrk_dscp_en                   : 1;
    UI32_T  decap_prop_ttl                : 1;
    UI32_T  decr_ttl                      : 1;
    UI32_T  is_mc                         : 1;
    UI32_T  adj_idx                       :18;
    UI32_T  rpf_pnd                       : 1;
    UI32_T                                : 1;
    UI32_T  seg_vmid_w1                   : 7;
    /* CLX DWORD 3 */
    UI32_T  rewr_idx_1_w0                 : 2;
    UI32_T  rewr_idx_0                    :13;
    UI32_T  rewr_idx_ctl                  : 2;
    UI32_T  flw_lbl                       :10;
    UI32_T  mrk_dscp_val_w1               : 5;
    /* CLX DWORD 4 */
    UI32_T  ts_8_15                       : 8;
    UI32_T  ts_0_7                        : 8;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  rewr_idx_1_w1                 :11;
} HAL_LIGHTNING_PKT_PPH_L3UC_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hit_idx_w0                    :12;
    UI32_T  mpls_pw_cw_vld                : 1;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  decap_act                     : 3;
    /* CLX DWORD 1 */
    UI32_T  vid_2nd_w0                    : 5;
    UI32_T  vid_1st                       :12;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  hit_idx_w1                    : 7;
    /* CLX DWORD 2 */
    UI32_T  mrk_dscp_val_w0               : 1;
    UI32_T  mrk_dscp_en                   : 1;
    UI32_T  decap_prop_ttl                : 1;
    UI32_T                                : 1;
    UI32_T  is_mc                         : 1;
    UI32_T  vid_ctl                       : 3;
    UI32_T  l2_sa_lrn_en_hw               : 1;
    UI32_T  l2_sa_lrn_en_hw_cvs           : 1;
    UI32_T                                :15;
    UI32_T  vid_2nd_w1                    : 7;
    /* CLX DWORD 3 */
    UI32_T  rewr_idx_1_w0                 : 2;
    UI32_T  rewr_idx_0                    :13;
    UI32_T  rewr_idx_ctl                  : 2;
    UI32_T  flw_lbl                       :10;
    UI32_T  mrk_dscp_val_w1               : 5;
    /* CLX DWORD 4 */
    UI32_T  ts_8_15                       : 8;
    UI32_T  ts_0_7                        : 8;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T  rewr_idx_1_w1                 :11;
} HAL_LIGHTNING_PKT_PPH_L3MC_T;

typedef struct
{
    /* CLX DWORD 0 */
    UI32_T  hit_idx_w0                    :12;
    UI32_T                                : 1;
    UI32_T  nvo3_encap_idx                :14;
    UI32_T  igr_l2_vid_num                : 2;
    UI32_T  decap_act                     : 3;
    /* CLX DWORD 1 */
    UI32_T  seg_vmid_w0                   :17;
    UI32_T  nvo3_adj_idx                  : 8;
    UI32_T  hit_idx_w1                    : 7;
    /* CLX DWORD 2 */
    UI32_T                                : 1;
    UI32_T  mrk_exp_en                    : 1;
    UI32_T  decap_prop_ttl                : 1;
    UI32_T  decr_ttl                      : 1;
    UI32_T                                : 1;
    UI32_T  adj_idx                       :18;
    UI32_T                                : 2;
    UI32_T  seg_vmid_w1                   : 7;
    /* CLX DWORD 3 */
    UI32_T                                :26;
    UI32_T  php_pop_keep_inner_qos        : 1;
    UI32_T  mrk_exp_val                   : 3;
    UI32_T                                : 2;
    /* CLX DWORD 4 */
    UI32_T  ts_8_15                       : 8;
    UI32_T  ts_0_7                        : 8;
    UI32_T  mrk_dei_val                   : 1;
    UI32_T  mrk_pcp_val                   : 3;
    UI32_T  mrk_pcp_dei_en                : 1;
    UI32_T                                :11;
} HAL_LIGHTNING_PKT_PPH_L25_T;

#else
#error "Host GPD endian is not defined!!\n"
#endif

#if defined(CLX_EN_BIG_ENDIAN)

/* RX GPD STRUCTURE */
typedef struct
{
    UI32_T  data_buf_addr_lo;
    UI32_T  data_buf_addr_hi;
    UI32_T  chksum              : 16;
    UI32_T  ioc                 :  1;
    UI32_T                      :  1;
    UI32_T  avbl_buf_len        : 14;
    UI32_T                      : 32;

    union
    {
        HAL_LIGHTNING_PKT_ITMH_FAB_T  itmh_fab;
        HAL_LIGHTNING_PKT_ITMH_ETH_T  itmh_eth;
        HAL_LIGHTNING_PKT_ETMH_FAB_T  etmh_fab;
        HAL_LIGHTNING_PKT_ETMH_ETH_T  etmh_eth;
    };
    union
    {
        HAL_LIGHTNING_PKT_PPH_L2_T    pph_l2;
        HAL_LIGHTNING_PKT_PPH_L3UC_T  pph_l3uc;
        HAL_LIGHTNING_PKT_PPH_L3MC_T  pph_l3mc;
        HAL_LIGHTNING_PKT_PPH_L25_T   pph_l25;
    };

    UI32_T  ts_16_23            :  8;
    UI32_T  ts_24_31            :  8;
    UI32_T                      :  6;
    UI32_T  ts_32_33            :  2;
    UI32_T                      :  8;
    UI32_T  hwo                 :  1;
    UI32_T  ch                  :  1;
    UI32_T  trn                 :  1;
    UI32_T  ecce                :  1;
    UI32_T  errf                :  1;
    UI32_T                      :  5;
    UI32_T  queue               :  6;
    UI32_T                      :  2;
    UI32_T  cnsm_buf_len        : 14;

} HAL_LIGHTNING_PKT_RX_GPD_T;

/* TX GPD STRUCTURE */
typedef struct
{
    UI32_T  data_buf_addr_lo;
    UI32_T  data_buf_addr_hi;
    UI32_T  chksum              : 16;
    UI32_T  ioc                 :  1;
    UI32_T                      :  1;
    UI32_T  data_buf_size       : 14;
    UI32_T                      : 32;

    union
    {
        HAL_LIGHTNING_PKT_ITMH_FAB_T  itmh_fab;
        HAL_LIGHTNING_PKT_ITMH_ETH_T  itmh_eth;
        HAL_LIGHTNING_PKT_ETMH_FAB_T  etmh_fab;
        HAL_LIGHTNING_PKT_ETMH_ETH_T  etmh_eth;
    };
    union
    {
        HAL_LIGHTNING_PKT_PPH_L2_T    pph_l2;
        HAL_LIGHTNING_PKT_PPH_L3UC_T  pph_l3uc;
        HAL_LIGHTNING_PKT_PPH_L3MC_T  pph_l3mc;
        HAL_LIGHTNING_PKT_PPH_L25_T   pph_l25;
    };

    UI32_T                      : 16;
    UI32_T  ptp_hdr             : 16;
    UI32_T  hwo                 :  1;
    UI32_T  ch                  :  1;
    UI32_T                      :  1;
    UI32_T  ecce                :  1;
    UI32_T  crce                :  1;
    UI32_T                      :  3;
    UI32_T  cos                 :  3;
    UI32_T  phc                 :  1;   /* PTP Header Control */
    UI32_T  ipc                 :  3;   /* Ingress Plane Control */
    UI32_T  prg                 :  1;   /* Purge */
    UI32_T                      :  2;
    UI32_T  pkt_len             : 14;   /* Total packet length */

} HAL_LIGHTNING_PKT_TX_GPD_T;

#elif defined(CLX_EN_LITTLE_ENDIAN)

/* RX GPD STRUCTURE */
typedef struct
{
    UI32_T  data_buf_addr_lo;
    UI32_T  data_buf_addr_hi;
    UI32_T  avbl_buf_len        : 14;
    UI32_T                      :  1;
    UI32_T  ioc                 :  1;
    UI32_T  chksum              : 16;
    UI32_T                      : 32;

    union
    {
        HAL_LIGHTNING_PKT_ITMH_FAB_T  itmh_fab;
        HAL_LIGHTNING_PKT_ITMH_ETH_T  itmh_eth;
        HAL_LIGHTNING_PKT_ETMH_FAB_T  etmh_fab;
        HAL_LIGHTNING_PKT_ETMH_ETH_T  etmh_eth;
    };
    union
    {
        HAL_LIGHTNING_PKT_PPH_L2_T    pph_l2;
        HAL_LIGHTNING_PKT_PPH_L3UC_T  pph_l3uc;
        HAL_LIGHTNING_PKT_PPH_L3MC_T  pph_l3mc;
        HAL_LIGHTNING_PKT_PPH_L25_T   pph_l25;
    };

    UI32_T                      :  8;
    UI32_T  ts_32_33            :  2;
    UI32_T                      :  6;
    UI32_T  ts_24_31            :  8;
    UI32_T  ts_16_23            :  8;
    UI32_T  cnsm_buf_len        : 14;
    UI32_T                      :  2;
    UI32_T  queue               :  6;
    UI32_T                      :  5;
    UI32_T  errf                :  1;
    UI32_T  ecce                :  1;
    UI32_T  trn                 :  1;
    UI32_T  ch                  :  1;
    UI32_T  hwo                 :  1;

} HAL_LIGHTNING_PKT_RX_GPD_T;

/* TX GPD STRUCTURE */
typedef struct
{
    UI32_T  data_buf_addr_lo;
    UI32_T  data_buf_addr_hi;
    UI32_T  data_buf_size       : 14;
    UI32_T                      :  1;
    UI32_T  ioc                 :  1;
    UI32_T  chksum              : 16;
    UI32_T                      : 32;

    union
    {
        HAL_LIGHTNING_PKT_ITMH_FAB_T  itmh_fab;
        HAL_LIGHTNING_PKT_ITMH_ETH_T  itmh_eth;
        HAL_LIGHTNING_PKT_ETMH_FAB_T  etmh_fab;
        HAL_LIGHTNING_PKT_ETMH_ETH_T  etmh_eth;
    };
    union
    {
        HAL_LIGHTNING_PKT_PPH_L2_T    pph_l2;
        HAL_LIGHTNING_PKT_PPH_L3UC_T  pph_l3uc;
        HAL_LIGHTNING_PKT_PPH_L3MC_T  pph_l3mc;
        HAL_LIGHTNING_PKT_PPH_L25_T   pph_l25;
    };

    UI32_T  ptp_hdr             : 16;
    UI32_T                      : 16;
    UI32_T  pkt_len             : 14;   /* Total packet length */
    UI32_T                      :  2;
    UI32_T  prg                 :  1;   /* Purge */
    UI32_T  ipc                 :  3;   /* Ingress Plane Control */
    UI32_T  phc                 :  1;   /* PTP Header Control */
    UI32_T  cos                 :  3;
    UI32_T                      :  3;
    UI32_T  crce                :  1;
    UI32_T  ecce                :  1;
    UI32_T                      :  1;
    UI32_T  ch                  :  1;
    UI32_T  hwo                 :  1;
} HAL_LIGHTNING_PKT_TX_GPD_T;

#else
#error "Host GPD endian is not defined\n"
#endif

/* ----------------------------------------------------------------------------------- PP Type */
typedef enum
{
    HAL_LIGHTNING_PKT_TMH_TYPE_ITMH_ETH = 0,
    HAL_LIGHTNING_PKT_TMH_TYPE_ITMH_FAB,
    HAL_LIGHTNING_PKT_TMH_TYPE_ETMH_FAB,
    HAL_LIGHTNING_PKT_TMH_TYPE_ETMH_ETH,
    HAL_LIGHTNING_PKT_TMH_TYPE_LAST

} HAL_LIGHTNING_PKT_TMH_TYPE_T;

typedef enum
{
    HAL_LIGHTNING_PKT_TMH_SRV_L2 = 0,
    HAL_LIGHTNING_PKT_TMH_SRV_L25_MPLS,
    HAL_LIGHTNING_PKT_TMH_SRV_L3,
    HAL_LIGHTNING_PKT_TMH_SRV_EGR,            /* L3 downgrade L2 */
    HAL_LIGHTNING_PKT_TMH_SRV_L25_NSH,
    HAL_LIGHTNING_PKT_TMH_SRV_L25_TRILL,
    HAL_LIGHTNING_PKT_TMH_SRV_LAST

} HAL_LIGHTNING_PKT_TMH_SRV_T;

typedef enum
{
    HAL_LIGHTNING_PKT_TMH_DECAP_NONE = 0,
    HAL_LIGHTNING_PKT_TMH_DECAP_1_MPLS_LABEL,
    HAL_LIGHTNING_PKT_TMH_DECAP_2_MPLS_LABEL,
    HAL_LIGHTNING_PKT_TMH_DECAP_3_MPLS_LABEL,
    HAL_LIGHTNING_PKT_TMH_DECAP_4_MPLS_LABEL,
    HAL_LIGHTNING_PKT_TMH_DECAP_IP_TRILL_NSH,
    HAL_LIGHTNING_PKT_TMH_DECAP_LAST

} HAL_LIGHTNING_PKT_TMH_DECAP_T;

typedef struct
{
    union
    {
        HAL_LIGHTNING_PKT_ITMH_FAB_T  itmh_fab;
        HAL_LIGHTNING_PKT_ITMH_ETH_T  itmh_eth;
        HAL_LIGHTNING_PKT_ETMH_FAB_T  etmh_fab;
        HAL_LIGHTNING_PKT_ETMH_ETH_T  etmh_eth;
    };
} HAL_LIGHTNING_PKT_TMH_T;

typedef struct
{
    union
    {
        HAL_LIGHTNING_PKT_PPH_L2_T    pph_l2;
        HAL_LIGHTNING_PKT_PPH_L3UC_T  pph_l3uc;
        HAL_LIGHTNING_PKT_PPH_L3MC_T  pph_l3mc;
        HAL_LIGHTNING_PKT_PPH_L25_T   pph_l25;
    };
} HAL_LIGHTNING_PKT_PPH_T;

/* ----------------------------------------------------------------------------------- Reg Type */
typedef enum
{
    HAL_LIGHTNING_PKT_L2_ISR_RCH0            = (0x1UL << 0),
    HAL_LIGHTNING_PKT_L2_ISR_RCH1            = (0x1UL << 1),
    HAL_LIGHTNING_PKT_L2_ISR_RCH2            = (0x1UL << 2),
    HAL_LIGHTNING_PKT_L2_ISR_RCH3            = (0x1UL << 3),
    HAL_LIGHTNING_PKT_L2_ISR_TCH0            = (0x1UL << 4),
    HAL_LIGHTNING_PKT_L2_ISR_TCH1            = (0x1UL << 5),
    HAL_LIGHTNING_PKT_L2_ISR_TCH2            = (0x1UL << 6),
    HAL_LIGHTNING_PKT_L2_ISR_TCH3            = (0x1UL << 7),
    HAL_LIGHTNING_PKT_L2_ISR_RX_QID_MAP_ERR  = (0x1UL << 8),
    HAL_LIGHTNING_PKT_L2_ISR_RX_FRAME_ERR    = (0x1UL << 9)

} HAL_LIGHTNING_PKT_L2_ISR_T;

typedef enum
{
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_GPD_HWO_ERROR         = (0x1UL << 0),   /* Tx GPD.hwo = 0                         */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_GPD_CHKSM_ERROR       = (0x1UL << 1),   /* Tx GPD.chksm is error                  */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_GPD_NO_OVFL_ERROR     = (0x1UL << 2),   /* S/W push too much GPD                  */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_GPD_DMA_READ_ERROR    = (0x1UL << 3),   /* AXI Rd Error when do GPD read          */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_BUF_SIZE_ERROR        = (0x1UL << 4),   /* Tx GPD.data_buf_size = 0               */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_RUNT_ERROR            = (0x1UL << 5),   /* Tx GPD.pkt_len < 64                    */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_OVSZ_ERROR            = (0x1UL << 6),   /* Tx GPD.pkt_len = 9217                  */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_LEN_MISMATCH_ERROR    = (0x1UL << 7),   /* Tx GPD.pkt_len != sum of data_buf_size */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_PKTPL_DMA_READ_ERROR  = (0x1UL << 8),   /* AXI Rd Error when do Payload read      */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_COS_ERROR             = (0x1UL << 9),   /* Tx GPD.cos is not match cos_to_tch_map */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_GPD_GT255_ERROR       = (0x1UL << 10),  /* Multi-GPD packet's GPD# > 255          */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_PFC                   = (0x1UL << 11),  /* */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_CREDIT_UDFL_ERROR     = (0x1UL << 12),  /* Credit Underflow (count down to 0)     */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_DMA_WRITE_ERROR       = (0x1UL << 13),  /* AXI Wr Error (GPD Write-Back)          */
    HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_STOP_CMD_CPLT         = (0x1UL << 14)

} HAL_LIGHTNING_PKT_TX_CHANNEL_L2_ISR_T;

typedef enum
{
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_AVAIL_GPD_LOW         = (0x1UL << 0),   /* Rx GPD.avbl_gpd_num < threshold        */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_AVAIL_GPD_EMPTY       = (0x1UL << 1),   /* Rx GPD.avbl_gpd_num = 0                */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_AVAIL_GPD_ERROR       = (0x1UL << 2),   /* Rx GPD.hwo = 0                         */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_GPD_CHKSM_ERROR       = (0x1UL << 3),   /* Rx GPD.chksm is error                  */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_DMA_READ_ERROR        = (0x1UL << 4),   /* DMAR error occurs in PCIE              */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_DMA_WRITE_ERROR       = (0x1UL << 5),   /* DMAW error occurs in PCIE              */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_STOP_CMD_CPLT         = (0x1UL << 6),   /* Stop Completion Acknowledge            */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_GPD_GT255_ERROR       = (0x1UL << 7),   /* Multi-GPD packet's GPD# > 255          */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_TOD_UNINIT            = (0x1UL << 8),   /* */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_PKT_ERROR_DROP        = (0x1UL << 9),   /* */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_UDSZ_DROP             = (0x1UL << 10),  /* */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_OVSZ_DROP             = (0x1UL << 11),  /* */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_CMDQ_OVF_DROP         = (0x1UL << 12),  /* */
    HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_FIFO_OVF_DROP         = (0x1UL << 13)

} HAL_LIGHTNING_PKT_RX_CHANNEL_L2_ISR_T;

typedef enum
{
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_IOC                      = (0x1UL << 0),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_CHKSUM                   = (0x1UL << 1),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_PFC                      = (0x1UL << 2),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_PKT_LEN_CHK              = (0x1UL << 3),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_EARLY_DONE_IRQ           = (0x1UL << 4),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_CHK_COS                  = (0x1UL << 5),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_ADV_GPD_WRBK             = (0x1UL << 6),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_GPD_WRBK_FULL_PKT_LEN    = (0x1UL << 7),
    HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_LAST                     = (0x1UL << 8)

} HAL_LIGHTNING_PKT_TX_CHANNEL_CFG_T;

typedef enum
{
    HAL_LIGHTNING_PKT_RX_CHANNEL_CFG_IOC      = (0x1UL << 0),
    HAL_LIGHTNING_PKT_RX_CHANNEL_CFG_CHKSUM   = (0x1UL << 1),
    HAL_LIGHTNING_PKT_RX_CHANNEL_CFG_LAST     = (0x1UL << 2)

} HAL_LIGHTNING_PKT_RX_CHANNEL_CFG_T;

/* ----------------------------------------------------------------------------------- Tx */
typedef enum
{
    HAL_LIGHTNING_PKT_TX_CHANNEL_0 = 0,
    HAL_LIGHTNING_PKT_TX_CHANNEL_1,
    HAL_LIGHTNING_PKT_TX_CHANNEL_2,
    HAL_LIGHTNING_PKT_TX_CHANNEL_3,
    HAL_LIGHTNING_PKT_TX_CHANNEL_LAST

} HAL_LIGHTNING_PKT_TX_CHANNEL_T;

typedef void
(*HAL_LIGHTNING_PKT_TX_FUNC_T)(
    const UI32_T                        unit,
    const void                          *ptr_sw_gpd,    /* SW-GPD to be processed  */
    void                                *ptr_coockie);  /* Private data of SDK     */

typedef struct HAL_LIGHTNING_PKT_TX_SW_GPD_S
{
    HAL_LIGHTNING_PKT_TX_FUNC_T               callback;       /* (unit, ptr_sw_gpd, ptr_cookie) */
    void                                *ptr_cookie;    /* Pointer of CLX_PKT_TX_PKT_T    */
    HAL_LIGHTNING_PKT_TX_GPD_T                tx_gpd;
    UI32_T                              gpd_num;
    struct HAL_LIGHTNING_PKT_TX_SW_GPD_S      *ptr_next;

#if defined (CLX_EN_NETIF)
    UI32_T                              channel;        /* For counter */
#endif

} HAL_LIGHTNING_PKT_TX_SW_GPD_T;

typedef struct
{
    UI32_T                              send_ok;
    UI32_T                              gpd_empty;
    UI32_T                              poll_timeout;

    /* queue */
    UI32_T                              enque_ok;
    UI32_T                              enque_retry;

    /* event */
    UI32_T                              trig_event;

    /* normal interrupt */
    UI32_T                              tx_done;

    /* abnormal interrupt */
    UI32_T                              gpd_hwo_err;          /* bit-0  */
    UI32_T                              gpd_chksm_err;        /* bit-1  */
    UI32_T                              gpd_no_ovfl_err;      /* bit-2  */
    UI32_T                              gpd_dma_read_err;     /* bit-3  */
    UI32_T                              buf_size_err;         /* bit-4  */
    UI32_T                              runt_err;             /* bit-5  */
    UI32_T                              ovsz_err;             /* bit-6  */
    UI32_T                              len_mismatch_err;     /* bit-7  */
    UI32_T                              pktpl_dma_read_err;   /* bit-8  */
    UI32_T                              cos_err;              /* bit-9  */
    UI32_T                              gpd_gt255_err;        /* bit-10 */
    UI32_T                              pfc;                  /* bit-11 */
    UI32_T                              credit_udfl_err;      /* bit-12 */
    UI32_T                              dma_write_err;        /* bit-13 */
    UI32_T                              sw_issue_stop;        /* bit-14 */

    /* others */
    UI32_T                              err_recover;
    UI32_T                              ecc_err;

} HAL_LIGHTNING_PKT_TX_CHANNEL_CNT_T;

typedef struct
{
    HAL_LIGHTNING_PKT_TX_CHANNEL_CNT_T        channel[HAL_LIGHTNING_PKT_TX_CHANNEL_LAST];
    UI32_T                              invoke_gpd_callback;
    UI32_T                              no_memory;

    /* queue */
    UI32_T                              deque_ok;
    UI32_T                              deque_fail;

    /* event */
    UI32_T                              wait_event;

} HAL_LIGHTNING_PKT_TX_CNT_T;

/* ----------------------------------------------------------------------------------- Rx */
typedef enum
{
    HAL_LIGHTNING_PKT_RX_CHANNEL_0 = 0,
    HAL_LIGHTNING_PKT_RX_CHANNEL_1,
    HAL_LIGHTNING_PKT_RX_CHANNEL_2,
    HAL_LIGHTNING_PKT_RX_CHANNEL_3,
    HAL_LIGHTNING_PKT_RX_CHANNEL_LAST
} HAL_LIGHTNING_PKT_RX_CHANNEL_T;

typedef enum
{
    HAL_LIGHTNING_PKT_C_NEXT   = 0, /* callback continuous */
    HAL_LIGHTNING_PKT_C_STOP   = 1,
    HAL_LIGHTNING_PKT_C_OTHERS = 2
} HAL_LIGHTNING_PKT_CALLBACK_NO_T;

typedef enum
{
    HAL_LIGHTNING_PKT_RX_CALLBACK_ACTION_INSERT = 0,
    HAL_LIGHTNING_PKT_RX_CALLBACK_ACTION_APPEND = 1,
    HAL_LIGHTNING_PKT_RX_CALLBACK_ACTION_DELETE = 2,
    HAL_LIGHTNING_PKT_RX_CALLBACK_ACTION_DELETE_ALL = 3
} HAL_LIGHTNING_PKT_RX_CALLBACK_ACTION_T;

typedef HAL_LIGHTNING_PKT_CALLBACK_NO_T
(*HAL_LIGHTNING_PKT_RX_FUNC_T)(
    const UI32_T                        unit,
    const void                          *ptr_sw_gpd,    /* SW-GPD to be processed  */
    void                                *ptr_cookie);   /* Private data of SDK     */

typedef struct HAL_LIGHTNING_PKT_RX_CALLBACK_S
{
    HAL_LIGHTNING_PKT_RX_FUNC_T               callback;       /* (unit, ptr_sw_gpd, ptr_cookie) */
    void                                *ptr_cookie;
    struct HAL_LIGHTNING_PKT_RX_CALLBACK_S    *ptr_next;
} HAL_LIGHTNING_PKT_RX_CALLBACK_T;

typedef struct HAL_LIGHTNING_PKT_RX_SW_GPD_S
{
    BOOL_T                              rx_complete;    /* FALSE when PDMA error occurs */
    HAL_LIGHTNING_PKT_RX_GPD_T                rx_gpd;
    struct HAL_LIGHTNING_PKT_RX_SW_GPD_S      *ptr_next;

#if defined (CLX_EN_NETIF)
    void                                *ptr_cookie;    /* Pointer of virt-addr */
#endif

} HAL_LIGHTNING_PKT_RX_SW_GPD_T;

typedef struct
{
    /* queue */
    UI32_T                              enque_ok;
    UI32_T                              enque_retry;
    UI32_T                              deque_ok;
    UI32_T                              deque_fail;

    /* event */
    UI32_T                              trig_event;

    /* normal interrupt */
    UI32_T                              rx_done;

    /* abnormal interrupt */
    UI32_T                              avbl_gpd_low;         /* bit-0  */
    UI32_T                              avbl_gpd_empty;       /* bit-1  */
    UI32_T                              avbl_gpd_err;         /* bit-2  */
    UI32_T                              gpd_chksm_err;        /* bit-3  */
    UI32_T                              dma_read_err;         /* bit-4  */
    UI32_T                              dma_write_err;        /* bit-5  */
    UI32_T                              sw_issue_stop;        /* bit-6  */
    UI32_T                              gpd_gt255_err;        /* bit-7  */
    UI32_T                              tod_uninit;           /* bit-8  */
    UI32_T                              pkt_err_drop;         /* bit-9  */
    UI32_T                              udsz_drop;            /* bit-10 */
    UI32_T                              ovsz_drop;            /* bit-11 */
    UI32_T                              cmdq_ovf_drop;        /* bit-12 */
    UI32_T                              fifo_ovf_drop;        /* bit-13 */

    /* others */
    UI32_T                              err_recover;
    UI32_T                              ecc_err;

#if defined (CLX_EN_NETIF)
    /* it means that user doesn't create intf on that port */
    UI32_T                              netdev_miss;
#endif


} HAL_LIGHTNING_PKT_RX_CHANNEL_CNT_T;

typedef struct
{
    HAL_LIGHTNING_PKT_RX_CHANNEL_CNT_T        channel[HAL_LIGHTNING_PKT_RX_CHANNEL_LAST];
    UI32_T                              invoke_gpd_callback;
    UI32_T                              no_memory;

    /* event */
    UI32_T                              wait_event;

} HAL_LIGHTNING_PKT_RX_CNT_T;

/* ----------------------------------------------------------------------------------- Reg */
#if defined(CLX_EN_LITTLE_ENDIAN)

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  tch_axlen_cfg               :  3;
        UI32_T                              :  5;
        UI32_T  tch_axi_free_arvalid        :  1;
        UI32_T                              :  7;
        UI32_T  tch_arvalid_thrhold_cfg     :  2;
        UI32_T                              :  6;
        UI32_T  tch_rready_low_4_hdr        :  1;
        UI32_T  tch_ios_crdt_add_en         :  1;
        UI32_T                              :  6;
    } field;
} HAL_LIGHTNING_PKT_AXI_LEN_CFG_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  pdma_lbk_en                 :  1;
        UI32_T                              :  3;
        UI32_T  pdma_lbk_plane              :  2;
        UI32_T                              :  2;
        UI32_T  pm_lbk_en                   :  1;
        UI32_T                              :  7;
        UI32_T  pm_lbk_rqid                 :  6;
        UI32_T                              :  2;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_LBK_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  pdma_lbk_rqid0              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid1              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid2              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid3              :  6;
        UI32_T                              :  2;
    } field;
} HAL_LIGHTNING_PKT_LBK_RQID0_3_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  pdma_lbk_rqid4              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid5              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid6              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid7              :  6;
        UI32_T                              :  2;
    } field;
} HAL_LIGHTNING_PKT_LBK_RQID4_7_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  cos_pfc_sts0                :  8;
        UI32_T  cos_pfc_sts1                :  8;
        UI32_T  cos_pfc_sts2                :  8;
        UI32_T  cos_pfc_sts3                :  8;
    } field;
} HAL_LIGHTNING_PKT_COS_PFC_STS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  pdma_ela_en                 :  1;
        UI32_T                              :  7;
        UI32_T  pdma_ela_valid_sel          :  8;
        UI32_T                              :  8;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_ELA_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  pdma_ela_word0_sel          :  8;
        UI32_T  pdma_ela_word1_sel          :  8;
        UI32_T  pdma_ela_word2_sel          :  8;
        UI32_T  pdma_ela_word3_sel          :  8;
    } field;
} HAL_LIGHTNING_PKT_ELA_SEL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  ingr_pln_ios_credit_base_size_lo    :  8;
        UI32_T  ingr_pln_ios_credit_base_size_hi    :  8;
        UI32_T  ingr_pln_ios_credit_set             :  1;
        UI32_T                                      :  7;
        UI32_T                                      :  1;
        UI32_T  ingr_pln_full_pkt_mode              :  1;
        UI32_T                                      :  6;
    } field;
} HAL_LIGHTNING_PKT_IGR_PLN_CREDIT_CFG_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  ingr_pln_cur_ios_credit_lo  :  8;
        UI32_T  ingr_pln_cur_ios_credit_hi  :  8;
        UI32_T  ingr_pln_ios_credit_ovfl    :  1;
        UI32_T  ingr_pln_ios_credit_udfl    :  1;
        UI32_T                              :  6;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_IGR_PLN_CREDIT_STS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  ingr_pln_ios_credit_rdy_lo_bound    :  8;
        UI32_T  ingr_pln_ios_credit_rdy_hi_bound    :  8;
        UI32_T                                      :  8;
        UI32_T                                      :  8;
    } field;
} HAL_LIGHTNING_PKT_IGR_PLN_CREDIT_THR_REG_T;


typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_stomp_crc_en            :  1;
        UI32_T                              :  7;
        UI32_T  rch_crc_regen_en            :  1;
        UI32_T                              :  7;
        UI32_T  rch_pfc_fun_en              :  1;
        UI32_T                              :  7;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_STOMP_CRC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_ioc_en                  :  1;
        UI32_T                              :  7;
        UI32_T  rch_chksm_en                :  1;
        UI32_T                              :  7;
        UI32_T                              :  8;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_MISC_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_gpd_pfc_lo              :  8;
        UI32_T  rch_gpd_pfc_hi              :  8;
        UI32_T                              :  8;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_GPD_PFC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_fifo_pfc_lo_lo          :  8;
        UI32_T  rch_fifo_pfc_lo_hi          :  3;
        UI32_T                              :  5;
        UI32_T  rch_fifo_pfc_hi_lo          :  8;
        UI32_T  rch_fifo_pfc_hi_hi          :  3;
        UI32_T                              :  5;
    } field;
} HAL_LIGHTNING_PKT_RCH_FIFO_PFC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_cmdq_pfc_lo             :  5;
        UI32_T                              :  3;
        UI32_T  rch_cmdq_pfc_hi             :  5;
        UI32_T                              :  3;
        UI32_T                              :  8;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_CMDQ_PFC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_start                   :  1;
        UI32_T  rch_resume                  :  1;
        UI32_T  rch_stop                    :  1;
        UI32_T                              :  5;
        UI32_T                              :  8;
        UI32_T  rch_gpd_add_no_lo           :  8;
        UI32_T  rch_gpd_add_no_hi           :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_CMD_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_fifo_ovf_drop_cnt_clr   :  1;
        UI32_T  rch_cmdq_ovf_drop_cnt_clr   :  1;
        UI32_T  rch_ovsz_drop_cnt_clr       :  1;
        UI32_T  rch_udsz_drop_cnt_clr       :  1;
        UI32_T  rch_pkterr_drop_cnt_clr     :  1;
        UI32_T  rch_flush_cnt_clr           :  1;
        UI32_T                              :  2;
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_CNT_CLR_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_active                  :  1;
        UI32_T  rch_avbl_gpd_pfc            :  1;
        UI32_T  rch_fifo_pfc                :  1;
        UI32_T  rch_cmdq_pfc                :  1;
        UI32_T  rch_pfc                     :  1;
        UI32_T                              :  3;
        UI32_T                              :  8;
        UI32_T  rch_avbl_gpd_no_lo          :  8;
        UI32_T  rch_avbl_gpd_no_hi          :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_STATUS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  tch_ioc_en                  :  1;
        UI32_T  tch_chksm_en                :  1;
        UI32_T  tch_pfc_en                  :  1;
        UI32_T  tch_pktlen_chk_en           :  1;
        UI32_T  tch_early_done_irq          :  1;
        UI32_T  tch_chk_cos_en              :  1;
        UI32_T  tch_adv_gpd_wrbk            :  1;
        UI32_T  tch_gpd_wrbk_full_pkt_len   :  1;
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_TCH_CFG_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  tch_start                   :  1;
        UI32_T  tch_resume                  :  1;
        UI32_T  tch_stop                    :  1;
        UI32_T                              :  5;
        UI32_T                              :  8;
        UI32_T  tch_gpd_add_no_lo           :  8;
        UI32_T  tch_gpd_add_no_hi           :  8;
    } field;
} HAL_LIGHTNING_PKT_TCH_CMD_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  tch_active                  :  1;
        UI32_T  tch_pfc                     :  1;
        UI32_T  tch_gpd_rd_dma_act          :  1;
        UI32_T                              :  5;
        UI32_T                              :  8;
        UI32_T  tch_avbl_gpd_no             :  1;
        UI32_T                              :  7;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_TCH_STS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  tch_gpd_dmar_qos            :  4;
        UI32_T                              :  4;
        UI32_T  tch_pkt_dmar_qos            :  4;
        UI32_T                              :  4;
        UI32_T  tch_gpd_dmaw_qos            :  4;
        UI32_T                              :  4;
        UI32_T                              :  8;
    } field;
} HAL_LIGHTNING_PKT_TCH_QOS_CFG_REG_T;

#elif defined(CLX_EN_BIG_ENDIAN)

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  6;
        UI32_T  tch_ios_crdt_add_en         :  1;
        UI32_T  tch_rready_low_4_hdr        :  1;
        UI32_T                              :  6;
        UI32_T  tch_arvalid_thrhold_cfg     :  2;
        UI32_T                              :  7;
        UI32_T  tch_axi_free_arvalid        :  1;
        UI32_T                              :  5;
        UI32_T  tch_axlen_cfg               :  3;
    } field;
} HAL_LIGHTNING_PKT_AXI_LEN_CFG_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  2;
        UI32_T  pm_lbk_rqid                 :  6;
        UI32_T                              :  7;
        UI32_T  pm_lbk_en                   :  1;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_plane              :  2;
        UI32_T                              :  3;
        UI32_T  pdma_lbk_en                 :  1;
    } field;
} HAL_LIGHTNING_PKT_LBK_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid3              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid2              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid1              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid0              :  6;
    } field;
} HAL_LIGHTNING_PKT_LBK_RQID0_3_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid7              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid6              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid5              :  6;
        UI32_T                              :  2;
        UI32_T  pdma_lbk_rqid4              :  6;
    } field;
} HAL_LIGHTNING_PKT_LBK_RQID4_7_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  cos_pfc_sts3                :  8;
        UI32_T  cos_pfc_sts2                :  8;
        UI32_T  cos_pfc_sts1                :  8;
        UI32_T  cos_pfc_sts0                :  8;
    } field;
} HAL_LIGHTNING_PKT_COS_PFC_STS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T  pdma_ela_valid_sel          :  8;
        UI32_T                              :  7;
        UI32_T  pdma_ela_en                 :  1;
    } field;
} HAL_LIGHTNING_PKT_ELA_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  pdma_ela_word3_sel          :  8;
        UI32_T  pdma_ela_word2_sel          :  8;
        UI32_T  pdma_ela_word1_sel          :  8;
        UI32_T  pdma_ela_word0_sel          :  8;
    } field;
} HAL_LIGHTNING_PKT_ELA_SEL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                                      :  6;
        UI32_T  ingr_pln_full_pkt_mode              :  1;
        UI32_T                                      :  1;
        UI32_T                                      :  7;
        UI32_T  ingr_pln_ios_credit_set             :  1;
        UI32_T  ingr_pln_ios_credit_base_size_hi    :  8;
        UI32_T  ingr_pln_ios_credit_base_size_lo    :  8;
    } field;
} HAL_LIGHTNING_PKT_IGR_PLN_CREDIT_CFG_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  6;
        UI32_T  ingr_pln_ios_credit_udfl    :  1;
        UI32_T  ingr_pln_ios_credit_ovfl    :  1;
        UI32_T  ingr_pln_cur_ios_credit_hi  :  8;
        UI32_T  ingr_pln_cur_ios_credit_lo  :  8;
    } field;
} HAL_LIGHTNING_PKT_IGR_PLN_CREDIT_STS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                                      :  8;
        UI32_T                                      :  8;
        UI32_T  ingr_pln_ios_credit_rdy_hi_bound    :  8;
        UI32_T  ingr_pln_ios_credit_rdy_lo_bound    :  8;
    } field;
} HAL_LIGHTNING_PKT_IGR_PLN_CREDIT_THR_REG_T;


typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  7;
        UI32_T  rch_pfc_fun_en              :  1;
        UI32_T                              :  7;
        UI32_T  rch_crc_regen_en            :  1;
        UI32_T                              :  7;
        UI32_T  rch_stomp_crc_en            :  1;
    } field;
} HAL_LIGHTNING_PKT_RCH_STOMP_CRC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  7;
        UI32_T  rch_chksm_en                :  1;
        UI32_T                              :  7;
        UI32_T  rch_ioc_en                  :  1;
    } field;
} HAL_LIGHTNING_PKT_RCH_MISC_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T  rch_gpd_pfc_hi              :  8;
        UI32_T  rch_gpd_pfc_lo              :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_GPD_PFC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  5;
        UI32_T  rch_fifo_pfc_hi_hi          :  3;
        UI32_T  rch_fifo_pfc_hi_lo          :  8;
        UI32_T                              :  5;
        UI32_T  rch_fifo_pfc_lo_hi          :  3;
        UI32_T  rch_fifo_pfc_lo_lo          :  8;
    } field;
} HAL_LIGHTNING_PKT_RCH_FIFO_PFC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  3;
        UI32_T  rch_cmdq_pfc_hi             :  5;
        UI32_T                              :  3;
        UI32_T  rch_cmdq_pfc_lo             :  5;
    } field;
} HAL_LIGHTNING_PKT_RCH_CMDQ_PFC_CTRL_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_gpd_add_no_hi           :  8;
        UI32_T  rch_gpd_add_no_lo           :  8;
        UI32_T                              :  8;
        UI32_T                              :  5;
        UI32_T  rch_stop                    :  1;
        UI32_T  rch_resume                  :  1;
        UI32_T  rch_start                   :  1;
    } field;
} HAL_LIGHTNING_PKT_RCH_CMD_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  2;
        UI32_T  rch_flush_cnt_clr           :  1;
        UI32_T  rch_pkterr_drop_cnt_clr     :  1;
        UI32_T  rch_udsz_drop_cnt_clr       :  1;
        UI32_T  rch_ovsz_drop_cnt_clr       :  1;
        UI32_T  rch_cmdq_ovf_drop_cnt_clr   :  1;
        UI32_T  rch_fifo_ovf_drop_cnt_clr   :  1;
    } field;
} HAL_LIGHTNING_PKT_RCH_CNT_CLR_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  rch_avbl_gpd_no_hi          :  8;
        UI32_T  rch_avbl_gpd_no_lo          :  8;
        UI32_T                              :  8;
        UI32_T                              :  3;
        UI32_T  rch_pfc                     :  1;
        UI32_T  rch_cmdq_pfc                :  1;
        UI32_T  rch_fifo_pfc                :  1;
        UI32_T  rch_avbl_gpd_pfc            :  1;
        UI32_T  rch_active                  :  1;
    } field;
} HAL_LIGHTNING_PKT_RCH_STATUS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T                              :  8;
        UI32_T  tch_gpd_wrbk_full_pkt_len   :  1;
        UI32_T  tch_adv_gpd_wrbk            :  1;
        UI32_T  tch_chk_cos_en              :  1;
        UI32_T  tch_early_done_irq          :  1;
        UI32_T  tch_pktlen_chk_en           :  1;
        UI32_T  tch_pfc_en                  :  1;
        UI32_T  tch_chksm_en                :  1;
        UI32_T  tch_ioc_en                  :  1;
    } field;
} HAL_LIGHTNING_PKT_TCH_CFG_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T  tch_gpd_add_no_hi           :  8;
        UI32_T  tch_gpd_add_no_lo           :  8;
        UI32_T                              :  8;
        UI32_T                              :  5;
        UI32_T  tch_stop                    :  1;
        UI32_T  tch_resume                  :  1;
        UI32_T  tch_start                   :  1;
    } field;
} HAL_LIGHTNING_PKT_TCH_CMD_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  7;
        UI32_T  tch_avbl_gpd_no             :  1;
        UI32_T                              :  8;
        UI32_T                              :  5;
        UI32_T  tch_gpd_rd_dma_act          :  1;
        UI32_T  tch_pfc                     :  1;
        UI32_T  tch_active                  :  1;
    } field;
} HAL_LIGHTNING_PKT_TCH_STS_REG_T;

typedef union
{
    UI32_T    reg;
    struct
    {
        UI32_T                              :  8;
        UI32_T                              :  4;
        UI32_T  tch_gpd_dmaw_qos            :  4;
        UI32_T                              :  4;
        UI32_T  tch_pkt_dmar_qos            :  4;
        UI32_T                              :  4;
        UI32_T  tch_gpd_dmar_qos            :  4;
    } field;
} HAL_LIGHTNING_PKT_TCH_QOS_CFG_REG_T;

#else
#error "Host GPD endian is not defined\n"
#endif

/* ----------------------------------------------------------------------------------- CLX_EN_NETIF */
#if defined (CLX_EN_NETIF)
#define HAL_LIGHTNING_PKT_DRIVER_MAJOR_NUM    (10)
#define HAL_LIGHTNING_PKT_DRIVER_MINOR_NUM    (252) /* DO NOT use MISC_DYNAMIC_MINOR */
#define HAL_LIGHTNING_PKT_DRIVER_NAME         "clx_netif"
#define HAL_LIGHTNING_PKT_DRIVER_PATH         "/dev/"HAL_LIGHTNING_PKT_DRIVER_NAME

/* These requirements come from CLX_NETIF APIs.
 * clx_netif -> hal_lightning_pkt_drv -> hal_lightning_pkt_knl
 */

typedef struct
{
    UI32_T          tx_pkt;
    UI32_T          tx_queue_full;
    UI32_T          tx_error;
    UI32_T          rx_pkt;

} HAL_LIGHTNING_PKT_NETIF_INTF_CNT_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    UI32_T                      port;       /* only support unit port and local port */

    /* metadata */
    UI8_T                       mac[6];

#define HAL_LIGHTNING_PKT_NETIF_INTF_FLAGS_MAC        (1UL << 0)
    UI32_T                      flags;


} HAL_LIGHTNING_PKT_NETIF_INTF_T;

#if defined(NETIF_EN_NETLINK)
typedef struct
{
    C8_T                        name[CLX_NETIF_NAME_LEN];
    C8_T                        mc_group_name[CLX_NETIF_NAME_LEN];
} HAL_LIGHTNING_PKT_NETIF_RX_DST_NETLINK_T;
#endif

typedef enum
{
    HAL_LIGHTNING_PKT_NETIF_RX_DST_SDK = 0,
#if defined(NETIF_EN_NETLINK)
    HAL_LIGHTNING_PKT_NETIF_RX_DST_NETLINK,
#endif
    HAL_LIGHTNING_PKT_NETIF_RX_DST_LAST
} HAL_LIGHTNING_PKT_NETIF_RX_DST_TYPE_T;

typedef struct
{
    /* unique key */
    UI32_T                              id;
    C8_T                                name[CLX_NETIF_NAME_LEN];
    UI32_T                              priority;

    /* match fields */
    UI32_T                              port;     /* only support unit port and local port */
    HAL_PKT_RX_REASON_BITMAP_T          reason_bitmap;
    UI8_T                               pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T                               mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T                              offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define HAL_LIGHTNING_PKT_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define HAL_LIGHTNING_PKT_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define HAL_LIGHTNING_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define HAL_LIGHTNING_PKT_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define HAL_LIGHTNING_PKT_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define HAL_LIGHTNING_PKT_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T                              flags;

    HAL_LIGHTNING_PKT_NETIF_RX_DST_TYPE_T     dst_type;
#if defined(NETIF_EN_NETLINK)
    HAL_LIGHTNING_PKT_NETIF_RX_DST_NETLINK_T  netlink;
#endif

} HAL_LIGHTNING_PKT_NETIF_PROFILE_T;


/* These requirements come from CLX_PKT APIs.
 * clx_pkt -> hal_lightning_pkt_srv -> hal_lightning_pkt_drv -> hal_lightning_pkt_knl
 */
typedef enum
{
    /* network interface */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_CREATE_INTF = 0,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_DESTROY_INTF,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_INTF,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_CREATE_PROFILE,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_DESTROY_PROFILE,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_PROFILE,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_INTF_CNT,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_CLEAR_INTF_CNT,
    /* driver */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_WAIT_RX_FREE,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_WAIT_TX_FREE,     /* waitTxFree(ASYNC) */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_SET_RX_CFG,       /* setRxConfig       */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_RX_CFG,       /* getRxConfig       */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_DEINIT_TASK,      /* deinitTask        */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_DEINIT_DRV,       /* deinitDrv         */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_INIT_TASK,        /* initTask          */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_INIT_DRV,         /* initDrv           */
    /* counter */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_TX_CNT,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_RX_CNT,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_CLEAR_TX_CNT,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_CLEAR_RX_CNT,
    /* port attribute */
    HAL_LIGHTNING_PKT_IOCTL_TYPE_SET_PORT_ATTR,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_GET_PORT_ATTR,
#if defined(NETIF_EN_NETLINK)
    HAL_LIGHTNING_PKT_IOCTL_TYPE_NL_SET_INTF_PROPERTY,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_NL_GET_INTF_PROPERTY,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_NL_CREATE_NETLINK,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_NL_DESTROY_NETLINK,
    HAL_LIGHTNING_PKT_IOCTL_TYPE_NL_GET_NETLINK,
#endif
    HAL_LIGHTNING_PKT_IOCTL_TYPE_LAST

} HAL_LIGHTNING_PKT_IOCTL_TYPE_T;

typedef enum
{
    HAL_LIGHTNING_PKT_IOCTL_RX_TYPE_INIT = 0,
    HAL_LIGHTNING_PKT_IOCTL_RX_TYPE_DEINIT,
    HAL_LIGHTNING_PKT_IOCTL_RX_TYPE_LAST,

} HAL_LIGHTNING_PKT_IOCTL_RX_TYPE_T;

typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;
    HAL_LIGHTNING_PKT_RX_CNT_T            rx_cnt;
    HAL_LIGHTNING_PKT_TX_CNT_T            tx_cnt;
    CLX_ERROR_NO_T                  rc;

} HAL_LIGHTNING_PKT_IOCTL_CH_CNT_COOKIE_T;

typedef struct
{
    UI32_T                          unit;
    HAL_LIGHTNING_PKT_NETIF_INTF_T        net_intf;       /* addIntf[In,Out], delIntf[In]              */
    HAL_LIGHTNING_PKT_NETIF_PROFILE_T     net_profile;    /* createProfile[In,Out], destroyProfile[In] */
    HAL_LIGHTNING_PKT_NETIF_INTF_CNT_T    cnt;
    CLX_ERROR_NO_T                  rc;

} HAL_LIGHTNING_PKT_IOCTL_NETIF_COOKIE_T;

typedef struct
{
    CLX_ADDR_T                      callback;       /* (unit, ptr_sw_gpd, ptr_cookie) */
    CLX_ADDR_T                      cookie;         /* Pointer of CLX_PKT_TX_PKT_T    */
    UI32_T                          channel;
    UI32_T                          gpd_num;
    CLX_ADDR_T                      hw_gpd_addr;
    CLX_ADDR_T                      sw_gpd_addr;

} HAL_LIGHTNING_PKT_IOCTL_TX_GPD_T;

typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;            /* sendGpd[In]      */
    CLX_ADDR_T                      ioctl_gpd_addr;     /* sendGpd[In]      */
    CLX_ADDR_T                      done_sw_gpd_addr;   /* waitTxFree[Out]  */

} HAL_LIGHTNING_PKT_IOCTL_TX_COOKIE_T;

typedef struct
{
    BOOL_T                          rx_complete;        /* FALSE when PDMA error occurs                 */
    CLX_ADDR_T                      hw_gpd_addr;        /* Pointer to HW GPD in user's SW GPD struct    */
    CLX_ADDR_T                      dma_buf_addr;       /* Pointer to DMA buffer allocated by the user (virtual) */

} HAL_LIGHTNING_PKT_IOCTL_RX_GPD_T;

typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;            /* getRxCnt[In], clearRxInt[In]     */
    CLX_ADDR_T                      ioctl_gpd_addr;     /* waitRxFree[Out]                  */
    UI32_T                          buf_len;            /* setRxCfg[In]                     */
    HAL_LIGHTNING_PKT_IOCTL_RX_TYPE_T     rx_type;            /* setRxCfg[In]                     */

} HAL_LIGHTNING_PKT_IOCTL_RX_COOKIE_T;

typedef struct
{
    UI32_T                          port;
    UI32_T                          status;
    CLX_PORT_SPEED_T                speed;

} HAL_LIGHTNING_PKT_IOCTL_PORT_COOKIE_T;


#if defined(NETIF_EN_NETLINK)

typedef struct
{
    /* intf property */
    UI32_T                          intf_id;
    CLX_NETIF_INTF_PROPERTY_T       property;
    UI32_T                          param0;
    UI32_T                          param1;

    /* netlink */
    CLX_NETIF_NETLINK_T             netlink;

    CLX_ERROR_NO_T                  rc;

} HAL_LIGHTNING_PKT_NL_IOCTL_COOKIE_T;


#endif  /* End of NETIF_EN_NETLINK */

typedef union
{
    UI32_T                          value;
    struct
    {
        UI32_T                      unit :  6;      /* Maximum unit number is 64.       */
        HAL_LIGHTNING_PKT_IOCTL_TYPE_T    type : 10;      /* Maximum 1024 IOCTL types         */
        UI32_T                      rsvd : 16;
    } field;

} HAL_LIGHTNING_PKT_IOCTL_CMD_T;

#endif /* End of CLX_EN_NETIF */

//} 
/*---------------------------------------------------------------------------*/
/* perf */
CLX_ERROR_NO_T
hal_lightning_pkt_getTxIntrCnt(
    const UI32_T                        unit,
    const UI32_T                        channel,
    UI32_T                              *ptr_intr_cnt);

CLX_ERROR_NO_T
hal_lightning_pkt_getRxIntrCnt(
    const UI32_T                        unit,
    const UI32_T                        channel,
    UI32_T                              *ptr_intr_cnt);

/* ioctl */
CLX_ERROR_NO_T
hal_lightning_pkt_getTxKnlCnt(
    const UI32_T                        unit,
    HAL_LIGHTNING_PKT_IOCTL_CH_CNT_COOKIE_T   *ptr_cookie);

CLX_ERROR_NO_T
hal_lightning_pkt_getRxKnlCnt(
    const UI32_T                        unit,
    HAL_LIGHTNING_PKT_IOCTL_CH_CNT_COOKIE_T   *ptr_cookie);

CLX_ERROR_NO_T
hal_lightning_pkt_clearTxKnlCnt(
    const UI32_T                        unit,
    HAL_LIGHTNING_PKT_IOCTL_TX_COOKIE_T       *ptr_cookie);

CLX_ERROR_NO_T
hal_lightning_pkt_clearRxKnlCnt(
    const UI32_T                        unit,
    HAL_LIGHTNING_PKT_IOCTL_RX_COOKIE_T       *ptr_cookie);

CLX_ERROR_NO_T
hal_lightning_pkt_setRxKnlConfig(
    const UI32_T                        unit,
    HAL_LIGHTNING_PKT_IOCTL_RX_COOKIE_T       *ptr_cookie);

CLX_ERROR_NO_T
hal_lightning_pkt_getRxKnlConfig(
    const UI32_T                        unit,
    HAL_LIGHTNING_PKT_IOCTL_RX_COOKIE_T       *ptr_cookie);

/* perf */
CLX_ERROR_NO_T
hal_lightning_pkt_getNetDev(
    const UI32_T                        unit,
    const UI32_T                        port,
    struct net_device                   **pptr_net_dev);

CLX_ERROR_NO_T
hal_lightning_pkt_prepareGpd(
    const UI32_T                        unit,
    const CLX_ADDR_T                    phy_addr,
    const struct sk_buff                *ptr_skb,
    const UI32_T                        port,
    HAL_LIGHTNING_PKT_TX_SW_GPD_T       *ptr_sw_gpd);

CLX_ERROR_NO_T
hal_lightning_pkt_sendGpd(
    const UI32_T                    unit,
    const HAL_LIGHTNING_PKT_TX_CHANNEL_T  channel,
          HAL_LIGHTNING_PKT_TX_SW_GPD_T   *ptr_sw_gpd);

CLX_ERROR_NO_T
hal_lightning_pkt_init(
    const UI32_T                        unit);

CLX_ERROR_NO_T
hal_lightning_pkt_exit(
    const UI32_T                        unit);

ssize_t
hal_lightning_pkt_dev_tx(
    struct file                         *file,
    const char __user                   *buf,
    size_t                              count,
    loff_t                              *pos);

long
hal_lightning_pkt_dev_ioctl(
    struct file                         *filp,
    unsigned int                        cmd,
    unsigned long                       arg);

#endif /* end of HAL_LIGHTNING_PKT_KNL_H */
