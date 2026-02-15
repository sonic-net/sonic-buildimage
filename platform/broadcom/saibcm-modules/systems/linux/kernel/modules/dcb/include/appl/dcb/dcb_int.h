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
 * File:        dcb_int.h
 * Purpose:     DCB Library Internal
 */

#ifndef   _DCB_INT_H_
#define   _DCB_INT_H_

#ifdef DCB_CUSTOM_CONFIG
/* Allow application to override soc_cm_print, etc. */
#include <dcb_custom_config.h>
#else
#ifdef __KERNEL__
#include <lkm.h>
#else
/* Needed for printf */
#include <stdio.h>
#endif
#endif

#include <sal/types.h>
#include <soc/defs.h>

/* Include first <soc/dcb.h> to redefine macros */
#include <soc/dcb.h>     


/**********************
 * Stubs and Redefines
 */

/*
 * <soc/drv.h>
 */
typedef struct soc_control_s {
    dcb_op_t    *dcb_op;
} soc_control_t;
extern soc_control_t       soc_control;

#define SOC_CONTROL(_u)    (&soc_control)

/*
 * <soc/dcb.h>
 */
#ifdef SOC_DCB
#undef SOC_DCB
#endif
#define SOC_DCB(_h)                          DCB_OP(_h)

#ifdef SOC_DCB_IDX2PTR
#undef SOC_DCB_IDX2PTR
#endif
#define SOC_DCB_IDX2PTR(_u, _dcb, _i)        NULL

#ifdef SOC_DCB_PTR2IDX
#undef SOC_DCB_PTR2IDX
#endif
#define SOC_DCB_PTR2IDX(_u, _dcb1, _dcb2)    0

/*
 * <soc/dma.h>
 */
#define SOC_DMA_HG_GET(_flags)          0
#define SOC_DMA_PURGE_GET(_flags)       0
#define SOC_DMA_COS_GET(flags)          0
#define SOC_DMA_CRC_GET(flags)          0
#define SOC_DV_TX_ALIGN(dv, i)          0

typedef struct dv_s {
    int         dv_unit;
    int         dv_cnt;
    int         dv_vcnt;
    int         dv_flags;
#   define      DV_F_NOTIFY_DSC         0x01
#   define      DV_F_NOTIFY_CHN         0x02
#   define      DV_F_COMBINE_DCB        0x04
#   define      DV_F_NEEDS_REFILL       0x10
    dcb_t       *dv_dcb;
} dv_t;

/*
 * <soc/cm.h>
 */
#ifdef __KERNEL__
#define printf printk
#endif

#ifndef soc_cm_debug
#define soc_cm_debug(flags, format, ...)  printf(format, __VA_ARGS__)
#endif
#ifndef soc_cm_print
#define soc_cm_print(format, ...)         printf(format, __VA_ARGS__)
#endif

#define soc_cm_l2p(unit, addr) ((uint32)(((sal_vaddr_t)(addr))&0xFFFFFFFF))
#define soc_cm_p2l(dev, addr)  (addr)

/*
 * <soc/enet.h>
 */
typedef int enet_hdr_t;

/*
 * <soc/util.h>
 */
extern uint32 _dcb_htonl(uint32 value);

#define soc_ntohl(_x)                                            \
    ((((uint8 *)&(_x))[0] << 24) | (((uint8 *)&(_x))[1] << 16) | \
     (((uint8 *)&(_x))[2] << 8)  | (((uint8 *)&(_x))[3]))
   
#define soc_htonl(_x)  _dcb_htonl(_x)

/*
 * <soc/feature.h>
 */
#define soc_feature(_u, _f)    0

/*
 * <soc/error.h>
 */
#define SOC_E_PARAM  -4
#define SOC_E_FULL   -6

/*
 * <assert.h>
 */
#define assert(_x)

/*
 * <shared/types.h>
 */
#define _SHR_L3_EGRESS_IDX_MIN   (0x186A0) /* Egress object start index */

#endif /* _DCB_INT_H_ */
