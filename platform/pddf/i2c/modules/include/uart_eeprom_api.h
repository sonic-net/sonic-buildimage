#ifndef __UART_EEPROM_API_H__
#define __UART_EEPROM_API_H__

extern ssize_t eeprom_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t eeprom_store_default(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);

extern int bmc_get_eeprom_default(struct i2c_client *client, EEPROM_DATA_ATTR *info, void *data);

#endif
