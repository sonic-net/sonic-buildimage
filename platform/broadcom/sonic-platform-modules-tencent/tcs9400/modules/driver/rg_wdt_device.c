#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <rg_wdt.h>

static int g_rg_wdt_device_debug = 0;
static int g_rg_wdt_device_error = 0;

module_param(g_rg_wdt_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_rg_wdt_device_error, int, S_IRUGO | S_IWUSR);

#define RG_WDT_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_rg_wdt_device_debug) { \
        printk(KERN_INFO "[RG_WDT_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RG_WDT_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_rg_wdt_device_error) { \
        printk(KERN_ERR "[RG_WDT_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static rg_wdt_device_t rg_wdt_device_data_0 = {
    .feed_wdt_type = 2,
    .hw_margin = 180000,
    .feed_time = 30000,
    .config_dev_name = "/dev/cpld5",
    .config_mode = 2,
    .priv_func_mode = 4,
    .enable_reg = 0x42,
    .enable_val = 0x1,
    .disable_val = 0x0,
    .enable_mask = 0x1,
    .timeout_cfg_reg = 0x43,
    .hw_algo = "toggle",
    .wdt_config_mode.logic_wdt = {
        .feed_dev_name = "/dev/cpld5",
        .feed_reg = 0x42,
        .active_val = 0x1,
        .logic_func_mode = 0x1,
    },
    .timer_accuracy = 6000,          /* 6s */
    .sysfs_index = SYSFS_NO_CFG,
};

static void rg_wdt_device_release(struct device *dev)
{
    return;
}

static struct platform_device rg_wdt_device[] = {
    {
        .name   = "rg_wdt",
        .id = 0,
        .dev    = {
            .platform_data  = &rg_wdt_device_data_0,
            .release = rg_wdt_device_release,
        },
    },
};

static int __init rg_wdt_device_init(void)
{
    int i;
    int ret = 0;
    rg_wdt_device_t *rg_wdt_device_data;

    RG_WDT_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(rg_wdt_device); i++) {
        rg_wdt_device_data = rg_wdt_device[i].dev.platform_data;
        ret = platform_device_register(&rg_wdt_device[i]);
        if (ret < 0) {
            rg_wdt_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "rg-wdt.%d register failed!\n", i + 1);
        } else {
            rg_wdt_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit rg_wdt_device_exit(void)
{
    int i;
    rg_wdt_device_t *rg_wdt_device_data;

    RG_WDT_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(rg_wdt_device) - 1; i >= 0; i--) {
        rg_wdt_device_data = rg_wdt_device[i].dev.platform_data;
        if (rg_wdt_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&rg_wdt_device[i]);
        }
    }
}

module_init(rg_wdt_device_init);
module_exit(rg_wdt_device_exit);
MODULE_DESCRIPTION("RG WDT Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic_rd@ruijie.com.cn");
