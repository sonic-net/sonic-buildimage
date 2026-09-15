#ifndef __LIBOBMC_IPMI_H
#define __LIBOBMC_IPMI_H

#include <stdbool.h>

enum obmc_ipmi_reconfig_type {
    OBMC_IPMI_RECONFIG_CP,
    OBMC_IPMI_RECONFIG_DP,
    OBMC_IPMI_RECONFIG_BOTH,
};

/*
 * Flush the PCI MMIO IDP contents into the eeprom on the BMC
 * side. This is the equivalent of "ipmitool raw 6 0x90 1 4 0".
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_main_idp_flush();

/*
 * Fetch the main IDP write status. This is the equivalent of
 * "ipmitool raw 6 0x90 1 5 0".
 *
 * returns:
 *   > 0: main IDP is busy
 *     0: main IDP is idle
 *   < 0: error
 */
int obmc_ipmi_main_idp_status();

/*
 * Reconfigure (0x5a5a) the CPLD only, DP only, or CPLD+DP FPGAs. This is the
 * equivalent of "ipmitool raw 6 0x90 1 6 1" (or 5 or 2)
 *
 * inputs:
 *   all: reconfigure the CPLD only ("cp"), DP only ("dp"), or CPLD+DP FPGAs ("all")
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_fpga_reconfig(enum obmc_ipmi_reconfig_type reconfig_type);

/*
 * Power reset (0xa5a5) the CPLD or CPLD+DP FPGAs. This is the
 * equivalent of "ipmitool raw 6 0x90 1 6 3" (or 4)
 *
 * inputs:
 *   all: power reset the CPLD only (false), or CPLD+DP FPGAs (true)
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_board_reset(bool all);

/*
 * Flush a PCI MMIO fan IDP contents into the eeprom on the BMC side.
 * This is the equivalent of "ipmitool raw 6 0x90 1 20 <fan_num>".
 *
 * inputs:
 *   fan_num: the fan number (zero based)
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_fan_idp_flush(unsigned fan_num);

/*
 * Fetch a fan IDP write status. This is the equivalent of
 * "ipmitool raw 6 0x90 1 21 <fan_num>".
 *
 * inputs:
 *   fan_num: the fan number (zero based)
 *
 * returns:
 *   > 0: fan IDP is busy
 *     0: fan IDP is idle
 *   < 0: error
 */
int obmc_ipmi_fan_idp_status(unsigned fan_num);

/*
 *  To write FRU IDP EEPROM. This is the equivalent of
 * "ipmitool raw 6 0x90 1 22 <fru_num>".
 *
 * inputs:
 *   fru_index: the fru index (one based)
 *
 * returns:
 *   > 0: fan IDP is busy
 *     0: fan IDP is idle
 *   < 0: error
 */
int obmc_ipmi_fru_idp_flush(unsigned fru_num);

/*
 * Fetch a fru IDP write status. This is the equivalent of
 * "ipmitool raw 6 0x90 1 23".
 *
 * inputs:
 *   fru_num: the fan number (zero based)
 *
 * returns:
 *   > 0: fru IDP is busy
 *     0: fru IDP is idle
 *   < 0: error
 */
int obmc_ipmi_fru_idp_status();

/*
 * Send 'sel time set' command to update the BMC RTC. This is the
 * equivalent of "ipmitool sel time set now"
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_sel_time_set_now();

/*
 * To send obmc to do upgrade. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 1 0"
 *
 * inputs:
 *   none
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_upgrade_inform();

/*
 * To send board version and get current CPLD version. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 2 <brd_rev>"
 *
 * inputs:
 *   brd_rev: Board version - 1 (MAX2); 2 (MAX10)
 *
 * returns:
 *     CPLD current version
 *
 */
int obmc_ipmi_brd_send_and_cpld_version_get(unsigned brd_rev);

/*
 * To get the current SPI bank. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 8 0"
 *
 * returns:
     > 0: success KGI
 *     0: success CI
 *   < 0: error
 */
int obmc_ipmi_spi_bank_get();

/*
 * To enable/disable kgi update. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 13 0"
 *
 * inputs:
 *   switch: 0 - Enable; None 0 - Disable
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_kgi_update_enable(int kgi_switch);

/*
 * To get kgi update status. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 14 0"
 *
 * inputs: None
 * returns:
 *   > 0: kgi update in progress
 *     0: success, kig update done
 *   < 0: error
 */
int obmc_ipmi_kgi_update_status_get();

/*
 * To process console switch request from Host. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 30 <console_type>"
 *
 * inputs:
 *   console_type: which console should be showing on the terminal
 *      :   1- CPU
 *      :   2 - BMC
 *      :   3 - SOL
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_host_console_switch(unsigned console_type);

/*
 * To send major revision number. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 31 <major_revision_number>"
 *
 * inputs:
 *   major revision number
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_major_revision_number_send(unsigned char version_number);

/*
 * To send minor revision number. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 32 <minor_revision_number>"
 *
 * inputs:
 *   minor revision number
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_minor_revision_number_send(unsigned char version_number);

/*
 * To send build revision number. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 33 <build_revision_number>"
 *
 * inputs:
 *   build revision number
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_build_revision_number_send(unsigned char version_number);

/*
 * To get upgrade status
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 3 0"
 *
 * inputs: None
 *
 * returns:
     > 0: upgrade in progress
 *     0: success, upgrade done
 *   < 0: error
 */
int obmc_ipmi_upgrade_status_get();

/*
 * To inform BMC to download image from USB interface.
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 0 <image_size>"
 *
 * inputs:
 *   Image size in Mbyte
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_usb_image_download(unsigned int image_size);

/*
 * To process serial BMC enable/disable request from Host. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 34 <console_state>"
 *
 * inputs:
 *   console_state: State of BMC serial console
 *      :   1- Enable
 *      :   2- Disable
 *
 * returns:
 *     0: success
 *   < 0: error
 */
int obmc_ipmi_serial_console(unsigned state);

/*
 * To get BMC serial console status. This is the
 * equivalent of "IPMITOOL -N 5 raw 6 0x90 1 35 0"
 *
 * inputs: None
 * returns:
 *     0: Enable
 *     1: Disable
 *   < 0: error
 */
int obmc_ipmi_serial_console_status_get();

/*
 * Return the current BMC version. This pulls the first two
 * bytes of the 'Aux Firmware Rev Info' returned by command
 * "IPMITOOL -N 5 mc info", or "IPMITOOL -N 5 raw 6 1"
 *
 * inputs: None
 *
 * returns:
 *   >= 0: (major << 8) | minor (success)
 *    < 0: error
 */
int obmc_ipmi_fw_version_get();

#endif
