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

/* FILE NAME:  hal_mt_namchabarwa_pkt_knl.h
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 */

#ifndef HAL_MT_NAMCHABARWA_PKT_KNL_H
#define HAL_MT_NAMCHABARWA_PKT_KNL_H

/* INCLUDE FILE DECLARTIONS
 */
#include <clx_types.h>
#include <clx_error.h>
#include <clx_pkt.h>
#include <hal_pkt_rsrc_knl.h>
#include <clx_netif.h>

#define HAL_MT_NAMHABARWA_MSI_NUM             (21)
#define HAL_MT_NAMHABARWA_INTR_ALL_MSI_OFFSET (20)

#define HAL_MT_NAMCHABARWA_PORT_NUM (256)

#define HAL_MT_NAMCHABARWA_GET_BIT(flags, bit)            ((((flags) & (bit)) > 0) ? 1 : 0)
#define HAL_MT_NAMCHABARWA_SET_BIT(bitmap, mask_bitmap)   (bitmap = ((bitmap) | (mask_bitmap)))
#define HAL_MT_NAMCHABARWA_CLEAR_BIT(bitmap, mask_bitmap) (bitmap = ((bitmap) & (~(mask_bitmap))))

#define HAL_MT_NAMCHABARWA_CHAIN28_SLV_INTR_REG (0x587FFFC)
#define HAL_MT_NAMCHABARWA_PDMA_SLV_INTR        (0x1 << 1)

/* pcx top reg definition */
#define HAL_MT_NAMCHABARWA_PCX_TOP_BASE_ADDR (0x051C0000)
#define HAL_MT_NAMCHABARWA_PCX_TOP_GET_ADDR(offset) \
    (HAL_MT_NAMCHABARWA_PCX_TOP_BASE_ADDR + (offset))
#define HAL_MT_NAMCHABARWA_PCX_TOP_SYM_CMST_RESP_ERR_LOG HAL_MT_NAMCHABARWA_PCX_TOP_GET_ADDR(0x28)
#define HAL_MT_NAMCHABARWA_INTR_TOP_MASK_REG             HAL_MT_NAMCHABARWA_PCX_TOP_GET_ADDR(0xBC)

/*PDMA reg definition*/
#define HAL_MT_NAMCHABARWA_PKT_PDMA_BASE_ADDR (0x51c1400)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(offset) \
    (HAL_MT_NAMCHABARWA_PKT_PDMA_BASE_ADDR + (offset))

#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH_ENABLE      HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x4)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DESC_LOCATION  HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x8)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DESC_ENDIAN    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xC)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_DATA_ENDIAN    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x10)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_FIFIO_PATH_SEL HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x30)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CRC_EN         HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x34)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_P2H_RX_FIFO_ALM_FULL \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x38)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_P2H_TX_FIFO_ALM_FULL \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x3c)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_P2E_RX_FIFO_ALM_FULL \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x40)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_P2E_TX_FIFO_ALM_FULL \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x44)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_FIFO_DEBUG_EN HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x50)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xA4)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_PDMA2PCIE_INTR_CLR \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xA8)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_NORMAL_INTR HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xB4)

#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_RING_BASE HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xB8)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_RING_SIZE HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x158)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_DESC_WORK_IDX \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x1A8)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_STA_CH0_DESC_POP_IDX HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x1F8)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_CH0_MODE         HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x248)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_PDMA_CH_RESET    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x348)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESET    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x34C)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFW_CHANNEL_RESTART  HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x350)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_ABNORMAL_INTR    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x354)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_INTR_MSK \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x358)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_CH0_ERR_TYPE \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x3B0)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_STA_PDMA_CH0_OUTSTD_DESC \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0x400)

#define HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_MALFORM_INTR \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xD10)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK_MODE \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xd1c)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_MASK_ALL \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xd20)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CFG_PDMA2PCIE_INTR_CH0_MASK \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xd24)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xd6c)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR_MSK \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xd70)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_IRQ_PDMA_ABNORMAL_CH0_INTR_TST \
    HAL_MT_NAMCHABARWA_PKT_PDMA_GET_ADDR(0xd74)

#define HAL_MT_NAMCHABARWA_PKT_GET_MMIO(__tbl__) (0xFFFFFFFF & (__tbl__))
#define HAL_MT_NAMCHABARWA_PKT_GET_PDMA_DWORD_RCH_REG(__tbl__, __channel__) \
    ((__tbl__) + (0x8 * (__channel__)))
#define HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_RCH_REG(__tbl__, __channel__) \
    ((__tbl__) + (0x4 * (__channel__)))
#define HAL_MT_NAMCHABARWA_PKT_GET_PDMA_DWORD_TCH_REG(__tbl__, __channel__) \
    ((__tbl__) + (0x8 * (__channel__ + 4)))
#define HAL_MT_NAMCHABARWA_PKT_GET_PDMA_WORD_TCH_REG(__tbl__, __channel__) \
    ((__tbl__) + (0x4 * (__channel__ + 4)))

#define HAL_MT_NAMCHABARWA_L2_FIFO_CHANNEL   18
#define HAL_MT_NAMCHABARWA_IOAM_FIFO_CHANNEL 19

/*enable pdma channel*/
#define HAL_MT_NAMCHABARWA_PKT_PDMA_ENABLE_CHANNEL  0x1
#define HAL_MT_NAMCHABARWA_PKT_PDMA_DISABLE_CHANNEL 0x0

#define HAL_MT_NAMCHABARWA_PKT_SWAP16(__data__) (((__data__) >> 8) | ((__data__) << 8))
#define HAL_MT_NAMCHABARWA_PKT_SWAP32(__data__)                                           \
    (((__data__) >> 24) | (((__data__) >> 8) & 0xFF00) | (((__data__) << 8) & 0xFF0000) | \
     ((__data__) << 24))

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_LE16(__data__) HAL_MT_NAMCHABARWA_PKT_SWAP16(__data__)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_LE32(__data__) HAL_MT_NAMCHABARWA_PKT_SWAP32(__data__)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_BE16(__data__)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_BE32(__data__)
#define HAL_MT_NAMCHABARWA_PKT_BE_TO_HOST16(__data__)
#define HAL_MT_NAMCHABARWA_PKT_BE_TO_HOST32(__data__)
#endif

#if defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN) || defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN)

#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_BE16(__data__) HAL_MT_NAMCHABARWA_PKT_SWAP16(__data__)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_BE32(__data__) HAL_MT_NAMCHABARWA_PKT_SWAP32(__data__)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_LE16(__data__)
#define HAL_MT_NAMCHABARWA_PKT_HOST_TO_LE32(__data__)
#define HAL_MT_NAMCHABARWA_PKT_BE_TO_HOST16(__data__) HAL_MT_NAMCHABARWA_PKT_SWAP16(__data__)
#define HAL_MT_NAMCHABARWA_PKT_BE_TO_HOST32(__data__) HAL_MT_NAMCHABARWA_PKT_SWAP32(__data__)

#endif

typedef enum {
    HAL_MT_NAMCHABARWA_PKT_PDMA_CH_MODE_HOSTMEM_TO_HOSTMEM = 0,
    HAL_MT_NAMCHABARWA_PKT_PDMA_CH_MODE_LOCALBUS_TO_HOSTMEM = 1,
    HAL_MT_NAMCHABARWA_PKT_PDMA_CH_MODE_HOSTMEM_TO_LOCALBUS = 2,
    HAL_MT_NAMCHABARWA_PKT_PDMA_CH_MODE_LOCALBUS_TO_LOCALBUS = 3
} HAL_MT_NAMCHABARWA_PKT_PDMA_CH_MODE;

typedef enum {
    HAL_MT_NAMCHABARWA_PKT_PDMA_ACCESS_INDIRECT = 0,
    HAL_MT_NAMCHABARWA_PKT_PDMA_ACCESS_DIRECT = 1
} HAL_MT_NAMCHABARWA_PKT_PDMA_ACCESS_MODE;

/*select pdma byte endian*/
#define HAL_MT_NAMCHABARWA_PKT_PDMA_BYTE_SWAP_DISABLE 0x0
#define HAL_MT_NAMCHABARWA_PKT_PDMA_BYTE_SWAP_ENABLE  0x1

#define HAL_MT_NAMCHABARWA_DMA_PKT_CHANNEL_BITMAP (0xFF)

/* NAMING DECLARATIONS
 */
/* PKT definitions */
#define HAL_MT_NAMCHABARWA_PKT_EMAC_SZ    (12)
#define HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ (40)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_HDR_SZ \
    (HAL_MT_NAMCHABARWA_PKT_EMAC_SZ + HAL_MT_NAMCHABARWA_PKT_PPH_HDR_SZ)
#define HAL_MT_NAMCHABARWA_PKT_TX_MAX_LEN (10200)
#define HAL_MT_NAMCHABARWA_PKT_RX_MAX_LEN \
    (10200 + HAL_MT_NAMCHABARWA_PKT_PDMA_HDR_SZ)      /* EPP tunnel header */
#define HAL_MT_NAMCHABARWA_PKT_MIN_LEN           (64) /* Ethernet definition */
#define HAL_MT_NAMCHABARWA_PKT_TMH_HDR_SZ        (0)
#define HAL_MT_NAMCHABARWA_PKT_CRC_LEN           (4)
#define HAL_MT_NAMCHABARWA_PKT_CPU_PORT(unit)    (256)
#define HAL_MT_NAMCHABARWA_PKT_PORTNUM_PER_SLICE (32)
#define HAL_MT_NAMCHABARWA_PKT_SRC_PORT(slice, port) \
    (slice * HAL_MT_NAMCHABARWA_PKT_PORTNUM_PER_SLICE + port)

/* SOP & EOP */
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CH_PKT_NOT_SOP (0)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CH_PKT_SOP     (1)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CH_PKT_NOT_EOP (0)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CH_PKT_EOP     (1)

/* PRG */
#define HAL_MT_NAMCHABARWA_PKT_PRG_PROCESS_GPD (0) /* Normal   */
#define HAL_MT_NAMCHABARWA_PKT_PRG_SKIP_GPD    (1) /* Skip     */

/* CRCC */
#define HAL_MT_NAMCHABARWA_PKT_CRCC_SUM_BY_HW (0) /* calculated by HW */
#define HAL_MT_NAMCHABARWA_PKT_CRCC_SUM_BY_SW (1) /* calculated by SW */

/* DESCRIPTOR INTERRUPT */
#define HAL_MT_NAMCHABARWA_PKT_DESC_NO_INTR (0) /* trigger interrupt each GPD */
/* trigger interrupt when ch=0, default setting */
#define HAL_MT_NAMCHABARWA_PKT_DESC_HAS_INTR (1)

/* GPD */
#define HAL_MT_NAMCHABARWA_PKT_RX_GPD_NUM (1024)
#define HAL_MT_NAMCHABARWA_PKT_TX_GPD_NUM (1024)

/* ECC */
#define HAL_MT_NAMCHABARWA_PKT_ECC_ERROR_OCCUR (1)

/* CPU, CPI queue number */
#define HAL_MT_NAMCHABARWA_PKT_CPU_QUE_NUM (48)
#define HAL_MT_NAMCHABARWA_PKT_CPI_QUE_NUM (8)

/* PDMA Definitions */
#define HAL_MT_NAMCHABARWA_PKT_PDMA_MAX_GPD_PER_PKT  (100)       /* <= 256   */
#define HAL_MT_NAMCHABARWA_PKT_PDMA_TX_INTR_TIMEOUT  (10 * 1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_PDMA_TX_POLL_MAX_LOOP (10 * 1000) /* int */

/* Mode */
#define HAL_MT_NAMCHABARWA_PKT_TX_WAIT_MODE (HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC)

/* TX Queue */
#define HAL_MT_NAMCHABARWA_PKT_TX_TASK_MAX_LOOP (HAL_DFLT_CFG_PKT_TX_QUEUE_LEN)

/* RX Queue */
#define HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM     (HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST)
#define HAL_MT_NAMCHABARWA_PKT_RX_TASK_MAX_LOOP (HAL_DFLT_CFG_PKT_RX_QUEUE_LEN)

/* MACRO FUNCTION DECLARATIONS
 */
/*---------------------------------------------------------------------------*/
/* Alignment to 16-bytes */
#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_ALIGN_ADDR(pdma_addr, align_sz) \
    (((pdma_addr) + (align_sz)) & 0xFFFFFFFFFFFFFFF0)
#else
#define HAL_MT_NAMCHABARWA_PKT_PDMA_ALIGN_ADDR(pdma_addr, align_sz) \
    (((pdma_addr) + (align_sz)) & 0xFFFFFFF0)
#endif
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_SET_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) | (mask_bitmap)))
#define HAL_MT_NAMCHABARWA_PKT_CLR_BITMAP(bitmap, mask_bitmap) \
    (bitmap = ((bitmap) & (~(mask_bitmap))))
#define HAL_MT_NAMCHABARWA_GET_BITMAP(flags, bit) ((((flags) & (bit)) > 0) ? 1 : 0)

/* ----------------------------------------------------------------------------------- PP Type */
typedef enum {
    HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L2 = 0,
    HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L25,
    HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L3UC = 2,
    HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_L3MC = 2,
    HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_LAST

} HAL_MT_NAMCHABARWA_PKT_PPH_TYPE_T;

/* DATA TYPE DECLARATIONS
 */
typedef enum {
    HAL_MT_NAMCHABARWA_PKT_TX_WAIT_ASYNC = 0,
    HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_INTR = 1,
    HAL_MT_NAMCHABARWA_PKT_TX_WAIT_SYNC_POLL = 2

} HAL_MT_NAMCHABARWA_PKT_TX_WAIT_T;

typedef enum {
    HAL_MT_NAMCHABARWA_PKT_RX_SCHED_RR = 0,
    HAL_MT_NAMCHABARWA_PKT_RX_SCHED_WRR = 1

} HAL_MT_NAMCHABARWA_PKT_RX_SCHED_T;

#pragma pack(1)
#if defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
typedef struct {
    // 32
    UI32_T dst_idx_hi : 9; /*"UC_PORT: 0~2k-1
                            UC_LAG: 3.5k~4k-1
                            TNL: 4k~12k-1
                            MC_L2BD: 12k~28k-1
                            MC_L3MEL: 28k~36k-1"*/
    UI32_T hash_val : 16;  /*for egress mpls/tunnel entropy*/
    UI32_T color : 2;      /*internal drop precedence*/
    UI32_T tc : 3;         /*internal traffic class*/
    UI32_T fwd_op : 2;     /*tenant packet forwarding operation
                               0: L2
                               1: L2.5 MPLS
                               2: L3 UC/L3 MC*/

    // 32
    UI32_T igr_acl_label_hi : 10; /*Reload as IOAM 10b flow id.*/
    UI32_T skip_epp : 1; /*skip EPP if destination is to the CPU port. Keep original packet format*/
    UI32_T
    src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                             4K - 20K: system wide single IP/LSP tunnel index to reach remote peer"*/
    UI32_T dst_idx_lo : 7; /*"UC_PORT: 0~2k-1
                                UC_LAG: 3.5k~4k-1
                                TNL: 4k~12k-1
                                MC_L2BD: 12k~28k-1
                                MC_L3MEL: 28k~36k-1"*/

    // 32
    UI32_T mirror_bmap : 8; /*mirror copy bit map*/
    UI32_T skip_ipp : 1;    /*skip IPP (relevant if ingress from CPU port)*/
    UI32_T slice_id : 3;    /*Ingress slice id. There are 8 slices.*/
    UI32_T die_id : 1;      /*No use, Dont Care*/
    UI32_T port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    UI32_T pkt_journal : 1;      /*instruct downstream pipeline stages to capture states*/
    UI32_T qos_pcp_dei_val : 4;  /*PCP value to mark, or  PCP determined at IDS*/
    UI32_T qos_tnl_uniform : 1;  /*while decap, use outer qos to inner qos*/
    UI32_T qos_dnt_modify : 1;   /*indicating that egress side not rewrite pkg qos*/
    UI32_T igr_acl_label_lo : 6; /*Reload as IOAM 10b flow id.*/

    // 32
    UI32_T evpn_esi_hi : 4; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    UI32_T igr_is_fab : 1;  /*indicate if igr port_type is FAB*/
    UI32_T decap_act : 3;   /*"decap action
                                0: no decap
                                1: MPLS pop or transit
                                2: IP tunnel decap
                                3: erspan termination
                                4: SRH remove, next header field of preceding header may need update
                                5: IPv6 hdr with all its extention hdr
                                6: frc decap ip/mpls"*/
    UI32_T src_bdi : 14;    /*ingress Bdi*/
    UI32_T cpu_reason : 10; /*[9]No use, Dont Care; [8:0] reason code*/

    // 32
    UI32_T tnl_bd_hi : 2;     /*encapsulation adjacency index (MAC DA/VLAN)\*/
    UI32_T tnl_idx : 13;      /*0-8K-1: tunnel index*/
    UI32_T mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    UI32_T evpn_esi_lo : 16;  /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/

    // 32
    UI32_T tapping_push_o : 1;  /*#per flow*/
    UI32_T src_vlan : 12;       /*the default VLAN of the ingress interface or input packet CVLAN*/
    UI32_T pvlan_port_type : 2; /*let RW know to keep the original packet vlan or not*/
    UI32_T igr_vid_pop_num : 3; /*ingress vid num to pop*/
    UI32_T ecn : 2;
    UI32_T ecn_enable : 1;
    UI32_T mpls_ctl : 4;  /*MPLS Encap control bits*/
    UI32_T tnl_bd_lo : 7; /*encapsulation adjacency index (MAC DA/VLAN)\*/

    // 32
    UI32_T : 9;
    UI32_T : 22;
    UI32_T tapping_push_t : 1; /*#per flow*/
    // 32
    UI32_T ptp_info_hi : 6; /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    UI32_T : 2;
    UI32_T mac_learn_en : 1;
    UI32_T : 23;
    // 32
    UI32_T int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    UI32_T int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    UI32_T int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care. N/A
                               for ioam mode*/
    UI32_T ptp_info_lo : 26; /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    // 32
    UI32_T timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/

} HAL_MT_NAMCHABARWA_PKT_PPH_L2_T;

typedef struct {
    // 32
    UI32_T dst_idx_hi : 9; /*"UC_PORT: 0~2k-1
                            UC_LAG: 3.5k~4k-1
                            TNL: 4k~12k-1
                            MC_L2BD: 12k~28k-1
                            MC_L3MEL: 28k~36k-1"*/
    UI32_T hash_val : 16;  /*for egress mpls/tunnel entropy*/
    UI32_T color : 2;      /*internal drop precedence*/
    UI32_T tc : 3;         /*internal traffic class*/
    UI32_T fwd_op : 2;     /*tenant packet forwarding operation
                               0: L2
                               1: L2.5 MPLS
                               2: L3 UC/L3 MC*/

    // 32
    UI32_T igr_acl_label_hi : 10; /*Reload as IOAM 10b flow id.*/
    UI32_T skip_epp : 1; /*skip EPP if destination is to the CPU port. Keep original packet format*/
    UI32_T
    src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                             4K - 20K: system wide single IP/LSP tunnel index to reach remote peer"*/
    UI32_T dst_idx_lo : 7; /*"UC_PORT: 0~2k-1
                                UC_LAG: 3.5k~4k-1
                                TNL: 4k~12k-1
                                MC_L2BD: 12k~28k-1
                                MC_L3MEL: 28k~36k-1"*/

    // 32
    UI32_T mirror_bmap : 8; /*mirror copy bit map*/
    UI32_T skip_ipp : 1;    /*skip IPP (relevant if ingress from CPU port)*/
    UI32_T slice_id : 3;    /*Ingress slice id. There are 8 slices.*/
    UI32_T die_id : 1;      /*No use, Dont Care*/
    UI32_T port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    UI32_T pkt_journal : 1;      /*instruct downstream pipeline stages to capture states*/
    UI32_T qos_pcp_dei_val : 4;  /*PCP value to mark, or  PCP determined at IDS*/
    UI32_T qos_tnl_uniform : 1;  /*while decap, use outer qos to inner qos*/
    UI32_T qos_dnt_modify : 1;   /*indicating that egress side not rewrite pkg qos*/
    UI32_T igr_acl_label_lo : 6; /*Reload as IOAM 10b flow id.*/

    // 32
    UI32_T evpn_esi_hi : 4; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    UI32_T igr_is_fab : 1;  /*indicate if igr port_type is FAB*/
    UI32_T decap_act : 3;   /*"decap action
                                0: no decap
                                1: MPLS pop or transit
                                2: IP tunnel decap
                                3: erspan termination
                                4: SRH remove, next header field of preceding header may need update
                                5: IPv6 hdr with all its extention hdr
                                6: frc decap ip/mpls"*/
    UI32_T src_bdi : 14;    /*ingress Bdi*/
    UI32_T cpu_reason : 10; /*[9]No use, Dont Care; [8:0] reason code*/

    // 32
    UI32_T tnl_bd_hi : 2;     /*encapsulation adjacency index (MAC DA/VLAN)\*/
    UI32_T tnl_idx : 13;      /*0-8K-1: tunnel index*/
    UI32_T mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    UI32_T evpn_esi_lo : 16;  /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/

    // 32
    UI32_T mac_da_hi : 1;      /*#per flow*/
    UI32_T dst_bdi : 14;       /*from lcl*/
    UI32_T mpls_inner_l2 : 1;  /*# MPLS l2 or l3 VPN format*/
    UI32_T decap_prop_ttl : 1; /*propagate TTL*/
    UI32_T decr_ttl : 1;       /*decrement TTL*/
    UI32_T ecn : 2;
    UI32_T ecn_enable : 1;
    UI32_T mpls_ctl : 4;  /*MPLS Encap control bits*/
    UI32_T tnl_bd_lo : 7; /*encapsulation adjacency index (MAC DA/VLAN)\*/

    // 32
    UI32_T mac_da_mi : 32; /*#per flow*/

    // 32
    UI32_T ptp_info_hi : 6;    /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    UI32_T : 1;
    UI32_T srv6_encap_end : 1; /*# SRv6 b6 encaps/insert, bm indicator*/
    UI32_T mac_learn_en : 1;
    UI32_T srv6_func_hit : 1;
    UI32_T srv6_insert_red : 1;
    UI32_T srv6_encaps_red : 1;
    UI32_T usid_arg_en : 1;
    UI32_T usid_func_en : 1;
    UI32_T decr_sl : 1;
    UI32_T nxt_sid_opcode : 2; /*#per flow*/
    UI32_T mac_da_lo : 15;     /*#per flow*/

    // 32
    UI32_T int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    UI32_T int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    UI32_T int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care. N/A
                               for ioam mode*/
    UI32_T ptp_info_lo : 26; /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/

    // 32
    UI32_T timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/

} HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T;

typedef HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T HAL_MT_NAMCHABARWA_PKT_PPH_L3MC_T;

typedef struct {
    // 32
    UI32_T dst_idx_hi : 9; /*"UC_PORT: 0~2k-1
                            UC_LAG: 3.5k~4k-1
                            TNL: 4k~12k-1
                            MC_L2BD: 12k~28k-1
                            MC_L3MEL: 28k~36k-1"*/
    UI32_T hash_val : 16;  /*for egress mpls/tunnel entropy*/
    UI32_T color : 2;      /*internal drop precedence*/
    UI32_T tc : 3;         /*internal traffic class*/
    UI32_T fwd_op : 2;     /*tenant packet forwarding operation
                               0: L2
                               1: L2.5 MPLS
                               2: L3 UC/L3 MC*/

    // 32
    UI32_T igr_acl_label_hi : 10; /*Reload as IOAM 10b flow id.*/
    UI32_T skip_epp : 1; /*skip EPP if destination is to the CPU port. Keep original packet format*/
    UI32_T
    src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                             4K - 20K: system wide single IP/LSP tunnel index to reach remote peer"*/
    UI32_T dst_idx_lo : 7; /*"UC_PORT: 0~2k-1
                                UC_LAG: 3.5k~4k-1
                                TNL: 4k~12k-1
                                MC_L2BD: 12k~28k-1
                                MC_L3MEL: 28k~36k-1"*/

    // 32
    UI32_T mirror_bmap : 8; /*mirror copy bit map*/
    UI32_T skip_ipp : 1;    /*skip IPP (relevant if ingress from CPU port)*/
    UI32_T slice_id : 3;    /*Ingress slice id. There are 8 slices.*/
    UI32_T die_id : 1;      /*No use, Dont Care*/
    UI32_T port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    UI32_T pkt_journal : 1;      /*instruct downstream pipeline stages to capture states*/
    UI32_T qos_pcp_dei_val : 4;  /*PCP value to mark, or  PCP determined at IDS*/
    UI32_T qos_tnl_uniform : 1;  /*while decap, use outer qos to inner qos*/
    UI32_T qos_dnt_modify : 1;   /*indicating that egress side not rewrite pkg qos*/
    UI32_T igr_acl_label_lo : 6; /*Reload as IOAM 10b flow id.*/

    // 32
    UI32_T mpls_lbl_hi : 4; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    UI32_T igr_is_fab : 1;  /*indicate if igr port_type is FAB*/
    UI32_T decap_act : 3;   /*"decap action
                                0: no decap
                                1: MPLS pop or transit
                                2: IP tunnel decap
                                3: erspan termination
                                4: SRH remove, next header field of preceding header may need update
                                5: IPv6 hdr with all its extention hdr
                                6: frc decap ip/mpls"*/
    UI32_T src_bdi : 14;    /*ingress Bdi*/
    UI32_T cpu_reason : 10; /*[9]No use, Dont Care; [8:0] reason code*/

    // 32
    UI32_T tnl_bd_hi : 2;    /*encapsulation adjacency index (MAC DA/VLAN)\*/
    UI32_T tnl_idx : 13;     /*0-8K-1: tunnel index*/
    UI32_T is_swap : 1;      /*MPLS Decap stack has PWCW from ingress LSP*/
    UI32_T mpls_lbl_lo : 16; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/

    // 32
    UI32_T : 23;
    UI32_T decap_prop_ttl : 1; /*propagate TTL*/
    UI32_T decr_ttl : 1;       /*decrement TTL*/
    UI32_T tnl_bd_lo : 7;      /*encapsulation adjacency index (MAC DA/VLAN)\*/

    // 32
    UI32_T : 32;

    // 32
    UI32_T : 8;
    UI32_T mac_learn_en : 1;
    UI32_T : 23;

    // 32
    UI32_T : 32;

    // 32
    UI32_T timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/

} HAL_MT_NAMCHABARWA_PKT_PPH_L25_T;
#endif
#if defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN)
typedef struct {
    UI32_T fwd_op : 2;    /*tenant packet forwarding operation
                              0: L2
                              1: L2.5 MPLS
                              2: L3 UC/L3 MC*/
    UI32_T tc : 3;        /*internal traffic class*/
    UI32_T color : 2;     /*internal drop precedence*/
    UI32_T hash_val : 16; /*for egress mpls/tunnel entropy*/
    UI32_T dst_idx : 16;  /*"UC_PORT: 0~2k-1
                              UC_LAG: 3.5k~4k-1
                              TNL: 4k~12k-1
                              MC_L2BD: 12k~28k-1
                              MC_L3MEL: 28k~36k-1"*/
    UI32_T
    src_idx : 14;         /*"0 - 4K: system wide physical port/lag
                              4K - 20K: system wide single IP/LSP tunnel index to reach remote peer"*/
    UI32_T skip_epp : 1; /*skip EPP if destination is to the CPU port. Keep original packet format*/
    UI32_T igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    UI32_T qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    UI32_T qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    UI32_T qos_pcp_dei_val : 4; /*PCP value to mark, or  PCP determined at IDS*/
    UI32_T pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    UI32_T port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    UI32_T die_id : 1;   /*No use, Dont Care*/
    UI32_T slice_id : 3; /*Ingress slice id. There are 8 slices.*/
    UI32_T skip_ipp : 1; /*skip IPP (relevant if ingress from CPU port)*/
    UI32_T mirror_bmap : 8;   /*mirror copy bit map*/
    UI32_T cpu_reason : 10;   /*[9]No use, Dont Care; [8:0] reason code*/
    UI32_T src_bdi : 14;      /*ingress Bdi*/
    UI32_T decap_act : 3;     /*"decap action
                                  0: no decap
                                  1: MPLS pop or transit
                                  2: IP tunnel decap
                                  3: erspan termination
                                  4: SRH remove, next header field of preceding header may need update
                                  5: IPv6 hdr with all its extention hdr
                                  6: frc decap ip/mpls"*/
    UI32_T igr_is_fab : 1;    /*indicate if igr port_type is FAB*/
    UI32_T evpn_esi : 20;     /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    UI32_T mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    UI32_T tnl_idx : 13;      /*0-8K-1: tunnel index*/
    UI32_T tnl_bd : 9;        /*encapsulation adjacency index (MAC DA/VLAN)\*/

    UI32_T mpls_ctl : 4;      /*MPLS Encap control bits*/

    UI32_T ecn_enable : 1;
    UI32_T ecn : 2;
    UI32_T igr_vid_pop_num : 3; /*ingress vid num to pop*/
    UI32_T pvlan_port_type : 2; /*let RW know to keep the original packet vlan or not*/
    UI32_T src_vlan : 12;       /*the default VLAN of the ingress interface or input packet CVLAN*/
    UI32_T tapping_push_o : 1;  /*#per flow*/
    UI32_T tapping_push_t : 1;  /*#per flow*/
    UI32_T : 22;
    UI32_T : 32;
    UI32_T mac_learn_en : 1;
    UI32_T : 2;
    UI32_T ptp_info : 32;   /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    UI32_T int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care. N/A
                               for ioam mode*/
    UI32_T int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    UI32_T int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    UI32_T timestamp : 32;  /*time stamp 2-bit sec; 30-bit ns*/

} HAL_MT_NAMCHABARWA_PKT_PPH_L2_T;

typedef struct {
    UI32_T fwd_op : 2;    /*tenant packet forwarding operation
                              0: L2
                              1: L2.5 MPLS
                              2: L3 UC/L3 MC*/
    UI32_T tc : 3;        /*internal traffic class*/
    UI32_T color : 2;     /*internal drop precedence*/
    UI32_T hash_val : 16; /*for egress mpls/tunnel entropy*/
    UI32_T dst_idx : 16;  /*"UC_PORT: 0~2k-1
                              UC_LAG: 3.5k~4k-1
                              TNL: 4k~12k-1
                              MC_L2BD: 12k~28k-1
                              MC_L3MEL: 28k~36k-1"*/
    UI32_T
    src_idx : 14;         /*"0 - 4K: system wide physical port/lag
                              4K - 20K: system wide single IP/LSP tunnel index to reach remote peer"*/
    UI32_T skip_epp : 1; /*skip EPP if destination is to the CPU port. Keep original packet format*/
    UI32_T igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    UI32_T qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    UI32_T qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    UI32_T qos_pcp_dei_val : 4; /*PCP value to mark, or  PCP determined at IDS*/
    UI32_T pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    UI32_T port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    UI32_T die_id : 1;   /*No use, Dont Care*/
    UI32_T slice_id : 3; /*Ingress slice id. There are 8 slices.*/
    UI32_T skip_ipp : 1; /*skip IPP (relevant if ingress from CPU port)*/
    UI32_T mirror_bmap : 8;   /*mirror copy bit map*/
    UI32_T cpu_reason : 10;   /*[9]No use, Dont Care; [8:0] reason code*/
    UI32_T src_bdi : 14;      /*ingress Bdi*/
    UI32_T decap_act : 3;     /*"decap action
                                  0: no decap
                                  1: MPLS pop or transit
                                  2: IP tunnel decap
                                  3: erspan termination
                                  4: SRH remove, next header field of preceding header may need update
                                  5: IPv6 hdr with all its extention hdr
                                  6: frc decap ip/mpls"*/
    UI32_T igr_is_fab : 1;    /*indicate if igr port_type is FAB*/
    UI32_T evpn_esi : 20;     /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    UI32_T mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    UI32_T tnl_idx : 13;      /*0-8K-1: tunnel index*/
    UI32_T tnl_bd : 9;        /*encapsulation adjacency index (MAC DA/VLAN)\*/
    UI32_T mpls_ctl : 4;      /*MPLS Encap control bits*/
    UI32_T ecn_enable : 1;
    UI32_T ecn : 2;
    UI32_T decr_ttl : 1;       /*decrement TTL*/
    UI32_T decap_prop_ttl : 1; /*propagate TTL*/
    UI32_T mpls_inner_l2 : 1;  /*# MPLS l2 or l3 VPN format*/
    UI32_T dst_bdi : 14;       /*from lcl*/
    UI64_T mac_da : 48;        /*#per flow*/
    UI32_T nxt_sid_opcode : 2; /*#per flow*/
    UI32_T decr_sl : 1;
    UI32_T usid_func_en : 1;
    UI32_T usid_arg_en : 1;
    UI32_T srv6_encaps_red : 1;
    UI32_T srv6_insert_red : 1;
    UI32_T srv6_func_hit : 1;
    UI32_T mac_learn_en : 1;
    UI32_T srv6_encap_end : 1; /*# SRv6 b6 encaps/insert, bm indicator*/
    UI32_T : 1;
    UI32_T ptp_info : 32;      /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    UI32_T int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care. N/A
                               for ioam mode*/
    UI32_T int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    UI32_T int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    UI32_T timestamp : 32;  /*time stamp 2-bit sec; 30-bit ns*/
} HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T;

typedef HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T HAL_MT_NAMCHABARWA_PKT_PPH_L3MC_T;

typedef struct {
    UI32_T fwd_op : 2;    /*tenant packet forwarding operation
                              0: L2
                              1: L2.5 MPLS
                              2: L3 UC/L3 MC*/
    UI32_T tc : 3;        /*internal traffic class*/
    UI32_T color : 2;     /*internal drop precedence*/
    UI32_T hash_val : 16; /*for egress mpls/tunnel entropy*/
    UI32_T dst_idx : 16;  /*"UC_PORT: 0~2k-1
                              UC_LAG: 3.5k~4k-1
                              TNL: 4k~12k-1
                              MC_L2BD: 12k~28k-1
                              MC_L3MEL: 28k~36k-1"*/
    UI32_T
    src_idx : 14;         /*"0 - 4K: system wide physical port/lag
                              4K - 20K: system wide single IP/LSP tunnel index to reach remote peer"*/
    UI32_T skip_epp : 1; /*skip EPP if destination is to the CPU port. Keep original packet format*/
    UI32_T igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    UI32_T qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    UI32_T qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    UI32_T qos_pcp_dei_val : 4; /*PCP value to mark, or  PCP determined at IDS*/
    UI32_T pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    UI32_T port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    UI32_T die_id : 1;   /*No use, Dont Care*/
    UI32_T slice_id : 3; /*Ingress slice id. There are 8 slices.*/
    UI32_T skip_ipp : 1; /*skip IPP (relevant if ingress from CPU port)*/
    UI32_T mirror_bmap : 8;    /*mirror copy bit map*/
    UI32_T cpu_reason : 10;    /*[9]No use, Dont Care; [8:0] reason code*/
    UI32_T src_bdi : 14;       /*ingress Bdi*/
    UI32_T decap_act : 3;      /*"decap action
                                   0: no decap
                                   1: MPLS pop or transit
                                   2: IP tunnel decap
                                   3: erspan termination
                                   4: SRH remove, next header field of preceding header may need update
                                   5: IPv6 hdr with all its extention hdr
                                   6: frc decap ip/mpls"*/
    UI32_T igr_is_fab : 1;     /*indicate if igr port_type is FAB*/
    UI32_T mpls_lbl : 20;      /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    UI32_T is_swap : 1;        /*MPLS Decap stack has PWCW from ingress LSP*/
    UI32_T tnl_idx : 13;       /*0-8K-1: tunnel index*/
    UI32_T tnl_bd : 9;         /*encapsulation adjacency index (MAC DA/VLAN)\*/
    UI32_T decr_ttl : 1;       /*decrement TTL*/
    UI32_T decap_prop_ttl : 1; /*propagate TTL*/
    UI32_T : 14;
    UI64_T : 64;
    UI32_T mac_learn_en : 1;
    UI32_T : 8;
    UI32_T : 32;
    UI32_T timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/
} HAL_MT_NAMCHABARWA_PKT_PPH_L25_T;
#endif
#pragma pack()

#if defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)

// dst idx bit2 bit7
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_DST_IDX(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->dst_idx_hi << 7 | ptr_pph->dst_idx_lo) : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_DST_IDX(ptr_pph, dst_idx) \
    {                                                            \
        if (ptr_pph != NULL) {                                   \
            ptr_pph->dst_idx_hi = (dst_idx >> 7) & 0x3;          \
            ptr_pph->dst_idx_lo = dst_idx & 0x7f;                \
        }                                                        \
    }

// igr_acl_label bit10 bit6
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_IGR_ACL_LABLE(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->igr_acl_label_hi << 6 | ptr_pph->igr_acl_label_lo) : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_IGR_ACL_LABLE(ptr_pph, igr_acl_label) \
    {                                                                        \
        if (ptr_pph != NULL) {                                               \
            ptr_pph->igr_acl_label_hi = (igr_acl_label >> 6) & 0x3ff;        \
            ptr_pph->igr_acl_label_lo = igr_acl_label & 0x3f;                \
        }                                                                    \
    }

// evpn_esi bit4 bit16
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_EVPN_ESI(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->evpn_esi_hi << 16 | ptr_pph->evpn_esi_lo) : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_EVPN_ESI(ptr_pph, evpn_esi) \
    {                                                              \
        if (ptr_pph != NULL) {                                     \
            ptr_pph->evpn_esi_hi = (evpn_esi >> 16) & 0xf;         \
            ptr_pph->evpn_esi_lo = evpn_esi & 0xffff;              \
        }                                                          \
    }

// tnl_bd bit2 bit7
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_TNL_BD(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->tnl_bd_hi << 7 | ptr_pph->tnl_bd_lo) : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_TNL_BD(ptr_pph, tnl_bd) \
    {                                                          \
        if (ptr_pph != NULL) {                                 \
            ptr_pph->tnl_bd_hi = (tnl_bd >> 7) & 0x3;          \
            ptr_pph->tnl_bd_lo = tnl_bd & 0x7f;                \
        }                                                      \
    }

// ptp_info bit6 bit26
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_PTP_INFO(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->ptp_info_hi << 26 | ptr_pph->ptp_info_lo) : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_PTP_INFO(ptr_pph, ptp_info) \
    {                                                              \
        if (ptr_pph != NULL) {                                     \
            ptr_pph->ptp_info_hi = (ptp_info >> 26) & 0x3ffffff;   \
            ptr_pph->ptp_info_lo = ptp_info & 0x3f;                \
        }                                                          \
    }

// mac_da bit1 bit32 bit15
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_MAC_DA(ptr_pph)                                           \
    ((ptr_pph != NULL) ? ((UI64_T)((UI64_T)ptr_pph->mac_da_hi << 47 | ptr_pph->mac_da_mi << 15 | \
                                   ptr_pph->mac_da_lo)) :                                        \
                         0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_MAC_DA(ptr_pph, mac_da) \
    {                                                          \
        if (ptr_pph != NULL) {                                 \
            ptr_pph->mac_da_hi = (mac_da >> 47) & 0x1;         \
            ptr_pph->mac_da_mi = (mac_da >> 15) & 0xffffffff;  \
            ptr_pph->mac_da_lo = mac_da & 0x7fff;              \
        }                                                      \
    }

// mpls_lbl bit4 bit16
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_MPLS_LBL(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->mpls_lbl_hi << 16 | ptr_pph->mpls_lbl_lo) : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_MPLS_LBL(ptr_pph, mpls_lbl) \
    {                                                              \
        if (ptr_pph != NULL) {                                     \
            ptr_pph->mpls_lbl_hi = (mpls_lbl >> 16) & 0xf;         \
            ptr_pph->mpls_lbl_lo = mpls_lbl & 0xffff;              \
        }                                                          \
    }

#endif
#if defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN)
// dst_idx
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_DST_IDX(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->dst_idx : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_DST_IDX(ptr_pph, dst_idx) \
    {                                                            \
        if (ptr_pph != NULL) {                                   \
            ptr_pph->dst_idx = dst_idx;                          \
        }                                                        \
    }

// igr_acl_label
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_IGR_ACL_LABLE(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->igr_acl_label : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_IGR_ACL_LABLE(ptr_pph, igr_acl_label) \
    {                                                                        \
        if (ptr_pph != NULL) {                                               \
            ptr_pph->igr_acl_label = igr_acl_label;                          \
        }                                                                    \
    }

// evpn_esi
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_EVPN_ESI(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->evpn_esi : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_EVPN_ESI(ptr_pph, evpn_esi) \
    {                                                              \
        if (ptr_pph != NULL) {                                     \
            ptr_pph->evpn_esi = evpn_esi;                          \
        }                                                          \
    }

// tnl_bd
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_TNL_BD(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->tnl_bd : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_TNL_BD(ptr_pph, tnl_bd) \
    {                                                          \
        if (ptr_pph != NULL) {                                 \
            ptr_pph->tnl_bd = tnl_bd;                          \
        }                                                      \
    }

// ptp_info
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_PTP_INFO(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->ptp_info : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_PTP_INFO(ptr_pph, ptp_info) \
    {                                                              \
        if (ptr_pph != NULL) {                                     \
            ptr_pph->ptp_info = ptp_info;                          \
        }                                                          \
    }

// mac_da
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_MAC_DA(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->mac_da : 0xffffffffffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_MAC_DA(ptr_pph, mac_da) \
    {                                                          \
        if (ptr_pph != NULL) {                                 \
            ptr_pph->mac_da = mac_da;                          \
        }                                                      \
    }

// mpls_lbl
#define HAL_MT_NAMCHABARWA_PKT_PPH_GET_MPLS_LBL(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->mpls_lbl : 0xffffffff)
#define HAL_MT_NAMCHABARWA_PKT_PPH_SET_MPLS_LBL(ptr_pph, mpls_lbl) \
    {                                                              \
        if (ptr_pph != NULL) {                                     \
            ptr_pph->mpls_lbl = mpls_lbl;                          \
        }                                                          \
    }

#endif

#pragma pack(1)
/* GPD STRUCTURE */

/*
 * descriptor
    63---------------47-----------------------------------------------0
    |       size     |                  s_addr                        |
    |      [63:48]   |                  [47:0]                        |
0x8  ----------------|------------------------------------------------|
    |      status    |                  d_addr                        |
    |      [63:48]   |                  [47:0]                        |
0x10 ----------------|------------------------------------------------|

*/
typedef struct {
    UI64_T s_addr_lo : 32;
    UI64_T s_addr_hi : 16;
    UI64_T size : 16;
    UI64_T d_addr_lo : 32;
    UI64_T d_addr_hi : 16;

    /*status[127:112]*/
    UI64_T interrupt : 1;
    UI64_T err : 1;
    UI64_T eop : 1;
    UI64_T sop : 1;
    UI64_T sinc : 1;
    UI64_T dinc : 1;
    UI64_T xfer_size : 5;
    UI64_T limit_xfer_en : 1;
    UI64_T reserve : 4;

} HAL_MT_NAMCHABARWA_PKT_GPD_T;
#pragma pack()
typedef HAL_MT_NAMCHABARWA_PKT_GPD_T HAL_MT_NAMCHABARWA_PKT_TX_GPD_T;
typedef HAL_MT_NAMCHABARWA_PKT_GPD_T HAL_MT_NAMCHABARWA_PKT_RX_GPD_T;

#define HAL_MT_NAMCHABARWA_PKT_NUM_GPDS(data) \
    ((data + sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T) - 1) / sizeof(HAL_MT_NAMCHABARWA_PKT_RX_GPD_T))

/* ----------------------------------------------------------------------------------- Tx */
typedef enum {
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_0 = 0,
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_1,
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_2,
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_3,
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST

} HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T;

#define HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL(channel_offset) (channel_offset + 4)

typedef void (*HAL_MT_NAMCHABARWA_PKT_TX_FUNC_T)(const UI32_T unit,
                                                 const void *ptr_sw_gpd, /* SW-GPD to be processed
                                                                          */
                                                 void *ptr_coockie); /* Private data of SDK     */

typedef struct HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_S {
    HAL_MT_NAMCHABARWA_PKT_TX_FUNC_T callback; /* (unit, ptr_sw_gpd, ptr_cookie) */
    void *ptr_cookie;                          /* Pointer of CLX_PKT_TX_PKT_T    */
    HAL_MT_NAMCHABARWA_PKT_TX_GPD_T tx_gpd;
    union {
        HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *ptr_pph_l2;
        HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T *ptr_pph_l3uc;
        HAL_MT_NAMCHABARWA_PKT_PPH_L3MC_T *ptr_pph_l3mc;
        HAL_MT_NAMCHABARWA_PKT_PPH_L25_T *ptr_pph_l25;
    };
    UI32_T gpd_num;
    struct HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_S *ptr_next;
    UI32_T channel; /* For counter */
} HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T;

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
    UI32_T rd_desc_err; /* bit-0  */
    UI32_T wr_desc_err; /* bit-1  */
    UI32_T rd_data_err; /* bit-2  */
    UI32_T wr_data_err; /* bit-3  */

    /* others */
    UI32_T err_recover;
    UI32_T ecc_err;

} HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_CNT_T;

typedef struct {
    HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_CNT_T channel[HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_LAST];
    UI32_T invoke_gpd_callback;
    UI32_T no_memory;

    /* queue */
    UI32_T deque_ok;
    UI32_T deque_fail;

    /* event */
    UI32_T wait_event;

} HAL_MT_NAMCHABARWA_PKT_TX_CNT_T;

/* ----------------------------------------------------------------------------------- Rx */
typedef enum {
    HAL_MT_NAMCHABARWA_PKT_C_NEXT = 0, /* callback continuous */
    HAL_MT_NAMCHABARWA_PKT_C_STOP = 1,
    HAL_MT_NAMCHABARWA_PKT_C_OTHERS = 2
} HAL_MT_NAMCHABARWA_PKT_CALLBACK_NO_T;

typedef enum {
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_0 = 0,
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_1,
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_2,
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_3,
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST
} HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_T;

#define HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL(channel_offset) (channel_offset)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_RD_DESC_ERROR (1 << 0)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_WR_DESC_ERROR (1 << 1)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_RD_DATA_ERROR (1 << 2)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CHANNEL_WR_DATA_ERROR (1 << 3)

#define HAL_MT_NAMCHABARWA_PKT_PDMA_PCIE_AR_ITF_HANG (1 << 0)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_PCIE_R_ITF_HANG  (1 << 1)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_PCIE_AW_ITF_HANG (1 << 2)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_PCIE_W_ITF_HANG  (1 << 3)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_PCIE_B_ITF_HANG  (1 << 4)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CMST_AR_ITF_HANG (1 << 5)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CMST_R_ITF_HANG  (1 << 6)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CMST_AW_ITF_HANG (1 << 7)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CMST_W_ITF_HANG  (1 << 8)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_CMST_B_ITF_HANG  (1 << 9)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO0_OVF (1 << 10)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO1_OVF (1 << 11)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO2_OVF (1 << 12)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_P2H_RX_FIFO3_OVF (1 << 13)
#define HAL_MT_NAMCHABARWA_PKT_PDMA_P2E_RX_FIFO_OVF  (1 << 14)

typedef struct HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_S {
    BOOL_T rx_complete; /* FALSE when PDMA error occurs */
    HAL_MT_NAMCHABARWA_PKT_RX_GPD_T rx_gpd;
    union {
        HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *ptr_pph_l2;
        HAL_MT_NAMCHABARWA_PKT_PPH_L3UC_T *ptr_pph_l3uc;
        HAL_MT_NAMCHABARWA_PKT_PPH_L3MC_T *ptr_pph_l3mc;
        HAL_MT_NAMCHABARWA_PKT_PPH_L25_T *ptr_pph_l25;
    };
    struct HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_S *ptr_next;

    void *ptr_cookie; /* Pointer of virt-addr */

} HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T;

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
    UI32_T rd_desc_err; /* bit-0  */
    UI32_T wr_desc_err; /* bit-1  */
    UI32_T rd_data_err; /* bit-2  */
    UI32_T wr_data_err; /* bit-3  */

    /* malform interrupt */
    UI32_T p2h_rx_fifo_ovf; /* bit-10 11 12 13 */

    /* others */
    UI32_T err_recover;
    UI32_T ecc_err;

#if defined(CLX_EN_NETIF)
    /* it means that user doesn't create intf on that port */
    UI32_T netdev_miss;
#endif

} HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_CNT_T;

typedef struct {
    HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_CNT_T channel[HAL_MT_NAMCHABARWA_PKT_RX_CHANNEL_LAST];
    UI32_T invoke_gpd_callback;
    UI32_T no_memory;

    /* event */
    UI32_T wait_event;

} HAL_MT_NAMCHABARWA_PKT_RX_CNT_T;

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
} HAL_MT_NAMCHABARWA_PKT_RCH_CMD_REG_T;

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
} HAL_MT_NAMCHABARWA_PKT_TCH_CMD_REG_T;

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
} HAL_MT_NAMCHABARWA_PKT_RCH_CMD_REG_T;

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
} HAL_MT_NAMCHABARWA_PKT_TCH_CMD_REG_T;

#else
#error "Host GPD endian is not defined\n"
#endif

/* ----------------------------------------------------------------------------------- CLX_EN_NETIF
 */
#if defined(CLX_EN_NETIF)
/* These requirements come from CLX_NETIF APIs.
 * clx_netif -> hal_mt_namchabarwa_pkt_drv -> hal_mt_namchabarwa_pkt_knl
 */

typedef struct {
    UI32_T tx_pkt;
    UI32_T tx_queue_full;
    UI32_T tx_error;
    UI32_T rx_pkt;

} HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_CNT_T;

typedef struct {
    /* unique key */
    UI32_T id;
    C8_T name[CLX_NETIF_NAME_LEN];
    UI32_T port; /* only support unit port and local port */
    UI32_T slice;
    UI32_T slice_port;

    /* metadata */
    UI8_T mac[6];

#define HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_FLAGS_MAC           (1UL << 0)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_FLAGS_VLAN_TAG_TYPE (1UL << 1)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_FLAGS_VLAN_TAG      (1UL << 2)
    UI32_T flags;

#define HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_FLAGS_VLAN_TAG_STRIP    (0)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_FLAGS_VLAN_TAG_KEEP     (1)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_FLAGS_VLAN_TAG_ORIGINAL (2)
    UI8_T vlan_tag_type; /* 0:VLAN_TAG_STRIP 1:VLAN_TAG_KEEP 2:VLAN_TAG_ORIGINAL*/

} HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T;

#endif /* End of CLX_EN_NETIF */

typedef struct {
    C8_T name[CLX_NETLINK_NAME_LEN];
    C8_T mc_group_name[CLX_NETLINK_NAME_LEN];
} HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_NETLINK_T;

typedef enum {
    HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_SDK = 0,
    HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_NETLINK,
    HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_LAST
} HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_TYPE_T;

#define HAL_PKT_PP_NUM         (512)
#define HAL_PKT_PP_BITMAP_SIZE (CLX_BITMAP_SIZE(HAL_PKT_PP_NUM))
typedef UI32_T HAL_MT_PKT_PP_REASON_BITMAP_T[HAL_PKT_PP_BITMAP_SIZE];

typedef struct {
    /* unique key */
    UI32_T id;
    C8_T name[CLX_NETIF_NAME_LEN];
    UI32_T priority;

    /* match fields */
    UI32_T port; /* only support unit port and local port */
    HAL_MT_PKT_PP_REASON_BITMAP_T reason_bitmap;
    UI8_T pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T flags;

    HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_TYPE_T dst_type;
    HAL_MT_NAMCHABARWA_PKT_NETIF_RX_DST_NETLINK_T netlink;

} HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T;

/* These requirements come from CLX_PKT APIs.
 * clx_pkt -> hal_mt_namchabarwa_pkt_srv -> hal_mt_namchabarwa_pkt_drv -> hal_mt_namchabarwa_pkt_knl
 */
typedef enum {
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_INIT = 0,
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_DEINIT,
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_LAST,

} HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;
    HAL_MT_NAMCHABARWA_PKT_RX_CNT_T rx_cnt;
    HAL_MT_NAMCHABARWA_PKT_TX_CNT_T tx_cnt;
    CLX_ERROR_NO_T rc;

} HAL_MT_NAMCHABARWA_PKT_IOCTL_CH_CNT_COOKIE_T;

typedef struct {
    UI32_T unit;
#if defined(CLX_EN_NETIF)
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_T net_intf; /* addIntf[In,Out], delIntf[In]              */
    HAL_MT_NAMCHABARWA_PKT_NETIF_INTF_CNT_T cnt;
#endif
    HAL_MT_NAMCHABARWA_PKT_NETIF_PROFILE_T
    net_profile; /* createProfile[In,Out], destroyProfile[In] */
    CLX_ERROR_NO_T rc;

} HAL_MT_NAMCHABARWA_PKT_IOCTL_NETIF_COOKIE_T;

typedef struct {
    CLX_ADDR_T callback; /* (unit, ptr_sw_gpd, ptr_cookie) */
    CLX_ADDR_T cookie;   /* Pointer of CLX_PKT_TX_PKT_T    */
    UI32_T channel;
    UI32_T gpd_num;
    CLX_ADDR_T hw_gpd_addr;
    CLX_ADDR_T sw_gpd_addr;

} HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;              /* sendGpd[In]      */
    CLX_ADDR_T ioctl_gpd_addr;   /* sendGpd[In]      */
    CLX_ADDR_T done_sw_gpd_addr; /* waitTxFree[Out]  */
    CLX_ERROR_NO_T rc;
} HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T;

typedef struct {
    BOOL_T rx_complete;      /* FALSE when PDMA error occurs                 */
    CLX_ADDR_T hw_gpd_addr;  /* Pointer to HW GPD in user's SW GPD struct    */
    CLX_ADDR_T dma_buf_addr; /* Pointer to DMA buffer allocated by the user (virtual) */

} HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T;

typedef struct {
    UI32_T channel;      /* initPktDrv[Out]      */
    CLX_ADDR_T phy_addr; /* initPktDrv[Out] DMA ring base phy addr*/
} HAL_MT_NAMCHABARWA_PKT_IOCTL_CHANNEL_RING_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;                                 /* getRxCnt[In], clearRxInt[In]     */
    CLX_ADDR_T ioctl_gpd_addr;                      /* waitRxFree[Out]                  */
    UI32_T buf_len;                                 /* setRxCfg[In]                     */
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_TYPE_T rx_type; /* setRxCfg[In]                     */
    CLX_ERROR_NO_T rc;
} HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T;

typedef struct {
    UI32_T port;
    UI32_T status;
    UI32_T speed;

} HAL_MT_NAMCHABARWA_PKT_IOCTL_PORT_COOKIE_T;

typedef struct {
    /* intf property */
    UI32_T intf_id;
    CLX_NETIF_INTF_PROPERTY_T property;
    UI32_T param0;
    UI32_T param1;

    /* netlink */
    CLX_NETIF_NETLINK_T netlink;

    CLX_ERROR_NO_T rc;

} HAL_MT_NAMCHABARWA_PKT_NL_IOCTL_COOKIE_T;

typedef struct {
    UI32_T unit;
    BOOL_T get;        /* ture:get false:set */
    UI8_T mod_dmac[6]; /* mod mac  */
} HAL_MT_NAMCHABARWA_PKT_IOCTL_MOD_COOKIE_T;

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
hal_mt_namchabarwa_pkt_sendGpdToCmodel(const UI32_T unit,
                                       const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T channel,
                                       HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *ptr_sw_gpd);

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
hal_mt_namchabarwa_pkt_sendGpd(const UI32_T unit,
                               const HAL_MT_NAMCHABARWA_PKT_TX_CHANNEL_T channel,
                               HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *ptr_sw_gpd);

#endif

/* ----------------------------------------------------------------------------------- pkt_srv */
/**
 * @brief To transfer the packet form CPU to the switch.
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     channel       - The target TX channel
 * @param [in]     ptr_tx_pkt    - Pointer for the TX packet
 * @return         CLX_E_OK    - Successfully transfer the packet.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_sendPacket(const UI32_T unit,
                                  const UI32_T channel,
                                  const CLX_PKT_TX_PKT_T *ptr_tx_pkt);

/**
 * @brief To invoke the user callback function for the RX packet.
 *
 * The packet will also be store in the packet monitor.
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     ptr_sw_gpd    - Pointer for the SW Tx GPD link list
 * @param [in]     ptr_cookie    - The Rx callback cookie
 * @return         CLX_E_OK    - Successfully invoke the callback function.
 */
HAL_MT_NAMCHABARWA_PKT_CALLBACK_NO_T
hal_mt_namchabarwa_pkt_invokeRxUsrCallback(const UI32_T unit,
                                           HAL_MT_NAMCHABARWA_PKT_RX_SW_GPD_T *ptr_sw_gpd,
                                           void *ptr_cookie);

/* ----------------------------------------------------------------------------------- Deinit */
/**
 * @brief To de-initialize the Task for packet module.
 *
 * @param [in]     unit         - Device unit number
 * @param [in]     ptr_data     - The pointer of data
 * @return         CLX_E_OK        - Successfully dinitialize the control block.
 * @return         CLX_E_OTHERS    - Initialize the control block failed.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_deinitTask(const UI32_T unit, void *ptr_data);

/**
 * @brief To invoke the functions to de-initialize the control block for each
 *        PDMA subsystem.
 *
 * @param [in]     unit    - Device unit number
 * @param [in]     ptr_data     - The pointer of data
 * @return         CLX_E_OK        - Successfully de-initialize the control blocks.
 * @return         CLX_E_OTHERS    - De-initialize the control blocks failed.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_deinitPktDrv(const UI32_T unit, void *ptr_data);

/* ----------------------------------------------------------------------------------- Init */
/**
 * @brief To initialize the Task for packet module.
 *
 * @param [in]     unit         - Device unit number
 * @param [in]     ptr_data     - The pointer of data
 * @return         CLX_E_OK        - Successfully dinitialize the control block.
 * @return         CLX_E_OTHERS    - Initialize the control block failed.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_initTask(const UI32_T unit, void *ptr_data);

/**
 * @brief To invoke the functions to initialize the control block for each
 *        PDMA subsystem.
 *
 * @param [in]     unit            - Device unit number
 * @param [in]     ptr_data     - The pointer of data
 * @return         CLX_E_OK        - Successfully initialize the control blocks.
 * @return         CLX_E_OTHERS    - Initialize the control blocks failed.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_initPktDrv(const UI32_T unit, void *ptr_data);

/**
 * @brief To initialize the packet module.
 *
 * @param [in]     unit    - Device unit number
 * @return         CLX_E_OK        - Successfully initialize the packet module.
 * @return         CLX_E_OTHERS    - Initialize the packet module failed.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_init(const UI32_T unit);

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_exit(const UI32_T unit);

/*---------------------------------------------------------------------------*/
/* perf */
/**
 * @brief To get the PDMA TX interrupt counters of the target channel.
 *
 * @param [in]     unit         - Device unit number
 * @param [in]     channel      - The target channel
 * @param [out]    ptr_intr_cnt - The Pointer of intr cnt
 * @return         CLX_E_OK    - Successfully get the counters.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getTxIntrCnt(const UI32_T unit, const UI32_T channel, UI32_T *ptr_intr_cnt);

/**
 * @brief To get the PDMA RX interrupt counters of the target channel.
 *
 * @param [in]     unit         - Device unit number
 * @param [in]     channel      - The target channel
 * @param [out]    ptr_intr_cnt - The Pointer of intr cnt
 * @return         CLX_E_OK    - Successfully get the counters.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxIntrCnt(const UI32_T unit, const UI32_T channel, UI32_T *ptr_intr_cnt);

/* ioctl */
#if defined(CLX_EN_NETIF)
/**
 * @brief To get the PDMA TX counters of the target channel.
 *
 * @param [in]     unit          - Device unit number
 * @param [out]    ptr_data      - Pointer of the data
 * @return         CLX_E_OK    - Successfully get the counters.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getTxKnlCnt(const UI32_T unit, void *ptr_data);

/**
 * @brief To get the PDMA RX counters of the target channel.
 *
 * @param [in]     unit          - Device unit number
 * @param [out]    ptr_data      - Pointer of the data
 * @return         CLX_E_OK    - Successfully get the counters.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxKnlCnt(const UI32_T unit, void *ptr_data);

/**
 * @brief To clear the PDMA TX counters of the target channel.
 *
 * @param [in]     unit          - Device unit number
 * @param [out]    ptr_data      - Pointer of the data
 * @return         CLX_E_OK    - Successfully clear the counters.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_clearTxKnlCnt(const UI32_T unit, void *ptr_data);

/**
 * @brief To clear the PDMA RX counters of the target channel.
 *
 * @param [in]     unit          - Device unit number
 * @param [out]    ptr_data      - Pointer of the data
 * @return         CLX_E_OK    - Successfully clear the counters.
 */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_clearRxKnlCnt(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_setRxKnlConfig(const UI32_T unit, void *ptr_data);

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxKnlConfig(const UI32_T unit, void *ptr_data);
#endif

/* perf */
CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_prepareGpd(const UI32_T unit,
                                  const CLX_ADDR_T phy_addr,
                                  const UI32_T len,
                                  HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *ptr_sw_gpd);

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_preparePPh(const UI32_T unit,
                                  const UI32_T port,
                                  HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *ptr_pph);

#endif /* end of HAL_MT_NAMCHABARWA_PKT_KNL_H */
