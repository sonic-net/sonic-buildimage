#ifndef __WB_I2C_DEV_H__
#define __WB_I2C_DEV_H__
#include <wb_logic_dev_common.h>

typedef struct i2c_dev_device_s {
    struct i2c_client *client;
    struct i2c_adapter *adap;
    bool is_little_endian;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    char i2c_name[MAX_NAME_SIZE];
    char i2c_alias[MAX_NAME_SIZE];
    uint32_t data_bus_width;
    uint32_t addr_bus_width;
    uint32_t per_rd_len;
    uint32_t per_wr_len;
    uint32_t i2c_len;
    uint32_t cs_index;
    uint32_t log_num;                           /* The number of write record registers is 64 at most */
    uint32_t log_index[BSP_KEY_DEVICE_NUM_MAX]; /* Write record register address = (base_addr & 0xFFFF) | (len << 16) */
    device_status_check_t status_check;
} i2c_dev_device_t;

int i2c_device_func_read(const char *path, uint32_t offset, uint8_t *buf, size_t count);
int i2c_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);

#endif
