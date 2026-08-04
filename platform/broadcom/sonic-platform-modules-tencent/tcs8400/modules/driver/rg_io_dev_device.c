#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <rg_io_dev.h>

static int g_rg_io_dev_device_debug = 0;
static int g_rg_io_dev_device_error = 0;

module_param(g_rg_io_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_rg_io_dev_device_error, int, S_IRUGO | S_IWUSR);

#define RG_IO_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_rg_io_dev_device_debug) { \
        printk(KERN_INFO "[RG_IO_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RG_IO_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_rg_io_dev_device_error) { \
        printk(KERN_ERR "[RG_IO_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static io_dev_device_t io_dev_device_data0 = {
    .io_dev_name = "cpld0",
    .io_base = 0x700,
    .io_len = 0x100,
    .indirect_addr = 0,
};

static io_dev_device_t io_dev_device_data1 = {
    .io_dev_name = "cpld1",
    .io_base = 0x900,
    .io_len = 0x100,
    .indirect_addr = 0,
};

static io_dev_device_t io_dev_device_data2 = {
    .io_dev_name = "cpld2",
    .io_base = 0xb00,
    .io_len = 0x100,
    .indirect_addr = 0,
};

static io_dev_device_t io_dev_device_data3 = {
    .io_dev_name = "cpld3",
    .io_base = 0x900,
    .io_len = 0x2000,
    .indirect_addr = 1,
    .wr_data = 0xfb,
    .addr_low = 0xfc,
    .addr_high = 0xfd,
    .rd_data = 0xfe,
    .opt_ctl = 0xff,
};

static void rg_io_dev_device_release(struct device *dev)
{
    return;
}

static struct platform_device io_dev_device[] = {
    {
        .name   = "rg-io-dev",
        .id = 1,
        .dev    = {
            .platform_data  = &io_dev_device_data0,
            .release = rg_io_dev_device_release,
        },
    },
    {
        .name   = "rg-io-dev",
        .id = 2,
        .dev    = {
            .platform_data  = &io_dev_device_data1,
            .release = rg_io_dev_device_release,
        },
    },
    {
        .name   = "rg-io-dev",
        .id = 3,
        .dev    = {
            .platform_data  = &io_dev_device_data2,
            .release = rg_io_dev_device_release,
        },
    },
    {
        .name   = "rg-io-dev",
        .id = 4,
        .dev    = {
            .platform_data  = &io_dev_device_data3,
            .release = rg_io_dev_device_release,
        },
    },
};

static int __init rg_io_dev_device_init(void)
{
    int i;
    int ret = 0;
    io_dev_device_t *io_dev_device_data;

    RG_IO_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(io_dev_device); i++) {
        io_dev_device_data = io_dev_device[i].dev.platform_data;
        ret = platform_device_register(&io_dev_device[i]);
        if (ret < 0) {
            io_dev_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "rg-io-dev.%d register failed!\n", i + 1);
        } else {
            io_dev_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit rg_io_dev_device_exit(void)
{
    int i;
    io_dev_device_t *io_dev_device_data;

    RG_IO_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(io_dev_device) - 1; i >= 0; i--) {
        io_dev_device_data = io_dev_device[i].dev.platform_data;
        if (io_dev_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&io_dev_device[i]);
        }
    }
}

module_init(rg_io_dev_device_init);
module_exit(rg_io_dev_device_exit);
MODULE_DESCRIPTION("RG IO DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic_rd@ruijie.com.cn");
