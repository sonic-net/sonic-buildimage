/*! \file bcmpkt_bcm78800_a0_rxpmd.h
 *
 * RX Packet Meta Data (RXPMD, called EP_TO_CPU in hardware) access interfaces.
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

#ifndef BCMPKT_BCM78800_A0_RXPMD_H
#define BCMPKT_BCM78800_A0_RXPMD_H

#include <bcmpkt/bcmpkt_flexhdr.h>

/*!
 * \brief Get flex field value from packet header data stream.
 *
 * \param [in] data Packet header data stream.
 * \param [in] fld_info Information of field within data stream.
 * \param [in] profile Profile
 * \param [out] val Field value.
 *
 * \retval SHR_E_NONE success.
 */
extern int
bcm78800_a0_rxpmd_flex_fget(uint32_t *data,
                            bcmpkt_flex_field_metadata_t *fld_info,
                            int profile,
                            uint32_t *val);

/*!
 * \brief Set flex field value from packet header data stream.
 *
 * \param [in] data Packet header data stream.
 * \param [in] fld_info Information of field within data stream.
 * \param [in] profile Profile
 * \param [in] val Field value.
 *
 * \retval SHR_E_NONE success.
 */
extern int
bcm78800_a0_rxpmd_flex_fset(uint32_t *data,
                            bcmpkt_flex_field_metadata_t *fld_info,
                            int profile,
                            uint32_t val);

#endif /* BCMPKT_BCM78800_A0_RXPMD_H */
