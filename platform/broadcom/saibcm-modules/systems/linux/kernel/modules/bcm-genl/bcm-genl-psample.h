/*
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

#ifndef __BCM_GENL_PSAMPLE_H__
#define __BCM_GENL_PSAMPLE_H__

#include <lkm.h>

#ifndef BCMGENL_PSAMPLE_SUPPORT
#define BCMGENL_PSAMPLE_SUPPORT (IS_ENABLED(CONFIG_PSAMPLE))
#endif

extern int
bcmgenl_psample_init(char *procfs_path);

extern int
bcmgenl_psample_cleanup(void);

#endif /* __BCM_GENL_PSAMPLE_H__ */
