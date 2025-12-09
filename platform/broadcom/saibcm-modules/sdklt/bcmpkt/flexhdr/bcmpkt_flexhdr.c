/*! \file bcmpkt_flexhdr.c
 *
 * Flexhdr access interface.
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
#include <bcmpkt/bcmpkt_flexhdr.h>
#include <bcmpkt/bcmpkt_flexhdr_internal.h>
#include <bcmpkt/bcmpkt_rxpmd_internal.h>


/* Define stub functions for base variant. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
bcmpkt_flex_pmd_info_t *  _bc##_flex_pmd_info_get(uint32_t hid) {return  NULL;}
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
shr_enum_map_t *  _bc##_flexhdr_map_get(void) {return NULL;}
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    int _bc##_flexhdr_variant_support_map[] = {-1, -1, -1, -1};
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

/* Array of device variant specific data */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1) \
    &_bd##_vu##_va##_flex_pmd_info_get,
static bcmpkt_flex_pmd_info_t * (*flex_pmd_info_get[])(uint32_t hid) = {
    NULL,
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    NULL
};

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1) \
    &_bd##_vu##_va##_flexhdr_map_get,
static shr_enum_map_t * (*flexhdr_map_get[])(void) = {
    NULL,
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    NULL
};

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1) \
    &_bd##_vu##_va##_flexhdr_variant_support_map[0],

int *bcmpkt_flexhdr_variant_support_map[] = {
    NULL,
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    NULL
};

int
bcmpkt_flexhdr_header_name_get(bcmlrd_variant_t variant,
                               uint32_t hid, char **name)
{
    shr_enum_map_t *id_map = NULL;

    if (name == NULL) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    id_map = flexhdr_map_get[variant]();
    if (id_map == NULL) {
        return SHR_E_UNAVAIL;
    }
    while (sal_strcasecmp(id_map->name, "flexhdr count") != 0) {
        if (id_map->val == (int)hid) {
            *name = id_map->name;
            return SHR_E_NONE;
        }
        id_map++;
    }

    return SHR_E_UNAVAIL;
}

int
bcmpkt_flexhdr_header_id_get(bcmlrd_variant_t variant,
                             char *name, uint32_t *hid)
{
    shr_enum_map_t *id_map = NULL;

    if ((name == NULL) || (hid == NULL)) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    id_map = flexhdr_map_get[variant]();
    if (id_map == NULL) {
        return SHR_E_UNAVAIL;
    }

    while (1) {
        if (sal_strcasecmp(id_map->name, name) == 0) {
            *hid = id_map->val;
            return SHR_E_NONE;
        }
        if (sal_strcasecmp(id_map->name, "flexhdr count") == 0) {
            break;
        }
        id_map++;
    }

    return SHR_E_UNAVAIL;
}

/*
 * SDKLT-43974: This is a simple workaround. Normal fix will be ready in the
 * JIRA.
 */
int
bcmpkt_flexhdr_len_get(bcmlrd_variant_t variant, uint32_t hid,
                       uint32_t *len)
{
    int rv;
    char *name = NULL;

    if (len == NULL) {
        return SHR_E_PARAM;
    }
    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    rv = bcmpkt_flexhdr_header_name_get(variant, hid, &name);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    if (sal_strcasecmp(name, "generic_loopback_t") == 0) {
        *len = 16;
    } else if (sal_strcasecmp(name, "hg3_base_t") == 0 ||
               sal_strcasecmp(name, "hg3_extension_0_t") == 0) {
        *len = 8;
    } else if (sal_strcasecmp(name, "vlan_t") == 0) {
        *len = 4;
    } else {
        /* no support*/
        *len = 0;
    }

    return SHR_E_NONE;
}

int
bcmpkt_flexhdr_is_supported(bcmlrd_variant_t variant, uint32_t hid,
                            bool *is_supported)
{
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;

    if (is_supported == NULL) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    *is_supported = pmd_info->is_supported;

    return SHR_E_NONE;
}

int
bcmpkt_flexhdr_field_get(bcmlrd_variant_t variant, uint32_t hid,
                         uint32_t *flexhdr, int profile, int fid, uint32_t *val)
{
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    bcmpkt_flex_field_metadata_t *fld_info = NULL;

    if ((flexhdr == NULL) || (val == NULL)) {
        return SHR_E_PARAM;
    }
    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }
    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->field_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (fid <= BCMPKT_FID_INVALID || fid >= pmd_info->field_info->num_fields) {
        return SHR_E_PARAM;
    }

    if (pmd_info->flex_fget != NULL) {
        if (pmd_info->flex_fget[fid] == NULL) {
            return SHR_E_UNAVAIL;
        }
        return (pmd_info->flex_fget[fid])(flexhdr, profile, val);
    } else if (pmd_info->flex_common_fget != NULL) {
        fld_info = &pmd_info->field_info->info[fid];
        return (pmd_info->flex_common_fget)(flexhdr, fld_info, profile, val);
    } else {
        return SHR_E_UNAVAIL;
    }

    return SHR_E_NONE;
}

int
bcmpkt_flexhdr_field_set(bcmlrd_variant_t variant, uint32_t hid,
                         uint32_t *flexhdr, int profile, int fid, uint32_t val)
{
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    bcmpkt_flex_field_metadata_t *fld_info = NULL;

    if (flexhdr == NULL) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->field_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (fid <= BCMPKT_FID_INVALID || fid >= pmd_info->field_info->num_fields) {
        return SHR_E_PARAM;
    }

    if (pmd_info->flex_fset != NULL) {
        if (pmd_info->flex_fset[fid] == NULL) {
            return SHR_E_UNAVAIL;
        }
        return (pmd_info->flex_fset[fid])(flexhdr, profile, val);
    } else if (pmd_info->flex_common_fset != NULL) {
        fld_info = &pmd_info->field_info->info[fid];
        return (pmd_info->flex_common_fset)(flexhdr, fld_info, profile, val);
    } else {
        return SHR_E_UNAVAIL;
    }

    return SHR_E_NONE;
}

int
bcmpkt_flexhdr_field_name_get(bcmlrd_variant_t variant, uint32_t hid,
                              int fid, char **name)
{
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;

    if (name == NULL) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }
    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->field_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (fid <= BCMPKT_FID_INVALID || fid >= pmd_info->field_info->num_fields) {
        return SHR_E_PARAM;
    }

    if (pmd_info->field_info->info == NULL) {
        return SHR_E_UNAVAIL;
    }
    *name = pmd_info->field_info->info[fid].name;

    return SHR_E_NONE;
}

int
bcmpkt_flexhdr_field_id_get(bcmlrd_variant_t variant, uint32_t hid,
                            char *name, int *fid)
{
    int i;
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;

    if ((name == NULL) || (fid == NULL)) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->field_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->field_info->info == NULL) {
        return SHR_E_UNAVAIL;
    }

    for (i = BCMPKT_FID_INVALID + 1; i < pmd_info->field_info->num_fields; i++) {
        if (sal_strcasecmp(pmd_info->field_info->info[i].name, name) == 0) {
            *fid = pmd_info->field_info->info[i].fid;
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}

int
bcmpkt_flexhdr_field_info_get(bcmlrd_variant_t variant, uint32_t hid,
                              bcmpkt_flex_field_info_t *info)
{
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;

    if (info == NULL) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->field_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    *info = *(pmd_info->field_info);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_flex_reasons_get(bcmlrd_variant_t variant,
                              uint32_t *rxpmd_flex, bcmpkt_bitmap_t *reasons)
{
    int32_t ret = SHR_E_NONE;
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    uint32_t hid;

    if ((rxpmd_flex == NULL) || (reasons == NULL)) {
        return SHR_E_PARAM;
    }

    ret = bcmpkt_flexhdr_header_id_get(variant, "RXPMD_FLEX_T", &hid);
    if (ret < 0) {
        return ret;
    }
    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }
    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->reasons_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    SHR_BITCLR_RANGE
        (((*reasons).pbits), 0, pmd_info->reasons_info->num_reasons);
    pmd_info->reasons_info->reason_decode (rxpmd_flex, reasons);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_flex_reasons_set(bcmlrd_variant_t variant,
                              bcmpkt_bitmap_t *reasons, uint32_t *rxpmd_flex)
{
    int32_t ret = SHR_E_NONE;
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    uint32_t hid;

    if ((rxpmd_flex == NULL) || (reasons == NULL)) {
        return SHR_E_PARAM;
    }

    ret = bcmpkt_flexhdr_header_id_get(variant, "RXPMD_FLEX_T", &hid);
    if (ret < 0) {
        return ret;
    }
    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }
    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->reasons_info == NULL) {
        return SHR_E_UNAVAIL;
    }


    pmd_info->reasons_info->reason_encode(reasons, rxpmd_flex);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_flex_reason_name_get(bcmlrd_variant_t variant,
                                  int reason, char **name)
{
    int32_t ret = SHR_E_NONE;
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    uint32_t hid;

    if (name == NULL) {
        return SHR_E_PARAM;
    }

    ret = bcmpkt_flexhdr_header_id_get(variant, "RXPMD_FLEX_T", &hid);
    if (ret < 0) {
        return ret;
    }
    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }
    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->reasons_info == NULL) {
        return SHR_E_UNAVAIL;
    }


    if (reason <= 0 ||
        reason > pmd_info->reasons_info->num_reasons) {
        return SHR_E_PARAM;
    }

    if (pmd_info->reasons_info->reason_names == NULL) {
        return SHR_E_UNAVAIL;
    }
    *name = pmd_info->reasons_info->reason_names[reason].name;

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_flex_reason_max_get(bcmlrd_variant_t variant, uint32_t *num)
{
    int32_t ret = SHR_E_NONE;
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    uint32_t hid;

    if (num == NULL) {
        return SHR_E_PARAM ;
    }

    ret = bcmpkt_flexhdr_header_id_get(variant, "RXPMD_FLEX_T", &hid);
    if (ret < 0) {
        return ret;
    }
    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }
    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->reasons_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    *num = pmd_info->reasons_info->num_reasons;

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_flex_reason_id_get(bcmlrd_variant_t variant,
                                char *name, int *rid)
{
    int32_t ret = SHR_E_NONE;
    bcmpkt_flex_pmd_info_t *pmd_info = NULL;
    int i;
    uint32_t hid;

    if ((name == NULL) || (rid == NULL)) {
        return SHR_E_PARAM;
    }

    ret = bcmpkt_flexhdr_header_id_get(variant, "RXPMD_FLEX_T", &hid);
    if (ret < 0) {
        return ret;
    }
    if (flex_pmd_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }
    pmd_info = flex_pmd_info_get[variant](hid);
    if (pmd_info == NULL) {
        return SHR_E_UNAVAIL;
    }
    if (pmd_info->reasons_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    if (pmd_info->reasons_info->reason_names == NULL) {
        return SHR_E_UNAVAIL;
    }
    for (i = 0; i < pmd_info->reasons_info->num_reasons; i++) {
        if (sal_strcasecmp(pmd_info->reasons_info->reason_names[i].name,
            name) == 0) {
            *rid = pmd_info->reasons_info->reason_names[i].val;
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}

int *
bcmpkt_flexhdr_support_map_get(bcmlrd_variant_t variant)
{
    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return NULL;
    }

    return bcmpkt_flexhdr_variant_support_map[variant];
}
