/*! \file bcmltd_variant.h
 *
 * \brief BCMLTD Variant interfaces and definitions
 *
 * Logical table variant inclusion and exclusion support within the
 * BCMLTD can be specified as a combination of the following defines:
 *
 *     #define BCMLTD_CONFIG_INCLUDE_<CHIPNAME>_<REV>X_<VARIANT> [1|0]
 *           -- Include or exclude the specified variant
 *     Example: #define BCMLTD_CONFIG_INCLUDE_BCM56880_A0_DNA_6_5_30_1_1    1
 *
 * The value of BCMLTD_CONFIG_INCLUDE_VARIANT_DEFAULT is used for any
 * variants which are left unspecified. Set this value to 1 or 0 to
 * include or exclude all variants by default.
 *
 * BCMLTD_VARIANT_ENTRY macros.
 *
 * If a list of variant entries is needed, before including this file,
 * define BCMLTD_VARIANT_ENTRY as a macro to operate on the following
 * parameters:
 *
 *     #define BCMLTD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)
 *
 *     _bd: SW Base Driver (lower case)
 *     _bu: SW Base Driver (upper case)
 *     _va: Variant name (lower case or empty for BASE)
 *     _ve: Variant enum symbol (upper case)
 *     _vu: Variant name (underscore or empty for BASE)
 *     _vv: Variant numeric value
 *     _vo: Device relative offset
 *     _vd: Variant Description
 *     _r0: Reserved
 *     _r1: Reserved
 *
 * Note that BCMLTD_VARIANT_ENTRY will be undefined at the end of this file.
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

#ifndef BCMLTD_VARIANT_H
#define BCMLTD_VARIANT_H

#include <bcmltd/chip/bcmltd_variant_defs.h>

#endif /* BCMLTD_VARIANT_H */

/* This include must be placed outside the include guard. */
#include <bcmltd/chip/bcmltd_chip_variant.h>

