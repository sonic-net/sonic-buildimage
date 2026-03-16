#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_uart_16550_ocore.h>

static int g_wb_uart_16550_ocore_device_debug = 0;
static int g_wb_uart_16550_ocore_device_error = 0;

module_param(g_wb_uart_16550_ocore_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_uart_16550_ocore_device_error, int, S_IRUGO | S_IWUSR);

#define WB_UART_16550_OCORE_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_uart_16550_ocore_device_debug) { \
        printk(KERN_INFO "[WB_UART_16550_OCORE_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_UART_16550_OCORE_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_uart_16550_ocore_device_error) { \
        printk(KERN_ERR "[WB_UART_16550_OCORE_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static uart_16550_ocore_device_t uart_16550_ocore_device_data_0 = {

};

static uart_16550_ocore_device_t uart_16550_ocore_device_data_1 = {

};

static uart_16550_ocore_device_t uart_16550_ocore_device_data_2 = {

};

static uart_16550_ocore_device_t uart_16550_ocore_device_data_3 = {

};

static void wb_uart_16550_ocore_device_release(struct device *dev)
{
    return;
}

static struct platform_device uart_16550_ocore_device[] = {

};

static int __init wb_uart_16550_ocore_device_init(void)
{
    int i;
    int ret = 0;
    uart_16550_ocore_device_t *uart_16550_ocore_device_data;

    WB_UART_16550_OCORE_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(uart_16550_ocore_device); i++) {
        uart_16550_ocore_device_data = uart_16550_ocore_device[i].dev.platform_data;
        ret = platform_device_register(&uart_16550_ocore_device[i]);
        if (ret < 0) {
            uart_16550_ocore_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-uart-16550-ocore.%d register failed!\n", i + 1);
        } else {
            uart_16550_ocore_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_uart_16550_ocore_device_exit(void)
{
    int i;
    uart_16550_ocore_device_t *uart_16550_ocore_device_data;

    WB_UART_16550_OCORE_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(uart_16550_ocore_device) - 1; i >= 0; i--) {
        uart_16550_ocore_device_data = uart_16550_ocore_device[i].dev.platform_data;
        if (uart_16550_ocore_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&uart_16550_ocore_device[i]);
        }
    }
}

module_init(wb_uart_16550_ocore_device_init);
module_exit(wb_uart_16550_ocore_device_exit);
MODULE_DESCRIPTION("UART DRV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
