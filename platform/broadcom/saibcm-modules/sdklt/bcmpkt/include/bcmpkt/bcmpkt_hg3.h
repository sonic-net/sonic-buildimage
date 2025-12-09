/*! \file bcmpkt_hg3.h
 *
 * Common macros and definitions for Higig3 protocol
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

#ifndef BCMPKT_HG3_H
#define BCMPKT_HG3_H

/* Note, ether type set to same value as reset value of R_GSH_ETHERTYPEr(700) */
/*! Ethernet type used for Higig3 header */
#define BCMPKT_HG3_ETHER_TYPE                     0x2BC

/*! Higig3 base header size (bytes). */
#define BCMPKT_HG3_BASE_HEADER_SIZE_BYTES         8
/*! Higig3 base header size (words). */
#define BCMPKT_HG3_BASE_HEADER_SIZE_WORDS         2

/*! Higig3 extension 0 header size (bytes). */
#define BCMPKT_HG3_EXT0_HEADER_SIZE_BYTES         8
/*! Higig3 extension 0 header size (words). */
#define BCMPKT_HG3_EXT0_HEADER_SIZE_WORDS         2

/*! Higig3 header size (bytes). Includes base and ext0 header */
#define BCMPKT_HG3_SIZE_BYTES       (BCMPKT_HG3_BASE_HEADER_SIZE_BYTES + \
                                     BCMPKT_HG3_EXT0_HEADER_SIZE_BYTES)
/*! Higig3 header size (words). Includes base and ext0 header */
#define BCMPKT_HG3_SIZE_WORDS       (BCMPKT_HG3_BASE_HEADER_SIZE_WORDS + \
                                     BCMPKT_HG3_EXT0_HEADER_SIZE_WORDS)

/*! Higig3 extension 0 field max. */
#define BCMPKT_HG3_EXT0_FID_MAX                   32
#endif /* BCMPKT_HG3_H */
