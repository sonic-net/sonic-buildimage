/*! \file ngptpclock_ioctl.h
 *
 * NGPTPCLOCK I/O control definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 * IMPORTANT!
 * All shared structures must be properly 64-bit aligned.
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

#ifndef NGPTPCLOCK_IOCTL_H
#define NGPTPCLOCK_IOCTL_H

/* Module Information */
#define NGPTPCLOCK_MODULE_NAME     "linux_ngptpclock"

/*!
 * \brief PTP hardware clock driver commands
 */
/*! Initialize PTP hardware clock driver module */
#define NGPTPCLOCK_HW_INIT                     0
/*! Clean up PTP hardware clock driver module */
#define NGPTPCLOCK_HW_CLEANUP                  1

#endif /* NGPTPCLOCK_IOCTL_H */

