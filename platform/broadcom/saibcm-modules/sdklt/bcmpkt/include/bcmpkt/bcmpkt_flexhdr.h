/*! \file bcmpkt_flexhdr.h
 *
 *  Flexhdr access interface.
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

#ifndef BCMPKT_FLEXHDR_H
#define BCMPKT_FLEXHDR_H

#include <shr/shr_bitop.h>
#include <sal/sal_types.h>
#include <bcmlrd/bcmlrd_conf.h>
#include <bcmdrd/bcmdrd_types.h>
#include <bcmpkt/bcmpkt_pmd.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>
#include <bcmpkt/bcmpkt_flexhdr_field.h>

/*! Invalid profile ID. */
#define BCMPKT_FLEXHDR_PROFILE_NONE    -1

/*! Max profile count. */
#define BCMPKT_FLEXHDR_PROFILE_MAX     128

/*! Max profile count. */
#define BCMPKT_FLEXHDR_PROFILE_BITMAP_MAX     4

/*! CELL Error status bitmap. */
#define BCMPKT_RXFLEXMETA_ST_CELL_ERROR (0x1 << 18)

/*!
 * \name Packet FLEX reason utility macros.
 * \anchor BCMPKT_RXPMD_FLEX_REASON_OPS
 */
/*! \{ */
/*!
 * Macro to check if a reason is included in a
 * set of reasons (\ref bcmpkt_bitmap_t). Returns:
 *   zero     => reason is not included in the set
 *   non-zero => reason is included in the set
 */
#define BCMPKT_RXPMD_FLEX_REASON_GET(_reasons, _reason) \
        SHR_BITGET(((_reasons).pbits), (_reason))

/*!
 * Macro to add a reason to a set of
 * reasons (\ref bcmpkt_bitmap_t)
 */
#define BCMPKT_RXPMD_FLEX_REASON_SET(_reasons, _reason) \
        SHR_BITSET(((_reasons).pbits), (_reason))

/*!
 * Macro to clear a reason from a set of
 * reasons (\ref bcmpkt_bitmap_t)
 */
#define BCMPKT_RXPMD_FLEX_REASON_CLEAR(_reasons, _reason) \
        SHR_BITCLR(((_reasons).pbits), (_reason))

/*!
 * Macro to add all reasons to a set of reasons.
 */
#define BCMPKT_RXPMD_FLEX_REASON_SET_ALL(_reasons, _count) \
        SHR_BITSET_RANGE(((_reasons).pbits), 0, _count)

/*!
 * Macro to clear all reasons.
 */
#define BCMPKT_RXPMD_FLEX_REASON_CLEAR_ALL(_reasons, _count) \
        SHR_BITCLR_RANGE(((_reasons).pbits), 0, _count)

/*!
 * Macro to check for no reason.
 */
#define BCMPKT_RXPMD_FLEX_REASON_IS_NULL(_reasons, _count) \
        SHR_BITNULL_RANGE(((_reasons).pbits), 0, _count)

/*!
 * Macro to get reasons number.
 */
#define BCMPKT_RXPMD_FLEX_REASONS_COUNT(_reasons, _count, _reason_count) \
        SHR_BITCOUNT_RANGE(((_reasons).pbits), _count, 0, _reason_count)

/*!
 * Macro to compare 2 reasons, return 1 for exact match.
 */
#define BCMPKT_RXPMD_FLEX_REASON_EQ(_reasons1, _reasons2, _count) \
        SHR_BITEQ_RANGE(((_reasons1).pbits), ((_reasons2).pbits), \
                        0, _count)
/*! \} */

/*!
 * Flex header field profile info.
 */
typedef struct bcmpkt_flex_field_profile_s {
    /*! Minbit in NPL header. */
    uint32_t minbit;

    /*! Maxbit in NPL header. */
    uint32_t maxbit;
} bcmpkt_flex_field_profile_t;

/*!
 * Flex header field data.
 */
typedef struct bcmpkt_flex_field_metadata_s {
    /*! Field name. */
    char *name;

    /*! Field ID. */
    int fid;

    /*! Number of profiles defined in NPL. */
    int profile_cnt;

    /*! Field boundary for each profile defined in NPL. */
    bcmpkt_flex_field_profile_t profile[BCMPKT_FLEXHDR_PROFILE_MAX];
} bcmpkt_flex_field_metadata_t;

/*!
 * Flex header field info structure.
 */
typedef struct bcmpkt_flex_field_info_s {

    /*! Number of header fields. */
    int num_fields;

    /*! Header field names. */
    bcmpkt_flex_field_metadata_t *info;

    /*! Profile bitmap count. */
    int profile_bmp_cnt;

    /*! Profile bitmap. */
    uint32_t profile_bmp[BCMPKT_FLEXHDR_PROFILE_BITMAP_MAX];
} bcmpkt_flex_field_info_t;

/*! RXPMD data update function pointer. */
typedef int (*bcmpkt_rxpmd_data_set_f)(
    int unit,
    bcmpkt_flex_field_metadata_t *pmd_fld_info);

/*! Process RXPMD entry. */
typedef int (*bcmpkt_rxpmd_data_process_f)(int unit, uint64_t prof_id);

/*! Update RXMPMD data from HW during warmboot. */
typedef int (*bcmpkt_rxpmd_data_update_f)(int unit);

/*! Array of RXPMD LT subscribe function pointers. */
typedef struct bcmpkt_rxpmd_func_s {
    /*!  Set RXMPMD data. */
    bcmpkt_rxpmd_data_set_f rxpmd_data_set;

    /*!  Process RXMPMD data flow entry. */
    bcmpkt_rxpmd_data_process_f rxpmd_data_flow;

    /*!  Process RXMPMD data remap entry. */
    bcmpkt_rxpmd_data_process_f rxpmd_data_remap;

    /*! Update RXMPMD data from HW during warmboot. */
    bcmpkt_rxpmd_data_update_f rxpmd_data_update;
} bcmpkt_rxpmd_func_t;

/*! Externs for the rxpmd functions. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_func_t _bd##_rxpmd_func;
#include <bcmdrd/bcmdrd_devlist.h>

/*!
 * \brief Get Header name for a given header ID.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [out] name flexhdr name string.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_flexhdr_header_name_get(bcmlrd_variant_t variant,
                               uint32_t hid, char **name);

/*!
 * \brief Get Header encapsulation length for a given header ID.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [out] len header length.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_flexhdr_len_get(bcmlrd_variant_t variant, uint32_t hid,
                       uint32_t *len);

/*!
 * \brief Get Header ID for a given flexhdr name.
 *
 * \param [in] variant Variant type.
 * \param [in] name flexhdr name string.
 * \param [out] hid flexhdr ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_flexhdr_header_id_get(bcmlrd_variant_t variant,
                             char* name, uint32_t *hid);

/*!
 * \brief Check if flexhdr is supported.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [out] is_supported Supported for flexhdr.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not supported.
 */
extern int
bcmpkt_flexhdr_is_supported(bcmlrd_variant_t variant, uint32_t hid,
                            bool *is_supported);

/*!
 * \brief Get field name for a given flexhdr field ID.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [in] fid flexhdr field ID.
 * \param [out] name flexhdr field name string.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_flexhdr_field_name_get(bcmlrd_variant_t variant, uint32_t hid,
                              int fid, char **name);

/*!
 * \brief Get field ID for a given flexhdr field name.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [in] name flexhdr field name string.
 * \param [out] fid flexhdr Field ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_flexhdr_field_id_get(bcmlrd_variant_t variant, uint32_t hid,
                            char* name, int *fid);

/*!
 * \brief Get field info for a given flexhdr type.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [out] info field information.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_flexhdr_field_info_get(bcmlrd_variant_t variant, uint32_t hid,
                              bcmpkt_flex_field_info_t *info);

/*!
 * \brief Get RX reasons from RXPMD_FLEX.
 *
 * Decode packet's RX reasons into "reasons". A received packet may have one RX
 * reason, multiple RX reasons, or none reason. RX reasons are in the format of
 * bitmap. Each bit means one reason type (refer to \ref BCMPKT_RX_REASON_XXX).
 *
 * User may use \ref BCMPKT_RXPMD_FLEX_REASON_OPS to parse each individual reason based
 * on this function's return value "reasons".
 *
 * \param [in] variant Variant type.
 * \param [in] rxpmd_flex RXPMD_FLEX handle.
 * \param [out] reasons RX reasons in bit array.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support Reason.
 * \retval SHR_E_INTERNAL Internal issue.
 */
extern int
bcmpkt_rxpmd_flex_reasons_get(bcmlrd_variant_t variant, uint32_t *rxpmd_flex,
                              bcmpkt_bitmap_t  *reasons);

/*!
 * \brief Set RX reasons into the RXPMD_FLEX. (Internally used for filter configuration.)
 *
 * Set RX reasons into RXPMD_FLEX data for packet filter purpose.
 *
 * \param [in] variant Variant type.
 * \param [in] reasons Reasons bit array.
 * \param [in,out] rxpmd_flex RXPMD_FLEX handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support Reason.
 * \retval SHR_E_INTERNAL Internal issue.
 */
extern int
bcmpkt_rxpmd_flex_reasons_set(bcmlrd_variant_t variant,
                              bcmpkt_bitmap_t *reasons, uint32_t *rxpmd_flex);

/*!
 * \brief Get an RX reason's name.
 *
 * \param [in] variant Variant type.
 * \param [in] reason Reason ID.
 * \param [out] name Reason name string handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_rxpmd_flex_reason_name_get(bcmlrd_variant_t variant,
                                  int reason, char **name);

/*!
 * \brief Get max number of RX reason types.
 *
 * \param [in] variant Variant type.
 * \param [out] num Maximum RX reason types.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_rxpmd_flex_reason_max_get(bcmlrd_variant_t variant, uint32_t *num);

/*!
 * \brief Get reason ID for a given RX reason name.
 *
 * \param [in] variant Variant type.
 * \param [in] name Reason name string handle.
 * \param [out] rid Reason ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_rxpmd_flex_reason_id_get(bcmlrd_variant_t variant,
                                char* name, int *rid);

/*!
 * \brief Intialize RXPMD module
 *
 * \param [in] unit Device ID.
 * \param [in] warm Warmboot flag.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNIT Incorrect unit.
 */
extern int
bcmpkt_flexhdr_init(int unit, bool warm);

/*!
 * \brief Cleanup RXPMD module
 *
 * \param [in] unit Device ID.
 *
 * \retval SHR_E_NONE success.
 */
extern int
bcmpkt_flexhdr_cleanup(int unit);

#endif /* BCMPKT_FLEXHDR_H */
