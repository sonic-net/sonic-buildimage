/*! \file bcmgenl_psample.h
 *
 * BCMGENL psample module.
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

#ifndef BCMGENL_PSAMPLE_H
#define BCMGENL_PSAMPLE_H

#include <linux/kconfig.h>

#ifndef BCMGENL_PSAMPLE_SUPPORT
#define BCMGENL_PSAMPLE_SUPPORT (IS_ENABLED(CONFIG_PSAMPLE))
#endif /* BCMGENL_PSAMPLE_SUPPORT */

extern int bcmgenl_psample_init(void);
extern int bcmgenl_psample_cleanup(void);

#endif /* BCMGENL_PSAMPLE_H */
