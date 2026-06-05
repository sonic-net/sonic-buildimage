/*
 *
 * $Id:$
 *
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 *
 *
 *
 */

#ifndef _PORTMOD_DEFS_H__H_
#define _PORTMOD_DEFS_H__H_

#include <phymod/phymod.h>
#include "plp_soc_portmod.h"


#define MAX_LANES_PER_PORT (24)
#define MAX_PHYN (3)
#define MAX_NUM_CORES (6)

typedef int ilkn_retransmit_config_t;
typedef int phymod_firmware_mode_t;
typedef soc_port_ability_t portmod_port_ability_t;
typedef soc_port_mode_t portmod_port_mode_t;

typedef  unsigned int portmod_pa_encap_t;


typedef struct portmod_lanes_assign_info_s{
    uint32_t valid_entries;
    uint32_t lane_assign[MAX_LANES_PER_PORT];
}portmod_lanes_assign_info_t;

#endif /*_PORTMOD_DEFS_H__H_*/
