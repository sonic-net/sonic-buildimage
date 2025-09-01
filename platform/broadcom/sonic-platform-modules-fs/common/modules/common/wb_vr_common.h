#ifndef __WB_VR_COMMON_H__
#define __WB_VR_COMMON_H__

#include <linux/i2c.h>

#define DEVICE_ID_LEN       (256)

typedef struct wb_vr_common_s {
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    struct i2c_client *client;
    struct i2c_adapter *adap;
} wb_vr_common_t;

typedef struct wb_vr_common_device_s {
    uint32_t    i2c_bus;
    uint32_t    i2c_addr;
    int         device_flag;
} wb_vr_common_device_t;

typedef struct {
    char        device_name[I2C_NAME_SIZE];
    uint8_t    device_id[DEVICE_ID_LEN];
    uint32_t    dev_id_len;
} vr_dev_info_t;

#endif
