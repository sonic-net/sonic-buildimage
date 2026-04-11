#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

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
    DEBUG_VERBOSE("wb_gpio_device_init enter!\n");
    return platform_device_register(&wb_gpio_d1500_device);
}

static void __exit wb_gpio_device_exit(void)
{
    DEBUG_VERBOSE("wb_gpio_device_exit enter!\n");
    return platform_device_unregister(&wb_gpio_d1500_device);
}

module_init(wb_gpio_device_init);
module_exit(wb_gpio_device_exit);
MODULE_DESCRIPTION("GPIO Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
