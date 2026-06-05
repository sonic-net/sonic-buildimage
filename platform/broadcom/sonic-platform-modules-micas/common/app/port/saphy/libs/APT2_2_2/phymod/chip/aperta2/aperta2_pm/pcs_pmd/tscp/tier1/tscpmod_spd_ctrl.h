/*------------------------------------------------------------------------------
 * $Id: tscpmod_spd_ctrl.h $
 *
 * $Copyright: (c) 2017 Broadcom Corporation All Rights Reserved.$
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *------------------------------------------------------------------------------
 *  Description: Structures and function prototypes for Speed Table, AM Table,
 *               and UM Table access.
 *----------------------------------------------------------------------------*/
#ifndef _TSCPMOD_SPD_CTRL_H_
#define _TSCPMOD_SPD_CTRL_H_ 

#include <phymod/phymod.h>
#include "tscpmod_spd_ctrl_defines.h"

typedef struct 
{
  uint8_t  am_table_index;
  uint8_t  fec_arch;
  uint8_t  tc_xor_control;
  uint8_t  rs_fec_sync_header_mode;
  uint8_t  rs_fec_cwm_nibble_match_count;
  uint8_t  base_r_fec_sync_header_mode;
  uint8_t  rs_fec_symbol_error_window_mode;
  uint16_t rs_fec_symbol_error_count_threshold;
  uint8_t  deskew_forwarding_threshold_2xn;
  uint8_t  deskew_forwarding_threshold_1xn;
  uint8_t  bit_mux_mode;
  uint8_t  num_lanes;
  uint8_t  deskew_window_without_rs_fec;
  uint8_t  deskew_window_with_rs_fec;
  uint8_t  ber_fsm_disable_with_rs_fec;
  uint8_t  ber_fsm_disable_without_rs_fec;
  uint8_t  ber_window_mode;
  uint8_t  ber_trigger_count;
  uint8_t  credit_quotient;
  uint8_t  use_cl49_block_sync;
  uint8_t  scr_mode;
  uint8_t  codec_mode;
  uint8_t  training_en;
  uint8_t  scrambler_dis;
  uint8_t  an_timer_select;
  uint8_t  t_pma_watermark;
  uint8_t  loop_block_count_for_ts;
  uint16_t loop_bit_count_for_ts;
  uint8_t  am_lock_fsm_mode;
  uint8_t  t_pma_start_mode;
  uint8_t  hi_ser_enable;
  uint8_t  t_pma_input_width_mode;
  uint8_t  t_pma_output_width_mode;
  uint8_t  hi_ser_2;
  uint8_t  t_pma_tdm;
} tscpmod_spd_id_tbl_entry_t;

typedef struct
{
  uint8_t  um_modifier_mpp0;
  uint8_t  um_modifier_mpp1;
  uint8_t  am_drop_control;
  uint8_t  cm_am_sel;
  uint8_t  am_spacing_credits;
  uint8_t  am_spacing_no_rs;
  uint16_t am_spacing_rs;
  uint8_t  use_fixed;
  uint32_t cm;
  uint8_t  um_table_index;
  uint8_t  um_table_count;
  uint8_t  pad_mode;
  uint8_t  am_size;
  uint8_t  am_compression_mode;
} tscpmod_am_tbl_entry_t;

typedef struct
{
  uint32_t um;
  uint8_t  cm_pad;
  uint8_t  um_pad;
  uint8_t  cm_control;
} tscpmod_um_tbl_entry_t;


/* Function Prototypes */
extern void                 plp_aperta2_tscpmod_spd_ctrl_unpack_spd_id_tbl_entry(uint32_t *packed_entry, tscpmod_spd_id_tbl_entry_t *entry);
extern void                 plp_aperta2_tscpmod_spd_ctrl_pack_spd_id_tbl_entry(tscpmod_spd_id_tbl_entry_t *entry, uint32_t *packed_entry);

extern void                 plp_aperta2_tscpmod_spd_ctrl_unpack_am_tbl_entry(uint32_t *packed_entry, tscpmod_am_tbl_entry_t *entry);
extern void                 plp_aperta2_tscpmod_spd_ctrl_pack_am_tbl_entry(tscpmod_am_tbl_entry_t *entry, uint32_t *packed_entry);

extern void                 plp_aperta2_tscpmod_spd_ctrl_unpack_um_tbl_entry(uint32_t *packed_entry, tscpmod_um_tbl_entry_t *entry);
extern void                 plp_aperta2_tscpmod_spd_ctrl_pack_um_tbl_entry(tscpmod_um_tbl_entry_t *entry, uint32_t *packed_entry);

#endif  /* _TBHMOD_SPD_CTRL_H_ */
