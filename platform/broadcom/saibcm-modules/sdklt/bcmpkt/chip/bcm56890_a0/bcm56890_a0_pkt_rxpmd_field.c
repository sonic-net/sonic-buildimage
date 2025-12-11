/*! \file bcm56890_a0_pkt_rxpmd_field.c
 *
 * This file provides RXPMD access functions for BCM56890_A0.
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

#include <sal/sal_types.h>
#include <shr/shr_error.h>
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_rxpmd_internal.h>
#include <bcmpkt/bcmpkt_flexhdr_internal.h>
#include <bcmpkt/chip/bcm56890_a0/bcmpkt_bcm56890_a0_rxpmd.h>

#define BSL_LOG_MODULE BSL_LS_BCMPKT_FLEX_HDR

#define MASK(_bn) (((uint32_t)0x1<<(_bn))-1)
#define WORD_FIELD_GET(_d,_s,_l) (((_d) >> (_s)) & MASK(_l))
#define WORD_FIELD_SET(_d,_s,_l,_v) (_d)=(((_d) & ~(MASK(_l) << (_s))) | (((_v) & MASK(_l)) << (_s)))
#define WORD_FIELD_MASK(_d,_s,_l) (_d)=((_d) | (MASK(_l) << (_s)))

int bcm56890_a0_rxpmd_flex_fget(uint32_t *data,
                                bcmpkt_flex_field_metadata_t *fld_info,
                                int prof,
                                uint32_t *val)
{
    uint32_t hdr_words = 14; /* MPB_FLEX_DATA size in words. */
    uint32_t minbit = fld_info->profile[prof].minbit;
    uint32_t maxbit = fld_info->profile[prof].maxbit;
    uint32_t minword = minbit / 32;
    uint32_t low_bit = minbit - (minword * 32);
    uint32_t high_bit = maxbit - (minword * 32);
    uint32_t diff = high_bit - low_bit;
    uint32_t index = hdr_words - minword - 1;

    /* Profile not valid for this field. */
    if ((minbit == 0xFFFFFFFF) ||
        (prof >= fld_info->profile_cnt)) {
        return SHR_E_PARAM;
    }

    /* Skip fields with minbit >= 448.*/
    if (minbit >= 448) {
        return SHR_E_PARAM;
    }

    if (diff == 31) {
        *val = data[index];
    } else if (diff < 31) {
        *val = WORD_FIELD_GET(data[index], low_bit, diff+1);
    } else {
        return SHR_E_PARAM;
    }

    return SHR_E_NONE;
}

int bcm56890_a0_rxpmd_flex_fset(uint32_t *data,
                                bcmpkt_flex_field_metadata_t *fld_info,
                                int prof,
                                uint32_t val)
{
    uint32_t hdr_words = 14; /* MPB_FLEX_DATA size in words. */
    uint32_t minbit = fld_info->profile[prof].minbit;
    uint32_t maxbit = fld_info->profile[prof].maxbit;
    uint32_t minword = minbit / 32;
    uint32_t low_bit = minbit - (minword * 32);
    uint32_t high_bit = maxbit - (minword * 32);
    uint32_t diff = high_bit - low_bit;
    uint32_t index = hdr_words - minword - 1;

    /* Profile not valid for this field. */
    if ((minbit == 0xFFFFFFFF) ||
        (prof >= fld_info->profile_cnt)) {
        return SHR_E_PARAM;
    }

    /* Skip fields with minbit >= 448.*/
    if (minbit >= 448) {
        return SHR_E_PARAM;
    }

    if (diff == 31) {
        data[index] = val;
    } else if (diff < 31) {
        WORD_FIELD_SET(data[index], low_bit, diff+1, val);
    } else {
        return SHR_E_PARAM;
    }

    return SHR_E_NONE;
}
