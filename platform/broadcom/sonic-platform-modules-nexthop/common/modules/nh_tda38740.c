// SPDX-License-Identifier: GPL-2.0+
/**
 * Hardware monitoring driver for Infineon Integrated-pol-voltage-regulators or
 *   Digital-Multiphase-Controllers
 * Driver for TDA38725,TDA38725A,TDA38740,TDA38740A,XDPE1A2G5B,XDPE19284C,XDPE192C4B
 *
 * Adapted from Infineon, and modified to be compatible with linux kernel v6.1
 * as well as SONiC
 *
 * Copyright (c) 2024 Infineon Technologies
 * Copyright (c) 2025 Nexthop Systems Inc.
 */
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/mod_devicetable.h>
#include <linux/pmbus.h>
#include <linux/version.h>
#include "nh_pmbus.h"

MODULE_IMPORT_NS(PMBUS);

#define TDA38725_IC_DEVICE_ID "\x92"
#define TDA38725A_IC_DEVICE_ID "\xA9"
#define TDA38740_IC_DEVICE_ID "\x84"
#define TDA38740A_IC_DEVICE_ID "\xA8"
#define XDPE1A2G5B_IC_DEVICE_ID "\x01\x9E"
#define XDPE19284C_IC_DEVICE_ID "\x02\x98"
#define XDPE192C4B_IC_DEVICE_ID "\x01\x99"

enum chips {
	tda38725,
	tda38725a,
	tda38740,
	tda38740a,
	xdpe1a2g5b,
	xdpe19284c,
	xdpe192c4b
};

struct tda38740_data {
	enum chips id;
	struct pmbus_driver_info info;
	u32 vout_multiplier[2];
};

#define to_tda38740_data(x) container_of(x, struct tda38740_data, info)

static int tda38740_read_word_data(struct i2c_client *client, int page,
				   int phase, int reg)
{
	const struct pmbus_driver_info *info = nh_pmbus_get_driver_info(client);
	const struct tda38740_data *data = to_tda38740_data(info);
	int ret = 0;

	/* Virtual PMBUS Command not supported */
	if (reg >= PMBUS_VIRT_BASE) {
		ret = -ENXIO;
		return ret;
	}
	switch (reg) {
	case PMBUS_READ_VOUT:
		ret = nh_pmbus_read_word_data(client, page, phase, reg);
		ret = ((ret * data->vout_multiplier[0]) /
		       data->vout_multiplier[1]);
		break;
	default:
		ret = nh_pmbus_read_word_data(client, page, phase, reg);
		break;
	}
	return ret;
}

static struct pmbus_driver_info tda38740_info[] = {
	[tda38725] = {
		.pages = 1,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_POWER] = linear,
		.format[PSC_TEMPERATURE] = linear,

		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_STATUS_INPUT
			| PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP
			| PMBUS_HAVE_IIN
			| PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
			| PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
			| PMBUS_HAVE_POUT | PMBUS_HAVE_PIN,
#if IS_ENABLED(CONFIG_SENSORS_TDA38740_REGULATOR)
		.num_regulators = 1,
		.reg_desc = tda38740_reg_desc,
#endif
	},
	[tda38725a] = {
		.pages = 1,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_POWER] = linear,
		.format[PSC_TEMPERATURE] = linear,

		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_STATUS_INPUT
			| PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP
			| PMBUS_HAVE_IIN
			| PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
			| PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
			| PMBUS_HAVE_POUT | PMBUS_HAVE_PIN,
#if IS_ENABLED(CONFIG_SENSORS_TDA38740_REGULATOR)
		.num_regulators = 1,
		.reg_desc = tda38740_reg_desc,
#endif
	},
	[tda38740] = {
		.pages = 1,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_POWER] = linear,
		.format[PSC_TEMPERATURE] = linear,

		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_STATUS_INPUT
			| PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP
			| PMBUS_HAVE_IIN
			| PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
			| PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
			| PMBUS_HAVE_POUT | PMBUS_HAVE_PIN,
#if IS_ENABLED(CONFIG_SENSORS_TDA38740_REGULATOR)
		.num_regulators = 1,
		.reg_desc = tda38740_reg_desc,
#endif
	},
	[tda38740a] = {
		.pages = 1,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_POWER] = linear,
		.format[PSC_TEMPERATURE] = linear,

		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_STATUS_INPUT
			| PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP
			| PMBUS_HAVE_IIN
			| PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
			| PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
			| PMBUS_HAVE_POUT | PMBUS_HAVE_PIN,
#if IS_ENABLED(CONFIG_SENSORS_TDA38740_REGULATOR)
		.num_regulators = 1,
		.reg_desc = tda38740_reg_desc,
#endif
	},
	[xdpe1a2g5b] = {
		.pages = 2,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_TEMPERATURE] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_POWER] = linear,
		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
			PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
			PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
			PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
		.func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
			PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
			PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
			PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
	},
	[xdpe19284c] = {
		.pages = 2,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_TEMPERATURE] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_POWER] = linear,
		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
			PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
			PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
			PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
		.func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
			PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
			PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
			PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
	},
	[xdpe192c4b] = {
		.pages = 2,
		.read_word_data = tda38740_read_word_data,
		.format[PSC_VOLTAGE_IN] = linear,
		.format[PSC_VOLTAGE_OUT] = linear,
		.format[PSC_TEMPERATURE] = linear,
		.format[PSC_CURRENT_IN] = linear,
		.format[PSC_CURRENT_OUT] = linear,
		.format[PSC_POWER] = linear,
		.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
			PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
			PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
			PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
		.func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
			PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
			PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
			PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
	},
};

static const struct i2c_device_id tda38740_id[] = {
	{ "nh_tda38725", tda38725 },     { "nh_tda38725a", tda38725a },
	{ "nh_tda38740", tda38740 },     { "nh_tda38740a", tda38740a },
	{ "nh_xdpe1a2g5b", xdpe1a2g5b }, { "nh_xdpe19284c", xdpe19284c },
	{ "nh_xdpe192c4b", xdpe192c4b }, {}
};

static int tda38740_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	enum chips chip_id;
	struct tda38740_data *data;
	struct pmbus_driver_info *info;

	pr_info("Inside %s\n", __func__);

	if (dev_fwnode(dev))
		chip_id = (enum chips)device_get_match_data(dev);
	else
		chip_id = i2c_match_id(tda38740_id, client)->driver_data;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->id = chip_id;
	info = &data->info;

	// Copy the base configuration
	*info = tda38740_info[chip_id];
	info->read_word_data = tda38740_read_word_data;

	// Set default multiplier values
	data->vout_multiplier[0] = 1;
	data->vout_multiplier[1] = 1;

	return nh_pmbus_do_probe(client, info);
}

MODULE_DEVICE_TABLE(i2c, tda38740_id);

static const struct of_device_id __maybe_unused tda38740_of_match[] = {
	{ .compatible = "infineon,nh_tda38725", .data = (void *)tda38725 },
	{ .compatible = "infineon,nh_tda38725a", .data = (void *)tda38725a },
	{ .compatible = "infineon,nh_tda38740", .data = (void *)tda38740 },
	{ .compatible = "infineon,nh_tda38740a", .data = (void *)tda38740a },
	{ .compatible = "infineon,nh_xdpe1a2g5b", .data = (void *)xdpe1a2g5b },
	{ .compatible = "infineon,nh_xdpe19284c", .data = (void *)xdpe19284c },
	{ .compatible = "infineon,nh_xdpe192c4b", .data = (void *)xdpe192c4b },
	{}
};

MODULE_DEVICE_TABLE(of, tda38740_of_match);

/**
 *  This is the driver that will be inserted
 */
static struct i2c_driver tda38740_driver = {
	.driver = {
		.name = "nh_tda38740",
		.of_match_table = of_match_ptr(tda38740_of_match),
	},
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
	.probe_new = tda38740_probe,
#else
	.probe = tda38740_probe,
#endif
	.id_table = tda38740_id,
};

module_i2c_driver(tda38740_driver);

MODULE_AUTHOR("Ashish Yadav <Ashish.Yadav@infineon.com>");
MODULE_AUTHOR("Nexthop Systems Inc.");
MODULE_DESCRIPTION("PMBus driver for Infineon IPOL/DMC");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(PMBUS);
