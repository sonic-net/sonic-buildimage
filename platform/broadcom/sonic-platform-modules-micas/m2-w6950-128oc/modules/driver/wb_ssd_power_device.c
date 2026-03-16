#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_ssd_power.h>

static int g_wb_ssd_power_device_debug = 0;
static int g_wb_ssd_power_device_error = 0;

module_param(g_wb_ssd_power_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_ssd_power_device_error, int, S_IRUGO | S_IWUSR);

#define WB_SSD_POWER_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_ssd_power_device_debug) { \
        printk(KERN_INFO "[WB_SSD_POWER_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SSD_POWER_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_ssd_power_device_error) { \
        printk(KERN_ERR "[WB_SSD_POWER_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static ssd_power_device_t ssd_power_device_data0 = {

};

static ssd_power_device_t ssd_power_device_data1 = {

};

static void wb_ssd_power_device_release(struct device *dev)
{
    return;
}

static struct platform_device ssd_power_device[] = {

};

static int __init wb_ssd_power_device_init(void)
{
    int i;
    int ret = 0;
    ssd_power_device_t *ssd_power_device_data;

    WB_SSD_POWER_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(ssd_power_device); i++) {
        ssd_power_device_data = ssd_power_device[i].dev.platform_data;
        ret = platform_device_register(&ssd_power_device[i]);
        if (ret < 0) {
            ssd_power_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-io-dev.%d register failed!\n", i + 1);
        } else {
            ssd_power_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_ssd_power_device_exit(void)
{
    int i;
    ssd_power_device_t *ssd_power_device_data;

    WB_SSD_POWER_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(ssd_power_device) - 1; i >= 0; i--) {
        ssd_power_device_data = ssd_power_device[i].dev.platform_data;
        if (ssd_power_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&ssd_power_device[i]);
        }
    }
}

module_init(wb_ssd_power_device_init);
module_exit(wb_ssd_power_device_exit);
MODULE_DESCRIPTION("SSD POWER DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
