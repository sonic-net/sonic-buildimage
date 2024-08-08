/*! \file bcmlrd_local_types.h
 *
 * \brief Local Logical Table Types
 *
 * This file should not depend on any other header files than the SAL
 * types. It is used for building libraries that are only a small
 * subset of the full SDK (e.g. the PMD library).
 *
 */
/*
 * $Copyright: Copyright 2018-2023 Broadcom. All rights reserved.
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
 * be found in the LICENSES folder.$
 */

#ifndef BCMLRD_LOCAL_TYPES_H
#define BCMLRD_LOCAL_TYPES_H

#include <sal/sal_types.h>

/*! Create enumeration values from list of supported variants. */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
    BCMLRD_VARIANT_T_##_bd##_##_ve,

/*! Enumeration for all device variants. */
typedef enum bcmlrd_variant_e {
    BCMLRD_VARIANT_T_NONE = 0,
/*! \cond */
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
/*! \endcond */
     BCMLRD_VARIANT_T_COUNT
} bcmlrd_variant_t;

/*!
 * \brief Information on match ID fields.
 *
 * This structure is used to store information for each
 * match id field.
 *
 */
typedef struct bcmlrd_match_id_db_s {
    /*! Match ID name. */
    const char *name;

    /*! Match. */
    uint32_t match;

    /*! Mask for match. */
    uint32_t match_mask;

    /*! Maxbit of the match id field in the physical container. */
    uint8_t match_maxbit;

    /*! Minbit of the match id field in the physical container. */
    uint8_t match_minbit;

    /*! Maxbit of the match id field. */
    uint8_t maxbit;

    /*! Minbit of the match id field. */
    uint8_t minbit;

    /*! Default value for the match id field. */
    uint32_t value;

    /*! Mask for the default value for the match id field. */
    uint32_t mask;

    /*! Maxbit of the field within match_id container. */
    uint8_t pmaxbit;

    /*! Minbit of the field within match_id container. */
    uint8_t pminbit;

    /*! ARC ID zone minbit. */
    uint8_t zone_minbit;

    /*! ARC ID mask. */
    uint64_t arc_id_mask;

    /*! Number of words used by zone bitmap. */
    uint8_t num_zone_bmp_words;

    /*! Zone bitmap. */
    uint32_t *zone_bmp;
} bcmlrd_match_id_db_t;

/*!
 * \brief Get device variant.
 *
 * Get device logical table variant,
 * which is an enumeration of all supported logical table variants.
 *
 * \param [in] unit Unit number.
 *
 * \retval Variant type.
 */
extern bcmlrd_variant_t
bcmlrd_variant_get(int unit);

/*!
 * \brief Set device variant.
 *
 * Set device logical table variant,
 * which is an enumeration of all supported logical table variants.
 *
 * \param [in] unit     Unit number.
 * \param [in] variant  BCMLRD variant enumeration.
 *
 * \retval 0  OK
 * \retval <0 ERROR
 */
extern int
bcmlrd_variant_set(int unit, bcmlrd_variant_t variant);

#endif /* BCMLRD_LOCAL_TYPES_H */
