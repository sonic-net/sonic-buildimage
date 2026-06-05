/***********************************************************************************
 *                                                                                 *
 * Copyright: (c) 2021 Broadcom.                                                   *
 * Broadcom Proprietary and Confidential. All rights reserved.                     *
 *                                                                                 *
 ***********************************************************************************/

/**************************************************************************************
 *  File Name     :  peregrine5_pc_usr_includes.h                                        *
 *  Created On    :  05/07/2014                                                       *
 *  Created By    :  Kiran Divakar                                                    *
 *  Description   :  Header file which includes all required std libraries and macros *
 *  Revision      :   *
 *                                                                                    *
 **************************************************************************************
 **************************************************************************************/

#ifndef PEREGRINE5_PC_API_USR_INCLUDES_H
#define PEREGRINE5_PC_API_USR_INCLUDES_H

/* Standard libraries that can be replaced by your custom libraries */
#ifdef _MSC_VER
/* Enclose all standard headers in a pragma to remove warings for MS compiler */
#pragma warning( push, 0 )
#endif
#ifdef NON_SDK
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#ifdef SERDES_API_FLOATING_POINT
#include <math.h>
#endif
#endif

#include "peregrine5_pc_ipconfig.h"

/*! @file
 *  @brief Header file which includes all required std libraries and macros
 *
 * The user is expected to replace the macro definitions with their required implementation
 */

/*! @addtogroup APITag
 * @{
 */

/*! @defgroup SerdesAPIUsrIncludesTag User Provided Macros
 * Contains macros with default definitions which can be redefined
 * by users if desired.
 */

/*! @addtogroup SerdesAPIUsrIncludesTag
 * @{
 */

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#if defined _MSC_VER
#define API_FUNCTION_NAME __FUNCTION__
#else
#define API_FUNCTION_NAME __func__
#endif
/* Redefine macros according your compiler requirements */
#ifdef NON_SDK
#ifndef SMALL_FOOTPRINT
# ifdef SDK_LIBRARY
#  define USR_PRINTF(paren_arg_list)    ((void)printf paren_arg_list)
# else
#  define USR_PRINTF(paren_arg_list)    ((void)usr_logger_write paren_arg_list)
# endif
/*! @def USR_VPRINTF
 * Is likely not being used, and may be removed in the future.
 * usr_logger_verbose_write() appears broken (no 'sa__' arg)
 */
#define USR_VPRINTF(paren_arg_list)   ((void)usr_logger_verbose_write paren_arg_list)
#else
#define USR_PRINTF(paren_arg_list)
#endif

#define USR_MEMSET(mem, val, num)     memset(mem, val, num)
#define USR_STRLEN(string)            strlen(string)
#define USR_STRNCAT(str1, str2, num)  strncat(str1, str2, num)
#define USR_STRCPY(str1, str2)        strcpy(str1, str2)
#define USR_STRCMP(str1, str2)        strcmp(str1, str2)
#define USR_STRNCMP(str1, str2, num)  strncmp(str1, str2, num)
#define USR_SNPRINTF                  snprintf
#define USR_CALLOC(nmemb, size)       calloc(nmemb, size)
#define USR_FREE(ptr)                 free(ptr)

#else /* NON_SDK */
#include <phymod/phymod.h>
#ifdef SERDES_API_FLOATING_POINT
#include <math.h>
#endif

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
#define USR_CALLOC(memb, size)        PHYMOD_MALLOC(memb * size, "bcm")
#define USR_FREE                      PHYMOD_FREE
#define USR_SNPRINTF                  PHYMOD_SNPRINTF

#ifndef WIN32
typedef unsigned long int       uintptr_t;
#endif

#endif /* NON_SDK */

#define USR_UINTPTR                   uintptr_t


#ifdef SERDES_API_FLOATING_POINT
#define USR_DOUBLE                    double
#else
#define USR_DOUBLE       int
#define double      int 
#define float       int
#endif

typedef uint8_t srds_core_t; /* unsigned types only permitted */

/* Mutex Definitions */
#ifndef NO_VARIADIC_MACROS
#define USR_CREATE_LOCK
#define USR_ACQUIRE_LOCK
#define USR_RELEASE_LOCK
#define USR_DESTROY_LOCK
#endif

/* Implementation specific macros below */
#ifndef NO_VARIADIC_MACROS
/*! @def usr_logger_write
 * is an API user provided Dependency function. In practice it's return value is
 * never checked. To do so, the following define can be used. If using one of the API error checking
 * macros (like EFUN()) which calls the API error processing functons, the
 * ERR_CODE_SRDS_REG_ACCESS_FAIL error code should be returned to avoid error processing recursion.
 * The (void) cast serves to quiet compiler warnings if so configured, otherwise it can be removed.
 * usr_logger_write(...)  ((void)(logger_write(0, -1, __VA_ARGS__) ? ERR_CODE_SRDS_REG_ACCESS_FAIL : ERR_CODE_NONE))
 * ********************************************************************************************
 */
#define usr_logger_write(...) logger_write(sa__, -1,__VA_ARGS__)

/*! @def usr_logger_verbose_write
 * appears broken (no 'sa__' arg), is likely not being used, and may be removed.
 */
#define usr_logger_verbose_write(...) logger_write(1,__VA_ARGS__)
#endif
#define USR_DELAY_MS(stuff) plp_aperta2_peregrine5_pc_acc_delay_ms(sa__,stuff)
#define USR_DELAY_US(stuff) plp_aperta2_peregrine5_pc_acc_delay_us(sa__,stuff)
#define USR_DELAY_NS(stuff) plp_aperta2_peregrine5_pc_acc_delay_ns(sa__,stuff)
/*! @} SerdesAPIUsrIncludesTag */
/*! @} APITag */
#endif /* !SERDES_API_USR_INCLUDES_H */
