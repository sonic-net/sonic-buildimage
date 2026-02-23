/*! \file bcmpkt_lbhdr_internal.h
 *
 * Loopback header (LBHDR, called LOOPBACK_MH in hardware) access interface
 * (Internal use only).
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

#ifndef BCMPKT_LBHDR_INTERNAL_H
#define BCMPKT_LBHDR_INTERNAL_H

#include <shr/shr_types.h>
#include <bcmpkt/bcmpkt_pmd.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>
#include <bcmpkt/bcmpkt_lbhdr_defs.h>

/*!
 * Array of LBHDR field getter functions for a particular device
 * type.
 */
typedef struct bcmpkt_lbhdr_fget_s {
    bcmpkt_field_get_f fget[BCMPKT_LBHDR_FID_COUNT];
} bcmpkt_lbhdr_fget_t;

/*!
 * Array of LBHDR field setter functions for a particular device
 * type. These functions are used for internally configuring packet
 * filter.
 */
typedef struct bcmpkt_lbhdr_fset_s {
    bcmpkt_field_set_f fset[BCMPKT_LBHDR_FID_COUNT];
} bcmpkt_lbhdr_fset_t;

/*!
 * Array of LBHDR field address and length getter functions for a multiple
 * words field of a particular device type. *addr is output address and return
 * length.
 */
typedef struct bcmpkt_lbhdr_figet_s {
    bcmpkt_ifield_get_f fget[BCMPKT_LBHDR_I_FID_COUNT];
} bcmpkt_lbhdr_figet_t;

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_lbhdr_view_info_get(bcmpkt_pmd_view_info_t *info);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#endif /* BCMPKT_LBHDR_INTERNAL_H */
