/*
 * $Id: $
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
 *
 * Broadcom System Log (bSL)
 *
 * Defines for INTERNAL usage only.
 */

#ifndef _SHR_BSL_H_
#define _SHR_BSL_H_

#ifdef DCB_CUSTOM_CONFIG
/* Allow application to override BSL_META, LOG_CLI, etc. */
#include <dcb_custom_config.h>
#else
#ifdef __KERNEL__
#include <lkm.h>

#define printf printk
#else
/* Needed for printf */
#include <stdio.h>
#endif

#define BSL_META(_str) _str
#define BSL_META_U(_unit, _str) _str
#define LOG_ERROR(_chk, _str) printf _str
#define LOG_CLI(_str) printf _str

#endif
#endif /* _SHR_BSL_H_ */
