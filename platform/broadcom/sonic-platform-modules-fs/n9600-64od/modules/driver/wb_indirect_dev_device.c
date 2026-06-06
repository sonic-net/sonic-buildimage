#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_indirect_dev.h>

static int g_indirect_dev_device_debug = 0;
static int g_indirect_dev_device_error = 0;

module_param(g_indirect_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_indirect_dev_device_error, int, S_IRUGO | S_IWUSR);

#define INDIRECT_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_indirect_dev_device_debug) { \
        printk(KERN_INFO "[INDIRECT_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define INDIRECT_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_indirect_dev_device_error) { \
        printk(KERN_ERR "[INDIRECT_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

/* CPLD-I2C-MASTER-0 */
static indirect_dev_device_t indirect_dev_device_data0 = {
    .logic_func_mode = 4,
    .dev_name = "cpld2",
    .logic_dev_name = "/dev/cpld1",
    .data_bus_width    = 4,
    .wr_data        = 0x70,
    .addr_low       = 0x74,
    .addr_high      = 0x75,
    .rd_data        = 0x76,
    .opt_ctl        = 0x7a,
    .indirect_len   = 0x200,
};

/* CPLD-I2C-MASTER-1 */
static indirect_dev_device_t indirect_dev_device_data1 = {
    .logic_func_mode = 4,
    .dev_name = "cpld3",
    .logic_dev_name = "/dev/cpld1",
    .data_bus_width    = 4,
    .wr_data        = 0x80,
    .addr_low       = 0x84,
    .addr_high      = 0x85,
    .rd_data        = 0x86,
    .opt_ctl        = 0x8a,
    .indirect_len   = 0x200,
};

/* CPLD-I2C-MASTER-2 */
static indirect_dev_device_t indirect_dev_device_data2 = {
    .logic_func_mode = 4,
    .dev_name = "cpld4",
    .logic_dev_name = "/dev/cpld1",
    .data_bus_width    = 4,
    .wr_data        = 0x90,
    .addr_low       = 0x94,
    .addr_high      = 0x95,
    .rd_data        = 0x96,
    .opt_ctl        = 0x9a,
    .indirect_len   = 0x200,
};

/* CPLD-I2C-MASTER-3 */
static indirect_dev_device_t indirect_dev_device_data3 = {
    .logic_func_mode = 4,
    .dev_name = "cpld5",
    .logic_dev_name = "/dev/cpld1",
    .data_bus_width    = 4,
    .wr_data        = 0xa0,
    .addr_low       = 0xa4,
    .addr_high      = 0xa5,
    .rd_data        = 0xa6,
    .opt_ctl        = 0xaa,
    .indirect_len   = 0x200,
};

static void indirect_dev_device_bus_device_release(struct device *dev)
{
    return;
}

static struct platform_device indirect_dev_device[] = {
    {
        .name   = "wb-indirect-dev",
        .id = 1,
        .dev    = {
            .platform_data  = &indirect_dev_device_data0,
            .release = indirect_dev_device_bus_device_release,
        },
    },
    {
        .name   = "wb-indirect-dev",
        .id = 2,
        .dev    = {
            .platform_data  = &indirect_dev_device_data1,
            .release = indirect_dev_device_bus_device_release,
        },
    },
    {
        .name   = "wb-indirect-dev",
        .id = 3,
        .dev    = {
            .platform_data  = &indirect_dev_device_data2,
            .release = indirect_dev_device_bus_device_release,
        },
    },
    {
        .name   = "wb-indirect-dev",
        .id = 4,
        .dev    = {
            .platform_data  = &indirect_dev_device_data3,
            .release = indirect_dev_device_bus_device_release,
        },
    },
};

static int __init indirect_dev_device_bus_device_init(void)
{
    int i;
    int ret = 0;
    indirect_dev_device_t *indirect_dev_device_data;

    INDIRECT_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(indirect_dev_device); i++) {
        indirect_dev_device_data = indirect_dev_device[i].dev.platform_data;
        ret = platform_device_register(&indirect_dev_device[i]);
        if (ret < 0) {
            indirect_dev_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-indirect-dev.%d register failed!\n", i + 1);
        } else {
            indirect_dev_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit indirect_dev_device_bus_device_exit(void)
{
    int i;
    indirect_dev_device_t *indirect_dev_device_data;

    INDIRECT_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(indirect_dev_device) - 1; i >= 0; i--) {
        indirect_dev_device_data = indirect_dev_device[i].dev.platform_data;
        if (indirect_dev_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&indirect_dev_device[i]);
        }
    }
}

module_init(indirect_dev_device_bus_device_init);
module_exit(indirect_dev_device_bus_device_exit);
MODULE_DESCRIPTION("INDIRECT DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
