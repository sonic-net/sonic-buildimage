/*! \file bcmpkt_rxpmd.h
 *
 * RX Packet Meta Data (RXPMD, called EP_TO_CPU in hardware) access interfaces.
 *
 */
/*
 * Copyright 2018-2024 Broadcom. All rights reserved.
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

#ifndef BCMPKT_RXPMD_H
#define BCMPKT_RXPMD_H

#include <sal/sal_types.h>
#include <shr/shr_bitop.h>
#include <bcmpkt/bcmpkt_rxpmd_defs.h>
#include <bcmdrd/bcmdrd_types.h>
#include <bcmlrd/bcmlrd_conf.h>
#include <bcmlrd/bcmlrd_match_id_db.h>
#include <bcmpkt/bcmpkt_rxpmd_field.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id.h>
#include <bcmpkt/bcmpkt_rxpmd_fid.h>

/*! RX raw packet metadata maximum size (words). */
#define BCMPKT_RXPMD_SIZE_WORDS         18

/*! RX raw packet metadata maximum size (bytes). */
#define BCMPKT_RXPMD_SIZE_BYTES         (BCMPKT_RXPMD_SIZE_WORDS * 4)

/*! CELL Error status bitmap. */
#define BCMPKT_RXMETA_ST_CELL_ERROR     (0x1 << 18)

/*! RXPMD FID field supported check. */
#define BCMPKT_RXPMD_FID_SUPPORTED(_st, _f) SHR_BITGET((_st)->fbits, _f)

/*! \brief Packet reasons bitmap.
 * Set of "reasons" (\ref BCMPKT_RX_REASON_XXX) why a packet came to the CPU.
 */
typedef struct bcmpkt_rx_reasons_s {
    /*! Bitmap container */
    SHR_BITDCLNAME(pbits, BCMPKT_RX_REASON_COUNT);
} bcmpkt_rx_reasons_t;

/*!
 * \name RXPMD Dump flags. (deprecated by BCMPKT_DUMP_F_XXX)
 * \anchor BCMPKT_RXPMD_DUMP_F_XXX
 */
/*! \{ */
/*!
 * Dump all fields contents.
 */
#define BCMPKT_RXPMD_DUMP_F_ALL         0
/*!
 * Dump non-zero field content only.
 */
#define BCMPKT_RXPMD_DUMP_F_NONE_ZERO   1
/*! \} */

/*!
 * \name Packet RX reason utility macros.
 * \anchor BCMPKT_RX_REASON_OPS
 */
/*! \{ */
/*!
 * Macro to check if a reason (\ref BCMPKT_RX_REASON_XXX) is included in a
 * set of reasons (\ref bcmpkt_rx_reasons_t). Returns:
 *   zero     => reason is not included in the set
 *   non-zero => reason is included in the set
 */
#define BCMPKT_RX_REASON_GET(_reasons, _reason) \
        SHR_BITGET(((_reasons).pbits), (_reason))

/*!
 * Macro to add a reason (\ref BCMPKT_RX_REASON_XXX) to a set of
 * reasons (\ref bcmpkt_rx_reasons_t)
 */
#define BCMPKT_RX_REASON_SET(_reasons, _reason) \
        SHR_BITSET(((_reasons).pbits), (_reason))

/*!
 * Macro to add all reasons (\ref BCMPKT_RX_REASON_XXX) to a set of
 * reasons (\ref bcmpkt_rx_reasons_t)
 */
#define BCMPKT_RX_REASON_SET_ALL(_reasons) \
        SHR_BITSET_RANGE(((_reasons).pbits), 0, BCMPKT_RX_REASON_COUNT)

/*!
 * Macro to clear a reason (\ref BCMPKT_RX_REASON_XXX) from a set of
 * reasons (\ref bcmpkt_rx_reasons_t)
 */
#define BCMPKT_RX_REASON_CLEAR(_reasons, _reason) \
        SHR_BITCLR(((_reasons).pbits), (_reason))

/*!
 * Macro to clear a set of reasons (\ref bcmpkt_rx_reasons_t).
 */
#define BCMPKT_RX_REASON_CLEAR_ALL(_reasons) \
        SHR_BITCLR_RANGE(((_reasons).pbits), 0, BCMPKT_RX_REASON_COUNT)
/*!
 * Macro to check for no reason (\ref bcmpkt_rx_reasons_t).
 */
#define BCMPKT_RX_REASON_IS_NULL(_reasons) \
        SHR_BITNULL_RANGE(((_reasons).pbits), \
                          0, BCMPKT_RX_REASON_COUNT)

/*!
 * Macro to iterate every reason (\ref bcmpkt_rx_reasons_t).
 */
#define BCMPKT_RX_REASON_ITER(_reasons, reason) \
    for(reason = BCMPKT_RX_REASON_NONE; reason < (int)BCMPKT_RX_REASON_COUNT; reason++) \
        if(BCMPKT_RX_REASON_GET(_reasons, reason))

/*!
 * Macro to get reasons number (\ref bcmpkt_rx_reasons_t).
 */
#define BCMPKT_RX_REASONS_COUNT(_reasons, _count) \
        SHR_BITCOUNT_RANGE(((_reasons).pbits), _count, \
                           0, BCMPKT_RX_REASON_COUNT)

/*!
 * Macro to compare 2 reasons (\ref bcmpkt_rx_reasons_t), return 1 for exact match.
 */
#define BCMPKT_RX_REASON_EQ(_reasons1, _reasons2) \
        SHR_BITEQ_RANGE(((_reasons1).pbits), ((_reasons2).pbits), \
                        0, BCMPKT_RX_REASON_COUNT)
/*! \} */


/*!
 * \brief Get RXPMD's size for a given device type.
 *
 * \param [in] dev_type Device type.
 * \param [out] len Bytes of RXPMD length.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Unsupported device type or bad \c len pointer.
 * \retval SHR_E_UNAVAIL Not support RXPMD get function.
 */
extern int
bcmpkt_rxpmd_len_get(bcmdrd_dev_type_t dev_type, uint32_t *len);


/*!
 * \brief Get module header's pointer of the RXPMD.
 *
 * This function is used for geting Module header's pointer in RXPMD.
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd RXPMD handle.
 * \param [out] hg_hdr HiGig header handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support HiGig header.
 * \retval SHR_E_INTERNAL Internal issue.
 */
extern int
bcmpkt_rxpmd_mh_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                    uint32_t **hg_hdr);

/*!
 * \brief Get RX reasons from RXPMD.
 *
 * Decode packet's RX reasons into "reasons". A received packet may have one RX
 * reason, multiple RX reasons, or none reason. RX reasons are in the format of
 * bitmap. Each bit means one reason type (refer to \ref BCMPKT_RX_REASON_XXX).
 *
 * User may use \ref BCMPKT_RX_REASON_OPS to parse each individual reason based
 * on this function's return value "reasons".
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd RXPMD handle.
 * \param [out] reasons RX reasons in bit array.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support Reason.
 * \retval SHR_E_INTERNAL Internal issue.
 */
extern int
bcmpkt_rxpmd_reasons_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                         bcmpkt_rx_reasons_t  *reasons);

/*!
 * \brief Set RX reasons into the RXPMD. (Internally used for filter configuration.)
 *
 * Set RX reasons into RXPMD data for packet filter purpose.
 *
 * \param [in] dev_type Device type.
 * \param [in] reasons Reasons bit array.
 * \param [in,out] rxpmd RXPMD handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support Reason.
 * \retval SHR_E_INTERNAL Internal issue.
 */
extern int
bcmpkt_rxpmd_reasons_set(bcmdrd_dev_type_t dev_type,
                         bcmpkt_rx_reasons_t *reasons, uint32_t *rxpmd);

/*!
 * \brief Get field name for a given RXPMD field ID.
 *
 * \param [in] fid RXPMD field ID, refer to \ref BCMPKT_RXPMD_XXX.
 * \param [out] name RXPMD field name string.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_rxpmd_field_name_get(int fid, char **name);

/*!
 * \brief Get field ID for a given RXPMD field name.
 *
 * \param [in] name RXPMD field name string.
 * \param [out] fid RXPMD Field ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_rxpmd_field_id_get(char* name, int *fid);

/*!
 * \brief Get an RX reason's name.
 *
 * \param [in] reason Reason ID.
 * \param [out] name Reason name string handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_rx_reason_name_get(int reason, char **name);

/*!
 * \brief Return the RXPMD match id information.
 *
 * This routine returns the RXPMD match id information
 * for the given match id name.
 *
 * \param [in]  variant         Variant type.
 * \param [in]  spec            Match ID name.
 * \param [out] info            Match ID data.
 *
 * \retval SHR_E_NONE           Success.
 * \retval SHR_E_PARAM          Invalid variant, spec or info.
 * \retval SHR_E_UNAVAIL        Match ID data is not available.
 *
 */
extern int
bcmpkt_rxpmd_match_id_data_get(bcmlrd_variant_t variant, const char *spec,
                               const bcmlrd_match_id_db_t **info);

#endif /* BCMPKT_RXPMD_H */
