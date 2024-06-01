#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_io_dev.h>

static int g_wb_io_dev_device_debug = 0;
static int g_wb_io_dev_device_error = 0;

module_param(g_wb_io_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_io_dev_device_error, int, S_IRUGO | S_IWUSR);

#define WB_IO_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_io_dev_device_debug) { \
        printk(KERN_INFO "[WB_IO_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_IO_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_io_dev_device_error) { \
        printk(KERN_ERR "[WB_IO_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
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

static void wb_io_dev_device_release(struct device *dev)
{
    return;
}

static struct platform_device io_dev_device[] = {
    {
        .name   = "wb-io-dev",
        .id = 1,
        .dev    = {
            .platform_data  = &io_dev_device_data0,
            .release = wb_io_dev_device_release,
        },
    },
    {
        .name   = "wb-io-dev",
        .id = 2,
        .dev    = {
            .platform_data  = &io_dev_device_data1,
            .release = wb_io_dev_device_release,
        },
    },
};

static int __init wb_io_dev_device_init(void)
{
    int i;
    int ret = 0;
    io_dev_device_t *io_dev_device_data;

    WB_IO_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(io_dev_device); i++) {
        io_dev_device_data = io_dev_device[i].dev.platform_data;
        ret = platform_device_register(&io_dev_device[i]);
        if (ret < 0) {
            io_dev_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-io-dev.%d register failed!\n", i + 1);
        } else {
            io_dev_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_io_dev_device_exit(void)
{
    int i;
    io_dev_device_t *io_dev_device_data;

    WB_IO_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(io_dev_device) - 1; i >= 0; i--) {
        io_dev_device_data = io_dev_device[i].dev.platform_data;
        if (io_dev_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&io_dev_device[i]);
        }
    }
}

module_init(wb_io_dev_device_init);
module_exit(wb_io_dev_device_exit);
MODULE_DESCRIPTION("IO DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
