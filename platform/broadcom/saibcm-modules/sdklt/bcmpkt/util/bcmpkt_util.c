/*! \file bcmpkt_util.c
 *
 * BCMPKT utility functions.
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

#include <sal/sal_libc.h>
#include <shr/shr_types.h>
#include <bcmdrd/bcmdrd_types.h>
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
#include <bcmpkt/bcmpkt_util.h>

/*! This sequence should be same as bcmdrd_cm_dev_type_t */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    {#_bd, _dv, BCMDRD_DEV_T_##_bd},
static const struct {
    char *dev_name;
    uint32_t id;
    bcmdrd_dev_type_t dev_type;
} device_types[] = {
    {"none", 0, BCMDRD_DEV_T_NONE},
#include <bcmdrd/bcmdrd_devlist.h>
    {"invalid",0, BCMDRD_DEV_T_COUNT}
};

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
    {#_bd, #_ve, BCMLRD_VARIANT_T_##_bd##_##_ve},
static const struct {
    char *dev_name;
    char *var_name;
    bcmlrd_variant_t var_type;
} variant_types[] = {
    {"none", "none", BCMLRD_VARIANT_T_NONE},
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    {"invalid", "invalid", BCMLRD_VARIANT_T_COUNT}
};

bcmdrd_dev_type_t
bcmpkt_util_dev_type_get(const char *dev_name)
{
    int idx;

    for (idx = 0; idx < COUNTOF(device_types); idx++) {
        if (sal_strcasecmp(dev_name, device_types[idx].dev_name) == 0) {
            return device_types[idx].dev_type;
        }
    }
    return BCMDRD_DEV_T_NONE;
}

bcmlrd_variant_t
bcmpkt_util_variant_type_get(const char *dev_name, const char *var_name)
{
    int idx;

    for (idx = 0; idx < COUNTOF(variant_types); idx++) {
        if (sal_strcasecmp(dev_name, variant_types[idx].dev_name) == 0 &&
            sal_strcasecmp(var_name, variant_types[idx].var_name) == 0) {
            return variant_types[idx].var_type;
        }
    }
    return BCMLRD_VARIANT_T_NONE;
}

uint32_t
bcmpkt_util_dev_id_get(const bcmdrd_dev_type_t dev_type)
{
    int idx;

    for (idx = 0; idx < COUNTOF(device_types); idx++) {
        if (dev_type == device_types[idx].dev_type) {
            return device_types[idx].id;
        }
    }
    return BCMDRD_DEV_T_NONE;
}

void
bcmpkt_util_rcpu_hdr_init(const bcmdrd_dev_type_t dev_type,
                          bcmpkt_rcpu_hdr_t *rhdr)

{
    if (rhdr) {
        sal_memset(rhdr, 0, sizeof(*rhdr));
        rhdr->tpid = BCMPKT_RCPU_TPID;
        rhdr->vlan = BCMPKT_RCPU_VLAN;
        rhdr->ethertype = BCMPKT_RCPU_ETYPE;
        rhdr->flags = BCMPKT_RCPU_F_MODHDR;
        rhdr->signature = bcmpkt_util_dev_id_get(dev_type);
    }
}
