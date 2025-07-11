#ifndef __UART_THERMAL_API_H__
#define __UART_THERMAL_API_H__

extern ssize_t thermal_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t thermal_store_default(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);

extern int bmc_get_temp(struct i2c_client *client, THERMAL_DATA_ATTR *info, void *data);
extern int bmc_set_temp(struct i2c_client *client, THERMAL_DATA_ATTR *info, void *data);

#endif
