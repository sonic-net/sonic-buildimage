/**************************************************************************************
 *  File Name     :  blackhawk_tsc_usr_includes.h                                        *
 *  Created On    :  05/07/2014                                                       *
 *  Created By    :  Kiran Divakar                                                    *
 *  Description   :  Header file which includes all required std libraries and macros *
 * $Copyright: (c) 2020 Broadcom.                                               *
 * Broadcom Proprietary and Confidential. All rights reserved.$                 *
 *                                                                                    *
 **************************************************************************************
 **************************************************************************************/

/** @file blackhawk_tsc_usr_includes.h
 * Header file which includes all required std libraries and macros
 */

/* The user is expected to replace the macro definitions with their required implementation */

#ifndef BLACKHAWK_TSC_API_USR_INCLUDES_H
#define BLACKHAWK_TSC_API_USR_INCLUDES_H

/* Standard libraries that can be replaced by your custom libraries */
#ifdef _MSC_VER
/* Enclose all standard headers in a pragma to remove warings for MS compiler */
#pragma warning( push, 0 )
#endif
#ifdef NON_SDK
#include <stdint.h>
#include <string.h>
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

/* Redefine macros according your compiler requirements */
#ifdef NON_SDK
#ifndef NO_VARIADIC_MACROS
#define USR_PRINTF(paren_arg_list)    ((void)usr_logger_write paren_arg_list)
#else
#define USR_PRINTF(paren_arg_list)
#endif /* NO_VARIADIC_MACROS */
#define USR_MEMSET(mem, val, num)     memset(mem, val, num)
#define USR_STRLEN(string)            strlen(string)
#define USR_STRNCAT(str1, str2, num)  strncat(str1, str2, num)
#define USR_STRCPY(str1, str2)        strcpy(str1, str2)
#define USR_STRNCPY(str1, str2, num)  strncpy(str1, str2, num)
#define USR_STRCMP(str1, str2)        strcmp(str1, str2)
#define USR_STRNCMP(str1, str2, num)  strncmp(str1, str2, num)


#else /* NON_SDK */

#include <phymod/phymod.h>

#define USR_PRINTF(stuff)             PHYMOD_DIAG_OUT(stuff)
#define USR_MEMSET(mem, val, num)     PHYMOD_MEMSET(mem, val, num)
#define USR_STRLEN(string)            PHYMOD_STRLEN(string)
#define USR_STRCAT(str1, str2)        PHYMOD_STRCAT(str1, str2)
#define USR_STRNCAT(str1, str2, num)  PHYMOD_STRNCAT(str1, str2, num)
#define USR_STRCPY(str1, str2)        PHYMOD_STRCPY(str1, str2)
#define USR_STRNCPY(str1, str2, num)  PHYMOD_STRNCPY(str1, str2, num)
#define USR_STRCMP(str1, str2)        PHYMOD_STRCMP(str1, str2)
#define USR_STRNCMP(str1, str2, num)  PHYMOD_STRNCMP(str1, str2, num)
#define USR_SPRINTF                   PHYMOD_SPRINTF
#define USR_ACQUIRE_LOCK
#define USR_RELEASE_LOCK


typedef unsigned long int       uintptr_t;

#endif /* NON_SDK */

#define USR_UINTPTR     uintptr_t

#ifdef SERDES_API_FLOATING_POINT
#define USR_DOUBLE      double
#else
#define USR_DOUBLE      int
#define double          undefined
#define float           undefined
#endif

#ifndef UINT16_MAX
#define UINT16_MAX      0xFFFFU
#endif


/* Implementation specific macros below */
#ifdef SRDS_API_ALL_FUNCTIONS_HAVE_ACCESS_STRUCT
#ifndef NO_VARIADIC_MACROS
#define usr_logger_write(...) logger_write(sa__, -1,__VA_ARGS__)
#endif
#define USR_DELAY_MS(stuff) plp_aperta_blackhawk_tsc_delay_ms(sa__,stuff)
#define USR_DELAY_US(stuff) plp_aperta_blackhawk_tsc_delay_us(sa__,stuff)
#define USR_DELAY_NS(stuff) plp_aperta_blackhawk_tsc_delay_ns(sa__,stuff)
#else
#ifndef NO_VARIADIC_MACROS
#define usr_logger_write(...) logger_write(-1,__VA_ARGS__)
#endif
#define USR_DELAY_MS(stuff) plp_aperta_blackhawk_tsc_delay_ms(stuff)
#define USR_DELAY_US(stuff) plp_aperta_blackhawk_tsc_delay_us(stuff)
#define USR_DELAY_NS(stuff) plp_aperta_blackhawk_tsc_delay_ns(stuff)
#endif

#if defined _MSC_VER
#define API_FUNCTION_NAME __FUNCTION__
#else
#define API_FUNCTION_NAME __func__
#endif

#endif
