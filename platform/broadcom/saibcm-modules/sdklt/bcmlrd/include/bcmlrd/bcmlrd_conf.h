/*! \file bcmlrd_conf.h
 *
 * \brief Public interface to access the configuration.
 *
 * This file should not depend on any other header files than the SAL
 * types. It is used for building libraries that are only a small
 * subset of the full SDK (e.g. the PMD library).
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

#ifndef BCMLRD_CONF_H
#define BCMLRD_CONF_H

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

/*!
 * \brief Return bcmlrd_variant_t enum from LTL DEVICE_VARIANT_T.
 *
 * Return a bcmlrd_variant_t enum value from a LTL DEVICE_VARIANT_T
 * symbolic value.
 *
 * The DEVICE_VARIANT_T LTL enum is based on all the devices and
 * variants present in the source tree when SDKLT logical table code
 * is generated and is numbered starting at one, assigning a unique
 * integer for each base and variant logical table configuration. The
 * bcmlrd_variant_t C enum values are determined at compile time based
 * on which BCMDRD devices are configured for a particular compile and is
 * also numbered starting at one. If a device is not enabled by
 * BCMDRD, then all of the associated bcmlrd_variant_t enum symbols
 * associated with that device are not present.
 *
 * \param [in] device_variant DEVICE_VARIANT_T symbolic value.
 *
 * \retval bcmlrd_variant_t value
 */
bcmlrd_variant_t
bcmlrd_variant_from_device_variant_t(uint64_t device_variant);

/*!
 * \brief Return LTL DEVICE_VARIANT_T from the variant_t.
 *
 * Return a LTL DEVICE_VARIANT_T symbolic value from a
 * bcmlrd_variant enum value.
 *
 * \param [in] variant bcmlrd_variant_t value.
 *
 * \retval DEVICE_VARIANT_T symbolic value.
 *
 */
uint64_t
bcmlrd_device_variant_t_from_variant(bcmlrd_variant_t variant);

/*!
 * \brief Return a bcmlrd_variant_t enum value from a given variant string.
 *
 * Return a bcmlrd_variant_t enum value from a given variant string.
 *
 * \param [in] unit Unit number.
 * \param [in] variant_string variant name.
 * \param [out] variant variant value.
 *
 * \retval 0  OK
 * \retval <0 ERROR
 */
int
bcmlrd_variant_from_variant_string(int unit,
                                   const char* variant_string,
                                   bcmlrd_variant_t* variant);

/*!
 * \brief Get base device variant.
 *
 * Get device logical table base variant, which is a set of initial
 * mappings for a device.
 *
 * \param [in] unit Unit number.
 *
 * \retval Variant type.
 */
extern bcmlrd_variant_t
bcmlrd_base_get(int unit);

/*!
 * \brief Get variant string.
 *
 * Get the variant string for the given bcmlrd_variant_t.
 *
 * \param [in] variant Variant enumeration value.
 *
 * \return Pointer to variant name.
 */
extern const char *
bcmlrd_variant_string(bcmlrd_variant_t variant);

/*!
 * \brief Get variant name.
 *
 * Get the variant name for the given unit. If the unit does not
 * exist, an empty string ("") is returned.
 *
 * \param [in] unit Unit number.
 *
 * \return Pointer to variant name.
 */
extern const char *
bcmlrd_variant_name(int unit);

#endif /* BCMLRD_CONF_H */
