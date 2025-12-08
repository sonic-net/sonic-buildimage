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
 * File:        soc/shared/dcbformats/type38.h
 * Purpose:     Define dma control block (DCB) format for a type38 DCB
 *              used by the 56980 (Tomahawk 3)
 *
 *              This file is shared between the SDK and the embedded applications.
 */

#ifndef _SOC_SHARED_DCBFORMATS_TYPE38_H
#define _SOC_SHARED_DCBFORMATS_TYPE38_H

/*
 * DMA Control Block - Type 38
 * Used on 56980 devices
 * 16 words
 */
/* Tomahawk 3 CMICX DCB */
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
} dcb38_t;

/* EP_TO_CPU_HDR
 * - MODULE_HDR               [288:415] WORD9 - WORD12
 * - EP_TO_CPU_COMPOSITES_BUS [  0:287] WORD0 - WORD8
 */
typedef struct {
    uint32  mh0;                               /* WORD12 in EP_TO_CPU_HDR */
    uint32  mh1;                               /* WORD11 in EP_TO_CPU_HDR */
    uint32  mh2;                               /* WORD10 in EP_TO_CPU_HDR */
    uint32  mh3;                               /* WORD9  in EP_TO_CPU_HDR */

                                               /* WORD8  in EP_TO_CPU_HDR */
                                               /* WORD8  in EP_TO_CPU_COMPOSITES_BUS */
#ifdef LE_HOST
    uint32 timestamp_hi:16,                    /* TIMESTAMP_HI             [256:271] 16 */
           timestamp_type:2,                   /* TIMESTAMP_TYPE           [272:273] 2 */
           decap_tunnel_type:5,                /* TUNNEL_DECAP_TYPE        [274:278] 5 */
           uc_switch_drop:1,                   /* UC_SW_COPY_DROPPED       [279:279] 1 */
           ucq:1,                              /* UNICAST_QUEUE            [280:280] 1 */
           :7;                                 /* RESERVED                 [281:287] 7 */
#else
    uint32 :7,                                 /* RESERVED                 [281:287] 7 */
           ucq:1,                              /* UNICAST_QUEUE            [280:280] 1 */
           uc_switch_drop:1,                   /* UC_SW_COPY_DROPPED       [279:279] 1 */
           decap_tunnel_type:5,                /* TUNNEL_DECAP_TYPE        [274:278] 5 */
           timestamp_type:2,                   /* TIMESTAMP_TYPE           [272:273] 2 */
           timestamp_hi:16;                    /* TIMESTAMP_HI             [256:271] 16 */
#endif

                                               /* WORD7  in EP_TO_CPU_HDR */
                                               /* WORD7  in EP_TO_CPU_COMPOSITES_BUS */
    uint32 timestamp;                          /* TIMESTAMP                [224:255] 32 */

                                               /* WORD6  in EP_TO_CPU_HDR */
                                               /* WORD6  in EP_TO_CPU_COMPOSITES_BUS */
#ifdef LE_HOST
    uint32 :2,                                 /* RESERVED                 [192:193] 2 */
           entropy_label:20,                   /* ENTROPY LABEL            [194:213] 20 */
           special_pkt_indicator:1,            /* SPECIAL_PACKET_INDICATOR [214:214] 1 */
           srcport:8,                          /* SRC_PORT_NUM             [215:222] 8 */
           switch_pkt:1;                       /* SWITCH                   [223:223] 1 */
#else
    uint32 switch_pkt:1,                       /* SWITCH                   [223:223] 1 */
           srcport:8,                          /* SRC_PORT_NUM             [215:222] 8 */
           special_pkt_indicator:1,            /* SPECIAL_PACKET_INDICATOR [214:214] 1 */
           entropy_label:20,                   /* ENTROPY LABEL            [194:213] 20 */
           :2;                                 /* RESERVED                 [192:193] 2 */
#endif

                                               /* WORD5  in EP_TO_CPU_HDR */
                                               /* WORD5  in EP_TO_CPU_COMPOSITES_BUS */
#ifdef LE_HOST
    uint32 pkt_len:14,                         /* PKT_LENGTH	           [160:173] 14 */
           regen_crc:1,                        /* REGEN_CRC	           [174:174] 1 */
           replication_or_nhi:17;              /* REPLICATION_OR_NHOP_INDEX[175:191] 17 */
#else
    uint32 replication_or_nhi:17,              /* REPLICATION_OR_NHOP_INDEX[175:191] 17 */
           regen_crc:1,                        /* REGEN_CRC	           [174:174] 1 */
           pkt_len:14;                         /* PKT_LENGTH	           [160:173] 14 */
#endif

                                               /* WORD4  in EP_TO_CPU_HDR */
                                               /* WORD4  in EP_TO_CPU_COMPOSITES_BUS */
#ifdef LE_HOST
    uint32 loopback_pkt_type:3,                /* LOOPBACK_PACKET_TYPE     [128:130] 3 */
           match_rule:8,                       /* MATCHED_RULE             [131:138] 8 */
           mtp_index:5,                        /* MTP_INDEX                [139:143] 5 */
           outer_cfi:1,                        /* OUTER_CFI                [144:144] 1 */
           outer_pri:3,                        /* OUTER_PRI                [145:147] 3 */
           outer_vid:12;                       /* OUTER_VID                [148:159] 12 */
#else
    uint32 outer_vid:12,                       /* OUTER_VID                [148:159] 12 */
           outer_pri:3,                        /* OUTER_PRI                [145:147] 3 */
           outer_cfi:1,                        /* OUTER_CFI                [144:144] 1 */
           mtp_index:5,                        /* MTP_INDEX                [139:143] 5 */
           match_rule:8,                       /* MATCHED_RULE             [131:138] 8 */
           loopback_pkt_type:3;                /* LOOPBACK_PACKET_TYPE     [128:130] 3 */
#endif

                                               /* WORD3  in EP_TO_CPU_HDR */
                                               /* WORD3  in EP_TO_CPU_COMPOSITES_BUS */
    union {
#ifdef LE_HOST
        struct {
            uint32 cpu_opcode_type:4,          /* CPU_OPCODE_TYPE          [ 96: 99] 4 */
                   do_not_change_ttl:1,        /* DO_NOT_CHANGE_TTL	   [100:100] 1 */
                   dscp:6,                     /* DSCP                     [101:106] 6 */
                   ecn:2,                      /* ECN                      [107:108] 2 */
                   tag_status:2,               /* INCOMING_TAG_STATUS      [109:110] 2 */
                   ing_l3_intf:13,             /* ING_L3_INTF              [111:123] 13 */
                   otag_action:2,              /* ING_OTAG_ACTION          [124:125] 2 */
                   ip_routed:1,                /* IP_ROUTED                [126:126] 1 */
                   l3only:1;                   /* L3ONLY                   [127:127] 1 */
        } overlay1;
        struct {
            uint32 cpu_opcode_type:4,          /* CPU_OPCODE_TYPE          [ 96: 99] 4 */
                   do_not_change_ttl:1,        /* DO_NOT_CHANGE_TTL	   [100:100] 1 */
                   special_pkt_type:3,         /* SPECIAL_PACKET_TYPE      [101:103] 3 */
                   :3,                         /* RESERVED                 [104:106] 3 */
                   ecn:2,                      /* ECN                      [107:108] 2 */
                   tag_status:2,               /* INCOMING_TAG_STATUS      [109:110] 2 */
                   ing_l3_intf:13,             /* ING_L3_INTF              [111:123] 13 */
                   otag_action:2,              /* ING_OTAG_ACTION          [124:125] 2 */
                   ip_routed:1,                /* IP_ROUTED                [126:126] 1 */
                   l3only:1;                   /* L3ONLY                   [127:127] 1 */
        } overlay2;
#else
        struct {
            uint32 l3only:1,                   /* L3ONLY                   [127:127] 1 */
                   ip_routed:1,                /* IP_ROUTED                [126:126] 1 */
                   otag_action:2,              /* ING_OTAG_ACTION          [124:125] 2 */
                   ing_l3_intf:13,             /* ING_L3_INTF              [111:123] 13 */
                   tag_status:2,               /* INCOMING_TAG_STATUS      [109:110] 2 */
                   ecn:2,                      /* ECN                      [107:108] 2 */
                   dscp:6,                     /* DSCP                     [101:106] 6 */
                   do_not_change_ttl:1,        /* DO_NOT_CHANGE_TTL	   [100:100] 1 */
                   cpu_opcode_type:4;          /* CPU_OPCODE_TYPE          [ 96: 99] 4 */
        } overlay1;
        struct {
            uint32 l3only:1,                   /* L3ONLY                   [127:127] 1 */
                   ip_routed:1,                /* IP_ROUTED                [126:126] 1 */
                   otag_action:2,              /* ING_OTAG_ACTION          [124:125] 2 */
                   ing_l3_intf:13,             /* ING_L3_INTF              [111:123] 13 */
                   tag_status:2,               /* INCOMING_TAG_STATUS      [109:110] 2 */
                   ecn:2,                      /* ECN                      [107:108] 2 */
                   :3,                         /* RESERVED                 [104:106] 3 */
                   special_pkt_type:3,         /* SPECIAL_PACKET_TYPE      [101:103] 3 */
                   do_not_change_ttl:1,        /* DO_NOT_CHANGE_TTL	   [100:100] 1 */
                   cpu_opcode_type:4;          /* CPU_OPCODE_TYPE          [ 96: 99] 4 */
        } overlay2;
#endif
    } word3;
                                               /* WORD2  in EP_TO_CPU_HDR */
                                               /* WORD2  in EP_TO_CPU_COMPOSITES_BUS */
    uint32 reason_hi;                          /* CPU_OPCODE_HI            [ 64: 95] 32 */

                                               /* WORD1  in EP_TO_CPU_HDR */
                                               /* WORD1  in EP_TO_CPU_COMPOSITES_BUS */
    uint32 reason;                             /* CPU_OPCODE_LO            [ 32: 63] 32 */

    union {
                                               /* WORD0  in EP_TO_CPU_HDR */
                                               /* WORD0  in EP_TO_CPU_COMPOSITES_BUS */
#ifdef LE_HOST
        struct {
            uint32 rx_bfd_session_index:12,    /* RX_BFD_SESSION_INDEX     [  0: 11] 12 */
                   rx_bfd_start_offset_type:2, /* RX_BFD_START_OFFSET_TYPE [ 12: 13] 2 */
                   rx_bfd_start_offset:8,      /* RX_BFD_START_OFFSET      [ 14: 21] 8 */
                   queue_num:6,                /* MMU_COS                  [ 22: 27] 6 */
                   bpdu:1,                     /* BPDU                     [ 28: 28] 1 */
                   chg_dscp:1,                 /* CHANGE_DSCP              [ 29: 29] 1 */
                   chg_ecn:1,                  /* CHANGE_ECN               [ 30: 30] 1 */
                   :1;                         /* RESERVED                 [ 31: 31] 1 */
        } overlay1;
        struct {
            uint32 i2e_classid:12,             /* I2E_CLASSID              [  0: 11] 12 */
                   i2e_classid_type:4,         /* I2E_CLASSID_TYPE         [ 12: 15] 4 */
                   :6,                         /* RESERVED2                [ 16: 21] 6 */
                   cpu_cos:6,                  /* CPU_COS                  [ 22: 27] 6 */
                   bpdu:1,                     /* BPDU                     [ 28: 28] 1 */
                   chg_dscp:1,                 /* CHANGE_DSCP              [ 29: 29] 1 */
                   chg_ecn:1,                  /* CHANGE_ECN               [ 30: 30] 1 */
                   :1;                         /* RESERVED                 [ 31: 31] 1 */
        } overlay2;
#else
        struct {
            uint32 :1,                         /* RESERVED                 [ 31: 31] 1 */
                   chg_ecn:1,                  /* CHANGE_ECN               [ 30: 30] 1 */
                   chg_dscp:1,                 /* CHANGE_DSCP              [ 29: 29] 1 */
                   bpdu:1,                     /* BPDU                     [ 28: 28] 1 */
                   queue_num:6,                /* MMU_COS                  [ 22: 27] 6 */
                   rx_bfd_start_offset:8,      /* RX_BFD_START_OFFSET      [ 14: 21] 8 */
                   rx_bfd_start_offset_type:2, /* RX_BFD_START_OFFSET_TYPE [ 12: 13] 2 */
                   rx_bfd_session_index:12;    /* RX_BFD_SESSION_INDEX     [  0: 11] 12 */
        } overlay1;
        struct {
            uint32 :1,                         /* RESERVED                 [ 31: 31] 1 */
                   chg_ecn:1,                  /* CHANGE_ECN               [ 30: 30] 1 */
                   chg_dscp:1,                 /* CHANGE_DSCP              [ 29: 29] 1 */
                   bpdu:1,                     /* BPDU                     [ 28: 28] 1 */
                   cpu_cos:6,                  /* CPU_COS                  [ 22: 27] 6 */
                   :6,                         /* RESERVED2                [ 16: 21] 6 */
                   i2e_classid_type:4,         /* I2E_CLASSID_TYPE         [ 12: 15] 4 */
                   i2e_classid:12;             /* I2E_CLASSID              [  0: 11] 12 */
        } overlay2;
#endif
    } word0;
    uint32 rsvd;                  /* 4 byte padding to make it 56 bytes */
} ep_to_cpu_hdr38_t;

#define SOC_CPU_OPCODE_TYPE_IP_0        0
#define SOC_CPU_OPCODE_TYPE_IP_1        1
#define SOC_CPU_OPCODE_TYPE_EP          2
#define SOC_CPU_OPCODE_TYPE_NLF         3

#endif /* _SOC_SHARED_DCBFORMATS_TYPE38_H */
