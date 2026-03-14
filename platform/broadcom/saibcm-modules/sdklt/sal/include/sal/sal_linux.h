/*! \file sal_linux.h
 *
 * Convenience file for mapping SAL C library functions to Linux.
 *
 * The main purpose of this file is to allow shared SDK source files
 * to be used for building Linux kernel modules.
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

#ifndef SAL_LINUX_H
#define SAL_LINUX_H

#if !defined(SAL_CONFIG_H) || !defined(SAL_LINUX)

/*
 * If we get here it means that some file other than sal_config.h
 * included this file before sal_config.h did.
 */
#error sal_linux.h file cannot be included by regular source files

#else


/*!
 * \cond SAL_LINUX
 */

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
/* Use built-in version */
#include <linux/stdarg.h>
#else
/* Borrow from compiler */
#include <stdarg.h>
#endif

#include <linux/types.h>
#define SAL_CONFIG_DEFINE_SIZE_T        0
#define SAL_CONFIG_DEFINE_UINT8_T       0
#define SAL_CONFIG_DEFINE_UINT16_T      0
#define SAL_CONFIG_DEFINE_UINT32_T      0
#define SAL_CONFIG_DEFINE_UINT64_T      0
#define SAL_CONFIG_DEFINE_UINTPTR_T     0
#define SAL_CONFIG_DEFINE_INT8_T        0
#define SAL_CONFIG_DEFINE_INT16_T       0
#define SAL_CONFIG_DEFINE_INT32_T       0
#define SAL_CONFIG_DEFINE_INT64_T       0
#define SAL_CONFIG_DEFINE_PRIu32        0
#define SAL_CONFIG_DEFINE_PRId32        0
#define SAL_CONFIG_DEFINE_PRIx32        0
#define SAL_CONFIG_DEFINE_PRIu64        0
#define SAL_CONFIG_DEFINE_PRId64        0
#define SAL_CONFIG_DEFINE_PRIx64        0
#define SAL_CONFIG_DEFINE_BOOL_T        0
#define SAL_CONFIG_DEFINE_DMA_ADDR_T    0

#include <linux/string.h>
#ifndef sal_memcmp
#define sal_memcmp memcmp
#endif
#ifndef sal_memcpy
#define sal_memcpy memcpy
#endif
#ifndef sal_memset
#define sal_memset memset
#endif
#ifndef sal_memmove
#define sal_memmove memmove
#endif
#ifndef sal_strcpy
#define sal_strcpy strcpy
#endif
#ifndef sal_strncpy
#define sal_strncpy strncpy
#endif
#ifndef sal_strlen
#define sal_strlen strlen
#endif
#ifndef sal_strcmp
#define sal_strcmp strcmp
#endif
#ifndef sal_strncmp
#define sal_strncmp strncmp
#endif
#ifndef sal_strchr
#define sal_strchr strchr
#endif
#ifndef sal_strrchr
#define sal_strrchr strrchr
#endif
#ifndef sal_strstr
#define sal_strstr strstr
#endif
#ifndef sal_strcat
#define sal_strcat strcat
#endif
#ifndef sal_strncat
#define sal_strncat strncat
#endif
#ifndef sal_strcasecmp
#define sal_strcasecmp strcasecmp
#endif
#ifndef sal_strncasecmp
#define sal_strncasecmp strncasecmp
#endif

#include <linux/ctype.h>
#ifndef sal_tolower
#define sal_tolower tolower
#endif
#ifndef sal_toupper
#define sal_toupper toupper
#endif
#ifndef sal_isspace
#define sal_isspace isspace
#endif
#ifndef sal_isupper
#define sal_isupper isupper
#endif
#ifndef sal_islower
#define sal_islower islower
#endif
#ifndef sal_isalpha
#define sal_isalpha isalpha
#endif
#ifndef sal_isdigit
#define sal_isdigit isdigit
#endif
#ifndef sal_isalnum
#define sal_isalnum isalnum
#endif
#ifndef sal_isxdigit
#define sal_isxdigit isxdigit
#endif

#include <linux/kernel.h>
#ifndef sal_vsnprintf
#define sal_vsnprintf vsnprintf
#endif
#ifndef sal_vsprintf
#define sal_vsprintf vsprintf
#endif
#ifndef sal_snprintf
#define sal_snprintf snprintf
#endif
#ifndef sal_sprintf
#define sal_sprintf sprintf
#endif

/*!
 * \endcond
 */

#endif /* SAL_CONFIG_H */

#else

/*
 * If we get here it means that some file other than sal_config.h
 * included this file after sal_config.h already included it once.
 */
#error sal_linux.h file cannot be included by regular source files

#endif /* SAL_LINUX_H */
