/*! \file ngedk_kapi.h
 *
 * NGEDK kernel API.
 *
 * This file is intended for use by other kernel modules relying on the NGEDK.
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

#ifndef NGEDK_KAPI_H
#define NGEDK_KAPI_H

/*!
 * \brief Converts physical address to virtual address.
 *
 * \param [in] paddr physical address.
 *
 * \retval void * Corresponding virtual address.
 */
extern void *
ngedk_dmamem_map_p2v(dma_addr_t paddr);

#endif /* NGEDK_KAPI_H */

