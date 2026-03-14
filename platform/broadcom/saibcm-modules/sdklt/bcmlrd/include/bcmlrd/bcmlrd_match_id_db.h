/*! \file bcmlrd_match_id_db.h
 *
 * \brief Match ID DB data structures and APIs.
 *
 * This file constains the collection of
 * Match ID DB related data structures and APIs.
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

#ifndef BCMLRD_MATCH_ID_DB_H
#define BCMLRD_MATCH_ID_DB_H

#include <sal/sal_types.h>
#include <bcmlrd/bcmlrd_id_types.h>

/*!
 * \brief Information on match ID fields.
 *
 * This structure is used to store information for each
 * match id field.
 *
 */
typedef struct bcmlrd_match_id_db_s {
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

    /*! ARC ID zone minbit. */
    uint8_t zone_minbit;

    /*! ARC ID mask. */
    uint64_t arc_id_mask;

    /*! Number of words used by zone bitmap. */
    uint8_t num_zone_bmp_words;

    /*! Zone bitmap. */
    uint32_t *zone_bmp;
} bcmlrd_match_id_db_t;

/*!
 * \brief Information on match ID fields.
 *
 * This structure is used to store information match id data.
 *
 */
typedef struct bcmlrd_match_id_db_info_s {
    /*! Number of entries in the match ID DB. */
    uint32_t num_entries;

    /*! Pointer to match ID DB. */
    const bcmlrd_match_id_db_t *db;
} bcmlrd_match_id_db_info_t;

/*!
 * \brief Function pointer to retrieve the match id information.
 */
typedef int (*bcmlrd_match_id_db_get_t)(int unit, const bcmlrd_sid_t sid,
                                 const bcmlrd_fid_t fid,
                                 const bcmlrd_match_id_db_t **info);

/*!
 * \brief Information on physical containers.
 *
 * This structure is used to store information for each
 * physical container that a logical field is mapped to.
 *
 */
typedef struct bcmlrd_cont_info_s {
    /*! Section in which the container is available. */
    uint8_t section_id;

    /*! Offset of the container within the section. */
    uint16_t cont_id;

    /*! Width of the container in the section. */
    uint8_t width;

    /*! Bit offset of the container within the section. */
    uint16_t bit_offset;
} bcmlrd_cont_info_t;

/*!
 * \brief PDD information for physical containers.
 *
 * This structure is used to store PDD information for each
 * physical container that a logical field is mapped to.
 *
 */
typedef struct bcmlrd_pdd_info_s {
    /*!
     * Physical container id. This is the bit id of
     * the physical container in the PDD bitmap.
     */
    uint16_t phy_cont_id;

    /*!
     * SBR Physical container id. This is the bit id of
     * the physical container in the SBR bitmap.
     */
    uint16_t sbr_phy_cont_id;

    /*! Physical container size. */
    uint16_t phy_cont_size;

    /*! Offset of action in the physical container. */
    uint8_t offset;

    /*! Width of action in the physical container from the offset */
    uint8_t width;

    /*! MFAP_INDEX to represend order of containers in contiguous */
    uint8_t mfap_index;

    /*! If set, then PDD is aligned from LSB. */
    bool is_lsb;

    /*! Absolute offset of container in the container list. */
    uint16_t bit_offset;
} bcmlrd_pdd_info_t;

/*!
 * \brief Container information per logical field.
 *
 * This structure is used to maintain the container information
 * per logical field.
 *
 * Each logical field can be mapped to multiple containers.
 * In which case, the information would be available as
 * an array of this structure.
 * Count specifies the array length.
 *
 */
typedef struct bcmlrd_field_cont_info_s {
    /*! Number of instances that physical container is mapped in the TILE. */
    uint8_t instances;

    /*! Number of containers that logical field is mapped to. */
    uint8_t count;

    /*! Physical container information. */
    const bcmlrd_cont_info_t *info;
} bcmlrd_field_cont_info_t;

/*!
 * \brief SBR type.
 */
typedef enum bcmlrd_field_sbr_type_e {
    /*! Non SBR eligible action. */
    BCMLRD_SBR_NONE,

    /*! Non SBR eligible action, mapped to SBR container. */
    BCMLRD_SBR_INTERNAL,

    /*! SBR eligible action, mapped to SBR container. */
    BCMLRD_SBR_EXTERNAL
} bcmlrd_field_sbr_type_t;

/*!
 * \brief PDD information on containers per logical field.
 *
 * This structure is used to maintain the PDD information for containers
 * per logical field.
 *
 * Each logical field can be mapped to multiple containers.
 * In which case, the information would be available as
 * an array of this structure.
 * Count specifies the array length.
 *
 */
typedef struct bcmlrd_field_pdd_info_s {
    /*!  SBR type of the field. */
    bcmlrd_field_sbr_type_t sbr_type;

    /*! Number of containers that logical field is mapped to. */
    uint8_t count;

    /*! PDD information for each physical container. */
    const bcmlrd_pdd_info_t *info;
} bcmlrd_field_pdd_info_t;

/*!
 * \brief Container map information for logical field.
 *
 * This structure provides container and PDD information for
 * each physical container that the logical field is mapped to.
 *
 */
typedef struct bcmlrd_field_info_s {
    /*! Name of the physical field. */
    const char *name;

   /*! Field ID. */
   bcmltd_fid_t id;

   /*! Container information for the logical field. */
   const bcmlrd_field_cont_info_t *cont_info;

   /*! PDD container information for the logical field. */
   const bcmlrd_field_pdd_info_t *pdd_info;

} bcmlrd_field_info_t;

/*!
 * \brief Table tile information for the special tables.
 *
 * This structure provides physical container information for each
 * logical field in the tile mapped to the table.
 *
 */

typedef struct bcmlrd_tile_pcm_info_s {
    /*! Mux information for this logical table. */
    uint32_t tile_id;

    /*! Number of fields in the table. */
    uint16_t field_count;

    /*! Field information for each field. */
    const bcmlrd_field_info_t *field_info;

} bcmlrd_tile_pcm_info_t;

/*!
 * \brief Table information for the special tables.
 *
 * This structure provides physical container information for each
 * logical field.
 *
 */
typedef struct bcmlrd_table_pcm_info_s {
    /*! Logical Table source ID. */
    uint32_t src_id;

    /*! Number of tiles in the table. */
    uint8_t tile_count;

    /*! Tile PCM information for each field. */
    const bcmlrd_tile_pcm_info_t *tile_info;

} bcmlrd_table_pcm_info_t;

/*!
 * \brief Table PCM configuration storage compact representation.
 */
typedef struct bcmlrd_pcm_conf_compact_rep_s {
    /*! PCM configuration name. */
    const char *name;

    /*! Number of tables that support PCM in the device. */
    uint32_t num_pcm;

    /*! Pointer to the array of PCM configurations. */
    const bcmlrd_table_pcm_info_t **pcm;

} bcmlrd_pcm_conf_compact_rep_t;

/*!
 * \brief Table PCM configuration storage representation.
 */
typedef bcmlrd_pcm_conf_compact_rep_t bcmlrd_pcm_conf_rep_t;

/*!
 * \brief Return the PCM configuration for the given table.
 *
 * This routine returns the PCM configuration
 * for the given unit, sid.
 *
 * \param [in]  unit            Unit number.
 * \param [in]  sid             Logical Table symbol ID.
 * \param [out] pcm_info        PCM configuration.
 *
 * \retval SHR_E_NONE           Success.
 * \retval SHR_E_UNAVAIL        Unit/Table/PCM configuration not found.
 *
 */
extern int
bcmlrd_table_pcm_conf_get(int unit,
                          bcmlrd_sid_t sid,
                          const bcmlrd_table_pcm_info_t **pcm_info);

/*!
 * \brief Return the match id information.
 *
 * This routine returns the match id information
 * for the given unit, table and field.
 *
 * \param [in]  unit            Unit number.
 * \param [in]  sid             Logical Table symbol ID.
 * \param [in]  fid             Logical field symbol ID.
 * \param [out] info        Match ID data.
 *
 * \retval SHR_E_NONE           Success.
 * \retval SHR_E_PARAM          Invalid unit, sid, fid or info.
 * \retval SHR_E_UNAVAIL        Match ID data is not found.
 *
 */
extern int
bcmlrd_table_match_id_db_get(int unit,
                             const bcmlrd_sid_t sid,
                             const bcmlrd_fid_t fid,
                             const bcmlrd_match_id_db_t **info);

/*!
 * \brief Return the match id information.
 *
 * This routine returns the match id information
 * for the given match id in string format.
 *
 * \param [in]  unit            Unit number.
 * \param [in]  spec            Match ID name.
 * \param [out] info            Match ID data.
 *
 * \retval SHR_E_NONE           Success.
 * \retval SHR_E_PARAM          Invalid unit, sid, fid or info.
 * \retval SHR_E_NOT_FOUND      Match ID data is not found.
 *
 */
extern int
bcmlrd_table_match_id_data_get(int unit,
                               const char *spec,
                               const bcmlrd_match_id_db_t **info);


#endif /* BCMLRD_MATCH_ID_DB_H */
