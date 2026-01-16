/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2023
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

/* FILE NAME:  hal_lt_dawn_pkt_knl.h
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 */

#ifndef HAL_LT_DAWN_PKT_KNL_H
#define HAL_LT_DAWN_PKT_KNL_H

/* INCLUDE FILE DECLARTIONS
 */
#include <clx_types.h>
#include <clx_error.h>
#include <clx_pkt.h>
#include <hal_pkt_rsrc_knl.h>
#include <clx_netif.h>

#define HAL_LT_DAWN_PORT_NUM (128)

/* CP_COMMON */
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_EN_HI       (0x012C0000)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_LO_HI       (0x012C0004)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_LVL_HI      (0x012C0008)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_LVL_LO      (0x012C000C)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_MASK_SET_HI (0x012C0010)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_MASK_SET_LO (0x012C0014)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_MASK_CLR_HI (0x012C0018)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_MASK_CLR_LO (0x012C001C)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_MASK_VAL_HI (0x012C0020)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_MASK_VAL_LO (0x012C0024)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_STAT_HI     (0x012C0028)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_STAT_LO     (0x012C002C)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_CLR_HI      (0x012C0030)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_CLR_LO      (0x012C0034)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_SET_HI      (0x012C0038)
#define HAL_LT_DAWN_PKT_CP_COMMON_INT_SET_LO      (0x012C003C)

/* PDMA */
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_STAT     (0x013F1000)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_CLR      (0x013F1004)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_EN       (0x013F1010)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_LVL      (0x013F1014)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_MASK_SET (0x013F1018)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_MASK_CLR (0x013F101C)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_MASK_VAL (0x013F1020)
#define HAL_LT_DAWN_PKT_PDMA_ERR_INT_SET      (0x013F1024)
#define HAL_LT_DAWN_PKT_PDMA_CREDIT_CFG       (0x013F1100)

/* Rx */
#define HAL_LT_DAWN_PKT_PDMA_RCH_GPD_RING_START_ADDR_LO (0x013F12E4)
#define HAL_LT_DAWN_PKT_PDMA_RCH_GPD_RING_START_ADDR_HI (0x013F12E8)
#define HAL_LT_DAWN_PKT_PDMA_RCH_GPD_RING_SIZE          (0x013F12EC)
#define HAL_LT_DAWN_PKT_PDMA_RCH_CMD                    (0x013F1300)
#define HAL_LT_DAWN_PKT_PDMA_RCH_INT_EN                 (0x013F1360)
#define HAL_LT_DAWN_PKT_PDMA_RCH_INT_LVL                (0x013F1364)
#define HAL_LT_DAWN_PKT_PDMA_RCH_INT_MASK               (0x013F1368)
#define HAL_LT_DAWN_PKT_PDMA_RCH_INT_SET                (0x013F1370)
#define HAL_LT_DAWN_PKT_PDMA_RCH_INT_CLR                (0x013F1374)
#define HAL_LT_DAWN_PKT_PDMA_RCH_INT_STAT               (0x013F1378)

/* Tx */
#define HAL_LT_DAWN_PKT_PDMA_TCH_GPD_RING_START_ADDR_LO (0x013F1A00)
#define HAL_LT_DAWN_PKT_PDMA_TCH_GPD_RING_START_ADDR_HI (0x013F1A04)
#define HAL_LT_DAWN_PKT_PDMA_TCH_GPD_RING_SIZE          (0x013F1A08)
#define HAL_LT_DAWN_PKT_PDMA_TCH_CMD                    (0x013F1A20)
#define HAL_LT_DAWN_PKT_PDMA_TCH_INT_EN                 (0x013F1A40)
#define HAL_LT_DAWN_PKT_PDMA_TCH_INT_LVL                (0x013F1A44)
#define HAL_LT_DAWN_PKT_PDMA_TCH_INT_MASK               (0x013F1A48)
#define HAL_LT_DAWN_PKT_PDMA_TCH_INT_SET                (0x013F1A50)
#define HAL_LT_DAWN_PKT_PDMA_TCH_INT_CLR                (0x013F1A54)
#define HAL_LT_DAWN_PKT_PDMA_TCH_INT_STAT               (0x013F1A58)

#define HAL_LT_DAWN_PKT_GET_MMIO(__tbl__)                      (0x00FFFFFF & (__tbl__))
#define HAL_LT_DAWN_PKT_GET_PDMA_RCH_REG(__tbl__, __channel__) ((__tbl__) + (0x200 * (__channel__)))
#define HAL_LT_DAWN_PKT_GET_PDMA_TCH_REG(__tbl__, __channel__) ((__tbl__) + (0x100 * (__channel__)))

/* NAMING DECLARATIONS
 */
/* PKT definitions */
#define HAL_LT_DAWN_PKT_TX_MAX_LEN (9216)
#define HAL_LT_DAWN_PKT_RX_MAX_LEN (9216 + 86) /* EPP tunnel header */
#define HAL_LT_DAWN_PKT_MIN_LEN    (64)        /* Ethernet definition */
#define HAL_LT_DAWN_PKT_TMH_HDR_SZ (20)
#define HAL_LT_DAWN_PKT_PPH_HDR_SZ (20)
#define HAL_LT_DAWN_PKT_CRC_LEN    (4)

/* CH */
#define HAL_LT_DAWN_PKT_CH_LAST_GPD   (0)
#define HAL_LT_DAWN_PKT_CH_MIDDLE_GPD (1)

/* PRG */
#define HAL_LT_DAWN_PKT_PRG_PROCESS_GPD (0) /* Normal   */
#define HAL_LT_DAWN_PKT_PRG_SKIP_GPD    (1) /* Skip     */

/* CRCC */
#define HAL_LT_DAWN_PKT_CRCC_SUM_BY_HW (0) /* calculated by HW */
#define HAL_LT_DAWN_PKT_CRCC_SUM_BY_SW (1) /* calculated by SW */

/* IOC */
#define HAL_LT_DAWN_PKT_IOC_NO_INTR  (0) /* trigger interrupt each GPD */
#define HAL_LT_DAWN_PKT_IOC_HAS_INTR (1) /* trigger interrupt when ch=0, default setting */

/* HWO */
#define HAL_LT_DAWN_PKT_HWO_SW_OWN (0)
#define HAL_LT_DAWN_PKT_HWO_HW_OWN (1)

/* ECC */
#define HAL_LT_DAWN_PKT_ECC_ERROR_OCCUR (1)

/* CPU, CPI queue number */
#define HAL_LT_DAWN_PKT_CPU_QUE_NUM (48)
#define HAL_LT_DAWN_PKT_CPI_QUE_NUM (8)

/* PDMA Definitions */
#define HAL_LT_DAWN_PKT_PDMA_MAX_GPD_PER_PKT  (100)       /* <= 256   */
#define HAL_LT_DAWN_PKT_PDMA_TX_INTR_TIMEOUT  (10 * 1000) /* us */
#define HAL_LT_DAWN_PKT_PDMA_TX_POLL_MAX_LOOP (10 * 1000) /* int */

/* Mode */
#define HAL_LT_DAWN_PKT_TX_WAIT_MODE (HAL_LT_DAWN_PKT_TX_WAIT_ASYNC)

/* TX Queue */
#define HAL_LT_DAWN_PKT_TX_TASK_MAX_LOOP (HAL_DFLT_CFG_PKT_TX_QUEUE_LEN)

/* RX Queue */
#define HAL_LT_DAWN_PKT_RX_QUEUE_NUM     (HAL_LT_DAWN_PKT_RX_CHANNEL_LAST)
#define HAL_LT_DAWN_PKT_RX_TASK_MAX_LOOP (HAL_DFLT_CFG_PKT_TX_QUEUE_LEN)

/* MACRO FUNCTION DECLARATIONS
 */
/*---------------------------------------------------------------------------*/
/* [CL8360] Alignment to 64-bytes */
#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define HAL_LT_DAWN_PKT_PDMA_ALIGN_ADDR(pdma_addr, align_sz) \
    (((pdma_addr) + (align_sz)) & 0xFFFFFFFFFFFFFFC0)
#else
#define HAL_LT_DAWN_PKT_PDMA_ALIGN_ADDR(pdma_addr, align_sz) \
    (((pdma_addr) + (align_sz)) & 0xFFFFFFC0)
#endif

/*---------------------------------------------------------------------------*/
#define HAL_LT_DAWN_PKT_SET_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) | (mask_bitmap)))
#define HAL_LT_DAWN_PKT_CLR_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) & (~(mask_bitmap))))

typedef enum {
    HAL_LT_DAWN_PKT_TMH_TYPE_ITMH_ETH = 0,
    HAL_LT_DAWN_PKT_TMH_TYPE_ITMH_FAB,
    HAL_LT_DAWN_PKT_TMH_TYPE_ETMH_FAB,
    HAL_LT_DAWN_PKT_TMH_TYPE_ETMH_ETH,
    HAL_LT_DAWN_PKT_TMH_TYPE_LAST

} HAL_LT_DAWN_PKT_TMH_TYPE_T;

/* DATA TYPE DECLARATIONS
 */
typedef enum {
    HAL_LT_DAWN_PKT_TX_WAIT_ASYNC = 0,
    HAL_LT_DAWN_PKT_TX_WAIT_SYNC_INTR = 1,
    HAL_LT_DAWN_PKT_TX_WAIT_SYNC_POLL = 2

} HAL_LT_DAWN_PKT_TX_WAIT_T;

typedef enum {
    HAL_LT_DAWN_PKT_RX_SCHED_RR = 0,
    HAL_LT_DAWN_PKT_RX_SCHED_WRR = 1

} HAL_LT_DAWN_PKT_RX_SCHED_T;

/* GPD and Packet Strucutre Definition */
#if defined(CLX_EN_BIG_ENDIAN)

typedef struct {
    /* CLX DWORD 0 */
    UI32_T typ : 2;
    UI32_T tc : 4;
    UI32_T color : 2;
    UI32_T srv : 3;
    UI32_T trig : 1;
    UI32_T igr_phy_port : 12;
    UI32_T hsh_val_w0 : 8;
    /* CLX DWORD 1 */
    UI32_T hsh_val_w1 : 2;
    UI32_T dst_idx : 15;
    UI32_T src_idx : 15;
    /* CLX DWORD 2 */
    UI32_T intf_fdid : 14;
    UI32_T nvo3_mgid_is_transit : 1;
    UI32_T skip_epp : 1;
    UI32_T steer_applied : 1;
    UI32_T nvo3_ip_tnl_decap_prop_ttl : 1;
    UI32_T nvo3_mpls_uhp_prop_ttl : 1;
    UI32_T ecn : 2;
    UI32_T store_and_forward : 1;
    UI32_T lag_epoch : 1;
    UI32_T src_supp_tag : 5;
    UI32_T one_arm_rte_srv_fdid : 1;
    UI32_T fab_one_arm_rte : 1;
    UI32_T skip_ipp : 1;
    UI32_T igr_fab_port_grp : 1;
    /* CLX DWORD 3 */
    UI32_T : 2;
    UI32_T nvo3_mgid : 15;
    UI32_T nvo3_intf : 14;
    UI32_T nvo3_src_supp_tag_w0 : 1;
    /* CLX DWORD 4 */
    UI32_T nvo3_src_supp_tag_w1 : 4;
    UI32_T mir_bmap : 8;
    UI32_T cp_to_cpu_code : 4;
    UI32_T cp_to_cpu_bmap : 16;
} HAL_LT_DAWN_PKT_ITMH_ETH_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T typ : 2;
    UI32_T tc : 4;
    UI32_T color : 2;
    UI32_T srv : 3;
    UI32_T trig : 1;
    UI32_T igr_phy_port : 12;
    UI32_T hsh_val_w0 : 8;
    /* CLX DWORD 1 */
    UI32_T hsh_val_w1 : 2;
    UI32_T dst_idx : 15;
    UI32_T src_idx : 15;
    /* CLX DWORD 2 */
    UI32_T intf_fdid : 14;
    UI32_T nvo3_mgid_is_transit : 1;
    UI32_T skip_epp : 1;
    UI32_T steer_applied : 1;
    UI32_T nvo3_ip_tnl_decap_prop_ttl : 1;
    UI32_T nvo3_mpls_uhp_prop_ttl : 1;
    UI32_T ecn : 2;
    UI32_T store_and_forward : 1;
    UI32_T lag_epoch : 1;
    UI32_T src_supp_tag : 5;
    UI32_T one_arm_rte_srv_fdid : 1;
    UI32_T fab_one_arm_rte : 1;
    UI32_T : 2;
    /* CLX DWORD 3 */
    UI32_T : 32;
    /* CLX DWORD 4 */
    UI32_T : 20;
    UI32_T had_uturn : 1;
    UI32_T : 2;
    UI32_T excpt_code : 8;
    UI32_T exp_dscp_mrkd : 1;
} HAL_LT_DAWN_PKT_ETMH_FAB_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T typ : 2;
    UI32_T tc : 4;
    UI32_T color : 2;
    UI32_T srv : 3;
    UI32_T trig : 1;
    UI32_T igr_phy_port : 12;
    UI32_T hsh_val_w0 : 8;
    /* CLX DWORD 1 */
    UI32_T hsh_val_w1 : 2;
    UI32_T dst_idx : 15;
    UI32_T src_idx : 15;
    /* CLX DWORD 2 */
    UI32_T intf_fdid : 14;
    UI32_T nvo3_mgid_is_transit : 1;
    UI32_T skip_epp : 1;
    UI32_T steer_applied : 1;
    UI32_T nvo3_ip_tnl_decap_prop_ttl : 1;
    UI32_T nvo3_mpls_uhp_prop_ttl : 1;
    UI32_T ecn : 2;
    UI32_T igr_fab_port_grp : 1;
    UI32_T redir : 1;
    UI32_T excpt_code_mir_bmap : 8;
    UI32_T cp_to_cpu_bmap_w0 : 1;
    /* CLX DWORD 3 */
    UI32_T cp_to_cpu_bmap_w1 : 7;
    UI32_T egr_phy_port : 12;
    UI32_T src_supp_pnd : 1;
    UI32_T mc_vid_ctl : 3;
    UI32_T mc_vid_1st_w0 : 9;
    /* CLX DWORD 4 */
    UI32_T mc_vid_1st_w1 : 3;
    UI32_T mc_vid_2nd : 12;
    UI32_T mc_decr_ttl : 1;
    UI32_T mc_is_routed : 1;
    UI32_T mc_mel_vld : 1;
    UI32_T mc_cp_idx : 13;
    UI32_T exp_dscp_mrkd : 1;
} HAL_LT_DAWN_PKT_ETMH_ETH_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T decap_act : 3;
    UI32_T igr_l2_vid_num : 2;
    UI32_T nvo3_encap_idx : 14;
    UI32_T mpls_pw_cw_vld : 1;
    UI32_T hit_idx_w0 : 12;
    /* CLX DWORD 1 */
    UI32_T hit_idx_w1 : 7;
    UI32_T nvo3_adj_idx : 8;
    UI32_T seg_vmid_w0 : 17;
    /* CLX DWORD 2 */
    UI32_T seg_vmid_w1 : 7;
    UI32_T : 1;
    UI32_T l2_sa_lrn_en_hw_cvs : 1;
    UI32_T l2_sa_lrn_en_hw : 1;
    UI32_T vid_ctl : 3;
    UI32_T vid_1st : 12;
    UI32_T vid_2nd_w0 : 7;
    /* CLX DWORD 3 */
    UI32_T vid_2nd_w1 : 5;
    UI32_T flw_lbl : 10;
    UI32_T rewr_idx_ctl : 2;
    UI32_T rewr_idx_0 : 13;
    UI32_T rewr_idx_1_w0 : 2;
    /* CLX DWORD 4 */
    UI32_T rewr_idx_1_w1 : 11;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_dei_val : 1;
    UI32_T ts : 16;
} HAL_LT_DAWN_PKT_PPH_L2_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T decap_act : 3;
    UI32_T igr_l2_vid_num : 2;
    UI32_T nvo3_encap_idx : 14;
    UI32_T mpls_pw_cw_vld : 1;
    UI32_T hit_idx_w0 : 12;
    /* CLX DWORD 1 */
    UI32_T hit_idx_w1 : 7;
    UI32_T nvo3_adj_idx : 8;
    UI32_T seg_vmid_w0 : 17;
    /* CLX DWORD 2 */
    UI32_T seg_vmid_w1 : 7;
    UI32_T : 1;
    UI32_T rpf_pnd : 1;
    UI32_T adj_idx : 18;
    UI32_T is_mc : 1;
    UI32_T decr_ttl : 1;
    UI32_T decap_prop_ttl : 1;
    UI32_T mrk_dscp_en : 1;
    UI32_T mrk_dscp_val_w0 : 1;
    /* CLX DWORD 3 */
    UI32_T mrk_dscp_val_w1 : 5;
    UI32_T flw_lbl : 10;
    UI32_T rewr_idx_ctl : 2;
    UI32_T rewr_idx_0 : 13;
    UI32_T rewr_idx_1_w0 : 2;
    /* CLX DWORD 4 */
    UI32_T rewr_idx_1_w1 : 11;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_dei_val : 1;
    UI32_T ts : 16;
} HAL_LT_DAWN_PKT_PPH_L3UC_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T decap_act : 3;
    UI32_T igr_l2_vid_num : 2;
    UI32_T nvo3_encap_idx : 14;
    UI32_T mpls_pw_cw_vld : 1;
    UI32_T hit_idx_w0 : 12;
    /* CLX DWORD 1 */
    UI32_T hit_idx_w1 : 7;
    UI32_T nvo3_adj_idx : 8;
    UI32_T vid_1st : 12;
    UI32_T vid_2nd_w0 : 5;
    /* CLX DWORD 2 */
    UI32_T vid_2nd_w1 : 7;
    UI32_T : 15;
    UI32_T l2_sa_lrn_en_hw_cvs : 1;
    UI32_T l2_sa_lrn_en_hw : 1;
    UI32_T vid_ctl : 3;
    UI32_T is_mc : 1;
    UI32_T : 1;
    UI32_T decap_prop_ttl : 1;
    UI32_T mrk_dscp_en : 1;
    UI32_T mrk_dscp_val_w0 : 1;
    /* CLX DWORD 3 */
    UI32_T mrk_dscp_val_w1 : 5;
    UI32_T flw_lbl : 10;
    UI32_T rewr_idx_ctl : 2;
    UI32_T rewr_idx_0 : 13;
    UI32_T rewr_idx_1_w0 : 2;
    /* CLX DWORD 4 */
    UI32_T rewr_idx_1_w1 : 11;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_dei_val : 1;
    UI32_T ts : 16;
} HAL_LT_DAWN_PKT_PPH_L3MC_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T decap_act : 3;
    UI32_T igr_l2_vid_num : 2;
    UI32_T nvo3_encap_idx : 14;
    UI32_T : 1;
    UI32_T hit_idx_w0 : 12;
    /* CLX DWORD 1 */
    UI32_T hit_idx_w1 : 7;
    UI32_T nvo3_adj_idx : 8;
    UI32_T seg_vmid_w0 : 17;
    /* CLX DWORD 2 */
    UI32_T seg_vmid_w1 : 7;
    UI32_T : 2;
    UI32_T adj_idx : 18;
    UI32_T : 1;
    UI32_T decr_ttl : 1;
    UI32_T decap_prop_ttl : 1;
    UI32_T mrk_exp_en : 1;
    UI32_T : 1;
    /* CLX DWORD 3 */
    UI32_T : 2;
    UI32_T mrk_exp_val : 3;
    UI32_T php_pop_keep_inner_qos : 1;
    UI32_T : 26;
    /* CLX DWORD 4 */
    UI32_T : 11;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_dei_val : 1;
    UI32_T ts : 16;
} HAL_LT_DAWN_PKT_PPH_L25_T;

#elif defined(CLX_EN_LITTLE_ENDIAN)

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hsh_val_w0 : 8;
    UI32_T igr_phy_port : 12;
    UI32_T trig : 1;
    UI32_T srv : 3;
    UI32_T color : 2;
    UI32_T tc : 4;
    UI32_T typ : 2;
    /* CLX DWORD 1 */
    UI32_T src_idx : 15;
    UI32_T dst_idx : 15;
    UI32_T hsh_val_w1 : 2;
    /* CLX DWORD 2 */
    UI32_T igr_fab_port_grp : 1;
    UI32_T skip_ipp : 1;
    UI32_T fab_one_arm_rte : 1;
    UI32_T one_arm_rte_srv_fdid : 1;
    UI32_T src_supp_tag : 5;
    UI32_T lag_epoch : 1;
    UI32_T store_and_forward : 1;
    UI32_T ecn : 2;
    UI32_T nvo3_mpls_uhp_prop_ttl : 1;
    UI32_T nvo3_ip_tnl_decap_prop_ttl : 1;
    UI32_T steer_applied : 1;
    UI32_T skip_epp : 1;
    UI32_T nvo3_mgid_is_transit : 1;
    UI32_T intf_fdid : 14;
    /* CLX DWORD 3 */
    UI32_T nvo3_src_supp_tag_w0 : 1;
    UI32_T nvo3_intf : 14;
    UI32_T nvo3_mgid : 15;
    UI32_T : 2;
    /* CLX DWORD 4 */
    UI32_T cp_to_cpu_bmap : 16;
    UI32_T cp_to_cpu_code : 4;
    UI32_T mir_bmap : 8;
    UI32_T nvo3_src_supp_tag_w1 : 4;
} HAL_LT_DAWN_PKT_ITMH_ETH_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hsh_val_w0 : 8;
    UI32_T igr_phy_port : 12;
    UI32_T trig : 1;
    UI32_T srv : 3;
    UI32_T color : 2;
    UI32_T tc : 4;
    UI32_T typ : 2;
    /* CLX DWORD 1 */
    UI32_T src_idx : 15;
    UI32_T dst_idx : 15;
    UI32_T hsh_val_w1 : 2;
    /* CLX DWORD 2 */
    UI32_T : 2;
    UI32_T fab_one_arm_rte : 1;
    UI32_T one_arm_rte_srv_fdid : 1;
    UI32_T src_supp_tag : 5;
    UI32_T lag_epoch : 1;
    UI32_T store_and_forward : 1;
    UI32_T ecn : 2;
    UI32_T nvo3_mpls_uhp_prop_ttl : 1;
    UI32_T nvo3_ip_tnl_decap_prop_ttl : 1;
    UI32_T steer_applied : 1;
    UI32_T skip_epp : 1;
    UI32_T nvo3_mgid_is_transit : 1;
    UI32_T intf_fdid : 14;
    /* CLX DWORD 3 */
    UI32_T : 32;
    /* CLX DWORD 4 */
    UI32_T exp_dscp_mrkd : 1;
    UI32_T excpt_code : 8;
    UI32_T : 2;
    UI32_T had_uturn : 1;
    UI32_T : 20;
} HAL_LT_DAWN_PKT_ETMH_FAB_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hsh_val_w0 : 8;
    UI32_T igr_phy_port : 12;
    UI32_T trig : 1;
    UI32_T srv : 3;
    UI32_T color : 2;
    UI32_T tc : 4;
    UI32_T typ : 2;
    /* CLX DWORD 1 */
    UI32_T src_idx : 15;
    UI32_T dst_idx : 15;
    UI32_T hsh_val_w1 : 2;
    /* CLX DWORD 2 */
    UI32_T cp_to_cpu_bmap_w0 : 1;
    UI32_T excpt_code_mir_bmap : 8;
    UI32_T redir : 1;
    UI32_T igr_fab_port_grp : 1;
    UI32_T ecn : 2;
    UI32_T nvo3_mpls_uhp_prop_ttl : 1;
    UI32_T nvo3_ip_tnl_decap_prop_ttl : 1;
    UI32_T steer_applied : 1;
    UI32_T skip_epp : 1;
    UI32_T nvo3_mgid_is_transit : 1;
    UI32_T intf_fdid : 14;
    /* CLX DWORD 3 */
    UI32_T mc_vid_1st_w0 : 9;
    UI32_T mc_vid_ctl : 3;
    UI32_T src_supp_pnd : 1;
    UI32_T egr_phy_port : 12;
    UI32_T cp_to_cpu_bmap_w1 : 7;
    /* CLX DWORD 4 */
    UI32_T exp_dscp_mrkd : 1;
    UI32_T mc_cp_idx : 13;
    UI32_T mc_mel_vld : 1;
    UI32_T mc_is_routed : 1;
    UI32_T mc_decr_ttl : 1;
    UI32_T mc_vid_2nd : 12;
    UI32_T mc_vid_1st_w1 : 3;
} HAL_LT_DAWN_PKT_ETMH_ETH_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hit_idx_w0 : 12;
    UI32_T mpls_pw_cw_vld : 1;
    UI32_T nvo3_encap_idx : 14;
    UI32_T igr_l2_vid_num : 2;
    UI32_T decap_act : 3;
    /* CLX DWORD 1 */
    UI32_T seg_vmid_w0 : 17;
    UI32_T nvo3_adj_idx : 8;
    UI32_T hit_idx_w1 : 7;
    /* CLX DWORD 2 */
    UI32_T vid_2nd_w0 : 7;
    UI32_T vid_1st : 12;
    UI32_T vid_ctl : 3;
    UI32_T l2_sa_lrn_en_hw : 1;
    UI32_T l2_sa_lrn_en_hw_cvs : 1;
    UI32_T : 1;
    UI32_T seg_vmid_w1 : 7;
    /* CLX DWORD 3 */
    UI32_T rewr_idx_1_w0 : 2;
    UI32_T rewr_idx_0 : 13;
    UI32_T rewr_idx_ctl : 2;
    UI32_T flw_lbl : 10;
    UI32_T vid_2nd_w1 : 5;
    /* CLX DWORD 4 */
    UI32_T ts : 16;
    UI32_T mrk_dei_val : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T rewr_idx_1_w1 : 11;
} HAL_LT_DAWN_PKT_PPH_L2_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hit_idx_w0 : 12;
    UI32_T mpls_pw_cw_vld : 1;
    UI32_T nvo3_encap_idx : 14;
    UI32_T igr_l2_vid_num : 2;
    UI32_T decap_act : 3;
    /* CLX DWORD 1 */
    UI32_T seg_vmid_w0 : 17;
    UI32_T nvo3_adj_idx : 8;
    UI32_T hit_idx_w1 : 7;
    /* CLX DWORD 2 */
    UI32_T mrk_dscp_val_w0 : 1;
    UI32_T mrk_dscp_en : 1;
    UI32_T decap_prop_ttl : 1;
    UI32_T decr_ttl : 1;
    UI32_T is_mc : 1;
    UI32_T adj_idx : 18;
    UI32_T rpf_pnd : 1;
    UI32_T : 1;
    UI32_T seg_vmid_w1 : 7;
    /* CLX DWORD 3 */
    UI32_T rewr_idx_1_w0 : 2;
    UI32_T rewr_idx_0 : 13;
    UI32_T rewr_idx_ctl : 2;
    UI32_T flw_lbl : 10;
    UI32_T mrk_dscp_val_w1 : 5;
    /* CLX DWORD 4 */
    UI32_T ts : 16;
    UI32_T mrk_dei_val : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T rewr_idx_1_w1 : 11;
} HAL_LT_DAWN_PKT_PPH_L3UC_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hit_idx_w0 : 12;
    UI32_T mpls_pw_cw_vld : 1;
    UI32_T nvo3_encap_idx : 14;
    UI32_T igr_l2_vid_num : 2;
    UI32_T decap_act : 3;
    /* CLX DWORD 1 */
    UI32_T vid_2nd_w0 : 5;
    UI32_T vid_1st : 12;
    UI32_T nvo3_adj_idx : 8;
    UI32_T hit_idx_w1 : 7;
    /* CLX DWORD 2 */
    UI32_T mrk_dscp_val_w0 : 1;
    UI32_T mrk_dscp_en : 1;
    UI32_T decap_prop_ttl : 1;
    UI32_T : 1;
    UI32_T is_mc : 1;
    UI32_T vid_ctl : 3;
    UI32_T l2_sa_lrn_en_hw : 1;
    UI32_T l2_sa_lrn_en_hw_cvs : 1;
    UI32_T : 15;
    UI32_T vid_2nd_w1 : 7;
    /* CLX DWORD 3 */
    UI32_T rewr_idx_1_w0 : 2;
    UI32_T rewr_idx_0 : 13;
    UI32_T rewr_idx_ctl : 2;
    UI32_T flw_lbl : 10;
    UI32_T mrk_dscp_val_w1 : 5;
    /* CLX DWORD 4 */
    UI32_T ts : 16;
    UI32_T mrk_dei_val : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T rewr_idx_1_w1 : 11;
} HAL_LT_DAWN_PKT_PPH_L3MC_T;

typedef struct {
    /* CLX DWORD 0 */
    UI32_T hit_idx_w0 : 12;
    UI32_T : 1;
    UI32_T nvo3_encap_idx : 14;
    UI32_T igr_l2_vid_num : 2;
    UI32_T decap_act : 3;
    /* CLX DWORD 1 */
    UI32_T seg_vmid_w0 : 17;
    UI32_T nvo3_adj_idx : 8;
    UI32_T hit_idx_w1 : 7;
    /* CLX DWORD 2 */
    UI32_T : 1;
    UI32_T mrk_exp_en : 1;
    UI32_T decap_prop_ttl : 1;
    UI32_T decr_ttl : 1;
    UI32_T : 1;
    UI32_T adj_idx : 18;
    UI32_T : 2;
    UI32_T seg_vmid_w1 : 7;
    /* CLX DWORD 3 */
    UI32_T : 26;
    UI32_T php_pop_keep_inner_qos : 1;
    UI32_T mrk_exp_val : 3;
    UI32_T : 2;
    /* CLX DWORD 4 */
    UI32_T ts : 16;
    UI32_T mrk_dei_val : 1;
    UI32_T mrk_pcp_val : 3;
    UI32_T mrk_pcp_dei_en : 1;
    UI32_T : 11;
} HAL_LT_DAWN_PKT_PPH_L25_T;

#else
#error "Host GPD endian is not defined!!\n"
#endif

#if defined(CLX_EN_BIG_ENDIAN)

/* RX GPD STRUCTURE */
typedef struct {
    UI32_T data_buf_addr_lo;
    UI32_T data_buf_addr_hi;
    UI32_T chksum : 16;
    UI32_T ioc : 1;
    UI32_T : 1;
    UI32_T avbl_buf_len : 14;
    UI32_T : 32;

    union {
        HAL_LT_DAWN_PKT_ITMH_ETH_T itmh_eth;
        HAL_LT_DAWN_PKT_ETMH_FAB_T etmh_fab;
        HAL_LT_DAWN_PKT_ETMH_ETH_T etmh_eth;
    };
    union {
        HAL_LT_DAWN_PKT_PPH_L2_T pph_l2;
        HAL_LT_DAWN_PKT_PPH_L3UC_T pph_l3uc;
        HAL_LT_DAWN_PKT_PPH_L3MC_T pph_l3mc;
        HAL_LT_DAWN_PKT_PPH_L25_T pph_l25;
    };

    UI32_T : 32;
    UI32_T hwo : 1;
    UI32_T ch : 1;
    UI32_T trn : 1;
    UI32_T ecce : 1;
    UI32_T errf : 1;
    UI32_T : 5;
    UI32_T queue : 6;
    UI32_T : 2;
    UI32_T cnsm_buf_len : 14;

} HAL_LT_DAWN_PKT_RX_GPD_T;

/* TX GPD STRUCTURE */
typedef struct {
    UI32_T data_buf_addr_lo;
    UI32_T data_buf_addr_hi;
    UI32_T chksum : 16;
    UI32_T ioc : 1;
    UI32_T : 1;
    UI32_T data_buf_size : 14;
    UI32_T : 32;

    union {
        HAL_LT_DAWN_PKT_ITMH_ETH_T itmh_eth;
        HAL_LT_DAWN_PKT_ETMH_FAB_T etmh_fab;
        HAL_LT_DAWN_PKT_ETMH_ETH_T etmh_eth;
    };
    union {
        HAL_LT_DAWN_PKT_PPH_L2_T pph_l2;
        HAL_LT_DAWN_PKT_PPH_L3UC_T pph_l3uc;
        HAL_LT_DAWN_PKT_PPH_L3MC_T pph_l3mc;
        HAL_LT_DAWN_PKT_PPH_L25_T pph_l25;
    };

    UI32_T : 16;
    UI32_T ptp_hdr : 16;
    UI32_T hwo : 1;
    UI32_T ch : 1;
    UI32_T : 1;
    UI32_T ecce : 1;
    UI32_T : 4;
    UI32_T cos : 3;
    UI32_T phc : 1;      /* PTP Header Control */
    UI32_T ipc : 2;      /* Ingress Plane Control */
    UI32_T crcc : 1;
    UI32_T prg : 1;      /* Purge */
    UI32_T : 2;
    UI32_T pkt_len : 14; /* Total packet length */

} HAL_LT_DAWN_PKT_TX_GPD_T;

#elif defined(CLX_EN_LITTLE_ENDIAN)

/* RX GPD STRUCTURE */
typedef struct {
    UI32_T data_buf_addr_lo;
    UI32_T data_buf_addr_hi;
    UI32_T avbl_buf_len : 14;
    UI32_T : 1;
    UI32_T ioc : 1;
    UI32_T chksum : 16;
    UI32_T : 32;

    union {
        HAL_LT_DAWN_PKT_ITMH_ETH_T itmh_eth;
        HAL_LT_DAWN_PKT_ETMH_FAB_T etmh_fab;
        HAL_LT_DAWN_PKT_ETMH_ETH_T etmh_eth;
    };
    union {
        HAL_LT_DAWN_PKT_PPH_L2_T pph_l2;
        HAL_LT_DAWN_PKT_PPH_L3UC_T pph_l3uc;
        HAL_LT_DAWN_PKT_PPH_L3MC_T pph_l3mc;
        HAL_LT_DAWN_PKT_PPH_L25_T pph_l25;
    };

    UI32_T : 32;
    UI32_T cnsm_buf_len : 14;
    UI32_T : 2;
    UI32_T queue : 6;
    UI32_T : 5;
    UI32_T errf : 1;
    UI32_T ecce : 1;
    UI32_T trn : 1;
    UI32_T ch : 1;
    UI32_T hwo : 1;

} HAL_LT_DAWN_PKT_RX_GPD_T;

/* TX GPD STRUCTURE */
typedef struct {
    UI32_T data_buf_addr_lo;
    UI32_T data_buf_addr_hi;
    UI32_T data_buf_size : 14;
    UI32_T : 1;
    UI32_T ioc : 1;
    UI32_T chksum : 16;
    UI32_T : 32;

    union {
        HAL_LT_DAWN_PKT_ITMH_ETH_T itmh_eth;
        HAL_LT_DAWN_PKT_ETMH_FAB_T etmh_fab;
        HAL_LT_DAWN_PKT_ETMH_ETH_T etmh_eth;
    };
    union {
        HAL_LT_DAWN_PKT_PPH_L2_T pph_l2;
        HAL_LT_DAWN_PKT_PPH_L3UC_T pph_l3uc;
        HAL_LT_DAWN_PKT_PPH_L3MC_T pph_l3mc;
        HAL_LT_DAWN_PKT_PPH_L25_T pph_l25;
    };

    UI32_T ptp_hdr : 16;
    UI32_T : 16;
    UI32_T pkt_len : 14; /* Total packet length */
    UI32_T : 2;
    UI32_T prg : 1;      /* Purge */
    UI32_T crcc : 1;
    UI32_T ipc : 2;      /* Ingress Plane Control */
    UI32_T phc : 1;      /* PTP Header Control */
    UI32_T cos : 3;
    UI32_T : 4;
    UI32_T ecce : 1;
    UI32_T : 1;
    UI32_T ch : 1;
    UI32_T hwo : 1;
} HAL_LT_DAWN_PKT_TX_GPD_T;

#else
#error "Host GPD endian is not defined\n"
#endif

/* ----------------------------------------------------------------------------------- PP Type */
typedef struct {
    union {
        HAL_LT_DAWN_PKT_ITMH_ETH_T itmh_eth;
        HAL_LT_DAWN_PKT_ETMH_FAB_T etmh_fab;
        HAL_LT_DAWN_PKT_ETMH_ETH_T etmh_eth;
    };
} HAL_LT_DAWN_PKT_TMH_T;

/* ----------------------------------------------------------------------------------- Reg Type */
typedef enum {
    HAL_LT_DAWN_PKT_L2_ISR_RCH0 = (0x1UL << 0),
    HAL_LT_DAWN_PKT_L2_ISR_RCH1 = (0x1UL << 1),
    HAL_LT_DAWN_PKT_L2_ISR_RCH2 = (0x1UL << 2),
    HAL_LT_DAWN_PKT_L2_ISR_RCH3 = (0x1UL << 3),
    HAL_LT_DAWN_PKT_L2_ISR_TCH0 = (0x1UL << 4),
    HAL_LT_DAWN_PKT_L2_ISR_TCH1 = (0x1UL << 5),
    HAL_LT_DAWN_PKT_L2_ISR_TCH2 = (0x1UL << 6),
    HAL_LT_DAWN_PKT_L2_ISR_TCH3 = (0x1UL << 7),
    HAL_LT_DAWN_PKT_L2_ISR_RX_QID_MAP_ERR = (0x1UL << 8),
    HAL_LT_DAWN_PKT_L2_ISR_RX_FRAME_ERR = (0x1UL << 9)

} HAL_LT_DAWN_PKT_L2_ISR_T;

typedef enum {
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_GPD_HWO_ERROR = (0x1UL << 0),   /* Tx GPD.hwo = 0     */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_GPD_CHKSM_ERROR = (0x1UL << 1), /* Tx GPD.chksm is error   */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_GPD_NO_OVFL_ERROR = (0x1UL << 2), /* S/W push too much GPD */
    /* AXI Rd Error when do GPD read          */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_GPD_DMA_READ_ERROR = (0x1UL << 3),
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_BUF_SIZE_ERROR = (0x1UL << 4), /* Tx GPD.data_buf_size = 0 */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_RUNT_ERROR = (0x1UL << 5),     /* Tx GPD.pkt_len < 64     */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_OVSZ_ERROR = (0x1UL << 6),     /* Tx GPD.pkt_len = 9217     */
    /* Tx GPD.pkt_len != sum of data_buf_size */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_LEN_MISMATCH_ERROR = (0x1UL << 7),
    /* AXI Rd Error when do Payload read      */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_PKTPL_DMA_READ_ERROR = (0x1UL << 8),
    /* Tx GPD.cos is not match cos_to_tch_map */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_COS_ERROR = (0x1UL << 9),
    /* Multi-GPD packet's GPD# > 255          */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_GPD_GT255_ERROR = (0x1UL << 10),
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_PFC = (0x1UL << 11), /* */
    /* Credit Underflow (count down to 0)     */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_CREDIT_UDFL_ERROR = (0x1UL << 12),
    /* AXI Wr Error (GPD Write-Back)          */
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_DMA_WRITE_ERROR = (0x1UL << 13),
    HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_STOP_CMD_CPLT = (0x1UL << 14)

} HAL_LT_DAWN_PKT_TX_CHANNEL_L2_ISR_T;

typedef enum {
    /* Rx GPD.avbl_gpd_num < threshold        */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_AVAIL_GPD_LOW = (0x1UL << 0),
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_AVAIL_GPD_EMPTY = (0x1UL << 1), /* Rx GPD.avbl_gpd_num = 0 */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_AVAIL_GPD_ERROR = (0x1UL << 2), /* Rx GPD.hwo = 0 */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_GPD_CHKSM_ERROR = (0x1UL << 3), /* Rx GPD.chksm is error */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_DMA_READ_ERROR = (0x1UL << 4), /* DMAR error occurs in PCIE */
    /* DMAW error occurs in PCIE              */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_DMA_WRITE_ERROR = (0x1UL << 5),
    /* Stop Completion Acknowledge            */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_STOP_CMD_CPLT = (0x1UL << 6),
    /* Multi-GPD packet's GPD# > 255          */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_GPD_GT255_ERROR = (0x1UL << 7),
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_TOD_UNINIT = (0x1UL << 8),     /* */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_PKT_ERROR_DROP = (0x1UL << 9), /* */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_UDSZ_DROP = (0x1UL << 10),     /* */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_OVSZ_DROP = (0x1UL << 11),     /* */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_CMDQ_OVF_DROP = (0x1UL << 12), /* */
    HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_FIFO_OVF_DROP = (0x1UL << 13)

} HAL_LT_DAWN_PKT_RX_CHANNEL_L2_ISR_T;

/* ----------------------------------------------------------------------------------- Tx */
typedef enum {
    HAL_LT_DAWN_PKT_TX_CHANNEL_0 = 0,
    HAL_LT_DAWN_PKT_TX_CHANNEL_1,
    HAL_LT_DAWN_PKT_TX_CHANNEL_2,
    HAL_LT_DAWN_PKT_TX_CHANNEL_3,
    HAL_LT_DAWN_PKT_TX_CHANNEL_LAST

} HAL_LT_DAWN_PKT_TX_CHANNEL_T;

typedef void (*HAL_LT_DAWN_PKT_TX_FUNC_T)(const UI32_T unit,
                                          const void *ptr_sw_gpd, /* SW-GPD to be processed  */
                                          void *ptr_coockie);     /* Private data of SDK     */

typedef struct {
    UI32_T send_ok;
    UI32_T gpd_empty;
    UI32_T poll_timeout;

    /* queue */
    UI32_T enque_ok;
    UI32_T enque_retry;

    /* event */
    UI32_T trig_event;

    /* normal interrupt */
    UI32_T tx_done;

    /* abnormal interrupt */
    UI32_T gpd_hwo_err;        /* bit-0  */
    UI32_T gpd_chksm_err;      /* bit-1  */
    UI32_T gpd_no_ovfl_err;    /* bit-2  */
    UI32_T gpd_dma_read_err;   /* bit-3  */
    UI32_T buf_size_err;       /* bit-4  */
    UI32_T runt_err;           /* bit-5  */
    UI32_T ovsz_err;           /* bit-6  */
    UI32_T len_mismatch_err;   /* bit-7  */
    UI32_T pktpl_dma_read_err; /* bit-8  */
    UI32_T cos_err;            /* bit-9  */
    UI32_T gpd_gt255_err;      /* bit-10 */
    UI32_T pfc;                /* bit-11 */
    UI32_T credit_udfl_err;    /* bit-12 */
    UI32_T dma_write_err;      /* bit-13 */
    UI32_T sw_issue_stop;      /* bit-14 */

    /* others */
    UI32_T err_recover;
    UI32_T ecc_err;

} HAL_LT_DAWN_PKT_TX_CHANNEL_CNT_T;

typedef struct HAL_LT_DAWN_PKT_TX_SW_GPD_S {
    HAL_LT_DAWN_PKT_TX_FUNC_T callback; /* (unit, ptr_sw_gpd, ptr_cookie) */
    void *ptr_cookie;                   /* Pointer of CLX_PKT_TX_PKT_T    */
    HAL_LT_DAWN_PKT_TX_GPD_T tx_gpd;
    UI32_T gpd_num;
    struct HAL_LT_DAWN_PKT_TX_SW_GPD_S *ptr_next;
    UI32_T channel; /* For counter */

} HAL_LT_DAWN_PKT_TX_SW_GPD_T;

typedef struct {
    HAL_LT_DAWN_PKT_TX_CHANNEL_CNT_T channel[HAL_LT_DAWN_PKT_TX_CHANNEL_LAST];
    UI32_T invoke_gpd_callback;
    UI32_T no_memory;

    /* queue */
    UI32_T deque_ok;
    UI32_T deque_fail;

    /* event */
    UI32_T wait_event;

} HAL_LT_DAWN_PKT_TX_CNT_T;

/* ----------------------------------------------------------------------------------- Rx */
typedef enum {
    HAL_LT_DAWN_PKT_RX_CHANNEL_0 = 0,
    HAL_LT_DAWN_PKT_RX_CHANNEL_1,
    HAL_LT_DAWN_PKT_RX_CHANNEL_2,
    HAL_LT_DAWN_PKT_RX_CHANNEL_3,
    HAL_LT_DAWN_PKT_RX_CHANNEL_LAST
} HAL_LT_DAWN_PKT_RX_CHANNEL_T;

typedef struct HAL_LT_DAWN_PKT_RX_SW_GPD_S {
    BOOL_T rx_complete; /* FALSE when PDMA error occurs */
    HAL_LT_DAWN_PKT_RX_GPD_T rx_gpd;
    struct HAL_LT_DAWN_PKT_RX_SW_GPD_S *ptr_next;

#if defined(CLX_EN_NETIF)
    void *ptr_cookie; /* Pointer of virt-addr */
#endif

} HAL_LT_DAWN_PKT_RX_SW_GPD_T;

typedef struct {
    /* queue */
    UI32_T enque_ok;
    UI32_T enque_retry;
    UI32_T deque_ok;
    UI32_T deque_fail;

    /* event */
    UI32_T trig_event;

    /* normal interrupt */
    UI32_T rx_done;

    /* abnormal interrupt */
    UI32_T avbl_gpd_low;   /* bit-0  */
    UI32_T avbl_gpd_empty; /* bit-1  */
    UI32_T avbl_gpd_err;   /* bit-2  */
    UI32_T gpd_chksm_err;  /* bit-3  */
    UI32_T dma_read_err;   /* bit-4  */
    UI32_T dma_write_err;  /* bit-5  */
    UI32_T sw_issue_stop;  /* bit-6  */
    UI32_T gpd_gt255_err;  /* bit-7  */
    UI32_T tod_uninit;     /* bit-8  */
    UI32_T pkt_err_drop;   /* bit-9  */
    UI32_T udsz_drop;      /* bit-10 */
    UI32_T ovsz_drop;      /* bit-11 */
    UI32_T cmdq_ovf_drop;  /* bit-12 */
    UI32_T fifo_ovf_drop;  /* bit-13 */

    /* others */
    UI32_T err_recover;
    UI32_T ecc_err;

#if defined(CLX_EN_NETIF)
    /* it means that user doesn't create intf on that port */
    UI32_T netdev_miss;
#endif

} HAL_LT_DAWN_PKT_RX_CHANNEL_CNT_T;

typedef struct {
    HAL_LT_DAWN_PKT_RX_CHANNEL_CNT_T channel[HAL_LT_DAWN_PKT_RX_CHANNEL_LAST];
    UI32_T invoke_gpd_callback;
    UI32_T no_memory;

    /* event */
    UI32_T wait_event;

} HAL_LT_DAWN_PKT_RX_CNT_T;

/* ----------------------------------------------------------------------------------- Reg */
#if defined(CLX_EN_LITTLE_ENDIAN)
typedef union {
    UI32_T reg;
    struct {
        UI32_T rch_start : 1;
        UI32_T rch_resume : 1;
        UI32_T rch_stop : 1;
        UI32_T : 5;
        UI32_T : 8;
        UI32_T rch_gpd_add_no_lo : 8;
        UI32_T rch_gpd_add_no_hi : 8;
    } field;
} HAL_LT_DAWN_PKT_RCH_CMD_REG_T;

typedef union {
    UI32_T reg;
    struct {
        UI32_T tch_start : 1;
        UI32_T tch_resume : 1;
        UI32_T tch_stop : 1;
        UI32_T : 5;
        UI32_T : 8;
        UI32_T tch_gpd_add_no_lo : 8;
        UI32_T tch_gpd_add_no_hi : 8;
    } field;
} HAL_LT_DAWN_PKT_TCH_CMD_REG_T;

#elif defined(CLX_EN_BIG_ENDIAN)
typedef union {
    UI32_T reg;
    struct {
        UI32_T rch_gpd_add_no_hi : 8;
        UI32_T rch_gpd_add_no_lo : 8;
        UI32_T : 8;
        UI32_T : 5;
        UI32_T rch_stop : 1;
        UI32_T rch_resume : 1;
        UI32_T rch_start : 1;
    } field;
} HAL_LT_DAWN_PKT_RCH_CMD_REG_T;

typedef union {
    UI32_T reg;
    struct {
        UI32_T tch_gpd_add_no_hi : 8;
        UI32_T tch_gpd_add_no_lo : 8;
        UI32_T : 8;
        UI32_T : 5;
        UI32_T tch_stop : 1;
        UI32_T tch_resume : 1;
        UI32_T tch_start : 1;
    } field;
} HAL_LT_DAWN_PKT_TCH_CMD_REG_T;

#else
#error "Host GPD endian is not defined\n"
#endif

/* ----------------------------------------------------------------------------------- CLX_EN_NETIF
 */
#if defined(CLX_EN_NETIF)
/* These requirements come from CLX_NETIF APIs.
 * clx_netif -> hal_pkt_drv -> hal_pkt_knl
 */

typedef struct {
    UI32_T tx_pkt;
    UI32_T tx_queue_full;
    UI32_T tx_error;
    UI32_T rx_pkt;

} HAL_LT_DAWN_PKT_NETIF_INTF_CNT_T;

typedef struct {
    /* unique key */
    UI32_T id;
    C8_T name[CLX_NETIF_NAME_LEN];
    UI32_T port; /* only support unit port and local port */

    /* metadata */
    UI8_T mac[6];

#define HAL_LT_DAWN_PKT_NETIF_INTF_FLAGS_MAC (1UL << 0)
    UI32_T flags;

} HAL_LT_DAWN_PKT_NETIF_INTF_T;

typedef struct {
    C8_T name[CLX_NETLINK_NAME_LEN];
    C8_T mc_group_name[CLX_NETLINK_NAME_LEN];
} HAL_LT_DAWN_PKT_NETIF_RX_DST_NETLINK_T;

typedef enum {
    HAL_LT_DAWN_PKT_NETIF_RX_DST_SDK = 0,
    HAL_LT_DAWN_PKT_NETIF_RX_DST_NETLINK,
    HAL_LT_DAWN_PKT_NETIF_RX_DST_LAST
} HAL_LT_DAWN_PKT_NETIF_RX_DST_TYPE_T;

typedef struct {
    /* unique key */
    UI32_T id;
    C8_T name[CLX_NETIF_NAME_LEN];
    UI32_T priority;

    /* match fields */
    UI32_T port; /* only support unit port and local port */
    HAL_PKT_RX_REASON_BITMAP_T reason_bitmap;
    UI8_T pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define HAL_LT_DAWN_PKT_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define HAL_LT_DAWN_PKT_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define HAL_LT_DAWN_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define HAL_LT_DAWN_PKT_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define HAL_LT_DAWN_PKT_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define HAL_LT_DAWN_PKT_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T flags;

    HAL_LT_DAWN_PKT_NETIF_RX_DST_TYPE_T dst_type;
    HAL_LT_DAWN_PKT_NETIF_RX_DST_NETLINK_T netlink;

} HAL_LT_DAWN_PKT_NETIF_PROFILE_T;

/* These requirements come from CLX_PKT APIs.
 * clx_pkt -> hal_pkt_srv -> hal_pkt_drv -> hal_pkt_knl
 */
typedef enum {
    HAL_LT_DAWN_PKT_IOCTL_RX_TYPE_INIT = 0,
    HAL_LT_DAWN_PKT_IOCTL_RX_TYPE_DEINIT,
    HAL_LT_DAWN_PKT_IOCTL_RX_TYPE_LAST,

} HAL_LT_DAWN_PKT_IOCTL_RX_TYPE_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;
    HAL_LT_DAWN_PKT_RX_CNT_T rx_cnt;
    HAL_LT_DAWN_PKT_TX_CNT_T tx_cnt;
    CLX_ERROR_NO_T rc;

} HAL_LT_DAWN_PKT_IOCTL_CH_CNT_COOKIE_T;

typedef struct {
    UI32_T unit;
    HAL_LT_DAWN_PKT_NETIF_INTF_T net_intf;       /* addIntf[In,Out], delIntf[In]              */
    HAL_LT_DAWN_PKT_NETIF_PROFILE_T net_profile; /* createProfile[In,Out], destroyProfile[In] */
    HAL_LT_DAWN_PKT_NETIF_INTF_CNT_T cnt;
    CLX_ERROR_NO_T rc;

} HAL_LT_DAWN_PKT_IOCTL_NETIF_COOKIE_T;

typedef struct {
    CLX_ADDR_T callback; /* (unit, ptr_sw_gpd, ptr_cookie) */
    CLX_ADDR_T cookie;   /* Pointer of CLX_PKT_TX_PKT_T    */
    UI32_T channel;
    UI32_T gpd_num;
    CLX_ADDR_T hw_gpd_addr;
    CLX_ADDR_T sw_gpd_addr;

} HAL_LT_DAWN_PKT_IOCTL_TX_GPD_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;              /* sendGpd[In]      */
    CLX_ADDR_T ioctl_gpd_addr;   /* sendGpd[In]      */
    CLX_ADDR_T done_sw_gpd_addr; /* waitTxFree[Out]  */

} HAL_LT_DAWN_PKT_IOCTL_TX_COOKIE_T;

typedef struct {
    BOOL_T rx_complete;      /* FALSE when PDMA error occurs                 */
    CLX_ADDR_T hw_gpd_addr;  /* Pointer to HW GPD in user's SW GPD struct    */
    CLX_ADDR_T dma_buf_addr; /* Pointer to DMA buffer allocated by the user (virtual) */

} HAL_LT_DAWN_PKT_IOCTL_RX_GPD_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;                          /* getRxCnt[In], clearRxInt[In]     */
    CLX_ADDR_T ioctl_gpd_addr;               /* waitRxFree[Out]                  */
    UI32_T buf_len;                          /* setRxCfg[In]                     */
    HAL_LT_DAWN_PKT_IOCTL_RX_TYPE_T rx_type; /* setRxCfg[In]                     */
    CLX_ERROR_NO_T rc;
} HAL_LT_DAWN_PKT_IOCTL_RX_COOKIE_T;

typedef struct {
    UI32_T port;
    UI32_T status;
    UI32_T speed;

} HAL_LT_DAWN_PKT_IOCTL_PORT_COOKIE_T;

typedef struct {
    /* intf property */
    UI32_T intf_id;
    CLX_NETIF_INTF_PROPERTY_T property;
    UI32_T param0;
    UI32_T param1;

    /* netlink */
    CLX_NETIF_NETLINK_T netlink;

    CLX_ERROR_NO_T rc;

} HAL_LT_DAWN_PKT_NL_IOCTL_COOKIE_T;

#endif /* End of CLX_EN_NETIF */

/* ----------------------------------------------------------------------------------- pkt_drv */
#if defined(CLX_LAMP)
/**
 * @brief To send a packet to the C-model.
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     channel       - The target TX channel
 * @param [in]     ptr_sw_gpd    - Pointer for the SW Tx GPD link list
 * @return         CLX_E_OK    - Successfully send the packet to the C-model.
 */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_sendGpdToCmodel(const UI32_T unit,
                                const HAL_LT_DAWN_PKT_TX_CHANNEL_T channel,
                                HAL_LT_DAWN_PKT_TX_SW_GPD_T *ptr_sw_gpd);

#else
/**
 * @brief To perform the packet transmission form CPU to the switch.
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     channel       - The target TX channel
 * @param [in]     ptr_sw_gpd    - Pointer for the SW Tx GPD link list
 * @return         CLX_E_OK    - Successfully perform the transferring.
 */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_sendGpd(const UI32_T unit,
                        const HAL_LT_DAWN_PKT_TX_CHANNEL_T channel,
                        HAL_LT_DAWN_PKT_TX_SW_GPD_T *ptr_sw_gpd);

#endif

/* ----------------------------------------------------------------------------------- Deinit */
/**
 * @brief To de-initialize the Task for packet module.
 *
 * @param [in]     unit    - Device unit number
 * @param [in]     ptr_data    - Pointer of the data
 * @return         CLX_E_OK        - Successfully dinitialize the control block.
 * @return         CLX_E_OTHERS    - Initialize the control block failed.
 */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_deinitTask(const UI32_T unit, void *ptr_data);

/**
 * @brief To invoke the functions to de-initialize the control block for each
 *        PDMA subsystem.
 *
 * @param [in]     unit    - Device unit number
 * @param [in]     ptr_data - The pointer to the data
 * @return         CLX_E_OK        - Successfully de-initialize the control blocks.
 * @return         CLX_E_OTHERS    - De-initialize the control blocks failed.
 */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_deinitPktDrv(const UI32_T unit, void *ptr_data);

/* ----------------------------------------------------------------------------------- Init */
/**
 * @brief To initialize the Task for packet module.
 *
 * @param [in]     unit    - Device unit number
 * @param [in]     ptr_data - The data pointer
 * @return         CLX_E_OK        - Successfully dinitialize the control block.
 * @return         CLX_E_OTHERS    - Initialize the control block failed.
 */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_initTask(const UI32_T unit, void *ptr_data);

/**
 * @brief To invoke the functions to initialize the control block for each
 *        PDMA subsystem.
 *
 * @param [in]     unit    - Device unit number
 * @param [in]     ptr_data - The pointer to the data
 * @return         CLX_E_OK        - Successfully initialize the control blocks.
 * @return         CLX_E_OTHERS    - Initialize the control blocks failed.
 */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_initPktDrv(const UI32_T unit, void *ptr_data);

/*---------------------------------------------------------------------------*/
/* perf */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_getTxIntrCnt(const UI32_T unit, const UI32_T channel, UI32_T *ptr_intr_cnt);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_getRxIntrCnt(const UI32_T unit, const UI32_T channel, UI32_T *ptr_intr_cnt);

/* ioctl */
#if defined(CLX_EN_NETIF)
CLX_ERROR_NO_T
hal_lt_dawn_pkt_getTxKnlCnt(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_getRxKnlCnt(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_clearTxKnlCnt(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_clearRxKnlCnt(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_setRxKnlConfig(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_getRxKnlConfig(const UI32_T unit, void *ptr_data);
#endif

/* perf */
CLX_ERROR_NO_T
hal_lt_dawn_pkt_prepareGpd(const UI32_T unit,
                           const CLX_ADDR_T phy_addr,
                           const UI32_T len,
                           const UI32_T port,
                           HAL_LT_DAWN_PKT_TX_SW_GPD_T *ptr_sw_gpd);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_init(const UI32_T unit);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_exit(const UI32_T unit);

#endif /* end of HAL_LT_DAWN_PKT_KNL_H */
