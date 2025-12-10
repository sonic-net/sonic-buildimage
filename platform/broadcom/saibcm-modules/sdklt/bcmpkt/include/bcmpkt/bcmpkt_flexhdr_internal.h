/*! \file bcmpkt_flexhdr_internal.h
 *
 * \brief Flex Packet MetaData internal library.
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

#ifndef BCMPKT_FLEXHDR_INTERNAL_H
#define BCMPKT_FLEXHDR_INTERNAL_H

#include <shr/shr_types.h>
#include <bcmpkt/bcmpkt_flexhdr.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>

/*! PMD types.
 * This has to match the header IDs present in
 * xfc_map_parser/hdr/header_map.yml file.
 */
/*! Generic loopback header type */
#define BCMPKT_GENERIC_LOOPBACK_T       0
/*! Higig 3 header type */
#define BCMPKT_HG3_BASE_T               1
/*! Higig3 extension 0 header type */
#define BCMPKT_HG3_EXTENSION_0_T        2
/*! RXPMD flex header type */
#define BCMPKT_RXPMD_FLEX_T             3
/*! Count of PMD types */
#define BCMPKT_PMD_COUNT                4

/*! Get a flex field from a PMD buffer. */
typedef int32_t (*bcmpkt_flex_field_get_f)(uint32_t *data, int profile, uint32_t *val);

/*! Set a flex field within a PMD buffer. */
typedef int32_t (*bcmpkt_flex_field_set_f)(uint32_t *data, int profile, uint32_t val);

/*! Decode flex packet's RX reasons. */
typedef void (*bcmpkt_flex_reason_decode_f) (uint32_t *data, bcmpkt_bitmap_t *reasons);

/*! Encode flex packet's RX reasons */
typedef void (*bcmpkt_flex_reason_encode_f) (bcmpkt_bitmap_t *reasons, uint32_t *data);

/*! Get a flex field from a PMD buffer. */
typedef int (*bcmpkt_flex_field_common_get_f)(
    uint32_t *data,
    bcmpkt_flex_field_metadata_t *fld_info,
    int profile,
    uint32_t *val);

/*! Set a flex field from a PMD buffer. */
typedef int (*bcmpkt_flex_field_common_set_f)(
    uint32_t *data,
    bcmpkt_flex_field_metadata_t *fld_info,
    int profile,
    uint32_t val);

/*!
 * \brief Flex Packet reasons information structure.
 */
typedef struct bcmpkt_flex_reasons_info_s {
    /*! Number of reasons supported. */
    int num_reasons;

    /*! Reason names. */
    shr_enum_map_t *reason_names;

    /*! Encode RX reasons */
    bcmpkt_flex_reason_encode_f reason_encode;

    /*! Decode RX reasons */
    bcmpkt_flex_reason_decode_f reason_decode;

} bcmpkt_flex_reasons_info_t;

/*!
 * \brief Flex Packet metadata information structure.
 */
typedef struct bcmpkt_flex_pmd_info_s {

    /*! Header field info. */
    bcmpkt_flex_field_info_t *field_info;

    /*! Header support */
    bool is_supported;

    /*! Flex reasons info */
    bcmpkt_flex_reasons_info_t *reasons_info;

    /*! Flex field get functions. */
    bcmpkt_flex_field_get_f *flex_fget;

    /*! Flex field set functions. */
    bcmpkt_flex_field_set_f *flex_fset;

    /*! Flex field common get functions. */
    bcmpkt_flex_field_common_get_f flex_common_fget;

    /*! Flex field common set functions. */
    bcmpkt_flex_field_common_set_f flex_common_fset;
} bcmpkt_flex_pmd_info_t;

/*! \cond  Externs for the required functions. */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1) \
    extern bcmpkt_flex_pmd_info_t * _bd##_vu##_va##_flex_pmd_info_get(uint32_t hid);
#define BCMLRD_VARIANT_OVERRIDE
#include <bcmlrd/chip/bcmlrd_chip_variant.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern bcmpkt_flex_pmd_info_t * _bc##_flex_pmd_info_get(uint32_t hid);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)   \
    extern shr_enum_map_t * _bd##_vu##_va##_flexhdr_map_get(void);
#define BCMLRD_VARIANT_OVERRIDE
#include <bcmlrd/chip/bcmlrd_chip_variant.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern shr_enum_map_t * _bc##_flexhdr_map_get(void);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)   \
    extern int _bd##_vu##_va##_flexhdr_variant_support_map[BCMPKT_PMD_COUNT];
#define BCMLRD_VARIANT_OVERRIDE
#include <bcmlrd/chip/bcmlrd_chip_variant.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern int _bc##_flexhdr_variant_support_map[BCMPKT_PMD_COUNT];
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>
/*! \endcond */

/*!
 * \brief Get flex header support mapping for a given variant.
 *
 * \param [in] variant Variant type.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int *
bcmpkt_flexhdr_support_map_get(bcmlrd_variant_t variant);

#endif /* BCMPKT_FLEXHDR_INTERNAL_H */
