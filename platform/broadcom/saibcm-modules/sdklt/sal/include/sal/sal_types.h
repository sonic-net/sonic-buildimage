/*! \file sal_types.h
 *
 * Basic types and convenience macros.
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

#ifndef SAL_TYPES_H
#define SAL_TYPES_H

#include <sal_config.h>

/*!
 * \cond SAL_TYPES
 */

#if SAL_CONFIG_DEFINE_UINT8_T == 1
typedef SAL_CONFIG_TYPE_UINT8_T uint8_t;
#endif

#if SAL_CONFIG_DEFINE_UINT16_T == 1
typedef SAL_CONFIG_TYPE_UINT16_T uint16_t;
#endif

#if SAL_CONFIG_DEFINE_UINT32_T == 1
typedef SAL_CONFIG_TYPE_UINT32_T uint32_t;
#endif

#if SAL_CONFIG_DEFINE_UINT64_T == 1
typedef SAL_CONFIG_TYPE_UINT64_T uint64_t;
#endif

#if SAL_CONFIG_DEFINE_UINTPTR_T == 1
typedef SAL_CONFIG_TYPE_UINTPTR_T uintptr_t;
#endif

#if SAL_CONFIG_DEFINE_INT8_T == 1
typedef SAL_CONFIG_TYPE_INT8_T int8_t;
#endif

#if SAL_CONFIG_DEFINE_INT16_T == 1
typedef SAL_CONFIG_TYPE_INT16_T int16_t;
#endif

#if SAL_CONFIG_DEFINE_INT32_T == 1
typedef SAL_CONFIG_TYPE_INT32_T int32_t;
#endif

#if SAL_CONFIG_DEFINE_INT64_T == 1
typedef SAL_CONFIG_TYPE_INT64_T int64_t;
#endif

#if SAL_CONFIG_DEFINE_SIZE_T == 1
typedef SAL_CONFIG_TYPE_SIZE_T size_t;
#endif

#if SAL_CONFIG_DEFINE_BOOL_T == 1
typedef SAL_CONFIG_TYPE_BOOL_T bool;
#endif

#if SAL_CONFIG_DEFINE_DMA_ADDR_T == 1
typedef SAL_CONFIG_TYPE_DMA_ADDR_T dma_addr_t;
#endif

#if SAL_CONFIG_DEFINE_PRIu32 == 1
#define PRIu32 SAL_CONFIG_MACRO_PRIu32
#endif

#if SAL_CONFIG_DEFINE_PRId32 == 1
#define PRId32 SAL_CONFIG_MACRO_PRId32
#endif

#if SAL_CONFIG_DEFINE_PRIx32 == 1
#define PRIx32 SAL_CONFIG_MACRO_PRIx32
#endif

#if SAL_CONFIG_DEFINE_PRIu64 == 1
#define PRIu64 SAL_CONFIG_MACRO_PRIu64
#endif

#if SAL_CONFIG_DEFINE_PRId64 == 1
#define PRId64 SAL_CONFIG_MACRO_PRId64
#endif

#if SAL_CONFIG_DEFINE_PRIx64 == 1
#define PRIx64 SAL_CONFIG_MACRO_PRIx64
#endif

#ifndef offsetof
#define offsetof(_s, _m) ((unsigned long)&(((_s *)0)->_m))
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef COUNTOF
#define COUNTOF(ary) ((int) (sizeof(ary) / sizeof((ary)[0])))
#endif

#ifndef COMPILER_REFERENCE
#define COMPILER_REFERENCE(_a) ((void)(_a))
#endif

/*!
 * Compiler attribute keyword.
 *
 * We use this only for enhanced code validation, so it does not need
 * to be fully portable.
 */
#ifndef SAL_ATTR
#  if defined(__GNUC__) && !defined(__PEDANTIC__)
#    define SAL_ATTR(_a) __attribute__(_a)
#  else
#    define SAL_ATTR(_a)
#  endif
#endif

/*!
 * \endcond
 */

#endif /* SAL_TYPES_H */
