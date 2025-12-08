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
 * File:        soc/shared/dcbformats/type34.h
 * Purpose:     Define dma control block (DCB) format for a type34 DCB
 *              used by the 56160 (Hurricane3)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE34_H
#define _SOC_SHARED_DCBFORMATS_TYPE34_H

/*
 * DMA Control Block - Type 34
 * Used on 56160 devices
 * 16 words
 */
typedef struct {
        uint32  addr;                   /* T34.0: physical address */
                                        /* T34.1: Control 0 */
#ifdef  LE_HOST
        uint32  c_count:16,             /* Requested byte count */
                c_chain:1,              /* Chaining */
                c_sg:1,                 /* Scatter Gather */
                c_reload:1,             /* Reload */
                c_hg:1,                 /* Higig (TX) */
                c_stat:1,               /* Update stats (TX) */
                c_pause:1,              /* Pause packet (TX) */
                c_purge:1,              /* Purge packet (TX) */
                :9;                     /* Don't care */
#else
        uint32  :9,                     /* Don't care */
                c_purge:1,              /* Purge packet (TX) */
                c_pause:1,              /* Pause packet (TX) */
                c_stat:1,               /* Update stats (TX) */
                c_hg:1,                 /* Higig (TX) */
                c_reload:1,             /* Reload */
                c_sg:1,                 /* Scatter Gather */
                c_chain:1,              /* Chaining */
                c_count:16;             /* Requested byte count */
#endif  /* LE_HOST */
        uint32  reason_hi;              /* T34.2: CPU opcode (hi bits) */
        uint32  reason;                 /* T34.3: CPU opcode (low bits) */
#ifdef  LE_HOST
    union {                             /* T34.4 */
        struct {
            uint32  :16,                /* Reserved */
                    cpu_opcode_type:4,  /* CPU opcode type */
                    outer_vid:12;       /* Outer VLAN ID */
        } overlay1;
        struct {
            uint32  cpu_cos:6,          /* CPU COS */
                    regen_crc:1,        /* Packet modified and needs new CRC */
                    :25;                /* Reserved */
        } overlay2;
    } word4;

                                        /* T34.5 */
        uint32  srcport:8,              /* Source port number */
                pkt_len:14,             /* Packet length */
                match_rule:8,           /* Matched FP rule */
                hgi:2;                  /* Higig Interface Format Indicator */
#else
    union {                             /* T34.4 */
        struct {
            uint32  outer_vid:12,       /* Outer VLAN ID */
                    cpu_opcode_type:4,  /* CPU opcode type */
                    :16;                /* Reserved */
        } overlay1;
        struct {
            uint32  :25,                /* Reserved */
                    regen_crc:1,        /* Packet modified and needs new CRC */
                    cpu_cos:6;          /* CPU COS */
        } overlay2;
    } word4;

                                        /* T34.5 */
        uint32  hgi:2,                  /* Higig Interface Format Indicator */
                match_rule:8,           /* Matched FP rule */
                pkt_len:14,             /* Packet length */
                srcport:8;              /* Source port number */
#endif

        uint32  mh0;                    /* T34.6: Module Header word 0 */
        uint32  mh1;                    /* T34.7: Module Header word 1 */
        uint32  mh2;                    /* T34.8: Module Header word 2 */
        uint32  mh3;                    /* T34.9: Module Header word 3 */
#ifdef  LE_HOST
                                        /* T34.10 */
        uint32  inner_pri:3,            /* Inner priority */
                repl_nhi:13,            /* Replication or Next Hop index */
                :5,                     /* reserved.*/
                shaping_cos_sel:2,      /* */                    
                vfi_valid:1,            /* Validates VFI field */
                em_mtp_index:4,         /* em mirror-to-port index */
                oam_command:4;          /* Special packet type (OAM, BFD) */

                                        /* T34.11 */
        uint32  dscp:6,                 /* New DSCP (+ oam pkt type:3) */
                chg_tos:1,              /* DSCP has been changed by HW */
                imirror:1,              /* Ingress Mirroring PBE field*/
                emirror:1,              /* Egress Mirroring, PBE field */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_pri:3,            /* Outer priority */
                :5,                     /* Reserved */
                inner_vid:12,           /* Inner VLAN ID */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                replicated:1;           /* Replicated copy */

    

    union {                             /* T34.12 */
        struct {
            uint32  timestamp;          /* Timestamp */
        } overlay1;
        struct {
            uint32  vfi:14,             /* VFI or FID */
                    :18;                /* Reserved */
        } overlay2;
    } word12;

                                        /* T34.13 */
        uint32  itag_status:2,          /* Ingress Inner tag status */
                otag_action:2,          /* Ingress Outer tag action */
                itag_action:2,          /* Ingress Inner tag action */
                service_tag:1,          /* SD tag present */
                switch_pkt:1,           /* Switched packet */
                hg_lookup:1,            /* Higig lookup */
                hg_type:1,              /* 0: Higig+, 1: Higig2 */
                src_hg:1,               /* Source is Higig */
                l3routed:1,             /* Any IP routed packet */
                l3only:1,               /* L3 only IPMC packet */
                vntag_action:2,         /* VNTAG/ETAG*/
                oam_pkt:1,              /* OAM Packet indicator */
                do_not_change_ttl:1,    /* Do not change TTL */
                bpdu:1,                 /* BPDU Packet */
                timestamp_type:2,       /* Timestamp type */
                cos:4,                  /* Port queue number */
                mtp_index:5,            /* IM MTP index, PBE field */
                ecn:2,                  /* New ECN value */
                chg_ecn:1;              /* ECN changed */

                                        /* T34.14 */
        uint32  timestamp_hi;           /* Timestamp upper bits */

                                        /* T34.15: DMA Status 0 */
        uint32  count:16,               /* Transferred byte count */
                end:1,                  /* End bit (RX) */
                start:1,                /* Start bit (RX) */
                error:1,                /* Cell Error (RX) */
                :12,                    /* Reserved */
                done:1;                 /* Descriptor Done */
#else
                                        /* T34.10 */
        uint32  oam_command:4,          /* Special packet type (OAM, BFD) */
                em_mtp_index:4,         /* em mirror-to-port index */
                vfi_valid:1,            /* Validates VFI field */
                shaping_cos_sel:2,      /* */
                :5,                     /* reserved.*/
                repl_nhi:13,            /* Replication or Next Hop index */
                inner_pri:3;            /* Inner priority */

                                        /* T34.11 */
        uint32  replicated:1,           /* Replicated copy */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_vid:12,           /* Inner VLAN ID */
                :5,                     /* Reserved */
                outer_pri:3,            /* Outer priority */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                emirror:1,              /* Egress Mirroring, PBE field */
                imirror:1,              /* Ingress Mirroring PBE field*/
                chg_tos:1,              /* DSCP has been changed by HW */
                dscp:6;                 /* New DSCP (+ oam pkt type:3) */

    union {                             /* T34.12 */
        struct {
            uint32  timestamp;          /* Timestamp */
        } overlay1;
        struct {
            uint32  :18,                /* Reserved */
                    vfi:14;             /* VFI or FID */
        } overlay2;
    } word12;

                                        /* T34.13 */
        uint32  chg_ecn:1,              /* ECN changed */
                ecn:2,                  /* New ECN value */
                mtp_index:5,            /* IM MTP index, PBE field */
                cos:4,                  /* Port queue number */
                timestamp_type:2,       /* Timestamp type */
                bpdu:1,                 /* BPDU Packet */
                do_not_change_ttl:1,    /* Do not change TTL */
                oam_pkt:1,              /* OAM Packet indicator */
                vntag_action:2,         /* VNTAG/ETAG*/
                l3only:1,               /* L3 only IPMC packet */
                l3routed:1,             /* Any IP routed packet */
                src_hg:1,               /* Source is Higig */
                hg_type:1,              /* 0: Higig+, 1: Higig2 */
                hg_lookup:1,            /* Higig lookup */
                switch_pkt:1,           /* Switched packet */
                service_tag:1,          /* SD tag present */
                itag_action:2,          /* Ingress Inner tag action */
                otag_action:2,          /* Ingress Outer tag action */
                itag_status:2;          /* Ingress Inner tag status */

    
        uint32  timestamp_hi;           /* Timestamp upper bits */    

                                        /* T34.15: DMA Status 0 */
        uint32  done:1,                 /* Descriptor Done */
                :12,                    /* Reserved */
                error:1,                /* Cell Error (RX) */
                start:1,                /* Start bit (RX) */
                end:1,                  /* End bit (RX) */
                count:16;               /* Transferred byte count */
#endif
} dcb34_t;

#define SOC_CPU_OPCODE_TYPE_IP_0        0
#define SOC_CPU_OPCODE_TYPE_IP_1        1
#define SOC_CPU_OPCODE_TYPE_EP          2
#define SOC_CPU_OPCODE_TYPE_NLF         3

#endif

