#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

static int g_rg_gpio_device_debug = 0;
static int g_rg_gpio_device_error = 0;

module_param(g_rg_gpio_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_rg_gpio_device_error, int, S_IRUGO | S_IWUSR);

#define RG_GPIO_DEVICE_VERBOSE(fmt, args...) do {                                        \
    if (g_rg_gpio_device_debug) { \
        printk(KERN_INFO "[RG_GPIO_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RG_GPIO_DEVICE_ERROR(fmt, args...) do {                                        \
    if (g_rg_gpio_device_error) { \
        printk(KERN_ERR "[RG_GPIO_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static void rg_gpio_device_release(struct device *dev)
{
    return;
}

static struct platform_device rg_gpio_d1500_device = {
    .name   = "rg_gpio_d1500",
    .id = -1,
    .dev    = {
        .release = rg_gpio_device_release,
    },
};

static int __init rg_gpio_device_init(void)
{
    RG_GPIO_DEVICE_VERBOSE("rg_gpio_device_init enter!\n");
    return platform_device_register(&rg_gpio_d1500_device);
}

static void __exit rg_gpio_device_exit(void)
{
    RG_GPIO_DEVICE_VERBOSE("rg_gpio_device_exit enter!\n");
    return platform_device_unregister(&rg_gpio_d1500_device);
}

module_init(rg_gpio_device_init);
module_exit(rg_gpio_device_exit);
MODULE_DESCRIPTION("GPIO Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic_rd@ruijie.com.cn");
