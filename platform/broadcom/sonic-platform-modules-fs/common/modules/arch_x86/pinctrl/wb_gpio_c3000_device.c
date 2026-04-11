#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include "wb_pinctrl_intel.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

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
    DEBUG_VERBOSE("wb_c3000_gpio_device_init enter!\n");
    return platform_device_register(&c3000_gpio_device);

}

static void __exit wb_c3000_gpio_device_exit(void)
{

    DEBUG_VERBOSE("wb_c3000_gpio_device_exit enter!\n");
    platform_device_unregister(&c3000_gpio_device);
    return;
}

module_init(wb_c3000_gpio_device_init);
module_exit(wb_c3000_gpio_device_exit);
MODULE_DESCRIPTION("C3000 GPIO Controller device");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
