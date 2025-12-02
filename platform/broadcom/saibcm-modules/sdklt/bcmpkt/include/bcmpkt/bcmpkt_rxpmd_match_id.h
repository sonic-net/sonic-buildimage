/*! \file bcmpkt_rxpmd_match_id.h
 *
 * RX Packet Meta Data Match ID api's.
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

#ifndef BCMPKT_RXPMD_MATCH_ID_H
#define BCMPKT_RXPMD_MATCH_ID_H

#include <sal/sal_types.h>
#include <shr/shr_types.h>
#include <bcmlrd/bcmlrd_conf.h>

/*!
 * \brief Does the match id data contain the specified type
 *
 * This routine returns the true for false for a given match id using the
 * provided match id data.
 *
 * \param [in]  variant         Variant type.
 * \param [in]  match_id_array  Match ID data.
 * \param [in]  array_len       Match ID data length in words.
 * \param [in]  match_id        Match ID.
 *
 * \retval SHR_E_NONE           The match id data contains the specified type
 * \retval SHR_E_*              The match id data does not contain the specified
 *                              type or there was an error.
 *
 */
extern int
bcmpkt_rxpmd_match_id_present(bcmlrd_variant_t variant,
                              uint32_t *match_id_array,
                              uint32_t array_len,
                              uint32_t match_id);

/*!
 * \brief Does the arc id data contain the specified type
 *
 * This routine returns the true for false for a given match id using the
 * provided arc id data.
 *
 * \param [in]  variant         Variant type.
 * \param [in]  arc_id_array    ARC ID data.
 * \param [in]  array_len       Match ID data length in words.
 * \param [in]  match_id        Match ID.
 *
 * \retval SHR_E_NONE           The arc id data contains the specified type
 * \retval SHR_E_*              The arc id data does not contain the specified
 *                              type or there was an error.
 *
 */
extern int
bcmpkt_rxpmd_match_id_from_arc_id_present(bcmlrd_variant_t variant,
                                          uint32_t *arc_id_array,
                                          uint32_t  array_len,
                                          uint32_t  match_id);

/*!
 * \brief Does the match id data contain the specified type
 *
 * This routine returns the match id value for the specified variant given the
 * match id name (string value).
 *
 * \param [in]  variant         Variant type.
 * \param [in]  name            Match ID string name.
 * \param [out] match_id        Match ID value.
 *
 * \retval SHR_E_NONE           The match id value was found for the given name
 * \retval SHR_E_*              There was an error
 *
 */
extern int
bcmpkt_rxpmd_match_id_get(bcmlrd_variant_t variant,
                          char *name,
                          uint32_t *match_id);

/*!
 * \brief Information on match ID fields.
 *
 * This structure is used to store information for each
 * match id field.
 *
 */
typedef struct bcmpkt_rxpmd_match_id_db_s {
    /*! Match ID name. */
    const char *name;

    /*! Match. */
    uint32_t match;

    /*! Mask for match. */
    uint32_t match_mask;

    /*! Maxbit of the match id field in the physical container. */
    uint8_t match_maxbit;

    /*! Minbit of the match id field in the physical container. */
    uint8_t match_minbit;

    /*! Maxbit of the match id field. */
    uint8_t maxbit;

    /*! Minbit of the match id field. */
    uint8_t minbit;

    /*! Default value for the match id field. */
    uint32_t value;

    /*! Mask for the default value for the match id field. */
    uint32_t mask;

    /*! Maxbit of the field within match_id container. */
    uint8_t pmaxbit;

    /*! Minbit of the field within match_id container. */
    uint8_t pminbit;

    /*! ARC id zone minbit. */
    uint8_t zone_minbit;

    /*! ARC id mask. */
    uint64_t arc_id_mask;

    /*! Number of words used by zone bitmap. */
    uint8_t num_zone_bmp_words;

    /*! Zone bitmap. */
    uint32_t *zone_bmp;
} bcmpkt_rxpmd_match_id_db_t;

/*!
 * \brief Information on match ID fields.
 *
 * This structure is used to store information for the match id data.
 *
 */
typedef struct bcmpkt_rxpmd_match_id_db_info_s {
    /*! Number of entries in the match ID DB. */
    uint32_t num_entries;

    /*! Pointer to match ID DB. */
    const bcmpkt_rxpmd_match_id_db_t *db;
} bcmpkt_rxpmd_match_id_db_info_t;

/*!
 * \brief Information for the match ID map.
 *
 * This structure is used to store information for the match id map.
 *
 */
typedef struct bcmpkt_rxpmd_match_id_map_info_s {
    /*! Number of entries in the match ID Map. */
    uint32_t num_entries;

    /*! Pointer to match ID Map. */
    const shr_enum_map_t *map;
} bcmpkt_rxpmd_match_id_map_info_t;

#endif /* BCMPKT_RXPMD_MATCH_ID_H */
