#ifndef __WB_I2C_DEV_H__
#define __WB_I2C_DEV_H__
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define I2C_DEV_NAME_MAX_LEN (64)

typedef struct i2c_dev_device_s {
    struct i2c_client *client;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    char i2c_name[I2C_DEV_NAME_MAX_LEN];
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    uint32_t i2c_len;
} i2c_dev_device_t;

#endif
