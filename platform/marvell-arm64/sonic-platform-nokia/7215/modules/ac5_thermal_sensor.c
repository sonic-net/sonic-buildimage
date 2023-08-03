/*
 * HWMON Driver for AC5x thermal sensor
 *
 * Author: Natarajan Subbiramani <nataraja.subbiramani.ext@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/delay.h>

#define AC5_DEFAULT_TEMP_CRIT 100000
#define AC5_DEFAULT_TEMP_MAX  110000

#define AC5_TEMP_BASE_ADDR 0x944F80D0
static unsigned long thermal_base_addr=AC5_TEMP_BASE_ADDR;
module_param(thermal_base_addr, ulong, 0444);
MODULE_PARM_DESC(thermal_base_addr,
        "Initialize the base address of the thermal sensor");

struct ac5_thermal_data {
    struct device *dev;
    struct device *hwmon_dev;
    uint8_t * __iomem temp_base;
    int temp_input;
    int temp_crit;
    int temp_max;
};

static long ac5_thermal_read_reg_in_mcelcius(struct device *dev, struct ac5_thermal_data *data)
{
    volatile uint8_t * __iomem temp_base = data->temp_base;
    uint32_t regval;
    long output=data->temp_max;

    //STOP MEASUREMENT
    writel(0xF0F01034, temp_base);

    //delay for 1ms
    mdelay(1);

    //Read thermal value
    regval = readl(temp_base+0xC);

    //RE-START MEASUREMENT
    writel(0xF0F01035, temp_base);

    //Validate data
    if(regval & 0x10000) {
        //Calibrate it to milli-celcius
        output = (regval>> 6) & 0x3FF;
        output = ((output*42)-27250)*10;
    }

    return output;
}
static int ac5_thermal_read(struct device *dev, enum hwmon_sensor_types type,
        u32 attr, int channel, long *val)
{
    struct ac5_thermal_data *data = dev_get_drvdata(dev);

    switch (type) {
        case hwmon_temp:
            switch (attr) {
                case hwmon_temp_input:
                    *val = ac5_thermal_read_reg_in_mcelcius(dev, data);
                    break;
                case hwmon_temp_crit:
                    *val = data->temp_crit;
                    break;
                case hwmon_temp_max:
                    *val = data->temp_max;
                    break;
                default:
                    return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static int ac5_thermal_write(struct device *dev, enum hwmon_sensor_types type,
        u32 attr, int channel, long val)
{
    struct ac5_thermal_data *data = dev_get_drvdata(dev);
    switch (type) {
        case hwmon_temp:
            switch (attr) {
                case hwmon_temp_crit:
                    data->temp_crit = val;
                    break;
                case hwmon_temp_max:
                    data->temp_max = val;
                    break;
                default:
                    return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}


static umode_t ac5_thermal_is_visible(const void *data, enum hwmon_sensor_types type,
        u32 attr, int channel)
{
    switch (type) {
        case hwmon_temp:
            switch (attr) {
                case hwmon_temp_input:
                    return 0444;
                case hwmon_temp_crit:
                case hwmon_temp_max:
                    return 0644;
            }
            break;
        default:
            break;
    }
    return 0;
}

static const struct hwmon_channel_info *ac5_thermal_info[] = {
    HWMON_CHANNEL_INFO(temp,
            HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT),
    NULL
};

static const struct hwmon_ops ac5_thermal_hwmon_ops = {
    .is_visible = ac5_thermal_is_visible,
    .read = ac5_thermal_read,
    .write = ac5_thermal_write,
};

static const struct hwmon_chip_info ac5_thermal_chip_info = {
    .ops = &ac5_thermal_hwmon_ops,
    .info = ac5_thermal_info,
};

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
};

struct miscdevice ac5_thermal_misc_device = {
    .minor = TEMP_MINOR,
    .name = "ac5_thermal",
    .fops = &fops,
};

static int __init ac5_thermal_init_misc_driver(void)
{
    struct device *dev;
    struct ac5_thermal_data *thermal_data;
    int err;
    void * __iomem reg;

    err = misc_register(&ac5_thermal_misc_device);
    if (err) {
        pr_err("ac5_thermal misc_register failed!!!\n");
        return err;
    }

    dev = ac5_thermal_misc_device.this_device;
    thermal_data = devm_kzalloc(dev, sizeof(struct ac5_thermal_data), GFP_KERNEL);
    if (!thermal_data)
        return -ENOMEM;

    thermal_data->dev = dev;
    thermal_data->temp_crit  = AC5_DEFAULT_TEMP_CRIT;
    thermal_data->temp_max   = AC5_DEFAULT_TEMP_MAX;

    thermal_data->hwmon_dev = devm_hwmon_device_register_with_info(dev, ac5_thermal_misc_device.name,
            thermal_data, &ac5_thermal_chip_info,
            NULL);
    if (IS_ERR(thermal_data->hwmon_dev)) {
        dev_err(dev, "%s: hwmon registration failed.\n", ac5_thermal_misc_device.name);
        return PTR_ERR(thermal_data->hwmon_dev);
    }

    reg = devm_ioremap(dev, thermal_base_addr, 16);
    if (IS_ERR(reg)) {
        dev_err(dev, "%s: base addr remap failed\n", ac5_thermal_misc_device.name);
        return PTR_ERR(reg);
    }
    thermal_data->temp_base = reg;
    /*Enable measurement*/
    writel(0xF0F01035, thermal_data->temp_base);
    writel(0x0584e680, thermal_data->temp_base+8);

    dev_info(dev, "%s: initialized. base_addr: 0x%lx\n", dev_name(thermal_data->hwmon_dev), thermal_base_addr);

    return 0;
}

static void __exit ac5_thermal_exit_misc_driver(void)
{
    misc_deregister(&ac5_thermal_misc_device);
}

module_init(ac5_thermal_init_misc_driver);
module_exit(ac5_thermal_exit_misc_driver);

MODULE_AUTHOR("Natarajan Subbiramani <natarajan.subbiramani.ext@nokia.com>");
MODULE_DESCRIPTION("AC5 Thermal sensor Driver");
MODULE_LICENSE("GPL");
