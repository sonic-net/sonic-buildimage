/*
 *  $Id: aperta_helper.h $
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

#ifndef APERTA_HELPER_H
#define APERTA_HELPER_H
 
#include <string.h>
#include "epdm.h"
#include "epdm_sec.h"

#define if_error(API, NAME)                                   \
    do {                                                      \
        rv = API;                                             \
        if (rv != 0) {                                        \
            printf("Error : %s fails with rv=%d\n", NAME, rv); \
            return rv;                                        \
        }                                                     \
    } while(0);
#define APERTA_HELPER_LINE_SIDE     0
#define APERTA_HELPER_SYSTEM_SIDE   1

#define if_error_no_return(API, NAME)                                   \
    do {                                                      \
        rv = API;                                             \
        if (rv != 0) {                                        \
            printf("Error : %s fails with rv=%d\n", NAME, rv); \
        }                                                     \
    } while(0);

typedef struct aperta_config_spi_rom_s {
    unsigned char spirom_en;      /* 1 : SPIROM enable, 0:SPIROM Disable*/
    unsigned char addr_mode;      /* 0 : 16-bit address mode 1:24-bit address mode*/
    unsigned char read_sr_delay;  /* Delay between read and other command(1ms units)*/
    unsigned char write_delay;    /* Delay between write and other command(1ms units)*/
    unsigned char erase_delay;    /* Delay between erase and other command(1ms units)*/
} aperta_config_spi_rom_t;

typedef struct aperta_link_event_s {
    unsigned int port_data_rate;
    unsigned int pm_timesync_enabled;    /* 1 : represents enabled state of PM TimeSync*/
    unsigned int flags;                  /* 0x1 : Enable Rx  0x4 : Enable One Step processing*/
    
} aperta_link_event_t;

int plp_helper_100G_FEC_link_recovery(char* chip_name, bcm_plp_access_t plp_info);
int plp_helper_update_pll0_port(char* chip_name,  bcm_plp_access_t plp_info,
                bcm_plp_aperta_pll1_vco_t pll1_rate, unsigned int pll0_port_list[8]);
int plp_helper_config_spirom(char* chip_name,  bcm_plp_access_t plp_info,
                              aperta_config_spi_rom_t config_spi_rom);
int plp_helper_erase_spirom(char* chip_name,  bcm_plp_access_t plp_info);
int plp_helper_link_down_event(char* chip_name,  bcm_plp_access_t plp_info, aperta_link_event_t link_event);
int plp_helper_link_up_event(char* chip_name,  bcm_plp_access_t plp_info, aperta_link_event_t link_event);
int plp_helper_disable_incoming_reset(char *chip_name, bcm_plp_access_t plp_info);
int plp_helper_drop_tx_data_on_fault(char *chip_name, bcm_plp_access_t plp_info);
int plp_helper_check_datapath(char* chip_name,  bcm_plp_access_t plp_info);
int plp_helper_sw_flush_seq(char* chip_name,  bcm_plp_access_t sys_plp_info, bcm_plp_access_t line_plp_info);

#endif
