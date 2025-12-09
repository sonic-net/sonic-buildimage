/*! \file bcmpkt_lbhdr.h
 *
 * Loopback header (LBHDR, called LOOPBACK_MH in hardware) access interface.
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

#ifndef BCMPKT_LBHDR_H
#define BCMPKT_LBHDR_H

#include <shr/shr_bitop.h>
#include <sal/sal_types.h>
#include <bcmdrd/bcmdrd_types.h>
#include <bcmpkt/bcmpkt_lbhdr_defs.h>
#include <bcmpkt/bcmpkt_lbhdr_field.h>

/*! TX Packet MetaData size (bytes). */
#define BCMPKT_LBHDR_SIZE_BYTES         16
/*! TX Packet MetaData size (words). */
#define BCMPKT_LBHDR_SIZE_WORDS         4

/*! LBHDR FID field supported check. */
#define BCMPKT_LBHDR_FID_SUPPORTED(_st, _f) SHR_BITGET((_st)->fbits, _f)

/*!
 * \name LBHDR Dump flags. (deprecated by BCMPKT_DUMP_F_XXX)
 * \anchor BCMPKT_LBHDR_DUMP_F_XXX
 */
/*! \{ */
/*!
 * Dump all fields contents.
 */
#define BCMPKT_LBHDR_DUMP_F_ALL         0
/*!
 * Dump non-zero field content only.
 */
#define BCMPKT_LBHDR_DUMP_F_NONE_ZERO   1
/*! \} */

/*!
 * \name BCMPKT_LBHDR_START encodings.
 * \anchor BCMPKT_LBHDR_START_XXX
 */
/*! \{ */
/*!
 * Loopback header start of frame indicator's value.
 */
#define BCMPKT_LBHDR_START_IND                   251
/*! \} */

/*! \brief LBHDR field ID supported bit array.
 * Array of bits indicating whether a LBHDR field ID is supported by a given
 * device type.
 */
typedef struct bcmpkt_lbhdr_fid_support_s {
    /*! Field ID bitmap container */
    SHR_BITDCLNAME(fbits, BCMPKT_LBHDR_FID_COUNT);
} bcmpkt_lbhdr_fid_support_t;

/*!
 * \name Utility macros for \ref bcmpkt_lbhdr_fid_support_t.
 * \anchor BCMPKT_LBHDR_SUPPORT_OPS
 */
/*! \{ */
/*!
 * Macro to get a field ID's supported status.
 *
 * \retval zero Not supported
 * \retval non-zero Supported
 */
#define BCMPKT_LBHDR_FID_SUPPORT_GET(_support, _fid) \
        SHR_BITGET(((_support).fbits), (_fid))

/*!
 * Iterate over all supported LBHDR field IDs in the \c _support.
 */
#define BCMPKT_LBHDR_FID_SUPPORT_ITER(_support, _fid) \
    for(_fid = 0; _fid < BCMPKT_LBHDR_FID_COUNT; _fid++) \
        if(BCMPKT_LBHDR_FID_SUPPORT_GET(_support, _fid))
/*! \} */

/*!
 * \brief Get field name for a given LBHDR field ID.
 *
 * \param [in] fid LBHDR field ID, refer to \ref BCMPKT_LBHDR_XXX.
 * \param [out] name LBHDR field name string.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_lbhdr_field_name_get(int fid, char **name);

/*!
 * \brief Get field ID for a given LBHDR field name.
 *
 * \param [in] name LBHDR field name string.
 * \param [out] fid LBHDR Field ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_lbhdr_field_id_get(char* name, int *fid);

/*!
 * \brief Get supported LBHDR field IDs for a given device type.
 *
 * This function returns a structure with information about the LBHDR field IDs
 * a given device type supports.
 *
 * Use \ref BCMPKT_LBHDR_FID_SUPPORT_GET on the returned structure to get the
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
bcmpkt_lbhdr_fid_support_get(bcmdrd_dev_type_t dev_type,
                             bcmpkt_lbhdr_fid_support_t *support);

#endif /* BCMPKT_LBHDR_H */
