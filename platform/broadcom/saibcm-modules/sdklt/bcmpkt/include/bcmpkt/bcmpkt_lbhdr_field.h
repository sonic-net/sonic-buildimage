/*! \file bcmpkt_lbhdr_field.h
 *
 * Loopback header (LBHDR, LOOPBACK_MH in hardware) field access interface.
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

#ifndef BCMPKT_LBHDR_FIELD_H
#define BCMPKT_LBHDR_FIELD_H

/*!
 * \brief Get value from a LBHDR field.
 *
 * \param [in] dev_type Device type.
 * \param [in] lbhdr LBHDR handle.
 * \param [in] fid LBHDR field ID, refer to \ref BCMPKT_LBHDR_XXX.
 * \param [out] val Field value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_lbhdr_field_get(bcmdrd_dev_type_t dev_type, uint32_t *lbhdr,
                       int fid, uint32_t *val);

/*!
 * \brief Set value into a LBHDR field.
 *
 * \param [in] dev_type Device type.
 * \param [in,out] lbhdr LBHDR handle.
 * \param [in] fid LBHDR field ID, refer to \ref BCMPKT_LBHDR_XXX.
 * \param [in] val Set value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_lbhdr_field_set(bcmdrd_dev_type_t dev_type, uint32_t *lbhdr,
                       int fid, uint32_t val);

#endif /* BCMPKT_LBHDR_FIELD_H */

