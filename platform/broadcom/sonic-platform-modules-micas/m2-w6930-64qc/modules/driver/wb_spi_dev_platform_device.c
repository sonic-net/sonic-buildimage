#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

static int g_wb_spi_dev_platform_device_debug = 0;
static int g_wb_spi_dev_platform_device_error = 0;

module_param(g_wb_spi_dev_platform_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_dev_platform_device_error, int, S_IRUGO | S_IWUSR);

#define WB_SPI_DEV_PLATFORM_DEVICE_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_dev_platform_device_debug) { \
        printk(KERN_INFO "[WB_SPI_DEV_PLATFORM_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SPI_DEV_PLATFORM_DEVICE_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_dev_platform_device_error) { \
        printk(KERN_ERR "[WB_SPI_DEV_PLATFORM_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static void wb_spi_dev_platform_device_release(struct device *dev)
{
    return;
}

static struct platform_device wb_spi_dev_platform_device = {
    .name   = "wb-spi-dev-device",
    .id = 1,
    .dev    = {
        .release = wb_spi_dev_platform_device_release,
    },
};

static int __init wb_spi_dev_platform_device_init(void)
{
    WB_SPI_DEV_PLATFORM_DEVICE_VERBOSE("wb_spi_dev_platform_device_init enter!\n");
    return platform_device_register(&wb_spi_dev_platform_device);
}

static void __exit wb_spi_dev_platform_device_exit(void)
{
    WB_SPI_DEV_PLATFORM_DEVICE_VERBOSE("wb_spi_dev_platform_device_exit enter!\n");
    return platform_device_unregister(&wb_spi_dev_platform_device);
}

module_init(wb_spi_dev_platform_device_init);
module_exit(wb_spi_dev_platform_device_exit);
MODULE_DESCRIPTION("SPI Dev Platform Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
