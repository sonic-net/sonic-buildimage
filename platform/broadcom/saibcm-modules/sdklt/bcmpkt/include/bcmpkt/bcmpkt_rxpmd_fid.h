/*! \file bcmpkt_rxpmd_fid.h
 *
 * RX Packet Meta Data (RXPMD) field id header file.
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

#ifndef BCMPKT_RXPMD_FID_H
#define BCMPKT_RXPMD_FID_H

#include <shr/shr_bitop.h>
#include <bcmpkt/bcmpkt_rxpmd_defs.h>

/*! \brief RXPMD field ID supported bit array.
 * Array of bits indicating whether a RXPMD field ID is supported by a given
 * device type.
 */
typedef struct bcmpkt_rxpmd_fid_support_s {
    /*! Field ID bitmap container */
    SHR_BITDCLNAME(fbits, BCMPKT_RXPMD_FID_COUNT);
} bcmpkt_rxpmd_fid_support_t;

/*!
 * \name Utility macros for \ref bcmpkt_rxpmd_fid_support_t.
 * \anchor BCMPKT_RXPMD_SUPPORT_OPS
 */
/*! \{ */
/*!
 * Macro to get a field ID's supported status.
 *
 * \retval zero Not supported
 * \retval non-zero Supported
 */
#define BCMPKT_RXPMD_FID_SUPPORT_GET(_support, _fid) \
        SHR_BITGET(((_support).fbits), (_fid))

/*!
 * Iterate over all supported RXPMD field IDs in the \c _support.
 */
#define BCMPKT_RXPMD_FID_SUPPORT_ITER(_support, _fid) \
    for(_fid = 0; _fid < BCMPKT_RXPMD_FID_COUNT; _fid++) \
        if(BCMPKT_RXPMD_FID_SUPPORT_GET(_support, _fid))
/*! \} */


/*!
 * \brief Get supported RXPMD field IDs for a given device type.
 *
 * This function returns a structure with information about the RXPMD field IDs
 * a given device type supports.
 *
 * Use \ref BCMPKT_RXPMD_FID_SUPPORT_GET on the returned structure to get the
 * supported status of a specific field ID.
 *
 * \param [in] dev_type Device type.
 * \param [out] support Field ID supported status bitmap.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rxpmd_fid_support_get(bcmdrd_dev_type_t dev_type,
                             bcmpkt_rxpmd_fid_support_t *support);



#endif /* BCMPKT_RXPMD_FID_H */
