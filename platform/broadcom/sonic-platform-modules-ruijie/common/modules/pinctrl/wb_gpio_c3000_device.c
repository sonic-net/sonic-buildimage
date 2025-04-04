#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include "wb_pinctrl_intel.h"

static int g_wb_c300_gpio_device_debug = 0;
static int g_wb_c300_gpio_device_error = 0;

module_param(g_wb_c300_gpio_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_c300_gpio_device_error, int, S_IRUGO | S_IWUSR);

#define WB_C3000_GPIO_DEVICE_DEBUG(fmt, args...) do {                                        \
    if (g_wb_c300_gpio_device_debug) { \
        printk(KERN_INFO "[WB_C3000_GPIO_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_C3000_GPIO_DEVICE_ERROR(fmt, args...) do {                                        \
    if (g_wb_c300_gpio_device_error) { \
        printk(KERN_ERR "[WB_C3000_GPIO_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static wb_gpio_data_t c3000_gpio_device_data = {
    .irq = 15,
    .pci_domain = 0x0000,
    .pci_bus = 0x00,
    .pci_slot = 0x1f,
    .pci_fn = 1,
    .pci_bar = 0,
};

static void wb_c3000_gpio_device_release(struct device *dev)
{
    return;
}

static struct platform_device c3000_gpio_device = {
    .name   = "wb_gpio_c3000",
    .id = -1,
    .dev    = {
        .platform_data = &c3000_gpio_device_data,
        .release = wb_c3000_gpio_device_release,
    },
};

static int __init wb_c3000_gpio_device_init(void)
{
    WB_C3000_GPIO_DEVICE_DEBUG("wb_c3000_gpio_device_init enter!\n");
    return platform_device_register(&c3000_gpio_device);

}

static void __exit wb_c3000_gpio_device_exit(void)
{

    WB_C3000_GPIO_DEVICE_DEBUG("wb_c3000_gpio_device_exit enter!\n");
    platform_device_unregister(&c3000_gpio_device);
    return;
}

module_init(wb_c3000_gpio_device_init);
module_exit(wb_c3000_gpio_device_exit);
MODULE_DESCRIPTION("C3000 GPIO Controller device");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
