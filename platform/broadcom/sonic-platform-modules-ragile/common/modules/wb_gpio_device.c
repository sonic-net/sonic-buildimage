#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

static int g_wb_gpio_device_debug = 0;
static int g_wb_gpio_device_error = 0;

module_param(g_wb_gpio_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_gpio_device_error, int, S_IRUGO | S_IWUSR);

#define WB_GPIO_DEVICE_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_gpio_device_debug) { \
        printk(KERN_INFO "[WB_GPIO_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_GPIO_DEVICE_ERROR(fmt, args...) do {                                        \
    if (g_wb_gpio_device_error) { \
        printk(KERN_ERR "[WB_GPIO_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static void wb_gpio_device_release(struct device *dev)
{
    return;
}

static struct platform_device wb_gpio_d1500_device = {
    .name   = "wb_gpio_d1500",
    .id = -1,
    .dev    = {
        .release = wb_gpio_device_release,
    },
};

static int __init wb_gpio_device_init(void)
{
    WB_GPIO_DEVICE_VERBOSE("wb_gpio_device_init enter!\n");
    return platform_device_register(&wb_gpio_d1500_device);
}

static void __exit wb_gpio_device_exit(void)
{
    WB_GPIO_DEVICE_VERBOSE("wb_gpio_device_exit enter!\n");
    return platform_device_unregister(&wb_gpio_d1500_device);
}

module_init(wb_gpio_device_init);
module_exit(wb_gpio_device_exit);
MODULE_DESCRIPTION("GPIO Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
