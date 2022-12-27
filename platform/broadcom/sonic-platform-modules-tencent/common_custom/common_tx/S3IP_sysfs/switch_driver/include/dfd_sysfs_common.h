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
    /* current sensors */
    int (*get_main_board_curr_number)(void);
    ssize_t (*get_main_board_curr_alias)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_type)(unsigned int curr_index, char *buf, size_t count);
    ssize_t (*get_main_board_curr_max)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_max)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_min)(unsigned int curr_index, char *buf, size_t count);
    int (*set_main_board_curr_min)(unsigned int curr_index, const char *buf, size_t count);
    ssize_t (*get_main_board_curr_value)(unsigned int curr_index, char *buf, size_t count);
    /* syseeprom */
    int (*get_syseeprom_size)(void);
    ssize_t (*read_syseeprom_data)(char *buf, loff_t offset, size_t count);
    ssize_t (*write_syseeprom_data)(char *buf, loff_t offset, size_t count);
    /* fan */
    int (*get_fan_number)(void);
    int (*get_fan_motor_number)(unsigned int fan_index);
    ssize_t (*get_fan_model_name)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_serial_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_part_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_hardware_version)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_status)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_led_status)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_led_status)(unsigned int fan_index, int status);
    ssize_t (*get_fan_direction)(unsigned int fan_index, char *buf, size_t count);
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
    ssize_t (*get_psu_fan_speed)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_alias)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_type)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_value)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    /* transceiver */
    int (*get_eth_number)(void);
    ssize_t (*get_transceiver_power_on_status)(char *buf, size_t count);
    int (*set_transceiver_power_on_status)(int status);
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
    ssize_t (*get_slot_serial_number)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_part_number)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_hardware_version)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_status)(unsigned int slot_index, char *buf, size_t count);
    ssize_t (*get_slot_led_status)(unsigned int slot_index, char *buf, size_t count);
    int (*set_slot_led_status)(unsigned int slot_index, int status);
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
};

extern struct switch_drivers_s * switch_driver_get(void);

#endif /*_DFD_SYSFS_COMMON_H_ */
