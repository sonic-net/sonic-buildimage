#ifndef _WB_SENSORS_DRIVER_H_
#define _WB_SENSORS_DRIVER_H_

#define DFD_GET_TEMP_SENSOR_KEY1(dev_index, temp_index) \
    (((dev_index & 0xff) << 8) | (temp_index & 0xff))
#define DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, temp_type) \
        (((main_dev_id & 0x0f) << 4) | (temp_type & 0x0f))

extern dfd_debug_data_key_map_t vol_dbg_key_table[WB_SENSOR_END];
extern dfd_debug_data_key_map_t curr_dbg_key_table[WB_SENSOR_END];
extern dfd_debug_data_key_map_t temp_dbg_key_table[WB_SENSOR_END];
/**
 * dfd_get_temp_info - Get temperature information
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1/psu1
 * @temp_index: Temperature index, starting at 1
 * @temp_type: Read type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_temp_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t temp_index,
            uint8_t temp_attr, char *buf, size_t count);

/**
 * dfd_get_voltage_info - Get voltage information
 * @main_dev_id: Motherboard :0 Subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @in_index: Voltage index, starting at 1
 * @in_type:Voltage type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_voltage_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t in_index,
            uint8_t in_attr, char *buf, size_t count);

/**
 * dfd_get_current_info - Get current information
 * @main_dev_id: Motherboard :0 Subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @in_index: Current index, starting at 1
 * @in_type: Current type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_current_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t curr_index,
            uint8_t curr_attr, char *buf, size_t count);

/**
 * dfd_get_psu_sensor_info - Obtain PMBUS information about the power supply
 * @psu_index: Power index, starting at 1
 * @sensor_type: Type of the obtained pmbus information
 * @buf: pmbus results are stored in buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_psu_sensor_info(uint8_t psu_index, uint8_t sensor_type, char *buf, size_t count);

/**
 * dfd_get_power_info - Get power information
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @in_index: Power index, starting at 1
 * @in_type: Power type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
ssize_t dfd_get_power_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t in_index,
            uint8_t in_attr, char *buf, size_t count);

/**
 * dfd_get_main_board_monitor_flag - Get Monitor flag info
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_type: Type of the obtained pmbus information
 * @in_type: Voltage type,1:alias 2:type 3:max 4:max_hyst 5:min 6:input
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
int dfd_get_main_board_monitor_flag(uint8_t main_dev_id, uint8_t dev_index, uint8_t sensor_type,
        uint8_t sensor_index, char *buf, size_t count);

/**
 * dfd_get_temp_status - Get Monitor flag info
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_index: index of temp
 * @buf: temp status is stored in buf
 * @count: sizeof buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
int dfd_get_temp_status(uint8_t main_dev_id, uint8_t dev_index,
        unsigned int sensor_index, char *buf, size_t count);

/**
 * dfd_get_temp_status - Get Monitor flag info
 * @main_dev_id: Motherboard :0 Power supply :2 subcard :5
 * @dev_index: If no device index exists, the value is 0, and 1 indicates slot1
 * @sensor_index: index of temp
 * @buf: temp status is stored in buf
 * @count: sizeof buf
 * return: Success: Returns the length of buf
 *       : Failed: A negative value is returned
 */
int dfd_get_vol_status(uint8_t main_dev_id, uint8_t dev_index,
        unsigned int sensor_index, char *buf, size_t count);

#endif /* _WB_SENSORS_DRIVER_H_ */
