/*! \file bcmltd_id_types.h
 *
 * Logical Table Data ID Types header file
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

#ifndef BCMLTD_ID_TYPES_H
#define BCMLTD_ID_TYPES_H

/*!
 * \brief Logical table ID type.
 */
typedef uint32_t bcmltd_sid_t;

/*!
 * \brief Logical field ID type.
 */
typedef uint32_t bcmltd_fid_t;

/*!
 * \brief Global logical field ID type.
 */
typedef bcmltd_fid_t bcmltd_gfid_t;

/*!
 * \brief Invalid logical table ID.
 */
#define BCMLTD_SID_INVALID       ((bcmltd_sid_t)-1)

/*!
 * \brief Invalid logical table ID
 *
 * To store invalid LTID in HA for ISSU upgrade,
 * this invalid LTID has to be used.
 */
#define BCMLTD_INVALID_LT       BCMLTD_SID_INVALID

/*!
 * \brief Invalid logical field ID.
 */
#define BCMLTD_FID_INVALID       ((bcmltd_fid_t)-1)

#endif /* BCMLTD_ID_TYPES_H */
