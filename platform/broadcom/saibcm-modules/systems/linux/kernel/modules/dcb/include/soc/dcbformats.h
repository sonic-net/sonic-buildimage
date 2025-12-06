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
 * File:        soc/dcbformats.h
 * Purpose:     Define dma control block (DCB) formats for various chips.
 *              This information is only needed in soc/dcb.c, which
 *              provides a uniform interface to the reset of software
 *              for controlling DCBs.
 *
 * There are currently 32 DMA Control block formats
 * (also known as DMA descriptors):
 *
 *      Type 2          5670/75 (TX)
 *      Type 3          5690 aka DRACO1
 *      Type 4          5670/75 (RX) aka HERCULES
 *      Type 5          5673 aka LYNX
 *      Type 6          5665 aka TUCANA
 *      Type 7          5695 aka DRACO15
 *      Type 9          56504 aka XGS3 (Firebolt)
 *      Type 10         56601 aka XGS3 (Easyrider)
 *      Type 11         56800 aka XGS3 (HUMV)
 *      Type 12         56218 aka XGS3 (Raptor) (Derived from DCB 9)
 *      Type 13         56514 aka XGS3 (Firebolt2) (Derived from DCB 9)
 *      Type 14         56624 & 56680 aka XGS3 (Triumph & Valkyrie)
 *      Type 15         56224 aka XGS3 (Raven A0) (Derived from DCB 12)
 *      Type 16         56820 aka XGS3 (Scorpion)
 *      Type 17         53314 aka XGS3 (Hawkeye) (Derived from DCB 15)
 *      Type 18         56224 aka XGS3 (Raven B0) (Derived from DCB 15)
 *      Type 19         56634/56524/56685 aka XGS3 (Triumph2/Apollo/Valkyrie2)
 *      Type 20         56334 aka XGS3 (Enduro) (Derived from DCB 19)
 *      Type 21         56840 aka XGS3 (Trident)
 *      Type 23         56640 aka XGS3 (Triumph3)
 *      Type 24         56440 aka XGS3 (Katana)
 *      Type 25         Obsolete 88030 aka SBX Caladan3
 *      Type 26         56850 aka XGS3 (Trident2) (Derived from DCB 23)
 *      Type 27         Obsolete 88230 aka SBX Sirius
 *      Type 29         56450 aka XGX3 (Katana2)
 *      Type 31         53400 aka XGX3 (Greyhound)
 *      Type 32         56960 aka XGX3 (Tomahawk) (Derived from DCB 26)
 *      Type 33         56860 aka XGS3 (Trident2+) (Derived from DCB 26)
 *      Type 34         56160 aka XGX3 (Hurricane3)
 *      Type 35         56560 APACHE
 *      Type 36         56870 (Trident 3)
 *      Type 37         53570 aka XGX3 (Greyhound2)
 *      Type 38         56980 (Tomahawk 3)
 *
 * The 5670/75 uses two different formats depending on whether the DCB is
 * for transmit or receive.
 *
 * The 5695 can be configured to use type 3 or type 7 depending on whether
 * CMIC_CONFIG.EXTENDED_DCB_ENABLE is set or not.
 */

#ifndef _SOC_DCBFORMATS_H
#define _SOC_DCBFORMATS_H

#include <soc/types.h>
#include <soc/defs.h>

#define DCB_STRATA_OPTIM_PURGE  3
#define DCB_STRATA_CRC_LEAVE    0
#define DCB_STRATA_CRC_REGEN    2
#define DCB_XGS_CRC_LEAVE       0
#define DCB_XGS_CRC_REGEN       1

#define DCB_MAX_REQCOUNT        0x7fff          /* 32KB */
#define DCB_MAX_SIZE            (16*4)          /* type 14 */

#if defined(BCM_XGS3_SWITCH_SUPPORT)
/*
 * DMA Control Block - Type 9
 * Used on 56504 devices
 * 11 words
 */
typedef struct {
        uint32  addr;                   /* T9.0: physical address */
                                        /* T9.1: Control 0 */
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
        uint32  mh0;                    /* T9.2: Module Header word 0 */
        uint32  mh1;                    /* T9.3: Module Header word 1 */
        uint32  mh2;                    /* T9.4: Module Header word 2 */
#ifdef  LE_HOST
                                        /* T9.5: RX Status 0 */
        uint32  mtp_index_hi:5,         /* Mirror-to-Port Index */
                l3uc:1,                 /* L3 UC */
                l3only:1,               /* L3 only */
                l3ipmc:1,               /* L3 IPMC */
                l3_intf:12,             /* L3 Intf number */
                imirror:1,              /* Ingress Mirroring */
                emirror:1,              /* Egress Mirroring */
                cos:3,                  /* Packet Priority */
                cpu_cos:3,              /* CPU COS */
                chg_tos:1,              /* DSCP Changed */
                cell_error:1,           /* Cell CRC Checksum Error Detected */
                bpdu:1,                 /* BPDU Packet */
                add_vid:1;              /* VLAN ID Added */
                                        /* T9.6: RX Status 1 */
        uint32  reason:25,              /* CPU opcode */
                regen_crc:1,            /* Regenerate CRC */
                switch_pkt:1,           /* Switched packet */
                src_hg:1,               /* Source is Higig */
                purge_cell:1,           /* Packet is marked Purged */
                pkt_aged:1,             /* Pkt is Aged */
                mtp_index_lo:2;         /* Mirror-to-Port Index */
                                        /* T9.7: RX Status 2 */
        uint32  dscp_hi:2,              /* New DSCP */
                srcport:5,              /* Source port */
                nh_index:13,            /* Next hop index */
                match_rule:10,          /* Matched Rule */
                match_rule_valid:1,     /* Matched Rule valid */
                decap_iptunnel:1;       /* Decap IP Tunneling Packet */

                                        /* T9.8: RX Status 3 */
        uint32  :11,
                ingress_untagged:1,     /* Pkt came in untagged */
                outer_vid:12,           /* VID */
                outer_cfi:1,            /* CFI */
                outer_pri:3,            /* Priority */
                dscp_lo:4;              /* New DSCP */
#else
                                        /* T9.5: RX Status 0 */
        uint32  add_vid:1,              /* VLAN ID Added */
                bpdu:1,                 /* BPDU Packet */
                cell_error:1,           /* Cell CRC Checksum Error Detected */
                chg_tos:1,              /* DSCP Changed */
                cpu_cos:3,              /* CPU COS */
                cos:3,                  /* Packet Priority */
                emirror:1,              /* Egress Mirroring */
                imirror:1,              /* Ingress Mirroring */
                l3_intf:12,             /* L3 Intf number */
                l3ipmc:1,               /* L3 IPMC */
                l3only:1,               /* L3 only */
                l3uc:1,                 /* L3 UC */
                mtp_index_hi:5;         /* Mirror-to-Port Index */
                                        /* T9.6: RX Status 1 */
        uint32  mtp_index_lo:2,         /* Mirror-to-Port Index */
                pkt_aged:1,             /* Pkt is Aged */
                purge_cell:1,           /* Packet is marked Purged */
                src_hg:1,               /* Source is Higig */
                switch_pkt:1,           /* Switched packet */
                regen_crc:1,            /* Regenerate CRC */
                reason:25;              /* CPU opcode */
                                        /* T9.7: RX Status 2 */
        uint32  decap_iptunnel:1,       /* Decap IP Tunneling Packet */
                match_rule_valid:1,     /* Matched Rule valid */
                match_rule:10,          /* Matched Rule */
                nh_index:13,            /* Next hop index */
                srcport:5,              /* Source port */
                dscp_hi:2;              /* New DSCP */
                                        /* T9.8: RX Status 3 */
        uint32  dscp_lo:4,              /* New DSCP */
                outer_pri:3,            /* Priority (D)*/
                outer_cfi:1,            /* CFI (D)*/
                outer_vid:12,           /* VID (D)*/
                ingress_untagged:1,     /* Pkt came in untagged (D)*/
                :11;
#endif

        uint32  s_4;                    /* T9.9: RX Status 4 */
                                        /* T9.10: DMA Status 0 */
#ifdef  LE_HOST
        uint32  count:16,               /* Transferred byte count */
                end:1,                  /* End bit (RX) */
                start:1,                /* Start bit (RX) */
                error:1,                /* Cell Error (RX) */
                dc:12,                  /* Don't Care */
                done:1;                 /* Descriptor Done */
#else
        uint32  done:1,
                dc:12,
                error:1,
                start:1,
                end:1,
                count:16;
#endif
} dcb9_t;
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if defined(BCM_TRIUMPH2_SUPPORT)
/*
 * DMA Control Block - Type 19
 * Used on 5663x devices
 * 16 words
 */
#include <soc/shared/dcbformats/type19.h>
#endif  /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_ENDURO_SUPPORT)
/*
 * DMA Control Block - Type 20
 * Used on 5623x devices
 * 16 words
 */
typedef struct {
        uint32  addr;                   /* T20.0: physical address */
                                        /* T20.1: Control 0 */
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
        uint32  mh0;                    /* T20.2: Module Header word 0 */
        uint32  mh1;                    /* T20.3: Module Header word 1 */
        uint32  mh2;                    /* T20.4: Module Header word 2 */
        uint32  mh3;                    /* T20.5: Module Header word 3 */
#ifdef  LE_HOST
                                        /* T20.6: RX Status 0 */
        uint32  timestamp_type:2,       /* TIMESTAMP field type indicator */
                em_mtp_index:5,         /* EM Mirror-to-Port Index */
                :1,                     /* Reserved */
                cpu_cos:6,              /* COS queue for CPU packets */
                :2,                     /* Reserved */
                inner_vid:12,           /* Inner VLAN ID */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_pri:3;            /* Inner priority */

                                        /* T20.7 */ 
        uint32  reason_hi:16,           /* CPU opcode (high bits) */
                pkt_len:14,             /* Packet Length After Modification */
                :2;                     /* Reserved */

                                        /* T20.8 */
        uint32  reason;                 /* CPU opcode */

                                        /* T20.9 */
        uint32  dscp:8,                 /* New DSCP */
                chg_tos:1,              /* DSCP has been changed by HW */
                decap_tunnel_type:4,    /* Tunnel type that was decapsulated */
                regen_crc:1,            /* Packet modified and needs new CRC */
                :2,                     /* Reserved */
                outer_vid:12,           /* Outer VLAN ID */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_pri:3;            /* Outer priority */

                                        /* T20.10 */
        uint32  timestamp;              /* Timestamp */

                                        /* T20.11 */
        uint32  cos:4,                  /* COS */
                higig_cos:5,            /* Higig COS */
                vlan_cos:5,             /* VLAN COS */
                shaping_cos_sel:2,      /* Shaping COS Select */
                vfi:12,                 /* Internal VFI or FID value */
                vfi_valid:1,            /* VFI or FID */
                :3;                     /* Reserved */

                                        /* T20.12 */
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
                hg_lookup:1,            /* Reserved for future use */
                :4;                     /* Reserved */

                                        /* T20.13 */
        uint32  nhop_index:15,          /* L3 IP Multicast Interface Number or Next HOP Index */
                :1,                     /* Reserved */
                match_rule:8,           /* Matched FP rule */
                im_mtp_index:5,         /* IM Mirror-to-Port Index */
                ecn:2,                  /* Reserved */
                change_ecn:1;           /* Reserved */

                                        /* T20.14 */
        uint32  timestamp_upper;        /* OAM delay measurement: Upper 32-bit of 64-bit timestamp */

                                        /* T20.15: DMA Status 0 */
        uint32  count:16,               /* Transferred byte count */
                end:1,                  /* End bit (RX) */
                start:1,                /* Start bit (RX) */
                error:1,                /* Cell Error (RX) */
                dc:12,                  /* Don't Care */
                done:1;                 /* Descriptor Done */
#else
                                        /* T20.6: RX Status 0 */
        uint32  inner_pri:3,            /* Inner priority */
                inner_cfi:1,            /* Inner Canoncial Format Indicator */
                inner_vid:12,           /* Inner VLAN ID */
                :2,                     /* Reserved */
                cpu_cos:6,              /* COS queue for CPU packets */
                :1,                     /* Reserved */
                em_mtp_index:5,         /* EM Mirror-to-Port Index */
                timestamp_type:2;       /* TIMESTAMP field type indicator */

                                        /* T20.7 */
        uint32  :2,                     /* Reserved */
                pkt_len:14,             /* Packet Length After Modification */
                reason_hi:16;           /* CPU opcode (high bits) */

                                        /* T20.8 */
        uint32  reason;                 /* CPU opcode */

                                        /* T20.9 */
        uint32  outer_pri:3,            /* Outer priority */
                outer_cfi:1,            /* Outer Canoncial Format Indicator */
                outer_vid:12,           /* Outer VLAN ID */
                :2,                     /* Reserved */
                regen_crc:1,            /* Packet modified and needs new CRC */
                decap_tunnel_type:4,    /* Tunnel type that was decapsulated */
                chg_tos:1,              /* DSCP has been changed by HW */
                dscp:8;                 /* New DSCP */

                                        /* T20.10 */
        uint32  timestamp;              /* Timestamp */

                                        /* T20.11 */
        uint32  :3,                     /* Reserved */
                vfi_valid:1,            /* VFI or FID */
                vfi:12,                 /* Internal VFI or FID value */
                shaping_cos_sel:2,      /* Shaping COS Select */
                vlan_cos:5,             /* VLAN COS */
                higig_cos:5,            /* Higig COS */
                cos:4;                  /* COS */

                                        /* T20.12 */
        uint32  :4,                     /* Reserved */
                hg_lookup:1,            /* Reserved for future use */
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

                                        /* T20.13 */
        uint32  change_ecn:1,           /* Reserved */
                ecn:2,                  /* Reserved */
                im_mtp_index:5,         /* IM Mirror-to-Port Index */
                match_rule:8,           /* Matched FP rule */
                :1,                     /* Reserved */
                nhop_index:15;          /* L3 IP Multicast Interface Number or Next HOP Index */

                                        /* T20.14 */
        uint32  timestamp_upper;        /* OAM delay measurement: Upper 32-bit of 64-bit timestamp */

                                        /* T20.15: DMA Status 0 */
        uint32  done:1,                 /* Descriptor Done */
                dc:12,                  /* Don't Care */
                error:1,                /* Cell Error (RX) */
                start:1,                /* Start bit (RX) */
                end:1,                  /* End bit (RX) */
                count:16;               /* Transferred byte count */
#endif
} dcb20_t;
#endif  /* BCM_ENDURO_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_GREYHOUND_SUPPORT)
/*
 * DMA Control Block - Type 23
 * Used on 56640 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type23.h>

#endif  /* BCM_TRIUMPH3_SUPPORT || BCM_KATANA_SUPPORT || \
           BCM_TRIDENT2_SUPPORT || BCM_GREYHOUND_SUPPORT */

#if defined(BCM_KATANA_SUPPORT)
/*
 * DMA Control Block - Type 24
 * Used on 56440 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type24.h>

#endif  /* BCM_KATANA_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
/*
 * DMA Control Block - Type 26
 * Used on 56850 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type26.h>

#endif  /* BCM_TRIDENT2_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
/*
 * DMA Control Block - Type 29
 * Used on 56450 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type29.h>

#endif  /* BCM_KATANA2_SUPPORT */

#if defined(BCM_GREYHOUND_SUPPORT)
/*
 * DMA Control Block - Type 31
 * Used on 53400 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type31.h>

#endif  /* BCM_GREYHOUND_SUPPORT */

#if defined(BCM_TOMAHAWK_SUPPORT) || defined(BCM_TOMAHAWK2_SUPPORT)
/*
 * DMA Control Block - Type 32
 * Used on 5696X devices
 * 16 words
 */
#include <soc/shared/dcbformats/type32.h>

#endif  /* BCM_TOMAHAWK_SUPPORT */

#if defined(BCM_TRIDENT2PLUS_SUPPORT)
/*
 * DMA Control Block - Type 33
 * Used on 56860 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type33.h>

#endif  /* BCM_TRIDENT2PLUS_SUPPORT */

#if defined(BCM_HURRICANE3_SUPPORT)
/*
 * DMA Control Block - Type 34
 * Used on 56160 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type34.h>

#endif  /* BCM_HURRICANE3_SUPPORT */

#if defined(BCM_APACHE_SUPPORT)
/*
 * DMA Control Block - Type 35
 * Used on 56560 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type35.h>

#endif  /* BCM_APACHE_SUPPORT */

#if defined(BCM_TRIDENT3_SUPPORT)
/*
 * DMA Control Block - Type 36
 * Used on 56870 devices
 * 16 words
 */
#include <soc/shared/dcbformats/type36.h>

#endif  /* BCM_TRIDENT3_SUPPORT */

#if defined(BCM_GREYHOUND2_SUPPORT)
/*
 * DMA Control Block - Type 37
 * Used on XXXXX devices
 * 16 words
 */
#include <soc/shared/dcbformats/type37.h>

#endif  /* BCM_GREYHOUND2_SUPPORT */

#if defined(BCM_TOMAHAWK3_SUPPORT)
/*
 * DMA Control Block - Type 38
 * Used on XXXXX devices
 * 16 words
 */
#include <soc/shared/dcbformats/type38.h>

#endif  /* BCM_TOMAHAWK3_SUPPORT */


#endif  /* _SOC_DCBFORMATS_H */
