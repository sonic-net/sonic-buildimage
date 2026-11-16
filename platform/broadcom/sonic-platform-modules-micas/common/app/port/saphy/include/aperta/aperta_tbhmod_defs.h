/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * aperta_tbhmod_defs.h - self-contained TSCBH (Blackhawk) speed-config tables
 ********************************************************************************/
#ifndef _APERTA_TBHMOD_DEFS_H
#define _APERTA_TBHMOD_DEFS_H

#include <stdint.h>

/* ==========================================================================
 * OSR (over-sampling rate) modes - aperta_tbhmod_sc_defines.h
 * ========================================================================== */
#define OS_MODE_1            0
#define OS_MODE_2            1
#define OS_MODE_4            2
#define OS_MODE_21p25        4
#define OS_MODE_16p5         8
#define OS_MODE_20p625       12

/* ==========================================================================
 * PLL divider modes (lower 8 bits = integer divider, upper bits = flags).
 * ========================================================================== */
#define APERTA_TBHMOD_PLL_MODE_DIV_ZERO     0x00000000
#define APERTA_TBHMOD_PLL_MODE_DIV_66       0x00000042
#define APERTA_TBHMOD_PLL_MODE_DIV_80       0x00000050
#define APERTA_TBHMOD_PLL_MODE_DIV_82P5     0x80000052
#define APERTA_TBHMOD_PLL_MODE_DIV_85       0x00000055
#define APERTA_TBHMOD_PLL_MODE_DIV_132      0x00000084
#define APERTA_TBHMOD_PLL_MODE_DIV_160      0x000000A0
#define APERTA_TBHMOD_PLL_MODE_DIV_165      0x000000A5
#define APERTA_TBHMOD_PLL_MODE_DIV_170      0x000000AA

/* ==========================================================================
 * Speed-ID table geometry
 * ========================================================================== */
#define APERTA_TSCBH_SPEED_ID_TABLE_SIZE            76
#define APERTA_TSCBH_SPEED_ID_ENTRY_SIZE            5
#define APERTA_TSCBH_HW_SPEED_ID_TABLE_SIZE         64
#define APERTA_TSCBH_AM_TABLE_SIZE                  64
#define APERTA_TSCBH_AM_ENTRY_SIZE                  3
#define APERTA_TSCBH_UM_TABLE_SIZE                  64
#define APERTA_TSCBH_UM_ENTRY_SIZE                  2
#define APERTA_TSCBH_SPEED_PRIORITY_MAPPING_TABLE_SIZE 1
#define APERTA_TSCBH_SPEED_PRIORITY_MAPPING_ENTRY_SIZE 9
#define APERTA_TSCBH_NOF_LANES_IN_CORE              8

/* ==========================================================================
 * aperta_tbhmod_spd_intfc_type_t
 * Order is significant: it is used as the index into sc_pmd_entry tables.
 * ========================================================================== */
typedef enum {
    APERTA_TBHMOD_SPD_ZERO                  = 0,
    APERTA_TBHMOD_SPD_10000_XFI             ,
    APERTA_TBHMOD_SPD_20000_XFI             ,
    APERTA_TBHMOD_SPD_25000_XFI             ,
    APERTA_TBHMOD_SPD_40G_MLD_X2            ,
    APERTA_TBHMOD_SPD_40G_MLD_X4            ,
    APERTA_TBHMOD_SPD_50G_MLD_X2            ,
    APERTA_TBHMOD_SPD_50G_MLD_FEC_528_X2    ,
    APERTA_TBHMOD_SPD_100G_MLD_X4           ,
    APERTA_TBHMOD_SPD_100G_MLD_NO_FEC_X4    ,
    APERTA_TBHMOD_SPD_CL73_20G              ,
    APERTA_TBHMOD_SPD_CL73_25G              ,
    APERTA_TBHMOD_SPD_CL73_26G              ,
    APERTA_TBHMOD_SPD_200G_BRCM_NO_FEC_KR4_CR4          ,
    APERTA_TBHMOD_SPD_200G_IEEE_FEC_544_2XN_KR4_CR4     ,
    APERTA_TBHMOD_SPD_200G_BRCM_FEC_544_1XN_KR4_CR4     ,
    APERTA_TBHMOD_SPD_50G_BRCM_NOFEC_KR1_CR1            ,
    APERTA_TBHMOD_SPD_100G_BRCM_NOFEC_KR2_CR2           ,
    APERTA_TBHMOD_SPD_50G_BRCM_FEC_544_CR2_KR2          ,
    APERTA_TBHMOD_SPD_50G_IEEE_KR1_CR1                  ,
    APERTA_TBHMOD_SPD_50G_BRCM_FEC_528_CR1_KR1          ,
    APERTA_TBHMOD_SPD_100G_IEEE_KR2_CR2                 ,
    APERTA_TBHMOD_SPD_100G_BRCM_FEC_544_1XN_KR4_CR4     ,
    APERTA_TBHMOD_SPD_100G_BRCM_FEC_528_KR2_CR2         ,
    APERTA_TBHMOD_SPD_400G_IEEE_FEC_544_2XN_X8          ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_56                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_57                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_58                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_59                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_60                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_61                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_62                   ,
    APERTA_TBHMOD_SPD_CUSTOM_ENTRY_63                   ,
    APERTA_TBHMOD_SPD_10G_FEC_BASE_R_KR1_CR1            ,
    APERTA_TBHMOD_SPD_20G_FEC_BASE_R_KR1_CR1            ,
    APERTA_TBHMOD_SPD_25G_FEC_BASE_R_KR1_CR1            ,
    APERTA_TBHMOD_SPD_25G_FEC_RS_FEC_KR1_CR1            ,
    APERTA_TBHMOD_SPD_40G_FEC_BASE_R_KR4_CR4            ,
    APERTA_TBHMOD_SPD_50G_BRCM_FEC_272_KR1_CR1          ,
    APERTA_TBHMOD_SPD_100G_BRCM_FEC_272_KR2_CR2         ,
    APERTA_TBHMOD_SPD_200G_BRCM_FEC_272_2XN_KR4_CR4     ,
    APERTA_TBHMOD_SPD_200G_BRCM_FEC_272_1XN_KR4_CR4     ,
    APERTA_TBHMOD_SPD_400G_BRCM_FEC_544_2XN_X8          ,
    APERTA_TBHMOD_SPD_400G_BRCM_FEC_272_2XN_X8          ,
    APERTA_TBHMOD_SPD_150G_FEC_544_2XN_N3               ,
    APERTA_TBHMOD_SPD_300G_FEC_544_2XN_N6               ,
    APERTA_TBHMOD_SPD_350G_FEC_544_2XN_N7               ,
    APERTA_TBHMOD_SPD_12P5G_BRCM_KR1                    ,
    APERTA_TBHMOD_SPD_50G_BRCM_CR2_KR2_RS_FEC_FLEXE     ,
    APERTA_TBHMOD_SPD_50G_BRCM_FEC_544_CR2_KR2_FLEXE    ,
    APERTA_TBHMOD_SPD_ILLEGAL
} aperta_tbhmod_spd_intfc_type_t;

/* ==========================================================================
 * sc_pmd_entry_st
 * One entry per mapped speed id: OSR mode + PLL divider mode.
 * ========================================================================== */
typedef struct sc_pmd_entry_s {
    int t_pma_os_mode;
    int pll_mode;
} sc_pmd_entry_t;
#define sc_pmd_entry_st sc_pmd_entry_t

/* ==========================================================================
 * Lookup tables
 * ========================================================================== */
extern const sc_pmd_entry_t plp_aperta_tscbh_sc_pmd_entry[];
extern const sc_pmd_entry_t plp_aperta_tscbh_sc_pmd_entry_312M_ref[];

extern uint32_t plp_aperta_spd_id_entry_26[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_26_gsh[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_25[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_25_gsh[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_20[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_20_gsh[APERTA_TSCBH_SPEED_ID_TABLE_SIZE][APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_100g_4lane_no_fec_26[APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_spd_id_entry_100g_4lane_no_fec_25[APERTA_TSCBH_SPEED_ID_ENTRY_SIZE];
extern uint32_t plp_aperta_am_table_entry[APERTA_TSCBH_AM_TABLE_SIZE][APERTA_TSCBH_AM_ENTRY_SIZE];
extern uint32_t plp_aperta_um_table_entry[APERTA_TSCBH_UM_TABLE_SIZE][APERTA_TSCBH_UM_ENTRY_SIZE];
extern uint32_t plp_aperta_speed_priority_mapping_table[APERTA_TSCBH_SPEED_PRIORITY_MAPPING_TABLE_SIZE][APERTA_TSCBH_SPEED_PRIORITY_MAPPING_ENTRY_SIZE];

/* ==========================================================================
 * Lookup helpers
 * ========================================================================== */
extern int plp_aperta_tbhmod_get_mapped_speed(aperta_tbhmod_spd_intfc_type_t spd_intf, int *speed);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_26_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_25_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_20_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_26_gsh_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_25_gsh_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_20_gsh_get(void);
extern uint32_t* plp_aperta_tscbh_am_table_entry_get(void);
extern uint32_t* plp_aperta_tscbh_um_table_entry_get(void);
extern uint32_t* plp_aperta_tscbh_speed_priority_mapping_table_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_100g_4lane_no_fec_25_get(void);
extern uint32_t* plp_aperta_tscbh_spd_id_entry_100g_4lane_no_fec_26_get(void);

#endif /* _APERTA_TBHMOD_DEFS_H */
