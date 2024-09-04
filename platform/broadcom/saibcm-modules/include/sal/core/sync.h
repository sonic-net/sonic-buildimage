/*
 * $Id: sync.h,v 1.1 Broadcom SDK $
 * $Copyright: 2007-2023 Broadcom Inc. All rights reserved.
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
 */

#ifndef _SAL_SYNC_H
#define _SAL_SYNC_H

typedef struct sal_sem_s{
    char sal_opaque_type;
} *sal_sem_t;

#define sal_sem_FOREVER		(-1)
#define sal_sem_BINARY		1
#define sal_sem_COUNTING	0

sal_sem_t	sal_sem_create(char *desc, int binary, int initial_count);
void		sal_sem_destroy(sal_sem_t b);
int		sal_sem_take(sal_sem_t b, int usec);
int		sal_sem_give(sal_sem_t b);

#endif	/* !_SAL_SYNC_H */
