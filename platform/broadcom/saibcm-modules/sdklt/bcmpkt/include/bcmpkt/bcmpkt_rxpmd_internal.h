/*! \file bcmpkt_rxpmd_internal.h
 *
 * RX Packet MetaData internal library.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef BCMPKT_RXPMD_INTERNAL_H
#define BCMPKT_RXPMD_INTERNAL_H

#include <shr/shr_types.h>
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_pmd.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>

/*!
 * Array of RXPMD field getter functions for a particular device
 * type.
 */
typedef struct bcmpkt_rxpmd_fget_s {
    bcmpkt_field_get_f fget[BCMPKT_RXPMD_FID_COUNT];
} bcmpkt_rxpmd_fget_t;

/*!
 * Array of RXPMD field setter functions for a particular device
 * type. These functions are used for internally configuring packet
 * filter.
 */
typedef struct bcmpkt_rxpmd_fset_s {
    bcmpkt_field_set_f fset[BCMPKT_RXPMD_FID_COUNT];
} bcmpkt_rxpmd_fset_t;

/*!
 * Array of RXPMD field address and length getter functions for a multiple
 * words field of a particular device type. *addr is output address and return
 * length.
 */
typedef struct bcmpkt_rxpmd_figet_s {
    bcmpkt_ifield_get_f fget[BCMPKT_RXPMD_I_FID_COUNT];
} bcmpkt_rxpmd_figet_t;

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_rx_reason_decode(const uint32_t* data, bcmpkt_rx_reasons_t* reasons);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_rx_reason_encode(const bcmpkt_rx_reasons_t* reasons, uint32_t* data);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_rxpmd_view_info_get(bcmpkt_pmd_view_info_t *info);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_ep_rx_reason_decode(const uint32_t* data, bcmpkt_rx_reasons_t* reasons);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_ep_rx_reason_encode(const bcmpkt_rx_reasons_t* reasons, uint32_t* data);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#endif /* BCMPKT_RXPMD_INTERNAL_H */
