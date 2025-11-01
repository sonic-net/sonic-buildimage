/*
* Copyright (c) 2019  <sonic@h3c.com>
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
#ifndef _CPLD_H_
#define _CPLD_H_

#define CPLD_OPER_WARN_MSG0     "\n** !!!! cpld operation is dangerous, which may cause reboot/halt !!!!"
#define CPLD_OPER_WARN_MSG1     "\n** use 'echo offset:value' to set register value"
#define CPLD_OPER_WARN_MSG2     "\n** offset & value must be hex value, example: 'echo 0x0:0x17 > board_cpld'\n"

#define CPU_CPLD_NAME           cpu_cpld
#define BOARD_CPLD_NAME         board_cpld
#define BIOS_CPLD               bios_cpld

#define SLOT_CPLD_NAME          "slot%d_cpld"
#define BUFF_NAME               buffer

#define RESET_REBOOT_CAUSE_PATH "/host/reboot-cause/.reset_reboot_cause"
#define HIS_REBOOT_CAUSE_PATH   "/host/reboot-cause/.his_reboot_cause"

#define MODULE_NAME "cpld"
#define DBG_ECHO(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_CPLD_MODULE], level, BSP_LOG_FILE, fmt, ##args)

enum REBOOT_CAUSE_CODE_ID
{
    NON_HARDWARE = 0,
    POWER_LOSS,
    THERMAL_OVERLOAD_CPU,
    THERMAL_OVERLOAD_ASIC,
    THERMAL_OVERLOAD_OTHER,
    FAN_SPEED,
    WATCHDOG,
    HARDWARE_OTHER,
    CPU_COLD_RESET,
    CPU_WARM_RESET,
    BIOS_RESET,
    PSU_SHUTDOWN,
    BMC_SHUTDOWN,
    RESET_BUTTON_SHUTDOWM,
    RESET_BUTTON_COLD_REBOOT
};

extern struct mutex bsp_mac_inner_temp_lock;
extern struct mutex bsp_mac_width_temp_lock;

extern int cpld_sysfs_init(void);
extern void cpld_release_kobj(void);
extern int bsp_cpld_get_cpld_version(int cpld_index, u8 *cpld_version_hex);
extern int bsp_cpld_get_board_version(int cpld_index, u8 *board_version);
extern int bsp_set_mac_init_ok(u8 bit);
extern int bsp_set_cpld_tx_dis(u8 val);
extern int bsp_set_ssd_power_reset(unsigned int  bit);
extern ssize_t bsp_cpld_custom_read_hw_version(int cpld_index, char *buf);
extern ssize_t bsp_cpld_custom_read_alias(int cpld_index, char *buf);
extern ssize_t bsp_cpld_custom_read_type(int cpld_index, char *buf);
extern size_t bsp_cpld_custom_read_raw_data(int cpld_index, char *buf);
extern ssize_t bsp_cpld_custom_read_cpld_tx_dis(char *buf);

#endif
