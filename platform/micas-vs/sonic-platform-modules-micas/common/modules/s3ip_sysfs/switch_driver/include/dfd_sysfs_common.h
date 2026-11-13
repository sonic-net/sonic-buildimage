/*
 * A header definition for dfd_sysfs_common driver
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

#ifndef _DFD_SYSFS_COMMON_H_
#define _DFD_SYSFS_COMMON_H_

struct switch_drivers_s {
    /* temperature sensors */
    int (*get_main_board_temp_number)(void);
    ssize_t (*get_main_board_temp_alias)(unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_type)(unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_max)(unsigned int temp_index, char *buf, size_t count);
    int (*set_main_board_temp_max)(unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_main_board_temp_min)(unsigned int temp_index, char *buf, size_t count);
    int (*set_main_board_temp_min)(unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_main_board_temp_value)(unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_high)(unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_low)(unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_main_board_temp_monitor_flag)(unsigned int temp_index, char *buf, size_t count);
    /* voltage sensors */
    int (*get_main_board_vol_number)(void);
    ssize_t (*get_main_board_vol_alias)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_type)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_max)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_max)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_min)(unsigned int vol_index, char *buf, size_t count);
    int (*set_main_board_vol_min)(unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_main_board_vol_range)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_nominal_value)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_value)(unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_main_board_vol_monitor_flag)(unsigned int vol_index, char *buf, size_t count);
    /* current sensors */
    int (*get_main_board_curr_number)(void);
    ssize_t (*get_main_board_curr_alias)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_type)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_max)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_max)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_min)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_min)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_value)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_monitor_flag)(unsigned int curr_index, char *buf, size_t count);
    /* syseeprom */
    int (*get_syseeprom_size)(void);
    ssize_t (*read_syseeprom_data)(char *buf, loff_t offset, size_t count);
    ssize_t (*write_syseeprom_data)(char *buf, loff_t offset, size_t count);
    /* fan */
    int (*get_fan_number)(void);
    int (*get_fan_motor_number)(unsigned int fan_index);
    ssize_t (*get_fan_model_name)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_vendor)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_serial_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_part_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_hardware_version)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_status)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_present)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_led_status)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_led_status)(unsigned int fan_index, int status);
    ssize_t (*get_fan_direction)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_status)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_tolerance)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_target)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_max)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_min)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_ratio)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_ratio)(unsigned int fan_index, int ratio);
    /* PSU */
    int (*get_psu_number)(void);
    int (*get_psu_temp_number)(unsigned int psu_index);
    ssize_t (*get_psu_model_name)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_vendor)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_date)(unsigned int psu_index, char *buf, size_t count);
	ssize_t (*get_psu_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hw_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_serial_number)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_part_number)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hardware_version)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_type)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_curr)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_vol)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_curr)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_vol)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_max_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_present_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_status_pmbus)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed_cal)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_alias)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_type)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_value)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_attr_threshold)(unsigned int psu_index, unsigned int type,  char *buf, size_t count);
    int (*get_psu_eeprom_size)(unsigned int psu_index);
    ssize_t (*read_psu_eeprom_data)(unsigned int psu_index, char *buf, loff_t offset, size_t count);
    ssize_t (*get_psu_blackbox_info)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_pmbus_info)(unsigned int psu_index, char *buf, size_t count);
    int (*clear_psu_blackbox)(unsigned int psu_index, uint8_t value);
    /* transceiver */
    int (*get_eth_number)(void);
    ssize_t (*get_transceiver_power_on_status)(char *buf, size_t count);
    int (*set_transceiver_power_on_status)(int status);
    ssize_t (*get_transceiver_present_status)(char *buf, size_t count);
    ssize_t (*get_eth_power_on_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_power_on_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_tx_fault_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_tx_disable_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_tx_disable_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_present_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_rx_los_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_reset_status)(unsigned int eth_index, char *buf, size_t count);
    int (*set_eth_reset_status)(unsigned int eth_index, int status);
    ssize_t (*get_eth_low_power_mode_status)(unsigned int eth_index, char *buf, size_t count);
    ssize_t (*get_eth_interrupt_status)(unsigned int eth_index, char *buf, size_t count);
    int (*get_eth_eeprom_size)(unsigned int eth_index);
    ssize_t (*read_eth_eeprom_data)(unsigned int eth_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_eth_eeprom_data)(unsigned int eth_index, char *buf, loff_t offset, size_t count);
    ssize_t (*get_eth_optoe_type)(unsigned int sff_index, int *optoe_type, char *buf, size_t count);
    int (*set_eth_optoe_type)(unsigned int sff_index, int optoe_type);
    /* sysled */
    ssize_t (*get_sys_led_status)(char *buf, size_t count);
    int (*set_sys_led_status)(int status);
    ssize_t (*get_bmc_led_status)(char *buf, size_t count);
    int (*set_bmc_led_status)(int status);
    ssize_t (*get_sys_fan_led_status)(char *buf, size_t count);
    int (*set_sys_fan_led_status)(int status);
    ssize_t (*get_sys_psu_led_status)(char *buf, size_t count);
    int (*set_sys_psu_led_status)(int status);
    ssize_t (*get_id_led_status)(char *buf, size_t count);
    int (*set_id_led_status)(int status);
    /* FPGA */
    int (*get_main_board_fpga_number)(void);
    ssize_t (*get_main_board_fpga_alias)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_type)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_firmware_version)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_board_version)(unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_main_board_fpga_test_reg)(unsigned int fpga_index, char *buf, size_t count);
    int (*set_main_board_fpga_test_reg)(unsigned int fpga_index, unsigned int value);
    /* CPLD */
    int (*get_main_board_cpld_number)(void);
    ssize_t (*get_main_board_cpld_alias)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_type)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_firmware_version)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_board_version)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_main_board_cpld_test_reg)(unsigned int cpld_index, char *buf, size_t count);
    int (*set_main_board_cpld_test_reg)(unsigned int cpld_index, unsigned int value);
    /* watchdog */
    ssize_t (*get_watchdog_identify)(char *buf, size_t count);
    ssize_t (*get_watchdog_timeleft)(char *buf, size_t count);
    ssize_t (*get_watchdog_timeout)(char *buf, size_t count);
    int (*set_watchdog_timeout)(int value);
    ssize_t (*get_watchdog_enable_status)(char *buf, size_t count);
    int (*set_watchdog_enable_status)(int value);
    int (*set_watchdog_reset)(int value);
    /* slot */
    int (*get_slot_number)(void);
    int (*get_slot_temp_number)(unsigned int slot_index);
    int (*get_slot_vol_number)(unsigned int slot_index);
    int (*get_slot_curr_number)(unsigned int slot_index);
    int (*get_slot_cpld_number)(unsigned int slot_index);
    int (*get_slot_fpga_number)(unsigned int slot_index);
    ssize_t (*get_slot_model_name)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_vendor)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_serial_number)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_part_number)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_hardware_version)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_status)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_led_status)(unsigned int slot_index, char *buf, size_t count);
    int (*set_slot_led_status)(unsigned int slot_index, int status);
    ssize_t (*get_slot_power_status)(unsigned int slot_index, char *buf, size_t count);
    int (*set_slot_power_status)(unsigned int slot_index, int status);
    ssize_t (*get_slot_temp_alias)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_slot_temp_type)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_slot_temp_max)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_slot_temp_max)(unsigned int slot_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_slot_temp_min)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_slot_temp_min)(unsigned int slot_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_slot_temp_value)(unsigned int slot_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_alias)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_type)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_max)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    int (*set_slot_vol_max)(unsigned int slot_index, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_slot_vol_min)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    int (*set_slot_vol_min)(unsigned int slot_index, unsigned int vol_index, const char *buf, size_t count);
    ssize_t (*get_slot_vol_range)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_nominal_value)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_vol_value)(unsigned int slot_index, unsigned int vol_index, char *buf, size_t count);
    ssize_t (*get_slot_curr_alias)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_slot_curr_type)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_slot_curr_max)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    int (*set_slot_curr_max)(unsigned int slot_index, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_slot_curr_min)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    int (*set_slot_curr_min)(unsigned int slot_index, unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_slot_curr_value)(unsigned int slot_index, unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_alias)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_type)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_firmware_version)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_board_version)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    ssize_t (*get_slot_fpga_test_reg)(unsigned int slot_index, unsigned int fpga_index, char *buf, size_t count);
    int (*set_slot_fpga_test_reg)(unsigned int slot_index, unsigned int fpga_index, unsigned int value);
    ssize_t (*get_slot_cpld_alias)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_type)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_firmware_version)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_board_version)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_slot_cpld_test_reg)(unsigned int slot_index, unsigned int cpld_index, char *buf, size_t count);
    int (*set_slot_cpld_test_reg)(unsigned int slot_index, unsigned int cpld_index, unsigned int value);
    /* system */
    ssize_t (*get_system_value)(unsigned int type, int *value, char *buf, size_t count);
    ssize_t (*set_system_value)(unsigned int type, int value);
    ssize_t (*get_system_port_power_status)(unsigned int type, char *buf, size_t count);
    /* eeprom */
    int (*get_eeprom_number)(void);
    int (*get_eeprom_size)(unsigned int e2_index);
    ssize_t (*get_eeprom_alias)(unsigned int e2_index, char *buf, size_t count);
    ssize_t (*get_eeprom_tag)(unsigned int e2_index, char *buf, size_t count);
    ssize_t (*get_eeprom_type)(unsigned int e2_index, char *buf, size_t count);
    ssize_t (*read_eeprom_data)(unsigned int e2_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_eeprom_data)(unsigned int e2_index, char *buf, loff_t offset, size_t count);
};

extern struct switch_drivers_s * s3ip_switch_driver_get(void);

#endif /*_DFD_SYSFS_COMMON_H_ */
