/*! \file shr_bitop_range_clear.c
 *
 * Bit array operations.
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

#include <shr/shr_bitop.h>

/*!
 * INTERNAL USE ONLY.
 *
 * Same as shr_bitop_range_clear, but for a single SHR_BITDCL.
 */
static inline void
shr_bitop_range_clear_one_bitdcl(SHR_BITDCL *a, int offs, int n)
{
    SHR_BITDCL mask = ~0;

    mask >>= (SHR_BITWID - n);
    mask <<= offs;
    *a &= ~mask;
}

/*!
 * \brief Clear range of bits in a bit array.
 *
 * INTERNAL USE ONLY.
 *
 * Refer to \ref SHR_BITCLR_RANGE macro.
 */
void
shr_bitop_range_clear(SHR_BITDCL *a, int offs, int n)
{
    SHR_BITDCL *pa;
    int woffs, wremain;

    if (n <= 0) {
        return;
    }

    pa = a + (offs / SHR_BITWID);

    woffs = offs % SHR_BITWID;

    if (woffs != 0) {
        wremain = SHR_BITWID - woffs;
        if (n <= wremain) {
            shr_bitop_range_clear_one_bitdcl(pa, woffs, n);
            return;
        }
        shr_bitop_range_clear_one_bitdcl(pa, woffs, wremain);
        n -= wremain;
        ++pa;
    }
    while (n >= SHR_BITWID) {
        *(pa++) = 0;
        n -= SHR_BITWID;
    }

    if (n > 0) {
        shr_bitop_range_clear_one_bitdcl(pa, 0, n);
    }
}
