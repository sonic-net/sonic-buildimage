// SPDX-License-Identifier: GPL-2.0-or-later
/*
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
#include "pmbus.h"

#define XDPE132_PAGE_NUM 2

static struct pmbus_driver_info xdpe132_info = {
    .pages = XDPE132_PAGE_NUM,
    .format[PSC_VOLTAGE_IN] = linear,
    .format[PSC_VOLTAGE_OUT] = linear,
    .format[PSC_TEMPERATURE] = linear,
    .format[PSC_CURRENT_OUT] = linear,
    .format[PSC_CURRENT_IN] = linear,
    .format[PSC_POWER] = linear,

    .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_PIN |
               PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT |
               PMBUS_HAVE_STATUS_TEMP,
    .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_POUT | PMBUS_HAVE_PIN |
               PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT |
               PMBUS_HAVE_STATUS_TEMP,
};

static struct pmbus_platform_data pdata = {
//    .flags = PMBUS_SKIP_STATUS_CHECK,
};
static int xdpe132g5h_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    
    printk(KERN_INFO "load xdpe132g5h driver\n");    
    client->dev.platform_data = &pdata;
    info = devm_kmemdup(&client->dev, &xdpe132_info, sizeof(*info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    
    return pmbus_do_probe(client, info);
}

static const struct i2c_device_id xdpe132g5h_id[] = {
    {"xdpe132g5h", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, xdpe132g5h_id);

static const struct of_device_id __maybe_unused xdpe132g5h_of_match[] = {
    {.compatible = "infineon,xdpe132g5h"},
    {}
};
MODULE_DEVICE_TABLE(of, xdpe132g5h_of_match);

static struct i2c_driver xdpe132g5h_driver = {
    .driver = {
        .name = "xdpe132",
        .of_match_table = of_match_ptr(xdpe132g5h_of_match),
    },
    .probe = xdpe132g5h_probe,
    .id_table = xdpe132g5h_id,
};
module_i2c_driver(xdpe132g5h_driver);

MODULE_AUTHOR("daiaq@clounix.com");
MODULE_DESCRIPTION("PMBus driver for XDPE132G5H device");
MODULE_LICENSE("GPL");
