/*
 *         
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

/* 
 * $Id: phymod_diag.c $ 
 */
#include <phymod/phymod_util.h>
#ifdef PHYMOD_APERTA_SUPPORT
#include "phymod_dump_reg.h"
#endif

#ifdef PHYMOD_APERTA_SUPPORT
int plp_aperta_phymod_convert_dump_to_txt(char *file_name, char *die, char *in_verbosity) 
{
    char templatefile[PHYMOD_UTIL_STRING_MAX_SIZE];
    char dictfile[PHYMOD_UTIL_STRING_MAX_SIZE];
    char dumpfile[PHYMOD_UTIL_STRING_MAX_SIZE+1];
    char die_selection[5];
    char opfile[PHYMOD_UTIL_STRING_MAX_SIZE];
    char verbosity[3];
    PHYMOD_SPRINTF(templatefile,"%saperta_debug_display_template.txt",EPIL_UTIL_PATH);
    PHYMOD_SPRINTF(dictfile,"%saperta_reg_dict.py",EPIL_UTIL_PATH);
    PHYMOD_SPRINTF(opfile,"dbg_info.txt");
    if (PHYMOD_STRLEN(file_name) > PHYMOD_UTIL_STRING_MAX_SIZE) {
        PHYMOD_DEBUG_ERROR(("File name size exceeds\n"));
        return PHYMOD_E_PARAM;
    }
    PHYMOD_SPRINTF(dumpfile,"%s",file_name);
    PHYMOD_STRNCPY(die_selection, die, 1);
    PHYMOD_SPRINTF(verbosity,"%s",in_verbosity);
    die_selection[1] = '\0';

    plp_aperta_phymod_dump_display_debug_regs(templatefile, dumpfile, dictfile, die_selection, verbosity, opfile);
    return 0;

}
#endif

int plp_aperta_phymod_util_lane_config_get(const plp_aperta_phymod_access_t *phys, int *start_lane, int *num_of_lane)
{
    int i ;
    switch (phys->lane_mask) {
        case 0x1:
            *start_lane = 0;
            *num_of_lane = 1;
            break;
        case 0x2:
            *start_lane = 1;
            *num_of_lane = 1;
            break;
        case 0x4:
            *start_lane = 2;
            *num_of_lane = 1;
            break;
        case 0x8:
            *start_lane = 3;
            *num_of_lane = 1;
            break;
        case 0x3:
            *start_lane = 0;
            *num_of_lane = 2;
            break;
        case 0xc:
            *start_lane = 2;
            *num_of_lane = 2;
            break;
        case 0x7:
            *start_lane = 0;
            *num_of_lane = 3;
            break;
        case 0xf:
            *start_lane = 0;
            *num_of_lane = 4;
            break;
        case 0xff:
            *start_lane = 0;
            *num_of_lane = 8;
            break;
        case 0x10:
            *start_lane = 4;
            *num_of_lane = 1;
            break;
        case 0x20:
            *start_lane = 5;
            *num_of_lane = 1;
            break;
        case 0x30:
            *start_lane = 4;
            *num_of_lane = 2;
            break;
        case 0x40:
            *start_lane = 6;
            *num_of_lane = 1;
            break;
        case 0x80:
            *start_lane = 7;
            *num_of_lane = 1;
            break;
        case 0xc0:
            *start_lane = 6;
            *num_of_lane = 2;
            break;
        case 0x100:
            *start_lane = 8;
            *num_of_lane = 1;
            break;
        case 0x200:
            *start_lane = 9;
            *num_of_lane = 1;
            break;
        case 0x400:
            *start_lane = 10;
            *num_of_lane = 1;
            break;
        case 0x800:
            *start_lane = 11;
            *num_of_lane = 1;
            break;
        case 0x1000:
            *start_lane = 12;
            *num_of_lane = 1;
            break;
        case 0x2000:
            *start_lane = 13;
            *num_of_lane = 1;
            break;
        case 0x4000:
            *start_lane = 14;
            *num_of_lane = 1;
            break;
        case 0x8000:
            *start_lane = 15;
            *num_of_lane = 1;
            break;
        case 0x300:
            *start_lane = 8;
            *num_of_lane = 2;
            break;
        case 0xc00:
            *start_lane = 10;
            *num_of_lane = 2;
            break;
        case 0x3000:
            *start_lane = 12;
            *num_of_lane = 2;
            break;
        case 0xc000:
            *start_lane = 14;
            *num_of_lane = 2;
            break;
        case 0xFF00:
            *start_lane = 8;
            *num_of_lane = 8;
            break;
        case 0xF00:
            *start_lane = 8;
            *num_of_lane = 4;
            break;
        case 0xF000:
            *start_lane = 12;
            *num_of_lane = 4;
            break;

        default:
            /*Support non-consecutive lanes*/
            for(i = 0; i < 16; i++)
            {
                if(phys->lane_mask & (1 << i))
                {
                    *start_lane = i;
                    break;
                }
            }
            *num_of_lane = 4;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_phymod_swap_bit(uint16_t original_value, uint16_t *swapped_val)
{
    uint16_t new_val, temp_val;
    uint16_t i;

    new_val = 0;
    for (i = 0; i < 16; i++) {
        temp_val = 0x0;
        temp_val = (original_value & (0x1 << (15 - i))) >> (15 -i);
        temp_val = (temp_val << i) & (0x1 << i);
        new_val |= temp_val;
   }
   *swapped_val = new_val;
    return PHYMOD_E_NONE;
}

int plp_aperta_phymod_count_set_bits(int data)
{
    /* Take alternalte bits*/
    data = data - ((data >> 1) & 0x55555555);

    /* Take consecutive bits*/
    data = (data & 0x33333333) + ((data >> 2) & 0x33333333);

    /* Get the corner*/
    data = ((data + (data>> 4)) & 0x0F0F0F0F);

    return (data * 0x01010101) >> 24;
}

unsigned char plp_aperta_phymod_log2n(unsigned int n)
{
    return ((n > 1) ? (1 + plp_aperta_phymod_log2n(n / 2)) : 0);
}

int plp_aperta_phymod_util_lane_mask_get(int start_lane, int num_of_lane, uint32_t *lane_mask)
{
    int i ;
    uint32_t temp_mask = 0;

    for (i = 0; i <num_of_lane; i++) {
        temp_mask |= 1 << (start_lane + i);
    }

    *lane_mask = temp_mask;

    return PHYMOD_E_NONE;
}

int plp_aperta_phymod_custom_diag_dump_hdr(const plp_aperta_phymod_phy_access_t *phy)
{
    PHYMOD_DIAG_OUT(("PHY  SIDE  LANE_MAP  "));
    PHYMOD_DIAG_OUT(("PCS_SYNC  PCS_BLK_LOCK  PCS_RX_FAULT  PCS_TX_FAULT  PMD_RX_FAULT  PMD_TX_FAULT  HI_BER  BER_CNT  DELAY_SKEW"));
    PHYMOD_DIAG_OUT(("\n"));
    return (PHYMOD_E_NONE);
}

int plp_aperta_phymod_custom_diag_dump_display(const plp_aperta_phymod_phy_access_t *phy, plp_aperta_phymod_phy_diagnostics_t *phy_diag)
{
    PHYMOD_DIAG_OUT(("%2d   ", phy->access.addr));
    PHYMOD_DIAG_OUT(("%-4s  0x%-4x    ", PHYMOD_IS_SYSTEM_SIDE(phy) ? "SYS ": "LINE", phy->access.lane_mask));
    PHYMOD_DIAG_OUT(("%5d     %7d       %6d        %6d        ", phy_diag->pcs_sync, phy_diag->pcs_blk_lock, phy_diag->pcs_rx_fault, phy_diag->pcs_tx_fault));
    PHYMOD_DIAG_OUT(("%6d        %6d        ", phy_diag->pmd_rx_fault, phy_diag->pmd_tx_fault));
    PHYMOD_DIAG_OUT(("%4d    %4d     ", phy_diag->hi_ber, phy_diag->ber_cnt));
    PHYMOD_DIAG_OUT(("%7d", phy_diag->delay_skew));
    PHYMOD_DIAG_OUT(("\n"));
    return PHYMOD_E_NONE;
}
