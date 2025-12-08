/*
 * $Id: higig.h,v 1.29 Broadcom SDK $
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
 * File:        higig.h
 */

#include <soc/defs.h>
#include <soc/enet.h>

#ifndef _SOC_HIGIG_H
#define _SOC_HIGIG_H

#if defined(BCM_XGS_SUPPORT) || defined(BCM_PETRA_SUPPORT) || defined(BCM_DNX_SUPPORT)

/****************************************************************************
 * Higig Header Format
 *
 * On BCM5670/BCM5675 (Hercules/Hercules1.5), to send or receive packets
 * on an HG port, the following HIGIG header is always prepended to
 * an UNTAGGED ethernet frame.
 *
 * ANSI only permits signed or unsigned int for bit field type.  This
 * structure will only work for compilers for which uint32 is unsigned
 * int, and which completely pack bit fields beginning at the MSbit for
 * big-endian machines and at the LSbit for little-endian machines.
 *
 * NOTE: these structures already put the the individual bytes in memory
 * in big endian order for both big- and little-endian machines, so no
 * further swapping is required.
 ***************************************************************************/

typedef union soc_higig_hdr_u {
    struct {
        uint8   bytes[12];
    } overlay0;

#if defined(LE_HOST)

    struct {                            /* Byte # */
        uint32  start:8;                /* 0 */
        uint32  hgi:8;                  /* 1 */
        uint32  vlan_id_hi:4;           /* 2 */
        uint32  vlan_cfi:1;
        uint32  vlan_pri:3;
        uint32  vlan_id_lo:8;           /* 3 */
        uint32  opcode:3;               /* 4 */
        uint32  src_mod:5;
        uint32  src_port:6;             /* 5 */
        uint32  pfm:2;
        uint32  cos:3;                  /* 6 */
        uint32  dst_port:5;
        uint32  dst_mod:5;              /* 7 */
        uint32  cng:1;
        uint32  hdr_format:2;
        uint32  mirror:1;               /* 8 */
        uint32  mirror_done:1;
        uint32  mirror_only:1;
        uint32  ingress_tagged:1;
        uint32  dst_tgid:3;
        uint32  dst_t:1;
        uint32  _rsvd1:8;               /* 9 */
        uint32  _rsvd2:8;               /* 10 */
        uint32  _rsvd3:8;               /* 11 */
    } overlay1;

    struct {                            /* Byte # */
        uint32  _rsvd1:8;               /* 0 */
        uint32  _rsvd2:8;               /* 1 */
        uint32  _rsvd3:8;               /* 2 */
        uint32  _rsvd4:8;               /* 3 */
        uint32  _rsvd5:8;               /* 4 */
        uint32  tgid:6;                 /* 5 */
        uint32  _rsvd6:2;
        uint32  _rsvd7:3;               /* 6 */
        uint32  l2mc_ptr_lo:5;
        uint32  l2mc_ptr_hi:5;          /* 7 */
        uint32  _rsvd8:3;
        uint32  ctag_hi:8;              /* 8 */
        uint32  ctag_lo:8;              /* 9 */
        uint32  _rsvd9:8;               /* 10 */
        uint32  _rsvd10:8;              /* 11 */
    } overlay2;

    struct {                            /* Byte # */
        uint32  start:8;                /* 0 */
        uint32  dst_mod_6:1;            /* 1 */
        uint32  src_mod_6:1;
        uint32  hdr_ext_len:3;
        uint32  cng_hi:1;
        uint32  hgi:2;
        uint32  vlan_id_hi:4;           /* 2 */
        uint32  vlan_cfi:1;
        uint32  vlan_pri:3;
        uint32  vlan_id_lo:8;           /* 3 */
        uint32  opcode:3;               /* 4 */
        uint32  src_mod:5;
        uint32  src_port:6;             /* 5 */
        uint32  pfm:2;
        uint32  cos:3;                  /* 6 */
        uint32  dst_port:5;
        uint32  dst_mod:5;              /* 7 */
        uint32  cng:1;
        uint32  hdr_format:2;
        uint32  mirror:1;               /* 8 */
        uint32  mirror_done:1;
        uint32  mirror_only:1;
        uint32  ingress_tagged:1;
        uint32  dst_tgid:3;
        uint32  dst_t:1;
        uint32  vc_label_19_16:4;       /* 9 */
        uint32  label_present:1;
        uint32  l3:1;
        uint32  dst_mod_5:1;
        uint32  src_mod_5:1;
        uint32  vc_label_15_8:8;        /* 10 */
        uint32  vc_label_7_0:8;         /* 11 */
    } hgp_overlay1;

    struct {                            /* Byte # */
        uint32  start:8;                /* 0 */
        uint32  dst_mod_6:1;            /* 1 */
        uint32  src_mod_6:1;
        uint32  hdr_ext_len:3;
        uint32  cng_hi:1;
        uint32  hgi:2;
        uint32  vlan_id_hi:4;           /* 2 */
        uint32  vlan_cfi:1;
        uint32  vlan_pri:3;
        uint32  vlan_id_lo:8;           /* 3 */
        uint32  opcode:3;               /* 4 */
        uint32  src_mod:5;
        uint32  src_port:6;             /* 5 */
        uint32  pfm:2;
        uint32  cos:3;                  /* 6 */
        uint32  dst_port:5;
        uint32  dst_mod:5;              /* 7 */
        uint32  cng:1;
        uint32  hdr_format:2;
        uint32  mirror:1;               /* 8 */
        uint32  mirror_done:1;
        uint32  mirror_only:1;
        uint32  ingress_tagged:1;
        uint32  lag_failover:1;
        uint32  donot_learn:1;
        uint32  donot_modify:1;
        uint32  dst_t:1;
        uint32  vc_label_19_16:4;       /* 9 */
        uint32  label_present:1;
        uint32  l3:1;
        uint32  dst_mod_5:1;
        uint32  src_mod_5:1;
        uint32  vc_label_15_8:8;        /* 10 */
        uint32  vc_label_7_0:8;         /* 11 */
    } hgp_overlay2;

#else /* !LE_HOST */

    struct {                            /* Byte # */
        uint32  start:8;                /* 0 */
        uint32  hgi:8;                  /* 1 */
        uint32  vlan_pri:3;             /* 2 */
        uint32  vlan_cfi:1;
        uint32  vlan_id_hi:4;
        uint32  vlan_id_lo:8;           /* 3 */
        uint32  src_mod:5;              /* 4 */
        uint32  opcode:3;
        uint32  pfm:2;                  /* 5 */
        uint32  src_port:6;
        uint32  dst_port:5;             /* 6 */
        uint32  cos:3;
        uint32  hdr_format:2;           /* 7 */
        uint32  cng:1;
        uint32  dst_mod:5;
        uint32  dst_t:1;                /* 8 */
        uint32  dst_tgid:3;
        uint32  ingress_tagged:1;
        uint32  mirror_only:1;
        uint32  mirror_done:1;
        uint32  mirror:1;
        uint32  _rsvd1:8;               /* 9 */
        uint32  _rsvd2:8;               /* 10 */
        uint32  _rsvd3:8;               /* 11 */
    } overlay1;

    struct {                            /* Byte # */
        uint32  _rsvd1:8;               /* 0 */
        uint32  _rsvd2:8;               /* 1 */
        uint32  _rsvd3:8;               /* 2 */
        uint32  _rsvd4:8;               /* 3 */
        uint32  _rsvd5:8;               /* 4 */
        uint32  _rsvd6:2;               /* 5 */
        uint32  tgid:6;
        uint32  l2mc_ptr_lo:5;          /* 6 */
        uint32  _rsvd7:3;
        uint32  _rsvd8:3;               /* 7 */
        uint32  l2mc_ptr_hi:5;
        uint32  ctag_hi:8;              /* 8 */
        uint32  ctag_lo:8;              /* 9 */
        uint32  _rsvd9:8;               /* 10 */
        uint32  _rsvd10:8;              /* 11 */
    } overlay2;

    struct {                            /* Byte # */
        uint32  start:8;                /* 0 */
        uint32  hgi:2;                  /* 1 */
        uint32  cng_hi:1;
        uint32  hdr_ext_len:3;
        uint32  src_mod_6:1;
        uint32  dst_mod_6:1;
        uint32  vlan_pri:3;             /* 2 */
        uint32  vlan_cfi:1;
        uint32  vlan_id_hi:4;
        uint32  vlan_id_lo:8;           /* 3 */
        uint32  src_mod:5;              /* 4 */
        uint32  opcode:3;
        uint32  pfm:2;                  /* 5 */
        uint32  src_port:6;
        uint32  dst_port:5;             /* 6 */
        uint32  cos:3;
        uint32  hdr_format:2;           /* 7 */
        uint32  cng:1;
        uint32  dst_mod:5;
        uint32  dst_t:1;                /* 8 */
        uint32  dst_tgid:3;
        uint32  ingress_tagged:1;
        uint32  mirror_only:1;
        uint32  mirror_done:1;
        uint32  mirror:1;
        uint32  src_mod_5:1;            /* 9 */
        uint32  dst_mod_5:1;
        uint32  l3:1;
        uint32  label_present:1;
        uint32  vc_label_19_16:4;
        uint32  vc_label_15_8:8;        /* 10 */
        uint32  vc_label_7_0:8; /* 11 */
    } hgp_overlay1;

    struct {                            /* Byte # */
        uint32  start:8;                /* 0 */
        uint32  hgi:2;                  /* 1 */
        uint32  cng_hi:1;
        uint32  hdr_ext_len:3;
        uint32  src_mod_6:1;
        uint32  dst_mod_6:1;
        uint32  vlan_pri:3;             /* 2 */
        uint32  vlan_cfi:1;
        uint32  vlan_id_hi:4;
        uint32  vlan_id_lo:8;           /* 3 */
        uint32  src_mod:5;              /* 4 */
        uint32  opcode:3;
        uint32  pfm:2;                  /* 5 */
        uint32  src_port:6;
        uint32  dst_port:5;             /* 6 */
        uint32  cos:3;
        uint32  hdr_format:2;           /* 7 */
        uint32  cng:1;
        uint32  dst_mod:5;
        uint32  dst_t:1;                /* 8 */
        uint32  donot_modify:1;
        uint32  donot_learn:1;
        uint32  lag_failover:1;
        uint32  ingress_tagged:1;
        uint32  mirror_only:1;
        uint32  mirror_done:1;
        uint32  mirror:1;
        uint32  src_mod_5:1;            /* 9 */
        uint32  dst_mod_5:1;
        uint32  l3:1;
        uint32  label_present:1;
        uint32  vc_label_19_16:4;
        uint32  vc_label_15_8:8;        /* 10 */
        uint32  vc_label_7_0:8; /* 11 */
    } hgp_overlay2;

#endif /* !LE_HOST */

} soc_higig_hdr_t;

#define SOC_HIGIG_HDR_SIZE      ((int) sizeof(soc_higig_hdr_t))

/*
 * Higig Header Field Manipulation
 */

typedef enum {
    /* NOTE: strings in soc_higig_field_names[] must match */
    HG_invalid = -1,
    HG_start = 0,
    HG_hgi,
    HG_vlan_tag,                /* aggregate of pri, cfi, and id */
    HG_vlan_pri,
    HG_vlan_cfi,
    HG_vlan_id,
    HG_src_mod,
    HG_opcode,
    HG_pfm,
    HG_src_port,
    HG_dst_port,
    HG_cos,
    HG_hdr_format,
    HG_cng,
    HG_dst_mod,
    HG_dst_t,
    HG_dst_tgid,
    HG_ingress_tagged,
    HG_mirror_only,
    HG_mirror_done,
    HG_mirror,
    HG_tgid,
    HG_l2mc_ptr,
    HG_ctag,
    HG_hdr_ext_len,     /* HG+ fields */
    HG_l3,
    HG_donot_modify,
    HG_donot_learn,
    HG_lag_failover,
    HG_label_present,
    HG_vc_label,
#if defined(BCM_HIGIG2_SUPPORT)
    HG_tc,              /* HG2 fields */
    HG_mcst,
    HG_mgid,
    HG_lbid,
    HG_dp,
    HG_ehv,
    HG_ppd_type,
    HG_src_t,
    HG_multipoint,
    HG_fwd_type,
    HG_dst_vp,
    HG_src_vp,
    HG_vni,
    HG_data_container_type,
    HG_data_container,
    HG_preserve_dscp,
    HG_preserve_dot1p,
    HG_dst_type,
    HG_src_type,
    HG_deferred_drop,
    HG_vxlt_done,
    HG_deferred_change_pkt_pri,
    HG_new_pkt_pri,
    HG_deferred_change_dscp,
    HG_new_dscp,
    HG_label_overlay_type,
    HG_protection_status,
    HG_replication_id,
    HG_ether_type,
    HG_class_id,
    HG_cn,
    HG_sw_gen,
#endif /* BCM_HIGIG2_SUPPORT */
    HG_COUNT
} soc_higig_field_t;

#if defined(BCM_HIGIG2_SUPPORT)
#define SOC_HIGIG2_FIELD_NAMES_INIT                     \
    "tc",              /* HG2 fields */                 \
    "mcst",                                             \
    "mgid",                                             \
    "lbid",                                             \
    "dp",                                               \
    "ehv",                                              \
    "ppd_type",                                         \
    "src_t",                                            \
    "multipoint",                                       \
    "fwd_type",                                         \
    "dst_vp",                                           \
    "src_vp",                                           \
    "vni",                                              \
    "data_container_type",                              \
    "data_container",                                   \
    "preserve_dscp",                                    \
    "preserve_dot1p",                                   \
    "dst_type",                                         \
    "src_type",                                         \
    "deferred_drop",                                    \
    "vxlt_done",                                        \
    "deferred_change_pkt_pri",                          \
    "new_pkt_pri",                                      \
    "deferred_change_dscp",                             \
    "new_dscp",                                         \
    "label_overlay_type",                               \
    "protection_status",                                \
    "replication_id",                                   \
    "ether_type",                                       \
    "class_id",                                         \
    "cn",                                               \
    "sw_gen",
#else
#define SOC_HIGIG2_FIELD_NAMES_INIT
#endif /* BCM_HIGIG2_SUPPORT */

/* NOTE: strings must match soc_higig_field_t */
#define SOC_HIGIG_FIELD_NAMES_INIT                      \
    "start",                                            \
    "hgi",                                              \
    "vlan_tag",                                         \
    "vlan_pri",                                         \
    "vlan_cfi",                                         \
    "vlan_id",                                          \
    "src_mod",                                          \
    "opcode",                                           \
    "pfm",                                              \
    "src_port",                                         \
    "dst_port",                                         \
    "cos",                                              \
    "hdr_format",                                       \
    "cng",                                              \
    "dst_mod",                                          \
    "dst_t",                                            \
    "dst_tgid",                                         \
    "ingress_tagged",                                   \
    "mirror_only",                                      \
    "mirror_done",                                      \
    "mirror",                                           \
    "tgid",                                             \
    "l2mc_ptr",                                         \
    "ctag",                                             \
    "hdr_ext_len",     /* HG+ fields */                 \
    "l3",                                               \
    "donot_modify",                                     \
    "donot_learn",                                      \
    "lag_failover",                                     \
    "label_present",                                    \
    "vc_label",                                         \
    SOC_HIGIG2_FIELD_NAMES_INIT                         \
    NULL

#ifdef BCM_HIGIG2_SUPPORT

#define SOC_HIGIG2_START        0xfc /* Higig2 Module Header for CMIC TX */

typedef union soc_higig2_hdr_u {
    struct {
        uint8   bytes[16];
    } overlay0;

#if defined(LE_HOST)

    struct {                            /* Byte # */
        /* "Legacy" PPD Overlay 1 */
        uint32  start:8;                /* 0 */
        uint32  tc:4;                   /* 1 */
        uint32  mcst:1;
        uint32  _rsvd1:3;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  ppd_type:3;             /* 7 */
        uint32  _rsvd2:2;
        uint32  ehv:1;
        uint32  dp:2;
        uint32  mirror:1;               /* 8 */
        uint32  mirror_done:1;
        uint32  mirror_only:1;
        uint32  ingress_tagged:1;
        uint32  lag_failover:1;
        uint32  donot_learn:1;
        uint32  donot_modify:1;
        uint32  dst_t:1;
        uint32  vc_label_19_16:4;       /* 9 */
        uint32  label_present:1;
        uint32  l3:1;
        uint32  label_overlay_type:2;
        uint32  vc_label_15_8:8;        /* 10 */
        uint32  vc_label_7_0:8;         /* 11 */
        uint32  vlan_id_hi:4;           /* 12 */
        uint32  vlan_cfi:1;
        uint32  vlan_pri:3;
        uint32  vlan_id_lo:8;           /* 13 */
        uint32  opcode:3;               /* 14 */
        uint32  preserve_dot1p:1;
        uint32  preserve_dscp:1;
        uint32  src_t:1;
        uint32  pfm:2;
        uint32  _rsvd5:5;               /* 15 */
        uint32  hdr_ext_len:3;
   } ppd_overlay1;

    struct {                            /* Byte # */
        /* "Legacy" PPD Overlay 2 */
        uint32  start:8;                /* 0 */
        uint32  tc:4;                   /* 1 */
        uint32  mcst:1;
        uint32  _rsvd1:3;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  ppd_type:3;             /* 7 */
        uint32  _rsvd2:2;
        uint32  ehv:1;
        uint32  dp:2;
        uint32  ctag_hi:8;              /* 8 */
        uint32  ctag_lo:8;              /* 9 */
        uint32  _rsvd3:8;               /* 10 */
        uint32  _rsvd4:8;               /* 11 */
        uint32  vlan_id_hi:4;           /* 12 */
        uint32  vlan_cfi:1;
        uint32  vlan_pri:3;
        uint32  vlan_id_lo:8;           /* 13 */
        uint32  opcode:3;               /* 14 */
        uint32  _rsvd5:2;
        uint32  src_t:1;
        uint32  pfm:2;
        uint32  _rsvd6:5;               /* 15 */
        uint32  hdr_ext_len:3;
    } ppd_overlay2;

    struct {                            /* Byte # */
        /* "Next Gen" PPD Overlay 3 */
        uint32  start:8;                /* 0 */
        uint32  tc:4;                   /* 1 */
        uint32  mcst:1;
        uint32  _rsvd1:3;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  ppd_type:3;             /* 7 */
        uint32  _rsvd2:2;
        uint32  ehv:1;
        uint32  dp:2;
        uint32  vni_mid:2;              /* 8 */
        uint32  fwd_type:5;
        uint32  multipoint:1;
        uint32  vni_low:8;              /* 9 */
        uint32  dst_vp_high:8;          /* 10 */
        uint32  dst_vp_low:8;           /* 11 */
        uint32  src_vp_high:8;          /* 12 */
        uint32  src_vp_low:8;           /* 13 */
        uint32  opcode:3;               /* 14 */
        uint32  protection_status:1;
        uint32  lag_failover:1;
        uint32  donot_learn:1;
        uint32  donot_modify:1;
        uint32  mirror:1;
        uint32  source_type:1;          /* 15 */
        uint32  dest_type:1;
        uint32  preserve_dot1p:1;
        uint32  preserve_dscp:1;
        uint32  vni_high:4;
    } ppd_overlay3;

    struct {                            /* Byte # */
        /* "Offload Engine" PPD Overlay 4 */
        uint32  start:8;                /* 0 */
        uint32  tc:4;                   /* 1 */
        uint32  mcst:1;
        uint32  _rsvd1:3;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  ppd_type:3;             /* 7 */
        uint32  _rsvd2:2;
        uint32  ehv:1;
        uint32  dp:2;

        union {
            /* The data container has several sub-formats */
            struct {                    /* Byte # */
                /* Abstract data container */
                uint8  bytes[4];       /* 8-11 */
            } abstract;

            struct {                    /* Byte # */
                uint32  ctag_hi:8;      /* 8 */
                uint32  ctag_lo:8;      /* 9 */
                uint32  deferred_change_dscp:1; /* 10 */
                uint32  new_pkt_pri:3;
                uint32  deferred_change_pkt_pri:1;
                uint32  vxlt_done:2;
                uint32  deferred_drop:1;
                uint32  _rsvd1:2;       /* 11 */
                uint32  new_dscp:6;
            } offload_engine;
        } data_container;

        uint32  src_vp_high:8;          /* 12 */
        uint32  src_vp_low:8;           /* 13 */
        uint32  opcode:3;               /* 14 */
        uint32  source_type:1;
        uint32  _rsvd3:1;
        uint32  donot_learn:1;
        uint32  preserve_dot1p:1;
        uint32  preserve_dscp:1;
        uint32  data_container_type:4;  /* 15 */
        uint32  _rsvd4:4;
    } ppd_overlay4;

#else /* !LE_HOST */

    struct {                            /* Byte # */
        /* "Legacy" PPD Overlay 1 */
        uint32  start:8;                /* 0 */
        uint32  _rsvd1:3;               /* 1 */
        uint32  mcst:1;
        uint32  tc:4;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  dp:2;                   /* 7 */
        uint32  ehv:1;
        uint32  _rsvd2:2;
        uint32  ppd_type:3;
        uint32  dst_t:1;                /* 8 */
        uint32  donot_modify:1;
        uint32  donot_learn:1;
        uint32  lag_failover:1;
        uint32  ingress_tagged:1;
        uint32  mirror_only:1;
        uint32  mirror_done:1;
        uint32  mirror:1;
        uint32  label_overlay_type:2;               /* 9 */
        uint32  l3:1;
        uint32  label_present:1;
        uint32  vc_label_19_16:4;
        uint32  vc_label_15_8:8;        /* 10 */
        uint32  vc_label_7_0:8;         /* 11 */
        uint32  vlan_pri:3;             /* 12 */
        uint32  vlan_cfi:1;
        uint32  vlan_id_hi:4;
        uint32  vlan_id_lo:8;           /* 13 */
        uint32  pfm:2;                  /* 14 */
        uint32  src_t:1;
        uint32  preserve_dscp:1;
        uint32  preserve_dot1p:1;
        uint32  opcode:3;
        uint32  hdr_ext_len:3;          /* 15 */
        uint32  _rsvd5:5;
   } ppd_overlay1;

    struct {                            /* Byte # */
        /* "Legacy" PPD Overlay 2 */
        uint32  start:8;                /* 0 */
        uint32  _rsvd1:3;               /* 1 */
        uint32  mcst:1;
        uint32  tc:4;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  dp:2;                   /* 7 */
        uint32  ehv:1;
        uint32  _rsvd2:2;
        uint32  ppd_type:3;
        uint32  ctag_hi:8;              /* 8 */
        uint32  ctag_lo:8;              /* 9 */
        uint32  _rsvd3:8;               /* 10 */
        uint32  _rsvd4:8;               /* 11 */
        uint32  vlan_pri:3;             /* 12 */
        uint32  vlan_cfi:1;
        uint32  vlan_id_hi:4;
        uint32  vlan_id_lo:8;           /* 13 */
        uint32  pfm:2;                  /* 14 */
        uint32  src_t:1;
        uint32  _rsvd5:2;
        uint32  opcode:3;
        uint32  hdr_ext_len:3;          /* 15 */
        uint32  _rsvd6:5;
    } ppd_overlay2;

    struct {                            /* Byte # */
        /* "Next Gen" PPD Overlay 3 */
        uint32  start:8;                /* 0 */
        uint32  _rsvd1:3;               /* 1 */
        uint32  mcst:1;
        uint32  tc:4;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  dp:2;                   /* 7 */
        uint32  ehv:1;
        uint32  _rsvd2:2;
        uint32  ppd_type:3;
        uint32  multipoint:1;           /* 8 */
        uint32  fwd_type:5;
        uint32  vni_mid:2;
        uint32  vni_low:8;              /* 9 */
        uint32  dst_vp_high:8;          /* 10 */
        uint32  dst_vp_low:8;           /* 11 */
        uint32  src_vp_high:8;          /* 12 */
        uint32  src_vp_low:8;           /* 13 */
        uint32  mirror:1;               /* 14 */
        uint32  donot_modify:1;
        uint32  donot_learn:1;
        uint32  lag_failover:1;
        uint32  protection_status:1;
        uint32  opcode:3;
        uint32  vni_high:4;             /* 15 */
        uint32  preserve_dscp:1;
        uint32  preserve_dot1p:1;
        uint32  dest_type:1;
        uint32  source_type:1;
    } ppd_overlay3;

    struct {                            /* Byte # */
        /* "Offload Engine" PPD Overlay 4 */
        uint32  start:8;                /* 0 */
        uint32  _rsvd1:3;               /* 1 */
        uint32  mcst:1;
        uint32  tc:4;
        uint32  dst_mod:8;              /* 2 */
        uint32  dst_port:8;             /* 3 */
        uint32  src_mod:8;              /* 4 */
        uint32  src_port:8;             /* 5 */
        uint32  lbid:8;                 /* 6 */
        uint32  dp:2;                   /* 7 */
        uint32  ehv:1;
        uint32  _rsvd2:2;
        uint32  ppd_type:3;

        union {
            /* The data container has several sub-formats */
            struct {                    /* Byte # */
                /* Abstract data container */
                uint8  bytes[4];       /* 8-11 */
            } abstract;

            struct {                    /* Byte # */
                uint32  ctag_hi:8;      /* 8 */
                uint32  ctag_lo:8;      /* 9 */
                uint32  deferred_drop:1; /* 10 */
                uint32  vxlt_done:2;
                uint32  deferred_change_pkt_pri:1;
                uint32  new_pkt_pri:3;
                uint32  deferred_change_dscp:1;
                uint32  new_dscp:6;     /* 11 */
                uint32  _rsvd1:2;
            } offload_engine;
        } data_container;

        uint32  src_vp_high:8;          /* 12 */
        uint32  src_vp_low:8;           /* 13 */
        uint32  preserve_dscp:1;        /* 14 */
        uint32  preserve_dot1p:1;
        uint32  donot_learn:1;
        uint32  _rsvd3:1;
        uint32  source_type:1;
        uint32  opcode:3;
        uint32  _rsvd4:4;               /* 15 */
        uint32  data_container_type:4;
    } ppd_overlay4;
#endif /* !LE_HOST */

} soc_higig2_hdr_t;

#define SOC_HIGIG2_LABEL_OVERLAY_TYPE_VC_LABEL             0
#define SOC_HIGIG2_LABEL_OVERLAY_TYPE_SYSTEM_INGRESS_PORT  1
#define SOC_HIGIG2_LABEL_OVERLAY_TYPE_MIRROR_CLASS_TAG     2
#define SOC_HIGIG2_LABEL_OVERLAY_TYPE_QTAG                 3

#define SOC_HIGIG2_HDR_SIZE     ((int) sizeof(soc_higig2_hdr_t))

extern uint32 soc_higig2_field_get(int unit, soc_higig2_hdr_t *hg,
                                  soc_higig_field_t field);
extern void soc_higig2_field_set(int unit, soc_higig2_hdr_t *hg,
                                soc_higig_field_t field, uint32 val);
extern void soc_higig2_dump(int unit, char *pfx, soc_higig2_hdr_t *hg);


typedef union soc_higig3_hdr_u {
    struct {
        uint8   bytes[16];
    } overlay0;
#if defined(LE_HOST)
    struct {
        uint32  ethertype_high:8;
        uint32  ethertype_mirror_copy:1;
        uint32  ethertype_reserved:1;
        uint32  ethertype_exthdr:2;
        uint32  ethertype_ver:2;
        uint32  ethertype_low:2;
        uint32  system_dest_high:7;
        uint32  system_dest_type:1;
        uint32  system_dest_low:8;

        uint32  system_source_high:7;
        uint32  l3_routed:1;
        uint32  system_source_low:8;
        uint32  entropy:8;
        uint32  tc:4;
        uint32  cn:2;
        uint32  cng:2;

        uint32  class_id_high:2;
        uint32  flags:6;
        uint32  class_id_low:8;
        uint32  forwarding_domain_high:8;
        uint32  forwarding_domain_low:8;

        uint32  svp_high:8;
        uint32  svp_low:8;
        uint32  dvp_or_l3_iif_high:8;
        uint32  dvp_or_l3_iif_low:8;
    } hg3_overlay;
#else
    struct {
        /* Word 0 */
        uint32  ethertype_high:8;
        uint32  ethertype_low:2;
        uint32  ethertype_ver:2;
        uint32  ethertype_exthdr:2;
        uint32  ethertype_reserved:1;
        uint32  ethertype_mirror_copy:1;
        uint32  system_dest_type:1;
        uint32  system_dest_high:7;
        uint32  system_dest_low:8;

        /* Word 1  */
        uint32  l3_routed:1;
        uint32  system_source_high:7;
        uint32  system_source_low:8;
        uint32  entropy:8;
        uint32  cng:2;
        uint32  cn:2;
        uint32  tc:4;

        /* Extension header Type 0 */
        /* Word 2 */
        uint32  flags:6;
        uint32  class_id_high:2;
        uint32  class_id_low:8;
        uint32  forwarding_domain_high:8;
        uint32  forwarding_domain_low:8;

        /* Word 3 */
        uint32  svp_high:8;
        uint32  svp_low:8;
        uint32  dvp_or_l3_iif_high:8;
        uint32  dvp_or_l3_iif_low:8;
    } hg3_overlay;
#endif /* LE_HOST */
} soc_higig3_hdr_t;


#define SOC_HIGIG3_HDR_SIZE     ((int) sizeof(soc_higig3_hdr_t))
extern uint32 soc_higig3_field_get(int unit, soc_higig3_hdr_t *hg,
                                  soc_higig_field_t field);
extern void soc_higig3_field_set(int unit, soc_higig3_hdr_t *hg,
                                soc_higig_field_t field, uint32 val);
extern void soc_higig3_dump(int unit, char *pfx, soc_higig3_hdr_t *hg);

#endif /* BCM_HIGIG2_SUPPORT */

extern soc_higig_field_t soc_higig_name_to_field(int unit, char *name);
extern char *soc_higig_field_to_name(int unit, soc_higig_field_t f);

extern uint32 soc_higig_field_get(int unit, soc_higig_hdr_t *hg,
                                  soc_higig_field_t field);
extern void soc_higig_field_set(int unit, soc_higig_hdr_t *hg,
                                soc_higig_field_t field, uint32 val);

extern void soc_higig_dump(int unit, char *pfx, soc_higig_hdr_t *hg);

/* HIGIG Header field default values */

/* default SOP Symbol */
#define SOC_HIGIG_START         0xfb /* Default Start for BIGMAC */

/* HGI */
#define SOC_HIGIG_HGI           0x80 /* Default HGI */
#define SOC_HIGIG_HGI_MASK      0xc0 /* HGI field mask */

/* Module header Op-Codes */
#define SOC_HIGIG_OP_CPU        0x00 /* CPU Frame */
#define SOC_HIGIG_OP_UC         0x01 /* Unicast Frame */
#define SOC_HIGIG_OP_BC         0x02 /* Broadcast or DLF frame */
#define SOC_HIGIG_OP_MC         0x03 /* Multicast Frame */
#define SOC_HIGIG_OP_IPMC       0x04 /* IP Multicast Frame */

#define SOC_HIGIG_DST_MOD_CPU   0x00

/* Header Format */
#define SOC_HIGIG_HDR_DEFAULT   0x00
#define SOC_HIGIG_HDR_CLASS_TAG 0x01
#define SOC_HIGIG_HDR_EXT_LEN   0x02
#define SOC_HIGIG_HDR_RSVD2     0x03

#if defined(BCM_XGS_SUPPORT)

/* XGS Frame format */
typedef struct xgs_pkt_s {
    soc_higig_hdr_t     higig_hdr;
    enet_hdr_t          enet_hdr;
} xgs_pkt_t;

/* Hercules Header modes */
#define SOC_XGS_ENCAP_IEEE      0x00 /* Standard IEEE 802.3 (Default) */
#define SOC_XGS_ENCAP_HIGIG     0x01 /* Broadcom HIGIG Format */
#define SOC_XGS_ENCAP_ALLYR     0x02 /* BCM5632 compatible mode */
#define SOC_XGS_ENCAP_RSVP      0x03 /* Reserved */

/****************************************************************************
 * BCM5632 Header Format
 * When the MAC is in BCM5632 header mode, we prepend the following
 * two words of data to a standard 802.3 Ethernet Frame.
 *
 * ANSI only permits signed or unsigned int for bit field type.  This
 * structure will only work for compilers for which uint32 is unsigned
 * int, and which completely pack bit fields beginning at the MSbit for
 * big-endian machines and at the LSbit for little-endian machines.
 *
 * NOTE: these structures already put the the individual bytes in memory
 * in big endian order for both big- and little-endian machines, so no
 * further swapping is required.
 ****************************************************************************/

typedef union soc_bcm5632_hdr_u {
    struct {
        uint8   bytes[8];
    } overlay0;

#if defined(LE_HOST)

    struct {                            /* Byte # */
        uint32  d_portid:6;             /* 0 */
        uint32  _rsvd1:2;
        uint32  _rsvd2:8;               /* 1 */
        uint32  _rsvd3:8;               /* 2 */
        uint32  _rsvd4:8;               /* 3 */

        uint32  start:8;                /* 4 */
        uint32  length_hi:8;            /* 5 */
        uint32  length_lo:8;            /* 6 */
        uint32  s_portid:6;             /* 7 */
        uint32  _rsvd5:2;
    } overlay1;

#else /* !LE_HOST */

    struct {                            /* Byte # */
        uint32  _rsvd1:2;               /* 0 */
        uint32  d_portid:6;
        uint32  _rsvd2:8;               /* 1 */
        uint32  _rsvd3:8;               /* 2 */
        uint32  _rsvd4:8;               /* 3 */

        uint32  start:8;                /* 4 */
        uint32  length_hi:8;            /* 5 */
        uint32  length_lo:8;            /* 6 */
        uint32  _rsvd5:2;               /* 7 */
        uint32  s_portid:6;
    } overlay1;

#endif

} soc_bcm5632_hdr_t;

#define SOC_BCM5632_HDR_SIZE    ((int) sizeof(soc_bcm5632_hdr_t))

/* BCM5632 Frame format */
typedef struct b5632_pkt_s {
    soc_bcm5632_hdr_t   b5632_hdr;
    enet_hdr_t          enet_hdr;
} b5632_pkt_t;

/* BCM5632 Header field default values */

/* HGI */
#define SOC_BCM5632_HGI         0x00 /* Default HGI for BCM5632 */

#endif /* defined(BCM_XGS_SUPPORT) */


#define SOC_HIGIG_HDR_MAX_MODID           0xFF
#define SOC_HIGIG_E2ECC_MAX_PORT_STATUS   64

/* E2ECC Header */
typedef union soc_higig_e2ecc_hdr_u {
    struct {
        uint8   bytes[32];
    } overlay0;

    struct {
        uint32  words[8];
    } overlay1;

} soc_higig_e2ecc_hdr_t;


#endif /* defined(BCM_XGS_SUPPORT) || defined(BCM_PETRA_SUPPORT)*/

#endif  /* !_SOC_HIGIG_H */
