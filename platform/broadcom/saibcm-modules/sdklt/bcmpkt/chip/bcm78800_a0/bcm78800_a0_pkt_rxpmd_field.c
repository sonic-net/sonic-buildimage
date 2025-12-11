/*! \file bcm78800_a0_pkt_rxpmd_field.c
 *
 * This file provides RXPMD access functions for BCM78800_A0.
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
#include <bcmpkt/chip/bcm78800_a0/bcmpkt_bcm78800_a0_rxpmd.h>

#define BSL_LOG_MODULE BSL_LS_BCMPKT_FLEX_HDR

#define MASK(_bn) (((uint32_t)0x1<<(_bn))-1)
#define WORD_FIELD_GET(_d,_s,_l) (((_d) >> (_s)) & MASK(_l))
#define WORD_FIELD_SET(_d,_s,_l,_v) (_d)=(((_d) & ~(MASK(_l) << (_s))) | (((_v) & MASK(_l)) << (_s)))
#define WORD_FIELD_MASK(_d,_s,_l) (_d)=((_d) | (MASK(_l) << (_s)))

#define RXPMD_FIXED_WORD_COUNT   4

extern const bcmpkt_rxpmd_fget_t bcm78800_a0_rxpmd_fget;

int bcm78800_a0_rxpmd_flex_fget(uint32_t *data,
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
    /*
     * data - points to beginning of RXPMD flex data.
     * rxpmd_data - points to beginning of RXPMD data.
     * First 4 words of RXPMD header are fixed words and the subsequent
     * 14 words are flex words.
     */
    uint32_t *rxpmd_data = data - RXPMD_FIXED_WORD_COUNT;
    uint32_t egr_recirc_profile_index = 0;
    uint32_t cpu_dma_header_subtype = 0;
    uint32_t rxpmd_header_ver = 0;

    /* Profile not valid for this field. */
    if ((minbit == 0xFFFFFFFF) ||
        (prof >= fld_info->profile_cnt)) {
        return SHR_E_PARAM;
    }

    /*
     * RXPMD flex word data[0] is set by the EPOST.
     * cpu_dma_header_subtype = data[0][31:28].
     * egr_recirc_profile_index = data[0][27:24].
     */
    if (sal_strcmp(fld_info->name, "DROP_CODE_15_0") == 0) {
        rxpmd_header_ver = bcm78800_a0_rxpmd_fget.fget[BCMPKT_RXPMD_DMA_HEADER_VERSION](rxpmd_data);
        if (rxpmd_header_ver == 1) {
            cpu_dma_header_subtype = WORD_FIELD_GET(data[0], 28, 4);
            egr_recirc_profile_index =  WORD_FIELD_GET(data[0], 24, 4);
            if ((cpu_dma_header_subtype & 0x2) && (egr_recirc_profile_index%2)) {
                index = 2;
            }
        }
    }

    /* Skip fields with minbit >= 448.*/
    if (minbit >= 448) {
        *val = 0;
        return SHR_E_NONE;
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

int bcm78800_a0_rxpmd_flex_fset(uint32_t *data,
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
