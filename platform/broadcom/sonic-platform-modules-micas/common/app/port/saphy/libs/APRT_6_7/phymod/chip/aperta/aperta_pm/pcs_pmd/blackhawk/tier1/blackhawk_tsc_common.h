/********************************************************************************
 ********************************************************************************
 *                                                                              *
 *  Revision      :   *
 *                                                                              *
 *  Description   :  Defines and Enumerations required by Serdes APIs           *
 *                                                                              *
 * $Copyright: (c) 2020 Broadcom.                                               *
 * Broadcom Proprietary and Confidential. All rights reserved.$                 *
 ********************************************************************************
 ********************************************************************************/

/** @file blackhawk_tsc_common.h
 * Defines and Enumerations shared across M16/F16/BHK16 APIs BUT NOT MICROCODE
 */

#ifndef BLACKHAWK_TSC_API_COMMON_H
#define BLACKHAWK_TSC_API_COMMON_H

#include "blackhawk_tsc_ipconfig.h"

/** Macro to determine sign of a value */
#define sign(x) ((x>=0) ? 1 : -1)

#define UCODE_MAX_SIZE (84*1024)

/*
 * IP-Specific Iteration Bounds
 */
#   define DUAL_PLL_NUM_PLLS  2

#endif
