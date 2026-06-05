/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#ifndef __plp_aperta2_PHYMOD_CUSTOM_CONFIG_H__
#define __plp_aperta2_PHYMOD_CUSTOM_CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#ifdef SERDES_API_FLOATING_POINT
#include <math.h>
#endif
#ifdef WIN32
#include <stdint.h>
#endif
#ifndef BCM_PLP_CAPI_SUPPORT
#define BCM_PLP_CAPI_LOG_SUPPORT
#endif

/* #define PHYMOD_KOI_SUPPORT */
/* #define PHYMOD_ORCA_SUPPORT */
/* #define PHYMOD_SHORTFIN_SUPPORT */
/* #define PHYMOD_EVORA_SUPPORT */
/* #define PHYMOD_EUROPA_SUPPORT */
/* #define PHYMOD_DINO_SUPPORT */
/* #define PHYMOD_QUADRA28_SUPPORT */
/* #define PHYMOD_MADURA_SUPPORT */
/* #define PHYMOD_SEAHAWKS_SUPPORT */
/* #define PHYMOD_MIURA_SUPPORT */
/* #define PHYMOD_ESTOQUE_SUPPORT */
/* #define PHYMOD_ESTOQUEJ_SUPPORT */
/* #define PHYMOD_BARCHETTA_SUPPORT */
/* #define PHYMOD_BARCHETTA2_SUPPORT */
/* #define PHYMOD_BRONCOS_SUPPORT */
/* #define PHYMOD_RAVENS_SUPPORT */
/* #define PHYMOD_GALLARDO28_SUPPORT */
/* #define PHYMOD_GIANTS_SUPPORT */
/* #define PHYMOD_BLACKFIN_SUPPORT */
/* #define PHYMOD_MILLENIO_SUPPORT */
/* #define PHYMOD_MILLENIOB_SUPPORT */
/* #define PHYMOD_MONZA_SUPPORT */
/* #define PHYMOD_APERTA_SUPPORT */
/* #define PHYMOD_WHITETIP_SUPPORT */
/* #define PHYMOD_LONGFIN_SUPPORT */
/* #define PHYMOD_BROADFIN_SUPPORT */
/* #define PHYMOD_KAUAI_SUPPORT */
/* #define PHYMOD_LANAI_SUPPORT */
/* #define PHYMOD_RAMS_SUPPORT */
/* #define PHYMOD_COWBOYS_SUPPORT */
/* #define PHYMOD_AGERA_SUPPORT */
/* #define PHYMOD_AGERALITE_SUPPORT */
#define PHYMOD_APERTA2_SUPPORT
/* #define PHYMOD_AGERA2_SUPPORT */

#ifdef BCM_PLP_CAPI_SUPPORT
#define BCM_CAPI_MAX_PHY_RANGE 2
#endif

#ifdef  BCM_PLP_TIMESYNC_SUPPORT   /* from make/config.mk */
    #define PHYMOD_TIMESYNC_SUPPORT
#endif

#ifdef  PHYMOD_KOI_SUPPORT
#define PHYMOD_PHY848XX_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#ifdef  PHYMOD_ORCA_SUPPORT
#define PHYMOD_PHY848XX_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#ifdef  PHYMOD_SHORTFIN_SUPPORT
#define PHYMOD_XGBASET_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#ifdef  PHYMOD_KAUAI_SUPPORT
#define PHYMOD_XGBASET_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#ifdef  PHYMOD_LANAI_SUPPORT
#define PHYMOD_XGBASET_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#ifdef  PHYMOD_BLACKFIN_SUPPORT
#define PHYMOD_XGBASET_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || \
    defined(PHYMOD_BROADFIN_SUPPORT)
#define PHYMOD_XGBASET_SUPPORT
#define PLP_BASE_T_SPIROM_FIRMWARE
#endif

#ifdef  PLP_BASE_T_SPIROM_FIRMWARE  /* SPIROM mode options */
                                    /* DC_SPI        : Daisy Chained, multiple PHY chips share the same SPIROM */
                                    /* MULTI_SPI     : single SPIROM for the ports on a single PHY chip        */
                                    /* NON_SHARED_SPI: single SPIROM for a single port                         */
#define PLP_BASE_T_SPIROM_MODE         NON_SHARED_SPI   /* by default */
#endif

#ifdef  PHYMOD_SEAHAWKS_SUPPORT
#define PHYMOD_PHY542XX_SUPPORT
/* #define SGMII_AN_DISABLE */  /* uncomment for SGMII forced (non-AutoNeg) mode devices */
#endif

#ifdef  PHYMOD_BRONCOS_SUPPORT
#define PHYMOD_PHY542XX_SUPPORT
#endif

#ifdef  PHYMOD_RAVENS_SUPPORT
#define PHYMOD_PHY542XX_SUPPORT
#endif

#ifdef  PHYMOD_GIANTS_SUPPORT
#define PHYMOD_PHY542XX_SUPPORT
#endif

#ifdef  PHYMOD_RAMS_SUPPORT
#define PHYMOD_PHY542XX_SUPPORT
#endif

#ifdef  PHYMOD_COWBOYS_SUPPORT
#define PHYMOD_PHY542XX_SUPPORT
#endif

/************************************************************************************************************************
 *  Define the debug log level for entire system
 *  User has to set PHYMOD_LOG_SEVERITY_LEVEL to desired level to select the debug log level
 *  Supported debug log levels:
 *  0 - PHYMOD_LOG_SEVERITY_NO_INFO     : No information message is displayed while initialization
 *                                        except error and diagnostics API log
 *  1 - PHYMOD_LOG_SEVERITY_CRITICAL_INFO : Display critical information that provides top level view of the
 *                                          phy chip initialization. This is enabled by default.
 *  2 - PHYMOD_LOG_SEVERITY_DEBUG_INFO    : This will enable complete debugging info log. Recommended for debug purposes.
 *    Log priority 0, 1 and 2 denote priority from highest to lowest. For example, when PHYMOD_LOG_SEVERITY_LEVEL is set to
 *    PHYMOD_LOG_SEVERITY_DEBUG_INFO, log for higher priority (PHYMOD_LOG_SEVERITY_CRITICAL_INFO) will be enabled as well,
 *     which will include critical information as well as complete debug information.
 ************************************************************************************************************************
*/
#ifdef BCM_PLP_CAPI_LOG_SUPPORT
#define PHYMOD_LOG_SEVERITY_LEVEL                  PHYMOD_LOG_SEVERITY_CRITICAL_INFO
#else
#define PHYMOD_LOG_SEVERITY_LEVEL                  PHYMOD_LOG_SEVERITY_NO_INFO
#endif


/********************************************************************************************************************
 *  Debug level for sub layer is defined globally to select which layer debugging log need to be enabled selectively.
 *  it is enabled for selected layers if PHYMOD_LOG_SEVERITY_LEVEL is set to PHYMOD_LOG_SEVERITY_DEBUG_INFO.
 *  Following sub-layers are defined as bit field:
 *  DEBUG_BCM  0x1             : BCM layer log
 *  DEBUG_PHYMOD   0x2         : PHYMOD level log
 *  DEBUG_DRIVER 0x4           : Chip specific driver log
 *  DEBUG_MISC  0x8            : Firmware, SerDes, etc. log
 *  DEBUG_BUS        0x10      : DEBUG MDIO BUS read/write
 *  DEBUG_DEV    0x20          : Development debug info
 * e.g. #define DEBUG_ALL       (DEBUG_BCM | DEBUG_PHYMOD | DEBUG_DRIVER | DEBUG_MISC)
 *
 ********************************************************************************************************************
 */
#define DEBUG_ALL       ( DEBUG_BCM | DEBUG_PHYMOD | DEBUG_DRIVER | DEBUG_MISC )

#define PHYMOD_DEBUG_SUB_LEVEL   DEBUG_ALL

#if ((PHYMOD_LOG_SEVERITY_LEVEL >= PHYMOD_LOG_SEVERITY_DEBUG_INFO) && (PHYMOD_DEBUG_SUB_LEVEL & DEBUG_MISC))
#if defined(PHYMOD_MILLENIO_SUPPORT) || defined(PHYMOD_MILLENIOB_SUPPORT)
#define ENABLE_CAPI_LOG
#endif
#if defined(PHYMOD_BARCHETTA2_SUPPORT)
#define ENABLE_CAPI_LOG
#define ENABLE_CMD_SANITY_CHECK
#endif
#endif

#ifdef PHYMOD_BARCHETTA2_SUPPORT
#define ENABLE_PCS_SCRM_IDLE_GEN
#endif

#ifndef BCM_PLP_CAPI_LOG_SUPPORT
#define PHYMOD_PRINT        
#define PHYMOD_LOG_TIME    
#define PHYMOD_FILE_NAME   
#define PHYMOD_LINE_NUMBER 
#define PHYMOD_FUNCTION    
#else
#define PHYMOD_PRINT        printf    
#define PHYMOD_LOG_TIME       __TIME__
#define PHYMOD_FILE_NAME    __FILE__
#define PHYMOD_LINE_NUMBER   __LINE__ 
#define PHYMOD_FUNCTION       __func__
#endif
#define PHYMOD_FILE          FILE

#define my_malloc(x,y) malloc(x)
#define my_free(x)     free(x)

#ifdef WIN32
#undef interface
#define PHYMOD_USLEEP Sleep_us
#define PHYMOD_SLEEP  Sleep_ms
#define PHYMOD_MALLOC my_malloc
#define PHYMOD_FREE   my_free
typedef int int32_t ;
typedef unsigned int uint32 ;
typedef char int8_t ;
typedef short int16_t ;
typedef unsigned short uint16_t ;
#define __func__ __FUNCTION__
#else
#ifndef BCM_PLP_CAPI_ARMCC_SUPPORT
#define PHYMOD_USLEEP usleep
#define PHYMOD_SLEEP  sleep
#else
#define PHYMOD_USLEEP  udelay 
#define PHYMOD_SLEEP   delay
#endif
#define PHYMOD_MALLOC my_malloc
#define PHYMOD_FREE   my_free
#endif
#ifdef SERDES_API_FLOATING_POINT
   typedef float  phymod_plp_float_t;
#else
   typedef int  phymod_plp_float_t;
#endif
#ifdef BCM_PLP_CAPI_ARMCC_SUPPORT
static void udelay(unsigned int data)
{
    /*Add udelay code*/
}
static void delay(int data)
{
    /*Add delay code*/
}
#endif
typedef unsigned char       uint8 ;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;
typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;

#define _SOC_MSG(X)   X

#if ! defined(_SOC_EXIT_WITH_ERR)
#define _SOC_EXIT_WITH_ERR(E_CODE, STMT)  do {  \
                            PHYMOD_PRINT STMT;        \
                            return E_CODE;      \
                        } while ( 0 )
#endif

/* These functions map directly to Standard C functions */
#define PHYMOD_STRCMP    (strcmp)
#define PHYMOD_MEMSET    (memset)
#define PHYMOD_MEMCPY    (memcpy)
#define PHYMOD_MEMCMP    (memcmp)
#define PHYMOD_STRNCMP   (strncmp)
#define PHYMOD_STRCHR    (strchr)
#define PHYMOD_STRSTR    (strstr)
#define PHYMOD_STRLEN    (strlen)
#define PHYMOD_STRCPY    (strcpy)
#define PHYMOD_STRNCPY   (strncpy)
#define PHYMOD_STRNCAT   (strncat)
#define PHYMOD_STRCAT    (strcat)
#define PHYMOD_STRTOUL   (strtoul)
#define PHYMOD_STRTOL    (strtol)
#define PHYMOD_STRTOLL   (strtoll)
#define PHYMOD_IF_MALLOC (malloc)
#define PHYMOD_IF_FREE   (free)
#define PHYMOD_IF_MEMCPY (memcpy)
#define PHYMOD_IF_MEMSET (memset)
#define PHYMOD_SPRINTF   (sprintf)
#define PHYMOD_SNPRINTF  (snprintf)
#define PHYMOD_GETENV    (getenv)
#define PHYMOD_ATOI      (atoi)
#define PHYMOD_FOPEN     (fopen)
#define PHYMOD_FCLOSE    (fclose)
#define PHYMOD_FPRINTF   (fprintf)
#define PHYMOD_STRTOK    (strtok)
#define PHYMOD_FGETS     (fgets)
#define PHYMOD_ISSPACE   (isspace)
#define PHYMOD_LOG10     (log10)
#define PHYMOD_SQRT      (sqrt)

/* Defining packed attribute for compiler to disable structure padding */
#define COMPILER_PACKED_ATTRIBUTE  __attribute__((packed))

/* No need to define size_t */
#define PHYMOD_CONFIG_DEFINE_SIZE_T     0

#define COMPILER_64_TO_32_LO(dst, src)  ((dst) = (uint32) (src))
#define COMPILER_64_TO_32_HI(dst, src)  ((dst) = (uint32) ((src) >> 32))
#define COMPILER_64_HI(src)     ((uint32) ((src) >> 32))
#define COMPILER_64_LO(src)     ((uint32) (src))
#define COMPILER_64_ZERO(dst)       ((dst) = 0)
#define COMPILER_64_IS_ZERO(src)    ((src) == 0)

#define COMPILER_64_SET(dst, src_hi, src_lo)   \
        ((dst) = (((uint64) ((uint32)(src_hi))) << 32) | ((uint64) ((uint32)(src_lo))))

#define COMPILER_64_COPY(dst, src)  ((dst) = (src))

#define COMPILER_64_AND(dst, src)   ((dst) &= (src))
#define COMPILER_64_OR(dst, src)    ((dst) |= (src))
#define COMPILER_64_XOR(dst, src)   ((dst) ^= (src))
#define COMPILER_64_NOT(dst)        ((dst) = ~(dst))

#define COMPILER_64_LT(src1, src2)    ((src1) < (src2))
#define COMPILER_64_GE(src1, src2)    ((src1) >= (src2))
#define COMPILER_64_SHL(dst, bits)    ((dst) <<= (bits))
#define COMPILER_64_SHR(dst, bits)    ((dst) >>= (bits))
#define COMPILER_64_ADD_64(dst, src)  ((dst) += (src))
#define COMPILER_64_UDIV_64(dst, src) ((dst) /= (src))
#define COMPILER_64_UMUL_32(dst, src) ((dst) *= (src))
#define COMPILER_64_ADD_32(dst,src)   ((dst) += (src))
#define HWORD_SZ                        16
#define HWORD_MSK                       0xFFFF
#define COMPILER_32_HI(_v)              ((uint16_t) ((_v) >> HWORD_SZ))
#define COMPILER_32_LO(_v)              ((uint16_t) ((_v) & HWORD_MSK))
#define COMPILER_32_16(_hi,_lo)         ( (((uint32_t)(_hi)) << HWORD_SZ) | \
                                           ((uint32_t)(_lo)) )
#define COMPILER_32_SET(_v,_hi,_lo)     ((_v) = COMPILER_32_16(_hi,_lo))
#define COMPILER_64_HWORD0              0
#define COMPILER_64_HWORD1              16
#define COMPILER_64_HWORD2              32
#define COMPILER_64_HWORD3              48
#define COMPILER_64to16(_v,_s)          ((uint16_t) (((_v) >> (_s)) & HWORD_MSK))




#endif /* __plp_aperta2_PHYMOD_CUSTOM_CONFIG_H__ */
