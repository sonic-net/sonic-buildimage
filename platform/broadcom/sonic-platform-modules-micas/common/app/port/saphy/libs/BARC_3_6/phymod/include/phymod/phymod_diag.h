/*
 *         
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

/*
 *         
 * $Id: phymod_definitions.h,v 1.2.2.12 Broadcom SDK $
 * 
 * $Copyright: 
 * All Rights Reserved.$
 *         
 * Shell diagnostics of Phymod    
 *
 */

#ifndef _plp_barchetta_PHYMOD_DIAG_H_
#define _plp_barchetta_PHYMOD_DIAG_H_

#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_symbols.h>


/******************************************************************************
 Typedefs
******************************************************************************/
typedef int (*print_func_f)(const char *, ...);

typedef enum{
    PhymodDiagPrbsClear,
    PhymodDiagPrbsSet,
    PhymodDiagPrbsGet
}phymod_diag_prbs_operation_t;

typedef struct plp_barchetta_phymod_diag_prbs_set_args_s{
    uint32_t flags;
    plp_barchetta_phymod_prbs_t prbs_options;
    uint32_t enable;
    uint32_t loopback;
}plp_barchetta_phymod_diag_prbs_set_args_t;

typedef struct plp_barchetta_phymod_diag_prbs_get_args_s{
    uint32_t time;
}plp_barchetta_phymod_diag_prbs_get_args_t;

typedef struct plp_barchetta_phymod_diag_prbs_clear_args_s{
    uint32_t flags;
}plp_barchetta_phymod_diag_prbs_clear_args_t;


typedef union plp_barchetta_phymod_diag_prbs_command_args_u{
    plp_barchetta_phymod_diag_prbs_set_args_t set_params;
    plp_barchetta_phymod_diag_prbs_get_args_t get_params;
    plp_barchetta_phymod_diag_prbs_clear_args_t clear_params;
}plp_barchetta_phymod_prbs_command_args_t;

typedef struct plp_barchetta_phymod_diag_prbs_args_s{
    phymod_diag_prbs_operation_t prbs_cmd;
    plp_barchetta_phymod_prbs_command_args_t args;
}plp_barchetta_phymod_diag_prbs_args_t;


/******************************************************************************
Functions
******************************************************************************/

extern print_func_f plp_barchetta_phymod_diag_print_func;

int plp_barchetta_phymod_diag_symbols_table_get(plp_barchetta_phymod_phy_access_t *phy, plp_barchetta_phymod_symbols_t **symbols);
int phymod_diag_firmware_mode_set(plp_barchetta_phymod_core_access_t *cores, int array_size, phymod_firmware_lane_config_t fw_config);
int plp_barchetta_phymod_diag_firmware_load(plp_barchetta_phymod_core_access_t *cores, int array_size, char *firwmware_file);
int plp_barchetta_phymod_diag_prbs(plp_barchetta_phymod_phy_access_t *phys, int array_size, plp_barchetta_phymod_diag_prbs_args_t *prbs_diag_args);
int plp_barchetta_phymod_diag_reg_write(plp_barchetta_phymod_phy_access_t *phys, int array_size, uint32_t reg_addr, uint32_t val);
int plp_barchetta_phymod_diag_reg_read(plp_barchetta_phymod_phy_access_t *phys, int array_size, uint32_t reg_addr);
int plp_barchetta_phymod_diag_dsc(plp_barchetta_phymod_phy_access_t *phys, int array_size);
int plp_barchetta_phymod_diag_dsc_std(plp_barchetta_phymod_phy_access_t *phys, int array_size);
int plp_barchetta_phymod_diag_dsc_config(plp_barchetta_phymod_phy_access_t *phys, int array_size);


int plp_barchetta_phymod_diag_eyescan_run( 
    plp_barchetta_phymod_phy_access_t *phys,
    int unit,
    int* port_ids,
    int* line_rates, 
    int num_phys,
    plp_barchetta_phymod_eyescan_mode_t mode,
    plp_barchetta_phymod_phy_eyescan_options_t* eyescan_options);

#endif /*_plp_barchetta_PHYMOD_DIAG_H_*/
