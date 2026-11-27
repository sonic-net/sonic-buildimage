#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

/* CPLD-I2C-MASTER-1 */
static fpga_i2c_bus_device_t cpld_i2c_bus_device_data0 = {
    .adap_nr                 = 445,
    .i2c_timeout             = 3000,
    .i2c_scale               = 0x00,
    .i2c_filter              = 0x04,
    .i2c_stretch             = 0x08,
    .i2c_ext_9548_exits_flag = 0x0c,
    .i2c_ext_9548_addr       = 0x10,
    .i2c_ext_9548_chan       = 0x14,
    .i2c_in_9548_chan        = 0x18,
    .i2c_slave               = 0x1c,
    .i2c_reg                 = 0x20,
    .i2c_reg_len             = 0x30,
    .i2c_data_len            = 0x34,
    .i2c_ctrl                = 0x38,
    .i2c_status              = 0x3c,
    .i2c_err_vec             = 0x48,
    .i2c_data_buf            = 0x100,
    .i2c_data_buf_len        = 256,
    .dev_name                = "/dev/cpld10",
    .i2c_scale_value         = 0x4e,
    .i2c_filter_value        = 0x7c,
    .i2c_stretch_value       = 0x7c,
    .i2c_func_mode           = 5,
    .i2c_adap_reset_flag     = 1,
    .i2c_reset_addr          = 0x7c,
    .i2c_reset_on            = 0x00000001,
    .i2c_reset_off           = 0x00000000,
    .i2c_rst_delay_b         = 0, /* delay time before reset(us) */
    .i2c_rst_delay           = 1, /* reset time(us) */
    .i2c_rst_delay_a         = 1, /* delay time after reset(us) */
};

static void wb_fpga_i2c_bus_device_release(struct device *dev)
{
    return;
}

static struct platform_device fpga_i2c_bus_device[] = {
    {
        .name = "wb-fpga-i2c",
        .id = 45,
        .dev = {
            .platform_data = &cpld_i2c_bus_device_data0,
            .release = wb_fpga_i2c_bus_device_release,
        },
    },
};

static int __init wb_fpga_i2c_bus_device_init(void)
{
    int i;
    int ret = 0;
    fpga_i2c_bus_device_t *fpga_i2c_bus_device_data;

    DEBUG_VERBOSE("enter!\n");
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

    DEBUG_VERBOSE("enter!\n");
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
