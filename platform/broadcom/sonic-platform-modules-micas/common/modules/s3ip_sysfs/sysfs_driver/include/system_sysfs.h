/*
 * A header definition for system_sysfs driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _SYSTEM_SYSFS_H_
#define _SYSTEM_SYSFS_H_

struct s3ip_sysfs_system_drivers_s {
    ssize_t (*get_system_value)(unsigned int type, int *value, char *buf, size_t count);
    ssize_t (*set_system_value)(unsigned int type, int value);
    ssize_t (*get_system_port_power_status)(unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_system_drivers_register(struct s3ip_sysfs_system_drivers_s *drv);
extern void s3ip_sysfs_system_drivers_unregister(void);

/* system api type */
typedef enum wb_plat_system_type_e {
    WB_SYSTEM_BMC_READY         = 0x0000,  /* bmc ready         */
    WB_SYSTEM_SOL_ACTIVE        = 0x0100,  /* sol active        */
    WB_SYSTEM_PSU_RESET         = 0x0200,  /* psu reset         */
    WB_SYSTEM_CPU_BOARD_CTRL    = 0x0300,  /* cpu board control */
    WB_SYSTEM_CPU_BOARD_STATUS  = 0x0400,  /* cpu board status  */
    WB_SYSTEM_BIOS_UPGRADE      = 0x0500,  /* bios upgrade      */
    WB_SYSTEM_BIOS_SWITCH       = 0x0600,  /* bios switch       */
    WB_SYSTEM_BIOS_VIEW         = 0x0700,  /* bios flash view   */
    WB_SYSTEM_BIOS_BOOT_OK      = 0x0800,  /* bios boot status  */
    WB_SYSTEM_BIOS_FAIL_RECORD  = 0x0900,  /* bios startup failure record */
    WB_SYSTEM_BMC_RESET         = 0x0a00,  /* bmc reset         */
    WB_SYSTEM_MAC_BOARD_RESET   = 0x0b00,  /* mac board reset   */
    WB_SYSTEM_MAC_PWR_CTRL      = 0x0c00,  /* mac power on/off  */
    WB_SYSTEM_EMMC_PWR_CTRL     = 0x0d00,  /* emmc power on/off */
    WB_SYSTEM_PORT_PWR_CTL      = 0x0e00,  /* power power on/off*/
    WB_SYSTEM_BMC_VIEW          = 0x0f00,  /* bmc view          */
    WB_SYSTEM_BMC_SWITCH        = 0x1000,  /* bmc switch        */
} wb_plat_system_type_t;

#endif /*_SYSTEM_SYSFS_H_ */
