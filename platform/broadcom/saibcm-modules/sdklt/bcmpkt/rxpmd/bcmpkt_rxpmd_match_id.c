/*! \file bcmpkt_rxpmd_match_id.c
 *
 * RX Packet Metadata API to return the RXPMD match id information.
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
#include <shr/shr_bitop.h>
#include <sal/sal_libc.h>
#include <bcmlrd/bcmlrd_conf.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id_defs.h>

/* Define stub functions for base variant. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
bcmpkt_rxpmd_match_id_db_info_t * \
_bc##_rxpmd_match_id_db_info_get(void) {return  NULL;}
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
bcmpkt_rxpmd_match_id_map_info_t * \
_bc##_rxpmd_match_id_map_info_get(void) {return  NULL;}
#include <bcmdrd/bcmdrd_devlist.h>

/* Array of device variant specific api's */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1) \
    &_bd##_vu##_va##_rxpmd_match_id_db_info_get,
static bcmpkt_rxpmd_match_id_db_info_t * (*rxpmd_match_id_db_info_get[])(void) = {
    NULL,
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    NULL
};

/* Array of device variant specific api's */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1) \
    &_bd##_vu##_va##_rxpmd_match_id_map_info_get,
static bcmpkt_rxpmd_match_id_map_info_t * (*rxpmd_match_id_map_info_get[])(void) = {
    NULL,
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    NULL
};


int
bcmpkt_rxpmd_match_id_get(bcmlrd_variant_t  variant,
                                  char     *name,
                                  uint32_t *match_id)
{
    const bcmpkt_rxpmd_match_id_map_info_t *map_info;
    const shr_enum_map_t *id_map = NULL;
    uint32_t idx;

    if ((name == NULL) || (match_id == NULL)) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_match_id_map_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    map_info = rxpmd_match_id_map_info_get[variant]();
    if (map_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    id_map = map_info->map;
    if (id_map == NULL) {
        return SHR_E_UNAVAIL;
    }

    idx = 0;
    while (idx < map_info->num_entries) {
        if (sal_strcasecmp(id_map->name, name) == 0) {
            *match_id = id_map->val;
            return SHR_E_NONE;
        }
        idx++;
        id_map++;
    }

    return SHR_E_UNAVAIL;
}


int
bcmpkt_rxpmd_match_id_present(bcmlrd_variant_t variant,
                                     uint32_t *match_id_array,
                                     uint32_t  array_len,
                                     uint32_t  match_id)
{
    const bcmpkt_rxpmd_match_id_db_t *db;
    const bcmpkt_rxpmd_match_id_db_t *db_entry;
    const bcmpkt_rxpmd_match_id_db_info_t *db_info;
    uint32_t lsb, msb, match_data;
    uint32_t start, right_shift;

    if (match_id_array == NULL) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_match_id_db_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    db_info = rxpmd_match_id_db_info_get[variant]();
    if (db_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    db = db_info->db;
    if (db == NULL) {
        return SHR_E_UNAVAIL;
    }

    /* Check to see if db_entry is in the valid range */
    if (match_id >= db_info->num_entries) {
        return SHR_E_PARAM;
    }

    db_entry = &db[match_id];

    start = db_entry->match_minbit / 32;
    if (start >= array_len) {
        return SHR_E_PARAM;
    }

    /* Extract the bits for the match_id from the match_id data */
    right_shift = db_entry->match_minbit % 32;
    lsb = match_id_array[start] >> right_shift;

    if (start == 1) {
        msb = 0;
    } else {
        msb = match_id_array[start + 1] & ((1 << right_shift) - 1);
        msb <<= (32 - right_shift);
    }

    /* Mask off  the data and see if it matched for the match_id */
    match_data = msb | lsb;
    match_data &= db_entry->match_mask;
    if (match_data == db_entry->match) {
        return SHR_E_NONE;
    }

    return SHR_E_NOT_FOUND;
}

int
bcmpkt_rxpmd_match_id_from_arc_id_present(bcmlrd_variant_t variant,
                                          uint32_t *arc_id_array,
                                          uint32_t  array_len,
                                          uint32_t  match_id)
{
    uint64_t arc_id;
    const bcmpkt_rxpmd_match_id_db_t *db;
    const bcmpkt_rxpmd_match_id_db_t *db_entry;
    const bcmpkt_rxpmd_match_id_db_info_t *db_info;

    if ((arc_id_array == NULL) || (array_len != 2)) {
        return SHR_E_PARAM;
    }

    if (variant <= BCMLRD_VARIANT_T_NONE || variant >= BCMLRD_VARIANT_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_match_id_db_info_get[variant] == NULL) {
        return SHR_E_UNAVAIL;
    }

    db_info = rxpmd_match_id_db_info_get[variant]();
    if (db_info == NULL) {
        return SHR_E_UNAVAIL;
    }

    db = db_info->db;
    if (db == NULL) {
        return SHR_E_UNAVAIL;
    }

    /* Check to see if db_entry is in the valid range */
    if (match_id >= db_info->num_entries) {
        return SHR_E_PARAM;
    }

    db_entry = &db[match_id];

    if (db_entry->zone_bmp != NULL) {

        arc_id = (((uint64_t)arc_id_array[1] << 32) | arc_id_array[0]);
        arc_id = (arc_id & db_entry->arc_id_mask) >> db_entry->zone_minbit;

        /* Check to see if arc id is in the valid range */
        if (arc_id >= db_entry->num_zone_bmp_words * 32) {
            return SHR_E_NOT_FOUND;
        }

        if (SHR_BITGET(db_entry->zone_bmp, arc_id)) {
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}
