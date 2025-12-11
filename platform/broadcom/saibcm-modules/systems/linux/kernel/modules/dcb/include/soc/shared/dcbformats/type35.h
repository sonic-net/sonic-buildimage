/*
 * $Id$
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
 * File:        soc/shared/dcbformats/type35.h
 * Purpose:     Define dma control block (DCB) format for a type35 DCB
 *              used by the 56670 (MONTEREY)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE35_H
#define _SOC_SHARED_DCBFORMATS_TYPE35_H

/*
 * DMA Control Block - Type 35
 * Used on 56560 devices
 * 16 words
 */
typedef struct {
    uint32  addr;                       /* T32.0: physical address */
                                        /* T32.1: Control 0 */
#ifdef  LE_HOST
    uint32  c_count:16,                 /* Requested byte count */
            c_chain:1,                  /* Chaining */
            c_sg:1,                     /* Scatter Gather */
            c_reload:1,                 /* Reload */
            c_hg:1,                     /* Higig (TX) */
            c_stat:1,                   /* Update stats (TX) */
            c_pause:1,                  /* Pause packet (TX) */
            c_purge:1,                  /* Purge packet (TX) */
            :9;                         /* Don't care */
#else
    uint32  :9,                         /* Don't care */
            c_purge:1,                  /* Purge packet (TX) */
            c_pause:1,                  /* Pause packet (TX) */
            c_stat:1,                   /* Update stats (TX) */
            c_hg:1,                     /* Higig (TX) */
            c_reload:1,                 /* Reload */
            c_sg:1,                     /* Scatter Gather */
            c_chain:1,                  /* Chaining */
            c_count:16;                 /* Requested byte count */
#endif  /* LE_HOST */

                                        /* T32.2 (word 12 in EP_TO_CPU_HDR) */
    uint32  reason_hi;                  /* CPU opcode (hi bits) */
                                        /* T32.3 (word 11 in EP_TO_CPU_HDR) */
    uint32  reason;                     /* CPU opcode (low bits) */

#ifdef  LE_HOST
    union {                             /* T26.4 (word 10 in EP_TO_CPU_HDR) */
        struct {
            uint32  queue_num:15,       /* Queue number for CPU packets */
                    ep_mirror:1,        /* EP_MIRROR */
                    cpu_opcode_type:4,  /* CPU opcode type */
                    outer_vid:12;       /* Outer VLAN ID */
        } overlay1;
        struct {
            uint32  cpu_cos:6,          /* CPU COS */
                    :10,                /* Reserved */
                    cpu_opcode_type:4,  /* CPU opcode type */
                    outer_vid:12;       /* Outer VLAN ID */
        } overlay2;
    } word4;

                                        /* T26.5 (word 9 in EP_TO_CPU_HDR) */
    uint32  srcport:7,                  /* Source port number */
            :1,                         /* Reserved */
            pkt_len:14,                 /* Packet length */
            match_rule:8,               /* Matched FP rule */
            hgi:2;                      /* Higig Interface Format Indicator */
#else
    union {                             /* T26.4 (word 10 in EP_TO_CPU_HDR) */
        struct {
            uint32  outer_vid:12,       /* Outer VLAN ID */
                    cpu_opcode_type:4,  /* CPU opcode type */
                    ep_mirror:1,        /* EP_MIRROR */
                    queue_num:15;       /* Queue number for CPU packets */
        } overlay1;
        struct {
            uint32  outer_vid:12,       /* Outer VLAN ID */
                    cpu_opcode_type:4,  /* CPU opcode type */
                    :10,                /* Reserved */
                    cpu_cos:6;          /* CPU COS */
        } overlay2;
    } word4;

                                        /* T26.5 (word 9 in EP_TO_CPU_HDR) */
    uint32  hgi:2,                      /* Higig Interface Format Indicator */
            match_rule:8,               /* Matched FP rule */
            pkt_len:14,                 /* Packet length */
            :1,                         /* Reserved */
            srcport:7;                  /* Source port number */
#endif
                                        /* T32.6 (word 8 in EP_TO_CPU_HDR) */
    uint32  mh0;                        /* Module Header word 0 */
                                        /* T32.7 (word 7 in EP_TO_CPU_HDR) */
    uint32  mh1;                        /* Module Header word 1 */
                                        /* T32.8 (word 6 in EP_TO_CPU_HDR) */
    uint32  mh2;                        /* Module Header word 2 */
                                        /* T32.9 (word 5 in EP_TO_CPU_HDR) */
    uint32  mh3;                        /* Module Header word 3 */

#ifdef  LE_HOST
                                        /* T32.10 (word 4 in EP_TO_CPU_HDR) */
    uint32  inner_pri:3,                /* Inner priority */
            dvp_nhi_sel:1,              /* DVP overlay indicator */
            regen_crc:1,                /* Packet modified and needs new CRC */
            repl_nhi:17,                /* Replication or Next Hop index */
            :2,                         /* Reserved */
            vfi_valid:1,                /* Validates VFI field */
            olp_encap_oam_pkt:1,        /* OLP encapsulated OAM packet */
            :6;                         /* Reserved */

    union {                                     /* T33.11 (word 3 in EP_TO_CPU_HDR) */
        struct {
            uint32  dscp:6,                     /* New DSCP */
                    chg_tos:1,                  /* DSCP has been changed by HW */
                    vntag_action:2,             /* VN tag action */
                    outer_cfi:1,                /* Outer Canoncial Format Indicator */
                    outer_pri:3,                /* Outer priority */
                    decap_tunnel_type:4,        /* Tunnel type that was decapsulated */
                    :1,                         /* Reserved */
                    inner_vid:12,               /* Inner VLAN ID */
                    inner_cfi:1,                /* Inner Canoncial Format Indicator */
                    mcq:1;                      /* Multicast queue */
        } overlay1;

        struct {
            uint32  special_pkt_type:3, /* Special packet type (OAM, BFD) */
                    :29;
        } overlay2;
    } word3;
#else
                                        /* T32.10 (word 4 in EP_TO_CPU_HDR) */
    uint32  :6,                         /* Reserved */
            olp_encap_oam_pkt:1,        /* OLP encapsulated OAM packet */
            vfi_valid:1,                /* Validates VFI field */
            :2,                         /* Reserved */
            repl_nhi:17,                /* Replication or Next Hop index */
            regen_crc:1,                /* Packet modified and needs new CRC */
            dvp_nhi_sel:1,              /* DVP overlay indicator */
            inner_pri:3;                /* Inner priority */

    union {                                 /* T33.11 (word 3 in EP_TO_CPU_HDR) */
        struct {
            uint32  mcq:1,                      /* Multicast queue */
                inner_cfi:1,                /* Inner Canoncial Format Indicator */
                inner_vid:12,               /* Inner VLAN ID */
                :1,                         /* Reserved */
                decap_tunnel_type:4,        /* Tunnel type that was decapsulated */
                outer_pri:3,                /* Outer priority */
                outer_cfi:1,                /* Outer Canoncial Format Indicator */
                vntag_action:2,             /* VN tag action */
                chg_tos:1,                  /* DSCP has been changed by HW */
                dscp:6;                     /* New DSCP */
        } overlay1;

        struct {
            uint32  :29,
                    special_pkt_type:3;
        } overlay2;
    } word3;
#endif
                                        /* T32.12 (word 2 in EP_TO_CPU_HDR) */
    uint32  timestamp;                  /* Timestamp */

#ifdef  LE_HOST
                                        /* T32.13 (word 1 in EP_TO_CPU_HDR) */
    uint32  tag_status:2,               /* Ingress incoming tag status */
            otag_action:2,              /* Ingress outer tag action */
            itag_action:2,              /* Ingress inner tag action */
            service_tag:1,              /* SD tag present */
            switch_pkt:1,               /* Switched packet */
            all_switch_drop:1,          /* All switched copies dropped */
            hg_type:1,                  /* 0: Higig+, 1: Higig2 */
            src_hg:1,                   /* Source is Higig */
            l3routed:1,                 /* Any IP routed packet */
            l3only:1,                   /* L3 only IPMC packet */
            replicated:1,               /* Replicated copy */
            hg2_ext_hdr:1,              /* Extended Higig2 header valid */
            special_pkt:1,              /* Special Packet indicator */
            do_not_change_ttl:1,        /* Do not change TTL */
            bpdu:1,                     /* BPDU Packet */
            timestamp_type:2,           /* Timestamp type */
            loopback_pkt_type:3,        /* Loopback packet type */
            :1,                         /* Reserved */
            mtp_index:5,                /* MTP index */
            ecn:2,                      /* New ECN value */
            chg_ecn:1;                  /* ECN changed */

    union {                             /* T32.14 (word 0 in EP_TO_CPU_HDR) */
        uint32  timestamp_hi;           /* Timestamp upper bits */
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
        struct {
            uint32  rx_bfd_session_index:12,    /* Rx BFD pkt session ID */
                    rx_bfd_start_offset_type:2, /* Rx BFD pkt st offset type */
                    rx_bfd_start_offset:8,      /* Rx BFD pkt start offset */
                    :10;                        /* Reserved */
        } overlay4;
    } word14;

                                        /* T32.15: DMA Status 0 */
    uint32  count:16,                   /* Transferred byte count */
            end:1,                      /* End bit (RX) */
            start:1,                    /* Start bit (RX) */
            error:1,                    /* Cell Error (RX) */
            :12,                        /* Reserved */
            done:1;                     /* Descriptor Done */
#else
                                        /* T32.13 (word 1 in EP_TO_CPU_HDR) */
    uint32  chg_ecn:1,                  /* ECN changed */
            ecn:2,                      /* New ECN value */
            mtp_index:5,                /* MTP index */
            :1,                         /* Reserved */
            loopback_pkt_type:3,        /* Loopback packet type */
            timestamp_type:2,           /* Timestamp type */
            bpdu:1,                     /* BPDU Packet */
            do_not_change_ttl:1,        /* Do not change TTL */
            special_pkt:1,              /* Special Packet indicator */
            hg2_ext_hdr:1,              /* Extended Higig2 header valid */
            replicated:1,               /* Replicated copy */
            l3only:1,                   /* L3 only IPMC packet */
            l3routed:1,                 /* Any IP routed packet */
            src_hg:1,                   /* Source is Higig */
            hg_type:1,                  /* 0: Higig+, 1: Higig2 */
            all_switch_drop:1,          /* All switched copies dropped */
            switch_pkt:1,               /* Switched packet */
            service_tag:1,              /* SD tag present */
            itag_action:2,              /* Ingress inner tag action */
            otag_action:2,              /* Ingress outer tag action */
            tag_status:2;               /* Ingress incoming tag status */

    union {                             /* T32.14 (word 0 in EP_TO_CPU_HDR) */
        uint32  timestamp_hi;           /* Timestamp upper bits */
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
        struct {
            uint32  :10,                        /* Reserved */
                    rx_bfd_start_offset:8,      /* Rx BFD pkt start offset */
                    rx_bfd_start_offset_type:2, /* Rx BFD pkt st offset type */
                    rx_bfd_session_index:12;    /* Rx BFD pkt session ID */
        } overlay4;
    } word14;

                                        /* T32.15: DMA Status 0 */
    uint32  done:1,                     /* Descriptor Done */
            :12,                        /* Reserved */
            error:1,                    /* Cell Error (RX) */
            start:1,                    /* Start bit (RX) */
            end:1,                      /* End bit (RX) */
            count:16;                   /* Transferred byte count */
#endif
} dcb35_t;

#define SOC_CPU_OPCODE_TYPE_IP_0        0
#define SOC_CPU_OPCODE_TYPE_IP_1        1
#define SOC_CPU_OPCODE_TYPE_EP          2
#define SOC_CPU_OPCODE_TYPE_NLF         3

#endif /* _SOC_SHARED_DCBFORMATS_TYPE35_H */
