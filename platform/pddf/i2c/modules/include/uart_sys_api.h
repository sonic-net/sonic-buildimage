#ifndef __UART_SYS_API_H__
#define __UART_SYS_API_H__

extern ssize_t sys_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t sys_store_default(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);

extern int sys_set_enable(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);
extern int sys_get_bmc_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);
extern int sys_get_cpld_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);
extern int sys_get_bom_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);
extern int sys_get_pcb_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);
extern int sys_heartbeat_default(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);
extern int sys_set_rtc_time(struct i2c_client *client, SYS_DATA_ATTR *info, void *data);

#endif