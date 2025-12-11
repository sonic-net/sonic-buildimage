/*! \file bcmpkt_txpmd.c
 *
 * TX Packet MetaData (TXPMD, called SOBMH in hardware) access interface.
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
#include <bcmpkt/bcmpkt_txpmd.h>
#include <bcmpkt/bcmpkt_txpmd_internal.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>


#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_txpmd_fget_t _bd##_txpmd_fget;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_txpmd_fget,
static const bcmpkt_txpmd_fget_t *txpmd_fget[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_txpmd_fset_t _bd##_txpmd_fset;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_txpmd_fset,
static const bcmpkt_txpmd_fset_t *txpmd_fset[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_txpmd_figet_t _bd##_txpmd_figet;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_txpmd_figet,
static const bcmpkt_txpmd_figet_t *txpmd_figet[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_txpmd_view_info_get,
static void (*view_info_get[])(bcmpkt_pmd_view_info_t *info) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

static const shr_enum_map_t field_names[] =
{
    BCMPKT_TXPMD_FIELD_NAME_MAP_INIT
};

int
bcmpkt_txpmd_len_get(bcmdrd_dev_type_t dev_type, uint32_t *len)
{
    if (len == NULL) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (txpmd_figet[dev_type] == NULL ||
        txpmd_figet[dev_type]->fget[BCMPKT_TXPMD_I_SIZE] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *len = txpmd_figet[dev_type]->fget[BCMPKT_TXPMD_I_SIZE](NULL, NULL) * 4;

    return SHR_E_NONE;
}

int
bcmpkt_txpmd_field_get(bcmdrd_dev_type_t dev_type, uint32_t *txpmd,
                       int fid, uint32_t *val)
{

    if ((txpmd == NULL) || (val == NULL)) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_TXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (txpmd_fget[dev_type] == NULL ||
        txpmd_fget[dev_type]->fget[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *val = txpmd_fget[dev_type]->fget[fid](txpmd);

    return SHR_E_NONE;
}

int
bcmpkt_txpmd_field_set(bcmdrd_dev_type_t dev_type, uint32_t *txpmd,
                       int fid, uint32_t val)
{
    if (txpmd == NULL) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_TXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (txpmd_fset[dev_type] == NULL ||
        txpmd_fset[dev_type]->fset[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    txpmd_fset[dev_type]->fset[fid](txpmd, val);

    return SHR_E_NONE;
}

int
bcmpkt_txpmd_field_name_get(int fid, char **name)
{
    if (name == NULL) {
        return SHR_E_PARAM;
    }

    if (fid <= BCMPKT_TXPMD_FID_INVALID ||
        fid >= BCMPKT_TXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    *name = field_names[fid].name;

    return SHR_E_NONE;
}

int
bcmpkt_txpmd_field_id_get(char* name, int *fid)
{
    int i;

    if ((name == NULL) || (fid == NULL)) {
        return SHR_E_PARAM;
    }

    for (i = BCMPKT_TXPMD_FID_INVALID + 1; i < BCMPKT_TXPMD_FID_COUNT; i++) {
        if (sal_strcasecmp(field_names[i].name, name) == 0) {
            *fid = field_names[i].val;
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}

int
bcmpkt_txpmd_fid_support_get(bcmdrd_dev_type_t dev_type,
                             bcmpkt_txpmd_fid_support_t *support)
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

    for (i = BCMPKT_TXPMD_FID_INVALID + 1; i < BCMPKT_TXPMD_FID_COUNT; i++) {
        if (view_info.view_infos[i] >= -1) {
            SHR_BITSET(support->fbits, i);
        }
    }

    return SHR_E_NONE;
}

int
bcmpkt_txpmd_fid_view_get(bcmdrd_dev_type_t dev_type,
                          int fid, int *view)

{
    bcmpkt_pmd_view_info_t view_info;

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (view == NULL) {
        return SHR_E_PARAM;
    }

    if (fid <= BCMPKT_TXPMD_FID_INVALID ||
        fid >= BCMPKT_TXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (view_info_get[dev_type] == NULL) {
        return SHR_E_INTERNAL;
    }

    view_info_get[dev_type](&view_info);
    if ((view_info.view_types == NULL) || (view_info.view_infos == NULL)) {
        return SHR_E_UNAVAIL;
    }
    *view = view_info.view_infos[fid];

    return SHR_E_NONE;
}
