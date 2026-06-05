/*------------------------------------------------------------------------------
 * $Id: aperta_tbhmod_spd_ctrl.h $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *------------------------------------------------------------------------------
 *  Description: Structures and function prototypes for Speed Table, AM Table,
 *               and UM Table access.
 *----------------------------------------------------------------------------*/
#ifndef _APERTA_TBHMOD_SPD_CTRL_H_
#define _APERTA_TBHMOD_SPD_CTRL_H_

#ifdef _DV_TB_
#include <stdint.h>
#endif
#include <phymod/phymod.h>
#include "aperta_tbhmod_spd_ctrl_defines.h"

typedef struct
{
  uint8_t  am_table_index;
  uint8_t  fec_arch;
  uint8_t  symbol_interleave;
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
  uint16_t credit_remainder;
  uint16_t credit_divisor;
  uint8_t  use_cl49_block_sync;
  uint8_t  scr_mode;
  uint8_t  codec_mode;
  uint8_t  training_en;
  uint8_t  an_timer_select;
  uint8_t  pmd_pam4_mode;
  uint8_t  pmd_osr_mode;
  uint8_t  t_pma_watermark;
  uint8_t  l_tpma_watermark;
  uint8_t  loop_block_count_for_ts;
  uint16_t loop_bit_count_for_ts;
  uint8_t  am_lock_fsm_mode;
  uint8_t  t_pma_start_mode;
  uint8_t  hi_ser_enable;
} spd_id_tbl_entry_t;

typedef struct
{
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
} am_tbl_entry_t;

typedef struct
{
  uint32_t um;
  uint8_t  cm_pad;
  uint8_t  um_pad;
  uint8_t  cm_control;
} um_tbl_entry_t;


/* Function Prototypes */
extern void                 plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(uint32_t *packed_entry, spd_id_tbl_entry_t *entry);
extern void                 plp_aperta_spd_ctrl_pack_spd_id_tbl_entry(spd_id_tbl_entry_t *entry, uint32_t *packed_entry);

extern void                 plp_aperta_spd_ctrl_unpack_am_tbl_entry(uint32_t *packed_entry, am_tbl_entry_t *entry);
extern void                 plp_aperta_spd_ctrl_pack_am_tbl_entry(am_tbl_entry_t *entry, uint32_t *packed_entry);

extern void                 plp_aperta_spd_ctrl_unpack_um_tbl_entry(uint32_t *packed_entry, um_tbl_entry_t *entry);
extern void                 plp_aperta_spd_ctrl_pack_um_tbl_entry(um_tbl_entry_t *entry, uint32_t *packed_entry);
#endif  /* _APERTA_TBHMOD_SPD_CTRL_H_ */
