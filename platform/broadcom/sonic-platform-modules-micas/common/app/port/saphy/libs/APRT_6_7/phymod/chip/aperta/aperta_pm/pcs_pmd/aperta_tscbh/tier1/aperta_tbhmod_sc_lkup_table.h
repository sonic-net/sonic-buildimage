/*----------------------------------------------------------------------
 * $Id: aperta_tbhmod_sc_lkup_table.c, $
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *----------------------------------------------------------------------
 *  Description: define enumerators
 *----------------------------------------------------------------------*/
#ifndef aperta_tbhmod_sc_lkup_table_H_
#define aperta_tbhmod_sc_lkup_table_H_

#ifndef _SDK_APERTA_TBHMOD_
#ifndef _DV_TB_
 #define _SDK_APERTA_TBHMOD_ 1
#endif
#endif

#ifdef _SDK_APERTA_TBHMOD_
#include "aperta_tbhmod_enum_defines.h"
#include "aperta_tbhmod.h"
#include <phymod/phymod.h>
#endif
#include "tbhPCSRegEnums.h"
#include "aperta_tbhmod_sc_defines.h"

#ifndef _SDK_APERTA_TBHMOD_
#define PHYMOD_ST aperta_tbhmod_dv_st
#endif

#define APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE   64
#define APERTA_TSCBH_HW_AM_TABLE_SIZE    64
#define APERTA_TSCBH_HW_UM_TABLE_SIZE    64

#define APERTA_TSCBH_SPEED_ID_TABLE_SIZE  76
#define APERTA_TSCBH_SPEED_ID_ENTRY_SIZE  5
#define APERTA_TSCBH_AM_TABLE_SIZE  64
#define APERTA_TSCBH_AM_ENTRY_SIZE  3
#define APERTA_TSCBH_UM_TABLE_SIZE  64
#define APERTA_TSCBH_UM_ENTRY_SIZE  2

#define APERTA_TSCBH_SPEED_PRIORITY_MAPPING_TABLE_SIZE 1
#define APERTA_TSCBH_SPEED_PRIORITY_MAPPING_ENTRY_SIZE 9


typedef struct sc_pcs_entry_t
{
  int num_lanes;
  int os_mode;
  int t_fifo_mode;
  int t_enc_mode;
  int t_HG2_ENABLE;
  int t_pma_btmx_mode;

  int scr_mode;
  int descr_mode;
  int dec_tl_mode;
  int deskew_mode;
  int dec_fsm_mode;
  int r_HG2_ENABLE;

  int bs_sm_sync_mode;
  int bs_sync_en;
  int bs_dist_mode;
  int bs_btmx_mode;
  int cl72_en;

/*
  Notes:
    1. When the loop_cnt1 == 0, the clk_cnt1 is a don't care
    2. the pcs crd/cnt are valid only when byte replication is valid, say for 10M, 100M cases
*/
  /* credit settings*/

  int clkcnt0;
  int clkcnt1;
  int lpcnt0;
  int lpcnt1;
  int cgc;

/*  New SC override fields for GEN2 */

  int t_cl91_cw_scr;
  int t_5bit_xor_en;
  int t_pma_cl91_mux_sel;
  int t_pma_watermark;
  int t_pma_btmx_delay;
  int am_spacing_mul;
  int cl91_blksync_mode;
  int t_merge_mode;
  int r_cl91_cw_scr;
  int r_tc_in_mode;
  int r_tc_mode;
  int r_5bit_xor_en;
  int r_tc_out_mode;
  int r_HG2_en;
  int r_merge_mode;
  int USE_100G_AM0;
  int USE_100G_AM123;
  int ber_window_sel;
  int ber_count_sel;
  int cl74_shcorrupt;
  int corrupt_6th_group;
  int corrupt_2nd_group;


  int clkcnt0_by48;
  int clkcnt1_by48;
  int lpcnt0_by48;
  int lpcnt1_by48;
  int cgc_by48;

} sc_pcs_entry;


typedef struct sc_pmd_entry_t
{
  int t_pma_os_mode;
  int pll_mode;
} sc_pmd_entry_st;

extern const sc_pmd_entry_st plp_aperta_tscbh_sc_pmd_entry[];
extern const sc_pmd_entry_st plp_aperta_tscbh_sc_pmd_entry_312M_ref[];
extern int cl91_valid_speed(PHYMOD_ST* pc); 
int print_tbhmod_sc_lkup_table(PHYMOD_ST* pc);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_26_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_25_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_20_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_26_gsh_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_25_gsh_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_20_gsh_get(void);
extern uint32_t* plp_aperta_tscbh_am_table_entry_get(void);
extern uint32_t* plp_aperta_tscbh_um_table_entry_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_100g_4lane_no_fec_26_get(void);
extern uint32_t* aperta_tscbh_spd_id_entry_100g_4lane_no_fec_25_get(void);
extern uint32_t* plp_aperta_tscbh_speed_priority_mapping_table_get(void);


#endif
