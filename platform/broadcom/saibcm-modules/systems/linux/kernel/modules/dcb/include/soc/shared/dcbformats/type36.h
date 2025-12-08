/*
 * $Id: $
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
 * File:        soc/shared/dcbformats/type36.h
 * Purpose:     Define dma control block (DCB) format for a type36 DCB
 *              used by the 56870 (Trident 3)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE36_H
#define _SOC_SHARED_DCBFORMATS_TYPE36_H

/*
 * DMA Control Block - Type 36
 * Used on 56870 devices
 * 4 words
 */
typedef struct {
    uint32  addr_lo;                    /* T36.0: physical address, lower 32 bits */
    uint32  addr_hi;                    /* T36.0: physical address, upper 32 bits */

#ifdef  LE_HOST
    uint32  c_count:16,                 /* Requested byte count */
            c_chain:1,                  /* Chaining */
            c_sg:1,                     /* Scatter Gather */
            c_reload:1,                 /* Reload */
            c_hg:1,                     /* Higig (TX) */
            :2,                         /* reserved */
            c_purge:1,                  /* Purge packet (TX) */
            c_intr:2,                   /* Desc Intr Mode */
            c_desc:4,                   /* descriptors remaining */
            c_desc_status:1,            /* disable descriptor status write */
            :2;                         /* reserved */
#else
    uint32  :2,                         /* resered */
            c_desc_status:1,            /* disable descriptor status write */
            c_desc:4,                   /* descriptors remaining */
            c_intr:2,                   /* Desc Intr Mode */
            c_purge:1,                  /* Purge packet (TX) */
            :2,                         /* reserved */
            c_hg:1,                     /* Higig (TX) */
            c_reload:1,                 /* Reload */
            c_sg:1,                     /* Scatter Gather */
            c_chain:1,                  /* Chaining */
            c_count:16;                 /* Requested byte count */
#endif  /* LE_HOST */

#ifdef  LE_HOST
    uint32  count:16,                   /* Transferred byte count */
            end:1,                      /* End bit (RX) */
            start:1,                    /* Start bit (RX) */
            error:1,                    /* Cell Error (RX) */
            ecc_error:1,                /* packet ECC Error (RX) */
            :11,                        /* Reserved */
            done:1;                     /* Descriptor Done */
#else
    uint32  done:1,                     /* Descriptor Done */
            :11,                        /* Reserved */
            ecc_error:1,                /* Packet ECC Error (RX) */
            error:1,                    /* Cell Error (RX) */
            start:1,                    /* Start bit (RX) */
            end:1,                      /* End bit (RX) */
            count:16;                   /* Transferred byte count */
#endif
} dcb36_t;

typedef struct {
    uint32  mh0;                        /* Module Header word 0 */
                                        /* (word 14 in EP_TO_CPU_HDR) */
    uint32  mh1;                        /* Module Header word 1 */
                                        /* (word 13 in EP_TO_CPU_HDR) */
    uint32  mh2;                        /* Module Header word 2 */
                                        /* (word 12 in EP_TO_CPU_HDR) */
    uint32  mh3;                        /* Module Header word 3 */
                                        /* (word 11 in EP_TO_CPU_HDR) */

                                        /* (word 10 in EP_TO_CPU_HDR) */
                                        /* word 9 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  reason_hi;                  /* CPU opcode (hi bits) */

                                        /* (word 9 in EP_TO_CPU_HDR) */
                                        /* word 8 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  reason;                     /* CPU opcode (low bits) */

#ifdef  LE_HOST
                                        /* (word 8 in EP_TO_CPU_HDR) */
                                        /* word 7 in EP_TO_CPU_COMPOSITES_BUS */
    union {
        struct {
            uint32  srcport:8,          /* Source port number */
                    pkt_len:14,         /* Packet length */
                    cpu_cos:6,          /* CPU COS*/
                    ecn:2,              /* New ECN value */
                    chg_ecn:1,          /* ECN changed */
                    bfd_session_index_valid:1;  /* Indicate a BFD terminated packet */
        } overlay1;
        struct {
            uint32  srcport:8,          /* Source port number */
                    pkt_len:14,         /* Packet length */
                    queue_num:6,        /* MMU COS*/
                    ecn:2,              /* New ECN value */
                    chg_ecn:1,          /* ECN changed */
                    bfd_session_index_valid:1;  /* Indicate a BFD terminated packet */
        } overlay2;
    } word8;

#else
                                        /* (word 8 in EP_TO_CPU_HDR) */
                                        /* word 7 in EP_TO_CPU_COMPOSITES_BUS */
    union {
        struct {
            uint32  bfd_session_index_valid:1,  /* Indicate a BFD terminated packet */
                    chg_ecn:1,          /* ECN changed */
                    ecn:2,              /* New ECN value */
                    cpu_cos:6,          /* CPU COS*/
                    pkt_len:14,         /* Packet length */
                    srcport:8;          /* Source port number */
        } overlay1;
        struct {
            uint32  bfd_session_index_valid:1,  /* Indicate a BFD terminated packet */
                    chg_ecn:1,          /* ECN changed */
                    ecn:2,              /* New ECN value */
                    queue_num:6,        /* MMU COS*/
                    pkt_len:14,         /* Packet length */
                    srcport:8;          /* Source port number */
        } overlay2;
    } word8;
#endif


#ifdef  LE_HOST
                                        /* (word 7 in EP_TO_CPU_HDR) */
                                        /* word 6 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  nhi_a_hi:7,                 /* NEXT_HOP_INDEX_A (Underlay) */
            nhi_type:2,                 /* Type for NEXT_HOP_INDEX_A and B */
            match_rule:8,               /* Matched FP rule */
            forwarding_type:2,          /* Packet forwarding type */
            ucq:1,                      /* Unicast queue */
            outer_vid:12;               /* Outer VLAN ID */

                                        /* (word 6 in EP_TO_CPU_HDR) */
                                        /* word 5 in EP_TO_CPU_COMPOSITES_BUS */

    union {
        struct {
            uint32  outer_pri:3,                /* Outer priority */
                    inner_cfi:1,                /* Inner Canoncial Format Indicator */
                    inner_pri:3,                /* Inner priority */
                    nhi_b:16,                   /* NEXT_HOP_INDEX_B (Overlay) */
                    nhi_a_lo:9;                 /* NEXT_HOP_INDEX_A (Underlay) */
        } overlay1;
        struct {
            uint32  outer_pri:3,                /* Outer priority */
                    inner_cfi:1,                /* Inner Canoncial Format Indicator */
                    inner_pri:3,                /* Inner priority */
                    bfd_session_index:12,       /* Indicates the Session ID for the Rx BFD packet */
                    :4,                         /* Reserved */
                    nhi_a_lo:9;                 /* NEXT_HOP_INDEX_A (Underlay) */
        } overlay2;
    } word6;                                    /* (word 5 in EP_TO_CPU_HDR) */
                                        /* word 4 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  mtp_index:5,                /* MTP index */
            dscp:6,                     /* New DSCP */
            chg_tos:1,                  /* DSCP has been changed by HW */
            vntag_action:2,             /* VN tag action */
            outer_cfi:1,                /* Outer Canoncial Format Indicator */
            decap_tunnel_type:5,        /* Tunnel type that was decapsulated */
            inner_vid:12;               /* Inner VLAN ID */

#else
                                        /* (word 7 in EP_TO_CPU_HDR) */
                                        /* word 6 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  outer_vid:12,               /* Outer VLAN ID */
            ucq:1,                      /* Unicast queue */
            forwarding_type:2,          /* Packet forwarding type */
            match_rule:8,               /* Matched FP rule */
            nhi_type:2,                 /* Type for NEXT_HOP_INDEX_A and B */
            nhi_a_hi:7;                 /* NEXT_HOP_INDEX_A (Underlay) */

                                        /* (word 6 in EP_TO_CPU_HDR) */
                                        /* word 5 in EP_TO_CPU_COMPOSITES_BUS */
    union {
        struct {
            uint32  nhi_a_lo:9,                 /* NEXT_HOP_INDEX_A (Underlay) */
                    nhi_b:16,                   /* NEXT_HOP_INDEX_B (Overlay) */
                    inner_pri:3,                /* Inner priority */
                    inner_cfi:1,                /* Inner Canoncial Format Indicator */
                    outer_pri:3;                /* Outer priority */
        } overlay1;
        struct {
            uint32  nhi_a_lo:9,                 /* NEXT_HOP_INDEX_A (Underlay) */
                    :4,                         /* Reserved */
                    bfd_session_index:12,       /* Indicates the Session ID for the Rx BFD packet */
                    inner_pri:3,                /* Inner priority */
                    inner_cfi:1,                /* Inner Canoncial Format Indicator */
                    outer_pri:3;                /* Outer priority */
        } overlay2;

    } word6;
                                        /* (word 5 in EP_TO_CPU_HDR) */
                                        /* word 4 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  inner_vid:12,               /* Inner VLAN ID */
            decap_tunnel_type:5,        /* Tunnel type that was decapsulated */
            outer_cfi:1,                /* Outer Canoncial Format Indicator */
            vntag_action:2,             /* VN tag action */
            chg_tos:1,                  /* DSCP has been changed by HW */
            dscp:6,                     /* New DSCP */
            mtp_index:5;                /* MTP index */
#endif

                                        /* (word 4 in EP_TO_CPU_HDR) */
                                        /* word 3 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  timestamp;                  /* Timestamp */

#ifdef  LE_HOST
                                        /* (word 3 in EP_TO_CPU_HDR) */
                                        /* word 2 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  bpdu:1,                     /* BPDU Packet */
            :2,                         /* Reserved */
            hg_lookup_pkt:1,            /* Higig Lookup Packet */
            vrf:12,                     /* VRF */
            timestamp_type:2,           /* Timestamp type */
            l3_iif:14;                  /* L3 IIF */

                                        /* (word 2 in EP_TO_CPU_HDR) */
                                        /* word 1 in EP_TO_CPU_COMPOSITES_BUS*/
    uint32  match_id_hi:18,             /* Packet MATCH_ID upper bits*/
            otag_action:2,              /* Ingress outer tag action */
            itag_action:2,              /* Ingress inner tag action */
            service_tag:1,              /* SD tag present */
            switch_pkt:1,               /* Switched packet */
            uc_switch_drop:1,           /* Unicast switched copies dropped */
            src_hg:1,                   /* Source is Higig */
            forwarding_zone_id:3,       /* Indicates packet was routed - UC or MC */
            l3only:1,                   /* L3 only IPMC packet */
            replicated:1,               /* Replicated copy */
            do_not_change_ttl:1;        /* Do not change TTL */

                                        /* (word 1 in EP_TO_CPU_HDR) */
                                        /* word 0 in EP_TO_CPU_COMPOSITES_BUS*/
    uint32  match_id_lo;                /* Packet MATCH_ID lower bits*/

    union {                             /* (word 0 in EP_TO_CPU_HDR) */
        struct {                        /* EH_TYPE_0 */
            uint32  eh_queue_tag:16,
                    :3,
                    eh_tag_type:2,
                    eh_seg_sel:3,
                    :3,
                    os1588_done:1,
                    eh_type:4;
        } overlay1;
        struct {                        /* EH_TYPE_1 */
            uint32  classid: 9,
                    l3_iif:14,
                    classid_type:4,
                    os1588_done:1,
                    eh_type:4;
        } overlay2;
        struct {                        /* EH_TYPE 2 */
            uint32  eh_queue_tag: 14,
                    classid:9,
                    classid_type:4,
                    os1588_done:1,
                    eh_type:4;
        } overlay3;
    } word16;

#else
                                        /* (word 3 in EP_TO_CPU_HDR) */
                                        /* word 2 in EP_TO_CPU_COMPOSITES_BUS */
    uint32  l3_iif:14,                  /* L3 IIF */
            timestamp_type:2,           /* Timestamp type */
            vrf:12,                     /* VRF */
            hg_lookup_pkt:1,            /* Higig Lookup Packet */
            :2,                         /* Reserved */
            bpdu:1;                     /* BPDU Packet */

                                        /* (word 2 in EP_TO_CPU_HDR) */
                                        /* word 1 in EP_TO_CPU_COMPOSITES_BUS*/
    uint32  do_not_change_ttl:1,        /* Do not change TTL */
            replicated:1,               /* Replicated copy */
            l3only:1,                   /* L3 only IPMC packet */
            forwarding_zone_id:3,       /* Indicates packet was routed - UC or MC */
            src_hg:1,                   /* Source is Higig */
            uc_switch_drop:1,           /* Unicast switched copies dropped */
            switch_pkt:1,               /* Switched packet */
            service_tag:1,              /* SD tag present */
            itag_action:2,              /* Ingress inner tag action */
            otag_action:2,              /* Ingress outer tag action */
            match_id_hi:18;             /* Packet MATCH_ID upper bits*/

                                        /* (word 1 in EP_TO_CPU_HDR) */
                                        /* word 0 in EP_TO_CPU_COMPOSITES_BUS*/
    uint32  match_id_lo;                /* Packet MATCH_ID lower bits*/

    union {                             /* (word 0 in EP_TO_CPU_HDR) */
        struct {                        /* EH_TYPE_0 */
            uint32  eh_type:4,
                    os1588_done:1,
                    :3,
                    eh_seg_sel:3,
                    eh_tag_type:2,
                    :3,
                    eh_queue_tag:16;
        } overlay1;
        struct {                        /* EH_TYPE_1 */
            uint32  eh_type:4,
                    os1588_done:1,
                    classid_type:4,
                    l3_iif:14,
                    classid: 9;
        } overlay2;
        struct {                        /* EH_TYPE 2 */
            uint32  eh_type:4,
                    os1588_done:1,
                    classid_type:4,
                    classid:9,
                    eh_queue_tag:14;
        } overlay3;
    } word16;
#endif

    uint32 rsvd;                        /* padding to get header to 16 words */
} ep_to_cpu_hdr36_t;

#define SOC_CPU_OPCODE_TYPE_IP_0        0
#define SOC_CPU_OPCODE_TYPE_IP_1        1
#define SOC_CPU_OPCODE_TYPE_EP          2
#define SOC_CPU_OPCODE_TYPE_NLF         3

#endif /* _SOC_SHARED_DCBFORMATS_TYPE36_H */
