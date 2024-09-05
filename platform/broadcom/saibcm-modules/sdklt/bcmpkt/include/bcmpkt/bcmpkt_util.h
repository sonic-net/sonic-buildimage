/*! \file bcmpkt_util.h
 *
 * BCMPKT utility functions.
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

#ifndef BCMPKT_UTIL_H
#define BCMPKT_UTIL_H

#include <bcmdrd/bcmdrd_types.h>
#include <bcmlrd/bcmlrd_local_types.h>

/*!
 * \brief Get device dispatch type based on device name.
 *
 * Device name is case-insensitive.
 *
 * \param [in] dev_name Device name, e.g. "bcm56000_a0".
 *
 * \return Device dispatch type or BCMDRD_DEV_T_NONE if not found.
 */
extern bcmdrd_dev_type_t
bcmpkt_util_dev_type_get(const char *dev_name);

/*!
 * \brief Get variant dispatch type based on device and variant names.
 *
 * Device and variant names are case-insensitive.
 *
 * \param [in] dev_name Device name, e.g. "bcm56000_a0".
 * \param [in] var_name Variant name, e.g. "utltm".
 *
 * \return Variant dispatch type or BCMLRD_VARIANT_T_NONE if not found.
 */
extern bcmlrd_variant_t
bcmpkt_util_variant_type_get(const char *dev_name, const char *var_name);

#endif /* BCMPKT_UTIL_H */
