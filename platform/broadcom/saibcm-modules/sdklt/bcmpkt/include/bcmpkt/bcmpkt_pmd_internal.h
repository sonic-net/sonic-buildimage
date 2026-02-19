/*! \file bcmpkt_pmd_internal.h
 *
 * \brief Basic PMD definitions.
 *
 * The defintions are kept separate to minimize the header file
 * dependencies for the stand-alone PMD library.
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

#ifndef BCMPKT_PMD_INTERNAL_H
#define BCMPKT_PMD_INTERNAL_H

#include <shr/shr_types.h>

/*! Get a field from a PMD buffer. */
typedef uint32_t (*bcmpkt_field_get_f)(uint32_t *data);

/*! Set a field within a PMD buffer. */
typedef void (*bcmpkt_field_set_f)(uint32_t *data, uint32_t val);

/*! Get a complex field pointer or other attributions. */
typedef uint32_t (*bcmpkt_ifield_get_f)(uint32_t *data, uint32_t **addr);

/*!
 * \brief Packet metadata information structure.
 */
typedef struct bcmpkt_pmd_view_info_s {

    /*! View type list. */
    shr_enum_map_t *view_types;

    /*!
     * Each field's view code.
     * -2 means unavailable field.
     * -1 means common field.
     * others are correspondent view codes defined in view types.
     */
    int *view_infos;

    /*! View type get function. */
    bcmpkt_field_get_f view_type_get;

} bcmpkt_pmd_view_info_t;

#endif /* BCMPKT_PMD_INTERNAL_H */
