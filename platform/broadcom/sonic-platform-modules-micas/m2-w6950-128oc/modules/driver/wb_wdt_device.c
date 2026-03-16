#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_wdt.h>

static int g_wb_wdt_device_debug = 0;
static int g_wb_wdt_device_error = 0;

module_param(g_wb_wdt_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_wdt_device_error, int, S_IRUGO | S_IWUSR);

#define WB_WDT_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_wdt_device_debug) { \
        printk(KERN_INFO "[WB_WDT_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_WDT_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_wdt_device_error) { \
        printk(KERN_ERR "[WB_WDT_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static wb_wdt_device_t wb_wdt_device_data_0 = {

};

/* cpu led */
static wb_wdt_device_t wb_wdt_device_data_1 = {

};

static void wb_wdt_device_release(struct device *dev)
{
    return;
}

static struct platform_device wb_wdt_device[] = {

};

static int __init wb_wdt_device_init(void)
{
    int i;
    int ret = 0;
    wb_wdt_device_t *wb_wdt_device_data;

    WB_WDT_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(wb_wdt_device); i++) {
        wb_wdt_device_data = wb_wdt_device[i].dev.platform_data;
        ret = platform_device_register(&wb_wdt_device[i]);
        if (ret < 0) {
            wb_wdt_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-wdt.%d register failed!\n", i + 1);
        } else {
            wb_wdt_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_wdt_device_exit(void)
{
    int i;
    wb_wdt_device_t *wb_wdt_device_data;

    WB_WDT_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(wb_wdt_device) - 1; i >= 0; i--) {
        wb_wdt_device_data = wb_wdt_device[i].dev.platform_data;
        if (wb_wdt_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&wb_wdt_device[i]);
        }
    }
}

module_init(wb_wdt_device_init);
module_exit(wb_wdt_device_exit);
MODULE_DESCRIPTION("WB WDT Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
