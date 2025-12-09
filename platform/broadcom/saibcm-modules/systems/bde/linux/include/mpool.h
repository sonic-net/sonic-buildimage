/*
 * $Id: mpool.h,v 1.2 Broadcom SDK $
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

#ifndef __MPOOL_H__
#define __MPOOL_H__

struct mpool_mem_s;
typedef struct mpool_mem_s* mpool_handle_t;

extern int mpool_init(void);
extern mpool_handle_t mpool_create(void* base_address, int size);
extern void* mpool_alloc(mpool_handle_t pool, int size);
extern void  mpool_free(mpool_handle_t pool, void* ptr);
extern int mpool_destroy(mpool_handle_t pool);

extern int mpool_usage(mpool_handle_t pool);

#endif /* __MPOOL_H__ */
