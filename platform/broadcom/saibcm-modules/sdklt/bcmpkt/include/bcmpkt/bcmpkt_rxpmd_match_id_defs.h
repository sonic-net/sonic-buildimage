/*! \file bcmpkt_rxpmd_match_id_defs.h
 *
 * RX Packet Meta Data Match ID Variant defs.
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

#ifndef BCMPKT_RXPMD_MATCH_ID_DEFS_H
#define BCMPKT_RXPMD_MATCH_ID_DEFS_H

/*! \cond Declare externs. */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
extern bcmpkt_rxpmd_match_id_db_info_t * \
_bd##_vu##_va##_rxpmd_match_id_db_info_get(void);
#include <bcmlrd/chip/bcmlrd_chip_variant.h>

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
extern bcmpkt_rxpmd_match_id_map_info_t * \
_bd##_vu##_va##_rxpmd_match_id_map_info_get(void);
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
/*! \endcond */

#endif /* BCMPKT_RXPMD_MATCH_ID_DEFS_H */
