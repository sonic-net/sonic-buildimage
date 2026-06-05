/*----------------------------------------------------------------------
 * $Id: tscpmod_sc_lkup_table.c, $
 * $Copyright: $
 *
 * $Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.$
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: define enumerators  
 *----------------------------------------------------------------------*/
#ifndef tscpmod_sc_lkup_table_H_
#define tscpmod_sc_lkup_table_H_ 

#include "tscpmod.h"
#include <phymod/phymod.h>

#define TSCPMOD_HW_SPEED_ID_TABLE_SIZE   64
#define TSCPMOD_HW_AM_TABLE_SIZE    64
#define TSCPMOD_HW_UM_TABLE_SIZE    64


#define TSCPMOD_SPEED_ID_TABLE_SIZE  97
#define TSCPMOD_SPEED_ID_ENTRY_SIZE  5
#define TSCPMOD_AM_TABLE_SIZE  64
#define TSCPMOD_AM_ENTRY_SIZE  3
#define TSCPMOD_UM_TABLE_SIZE  64
#define TSCPMOD_UM_ENTRY_SIZE  2

#define TSCPMOD_SPEED_PRIORITY_MAPPING_TABLE_SIZE 1
#define TSCPMOD_SPEED_PRIORITY_MAPPING_ENTRY_SIZE 9

typedef struct tscpmod_sc_pmd_entry_t {
    int t_pma_os_mode;
    int pll_mode;
} tscpmod_sc_pmd_entry_st;

extern const tscpmod_sc_pmd_entry_st plp_aperta2_tscpmod_sc_pmd_entry[];

extern int plp_aperta2_tscpmod_get_mapped_speed(tscpmod_spd_intfc_type_t spd_intf, int *speed);
extern uint32_t* plp_aperta2_tscp_spd_id_entry_get(void);
extern uint32_t* plp_aperta2_tscp_spd_id_entry_100g_4lane_no_fec_get(void);
extern uint32_t* plp_aperta2_tscp_am_table_entry_get(void);
extern uint32_t* plp_aperta2_tscp_um_table_entry_get(void);
extern uint32_t* plp_aperta2_tscp_speed_priority_mapping_table_get(void);
extern uint32_t* plp_aperta2_tscp_spd_id_entry_gsh_get(void);
extern int plp_aperta2_tscpmod_mapped_speed_get_osmode (int mapped_speed_id);

#endif
