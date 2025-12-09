/*! \file bcmpkt_lbhdr.c
 *
 * Loopback header (LBHDR, called LOOPBACK_MH in hardware) access interface.
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

#include <shr/shr_error.h>
#include <bcmpkt/bcmpkt_lbhdr.h>
#include <bcmpkt/bcmpkt_lbhdr_internal.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_lbhdr_fget_t _bd##_lbhdr_fget;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_lbhdr_fget,
static const bcmpkt_lbhdr_fget_t *lbhdr_fget[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_lbhdr_fset_t _bd##_lbhdr_fset;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_lbhdr_fset,
static const bcmpkt_lbhdr_fset_t *lbhdr_fset[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_lbhdr_figet_t _bd##_lbhdr_figet;
#include <bcmdrd/bcmdrd_devlist.h>

#if 0
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_lbhdr_figet,
static const bcmpkt_lbhdr_figet_t *lbhdr_figet[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};
#endif

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_lbhdr_view_info_get,
static void (*view_info_get[])(bcmpkt_pmd_view_info_t *info) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

static const shr_enum_map_t field_names[] =
{
    BCMPKT_LBHDR_FIELD_NAME_MAP_INIT
};

int
bcmpkt_lbhdr_field_get(bcmdrd_dev_type_t dev_type, uint32_t *lbhdr,
                       int fid, uint32_t *val)
{

    if ((lbhdr == NULL) || (val == NULL)) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_LBHDR_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (lbhdr_fget[dev_type] == NULL ||
        lbhdr_fget[dev_type]->fget[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *val = lbhdr_fget[dev_type]->fget[fid](lbhdr);

    return SHR_E_NONE;
}

int
bcmpkt_lbhdr_field_set(bcmdrd_dev_type_t dev_type, uint32_t *lbhdr,
                       int fid, uint32_t val)
{

    if (lbhdr == NULL) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_LBHDR_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (lbhdr_fset[dev_type] == NULL ||
        lbhdr_fset[dev_type]->fset[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    lbhdr_fset[dev_type]->fset[fid](lbhdr, val);

    return SHR_E_NONE;
}

int
bcmpkt_lbhdr_field_name_get(int fid, char **name)
{

    if (name == NULL) {
        return SHR_E_PARAM;
    }

    if (fid <= BCMPKT_LBHDR_FID_INVALID ||
        fid >= BCMPKT_LBHDR_FID_COUNT) {
        return SHR_E_PARAM;
    }

    *name = field_names[fid].name;

    return SHR_E_NONE;
}

int
bcmpkt_lbhdr_field_id_get(char* name, int *fid)
{
    int i;

    if ((name == NULL) || (fid == NULL)) {
        return SHR_E_PARAM;
    }

    for (i = BCMPKT_LBHDR_FID_INVALID + 1; i < BCMPKT_LBHDR_FID_COUNT; i++) {
        if (sal_strcasecmp(field_names[i].name, name) == 0) {
            *fid = field_names[i].val;
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}

int
bcmpkt_lbhdr_fid_support_get(bcmdrd_dev_type_t dev_type,
                             bcmpkt_lbhdr_fid_support_t *support)
{
    int i;
    bcmpkt_pmd_view_info_t view_info;

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }
    if (view_info_get[dev_type] == NULL) {
        return SHR_E_INTERNAL;
    }
    if (support == NULL) {
        return SHR_E_PARAM;
    }
    sal_memset(support, 0, sizeof(*support));

    view_info_get[dev_type](&view_info);
    if ((view_info.view_types == NULL) || (view_info.view_infos == NULL)) {
        return SHR_E_UNAVAIL;
    }

    for (i = BCMPKT_LBHDR_FID_INVALID + 1; i < BCMPKT_LBHDR_FID_COUNT; i++) {
        if (view_info.view_infos[i] >= -1) {
            SHR_BITSET(support->fbits, i);
        }
    }

    return SHR_E_NONE;
}

