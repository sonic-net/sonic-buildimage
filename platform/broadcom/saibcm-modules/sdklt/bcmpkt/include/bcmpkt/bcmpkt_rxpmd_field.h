/*! \file bcmpkt_rxpmd_field.h
 *
 * RX Packet MetaData (RXPMD, called EP_TO_CPU in hardware) field api's.
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

#ifndef BCMPKT_RXPMD_FIELD_H
#define BCMPKT_RXPMD_FIELD_H


/*!
 * \brief Get value from an RXPMD field.
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd RXPMD handle.
 * \param [in] fid RXPMD field ID, refer to \ref BCMPKT_RXPMD_XXX.
 * \param [out] val Field value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_rxpmd_field_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                       int fid, uint32_t *val);

/*!
 * \brief Set value into an RXPMD field. (Internally used for filter config.)
 *
 * \param [in] dev_type Device type.
 * \param [in,out] rxpmd RXPMD handle.
 * \param [in] fid RXPMD field ID, refer to \ref BCMPKT_RXPMD_XXX.
 * \param [in] val Set value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_rxpmd_field_set(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                       int fid, uint32_t val);

/*!
 * \brief Get flex data handle from the RXPMD.
 *
 * This function is used for geting flex data handle from the \c rxpmd.
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd RXPMD handle.
 * \param [out] flexdata Flex data handle.
 * \param [out] len Flex data size in 4-bytes.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support flex data.
 * \retval SHR_E_INTERNAL Internal issue.
 */
extern int
bcmpkt_rxpmd_flexdata_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                          uint32_t **flexdata, uint32_t *len);


#endif /* BCMPKT_RXPMD_FIELD_H */

