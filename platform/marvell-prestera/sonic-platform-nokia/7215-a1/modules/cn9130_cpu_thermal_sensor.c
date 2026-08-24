/*
 * HWMON Driver for CN9130 thermal sensor
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

#define CN9130_DEFAULT_TEMP_CRIT 100000
#define CN9130_DEFAULT_TEMP_MAX  106000

#define CN9130_TEMP_BASE_ADDR 0xF06F8080
#define CN9130_TSEN_REG_CTRL_0_OFFSET 0x4
#define CN9130_TSEN_REG_CTRL_1_OFFSET 0x8
#define CN9130_TSEN_REG_STATUS_OFFSET 0xC
#define CN9130_TSEN_SENSOR_MAX_ID 6
static unsigned long thermal_base_addr=CN9130_TEMP_BASE_ADDR;
module_param(thermal_base_addr, ulong, 0444);
MODULE_PARM_DESC(thermal_base_addr,
        "Initialize the base address of the thermal sensor");

struct cn9130_thermal_data {
    struct device *dev;
    struct device *hwmon_dev;
    uint8_t * __iomem temp_base;
    int temp_input;
    int temp_crit;
    int temp_max;
};

static long cn9130_thermal_read_reg_in_mcelcius(struct device *dev, struct cn9130_thermal_data *data)
{
    volatile uint8_t * __iomem temp_base = data->temp_base;
    uint32_t regval;
    uint32_t status_regval=0;
    uint32_t output=data->temp_max;

    //STOP MEASUREMENT
    regval = readl(temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);
    regval &= ~( 1 << 0); //TSEN_STOP
    writel(regval, temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);

    //delay for 1ms
    mdelay(1);

    //Read thermal value
    status_regval = readl(temp_base+CN9130_TSEN_REG_STATUS_OFFSET);
    dev_dbg(dev, "%s: cn9130_thermal_read_reg_in_mcelcius: addr: %p value:0x%x\n",
	    dev_name(data->hwmon_dev), (void*)(temp_base+CN9130_TSEN_REG_STATUS_OFFSET), status_regval);

    //START MEASUREMENT
    regval = readl(temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);
    regval |= 1 << 0; //TSEN_START
    writel(regval, temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);

    //Validate data
    if(status_regval &= 0x3ff) {
        //Convert it to milli-celcius
        output = 150000 - (~(status_regval-1) & 0x3ff) * 423;
    }

    return output;
}
static int cn9130_thermal_read(struct device *dev, enum hwmon_sensor_types type,
        u32 attr, int channel, long *val)
{
    struct cn9130_thermal_data *data = dev_get_drvdata(dev);

    switch (type) {
        case hwmon_temp:
            switch (attr) {
                case hwmon_temp_input:
                    *val = cn9130_thermal_read_reg_in_mcelcius(dev, data);
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

static int cn9130_thermal_write(struct device *dev, enum hwmon_sensor_types type,
        u32 attr, int channel, long val)
{
    struct cn9130_thermal_data *data = dev_get_drvdata(dev);
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


static umode_t cn9130_thermal_is_visible(const void *data, enum hwmon_sensor_types type,
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

static const struct hwmon_channel_info *cn9130_thermal_info[] = {
    HWMON_CHANNEL_INFO(temp,
            HWMON_T_INPUT | HWMON_T_MAX | HWMON_T_CRIT),
    NULL
};

static const struct hwmon_ops cn9130_thermal_hwmon_ops = {
    .is_visible = cn9130_thermal_is_visible,
    .read = cn9130_thermal_read,
    .write = cn9130_thermal_write,
};

static const struct hwmon_chip_info cn9130_thermal_chip_info = {
    .ops = &cn9130_thermal_hwmon_ops,
    .info = cn9130_thermal_info,
};

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
};

struct miscdevice cn9130_thermal_device = {
    .minor = TEMP_MINOR,
    .name = "cn9130_thermal",
    .fops = &fops,
};

static int __init cn9130_thermal_init_driver(void)
{
    struct device *dev;
    struct cn9130_thermal_data *thermal_data;
    int err;
    void * __iomem reg;
    uint32_t regval=0;

    err = misc_register(&cn9130_thermal_device);
    if (err) {
        pr_err("cn9130_thermal misc_register failed!!!\n");
        return err;
    }

    dev = cn9130_thermal_device.this_device;
    thermal_data = devm_kzalloc(dev, sizeof(struct cn9130_thermal_data), GFP_KERNEL);
    if (!thermal_data)
        return -ENOMEM;

    thermal_data->dev = dev;
    thermal_data->temp_crit  = CN9130_DEFAULT_TEMP_CRIT;
    thermal_data->temp_max   = CN9130_DEFAULT_TEMP_MAX;

    thermal_data->hwmon_dev = devm_hwmon_device_register_with_info(dev, cn9130_thermal_device.name,
            thermal_data, &cn9130_thermal_chip_info,
            NULL);
    if (IS_ERR(thermal_data->hwmon_dev)) {
        dev_err(dev, "%s: hwmon registration failed.\n", cn9130_thermal_device.name);
        return PTR_ERR(thermal_data->hwmon_dev);
    }

    reg = devm_ioremap(dev, thermal_base_addr, 16);
    if (IS_ERR(reg)) {
        dev_err(dev, "%s: base addr remap failed\n", cn9130_thermal_device.name);
        return PTR_ERR(reg);
    }
    thermal_data->temp_base = reg;

    /*Enable measurement*/
    regval = readl(thermal_data->temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);
    regval |= 1 << 2; //TSEN_EN
    writel(regval, thermal_data->temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);
    mdelay(10);

    // Set temperature reading zone as max reading
    regval = readl(thermal_data->temp_base+CN9130_TSEN_REG_CTRL_1_OFFSET);
    regval &= ~(0x7 << 21);
    regval |= (CN9130_TSEN_SENSOR_MAX_ID & 0x7) << 21;
    writel(regval, thermal_data->temp_base+CN9130_TSEN_REG_CTRL_1_OFFSET);

    //START MEASUREMENT
    regval = readl(thermal_data->temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);
    regval |= 1 << 0; //TSEN_START
    writel(regval, thermal_data->temp_base+CN9130_TSEN_REG_CTRL_0_OFFSET);

    dev_info(dev, "%s: initialized. base_addr: 0x%lx virt_addr:%p\n",
	     dev_name(thermal_data->hwmon_dev), thermal_base_addr, (void *)thermal_data->temp_base);

    return 0;
}

static void __exit cn9101_thermal_exit_driver(void)
{
    misc_deregister(&cn9130_thermal_device);
}

module_init(cn9130_thermal_init_driver);
module_exit(cn9101_thermal_exit_driver);

MODULE_AUTHOR("Natarajan Subbiramani <natarajan.subbiramani.ext@nokia.com>");
MODULE_DESCRIPTION("CN9130 CPU Thermal sensor Driver");
MODULE_LICENSE("GPL");
