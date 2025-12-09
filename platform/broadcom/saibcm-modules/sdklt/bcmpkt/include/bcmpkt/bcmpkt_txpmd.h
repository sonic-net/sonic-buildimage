/*! \file bcmpkt_txpmd.h
 *
 * TX Packet MetaData (TXPMD, called SOBMH in hardware) access interface.
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

#ifndef BCMPKT_TXPMD_H
#define BCMPKT_TXPMD_H

#include <sal/sal_types.h>
#include <shr/shr_bitop.h>
#include <bcmpkt/bcmpkt_txpmd_defs.h>
#include <bcmdrd/bcmdrd_types.h>
#include <bcmpkt/bcmpkt_txpmd_field.h>

/*! TX Packet MetaData size (bytes). */
#define BCMPKT_TXPMD_SIZE_BYTES         16
/*! TX Packet MetaData size (words). */
#define BCMPKT_TXPMD_SIZE_WORDS         4

/*! TXPMD FID field supported check. */
#define BCMPKT_TXPMD_FID_SUPPORTED(_st, _f) SHR_BITGET((_st)->fbits, _f)

/*!
 * \name TXPMD Dump flags. (deprecated by BCMPKT_DUMP_F_XXX)
 * \anchor BCMPKT_TXPMD_DUMP_F_XXX
 */
/*! \{ */
/*!
 * Dump all fields contents.
 */
#define BCMPKT_TXPMD_DUMP_F_ALL         0
/*!
 * Dump non-zero field content only.
 */
#define BCMPKT_TXPMD_DUMP_F_NONE_ZERO   1
/*! \} */

/*! \brief TXPMD field ID supported bit array.
 * Array of bits indicating whether a TXPMD field ID is supported by a given
 * device type.
 */
typedef struct bcmpkt_txpmd_fid_support_s {
    /*! Field ID bitmap container */
    SHR_BITDCLNAME(fbits, BCMPKT_TXPMD_FID_COUNT);
} bcmpkt_txpmd_fid_support_t;

/*!
 * \name Utility macros for \ref bcmpkt_txpmd_fid_support_t.
 * \anchor BCMPKT_TXPMD_SUPPORT_OPS
 */
/*! \{ */
/*!
 * Macro to get a field ID's supported status.
 *
 * \retval zero Not supported
 * \retval non-zero Supported
 */
#define BCMPKT_TXPMD_FID_SUPPORT_GET(_support, _fid) \
        SHR_BITGET(((_support).fbits), (_fid))

/*!
 * Iterate over all supported TXPMD field IDs in the \c _support.
 */
#define BCMPKT_TXPMD_FID_SUPPORT_ITER(_support, _fid) \
    for(_fid = 0; _fid < BCMPKT_TXPMD_FID_COUNT; _fid++) \
        if(BCMPKT_TXPMD_FID_SUPPORT_GET(_support, _fid))
/*! \} */

/*!
 * \brief Get TXPMD's size for a given device type.
 *
 * \param [in] dev_type Device type.
 * \param [out] len Bytes of TXPMD length.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Unsupported device type or bad \c len pointer.
 * \retval SHR_E_UNAVAIL Not support TXPMD get function.
 */
extern int
bcmpkt_txpmd_len_get(bcmdrd_dev_type_t dev_type, uint32_t *len);

/*!
 * \brief Get field name for a given TXPMD field ID.
 *
 * \param [in] fid TXPMD field ID, refer to \ref BCMPKT_TXPMD_XXX.
 * \param [out] name TXPMD field name string.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_txpmd_field_name_get(int fid, char **name);

/*!
 * \brief Get field ID for a given TXPMD field name.
 *
 * \param [in] name TXPMD name string.
 * \param [out] fid TXPMD Field ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_txpmd_field_id_get(char* name, int *fid);

/*!
 * \brief Get supported TXPMD field IDs for a given device type.
 *
 * This function returns a structure with information about the TXPMD field IDs
 * a given device type supports.
 *
 * Use \ref BCMPKT_TXPMD_FID_SUPPORT_GET on the returned structure to get the
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
bcmpkt_txpmd_fid_support_get(bcmdrd_dev_type_t dev_type,
                             bcmpkt_txpmd_fid_support_t *support);

/*!
 * \brief Get view info for a given TXPMD field ID for a given device type.
 *
 * \param [in] dev_type Device type.
 * \param [in] fid TXPMD field ID, refer to \ref BCMPKT_TXPMD_XXX.
 * \param [out] view TXPMD view info. -2 for unsupported, -1 for global,
 *              others for view's value of \ref BCMPKT_TXPMD_HEADER_TYPE_XXX.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_txpmd_fid_view_get(bcmdrd_dev_type_t dev_type,
                          int fid, int *view);

#endif /* BCMPKT_TXPMD_H */
