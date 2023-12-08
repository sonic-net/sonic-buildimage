/*
 * An hwmon driver for accton as7716_32x sfp
 *
 * Copyright (C) 2014 Accton Technology Corporation.
 * Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define BIT_INDEX(i) (1UL << (i))
#define I2C_ADDR_CPLD1	0x60
#define I2C_ADDR_CPLD2	0x62
#define I2C_ADDR_CPLD3	0x64
#define CPLD1_OFFSET_QSFP_PRESET1 0x30
#define CPLD1_OFFSET_QSFP_PRESET2 0x31
#define CPLD1_OFFSET_QSFP_PRESET3 0x32
#define CPLD1_OFFSET_QSFP_PRESET4 0x33

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { 0x50, I2C_CLIENT_END };

/* Each client has this additional data
 */
struct as7716_32x_sfp_data {
	struct device	   *hwmon_dev;
	struct mutex		update_lock;
	char				valid;			 /* !=0 if registers are valid */
	unsigned long		last_updated;	 /* In jiffies */
	int					port;			 /* Front port index */
	char				eeprom[256];	 /* eeprom data */
	u32					is_present;		 /* present status */
};

static struct as7716_32x_sfp_data *as7716_32x_sfp_update_device(struct device *dev);
static ssize_t show_port_number(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_present(struct device *dev, struct device_attribute *da,char *buf);
static ssize_t show_eeprom(struct device *dev, struct device_attribute *da, char *buf);
extern int accton_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int accton_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

enum as7716_32x_sfp_sysfs_attributes {
	SFP_PORT_NUMBER,
	SFP_IS_PRESENT,
	SFP_IS_PRESENT_ALL,
	SFP_EEPROM
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(sfp_port_number,    S_IRUGO, show_port_number, NULL, SFP_PORT_NUMBER);
static SENSOR_DEVICE_ATTR(sfp_is_present,     S_IRUGO, show_present,     NULL, SFP_IS_PRESENT);
static SENSOR_DEVICE_ATTR(sfp_is_present_all, S_IRUGO, show_present,     NULL, SFP_IS_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(sfp_eeprom,	      S_IRUGO, show_eeprom,      NULL, SFP_EEPROM);

static struct attribute *as7716_32x_sfp_attributes[] = {
	&sensor_dev_attr_sfp_port_number.dev_attr.attr,
	&sensor_dev_attr_sfp_is_present.dev_attr.attr,
	&sensor_dev_attr_sfp_is_present_all.dev_attr.attr,
	&sensor_dev_attr_sfp_eeprom.dev_attr.attr,
	NULL
};

static ssize_t show_port_number(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct as7716_32x_sfp_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->port+1);
}

/* Error-check the CPLD read results. */
#define VALIDATED_READ(_buf, _rv, _read_expr, _invert)  \
do {                                                \
    _rv = (_read_expr);                             \
    if(_rv < 0) {                                   \
        return sprintf(_buf, "READ ERROR\n");       \
    }                                               \
    if(_invert) {                                   \
        _rv = ~_rv;                                 \
    }                                               \
    _rv &= 0xFF;                                    \
} while(0)

static ssize_t show_present(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	if(attr->index == SFP_IS_PRESENT_ALL) {
		int values[4];
		/*
		 * Report the SFP_PRESENCE status for all ports.
		 */

		/* QSFP_PRESENT Ports 1-8 */
		//VALIDATED_READ(buf, values[0], accton_i2c_cpld_read(0x62, 0x9), 1);
		VALIDATED_READ(buf, values[0], accton_i2c_cpld_read(I2C_ADDR_CPLD1, CPLD1_OFFSET_QSFP_PRESET1), 1);
		/* QSFP_PRESENT Ports 9-16 */
		VALIDATED_READ(buf, values[1], accton_i2c_cpld_read(I2C_ADDR_CPLD1, CPLD1_OFFSET_QSFP_PRESET2), 1);
		/* QSFP_PRESENT Ports 17-24 */
		VALIDATED_READ(buf, values[2], accton_i2c_cpld_read(I2C_ADDR_CPLD1, CPLD1_OFFSET_QSFP_PRESET3), 1);
		/* QSFP_PRESENT Ports 25-32 */
		VALIDATED_READ(buf, values[3], accton_i2c_cpld_read(I2C_ADDR_CPLD1, CPLD1_OFFSET_QSFP_PRESET4), 1);
		
		/* Return values 1 -> 32 in order */
		return sprintf(buf, "%.2x %.2x %.2x %.2x\n",
					   values[0], values[1], values[2], values[3]);
	}
	else { /* SFP_IS_PRESENT */
		struct as7716_32x_sfp_data *data = as7716_32x_sfp_update_device(dev);

		if (!data->valid) {
		    printk("return -EIO\n");
			return -EIO;
		}
	
		return sprintf(buf, "%d\n", data->is_present);
	}
}

static ssize_t show_eeprom(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct as7716_32x_sfp_data *data = as7716_32x_sfp_update_device(dev);

	if (!data->valid) {
		return 0;
	}

	if (!data->is_present) {
		return 0;
	}

	memcpy(buf, data->eeprom, sizeof(data->eeprom));

	return sizeof(data->eeprom);
}

static const struct attribute_group as7716_32x_sfp_group = {
	.attrs = as7716_32x_sfp_attributes,
};

static int as7716_32x_sfp_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	struct as7716_32x_sfp_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
		status = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct as7716_32x_sfp_data), GFP_KERNEL);
	if (!data) {
		status = -ENOMEM;
		goto exit;
	}

	mutex_init(&data->update_lock);
	data->port = dev_id->driver_data;
	i2c_set_clientdata(client, data);

	dev_info(&client->dev, "chip found\n");

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &as7716_32x_sfp_group);
	if (status) {
		goto exit_free;
	}

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	dev_info(&client->dev, "%s: sfp '%s'\n",
		 dev_name(data->hwmon_dev), client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &as7716_32x_sfp_group);
exit_free:
	kfree(data);
exit:

	return status;
}

static void as7716_32x_sfp_remove(struct i2c_client *client)
{
	struct as7716_32x_sfp_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &as7716_32x_sfp_group);
	kfree(data);

}

enum port_numbers {
as7716_32x_sfp1, as7716_32x_sfp2, as7716_32x_sfp3, as7716_32x_sfp4,
as7716_32x_sfp5, as7716_32x_sfp6, as7716_32x_sfp7, as7716_32x_sfp8,
as7716_32x_sfp9, as7716_32x_sfp10,as7716_32x_sfp11,as7716_32x_sfp12,
as7716_32x_sfp13,as7716_32x_sfp14,as7716_32x_sfp15,as7716_32x_sfp16,
as7716_32x_sfp17,as7716_32x_sfp18,as7716_32x_sfp19,as7716_32x_sfp20,
as7716_32x_sfp21,as7716_32x_sfp22,as7716_32x_sfp23,as7716_32x_sfp24,
as7716_32x_sfp25,as7716_32x_sfp26,as7716_32x_sfp27,as7716_32x_sfp28,
as7716_32x_sfp29,as7716_32x_sfp30,as7716_32x_sfp31,as7716_32x_sfp32
};

static const struct i2c_device_id as7716_32x_sfp_id[] = {
{ "as7716_32x_sfp1",  as7716_32x_sfp1 },  { "as7716_32x_sfp2",	as7716_32x_sfp2 },
{ "as7716_32x_sfp3",  as7716_32x_sfp3 },  { "as7716_32x_sfp4",	as7716_32x_sfp4 },
{ "as7716_32x_sfp5",  as7716_32x_sfp5 },  { "as7716_32x_sfp6",	as7716_32x_sfp6 },
{ "as7716_32x_sfp7",  as7716_32x_sfp7 },  { "as7716_32x_sfp8",	as7716_32x_sfp8 },
{ "as7716_32x_sfp9",  as7716_32x_sfp9 },  { "as7716_32x_sfp10", as7716_32x_sfp10 },
{ "as7716_32x_sfp11", as7716_32x_sfp11 }, { "as7716_32x_sfp12", as7716_32x_sfp12 },
{ "as7716_32x_sfp13", as7716_32x_sfp13 }, { "as7716_32x_sfp14", as7716_32x_sfp14 },
{ "as7716_32x_sfp15", as7716_32x_sfp15 }, { "as7716_32x_sfp16", as7716_32x_sfp16 },
{ "as7716_32x_sfp17", as7716_32x_sfp17 }, { "as7716_32x_sfp18", as7716_32x_sfp18 },
{ "as7716_32x_sfp19", as7716_32x_sfp19 }, { "as7716_32x_sfp20", as7716_32x_sfp20 },
{ "as7716_32x_sfp21", as7716_32x_sfp21 }, { "as7716_32x_sfp22", as7716_32x_sfp22 },
{ "as7716_32x_sfp23", as7716_32x_sfp23 }, { "as7716_32x_sfp24", as7716_32x_sfp24 },
{ "as7716_32x_sfp25", as7716_32x_sfp25 }, { "as7716_32x_sfp26", as7716_32x_sfp26 },
{ "as7716_32x_sfp27", as7716_32x_sfp27 }, { "as7716_32x_sfp28", as7716_32x_sfp28 },
{ "as7716_32x_sfp29", as7716_32x_sfp29 }, { "as7716_32x_sfp30", as7716_32x_sfp30 },
{ "as7716_32x_sfp31", as7716_32x_sfp31 }, { "as7716_32x_sfp32", as7716_32x_sfp32 },
{}
};
MODULE_DEVICE_TABLE(i2c, as7716_32x_sfp_id);

static struct i2c_driver as7716_32x_sfp_driver = {
	.class		  = I2C_CLASS_HWMON,
	.driver = {
		.name	  = "as7716_32x_sfp",
	},
	.probe		  = as7716_32x_sfp_probe,
	.remove		  = as7716_32x_sfp_remove,
	.id_table	  = as7716_32x_sfp_id,
	.address_list = normal_i2c,
};

static int as7716_32x_sfp_read_block(struct i2c_client *client, u8 command, u8 *data,
			  int data_len)
{
	int result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);

	if (unlikely(result < 0))
		goto abort;
	if (unlikely(result != data_len)) {
		result = -EIO;
		goto abort;
	}

	result = 0;

abort:
	return result;
}

static struct as7716_32x_sfp_data *as7716_32x_sfp_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct as7716_32x_sfp_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
		|| !data->valid) {
		int status = -1;
		int i = 0;
        u8 cpld_reg = 0x30 + (data->port/8);

		data->valid = 0;

		/* Read present status of the specified port number */
		data->is_present = 0;
		status = accton_i2c_cpld_read(I2C_ADDR_CPLD1, cpld_reg);

		if (status < 0) {
			dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD1, cpld_reg, status);
			goto exit;
		}

		data->is_present = (status & (1 << (data->port % 8))) ? 0 : 1;
        printk("data->is_present=%d, data->port=%d, status=0x%x\n",data->is_present, data->port, status);
		/* Read eeprom data based on port number */
		memset(data->eeprom, 0, sizeof(data->eeprom));

		/* Check if the port is present */
		if (data->is_present) {
			/* read eeprom */
			for (i = 0; i < sizeof(data->eeprom)/I2C_SMBUS_BLOCK_MAX; i++) {
				status = as7716_32x_sfp_read_block(client, i*I2C_SMBUS_BLOCK_MAX,
												   data->eeprom+(i*I2C_SMBUS_BLOCK_MAX),
												   I2C_SMBUS_BLOCK_MAX);
				if (status < 0) {
				    printk("unable to read eeprom from port(%d)\n", data->port);
					dev_dbg(&client->dev, "unable to read eeprom from port(%d)\n", data->port);
					goto exit;
				}
			}
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

exit:
	mutex_unlock(&data->update_lock);

	return data;
}

static int __init as7716_32x_sfp_init(void)
{
	//extern int platform_accton_as7716_32x(void);
	//if (!platform_accton_as7716_32x()) {
//		return -ENODEV;
	//}

	return i2c_add_driver(&as7716_32x_sfp_driver);
}

static void __exit as7716_32x_sfp_exit(void)
{
	i2c_del_driver(&as7716_32x_sfp_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton as7716_32x_sfp driver");
MODULE_LICENSE("GPL");

module_init(as7716_32x_sfp_init);
module_exit(as7716_32x_sfp_exit);
