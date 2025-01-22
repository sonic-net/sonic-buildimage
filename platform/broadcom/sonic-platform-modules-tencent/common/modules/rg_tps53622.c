/*
 * Hardware monitoring driver for Texas Instruments TPS53622
 *
 * Copyright (c) 2017 Ruijie Networks. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "rg_pmbus.h"

#define TPS53622_PROT_VR12_5MV		0x01 /* VR12.0 mode, 5-mV DAC */
#define TPS53622_PROT_VR12_5_10MV	0x02 /* VR12.5 mode, 10-mV DAC */
#define TPS53622_PROT_VR13_10MV		0x04 /* VR13.0 mode, 10-mV DAC */
#define TPS53622_PROT_IMVP8_5MV		0x05 /* IMVP8 mode, 5-mV DAC */
#define TPS53622_PROT_VR13_5MV		0x07 /* VR13.0 mode, 5-mV DAC */
#define TPS53622_PAGE_NUM		2

static int tps53622_identify(struct i2c_client *client,
			     struct pmbus_driver_info *info)
{
	u8 vout_params;
	int ret, i;

	/* Read the register with VOUT scaling value.*/
	for (i = 0; i < TPS53622_PAGE_NUM; i++) {
		ret = rg_pmbus_read_byte_data(client, i, PMBUS_VOUT_MODE);
		if (ret < 0)
			return ret;

		vout_params = ret & GENMASK(4, 0);

		switch (vout_params) {
		case TPS53622_PROT_VR13_10MV:
		case TPS53622_PROT_VR12_5_10MV:
			info->vrm_version[i] = vr13;
			break;
		case TPS53622_PROT_VR13_5MV:
		case TPS53622_PROT_VR12_5MV:
		case TPS53622_PROT_IMVP8_5MV:
			info->vrm_version[i] = vr12;
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;

}

static struct pmbus_driver_info tps53622_info = {
	.pages = TPS53622_PAGE_NUM,
	.format[PSC_VOLTAGE_IN] = linear,
	.format[PSC_VOLTAGE_OUT] = vid,
	.format[PSC_TEMPERATURE] = linear,
	.format[PSC_CURRENT_OUT] = linear,
	.format[PSC_POWER] = linear,
	.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
		PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
		PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP |
		PMBUS_HAVE_POUT,
	.func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
		PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
		PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP |
		PMBUS_HAVE_POUT,
	.identify = tps53622_identify,
};

static int tps53622_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct pmbus_driver_info *info;

	info = devm_kmemdup(&client->dev, &tps53622_info, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	return rg_pmbus_do_probe(client, id, info);
}

static const struct i2c_device_id tps53622_id[] = {
	{"rg_tps53622", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, tps53622_id);

static const struct of_device_id tps53622_of_match[] = {
	{.compatible = "ruijie,rg_tps53622"},
	{}
};
MODULE_DEVICE_TABLE(of, tps53622_of_match);

static struct i2c_driver tps53622_driver = {
	.driver = {
		.name = "rg_tps53622",
		.of_match_table = of_match_ptr(tps53622_of_match),
	},
	.probe = tps53622_probe,
	.remove = rg_pmbus_do_remove,
	.id_table = tps53622_id,
};

module_i2c_driver(tps53622_driver);

MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_DESCRIPTION("PMBus driver for Texas Instruments TPS53622");
MODULE_LICENSE("GPL");
