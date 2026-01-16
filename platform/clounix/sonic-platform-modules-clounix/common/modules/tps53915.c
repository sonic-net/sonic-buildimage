// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver// SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Hardware monitoring driver for MPS Multi-phase Digital VR Controllers
 *
 * Copyright (C) 2020 Nvidia Technologies Ltd.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pmbus.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include "pmbus.h"

#define TPS53915_PAGE_NUM 1
static struct pmbus_driver_info tps53915_info = {
    .pages = TPS53915_PAGE_NUM,
    .format[PSC_TEMPERATURE] = linear,
    .func[0] = PMBUS_HAVE_TEMP,
};

#define TPS_VOUT_MARGIN 0xD5
#define TPS_VOM_STEP_NUM 13
const char *margin_up[] = {"0", "+0.9", "+1.8", "+2.8", "+3.7", "+4.7", "+5.7", "+6.7", "+7.7", "+8.8", "+9.9", "+10.9", "+12.0"};
const char *margin_down[] = {"0", "-1.1", "-2.1", "-3.2", "-4.2", "-5.2", "-6.2", "-7.1", "-8.1", "-9.0", "-9.9", "-10.7", "-11.6"};

ssize_t vout_margin_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int status = 0;
    unsigned int vom_val_h, vom_val_l;
    struct i2c_client *client = to_i2c_client(dev->parent);

    status = i2c_smbus_read_byte_data(client, TPS_VOUT_MARGIN);
    if (status >= 0)
    {
        vom_val_h = (status & 0xF0) >> 4;
        vom_val_l = status & 0xF;

        if(vom_val_h > 0)
        {
            if(vom_val_h >= (TPS_VOM_STEP_NUM - 1))
            {
                return sprintf(buf, "%s%%\n", margin_up[TPS_VOM_STEP_NUM - 1]);
            }
            else
            {
                return sprintf(buf, "%s%%\n", margin_up[vom_val_h]);
            }
        }
        else
        {
            if(vom_val_l >= (TPS_VOM_STEP_NUM - 1))
            {
                return sprintf(buf, "%s%%\n", margin_down[TPS_VOM_STEP_NUM - 1]);
            }
            else
            {
                return sprintf(buf, "%s%%\n", margin_down[vom_val_l]);
            }
        }
    }

    return status;
}

SENSOR_DEVICE_ATTR(vout_margin, S_IWUSR|S_IRUGO, vout_margin_show, NULL, TPS_VOUT_MARGIN);

static struct attribute *tps_psu_attrs[] = {
    &sensor_dev_attr_vout_margin.dev_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = tps_psu_attrs,
};

static const struct attribute_group *attr_groups[] = {
    &attr_group,
    NULL,
};

static int tps_read_byte_data(struct i2c_client *client, int page, int reg)
{
    int rv = 0;
    switch(reg) {
        case TPS_VOUT_MARGIN:
            rv = pmbus_read_byte_data(client, page, reg);
            return (rv&0xFF);

        default:
            break;
    }

    return -ENODATA;
}

static struct pmbus_platform_data pdata = {
    .flags = PMBUS_SKIP_STATUS_CHECK,
};
static int tps_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;

    printk(KERN_INFO "load xdpe132g5h driver\n");
    client->dev.platform_data = &pdata;

    info = devm_kmemdup(&client->dev, &tps53915_info, sizeof(*info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    info->read_byte_data = tps_read_byte_data;
    info->groups = attr_groups;

    return pmbus_do_probe(client, info);
}

static const struct i2c_device_id tps_id[] = {
    {"tps53915", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, tps_id);

static const struct of_device_id __maybe_unused tps_of_match[] = {
    {.compatible = "TI,tps53915"},
    {}
};
MODULE_DEVICE_TABLE(of, tps_of_match);

static struct i2c_driver tps_driver = {
    .driver = {
        .name = "Clounix tps",
        .of_match_table = of_match_ptr(tps_of_match),
    },
    .probe = tps_probe,
    .id_table = tps_id,
};
module_i2c_driver(tps_driver);

MODULE_AUTHOR("daiaq@clounix.com");
MODULE_DESCRIPTION("PMBus driver for TPS53915");
MODULE_LICENSE("GPL");
