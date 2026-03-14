/*
 * $Id: type19.h,v 1.4 Broadcom SDK $
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
 * File:        soc/shared/dcbformats/type19.h
 * Purpose:     Define dma control block (DCB) format for a type19 DCB
 *              used by the 56640 (Triumph3/Firebolt4)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE19_H
#define _SOC_SHARED_DCBFORMATS_TYPE19_H

/*
 * DMA Control Block - Type 19
 * Used on 5663x devices
 * 16 words
 */
typedef struct {
        uint32  addr;                   /* T19.0: physical address */
                                        /* T19.1: Control 0 */
#ifdef  LE_HOST
        uint32  c_count:16,             /* Requested byte count */
                c_chain:1,              /* Chaining */
                c_sg:1,                 /* Scatter Gather */
                c_reload:1,             /* Reload */
                c_hg:1,                 /* Higig (TX) */
                c_stat:1,               /* update stats (TX) */
                c_pause:1,              /* Pause packet (TX) */
                c_purge:1,              /* Purge packet (TX) */
                :9;                     /* Don't care */
#else
        uint32  :9,                     /* Don't care */
                c_purge:1,              /* Purge packet (TX) */
                c_pause:1,
                c_stat:1,
                c_hg:1,
                c_reload:1,
                c_sg:1,
                c_chain:1,
                c_count:16;
#endif  /* LE_HOST */
        uint32  mh0;                    /* T19.2: Module Header word 0 */
        uint32  mh1;                    /* T19.3: Module Header word 1 */
        uint32  mh2;                    /* T19.4: Module Header word 2 */
        uint32  mh3;                    /* T19.5: Module Header word 3 */
#ifdef  LE_HOST
                                        /* T19.6: RX Status 0 */
        uint32  :3,                     /* Reserved */
                mtp_index:5,            /* MTP index */
                cpu_cos:6,              /* COS queue for CPU packets */
                :2,                     /* Reserved */
                inner_vid:12,           /* Inner VLAN ID */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_pri:3;            /* Inner priority */

                                        /* T19.7 */ 
        uint32  reason_hi:16,           /* CPU opcode (high bits) */
                pkt_len:14,             /* Packet length */
                :2;                     /* Reserved */

                                        /* T19.8 */
        uint32  reason;                 /* CPU opcode */

                                        /* T19.9 */
        uint32  dscp:8,                 /* New DSCP */
                chg_tos:1,              /* DSCP has been changed by HW */
                decap_tunnel_type:4,    /* Tunnel type that was decapsulated */
                regen_crc:1,            /* Packet modified and needs new CRC */
                :2,                     /* Reserved */
                outer_vid:12,           /* Outer VLAN ID */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_pri:3;            /* Outer priority */

                                        /* T19.10 */
        uint32  timestamp;              /* Timestamp */

                                        /* T19.11 */
        uint32  cos:4,                  /* COS */
                higig_cos:5,            /* Higig COS */
                vlan_cos:5,             /* VLAN COS */
                shaping_cos_sel:2,      /* Shaping COS Select */
                vfi:12,                 /* Internal VFI value */
                :4;                     /* Reserved */

                                        /* T19.12 */
        uint32  srcport:8,              /* Source port number */
                hgi:2,                  /* Higig Interface Format Indicator */
                itag_status:2,          /* Ingress incoming tag status */
                otag_action:2,          /* Ingress Outer tag action */
                itag_action:2,          /* Ingress Inner tag action */
		service_tag:1,          /* SD tag present */
		switch_pkt:1,           /* Switched packet */
                hg_type:1,              /* 0: Higig+, 1: Higig2 */
                src_hg:1,               /* Source is Higig */
                l3routed:1,             /* Any IP routed packet */
                l3only:1,               /* L3 only IPMC packet */
                replicated:1,           /* Replicated copy */
                imirror:1,              /* Ingress Mirroring */
                emirror:1,              /* Egress Mirroring */
                do_not_change_ttl:1,    /* Do not change TTL */
                bpdu:1,                 /* BPDU Packet */
                :5;                     /* Reserved */

                                        /* T19.13 */
        uint32  orig_dstport:6,         /* Original dst port (EP redirection) */
                true_egr_mirror:1,      /* True egress mirrored */
                egr_cpu_copy:1,         /* True egress copy-to-CPU */
                oam_pkt:1,              /* OAM packet */
                eh_tbl_idx:2,           /* Extended Higig table select */
                eh_tag_type:2,          /* Extended Higig tag type */
                eh_tm:1,                /* Extended Higig traffic manager ctrl */
                eh_queue_tag:16,        /* Extended Higig queue tag */
                :2;                     /* Reserved */

                                        /* T19.14 */
        uint32  l3_intf:15,             /* L3 Intf num / Next hop idx */
                :1,                     /* Reserved */
                match_rule:8,           /* Matched FP rule */
                :8;                     /* Reserved */

                                         /* T19.15: DMA Status 0 */
        uint32  count:16,                /* Transferred byte count */
                end:1,                   /* End bit (RX) */
                start:1,                 /* Start bit (RX) */
                error:1,                 /* Cell Error (RX) */
                pktdata_read_ecc_error:1,/* 2 bit ECC error while reading packet
                                            data from RX data Buffers.*/
                status_read_ecc_error:1, /* 2 bit ECC error while reading Status
                                            from RX Status Buffers.*/
                dc:10,                   /* Don't Care */
                done:1;                  /* Descriptor Done */
#else
                                        /* T19.6: RX Status 0 */
        uint32  inner_pri:3,            /* Inner priority */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_vid:12,           /* Inner VLAN ID */
                :2,                     /* Reserved */
                cpu_cos:6,              /* COS queue for CPU packets */
                mtp_index:5,            /* MTP index */
                :3;                     /* Reserved */

                                        /* T19.7 */
        uint32  :2,                     /* Reserved */
                pkt_len:14,             /* Packet length */
                reason_hi:16;           /* CPU opcode (high bits) */

                                        /* T19.8 */
        uint32  reason;                 /* CPU opcode */

                                        /* T19.9 */
        uint32  outer_pri:3,            /* Outer priority */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_vid:12,           /* Outer VLAN ID */
                :2,                     /* Reserved */
                regen_crc:1,            /* Packet modified and needs new CRC */
                decap_tunnel_type:4,    /* Tunnel type that was decapsulated */
                chg_tos:1,              /* DSCP has been changed by HW */
                dscp:8;                 /* New DSCP */

                                        /* T19.10 */
        uint32  timestamp;              /* Timestamp */

                                        /* T19.11 */
        uint32  :4,                     /* Reserved */
                vfi:12,                 /* Internal VFI value */
                shaping_cos_sel:2,      /* Shaping COS Select */
                vlan_cos:5,             /* VLAN COS */
                higig_cos:5,            /* Higig COS */
                cos:4;                  /* COS */

                                        /* T19.12 */
        uint32  :5,                     /* Reserved */
                bpdu:1,                 /* BPDU Packet */
                do_not_change_ttl:1,    /* Do not change TTL */
                emirror:1,              /* Egress Mirroring */
                imirror:1,              /* Ingress Mirroring */
		replicated:1,           /* Replicated copy */
		l3only:1,               /* L3 only IPMC packet */
                l3routed:1,             /* Any IP routed packet */
                src_hg:1,               /* Source is Higig */
                hg_type:1,              /* 0: Higig+, 1: Higig2 */
                switch_pkt:1,           /* Switched packet */
                service_tag:1,          /* SD tag present */
                itag_action:2,          /* Ingress Inner tag action */
                otag_action:2,          /* Ingress Outer tag action */
                itag_status:2,          /* Ingress incoming tag status */
                hgi:2,                  /* Higig Interface Format Indicator */
                srcport:8;              /* Source port number */

                                        /* T19.13 */
        uint32  :2,                     /* Reserved */
                eh_queue_tag:16,        /* Extended Higig queue tag */
                eh_tm:1,                /* Extended Higig traffic manager ctrl */
                eh_tag_type:2,          /* Extended Higig tag type */
                eh_tbl_idx:2,           /* Extended Higig table select */
                oam_pkt:1,              /* OAM packet */
                egr_cpu_copy:1,         /* True egress copy-to-CPU */
                true_egr_mirror:1,      /* True egress mirrored */
                orig_dstport:6;         /* Original dst port (EP redirection) */

                                        /* T19.14 */
        uint32  :8,                     /* Reserved */
                match_rule:8,           /* Matched FP rule */
                :1,                     /* Reserved */
                l3_intf:15;             /* L3 Intf num / Next hop idx */
 
                                         /* T19.15: DMA Status 0 */
        uint32  done:1,                  /* Descriptor Done */
                dc:10,                   /* Don't Care */
                status_read_ecc_error:1, /* 2 bit ECC error while reading Status
                                            from RX Status Buffers. */
                pktdata_read_ecc_error:1,/* 2 bit ECC error while reading packet
                                            data from RX data Buffers.*/
                error:1,                 /* Cell Error (RX) */
                start:1,                 /* Start bit (RX) */
                end:1,                   /* End bit (RX) */
                count:16;                /* Transferred byte count */
#endif
} dcb19_t;

#endif
