#ifndef __UART_PAYLOAD_API_H__
#define __UART_PAYLOAD_API_H__

extern ssize_t payload_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t payload_store_default(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);

extern int bmc_get_payload(struct i2c_client *client, PAYLOAD_DATA_ATTR *info, void *data);

#endif