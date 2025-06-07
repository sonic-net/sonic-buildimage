#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_i2c_dev.h>

static int g_wb_i2c_dev_device_debug = 0;
static int g_wb_i2c_dev_device_error = 0;

module_param(g_wb_i2c_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_i2c_dev_device_error, int, S_IRUGO | S_IWUSR);

#define WB_I2C_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_i2c_dev_device_debug) { \
        printk(KERN_INFO "[WB_I2C_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_I2C_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_i2c_dev_device_error) { \
        printk(KERN_ERR "[WB_I2C_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static i2c_dev_device_t i2c_dev_device_data0 = {
    .i2c_bus = 2,
    .i2c_addr = 0x0d,
    .i2c_name = "cpld2",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

static i2c_dev_device_t i2c_dev_device_data1 = {
    .i2c_bus = 8,
    .i2c_addr = 0x30,
    .i2c_name = "cpld3",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

static i2c_dev_device_t i2c_dev_device_data2 = {
    .i2c_bus = 8,
    .i2c_addr = 0x31,
    .i2c_name = "cpld4",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

static i2c_dev_device_t i2c_dev_device_data3 = {
    .i2c_bus = 6,
    .i2c_addr = 0x0d,
    .i2c_name = "cpld5",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

struct i2c_board_info i2c_dev_device_info[] = {
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data0,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data1,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data2,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data3,
    },
};

static int __init wb_i2c_dev_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    i2c_dev_device_t *i2c_dev_device_data;

    WB_I2C_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(i2c_dev_device_info); i++) {
        i2c_dev_device_data = i2c_dev_device_info[i].platform_data;
        i2c_dev_device_info[i].addr = i2c_dev_device_data->i2c_addr;
        adap = i2c_get_adapter(i2c_dev_device_data->i2c_bus);
        if (adap == NULL) {
            i2c_dev_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", i2c_dev_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &i2c_dev_device_info[i]);
        if (!client) {
            i2c_dev_device_data->client = NULL;
            printk(KERN_ERR "Failed to register i2c dev device %d at bus %d!\n",
                i2c_dev_device_data->i2c_addr, i2c_dev_device_data->i2c_bus);
        } else {
            i2c_dev_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_i2c_dev_device_exit(void)
{
    int i;
    i2c_dev_device_t *i2c_dev_device_data;

    WB_I2C_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(i2c_dev_device_info) - 1; i >= 0; i--) {
        i2c_dev_device_data = i2c_dev_device_info[i].platform_data;
        if (i2c_dev_device_data->client) {
            i2c_unregister_device(i2c_dev_device_data->client);
            i2c_dev_device_data->client = NULL;
        }
    }
}

module_init(wb_i2c_dev_device_init);
module_exit(wb_i2c_dev_device_exit);
MODULE_DESCRIPTION("I2C DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
