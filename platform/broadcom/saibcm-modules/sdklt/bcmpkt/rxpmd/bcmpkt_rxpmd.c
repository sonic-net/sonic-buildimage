/*! \file bcmpkt_rxpmd.c
 *
 * RX Packet Meta Data (RXPMD, called EP_TO_CPU in hardware) access interfaces.
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
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_rxpmd_internal.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_fget_t _bd##_rxpmd_fget;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_fget,
/*! This sequence should be same as bcmdrd_cm_dev_type_t */
static const bcmpkt_rxpmd_fget_t *rxpmd_fget[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_fset_t _bd##_rxpmd_fset;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_fset,
/*! This sequence should be same as bcmdrd_cm_dev_type_t */
static const bcmpkt_rxpmd_fset_t *rxpmd_fset[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_figet_t _bd##_rxpmd_figet;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_figet,
/*! This sequence should be same as bcmdrd_cm_dev_type_t */
static const bcmpkt_rxpmd_figet_t *rxpmd_figet[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rx_reason_decode,
/*! This sequence should be same as bcmdrd_cm_dev_type_t */
static void (*reason_fdecode[])(const uint32_t*, bcmpkt_rx_reasons_t*) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rx_reason_encode,
static void (*reason_fencode[])(const bcmpkt_rx_reasons_t*, uint32_t*) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_ep_rx_reason_decode,
/*! This sequence should be same as bcmdrd_cm_dev_type_t */
static void (*ep_reason_fdecode[])(const uint32_t*, bcmpkt_rx_reasons_t*) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_ep_rx_reason_encode,
static void (*ep_reason_fencode[])(const bcmpkt_rx_reasons_t*, uint32_t*) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_view_info_get,
static void (*view_info_get[])(bcmpkt_pmd_view_info_t *) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};


static const shr_enum_map_t field_names[] =
{
    BCMPKT_RXPMD_FIELD_NAME_MAP_INIT
};

static const shr_enum_map_t reason_names[] =
{
    BCMPKT_REASON_NAME_MAP_INIT
};

int
bcmpkt_rxpmd_len_get(bcmdrd_dev_type_t dev_type, uint32_t *len)
{

    if (len == NULL) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_figet[dev_type] == NULL ||
        rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_SIZE] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *len = rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_SIZE](NULL, NULL) * 4;

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_field_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                       int fid, uint32_t *val)
{

    if ((rxpmd == NULL) || (val == NULL)) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE || dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_RXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_fget[dev_type] == NULL ||
        rxpmd_fget[dev_type]->fget[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *val = rxpmd_fget[dev_type]->fget[fid](rxpmd);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_field_set(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                       int fid, uint32_t val)
{

    if (rxpmd == NULL) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE || dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_RXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_fset[dev_type] == NULL ||
        rxpmd_fset[dev_type]->fset[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    rxpmd_fset[dev_type]->fset[fid](rxpmd, val);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_mh_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                    uint32_t **hg_hdr)
{
    int len;

    if ((rxpmd == NULL) || (hg_hdr == NULL)) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE || dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_figet[dev_type] == NULL ||
        rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_MODULE_HDR] == NULL) {
        return SHR_E_UNAVAIL;
    }

    len = rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_MODULE_HDR](rxpmd, hg_hdr);
    if (len <= 0) {
        return SHR_E_INTERNAL;
    }

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_flexdata_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                          uint32_t **flexdata, uint32_t *len)
{

    if ((rxpmd == NULL) || (flexdata == NULL) || (len == NULL)) {
        return SHR_E_PARAM;
    }

    *len = 0;
    if (dev_type <= BCMDRD_DEV_T_NONE || dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_figet[dev_type] == NULL ||
        rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_FLEX_DATA] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *len = rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_FLEX_DATA](rxpmd, flexdata);
    if (*len == 0) {
        return SHR_E_INTERNAL;
    }

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_reasons_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                         bcmpkt_rx_reasons_t *reasons)
{
    uint32_t *reason = NULL;
    int len;
    shr_error_t rv = SHR_E_NONE;
    uint32_t reason_type = BCMPKT_RXPMD_REASON_T_FROM_IP;

    if ((rxpmd == NULL) || (reasons == NULL)) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE || dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_figet[dev_type] == NULL ||
        rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_REASON] == NULL) {
        return SHR_E_UNAVAIL;
    }

    BCMPKT_RX_REASON_CLEAR_ALL(*reasons);
    len = rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_REASON](rxpmd, &reason);
    if (len <= 0) {
        return SHR_E_INTERNAL;
    }

    rv = bcmpkt_rxpmd_field_get(dev_type, rxpmd, BCMPKT_RXPMD_REASON_TYPE,
                                &reason_type);
    if ((rv == SHR_E_NONE) && (reason_type == BCMPKT_RXPMD_REASON_T_FROM_EP)) {
        ep_reason_fdecode[dev_type](reason, reasons);
    } else {
        reason_fdecode[dev_type](reason, reasons);
    }

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_reasons_set(bcmdrd_dev_type_t dev_type,
                         bcmpkt_rx_reasons_t *reasons, uint32_t *rxpmd)
{
    uint32_t *reason = NULL;
    int len;
    shr_error_t rv = SHR_E_NONE;
    uint32_t reason_type = BCMPKT_RXPMD_REASON_T_FROM_IP;

    if ((rxpmd == NULL) || (reasons == NULL)) {
        return SHR_E_PARAM;
    }


    if (dev_type <= BCMDRD_DEV_T_NONE || dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_figet[dev_type] == NULL ||
        rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_REASON] == NULL) {
        return SHR_E_UNAVAIL;
    }
    len = rxpmd_figet[dev_type]->fget[BCMPKT_RXPMD_I_REASON](rxpmd, &reason);
    if (len <= 0) {
        return SHR_E_UNAVAIL;
    }

    sal_memset(reason, 0, len * 4);
    rv = bcmpkt_rxpmd_field_get(dev_type, rxpmd, BCMPKT_RXPMD_REASON_TYPE,
                                (uint32_t *)&reason_type);
    if ((rv == SHR_E_NONE) && (reason_type == BCMPKT_RXPMD_REASON_T_FROM_EP)) {
        ep_reason_fencode[dev_type](reasons, reason);
    } else {
        reason_fencode[dev_type](reasons, reason);
    }
    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_field_name_get(int fid, char **name)
{
    if (name == NULL) {
        return SHR_E_PARAM;
    }
    if (fid <= BCMPKT_RXPMD_FID_INVALID ||
        fid >= BCMPKT_RXPMD_FID_COUNT) {
        return SHR_E_PARAM;
    }

    *name = field_names[fid].name;

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_field_id_get(char* name, int *fid)
{
    int i;

    if ((name == NULL) || (fid == NULL)) {
        return SHR_E_PARAM;
    }

    for (i = BCMPKT_RXPMD_FID_INVALID + 1; i < BCMPKT_RXPMD_FID_COUNT; i++) {
        if (sal_strcasecmp(field_names[i].name, name) == 0) {
            *fid = field_names[i].val;
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}

int
bcmpkt_rxpmd_fid_support_get(bcmdrd_dev_type_t dev_type,
                             bcmpkt_rxpmd_fid_support_t *support)
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

    for (i = BCMPKT_RXPMD_FID_INVALID + 1; i < BCMPKT_RXPMD_FID_COUNT; i++) {
        if (view_info.view_infos[i] >= -1) {
            SHR_BITSET(support->fbits, i);
        }
    }

    return SHR_E_NONE;
}

int
bcmpkt_rx_reason_name_get(int reason, char **name)
{

    if (name == NULL) {
        return SHR_E_PARAM;
    }

    if (reason <= BCMPKT_RX_REASON_NONE ||
        reason > BCMPKT_RX_REASON_COUNT) {
        return SHR_E_PARAM;
    }

    *name = reason_names[reason].name;

    return SHR_E_NONE;
}

