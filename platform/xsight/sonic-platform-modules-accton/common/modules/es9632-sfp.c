/*
 * SFP driver for accton as7712_32x sfp
 *
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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
#include <linux/delay.h>

#define DRIVER_NAME	"es9632_sfp"

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)                                        \
		printk (KERN_INFO "%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#define NUM_OF_SFP_PORT 		32
#define EEPROM_NAME 			"eeprom"
#define EEPROM_SIZE				256	/*  256 byte eeprom */
#define BIT_INDEX(i) 			(1ULL << (i))
#define USE_I2C_BLOCK_READ 		0
#define I2C_RW_RETRY_COUNT		3
#define I2C_RW_RETRY_INTERVAL	100 /* ms */

#define SFP_EEPROM_A0_I2C_ADDR (0xA0 >> 1)

static ssize_t sfp_eeprom_read(struct i2c_client *, u8, u8 *,int);
static ssize_t sfp_eeprom_write(struct i2c_client *, u8 , const char *,int);

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static const struct i2c_device_id osfp_device_id[] = {
{ "osfp",  0 }, { /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, osfp_device_id);

enum driver_type_e {
	DRIVER_TYPE_OSFP
};

/* Each client has this additional data
 */
struct eeprom_data {
	char				 valid;			/* !=0 if registers are valid */
	unsigned long		 last_updated;	/* In jiffies */
	struct bin_attribute bin;			/* eeprom data */
};

struct qsfp_data {
	char			valid;		  	/* !=0 if registers are valid */
	unsigned long	last_updated;   /* In jiffies */
	u8				status[3];	  	/* bit0:port0, bit1:port1 and so on */
									/* index 0 => tx_fail
											 1 => tx_disable
											 2 => rx_loss */

	u8					device_id;
	struct eeprom_data	eeprom;
};

struct sfp_port_data {
	struct mutex		   update_lock;
	enum driver_type_e     driver_type;
	int 				   port;		/* CPLD port index */

	struct qsfp_data 	  *qsfp;
	struct i2c_client 	  *client;
};

static ssize_t sfp_eeprom_write(struct i2c_client *client, u8 command, const char *data,
			  int data_len)
{
#if USE_I2C_BLOCK_READ
	int status, retry = I2C_RW_RETRY_COUNT;

	if (data_len > I2C_SMBUS_BLOCK_MAX) {
		data_len = I2C_SMBUS_BLOCK_MAX;
	}

	while (retry) {
		status = i2c_smbus_write_i2c_block_data(client, command, data_len, data);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	if (unlikely(status < 0)) {
		return status;
	}

	return data_len;
#else
	int status, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_write_byte_data(client, command, *data);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	if (unlikely(status < 0)) {
		return status;
	}

	return 1;
#endif
}

static ssize_t sfp_port_write(struct sfp_port_data *data,
						  const char *buf, loff_t off, size_t count)
{
	ssize_t retval = 0;

	if (unlikely(!count)) {
		return count;
	}

	/*
	 * Write data to chip, protecting against concurrent updates
	 * from this host, but not from other I2C masters.
	 */
	mutex_lock(&data->update_lock);

	while (count) {
		ssize_t status;

		status = sfp_eeprom_write(data->client, off, buf, count);
		if (status <= 0) {
			if (retval == 0) {
				retval = status;
			}
			break;
		}
		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	mutex_unlock(&data->update_lock);
	return retval;
}


static ssize_t sfp_bin_write(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr,
				char *buf, loff_t off, size_t count)
{
	struct sfp_port_data *data;
	DEBUG_PRINT("%s(%d) offset = (%d), count = (%d)", off, count);
	data = dev_get_drvdata(container_of(kobj, struct device, kobj));
	return sfp_port_write(data, buf, off, count);
}

static ssize_t sfp_eeprom_read(struct i2c_client *client, u8 command, u8 *data,
			  int data_len)
{
#if USE_I2C_BLOCK_READ
	int status, retry = I2C_RW_RETRY_COUNT;

	if (data_len > I2C_SMBUS_BLOCK_MAX) {
		data_len = I2C_SMBUS_BLOCK_MAX;
	}

	while (retry) {
		status = i2c_smbus_read_i2c_block_data(client, command, data_len, data);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	if (unlikely(status < 0)) {
		goto abort;
	}
	if (unlikely(status != data_len)) {
		status = -EIO;
		goto abort;
	}

	//result = data_len;

abort:
	return status;
#else
	int status, retry = I2C_RW_RETRY_COUNT;

	while (retry) {
		status = i2c_smbus_read_byte_data(client, command);
		if (unlikely(status < 0)) {
			msleep(I2C_RW_RETRY_INTERVAL);
			retry--;
			continue;
		}

		break;
	}

	if (unlikely(status < 0)) {
		dev_dbg(&client->dev, "sfp read byte data failed, command(0x%2x), data(0x%2x)\r\n", command, status);
		goto abort;
	}

	*data  = (u8)status;
	status = 1;

abort:
	return status;
#endif
}

static ssize_t sfp_port_read(struct sfp_port_data *data,
				char *buf, loff_t off, size_t count)
{
	ssize_t retval = 0;

	if (unlikely(!count)) {
		DEBUG_PRINT("Count = 0, return");
		return count;
	}

	/*
	 * Read data from chip, protecting against concurrent updates
	 * from this host, but not from other I2C masters.
	 */
	mutex_lock(&data->update_lock);

	while (count) {
		ssize_t status;

		status = sfp_eeprom_read(data->client, off, buf, count);
		if (status <= 0) {
			if (retval == 0) {
				retval = status;
			}
			break;
		}

		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	mutex_unlock(&data->update_lock);
	return retval;

}

static ssize_t sfp_bin_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	struct sfp_port_data *data;
	DEBUG_PRINT("offset = (%d), count = (%d)", off, count);
	data = dev_get_drvdata(container_of(kobj, struct device, kobj));
	return sfp_port_read(data, buf, off, count);
}

static int sfp_sysfs_eeprom_init(struct kobject *kobj, struct bin_attribute *eeprom)
{
	int err;

	sysfs_bin_attr_init(eeprom);
	eeprom->attr.name = EEPROM_NAME;
	eeprom->attr.mode = S_IWUSR | S_IRUGO;
	eeprom->read  	  = sfp_bin_read;
	eeprom->write 	  = sfp_bin_write;
	eeprom->size  	  = EEPROM_SIZE;

	/* Create eeprom file */
	err = sysfs_create_bin_file(kobj, eeprom);
	if (err) {
		return err;
	}

	return 0;
}

static int sfp_sysfs_eeprom_cleanup(struct kobject *kobj, struct bin_attribute *eeprom)
{
	sysfs_remove_bin_file(kobj, eeprom);
	return 0;
}

static int osfp_i2c_check_functionality(struct i2c_client *client)
{
#if USE_I2C_BLOCK_READ
    return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK);
#else
    return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA);
#endif
}

static int qsfp_probe(struct i2c_client *client, const struct i2c_device_id *dev_id,
						  struct qsfp_data **data)
{
	int status;
	struct qsfp_data *qsfp;

	if (!osfp_i2c_check_functionality(client)) {
        status = -EIO;
        goto exit;
	}

	qsfp = kzalloc(sizeof(struct qsfp_data), GFP_KERNEL);
	if (!qsfp) {
		status = -ENOMEM;
		goto exit;
	}

	/* init eeprom */
	status = sfp_sysfs_eeprom_init(&client->dev.kobj, &qsfp->eeprom.bin);
	if (status) {
		goto exit_free;
	}

	*data = qsfp;
	dev_info(&client->dev, "qsfp '%s'\n", client->name);

	return 0;

exit_free:
	kfree(qsfp);
exit:

	return status;
}

static int sfp_device_probe(struct i2c_client *client,
			const struct i2c_device_id *dev_id)
{
	struct sfp_port_data *data = NULL;

	data = kzalloc(sizeof(struct sfp_port_data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->port 	 = dev_id->driver_data;
	data->client = client;

	if (client->addr != SFP_EEPROM_A0_I2C_ADDR) {
		return -ENODEV;
	}

	data->driver_type = DRIVER_TYPE_OSFP;
	return qsfp_probe(client, dev_id, &data->qsfp);
}

static int qfp_remove(struct i2c_client *client, struct qsfp_data *data)
{
	sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &data->eeprom.bin);
	kfree(data);
	return 0;
}

static int sfp_device_remove(struct i2c_client *client)
{
	struct sfp_port_data *data = i2c_get_clientdata(client);

	switch (data->driver_type) {
		case DRIVER_TYPE_OSFP:
			return qfp_remove(client, data->qsfp);
   	}

	return 0;
}

static struct i2c_driver sfp_driver = {
    .driver = {
        .name     = DRIVER_NAME,
    },
    .probe        = sfp_device_probe,
    .remove       = sfp_device_remove,
    .id_table     = osfp_device_id,
    .address_list = normal_i2c,
};

static int __init sfp_init(void)
{
	return i2c_add_driver(&sfp_driver);
}

static void __exit sfp_exit(void)
{
	i2c_del_driver(&sfp_driver);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton as7712_32x_sfp driver");
MODULE_LICENSE("GPL");

module_init(sfp_init);
module_exit(sfp_exit);
