#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

#define FPGA_INTERNAL_PCA9548        (1)
#define FPGA_EXTERNAL_PCA9548        (2)

static int g_wb_fpga_pca954x_device_debug = 0;
static int g_wb_fpga_pca954x_device_error = 0;

module_param(g_wb_fpga_pca954x_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_fpga_pca954x_device_error, int, S_IRUGO | S_IWUSR);

#define WB_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_fpga_pca954x_device_debug) { \
        printk(KERN_INFO "[WB_FPGA_PCA954X_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_FPGA_PCA954X_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_fpga_pca954x_device_error) { \
        printk(KERN_ERR "[WB_FPGA_PCA954X_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static fpga_pca954x_device_t fpga_pca954x_device_data0 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data1 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data2 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data3 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data4 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data5 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data6 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data7 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data8 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data9 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data10 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data11 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data12 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data13 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data14 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data15 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data16 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data17 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data18 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data19 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data20 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data21 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data22 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data23 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data24 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data25 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data26 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data27 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data28 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data29 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data30 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data31 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data32 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data33 = {

};

static fpga_pca954x_device_t fpga_pca954x_device_data34 = {

};

struct i2c_board_info fpga_pca954x_device_info[] = {

};

static int __init wb_fpga_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    WB_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_pca954x_device_info); i++) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        fpga_pca954x_device_info[i].addr = fpga_pca954x_device_data->i2c_addr;
        adap = i2c_get_adapter(fpga_pca954x_device_data->i2c_bus);
        if (adap == NULL) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", fpga_pca954x_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &fpga_pca954x_device_info[i]);
        if (!client) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "Failed to register fpga pca954x device %d at bus %d!\n",
                   fpga_pca954x_device_data->i2c_addr, fpga_pca954x_device_data->i2c_bus);
        } else {
            fpga_pca954x_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_fpga_pca954x_device_exit(void)
{
    int i;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    WB_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_pca954x_device_info) - 1; i >= 0; i--) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        if (fpga_pca954x_device_data->client) {
            i2c_unregister_device(fpga_pca954x_device_data->client);
            fpga_pca954x_device_data->client = NULL;
        }
    }
}

module_init(wb_fpga_pca954x_device_init);
module_exit(wb_fpga_pca954x_device_exit);
MODULE_DESCRIPTION("FPGA PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
