#ifndef __UART_FAN_API_H__
#define __UART_FAN_API_H__

extern ssize_t fan_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t fan_store_default(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);

extern int bmc_get_fan_board_type(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_status(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_presence(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_front_speed_rpm(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_rear_speed_rpm(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_direction(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_eeprom_model(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_eeprom_serial_number(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_eeprom_manufacturer(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_eeprom_module_model(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_eeprom_module_brand(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);
extern int bmc_get_fan_eeprom_direction(struct i2c_client *client, FAN_DATA_ATTR *info, void *data);

#endif
