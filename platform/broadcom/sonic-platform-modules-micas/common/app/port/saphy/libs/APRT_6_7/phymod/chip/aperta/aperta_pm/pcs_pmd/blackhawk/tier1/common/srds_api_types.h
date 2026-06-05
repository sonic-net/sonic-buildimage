/*****************************************************************************************
 *****************************************************************************************
 *                                                                                       *
 *  Description   :  Functions used internally and available in debug shell only         *
 *                                                                                       *
 * $Copyright: (c) 2020 Broadcom.                                               *
 * Broadcom Proprietary and Confidential. All rights reserved.$                 *
 *****************************************************************************************/

/** @file srds_api_types.h
 * Common types used by Serdes API functions
 * This file contains types which are common to all serdes cores
 */

#ifndef SRDS_API_TYPES_H
#define SRDS_API_TYPES_H

#include <phymod/phymod.h>
#include <phymod/phymod_config.h>

/*
struct srds_access_s;
typedef struct srds_access_s srds_access_t;    */

/* Opaque SerDes Mutex Struct to be defined by Upper level Software */
/*struct srds_mutex_s;
typedef struct srds_mutex_s srdes_mutex_t;
*/

typedef plp_aperta_phymod_phy_access_t srds_access_t;

#ifdef phymod_mutex_t
typedef phymod_mutex_t srds_mutex_t;
#else
typedef void* srds_mutex_t;
#endif

typedef struct {
    uint32_t signature;
    uint32_t diag_mem_ram_base;
    uint32_t diag_mem_ram_size;
    uint32_t core_var_ram_base; /* Must be the same as CORE_VAR_RAM_BASE */
    uint32_t core_var_ram_size;
    uint32_t lane_var_ram_base; /* Must be the same as LANE_VAR_RAM_BASE */
    uint32_t lane_var_ram_size; /* Must be the same as LANE_VAR_RAM_SIZE */
    uint32_t trace_mem_ram_base;
    uint32_t trace_mem_ram_size;
    uint32_t micro_var_ram_base;
    uint8_t  lane_count; /* lanes per micro */
    uint8_t  trace_memory_descending_writes;
    uint8_t  micro_count;
    uint8_t  micro_var_ram_size;
    uint16_t grp_ram_size;
    uint32_t ucode_version;
} srds_info_t;

#endif
