/*
 * Hardware monitoring driver for Texas Instruments TPS536C7
 *
 * Copyright (c) 2017 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2017 Vadim Pasternak <vadimp@mellanox.com>
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
#include "pmbus.h"


#define TPS536C7_PROT_VR12_5MV		0x01 /* VR12.0 mode, 5-mV DAC */
#define TPS536C7_PROT_VR12_5_10MV	0x02 /* VR12.5 mode, 10-mV DAC */
#define TPS536C7_PROT_VR13_10MV		0x04 /* VR13.0 mode, 10-mV DAC */
#define TPS536C7_PROT_IMVP8_5MV		0x05 /* IMVP8 mode, 5-mV DAC */
#define TPS536C7_PROT_VR13_5MV		0x07 /* VR13.0 mode, 5-mV DAC */
#define TPS536C7_PAGE_NUM		1

static int tps536c7_identify(struct i2c_client *client,
			     struct pmbus_driver_info *info)
{
    u8 buf[I2C_SMBUS_BLOCK_MAX];
    int phases_a = 0, phases_b = 0;
    int i, ret;
	

    return 1;
	
    ret = i2c_smbus_read_block_data(client, PMBUS_IC_DEVICE_ID, buf);
    if (ret < 0)
        return ret;
	
	dev_err(&client->dev, "jjj device ID: %s\n", buf);
    if (strncmp("TI\x53\x6C\x70", buf, 5)) {
        dev_err(&client->dev, "Unexpected device ID: %s\n", buf);
        return -ENODEV;
    }
    #if 0
    ret = i2c_smbus_read_block_data(client, TPS53676_USER_DATA_03, buf);
    if (ret < 0)
        return ret;
    for (i = 0; i < 2 * TPS53676_MAX_PHASES; i += 2) {
        if (buf[i + 1] & 0x80) {
            if (buf[i] & 0x08)
                phases_b++;
            else
                phases_a++;
            }
    }

    info->format[PSC_VOLTAGE_OUT] = linear;
    info->pages = 1;
    info->phases[0] = phases_a;
    if (phases_b > 0) {
        info->pages = 2;
        info->phases[1] = phases_b;
    }
	#endif
	
    return 0;
}

static struct pmbus_driver_info tps536c7_info = {
	.pages = TPS536C7_PAGE_NUM,
	.format[PSC_VOLTAGE_IN] = linear,
	.format[PSC_VOLTAGE_OUT] = linear,
	.format[PSC_TEMPERATURE] = linear,
	.format[PSC_CURRENT_OUT] = linear,
	.format[PSC_POWER] = linear,
	.func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
		PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
		PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP |
		PMBUS_HAVE_POUT,
	.identify = tps536c7_identify,
};

static int tps536c7_probe(struct i2c_client *client)
{
	struct pmbus_driver_info *info;

	info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	memcpy(info, &tps536c7_info, sizeof(*info));

	//return pmbus_do_probe(client, id, info);
          return pmbus_do_probe(client, info);
}

static const struct i2c_device_id tps536c7_id[] = {
	{"tps536c7", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, tps536c7_id);

static const struct of_device_id tps536c7_of_match[] = {
	{.compatible = "ti,tps536c7"},
	{}
};
MODULE_DEVICE_TABLE(of, tps536c7_of_match);

static struct i2c_driver tps536c7_driver = {
	.driver = {
		.name = "tps536c7",
		.of_match_table = of_match_ptr(tps536c7_of_match),
	},
	.probe = tps536c7_probe,
	//.remove = pmbus_do_remove,
	.id_table = tps536c7_id,
};

module_i2c_driver(tps536c7_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@mellanox.com>");
MODULE_DESCRIPTION("PMBus driver for Texas Instruments TPS536C7");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(PMBUS);
