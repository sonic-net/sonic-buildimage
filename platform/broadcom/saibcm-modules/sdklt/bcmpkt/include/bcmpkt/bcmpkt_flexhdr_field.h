/*! \file bcmpkt_flexhdr_field.h
 *
 *  Flexhdr field access interface.
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

#ifndef BCMPKT_FLEXHDR_FIELD_H
#define BCMPKT_FLEXHDR_FIELD_H

/*!
 * \brief Get value from a flexhdr field.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [in] flexhdr flexhdr handle.
 * \param [in] profile Flexible data profile.
 * \param [in] fid flexhdr field ID.
 * \param [out] val Field value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_flexhdr_field_get(bcmlrd_variant_t variant, uint32_t hid,
                         uint32_t *flexhdr, int profile, int fid, uint32_t *val);

/*!
 * \brief Set value into a flexhdr field.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [in,out] flexhdr flexhdr handle.
 * \param [in] profile Flexible data profile.
 * \param [in] fid flexhdr field ID.
 * \param [in] val Set value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_flexhdr_field_set(bcmlrd_variant_t variant, uint32_t hid,
                         uint32_t *flexhdr, int profile, int fid, uint32_t val);

/*!
 * \brief Get value from a flexhdr field.
 *
 * \param [in] unit Device unit number.
 * \param [in] hid flexhdr ID.
 * \param [in] flexhdr flexhdr handle.
 * \param [in] profile Flexible data profile.
 * \param [in] fid flexhdr field ID.
 * \param [out] val Field value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_flexhdr_device_field_get(int unit, uint32_t hid,
                                uint32_t *flexhdr, int profile, int fid,
                                uint32_t *val);

/*!
 * \brief Set value into a flexhdr field.
 *
 * \param [in] unit Device unit number.
 * \param [in] hid flexhdr ID.
 * \param [in,out] flexhdr flexhdr handle.
 * \param [in] profile Flexible data profile.
 * \param [in] fid flexhdr field ID.
 * \param [in] val Set value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_flexhdr_device_field_set(int unit, uint32_t hid,
                                uint32_t *flexhdr, int profile, int fid,
                                uint32_t val);

#endif /* BCMPKT_FLEXHDR_FIELD_H */
