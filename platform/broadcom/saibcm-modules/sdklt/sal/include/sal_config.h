/*! \file sal_config.h
 *
 * Broadcom System Abstraction Layer (SAL) configuration
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

#ifndef SAL_CONFIG_H
#define SAL_CONFIG_H

/*
 * Include system config file if specified:
 */
#ifdef SAL_INCLUDE_CUSTOM_CONFIG
#include <sal_custom_config.h>
#endif

/*
 * Optionally use Linux C library and header files:
 */
#ifdef SAL_LINUX
#include <sal/sal_linux.h>
/* Linux implies no standard C (see below) */
#ifndef SAL_NO_STDC
#define SAL_NO_STDC
#endif
#endif

/*
 * Use standard C library and header files by default:
 */
#ifndef SAL_NO_STDC
#include <sal/sal_stdc.h>
#endif

/*!
 * \cond SAL_CONFIG
 */

/* Type uint8_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_UINT8_T
#define SAL_CONFIG_DEFINE_UINT8_T               1
#endif

/* Default type definition for uint8_t */
#ifndef SAL_CONFIG_TYPE_UINT8_T
#define SAL_CONFIG_TYPE_UINT8_T                 unsigned char
#endif

/* Type uint16_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_UINT16_T
#define SAL_CONFIG_DEFINE_UINT16_T              1
#endif

/* Default type definition for uint16_t */
#ifndef SAL_CONFIG_TYPE_UINT16_T
#define SAL_CONFIG_TYPE_UINT16_T                unsigned short
#endif

/* Type uint32_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_UINT32_T
#define SAL_CONFIG_DEFINE_UINT32_T              1
#endif

/* Default type definition for uint32_t */
#ifndef SAL_CONFIG_TYPE_UINT32_T
#define SAL_CONFIG_TYPE_UINT32_T                unsigned int
#endif

/* Type uint64_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_UINT64_T
#define SAL_CONFIG_DEFINE_UINT64_T              1
#endif

/* Default type definition for uint64_t */
#ifndef SAL_CONFIG_TYPE_UINT64_T
#define SAL_CONFIG_TYPE_UINT64_T                unsigned long long
#endif

/* Type uintptr_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_UINTPTR_T
#define SAL_CONFIG_DEFINE_UINTPTR_T              1
#endif

/* Default type definition for uintptr_t */
#ifndef SAL_CONFIG_TYPE_UINTPTR_T
#define SAL_CONFIG_TYPE_UINTPTR_T               unsigned long long
#endif

/* Type int8_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_INT8_T
#define SAL_CONFIG_DEFINE_INT8_T                1
#endif

/* Default type definition for int8_t */
#ifndef SAL_CONFIG_TYPE_INT8_T
#define SAL_CONFIG_TYPE_INT8_T                  signed char
#endif

/* Type int16_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_INT16_T
#define SAL_CONFIG_DEFINE_INT16_T               1
#endif

/* Default type definition for int16_t */
#ifndef SAL_CONFIG_TYPE_INT16_T
#define SAL_CONFIG_TYPE_INT16_T                 signed short
#endif

/* Type int32_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_INT32_T
#define SAL_CONFIG_DEFINE_INT32_T               1
#endif

/* Default type definition for int32_t */
#ifndef SAL_CONFIG_TYPE_INT32_T
#define SAL_CONFIG_TYPE_INT32_T                 signed int
#endif

/* Type int64_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_INT64_T
#define SAL_CONFIG_DEFINE_INT64_T               1
#endif

/* Default type definition for int64_t */
#ifndef SAL_CONFIG_TYPE_INT64_T
#define SAL_CONFIG_TYPE_INT64_T                 signed long long
#endif

/* Type size_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_SIZE_T
#define SAL_CONFIG_DEFINE_SIZE_T                1
#endif

/* Default type definition for size_t */
#ifndef SAL_CONFIG_TYPE_SIZE_T
#define SAL_CONFIG_TYPE_SIZE_T                  unsigned int
#endif

/* Type bool is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_BOOL_T
#define SAL_CONFIG_DEFINE_BOOL_T                1
#endif

/* Default type definition for bool */
#ifndef SAL_CONFIG_TYPE_BOOL_T
#define SAL_CONFIG_TYPE_BOOL_T                  enum { false = 0, true = 1 }
#endif

/* Type dma_addr_t is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_DMA_ADDR_T
#define SAL_CONFIG_DEFINE_DMA_ADDR_T            1
#endif

/* Default type definition for dma_addr_t */
#ifndef SAL_CONFIG_TYPE_DMA_ADDR_T
#define SAL_CONFIG_TYPE_DMA_ADDR_T              unsigned int
#endif

/* Formatting macro SAL_PRIu32 is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_PRIu32
#define SAL_CONFIG_DEFINE_PRIu32                1
#endif

/* Default definition for formatting macro SAL_PRIu32 */
#ifndef SAL_CONFIG_MACRO_PRIu32
#define SAL_CONFIG_MACRO_PRIu32                 "u"
#endif

/* Formatting macro SAL_PRId32 is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_PRId32
#define SAL_CONFIG_DEFINE_PRId32                1
#endif

/* Default definition for formatting macro SAL_PRId32 */
#ifndef SAL_CONFIG_MACRO_PRId32
#define SAL_CONFIG_MACRO_PRId32                 "d"
#endif

/* Formatting macro SAL_PRIx32 is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_PRIx32
#define SAL_CONFIG_DEFINE_PRIx32                1
#endif

/* Default definition for formatting macro SAL_PRIx32 */
#ifndef SAL_CONFIG_MACRO_PRIx32
#define SAL_CONFIG_MACRO_PRIx32                 "x"
#endif

/* Formatting macro SAL_PRIu64 is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_PRIu64
#define SAL_CONFIG_DEFINE_PRIu64                1
#endif

/* Default definition for formatting macro SAL_PRIu64 */
#ifndef SAL_CONFIG_MACRO_PRIu64
#define SAL_CONFIG_MACRO_PRIu64                 "llu"
#endif

/* Formatting macro SAL_PRId64 is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_PRId64
#define SAL_CONFIG_DEFINE_PRId64                1
#endif

/* Default definition for formatting macro SAL_PRId64 */
#ifndef SAL_CONFIG_MACRO_PRId64
#define SAL_CONFIG_MACRO_PRId64                 "lld"
#endif

/* Formatting macro SAL_PRIx64 is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_PRIx64
#define SAL_CONFIG_DEFINE_PRIx64                1
#endif

/* Default definition for formatting macro SAL_PRIx64 */
#ifndef SAL_CONFIG_MACRO_PRIx64
#define SAL_CONFIG_MACRO_PRIx64                 "llx"
#endif

/* Assert macro is not provided by the system */
#ifndef SAL_CONFIG_DEFINE_ASSERT
#define SAL_CONFIG_DEFINE_ASSERT                1
#endif

/* Default definition for assert macro */
#ifndef SAL_CONFIG_MACRO_ASSERT
#define SAL_CONFIG_MACRO_ASSERT                 SAL_ASSERT_DEFAULT
#endif

/* Memory Barrier if necessary */
#ifndef SAL_CONFIG_MEMORY_BARRIER
#define SAL_CONFIG_MEMORY_BARRIER               ;
#endif

/*!
 * \endcond
 */

#endif /* SAL_CONFIG_H */
