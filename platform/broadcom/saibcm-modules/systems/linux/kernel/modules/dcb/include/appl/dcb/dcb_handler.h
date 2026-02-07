/* 
 * $Id:$
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
 * File:        dcb.h
 * Purpose:     DCB Library interface
 */

#ifndef   _DCB_DCB_H_
#define   _DCB_DCB_H_

#include <sal/types.h>
#include <soc/dcb.h>


/*
 * DCB Return Codes
 */
#define DCB_OK        0
#define DCB_ERROR     1


/*
 * DCB Handle
 */
typedef struct dcb_handle_s {
    dcb_op_t    *dcb_op;
} dcb_handle_t;

#define DCB_HANDLE(_h)         (_h)
#define DCB_OP(_h)             DCB_HANDLE(_h)->dcb_op


/*
 * Function to initialize the DCB library for a
 * given device type and revision ID.
 *
 * It must be called before accessing any of the DCB fields.
 */
extern int
dcb_handle_init(dcb_handle_t *handle, uint16 dev_id, uint8 rev_id);


/*
 * DCB Fields
 */

#define DCB_TYPE(_h)                                                    \
    DCB_OP(_h)->dcbtype

#define DCB_SIZE(_h)                                                    \
    DCB_OP(_h)->dcbsize

#define DCB_INIT(_h, _dcb)                                              \
    DCB_OP(_h)->init(_dcb)

#define DCB_REQCOUNT_SET(_h, _dcb, _count)                              \
    DCB_OP(_h)->reqcount_set(_dcb, _count)
#define DCB_REQCOUNT_GET(_h, _dcb)                                      \
    DCB_OP(_h)->reqcount_get(_dcb)

#ifdef INCLUDE_KNET
/* Mask off indicator for kernel processing done */
#define DCB_XFERCOUNT_GET(_h, _dcb)                                     \
    (DCB_OP(_h)->xfercount_get(_dcb) & ~SOC_DCB_KNET_DONE)
#else
#define DCB_XFERCOUNT_GET(_h, _dcb)                                     \
    DCB_OP(_h)->xfercount_get(_dcb)
#endif

#define DCB_DONE_SET(_h, _dcb, _val)                                    \
    DCB_OP(_h)->done_set(_dcb, _val)
#define DCB_DONE_GET(_h, _dcb)                                          \
    DCB_OP(_h)->done_get(_dcb)
#define DCB_SG_SET(_h, _dcb, _val)                                      \
    DCB_OP(_h)->sg_set(_dcb, _val)
#define DCB_SG_GET(_h, _dcb)                                            \
    DCB_OP(_h)->sg_get(_dcb)
#define DCB_CHAIN_SET(_h, _dcb, _val)                                   \
    DCB_OP(_h)->chain_set(_dcb, _val)
#define DCB_CHAIN_GET(_h, _dcb)                                         \
    DCB_OP(_h)->chain_get(_dcb)
#define DCB_RELOAD_SET(_h, _dcb, _val)                                  \
    DCB_OP(_h)->reload_set(_dcb, _val)
#define DCB_RELOAD_GET(_h, _dcb)                                        \
    DCB_OP(_h)->reload_get(_dcb)

#define DCB_TX_CRC_SET(_h, _dcb, _val)                                  \
    DCB_OP(_h)->tx_crc_set(_dcb, _val)
#define DCB_TX_COS_SET(_h, _dcb, _val)                                  \
    DCB_OP(_h)->tx_cos_set(_dcb, _val)
#define DCB_TX_DESTMOD_SET(_h, _dcb, _val)                              \
    DCB_OP(_h)->tx_destmod_set(_dcb, _val)
#define DCB_TX_DESTPORT_SET(_h, _dcb, _val)                             \
    DCB_OP(_h)->tx_destport_set(_dcb, _val)
#define DCB_TX_OPCODE_SET(_h, _dcb, _val)                               \
    DCB_OP(_h)->tx_opcode_set(_dcb, _val)
#define DCB_TX_SRCMOD_SET(_h, _dcb, _val)                               \
    DCB_OP(_h)->tx_srcmod_set(_dcb, _val)
#define DCB_TX_SRCPORT_SET(_h, _dcb, _val)                              \
    DCB_OP(_h)->tx_srcport_set(_dcb, _val)
#define DCB_TX_PRIO_SET(_h, _dcb, _val)                                 \
    DCB_OP(_h)->tx_prio_set(_dcb, _val)
#define DCB_TX_PFM_SET(_h, _dcb, _val)                                  \
    DCB_OP(_h)->tx_pfm_set(_dcb, _val)

#define DCB_RX_UNTAGGED_GET(_h, _dcb, _dt_mode, _ingport_is_hg)         \
    DCB_OP(_h)->rx_untagged_get(_dcb, _dt_mode, _ingport_is_hg)
#define DCB_RX_CRC_GET(_h, _dcb)                                        \
    DCB_OP(_h)->rx_crc_get(_dcb)
#define DCB_RX_COS_GET(_h, _dcb)                                        \
    DCB_OP(_h)->rx_cos_get(_dcb)
#define DCB_RX_DESTMOD_GET(_h, _dcb)                                    \
    DCB_OP(_h)->rx_destmod_get(_dcb)
#define DCB_RX_DESTPORT_GET(_h, _dcb)                                   \
    DCB_OP(_h)->rx_destport_get(_dcb)
#define DCB_RX_OPCODE_GET(_h, _dcb)                                     \
    DCB_OP(_h)->rx_opcode_get(_dcb)

#define DCB_RX_CLASSTAG_GET(_h, _dcb)                                   \
    DCB_OP(_h)->rx_classtag_get(_dcb)
#define DCB_RX_MATCHRULE_GET(_h, _dcb)                                  \
    DCB_OP(_h)->rx_matchrule_get(_dcb)
#define DCB_RX_START_GET(_h, _dcb)                                      \
    DCB_OP(_h)->rx_start_get(_dcb)
#define DCB_RX_END_GET(_h, _dcb)                                        \
    DCB_OP(_h)->rx_end_get(_dcb)
#define DCB_RX_ERROR_GET(_h, _dcb)                                      \
    DCB_OP(_h)->rx_error_get(_dcb)
#define DCB_RX_PRIO_GET(_h, _dcb)                                       \
    DCB_OP(_h)->rx_prio_get(_dcb)
#define DCB_RX_REASON_GET(_h, _dcb)                                     \
    DCB_OP(_h)->rx_reason_get(_dcb)
#define DCB_RX_REASON_HI_GET(_h, _dcb)                                  \
    DCB_OP(_h)->rx_reason_hi_get(_dcb)
#define DCB_RX_REASONS_GET(_h, _dcb, _reasons)                          \
    DCB_OP(_h)->rx_reasons_get(DCB_OP(_h), _dcb, _reasons)
#define DCB_RX_INGPORT_GET(_h, _dcb)                                    \
    DCB_OP(_h)->rx_ingport_get(_dcb)
#define DCB_RX_SRCPORT_GET(_h, _dcb)                                    \
    DCB_OP(_h)->rx_srcport_get(_dcb)
#define DCB_RX_SRCMOD_GET(_h, _dcb)                                     \
    DCB_OP(_h)->rx_srcmod_get(_dcb)
#define DCB_RX_MCAST_GET(_h, _dcb)                                      \
    DCB_OP(_h)->rx_mcast_get(_dcb)
#define DCB_RX_VCLABEL_GET(_h, _dcb)                                    \
    DCB_OP(_h)->rx_vclabel_get(_dcb)
#define DCB_RX_MIRROR_GET(_h, _dcb)                                     \
    DCB_OP(_h)->rx_mirror_get(_dcb)
#define DCB_RX_MATCHRULE_GET(_h, _dcb)                                  \
    DCB_OP(_h)->rx_matchrule_get(_dcb)
#define DCB_RX_TIMESTAMP_GET(_h, _dcb)                                  \
    DCB_OP(_h)->rx_timestamp_get(_dcb)
#define DCB_RX_TIMESTAMP_UPPER_GET(_h, _dcb)                            \
    DCB_OP(_h)->rx_timestamp_upper_get(_dcb)
    
#define DCB_HG_SET(_h, _dcb, _val)                                      \
    DCB_OP(_h)->hg_set(_dcb, _val)
#define DCB_HG_GET(_h, _dcb)                                            \
    DCB_OP(_h)->hg_get(_dcb)
#define DCB_STAT_SET(_h, _dcb, _val)                                    \
    DCB_OP(_h)->stat_set(_dcb, _val)
#define DCB_STAT_GET(_h, _dcb)                                          \
    DCB_OP(_h)->stat_get(_dcb)
#define DCB_PURGE_SET(_h, _dcb, _val)                                   \
    DCB_OP(_h)->purge_set(_dcb, _val)
#define DCB_PURGE_GET(_h, _dcb)                                         \
    DCB_OP(_h)->purge_get(_dcb)

#define DCB_MHP_GET(_h, _dcb)               \
    DCB_OP(_h)->mhp_get(_dcb)
#define DCB_RX_OUTER_VID_GET(_h, _dcb)      \
    DCB_OP(_h)->rx_outer_vid_get(_dcb)
#define DCB_RX_OUTER_PRI_GET(_h, _dcb)      \
    DCB_OP(_h)->rx_outer_pri_get(_dcb)
#define DCB_RX_OUTER_CFI_GET(_h, _dcb)      \
    DCB_OP(_h)->rx_outer_cfi_get(_dcb)
#define DCB_RX_OUTER_TAG_ACTION_GET(_h, _dcb)     \
    DCB_OP(_h)->rx_outer_tag_action_get(_dcb)
#define DCB_RX_INNER_VID_GET(_h, _dcb)      \
    DCB_OP(_h)->rx_inner_vid_get(_dcb)
#define DCB_RX_INNER_PRI_GET(_h, _dcb)      \
    DCB_OP(_h)->rx_inner_pri_get(_dcb)
#define DCB_RX_INNER_CFI_GET(_h, _dcb)      \
    DCB_OP(_h)->rx_inner_cfi_get(_dcb)
#define DCB_RX_INNER_TAG_ACTION_GET(_h, _dcb)     \
    DCB_OP(_h)->rx_inner_tag_action_get(_dcb)
#define DCB_RX_BPDU_GET(_h, _dcb)           \
    DCB_OP(_h)->rx_bpdu_get(_dcb)
#define DCB_RX_L3_INTF_GET(_h, _dcb)        \
    DCB_OP(_h)->rx_l3_intf_get(_dcb)
#define DCB_RX_DECAP_TUNNEL_GET(_h, _dcb)   \
    DCB_OP(_h)->rx_decap_tunnel_get(_dcb)
#define DCB_RX_SWITCH_DROP_GET(_h, _dcb)    \
    DCB_OP(_h)->rx_switch_drop_get(_dcb)
#define DCB_OLP_ENCAP_OAM_PKT_GET(_h, _dcb) \
    DCB_OP(_h)->olp_encap_oam_pkt_get(_dcb)
#define DCB_READ_ECC_ERROR_GET(_h, _dcb)    \
    DCB_OP(_h)->read_ecc_error_get(_dcb)


#endif /* _DCB_DCB_H_ */
