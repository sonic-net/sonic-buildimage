/*
 * $Id: dcb.h,v 1.32 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
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
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * File:        soc/dcb.h
 * Purpose:     Define dma control block (DCB) operations
 *              A uniform set of operations is defined here and loaded
 *              into SOC_CONTROL during initialization.
 */

#ifndef _SOC_DCB_H
#define _SOC_DCB_H

#include <soc/types.h>
#include <soc/rx.h>

typedef void    dcb_t;
struct dv_s;            /* in soc/dma.h, but we have recursive problems */

#ifdef INCLUDE_KNET
/*
 * On XGS devices bit 15 fo the Transferred Bytes field in
 * the DCBs is used to indicate that kernel processing is
 * complete. Using this bit reduces the theoretically maximum
 * supported packet size from 64K to 32K, but this is still
 * adequate for 16K jumbo packets.
 */
#define SOC_DCB_KNET_DONE       0x8000

#endif

/*
 * DCB Operations
 */

typedef struct dcb_op_s {
    int         dcbtype;
    int         dcbsize;
    soc_rx_reason_t **rx_reason_maps;
    soc_rx_reason_t * (*rx_reason_map_get)(struct dcb_op_s *dcb_op, dcb_t *dcb);
    void        (*rx_reasons_get)(struct dcb_op_s *dcb_op, dcb_t *dcb,
                                  soc_rx_reasons_t *reasons);
    void        (*init)(dcb_t *dcb);
    /* composite helper functions */
    int         (*addtx)(struct dv_s *dv, sal_vaddr_t addr, uint32 count,
                         pbmp_t l2pbm, pbmp_t utpbm, pbmp_t l3pbm,
                         uint32 flags, uint32 *hgh);
    int         (*addrx)(struct dv_s *dv, sal_vaddr_t addr, uint32 count,
                         uint32 flags);
    uint32      (*intrinfo)(int unit, dcb_t *dcb, int tx, uint32 *count);
    /* basic dcb values */
    void        (*reqcount_set)(dcb_t *dcb, uint32 count);
    uint32      (*reqcount_get)(dcb_t *dcb);
    uint32      (*xfercount_get)(dcb_t *dcb);
    void        (*addr_set)(int unit, dcb_t *dcb, sal_vaddr_t addr);
    sal_vaddr_t (*addr_get)(int unit, dcb_t *dcb);
    sal_paddr_t (*paddr_get)(dcb_t *dcb);
    void        (*done_set)(dcb_t *dcb, int val);
    uint32      (*done_get)(dcb_t *dcb);
    void        (*sg_set)(dcb_t *dcb, int val);
    uint32      (*sg_get)(dcb_t *dcb);
    void        (*chain_set)(dcb_t *dcb, int val);
    uint32      (*chain_get)(dcb_t *dcb);
    void        (*reload_set)(dcb_t *dcb, int val);
    uint32      (*reload_get)(dcb_t *dcb);
    void        (*desc_intr_set)(dcb_t *dcb, int val);
    uint32      (*desc_intr_get)(dcb_t *dcb);
    /* transmit dcb controls */
    void        (*tx_l2pbm_set)(dcb_t *dcb, pbmp_t pbm);
    void        (*tx_utpbm_set)(dcb_t *dcb, pbmp_t pbm);
    void        (*tx_l3pbm_set)(dcb_t *dcb, pbmp_t pbm);
    void        (*tx_crc_set)(dcb_t *dcb, int crc);
    void        (*tx_cos_set)(dcb_t *dcb, int cos);
    void        (*tx_destmod_set)(dcb_t *dcb, uint32 modid);
    void        (*tx_destport_set)(dcb_t *dcb, uint32 port);
    void        (*tx_opcode_set)(dcb_t *dcb, uint32 opcode);
    void        (*tx_srcmod_set)(dcb_t *dcb, uint32 modid);
    void        (*tx_srcport_set)(dcb_t *dcb, uint32 port);
    void        (*tx_prio_set)(dcb_t *dcb, uint32 prio);
    void        (*tx_pfm_set)(dcb_t *dcb, uint32 pfm);

    /* receive dcb controls */
    uint32      (*rx_untagged_get)(dcb_t *dcb, int dt_mode, int ingport_is_hg);
    uint32      (*rx_crc_get)(dcb_t *dcb);
    uint32      (*rx_cos_get)(dcb_t *dcb);
    uint32      (*rx_destmod_get)(dcb_t *dcb);
    uint32      (*rx_destport_get)(dcb_t *dcb);
    uint32      (*rx_opcode_get)(dcb_t *dcb);
    uint32      (*rx_classtag_get)(dcb_t *dcb);
    uint32      (*rx_matchrule_get)(dcb_t *dcb);
    uint32      (*rx_start_get)(dcb_t *dcb);
    uint32      (*rx_end_get)(dcb_t *dcb);
    uint32      (*rx_error_get)(dcb_t *dcb);
    uint32      (*rx_prio_get)(dcb_t *dcb);
    uint32      (*rx_reason_get)(dcb_t *dcb);
    uint32      (*rx_reason_hi_get)(dcb_t *dcb);
    uint32      (*rx_ingport_get)(dcb_t *dcb);
    uint32      (*rx_srcport_get)(dcb_t *dcb);
    uint32      (*rx_srcmod_get)(dcb_t *dcb);
    uint32      (*rx_mcast_get)(dcb_t *dcb);
    uint32      (*rx_vclabel_get)(dcb_t *dcb);
    uint32      (*rx_mirror_get)(dcb_t *dcb);
    uint32      (*rx_timestamp_get)(dcb_t *dcb);
    uint32      (*rx_timestamp_upper_get)(dcb_t *dcb);
    void        (*hg_set)(dcb_t *dcb, uint32 hg);
    uint32      (*hg_get)(dcb_t *dcb);
    void        (*stat_set)(dcb_t *dcb, uint32 stat);
    uint32      (*stat_get)(dcb_t *dcb);
    void        (*purge_set)(dcb_t *dcb, uint32 purge);
    uint32      (*purge_get)(dcb_t *dcb);
    uint32 *    (*mhp_get)(dcb_t *dcb);
    uint32      (*rx_outer_vid_get)(dcb_t *dcb);
    uint32      (*rx_outer_pri_get)(dcb_t *dcb);
    uint32      (*rx_outer_cfi_get)(dcb_t *dcb);
    uint32      (*rx_outer_tag_action_get)(dcb_t *dcb);
    uint32      (*rx_inner_vid_get)(dcb_t *dcb);
    uint32      (*rx_inner_pri_get)(dcb_t *dcb);
    uint32      (*rx_inner_cfi_get)(dcb_t *dcb);
    uint32      (*rx_inner_tag_action_get)(dcb_t *dcb);
    uint32      (*rx_bpdu_get)(dcb_t *dcb);
    uint32      (*rx_l3_intf_get)(dcb_t *dcb);
    uint32      (*rx_decap_tunnel_get)(dcb_t *dcb);
    uint32      (*rx_switch_drop_get)(dcb_t *dcb);
    uint32      (*olp_encap_oam_pkt_get)(dcb_t *dcb);
    uint32      (*read_ecc_error_get)(dcb_t *dcb);
    uint32      (*desc_remaining_get)(dcb_t *dcb);
    uint32      (*desc_remaining_set)(dcb_t *dcb, uint32 val);
    uint32      (*desc_status_get)(dcb_t *dcb);
    uint32      (*desc_status_set)(dcb_t *dcb, uint32 val);
    uint32      (*rx_hg2_ext_eh_type_get)(dcb_t *dcb);
} dcb_op_t;

#define SOC_DCB(_u)             SOC_CONTROL(_u)->dcb_op

#define SOC_DCB_TYPE(_u)        SOC_DCB(_u)->dcbtype
#define SOC_DCB_SIZE(_u)        SOC_DCB(_u)->dcbsize

#define SOC_DCB_RX_REASON_MAPS( _u)  SOC_DCB(_u)->rx_reason_maps

#define SOC_DCB_INIT(_u, _dcb)  SOC_DCB(_u)->init(_dcb)
#define SOC_DCB_ADDTX(_u, _dv, _addr, _count, _l2pbm, _utpbm, _l3pbm, _f, _hgh) \
        SOC_DCB(_u)->addtx(_dv, _addr, _count, _l2pbm, _utpbm, _l3pbm, _f, _hgh)
#define SOC_DCB_ADDRX(_u, _dv, _addr, _count, _f)       \
        SOC_DCB(_u)->addrx(_dv, _addr, _count, _f)
#define SOC_DCB_INTRINFO(_u, _dcb, _tx, _countp)        \
        SOC_DCB(_u)->intrinfo(_u, _dcb, _tx, _countp)

/* flags returned from INTRINFO */
#define SOC_DCB_INFO_DONE       0x02
#define SOC_DCB_INFO_PKTEND     0x01

#define SOC_DCB_REQCOUNT_SET(_u, _dcb, _count)  \
        SOC_DCB(_u)->reqcount_set(_dcb, _count)
#define SOC_DCB_REQCOUNT_GET(_u, _dcb)          \
        SOC_DCB(_u)->reqcount_get(_dcb)
#ifdef INCLUDE_KNET
/* Mask off indicator for kernel processing done */
#define SOC_DCB_XFERCOUNT_GET(_u, _dcb)         \
        (SOC_DCB(_u)->xfercount_get(_dcb) & ~SOC_DCB_KNET_DONE)
#else
#define SOC_DCB_XFERCOUNT_GET(_u, _dcb)         \
        SOC_DCB(_u)->xfercount_get(_dcb)
#endif
#define SOC_DCB_ADDR_SET(_u, _dcb, _addr)       \
        SOC_DCB(_u)->addr_set(_u, _dcb, _addr)
#define SOC_DCB_ADDR_GET(_u, _dcb)              \
        SOC_DCB(_u)->addr_get(_u, _dcb)
#define SOC_DCB_PADDR_GET(_u, _dcb)             \
        SOC_DCB(_u)->paddr_get(_dcb)
#define SOC_DCB_DONE_SET(_u, _dcb, _val)        \
        SOC_DCB(_u)->done_set(_dcb, _val)
#define SOC_DCB_DONE_GET(_u, _dcb)              \
        SOC_DCB(_u)->done_get(_dcb)
#define SOC_DCB_SG_SET(_u, _dcb, _val)          \
        SOC_DCB(_u)->sg_set(_dcb, _val)
#define SOC_DCB_SG_GET(_u, _dcb)                \
        SOC_DCB(_u)->sg_get(_dcb)
#define SOC_DCB_CHAIN_SET(_u, _dcb, _val)       \
        SOC_DCB(_u)->chain_set(_dcb, _val)
#define SOC_DCB_CHAIN_GET(_u, _dcb)             \
        SOC_DCB(_u)->chain_get(_dcb)
#define SOC_DCB_RELOAD_SET(_u, _dcb, _val)      \
        SOC_DCB(_u)->reload_set(_dcb, _val)
#define SOC_DCB_RELOAD_GET(_u, _dcb)            \
        SOC_DCB(_u)->reload_get(_dcb)

#define SOC_DCB_ALL_DESC_INTR      0x1
#define SOC_DCB_CNTLD_DESC_INTR    0x2

#define SOC_DCB_DESC_INTR_SET(_u, _dcb, _val)   \
        SOC_DCB(_u)->desc_intr_set(_dcb, _val)
#define SOC_DCB_DESC_INTR_GET(_u, _dcb)         \
        SOC_DCB(_u)->desc_intr_get(_dcb)

#define SOC_DCB_TX_L2PBM_SET(_u, _dcb, _pbm)    \
        SOC_DCB(_u)->tx_l2pbm_set(_dcb, _pbm)
#define SOC_DCB_TX_UTPBM_SET(_u, _dcb, _pbm)    \
        SOC_DCB(_u)->tx_utpbm_set(_dcb, _pbm)
#define SOC_DCB_TX_L3PBM_SET(_u, _dcb, _pbm)    \
        SOC_DCB(_u)->tx_l3pbm_set(_dcb, _pbm)
#define SOC_DCB_TX_CRC_SET(_u, _dcb, _val)      \
        SOC_DCB(_u)->tx_crc_set(_dcb, _val)
#define SOC_DCB_TX_COS_SET(_u, _dcb, _val)      \
        SOC_DCB(_u)->tx_cos_set(_dcb, _val)
#define SOC_DCB_TX_DESTMOD_SET(_u, _dcb, _val)  \
        SOC_DCB(_u)->tx_destmod_set(_dcb, _val)
#define SOC_DCB_TX_DESTPORT_SET(_u, _dcb, _val) \
        SOC_DCB(_u)->tx_destport_set(_dcb, _val)
#define SOC_DCB_TX_OPCODE_SET(_u, _dcb, _val)   \
        SOC_DCB(_u)->tx_opcode_set(_dcb, _val)
#define SOC_DCB_TX_SRCMOD_SET(_u, _dcb, _val)   \
        SOC_DCB(_u)->tx_srcmod_set(_dcb, _val)
#define SOC_DCB_TX_SRCPORT_SET(_u, _dcb, _val)  \
        SOC_DCB(_u)->tx_srcport_set(_dcb, _val)
#define SOC_DCB_TX_PRIO_SET(_u, _dcb, _val)     \
        SOC_DCB(_u)->tx_prio_set(_dcb, _val)
#define SOC_DCB_TX_PFM_SET(_u, _dcb, _val)      \
        SOC_DCB(_u)->tx_pfm_set(_dcb, _val)

#define SOC_DCB_RX_UNTAGGED_GET(_u, _dcb)       \
        SOC_DCB(_u)->rx_untagged_get(_dcb,      \
        SOC_DT_MODE(_u), IS_HG_PORT(_u, SOC_DCB_RX_INGPORT_GET(_u, _dcb)))
#define SOC_DCB_RX_CRC_GET(_u, _dcb)            \
        SOC_DCB(_u)->rx_crc_get(_dcb)
#define SOC_DCB_RX_COS_GET(_u, _dcb)            \
        SOC_DCB(_u)->rx_cos_get(_dcb)
#define SOC_DCB_RX_DESTMOD_GET(_u, _dcb)        \
        SOC_DCB(_u)->rx_destmod_get(_dcb)
#define SOC_DCB_RX_DESTPORT_GET(_u, _dcb)       \
        SOC_DCB(_u)->rx_destport_get(_dcb)
#define SOC_DCB_RX_OPCODE_GET(_u, _dcb)         \
        SOC_DCB(_u)->rx_opcode_get(_dcb)
#define SOC_DCB_RX_CLASSTAG_GET(_u, _dcb)       \
        SOC_DCB(_u)->rx_classtag_get(_dcb)
#define SOC_DCB_RX_MATCHRULE_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_matchrule_get(_dcb)
#define SOC_DCB_OLP_ENCAP_OAM_PKT_GET(_u, _dcb) \
        SOC_DCB(_u)->olp_encap_oam_pkt_get(_dcb)
#define SOC_DCB_READ_ECC_ERROR_GET(_u, _dcb)    \
        SOC_DCB(_u)->read_ecc_error_get(_dcb)
#define SOC_DCB_DESC_REMAINING_GET(_u, _dcb)    \
        SOC_DCB(_u)->desc_remaining_get(_dcb)
#define SOC_DCB_DESC_REMAINING_SET(_u, _dcb, _val)    \
        SOC_DCB(_u)->desc_remaining_set(_dcb, _val)
#define SOC_DCB_DESC_STATUS_GET(_u, _dcb)       \
        SOC_DCB(_u)->desc_status_get(_dcb)
#define SOC_DCB_DESC_STATUS_SET(_u, _dcb)       \
        SOC_DCB(_u)->desc_status_set(_dcb)
#define SOC_DCB_RX_START_GET(_u, _dcb)          \
        SOC_DCB(_u)->rx_start_get(_dcb)
#define SOC_DCB_RX_END_GET(_u, _dcb)            \
        SOC_DCB(_u)->rx_end_get(_dcb)
#define SOC_DCB_RX_ERROR_GET(_u, _dcb)          \
        SOC_DCB(_u)->rx_error_get(_dcb)
#define SOC_DCB_RX_PRIO_GET(_u, _dcb)           \
        SOC_DCB(_u)->rx_prio_get(_dcb)
#define SOC_DCB_RX_REASON_GET(_u, _dcb)         \
        SOC_DCB(_u)->rx_reason_get(_dcb)
#define SOC_DCB_RX_REASON_HI_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_reason_hi_get(_dcb)
#define SOC_DCB_RX_REASONS_GET(_u, _dcb, _reasons)  \
        SOC_DCB(_u)->rx_reasons_get(SOC_DCB(_u), _dcb, _reasons)
#define SOC_DCB_RX_INGPORT_GET(_u, _dcb)        \
        SOC_DCB(_u)->rx_ingport_get(_dcb)
#define SOC_DCB_RX_SRCPORT_GET(_u, _dcb)        \
        SOC_DCB(_u)->rx_srcport_get(_dcb)
#define SOC_DCB_RX_SRCMOD_GET(_u, _dcb)         \
        SOC_DCB(_u)->rx_srcmod_get(_dcb)
#define SOC_DCB_RX_MCAST_GET(_u, _dcb)          \
        SOC_DCB(_u)->rx_mcast_get(_dcb)
#define SOC_DCB_RX_VCLABEL_GET(_u, _dcb)        \
        SOC_DCB(_u)->rx_vclabel_get(_dcb)
#define SOC_DCB_RX_MIRROR_GET(_u, _dcb)         \
        SOC_DCB(_u)->rx_mirror_get(_dcb)
#define SOC_DCB_RX_TIMESTAMP_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_timestamp_get(_dcb)
#define SOC_DCB_RX_TIMESTAMP_UPPER_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_timestamp_upper_get(_dcb)

#define SOC_DCB_HG_SET(_u, _dcb, _val)          \
        SOC_DCB(_u)->hg_set(_dcb, _val)
#define SOC_DCB_HG_GET(_u, _dcb)                \
        SOC_DCB(_u)->hg_get(_dcb)
#define SOC_DCB_STAT_SET(_u, _dcb, _val)        \
        SOC_DCB(_u)->stat_set(_dcb, _val)
#define SOC_DCB_STAT_GET(_u, _dcb)              \
        SOC_DCB(_u)->stat_get(_dcb)
#define SOC_DCB_PURGE_SET(_u, _dcb, _val)       \
        SOC_DCB(_u)->purge_set(_dcb, _val)
#define SOC_DCB_PURGE_GET(_u, _dcb)             \
        SOC_DCB(_u)->purge_get(_dcb)

#define SOC_DCB_MHP_GET(_u, _dcb)               \
        SOC_DCB(_u)->mhp_get(_dcb)
#define SOC_DCB_RX_OUTER_VID_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_outer_vid_get(_dcb)
#define SOC_DCB_RX_OUTER_PRI_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_outer_pri_get(_dcb)
#define SOC_DCB_RX_OUTER_CFI_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_outer_cfi_get(_dcb)
#define SOC_DCB_RX_OUTER_TAG_ACTION_GET(_u, _dcb)     \
        SOC_DCB(_u)->rx_outer_tag_action_get(_dcb)
#define SOC_DCB_RX_INNER_VID_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_inner_vid_get(_dcb)
#define SOC_DCB_RX_INNER_PRI_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_inner_pri_get(_dcb)
#define SOC_DCB_RX_INNER_CFI_GET(_u, _dcb)      \
        SOC_DCB(_u)->rx_inner_cfi_get(_dcb)
#define SOC_DCB_RX_INNER_TAG_ACTION_GET(_u, _dcb)     \
        SOC_DCB(_u)->rx_inner_tag_action_get(_dcb)
#define SOC_DCB_RX_BPDU_GET(_u, _dcb)           \
        SOC_DCB(_u)->rx_bpdu_get(_dcb)
#define SOC_DCB_RX_L3_INTF_GET(_u, _dcb)           \
        SOC_DCB(_u)->rx_l3_intf_get(_dcb)
#define SOC_DCB_RX_DECAP_TUNNEL_GET(_u, _dcb)   \
        SOC_DCB(_u)->rx_decap_tunnel_get(_dcb)
#define SOC_DCB_RX_SWITCH_DROP_GET(_u, _dcb)   \
        SOC_DCB(_u)->rx_switch_drop_get(_dcb)

#define SOC_DCB_RX_HG2_EXT_EH_TYPE_GET(_u, _dcb)       \
        SOC_DCB(_u)->rx_hg2_ext_eh_type_get(_dcb)

#define SOC_DCB_IDX2PTR(_u, _dcb, _i) \
        (dcb_t *)((char *)_dcb + (SOC_DCB_SIZE(_u) * (_i)))
#define SOC_DCB_PTR2IDX(_u, _dcb1, _dcb2) \
        (int)(((char *)_dcb1 - (char *)_dcb2) / SOC_DCB_SIZE(_u))

extern void     soc_dcb_unit_init(int unit);

#endif  /* _SOC_DCB_H */
