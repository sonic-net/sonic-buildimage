/* 
 * $Id: types.h,v 1.1 Broadcom SDK $
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
 * File:        types.h
 * Purpose:     SAL Definitions 
 */

#ifndef   _SAL_TYPES_H_
#define   _SAL_TYPES_H_

#ifdef DCB_CUSTOM_CONFIG
/* Allow application to override sal_sprintf, sal_memset, sal_memcpy */
#include <dcb_custom_config.h>
#else
#ifdef __KERNEL__
#include <lkm.h>
#else
/* Needed for sprintf, memset */
#include <stdint.h>
#include <string.h>
#endif
#endif


/* Booleans */
#ifndef TRUE
#define TRUE               1
#endif

#ifndef FALSE
#define FALSE              0
#endif

/* Data types */
typedef unsigned char     uint8;        /*  8-bit quantity  */
typedef unsigned short    uint16;       /* 16-bit quantity */
typedef unsigned int      uint32;       /* 32-bit quantity */
typedef uint64_t          uint64;       /* 64-bit quantity */

typedef signed char       int8;         /*  8-bit quantity */
typedef signed short      int16;        /* 16-bit quantity */
typedef signed int        int32;        /* 32-bit quantity */

typedef uint32            sal_paddr_t;  /* Physical address (PCI address) */
typedef uintptr_t         sal_vaddr_t;  /* Virtual address (Host address) */

#define PTR_TO_INT(x)     ((uint32)(((uint64)(x))&0xFFFFFFFF))
#define PTR_HI_TO_INT(x)  ((uint32)((((uint64)(x))>>32)&0xFFFFFFFF))

typedef uint8             sal_mac_addr_t[6];  /* MAC address */

/* Macros */
#define COMPILER_REFERENCE(_a)    ((void)(_a))

/* Functions */
#define sal_memset        memset
#define sal_sprintf       sprintf
#define sal_memcpy        memcpy

#endif /* _SAL_TYPES_H_ */
