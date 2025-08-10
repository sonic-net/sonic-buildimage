#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/machine.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <linux/miscdevice.h>

static int gpio_sda = 17;
module_param(gpio_sda, int, S_IRUGO | S_IWUSR);

static int gpio_scl = 1;
module_param(gpio_scl, int, S_IRUGO | S_IWUSR);

static int gpio_udelay = 2;
module_param(gpio_udelay, int, S_IRUGO | S_IWUSR);

static int g_wb_i2c_gpio_device_debug = 0;
static int g_wb_i2c_gpio_device_error = 0;

module_param(g_wb_i2c_gpio_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_i2c_gpio_device_error, int, S_IRUGO | S_IWUSR);

#define WB_I2C_GPIO_DEVICE_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_i2c_gpio_device_debug) { \
        printk(KERN_INFO "[WB_I2C_GPIO_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_I2C_GPIO_DEVICE_ERROR(fmt, args...) do {                                        \
    if (g_wb_i2c_gpio_device_error) { \
        printk(KERN_ERR "[WB_I2C_GPIO_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

/****************** i2c adapter with gpio ***********************/
static struct i2c_gpio_platform_data i2c_pdata = {
    .udelay = 2,
    .scl_is_output_only = 0,
    .sda_is_open_drain  = 0,
    .scl_is_open_drain = 0,
};

static void i2c_gpio_release(struct device *dev)
{
    return;
}

static struct platform_device wb_i2c_gpio_device = {
    .name       = "wb-i2c-gpio",
    .id     = -1,
    .num_resources  = 0,
    .resource   = NULL,
    .dev        = {
        .platform_data = &i2c_pdata,
        .release = i2c_gpio_release,
    },
};

/*
 * i2c
 */
static struct gpiod_lookup_table wb_i2c_gpio_table = {
    .dev_id = "wb-i2c-gpio",
    .table = {
        GPIO_LOOKUP_IDX("wb_gpio_d1500", 17, NULL, 0,
                GPIO_ACTIVE_HIGH | GPIO_OPEN_DRAIN),
        GPIO_LOOKUP_IDX("wb_gpio_d1500", 1, NULL, 1,
                GPIO_ACTIVE_HIGH | GPIO_OPEN_DRAIN),
    },
};

static int __init wb_i2c_gpio_device_init(void)
{
    int err;

    WB_I2C_GPIO_DEVICE_VERBOSE("wb_i2c_gpio_device_init enter!\n");
    wb_i2c_gpio_table.table[0].chip_hwnum = gpio_sda;
    wb_i2c_gpio_table.table[1].chip_hwnum = gpio_scl;
    i2c_pdata.udelay = gpio_udelay;
    gpiod_add_lookup_table(&wb_i2c_gpio_table);

    err = platform_device_register(&wb_i2c_gpio_device);
    if (err < 0) {
        printk(KERN_ERR "register i2c gpio device fail(%d). \n", err);
        gpiod_remove_lookup_table(&wb_i2c_gpio_table);
        return -1;
    }
    return 0;
}

static void __exit wb_i2c_gpio_device_exit(void)
{
    WB_I2C_GPIO_DEVICE_VERBOSE("wb_i2c_gpio_device_exit enter!\n");
    platform_device_unregister(&wb_i2c_gpio_device);
    gpiod_remove_lookup_table(&wb_i2c_gpio_table);
}

module_init(wb_i2c_gpio_device_init);
module_exit(wb_i2c_gpio_device_exit);
MODULE_DESCRIPTION("I2C GPIO Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
