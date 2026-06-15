#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

static int g_wb_fpga_i2c_debug = 0;
static int g_wb_fpga_i2c_error = 0;

module_param(g_wb_fpga_i2c_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_fpga_i2c_error, int, S_IRUGO | S_IWUSR);

#define WB_FPGA_I2C_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_fpga_i2c_debug) { \
        printk(KERN_INFO "[WB_FPGA_I2C][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_FPGA_I2C_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_fpga_i2c_error) { \
        printk(KERN_ERR "[WB_FPGA_I2C][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

/* mac fpga i2c master */
static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data0 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data1 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data2 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data3 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data4 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data5 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data6 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data7 = {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data8= {

};

static fpga_i2c_bus_device_t fpga0_i2c_bus_device_data9 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data0 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data1 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data2 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data3 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data4 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data5 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data6 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data7 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data8 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data9 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data10 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data11 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data12 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data13 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data14 = {

};

static fpga_i2c_bus_device_t fpga0_dom_i2c_bus_device_data15 = {

};

/* uport fpga i2c master */
static fpga_i2c_bus_device_t fpga1_i2c_bus_device_data0 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data0 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data1 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data2 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data3 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data4 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data5 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data6 = {

};

static fpga_i2c_bus_device_t fpga1_dom_i2c_bus_device_data7 = {

};

/* dport fpga i2c master */
static fpga_i2c_bus_device_t fpga2_i2c_bus_device_data0 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data0 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data1 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data2 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data3 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data4 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data5 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data6 = {

};

static fpga_i2c_bus_device_t fpga2_dom_i2c_bus_device_data7 = {

};

static void wb_fpga_i2c_bus_device_release(struct device *dev)
{
    return;
}

static struct platform_device fpga_i2c_bus_device[] = {

};

static int __init wb_fpga_i2c_bus_device_init(void)
{
    int i;
    int ret = 0;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    WB_FPGA_I2C_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_i2c_bus_device); i++) {
        fpga_i2c_bus_device_data = fpga_i2c_bus_device[i].dev.platform_data;
        ret = platform_device_register(&fpga_i2c_bus_device[i]);
        if (ret < 0) {
            fpga_i2c_bus_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-fpga-i2c.%d register failed!\n", i + 1);
        } else {
            fpga_i2c_bus_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_fpga_i2c_bus_device_exit(void)
{
    int i;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    WB_FPGA_I2C_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_i2c_bus_device) - 1; i >= 0; i--) {
        fpga_i2c_bus_device_data = fpga_i2c_bus_device[i].dev.platform_data;
        if (fpga_i2c_bus_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&fpga_i2c_bus_device[i]);
        }
    }
}

module_init(wb_fpga_i2c_bus_device_init);
module_exit(wb_fpga_i2c_bus_device_exit);
MODULE_DESCRIPTION("FPGA I2C Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
