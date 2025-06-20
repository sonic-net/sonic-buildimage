#ifndef __UART_PSU_API_H__
#define __UART_PSU_API_H__

extern ssize_t psu_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t psu_store_default(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);

extern int bmc_get_psu_type(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_status(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_presence(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_v_in(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_v_out(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_i_in(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_i_out(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_p_in(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_p_out(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_direction(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_warning(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_direction_warning(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_model(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_serial_number(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_temp(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);
extern int bmc_get_psu_fan_speed(struct i2c_client *client, PSU_DATA_ATTR *info, void *data);


#endif
