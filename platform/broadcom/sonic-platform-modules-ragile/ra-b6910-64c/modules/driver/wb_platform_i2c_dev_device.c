#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_platform_i2c_dev.h>

static int g_wb_platform_i2c_dev_device_debug = 0;
static int g_wb_platform_i2c_dev_device_error = 0;

module_param(g_wb_platform_i2c_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_platform_i2c_dev_device_error, int, S_IRUGO | S_IWUSR);

#define WB_PLATFORM_I2C_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_platform_i2c_dev_device_debug) { \
        printk(KERN_INFO "[WB_PLATFORM_I2C_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_PLATFORM_I2C_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_platform_i2c_dev_device_error) { \
        printk(KERN_ERR "[WB_PLATFORM_I2C_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static platform_i2c_dev_device_t platform_i2c_dev_device_data0 = {
    .i2c_bus = 0,
    .i2c_addr = 0x0d,
    .i2c_name = "cpld1",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static platform_i2c_dev_device_t platform_i2c_dev_device_data1 = {
    .i2c_bus = 0,
    .i2c_addr = 0x32,
    .i2c_name = "cpld2",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static platform_i2c_dev_device_t platform_i2c_dev_device_data2 = {
    .i2c_bus = 2,
    .i2c_addr = 0x37,
    .i2c_name = "cpld3",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static platform_i2c_dev_device_t platform_i2c_dev_device_data3 = {
    .i2c_bus = 2,
    .i2c_addr = 0x33,
    .i2c_name = "cpld4",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static platform_i2c_dev_device_t platform_i2c_dev_device_data4 = {
    .i2c_bus = 1,
    .i2c_addr = 0x34,
    .i2c_name = "cpld5",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static platform_i2c_dev_device_t platform_i2c_dev_device_data5 = {
    .i2c_bus = 1,
    .i2c_addr = 0x36,
    .i2c_name = "cpld6",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static platform_i2c_dev_device_t platform_i2c_dev_device_data6 = {
    .i2c_bus = 2,
    .i2c_addr = 0x35,
    .i2c_name = "cpld7",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
};

static void wb_platform_i2c_dev_device_release(struct device *dev)
{
    return;
}

static struct platform_device platform_i2c_dev_device[] = {
    {
        .name   = "wb-platform-i2c-dev",
        .id = 1,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data0,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
    {
        .name   = "wb-platform-i2c-dev",
        .id = 2,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data1,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
    {
        .name   = "wb-platform-i2c-dev",
        .id = 3,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data2,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
    {
        .name   = "wb-platform-i2c-dev",
        .id = 4,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data3,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
    {
        .name   = "wb-platform-i2c-dev",
        .id = 5,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data4,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
    {
        .name   = "wb-platform-i2c-dev",
        .id = 6,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data5,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
    {
        .name   = "wb-platform-i2c-dev",
        .id = 7,
        .dev    = {
            .platform_data  = &platform_i2c_dev_device_data6,
            .release = wb_platform_i2c_dev_device_release,
        },
    },
};

static int __init wb_platform_i2c_dev_device_init(void)
{
    int i;
    int ret = 0;
    platform_i2c_dev_device_t *platform_i2c_dev_device_data;

    WB_PLATFORM_I2C_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(platform_i2c_dev_device); i++) {
        platform_i2c_dev_device_data = platform_i2c_dev_device[i].dev.platform_data;
        ret = platform_device_register(&platform_i2c_dev_device[i]);
        if (ret < 0) {
            platform_i2c_dev_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-platform-i2c-dev.%d register failed!\n", i + 1);
        } else {
            platform_i2c_dev_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_platform_i2c_dev_device_exit(void)
{
    int i;
    platform_i2c_dev_device_t *platform_i2c_dev_device_data;

    WB_PLATFORM_I2C_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(platform_i2c_dev_device) - 1; i >= 0; i--) {
        platform_i2c_dev_device_data = platform_i2c_dev_device[i].dev.platform_data;
        if (platform_i2c_dev_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&platform_i2c_dev_device[i]);
        }
    }
}

module_init(wb_platform_i2c_dev_device_init);
module_exit(wb_platform_i2c_dev_device_exit);
MODULE_DESCRIPTION("PLATFORM I2C DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
