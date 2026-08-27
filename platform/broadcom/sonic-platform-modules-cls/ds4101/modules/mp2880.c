// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for MPS MP2880 device
 */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include "pmbus.h"

static int mp2880_read_word_data(struct i2c_client *client, int page, int phase, int reg);

static struct pmbus_driver_info mp2880_info = {
	.pages = 1,

	.format[PSC_VOLTAGE_OUT] = linear,
	.format[PSC_VOLTAGE_IN] = linear,
	.format[PSC_CURRENT_OUT] = direct,
	.format[PSC_POWER] = direct,
	.format[PSC_TEMPERATURE] = direct,

	.m[PSC_VOLTAGE_OUT] = 512,
	.b[PSC_VOLTAGE_OUT] = 0,
	.R[PSC_VOLTAGE_OUT] = 0,
	.m[PSC_CURRENT_OUT] = 2,
	.b[PSC_CURRENT_OUT] = 0,
	.R[PSC_CURRENT_OUT] = 0,
	.m[PSC_POWER] = 4,
	.b[PSC_POWER] = 0,
	.R[PSC_POWER] = 0,
	.m[PSC_TEMPERATURE] = 1,
	.b[PSC_TEMPERATURE] = 0,
	.R[PSC_TEMPERATURE] = 0,

	.func[0] =
		PMBUS_HAVE_VOUT | PMBUS_HAVE_POUT |
	PMBUS_HAVE_TEMP | PMBUS_HAVE_IOUT,
	.read_word_data = &mp2880_read_word_data,
};

static int mp2880_read_word_data(struct i2c_client *client, int page, int phase, int reg)
{
	static char flag;
	int ret = 0;
	/*
	 * PSC_VOLTAGE_OUT format should be direct, but vout mode register is 0x00(linear).
	 * to avoid probe conflict in pmbus_probe, add this adaption here.
	 */
	if (2 != flag) {
		if (0 == flag)
			flag++;
		else if (1 == flag) {
			/* After pmbus_identify_common function, set the format as direct*/
			mp2880_info.format[PSC_VOLTAGE_OUT] = direct;
			flag++;
		}
	}

	switch (reg) {
	case PMBUS_READ_IOUT:
		ret = pmbus_read_word_data(client, page, phase, reg);
		if (ret < 0)
			return ret;
		else
			return ret & 0x7ff;
	default:
		return pmbus_read_word_data(client, page, phase, reg);
	}
}

static int mp2880_probe(struct i2c_client *client)
{
	// return pmbus_do_probe(client, id, &mp2880_info);
	return pmbus_do_probe(client, &mp2880_info);
}

static const struct i2c_device_id mp2880_id[] = {
	{"mp2880", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mp2880_id);

static const struct of_device_id __maybe_unused mp2880_of_match[] = {
	{ .compatible = "mps,mp2880", },
	{}
};

MODULE_DEVICE_TABLE(of, mp2880_of_match);

static struct i2c_driver mp2880_driver = {
	.driver = {
		   .name = "mp2880",
		   .of_match_table = of_match_ptr(mp2880_of_match),
	},
	.probe = mp2880_probe,
	//.remove = pmbus_do_remove,
	.id_table = mp2880_id,
};

module_i2c_driver(mp2880_driver);

MODULE_AUTHOR("Nicholas Wu <nicwu@celestica.com>");
MODULE_DESCRIPTION("PMBus driver for MPS MP2880");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(PMBUS);
