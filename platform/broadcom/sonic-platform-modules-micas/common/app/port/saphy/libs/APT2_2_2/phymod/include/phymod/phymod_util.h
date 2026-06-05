/*
 *         
 * $Id: phymod_definitions.h,v 1.2.2.12 Broadcom SDK $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *         
 * Shell diagnostics of Phymod    
 *
 */

#ifndef _plp_aperta2_PHYMOD_UTIL_H_
#define _plp_aperta2_PHYMOD_UTIL_H_

#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#define PHYMOD_UTIL_STRING_MAX_SIZE    1000
#define PHYMOD_IS_SYSTEM_SIDE(PHY)        (PHY->port_loc == phymodPortLocSys)
#define PHYMOD_IS_LINE_SIDE(PHY)          (PHY->port_loc == phymodPortLocLine || PHY->port_loc == phymodPortLocDC)
/******************************************************************************
Functions
******************************************************************************/


int plp_aperta2_phymod_util_lane_config_get(const plp_aperta2_phymod_access_t *phys, int *start_lane, int *num_of_lane);
int plp_aperta2_phymod_swap_bit(uint16_t original_value, uint16_t *swapped_val);
int plp_aperta2_phymod_count_set_bits(int data);
unsigned char plp_aperta2_phymod_log2n(unsigned int n);
int plp_aperta2_phymod_util_lane_mask_get(int start_lane, int num_of_lane, uint32_t *lane_mask);
int plp_aperta2_phymod_custom_diag_dump_hdr(const plp_aperta2_phymod_phy_access_t *phy);
int plp_aperta2_phymod_custom_diag_dump_display(const plp_aperta2_phymod_phy_access_t *phy, plp_aperta2_phymod_phy_diagnostics_t *phy_diag);
#ifdef PHYMOD_APERTA_SUPPORT
int phymod_convert_dump_to_txt(char *file_name, char *die, char *verbosity);
#endif
#define _SHR_LANEBMP_WBIT(_lane)                  (1U << (_lane))
#define PHYMOD_LANEPBMP_MEMBER(_bmp, _lane)       (((_bmp) & (_SHR_LANEBMP_WBIT(_lane))) != 0)

#endif /*_plp_aperta2_PHYMOD_UTIL_H_*/
