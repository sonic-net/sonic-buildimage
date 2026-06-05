/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#ifndef PHYMOD_LOG_H
#define PHYMOD_LOG_H

#ifdef WIN32
#define PLP_INLINE __inline
#else
#ifdef BCM_PLP_CAPI_ARMCC_SUPPORT
#define PLP_INLINE
#else
#define PLP_INLINE inline
#endif 
#endif

/* Define the debug log level for entire system
  *
  * User has to set PHYMOD_LOG_SEVERITY_LEVEL to desired level to select the debug log level
  *
  */
#define PHYMOD_LOG_SEVERITY_NO_INFO                          0     /* No information message is displayed while initialization
                                                                      except error and diagonostics API log */
#define PHYMOD_LOG_SEVERITY_CRITICAL_INFO                    1     /* Display critical information that provides top level view of the
                                                                      phy chip initialization. This is enabled by default. */
#define PHYMOD_LOG_SEVERITY_DEBUG_INFO                       2     /* This will enable complete debug info */


/*************************************************************************************
  *
  *  Sub Layer debugging levels, debug levels can be enabled/disabled at sublayer level
  *  e.g. BCM layer debugging can be enabled by passing sub level DEBUG_BCM
  *
  *************************************************************************************/
#define DEBUG_BCM         0x1   /* BCM layer log */
#define DEBUG_PHYMOD      0x2   /* PHYMOD level log */
#define DEBUG_DRIVER      0x4   /* Chip specific driver log */
#define DEBUG_MISC        0x8   /* Firmware, SerDes, etc. log */
#define DEBUG_BUS         0x10  /* Debug log forMDIO BUS read/write */
#define DEBUG_DEV         0x20  /* Development debug info */
#include <phymod_custom_config.h>

extern char plp_barchetta_log_level_string [4][20];

static PLP_INLINE int find_sub_level(char *str_, int *log_pos)
{
    if(NULL!=PHYMOD_STRSTR(str_,"phymod/chip/")){
        *log_pos = 2;
        return DEBUG_DRIVER;
    } else if (NULL!=PHYMOD_STRSTR(str_,"phymod/core/phymod_acc.c")) {
        *log_pos = 3;
        return DEBUG_BUS;
    } else if (NULL!=PHYMOD_STRSTR(str_,"phymod/")) {
        *log_pos = 1;
        return DEBUG_PHYMOD;
    } else if (NULL!=PHYMOD_STRSTR(str_,"bcm_")) {
        *log_pos = 0;
        return DEBUG_BCM;
    } else {
        return DEBUG_MISC;
    }
}

static PLP_INLINE int check_sub_level(int chk_){
    if(chk_ & PHYMOD_DEBUG_SUB_LEVEL)
        return 1;
    else
        return 0;
}

#ifndef phymod_log_formatted_message
void
phymod_log_formatted_message(const char * szFormat_p,
                                       ... );
#endif

/*  User can define their version of formated message  */
#define phymod_log_formatted_message    PHYMOD_PRINT

#ifdef BCM_PLP_CAPI_LOG_SUPPORT
#define PHYMOD_DEBUG_ERROR(stuff_)     { \
                                            phymod_log_formatted_message ("%s: %s: Line:%5d ",PHYMOD_FILE_NAME, PHYMOD_FUNCTION, PHYMOD_LINE_NUMBER); \
                                            phymod_log_formatted_message stuff_ ; \
                                        }
#define PHYMOD_DIAG_OUT(stuff_)        phymod_log_formatted_message stuff_ ;
#else
#define PHYMOD_DEBUG_ERROR(stuff_)
#define PHYMOD_DIAG_OUT(stuff_) 
#endif


#if PHYMOD_LOG_SEVERITY_LEVEL >= PHYMOD_LOG_SEVERITY_CRITICAL_INFO
#define PHYMOD_CRIT_INFO(stuff_)       phymod_log_formatted_message stuff_ ;
#else
#define PHYMOD_CRIT_INFO(stuff_)
#endif

#if PHYMOD_LOG_SEVERITY_LEVEL >= PHYMOD_LOG_SEVERITY_DEBUG_INFO
#define PHYMOD_DEBUG_LOG_INFO(chk_, stuff_) do {              \
        if (check_sub_level(chk_)) {             \
            phymod_log_formatted_message stuff_ ; \
        }                                       \
    } while (0)
#else
#define PHYMOD_DEBUG_LOG_INFO(chk_, stuff_)
#endif

/*
 * With the default Severity Level for Logging -
 * we can live without logging non CRITICAL errors.
 * But individual PHY driver features, shall return error
 * values on all error cases. User can set the Severity Level >=
 * PHYMOD_LOG_SEVERITY_DEBUG_INFO to see INFO and ERRORs
 */
#if PHYMOD_LOG_SEVERITY_LEVEL >= PHYMOD_LOG_SEVERITY_DEBUG_INFO
#define PHYMOD_DEBUG_INFO(stuff_) do {                  \
        int chk_,log_pos=0;                                       \
        chk_ = find_sub_level(PHYMOD_FILE_NAME, &log_pos);        \
        if (check_sub_level(chk_)) {                    \
            phymod_log_formatted_message ("[%s] %s: Line:%5d ",plp_barchetta_log_level_string[log_pos],\
            PHYMOD_FUNCTION,PHYMOD_LINE_NUMBER); \
            phymod_log_formatted_message stuff_ ;       \
        }                                               \
    } while (0)
#define PHYMOD_DEBUG_WARN_ERROR(stuff_)     { \
                                            phymod_log_formatted_message ("%s: %s: Line:%5d ",PHYMOD_FILE_NAME, PHYMOD_FUNCTION, PHYMOD_LINE_NUMBER); \
                                            phymod_log_formatted_message stuff_ ; \
                                        }
#else
#define PHYMOD_DEBUG_INFO(stuff_)
#define PHYMOD_DEBUG_WARN_ERROR(stuff_)
#endif

/* These are helper macros that can be used at respective layer to facilitate in
   checking sub-level */
/* Debug internal message */
#define PHYMOD_DEBUG_DEV_INFO(stuff_)         PHYMOD_DEBUG_LOG_INFO(DEBUG_DEV, stuff_)
/* FW and SERDES debug log wrapper functions */
#define FW_DEBUG_INFO(stuff_)                      PHYMOD_DEBUG_LOG_INFO(DEBUG_MISC, stuff_)

#define SERDES_DEBUG_INFO(stuff_)                  PHYMOD_DEBUG_LOG_INFO(DEBUG_MISC, stuff_)

#endif   /* PHYMOD_LOG_H */
