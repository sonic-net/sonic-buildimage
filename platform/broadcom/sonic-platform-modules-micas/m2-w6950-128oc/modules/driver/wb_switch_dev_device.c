#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mii.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <wb_mdio_dev.h>
#include <wb_switch_dev.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static void wb_switch_dev_device_release(struct device *dev)
{
    return;
}

static switch_dev_info_t switch_dev_info_array[] = {
    {
        .mdio_bus_name = "gpio",
        .mdio_dev_device = {
            { .dev_name = "cpu_phy" },
            { .dev_name = "mgmt_phy"},
            { .dev_name = "port2_phy"},
            { .dev_name = "port3_phy"},
            { .dev_name = "sfp_phy"},
        },
    },
};

/* Platform devices for switches */
static struct platform_device switch_dev_device_array[] = {
    {
        .name = "bcm53134",
        .id = 1, /* Device ID for switch 0 */
        .dev = {
            .platform_data = &switch_dev_info_array[0],
            .release = wb_switch_dev_device_release,
        },
    },
};

static int __init wb_switch_dev_device_init(void)
{
    int i;
    int ret = 0;
    switch_dev_info_t *switch_dev_info;

    DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(switch_dev_device_array); i++) {
        switch_dev_info = switch_dev_device_array[i].dev.platform_data;
        ret = platform_device_register(&switch_dev_device_array[i]);
        if (ret < 0) {
            switch_dev_info->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "switch device.%d register failed!\n", i + 1);
        } else {
            switch_dev_info->device_flag = 0; /* device register success, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_switch_dev_device_exit(void)
{
    int i;
    switch_dev_info_t *switch_dev_info;

    DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(switch_dev_device_array) - 1; i >= 0; i--) {
        switch_dev_info = switch_dev_device_array[i].dev.platform_data;
        if (switch_dev_info->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&switch_dev_device_array[i]);
        }
    }
}

module_init(wb_switch_dev_device_init);
module_exit(wb_switch_dev_device_exit);
MODULE_DESCRIPTION("Generic Switch Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");