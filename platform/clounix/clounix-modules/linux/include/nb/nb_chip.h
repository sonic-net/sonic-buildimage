/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
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

#ifndef __NB_CHIP_H__
#define __NB_CHIP_H__

#define NB_DMA_SCRATCH                    0x051C1400
#define NB_CTL_CHAIN_INTR_MSK             0x051C00BC
#define NB_TOP_STA_TOP_LVL_INTR_RAW       0x051C00C0
#define NB_TOP_STA_TOP_LVL_INTR           0x051C00C8
#define NB_CFG_PDMA_CH_ENABLE             0X051C1404
#define NB_CFG_PDMA_DESC_LOCATION         0x051C1408
#define NB_CFG_PDMA_DESC_ENDIAN           0x051C140C
#define NB_CFG_PDMA_CRC_EN                0x051C1434
#define NB_CFG_PDMA_DATA_ENDIAN_SWAP      0x051C1410
#define NB_CFG_AXI_PROTO_INFO             0x051C1414
#define NB_CFG_AXI_FIFO_ALM_FULL          0x051C1418
#define NB_CFG_AXI_TIMEOUT_THR            0x051C141C
#define NB_CFG_AXI0_OUTSTD_SIZE           0x051C1428
#define NB_CFG_AXI1_OUTSTD_SIZE           0x051C142C
#define NB_CFG_FIFIO_PATH_SEL             0x051C1430
#define NB_CFG_CRC_EN                     0x051C1434
#define NB_CFG_P2H_RX_FIFO_ALM_FULL       0x051C1438
#define NB_CFG_P2H_TX_FIFO_ALM_FULL       0x051C143c
#define NB_CFG_P2E_RX_FIFO_ALM_FULL       0x051C1440
#define NB_CFG_P2E_TX_FIFO_ALM_FULL       0x051C1444
#define NB_CFG_TX_FIFO_TIMEOUT_THR        0x051C1448
#define NB_CFG_SLV_TIMEOUT_THR            0x051C144C
#define NB_CFG_PDMA2PCIE_INTR_MASK        0X051C14A4
#define NB_CFW_PDMA2PCIE_INTR_CLR         0X051C14A8
#define NB_CFW_PDMA2PCIE_INTR_TEST        0X051C14AC
#define NB_STA_PDMA_NORMAL_INTR           0X051C14B4
#define NB_CFG_PDMA_CH0_RING_BASE         0X051C14B8
#define NB_CFG_PDMA_CH0_RING_SIZE         0X051C1558
#define NB_CFG_PDMA_CH0_DESC_WORK_IDX     0X051C15A8
#define NB_STA_PDMA_CH0_DESC_POP_IDX      0X051C15F8
#define NB_CFG_PDMA_CH0_MODE              0x051C1648
#define NB_CFW_PDMA_CH_RESET              0X051C174C
#define NB_CFW_PDMA_CH_RESTART            0X051C1750
#define NB_CFG_PDMA2PCIE_INTR_MASK_ALL    0X051C2120
#define NB_CFG_PDMA2PCIE_INTR_CH0_MASK    0X051C2124
#define NB_IRQ_PDMA_ABNORMAL_CH0_INTR     0X051C216C
#define NB_IRQ_PDMA_ABNORMAL_CH0_INTR_MSK 0X051C2170

#define NB_CHAIN_INTR_SIZE 32
#define L2_FIFO_CHANNEL    18
#define IOAM_FIFO_CHANNEL  19
#define NUM_MSI_IRQ        21

#define NB_CFG_PDMA_CHx_RING_BASE(__channel__) (NB_CFG_PDMA_CH0_RING_BASE + 0x8 * (__channel__))
#define NB_CFG_PDMA_CHx_RING_SIZE(__channel__) (NB_CFG_PDMA_CH0_RING_SIZE + 0x4 * (__channel__))
#define NB_CFG_PDMA_CHx_DESC_WORK_IDX(__channel__) \
    (NB_CFG_PDMA_CH0_DESC_WORK_IDX + 0x4 * (__channel__))
#define NB_STA_PDMA_CHx_DESC_POP_IDX(__channel__) \
    (NB_STA_PDMA_CH0_DESC_POP_IDX + 0x4 * (__channel__))
#define NB_CFG_PDMA_CHx_MODE(__channel__) (NB_CFG_PDMA_CH0_MODE + 0x4 * (__channel__))

#define CFG_PDMA2PCIE_INTR_CHx_MASK(__channel__)                               \
    (NB_CFG_PDMA2PCIE_INTR_CH0_MASK +                                          \
     ((__channel__) == L2_FIFO_CHANNEL || (__channel__) == IOAM_FIFO_CHANNEL ? \
          ((__channel__ - 2) * 4) :                                            \
          ((__channel__) * 4)))

#define IRQ_PDMA_ABNORMAL_CHx_INTR(__channel__)                                \
    (NB_IRQ_PDMA_ABNORMAL_CH0_INTR +                                           \
     ((__channel__) == L2_FIFO_CHANNEL || (__channel__) == IOAM_FIFO_CHANNEL ? \
          ((__channel__ - 2) * 0xC) :                                          \
          ((__channel__) * 0xC)))

#define IRQ_PDMA_ABNORMAL_CHx_INTR_MSK(__channel__)                            \
    (NB_IRQ_PDMA_ABNORMAL_CH0_INTR_MSK +                                       \
     ((__channel__) == L2_FIFO_CHANNEL || (__channel__) == IOAM_FIFO_CHANNEL ? \
          ((__channel__ - 2) * 0xC) :                                          \
          ((__channel__) * 0xC)))

#define CHAIN28_SLV_INTR_REG (0x587FFFC)
#define PDMA_ERROR_IRQ_BIT   (0x1 << 1)

#define NB_PKT_EMAC_SZ               (12)
#define NB_PKT_PPH_HDR_SZ            (40)
#define NB_PKT_PDMA_HDR_SZ           (NB_PKT_EMAC_SZ + NB_PKT_PPH_HDR_SZ)
#define NB_MAX_PKT_SIZE              (10200)
#define NB_PKT_TX_MAX_LEN            (NB_MAX_PKT_SIZE)
#define NB_PKT_RX_MAX_LEN            (NB_MAX_PKT_SIZE + NB_PKT_PDMA_HDR_SZ) /* EPP tunnel header */
#define NB_PKT_MIN_LEN               (64) /* Ethernet definition */
#define NB_PKT_CPU_PORT              (256)
#define NB_PKT_PORTNUM_PER_SLICE     (40)
#define NB_PKT_SRC_PORT(slice, port) (slice * NB_PKT_PORTNUM_PER_SLICE + port)

#define NB_SET_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) | (mask_bitmap)))
#define NB_CLR_BITMAP(bitmap, mask_bitmap) (bitmap = ((bitmap) & (~(mask_bitmap))))
#define NB_GET_BITMAP(flags, bit)          ((((flags) & (bit)) > 0) ? 1 : 0)

/* cpu reason */
#define NB_PKT_RX_MOD_REASON        (511)
#define NB_PKT_RX_EGR_SFLOW_SAMPLER (480)
#define NB_PKT_RX_IGR_SFLOW_SAMPLER (352)

#define NB_PKT_RX_IFA2_REASON (495)

typedef struct {
    uint32_t s_addr_lo : 32;
    uint32_t s_addr_hi : 16;
    uint32_t size : 16;
    uint32_t d_addr_lo : 32;
    uint32_t d_addr_hi : 16;

    /*status[127:112]*/
    uint32_t interrupt : 1;
    uint32_t err : 1;
    uint32_t eop : 1;
    uint32_t sop : 1;
    uint32_t sinc : 1;
    uint32_t dinc : 1;
    uint32_t xfer_size : 5;
    uint32_t limit_xfer_en : 1;
    uint32_t reserve : 4;

} nb_descriptor_t;

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define NB_DESCRIPTOR_ALIGN_ADDR(pdma_addr, align_sz) \
    (((pdma_addr) + (align_sz)) & 0xFFFFFFFFFFFFFFF0)
#else
#define NB_DESCRIPTOR_ALIGN_ADDR(pdma_addr, align_sz) (((pdma_addr) + (align_sz)) & 0xFFFFFFF0)
#endif

#define NB_PKT_SWAP16(__data__) (((__data__) >> 8) | ((__data__) << 8))
#define NB_PKT_SWAP32(__data__)                                                           \
    (((__data__) >> 24) | (((__data__) >> 8) & 0xFF00) | (((__data__) << 8) & 0xFF0000) | \
     ((__data__) << 24))

typedef enum {
    NB_PKT_PPH_TYPE_L2 = 0,
    NB_PKT_PPH_TYPE_L25,
    NB_PKT_PPH_TYPE_L3UC = 2,
    NB_PKT_PPH_TYPE_L3MC = 2,
    NB_PKT_PPH_TYPE_LAST

} NB_PKT_PPH_TYPE_T;

#pragma pack(push, 1)
#if defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
typedef struct {
    // 32
    uint32_t dst_idx_hi : 9; /*"UC_PORT: 0~2k-1
                            UC_LAG: 3.5k~4k-1
                            TNL: 4k~12k-1
                            MC_L2BD: 12k~28k-1
                            MC_L3MEL: 28k~36k-1"*/
    uint32_t hash_val : 16;  /*for egress mpls/tunnel entropy*/
    uint32_t color : 2;      /*internal drop precedence*/
    uint32_t tc : 3;         /*internal traffic class*/
    uint32_t fwd_op : 2;     /*tenant packet forwarding operation
                               0: L2
                               1: L2.5 MPLS
                               2: L3 UC/L3 MC*/

    // 32
    uint32_t igr_acl_label_hi : 10; /*Reload as IOAM 10b flow id.*/
    uint32_t skip_epp : 1;   /*skip EPP if destination is to the CPU port. Keep original packet
                                format*/
    uint32_t src_idx : 14;   /*"0 - 4K: system wide physical port/lag
                               4K - 20K: system wide single IP/LSP tunnel index to reach remote
                              peer"*/
    uint32_t dst_idx_lo : 7; /*"UC_PORT: 0~2k-1
                                UC_LAG: 3.5k~4k-1
                                TNL: 4k~12k-1
                                MC_L2BD: 12k~28k-1
                                MC_L3MEL: 28k~36k-1"*/

    // 32
    uint32_t mirror_bmap : 8; /*mirror copy bit map*/
    uint32_t skip_ipp : 1;    /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t slice_id : 3;    /*Ingress slice id. There are 8 slices.*/
    uint32_t die_id : 1;      /*No use, Dont Care*/
    uint32_t port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    uint32_t pkt_journal : 1;      /*instruct downstream pipeline stages to capture states*/
    uint32_t qos_pcp_dei_val : 4;  /*PCP value to mark, or  PCP determined at IDS*/
    uint32_t qos_tnl_uniform : 1;  /*while decap, use outer qos to inner qos*/
    uint32_t qos_dnt_modify : 1;   /*indicating that egress side not rewrite pkg qos*/
    uint32_t igr_acl_label_lo : 6; /*Reload as IOAM 10b flow id.*/

    // 32
    uint32_t evpn_esi_hi : 4; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t igr_is_fab : 1;  /*indicate if igr port_type is FAB*/
    uint32_t decap_act : 3;   /*"decap action
                                0: no decap
                                1: MPLS pop or transit
                                2: IP tunnel decap
                                3: erspan termination
                                4: SRH remove, next header field of preceding header may need update
                                5: IPv6 hdr with all its extention hdr
                                6: frc decap ip/mpls"*/
    uint32_t src_bdi : 14;    /*ingress Bdi*/
    uint32_t cpu_reason : 10; /*[9]No use, Dont Care; [8:0] reason code*/

    // 32
    uint32_t tnl_bd_hi : 2;     /*encapsulation adjacency index (MAC DA/VLAN)\*/
    uint32_t tnl_idx : 13;      /*0-8K-1: tunnel index*/
    uint32_t mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t evpn_esi_lo : 16;  /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/

    // 32
    uint32_t tapping_push_o : 1; /*#per flow*/
    uint32_t src_vlan : 12;      /*the default VLAN of the ingress interface or input packet CVLAN*/
    uint32_t pvlan_port_type : 2; /*let RW know to keep the original packet vlan or not*/
    uint32_t igr_vid_pop_num : 3; /*ingress vid num to pop*/
    uint32_t ecn : 2;
    uint32_t ecn_enable : 1;
    uint32_t mpls_ctl : 4;  /*MPLS Encap control bits*/
    uint32_t tnl_bd_lo : 7; /*encapsulation adjacency index (MAC DA/VLAN)\*/

    // 32
    uint32_t : 9;
    uint32_t : 22;
    uint32_t tapping_push_t : 1; /*#per flow*/
    // 32
    uint32_t ptp_info_hi : 6; /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    uint32_t : 2;
    uint32_t mac_learn_en : 1;
    uint32_t : 23;
    // 32
    uint32_t int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    uint32_t int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    uint32_t int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care.
                               N/A for ioam mode*/
    uint32_t ptp_info_lo : 26; /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    // 32
    uint32_t timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/

} nb_pkt_pph_l2_t;

typedef struct {
    // 32
    uint32_t dst_idx_hi : 9; /*"UC_PORT: 0~2k-1
                            UC_LAG: 3.5k~4k-1
                            TNL: 4k~12k-1
                            MC_L2BD: 12k~28k-1
                            MC_L3MEL: 28k~36k-1"*/
    uint32_t hash_val : 16;  /*for egress mpls/tunnel entropy*/
    uint32_t color : 2;      /*internal drop precedence*/
    uint32_t tc : 3;         /*internal traffic class*/
    uint32_t fwd_op : 2;     /*tenant packet forwarding operation
                               0: L2
                               1: L2.5 MPLS
                               2: L3 UC/L3 MC*/

    // 32
    uint32_t igr_acl_label_hi : 10; /*Reload as IOAM 10b flow id.*/
    uint32_t skip_epp : 1;   /*skip EPP if destination is to the CPU port. Keep original packet
                                format*/
    uint32_t src_idx : 14;   /*"0 - 4K: system wide physical port/lag
                               4K - 20K: system wide single IP/LSP tunnel index to reach remote
                              peer"*/
    uint32_t dst_idx_lo : 7; /*"UC_PORT: 0~2k-1
                                UC_LAG: 3.5k~4k-1
                                TNL: 4k~12k-1
                                MC_L2BD: 12k~28k-1
                                MC_L3MEL: 28k~36k-1"*/

    // 32
    uint32_t mirror_bmap : 8; /*mirror copy bit map*/
    uint32_t skip_ipp : 1;    /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t slice_id : 3;    /*Ingress slice id. There are 8 slices.*/
    uint32_t die_id : 1;      /*No use, Dont Care*/
    uint32_t port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    uint32_t pkt_journal : 1;      /*instruct downstream pipeline stages to capture states*/
    uint32_t qos_pcp_dei_val : 4;  /*PCP value to mark, or  PCP determined at IDS*/
    uint32_t qos_tnl_uniform : 1;  /*while decap, use outer qos to inner qos*/
    uint32_t qos_dnt_modify : 1;   /*indicating that egress side not rewrite pkg qos*/
    uint32_t igr_acl_label_lo : 6; /*Reload as IOAM 10b flow id.*/

    // 32
    uint32_t evpn_esi_hi : 4; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t igr_is_fab : 1;  /*indicate if igr port_type is FAB*/
    uint32_t decap_act : 3;   /*"decap action
                                0: no decap
                                1: MPLS pop or transit
                                2: IP tunnel decap
                                3: erspan termination
                                4: SRH remove, next header field of preceding header may need update
                                5: IPv6 hdr with all its extention hdr
                                6: frc decap ip/mpls"*/
    uint32_t src_bdi : 14;    /*ingress Bdi*/
    uint32_t cpu_reason : 10; /*[9]No use, Dont Care; [8:0] reason code*/

    // 32
    uint32_t tnl_bd_hi : 2;     /*encapsulation adjacency index (MAC DA/VLAN)\*/
    uint32_t tnl_idx : 13;      /*0-8K-1: tunnel index*/
    uint32_t mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t evpn_esi_lo : 16;  /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/

    // 32
    uint32_t mac_da_hi : 1;      /*#per flow*/
    uint32_t dst_bdi : 14;       /*from lcl*/
    uint32_t mpls_inner_l2 : 1;  /*# MPLS l2 or l3 VPN format*/
    uint32_t decap_prop_ttl : 1; /*propagate TTL*/
    uint32_t decr_ttl : 1;       /*decrement TTL*/
    uint32_t ecn : 2;
    uint32_t ecn_enable : 1;
    uint32_t mpls_ctl : 4;  /*MPLS Encap control bits*/
    uint32_t tnl_bd_lo : 7; /*encapsulation adjacency index (MAC DA/VLAN)\*/

    // 32
    uint32_t mac_da_mi : 32; /*#per flow*/

    // 32
    uint32_t ptp_info_hi : 6;    /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    uint32_t : 1;
    uint32_t srv6_encap_end : 1; /*# SRv6 b6 encaps/insert, bm indicator*/
    uint32_t mac_learn_en : 1;
    uint32_t srv6_func_hit : 1;
    uint32_t srv6_insert_red : 1;
    uint32_t srv6_encaps_red : 1;
    uint32_t usid_arg_en : 1;
    uint32_t usid_func_en : 1;
    uint32_t decr_sl : 1;
    uint32_t nxt_sid_opcode : 2; /*#per flow*/
    uint32_t mac_da_lo : 15;     /*#per flow*/

    // 32
    uint32_t int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    uint32_t int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    uint32_t int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care.
                               N/A for ioam mode*/
    uint32_t ptp_info_lo : 26; /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/

    // 32
    uint32_t timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/

} nb_pkt_pph_l3uc_t;

typedef nb_pkt_pph_l3uc_t nb_pkt_pph_l3mc_t;

typedef struct {
    // 32
    uint32_t dst_idx_hi : 9; /*"UC_PORT: 0~2k-1
                            UC_LAG: 3.5k~4k-1
                            TNL: 4k~12k-1
                            MC_L2BD: 12k~28k-1
                            MC_L3MEL: 28k~36k-1"*/
    uint32_t hash_val : 16;  /*for egress mpls/tunnel entropy*/
    uint32_t color : 2;      /*internal drop precedence*/
    uint32_t tc : 3;         /*internal traffic class*/
    uint32_t fwd_op : 2;     /*tenant packet forwarding operation
                               0: L2
                               1: L2.5 MPLS
                               2: L3 UC/L3 MC*/

    // 32
    uint32_t igr_acl_label_hi : 10; /*Reload as IOAM 10b flow id.*/
    uint32_t skip_epp : 1;   /*skip EPP if destination is to the CPU port. Keep original packet
                                format*/
    uint32_t src_idx : 14;   /*"0 - 4K: system wide physical port/lag
                               4K - 20K: system wide single IP/LSP tunnel index to reach remote
                              peer"*/
    uint32_t dst_idx_lo : 7; /*"UC_PORT: 0~2k-1
                                UC_LAG: 3.5k~4k-1
                                TNL: 4k~12k-1
                                MC_L2BD: 12k~28k-1
                                MC_L3MEL: 28k~36k-1"*/

    // 32
    uint32_t mirror_bmap : 8; /*mirror copy bit map*/
    uint32_t skip_ipp : 1;    /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t slice_id : 3;    /*Ingress slice id. There are 8 slices.*/
    uint32_t die_id : 1;      /*No use, Dont Care*/
    uint32_t port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    uint32_t pkt_journal : 1;      /*instruct downstream pipeline stages to capture states*/
    uint32_t qos_pcp_dei_val : 4;  /*PCP value to mark, or  PCP determined at IDS*/
    uint32_t qos_tnl_uniform : 1;  /*while decap, use outer qos to inner qos*/
    uint32_t qos_dnt_modify : 1;   /*indicating that egress side not rewrite pkg qos*/
    uint32_t igr_acl_label_lo : 6; /*Reload as IOAM 10b flow id.*/

    // 32
    uint32_t mpls_lbl_hi : 4; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t igr_is_fab : 1;  /*indicate if igr port_type is FAB*/
    uint32_t decap_act : 3;   /*"decap action
                                0: no decap
                                1: MPLS pop or transit
                                2: IP tunnel decap
                                3: erspan termination
                                4: SRH remove, next header field of preceding header may need update
                                5: IPv6 hdr with all its extention hdr
                                6: frc decap ip/mpls"*/
    uint32_t src_bdi : 14;    /*ingress Bdi*/
    uint32_t cpu_reason : 10; /*[9]No use, Dont Care; [8:0] reason code*/

    // 32
    uint32_t tnl_bd_hi : 2;    /*encapsulation adjacency index (MAC DA/VLAN)\*/
    uint32_t tnl_idx : 13;     /*0-8K-1: tunnel index*/
    uint32_t is_swap : 1;      /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t mpls_lbl_lo : 16; /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/

    // 32
    uint32_t : 23;
    uint32_t decap_prop_ttl : 1; /*propagate TTL*/
    uint32_t decr_ttl : 1;       /*decrement TTL*/
    uint32_t tnl_bd_lo : 7;      /*encapsulation adjacency index (MAC DA/VLAN)\*/

    // 32
    uint32_t : 32;

    // 32
    uint32_t : 8;
    uint32_t mac_learn_en : 1;
    uint32_t : 23;

    // 32
    uint32_t : 32;

    // 32
    uint32_t timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/

} nb_pkt_pph_l25_t;

#define NB_PKT_HOST_TO_BE16(__data__) NB_PKT_SWAP16(__data__)
#define NB_PKT_HOST_TO_BE32(__data__) NB_PKT_SWAP32(__data__)
#define NB_PKT_HOST_TO_LE16(__data__) (__data__)
#define NB_PKT_HOST_TO_LE32(__data__) (__data__)
#define NB_PKT_BE_TO_HOST16(__data__) NB_PKT_SWAP16(__data__)
#define NB_PKT_BE_TO_HOST32(__data__) NB_PKT_SWAP32(__data__)

// dst idx bit2 bit7
#define nb_pkt_pph_get_dst_idx(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->dst_idx_hi << 7 | ptr_pph->dst_idx_lo) : 0xffffffff)
#define nb_pkt_pph_set_dst_idx(ptr_pph, dst_idx)          \
    {                                                     \
        if (ptr_pph != NULL) {                            \
            ptr_pph->dst_idx_hi = (dst_idx >> 7) & 0x1ff; \
            ptr_pph->dst_idx_lo = dst_idx & 0x7f;         \
        }                                                 \
    }

// igr_acl_label bit10 bit6
#define nb_pkt_pph_get_igr_acl_lable(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->igr_acl_label_hi << 6 | ptr_pph->igr_acl_label_lo) : 0xffffffff)
#define nb_pkt_pph_set_igr_acl_lable(ptr_pph, igr_acl_label)          \
    {                                                                 \
        if (ptr_pph != NULL) {                                        \
            ptr_pph->igr_acl_label_hi = (igr_acl_label >> 6) & 0x3ff; \
            ptr_pph->igr_acl_label_lo = igr_acl_label & 0x3f;         \
        }                                                             \
    }

// evpn_esi bit4 bit16
#define nb_pkt_pph_get_evpn_esi(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->evpn_esi_hi << 16 | ptr_pph->evpn_esi_lo) : 0xffffffff)
#define nb_pkt_pph_set_evpn_esi(ptr_pph, evpn_esi)         \
    {                                                      \
        if (ptr_pph != NULL) {                             \
            ptr_pph->evpn_esi_hi = (evpn_esi >> 16) & 0xf; \
            ptr_pph->evpn_esi_lo = evpn_esi & 0xffff;      \
        }                                                  \
    }

// tnl_bd bit2 bit7
#define nb_pkt_pph_get_tnl_bd(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->tnl_bd_hi << 7 | ptr_pph->tnl_bd_lo) : 0xffffffff)
#define nb_pkt_pph_set_tnl_bd(ptr_pph, tnl_bd)        \
    {                                                 \
        if (ptr_pph != NULL) {                        \
            ptr_pph->tnl_bd_hi = (tnl_bd >> 7) & 0x3; \
            ptr_pph->tnl_bd_lo = tnl_bd & 0x7f;       \
        }                                             \
    }

// ptp_info bit6 bit26
#define nb_pkt_pph_get_ptp_info(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->ptp_info_hi << 26 | ptr_pph->ptp_info_lo) : 0xffffffff)
#define nb_pkt_pph_set_ptp_info(ptr_pph, ptp_info)          \
    {                                                       \
        if (ptr_pph != NULL) {                              \
            ptr_pph->ptp_info_hi = (ptp_info >> 26) & 0x3f; \
            ptr_pph->ptp_info_lo = ptp_info & 0x3ffffff;    \
        }                                                   \
    }

// mac_da bit1 bit32 bit15
#define nb_pkt_pph_get_mac_da(ptr_pph)                                                 \
    ((ptr_pph != NULL) ? ((uint64_t)((uint64_t)ptr_pph->mac_da_hi << 47 |              \
                                     ptr_pph->mac_da_mi << 15 | ptr_pph->mac_da_lo)) : \
                         0xffffffff)
#define nb_pkt_pph_set_mac_da(ptr_pph, mac_da)                \
    {                                                         \
        if (ptr_pph != NULL) {                                \
            ptr_pph->mac_da_hi = (mac_da >> 47) & 0x1;        \
            ptr_pph->mac_da_mi = (mac_da >> 15) & 0xffffffff; \
            ptr_pph->mac_da_lo = mac_da & 0x7fff;             \
        }                                                     \
    }

// mpls_lbl bit4 bit16
#define nb_pkt_pph_get_mpls_lbl(ptr_pph) \
    ((ptr_pph != NULL) ? (ptr_pph->mpls_lbl_hi << 16 | ptr_pph->mpls_lbl_lo) : 0xffffffff)
#define nb_pkt_pph_set_mpls_lbl(ptr_pph, mpls_lbl)         \
    {                                                      \
        if (ptr_pph != NULL) {                             \
            ptr_pph->mpls_lbl_hi = (mpls_lbl >> 16) & 0xf; \
            ptr_pph->mpls_lbl_lo = mpls_lbl & 0xffff;      \
        }                                                  \
    }

#elif defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN)
typedef struct {
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t hash_val : 16;       /*for egress mpls/tunnel entropy*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t qos_pcp_dei_val : 4; /*PCP value to mark, or  PCP determined at IDS*/
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    uint32_t die_id : 1;   /*No use, Dont Care*/
    uint32_t slice_id : 3; /*Ingress slice id. There are 8 slices.*/
    uint32_t skip_ipp : 1; /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t mirror_bmap : 8;   /*mirror copy bit map*/
    uint32_t cpu_reason : 10;   /*[9]No use, Dont Care; [8:0] reason code*/
    uint32_t src_bdi : 14;      /*ingress Bdi*/
    uint32_t decap_act : 3;     /*"decap action
                                  0: no decap
                                  1: MPLS pop or transit
                                  2: IP tunnel decap
                                  3: erspan termination
                                  4: SRH remove, next header field of preceding header may need update
                                  5: IPv6 hdr with all its extention hdr
                                  6: frc decap ip/mpls"*/
    uint32_t igr_is_fab : 1;    /*indicate if igr port_type is FAB*/
    uint32_t evpn_esi : 20;     /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t tnl_idx : 13;      /*0-8K-1: tunnel index*/
    uint32_t tnl_bd : 9;        /*encapsulation adjacency index (MAC DA/VLAN)\*/

    uint32_t mpls_ctl : 4;      /*MPLS Encap control bits*/

    uint32_t ecn_enable : 1;
    uint32_t ecn : 2;
    uint32_t igr_vid_pop_num : 3; /*ingress vid num to pop*/
    uint32_t pvlan_port_type : 2; /*let RW know to keep the original packet vlan or not*/
    uint32_t src_vlan : 12;      /*the default VLAN of the ingress interface or input packet CVLAN*/
    uint32_t tapping_push_o : 1; /*#per flow*/
    uint32_t tapping_push_t : 1; /*#per flow*/
    uint32_t : 22;
    uint32_t : 32;
    uint32_t mac_learn_en : 1;
    uint32_t : 2;
    uint32_t ptp_info : 32;   /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    uint32_t int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care.
                               N/A for ioam mode*/
    uint32_t int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    uint32_t int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    uint32_t timestamp : 32;  /*time stamp 2-bit sec; 30-bit ns*/

} nb_pkt_pph_l2_t;

typedef struct {
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t hash_val : 16;       /*for egress mpls/tunnel entropy*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t qos_pcp_dei_val : 4; /*PCP value to mark, or  PCP determined at IDS*/
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    uint32_t die_id : 1;   /*No use, Dont Care*/
    uint32_t slice_id : 3; /*Ingress slice id. There are 8 slices.*/
    uint32_t skip_ipp : 1; /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t mirror_bmap : 8;   /*mirror copy bit map*/
    uint32_t cpu_reason : 10;   /*[9]No use, Dont Care; [8:0] reason code*/
    uint32_t src_bdi : 14;      /*ingress Bdi*/
    uint32_t decap_act : 3;     /*"decap action
                                  0: no decap
                                  1: MPLS pop or transit
                                  2: IP tunnel decap
                                  3: erspan termination
                                  4: SRH remove, next header field of preceding header may need update
                                  5: IPv6 hdr with all its extention hdr
                                  6: frc decap ip/mpls"*/
    uint32_t igr_is_fab : 1;    /*indicate if igr port_type is FAB*/
    uint32_t evpn_esi : 20;     /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t mpls_pwcw_vld : 1; /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t tnl_idx : 13;      /*0-8K-1: tunnel index*/
    uint32_t tnl_bd : 9;        /*encapsulation adjacency index (MAC DA/VLAN)\*/
    uint32_t mpls_ctl : 4;      /*MPLS Encap control bits*/
    uint32_t ecn_enable : 1;
    uint32_t ecn : 2;
    uint32_t decr_ttl : 1;       /*decrement TTL*/
    uint32_t decap_prop_ttl : 1; /*propagate TTL*/
    uint32_t mpls_inner_l2 : 1;  /*# MPLS l2 or l3 VPN format*/
    uint32_t dst_bdi : 14;       /*from lcl*/
    uint64_t mac_da : 48;        /*#per flow*/
    uint32_t nxt_sid_opcode : 2; /*#per flow*/
    uint32_t decr_sl : 1;
    uint32_t usid_func_en : 1;
    uint32_t usid_arg_en : 1;
    uint32_t srv6_encaps_red : 1;
    uint32_t srv6_insert_red : 1;
    uint32_t srv6_func_hit : 1;
    uint32_t mac_learn_en : 1;
    uint32_t srv6_encap_end : 1; /*# SRv6 b6 encaps/insert, bm indicator*/
    uint32_t : 1;
    uint32_t ptp_info : 32;      /*#{seqID_8, ptp_off_7, csmoff_14, flg_3}*/
    uint32_t int_mm_mode : 1; /*0: MX; 1: MD. For INT_XD, it is set to MX though it is don’t care.
                               N/A for ioam mode*/
    uint32_t int_profile : 3; /*int profile; reload as  bit0: ioam_color_en, bit1: ioam_delay, bit2:
                               ioam_pkt_color*/
    uint32_t int_role : 2;    /* =0, int/ioam is disabled. =1:src, =2:tr, =3:sink*/
    uint32_t timestamp : 32;  /*time stamp 2-bit sec; 30-bit ns*/
} nb_pkt_pph_l3uc_t;

typedef nb_pkt_pph_l3uc_t nb_pkt_pph_l3mc_t;

typedef struct {
    uint32_t fwd_op : 2;          /*tenant packet forwarding operation
                                    0: L2
                                    1: L2.5 MPLS
                                    2: L3 UC/L3 MC*/
    uint32_t tc : 3;              /*internal traffic class*/
    uint32_t color : 2;           /*internal drop precedence*/
    uint32_t hash_val : 16;       /*for egress mpls/tunnel entropy*/
    uint32_t dst_idx : 16;        /*"UC_PORT: 0~2k-1
                                    UC_LAG: 3.5k~4k-1
                                    TNL: 4k~12k-1
                                    MC_L2BD: 12k~28k-1
                                    MC_L3MEL: 28k~36k-1"*/
    uint32_t src_idx : 14;        /*"0 - 4K: system wide physical port/lag
                                    4K - 20K: system wide single IP/LSP tunnel index to reach remote
                                   peer"*/
    uint32_t skip_epp : 1;        /*skip EPP if destination is to the CPU port. Keep original packet
                                     format*/
    uint32_t igr_acl_label : 16;  /*Reload as IOAM 10b flow id.*/
    uint32_t qos_dnt_modify : 1;  /*indicating that egress side not rewrite pkg qos*/
    uint32_t qos_tnl_uniform : 1; /*while decap, use outer qos to inner qos*/
    uint32_t qos_pcp_dei_val : 4; /*PCP value to mark, or  PCP determined at IDS*/
    uint32_t pkt_journal : 1;     /*instruct downstream pipeline stages to capture states*/
    uint32_t port_num : 6; /*system wide physical port number {die# (in chassis case 4), slice #(4),
                            port #(5)}.*/
    uint32_t die_id : 1;   /*No use, Dont Care*/
    uint32_t slice_id : 3; /*Ingress slice id. There are 8 slices.*/
    uint32_t skip_ipp : 1; /*skip IPP (relevant if ingress from CPU port)*/
    uint32_t mirror_bmap : 8;    /*mirror copy bit map*/
    uint32_t cpu_reason : 10;    /*[9]No use, Dont Care; [8:0] reason code*/
    uint32_t src_bdi : 14;       /*ingress Bdi*/
    uint32_t decap_act : 3;      /*"decap action
                                   0: no decap
                                   1: MPLS pop or transit
                                   2: IP tunnel decap
                                   3: erspan termination
                                   4: SRH remove, next header field of preceding header may need update
                                   5: IPv6 hdr with all its extention hdr
                                   6: frc decap ip/mpls"*/
    uint32_t igr_is_fab : 1;     /*indicate if igr port_type is FAB*/
    uint32_t mpls_lbl : 20;      /*# src_tnl_idx or mpls_esi_label (0 is not valid)*/
    uint32_t is_swap : 1;        /*MPLS Decap stack has PWCW from ingress LSP*/
    uint32_t tnl_idx : 13;       /*0-8K-1: tunnel index*/
    uint32_t tnl_bd : 9;         /*encapsulation adjacency index (MAC DA/VLAN)\*/
    uint32_t decr_ttl : 1;       /*decrement TTL*/
    uint32_t decap_prop_ttl : 1; /*propagate TTL*/
    uint32_t : 14;
    uint64_t : 64;
    uint32_t mac_learn_en : 1;
    uint32_t : 8;
    uint32_t : 32;
    uint32_t timestamp : 32; /*time stamp 2-bit sec; 30-bit ns*/
} nb_pkt_pph_l25_t;

#define NB_PKT_HOST_TO_LE16(__data__)   NB_PKT_SWAP16(__data__)
#define NB_PKT_HOST_TO_LE32(__data__)   NB_PKT_SWAP32(__data__)
#define NB_PKT_HOST_TO_BE16(__data__)   (__data__)
#define NB_PKT_HOST_TO_BE32(__data__)   (__data__)
#define NB_PKT_BE_TO_HOST16(__data__)   (__data__)
#define NB_PKT_BE_TO_HOST32(__data__)   (__data__)

// dst_idx
#define nb_pkt_pph_get_dst_idx(ptr_pph) ((ptr_pph != NULL) ? ptr_pph->dst_idx : 0xffffffff)
#define nb_pkt_pph_set_dst_idx(ptr_pph, dst_idx) \
    {                                            \
        if (ptr_pph != NULL) {                   \
            ptr_pph->dst_idx = dst_idx;          \
        }                                        \
    }

// igr_acl_label
#define nb_pkt_pph_get_igr_acl_lable(ptr_pph) \
    ((ptr_pph != NULL) ? ptr_pph->igr_acl_label : 0xffffffff)
#define nb_pkt_pph_set_igr_acl_lable(ptr_pph, igr_acl_label) \
    {                                                        \
        if (ptr_pph != NULL) {                               \
            ptr_pph->igr_acl_label = igr_acl_label;          \
        }                                                    \
    }

// evpn_esi
#define nb_pkt_pph_get_evpn_esi(ptr_pph) ((ptr_pph != NULL) ? ptr_pph->evpn_esi : 0xffffffff)
#define nb_pkt_pph_set_evpn_esi(ptr_pph, evpn_esi) \
    {                                              \
        if (ptr_pph != NULL) {                     \
            ptr_pph->evpn_esi = evpn_esi;          \
        }                                          \
    }

// tnl_bd
#define nb_pkt_pph_get_tnl_bd(ptr_pph) ((ptr_pph != NULL) ? ptr_pph->tnl_bd : 0xffffffff)
#define nb_pkt_pph_set_tnl_bd(ptr_pph, tnl_bd) \
    {                                          \
        if (ptr_pph != NULL) {                 \
            ptr_pph->tnl_bd = tnl_bd;          \
        }                                      \
    }

// ptp_info
#define nb_pkt_pph_get_ptp_info(ptr_pph) ((ptr_pph != NULL) ? ptr_pph->ptp_info : 0xffffffff)
#define nb_pkt_pph_set_ptp_info(ptr_pph, ptp_info) \
    {                                              \
        if (ptr_pph != NULL) {                     \
            ptr_pph->ptp_info = ptp_info;          \
        }                                          \
    }

// mac_da
#define nb_pkt_pph_get_mac_da(ptr_pph) ((ptr_pph != NULL) ? ptr_pph->mac_da : 0xffffffffffffffff)
#define nb_pkt_pph_set_mac_da(ptr_pph, mac_da) \
    {                                          \
        if (ptr_pph != NULL) {                 \
            ptr_pph->mac_da = mac_da;          \
        }                                      \
    }

// mpls_lbl
#define nb_pkt_pph_get_mpls_lbl(ptr_pph) ((ptr_pph != NULL) ? ptr_pph->mpls_lbl : 0xffffffff)
#define nb_pkt_pph_set_mpls_lbl(ptr_pph, mpls_lbl) \
    {                                              \
        if (ptr_pph != NULL) {                     \
            ptr_pph->mpls_lbl = mpls_lbl;          \
        }                                          \
    }
#endif
#pragma pack(pop)

int
nb_init_dma_driver(uint32_t unit);

void
nb_cleanup_dma_driver(uint32_t unit);

int
nb_init_pkt_driver(uint32_t unit);

void
nb_deinit_pkt_driver(uint32_t unit);

#endif // __NB_CHIP_H__