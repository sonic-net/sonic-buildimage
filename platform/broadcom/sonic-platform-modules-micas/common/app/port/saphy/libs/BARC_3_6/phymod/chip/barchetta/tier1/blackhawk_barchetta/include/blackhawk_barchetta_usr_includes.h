/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/**************************************************************************************
 *  File Name     :  blackhawk_barchetta_usr_includes.h                                        *
 *  Created On    :  05/07/2014                                                       *
 *  Created By    :  Kiran Divakar                                                    *
 *  Description   :  Header file which includes all required std libraries and macros *
 *  Revision      :   *
 *                                                                                    *
 **************************************************************************************
 **************************************************************************************/

/** @file blackhawk_barchetta_usr_includes.h
 * Header file which includes all required std libraries and macros
 */

/* The user is expected to replace the macro definitions with their required implementation */

#ifndef BLACKHAWK_BARCHETTA_API_USR_INCLUDES_H
#define BLACKHAWK_BARCHETTA_API_USR_INCLUDES_H

/* Standard libraries that can be replaced by your custom libraries */
#ifdef _MSC_VER
/* Enclose all standard headers in a pragma to remove warings for MS compiler */
#pragma warning( push, 0 )
#endif
#ifndef EXCLUDE_STD_HEADERS
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif
typedef unsigned long uintptr_t;
#if defined _MSC_VER
#define API_FUNCTION_NAME __FUNCTION__
#else
#define API_FUNCTION_NAME __func__
#endif

/* In order to support coexistence of A0 and B0 variants of same IP */
#define SERDES_MULTI_INFO_TABLE_EN

/* Redefine macros according your compiler requirements */

#define USR_PRINTF(stuff)             PHYMOD_DIAG_OUT(stuff)
#define USR_MEMSET(mem, val, num)     PHYMOD_MEMSET(mem, val, num)
#define USR_STRLEN(string)            PHYMOD_STRLEN(string)
#define USR_STRNCAT(str1, str2, num)  PHYMOD_STRNCAT(str1, str2, num)
#define USR_STRCPY(str1, str2)        PHYMOD_STRCPY(str1, str2)
#define USR_STRCMP(str1, str2)        PHYMOD_STRCMP(str1, str2)
#define USR_STRNCMP(str1, str2, num)  PHYMOD_STRNCMP(str1, str2, num)
#ifndef NO_VARIADIC_MACROS
#define USR_SPRINTF(...)   (void)sprintf (__VA_ARGS__)
#endif
#define USR_UINTPTR                   uintptr_t

#ifdef SERDES_API_FLOATING_POINT
#define USR_DOUBLE                    double
#else
#define USR_DOUBLE       int
#define double       undefined
#define float        undefined
#endif

/* Syncronization macro Definitions */
#ifndef NO_VARIADIC_MACROS
#define USR_CREATE_LOCK 
#define USR_ACQUIRE_LOCK 
#define USR_RELEASE_LOCK 
#define USR_DESTROY_LOCK
#endif

/* Implementation specific macros below */
#ifdef SRDS_API_ALL_FUNCTIONS_HAVE_ACCESS_STRUCT
#ifndef NO_VARIADIC_MACROS
#define usr_logger_write(...) logger_write(sa__, -1,__VA_ARGS__)
#endif
#define USR_DELAY_MS(stuff) plp_barchetta_blackhawk_barchetta_delay_ms(sa__,stuff)
#define USR_DELAY_US(stuff) plp_barchetta_blackhawk_barchetta_delay_us(sa__,stuff)
#define USR_DELAY_NS(stuff) plp_barchetta_blackhawk_barchetta_delay_ns(sa__,stuff)
#else
#ifndef NO_VARIADIC_MACROS
#define usr_logger_write(...) logger_write(-1,__VA_ARGS__)
#endif
#define USR_DELAY_MS(stuff) plp_barchetta_blackhawk_barchetta_delay_ms(stuff)
#define USR_DELAY_US(stuff) plp_barchetta_blackhawk_barchetta_delay_us(stuff)
#define USR_DELAY_NS(stuff) plp_barchetta_blackhawk_barchetta_delay_ns(stuff)
#endif

#endif
