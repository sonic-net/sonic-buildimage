/*! \file ngknet_procfs.h
 *
 * Procfs-related definitions and APIs for NGKNET module.
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

#ifndef NGKNET_PROCFS_H
#define NGKNET_PROCFS_H

/*!
 * \brief Initialize procfs for KNET driver.
 *
 * Create procfs read/write interfaces.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngknet_procfs_init(void);

/*!
 * \brief Clean up procfs for KNET driver.
 *
 * Clean up resources allocated by \ref ngknet_procfs_init.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngknet_procfs_cleanup(void);

#endif /* NGKNET_PROCFS_H */

