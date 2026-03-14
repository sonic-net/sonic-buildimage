/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>


/* Size of EEPROM in bytes */
#define EEPROM_SIZE		256

#define SLICE_BITS		(6)
#define SLICE_SIZE		(1 << SLICE_BITS)
#define SLICE_NUM		(EEPROM_SIZE/SLICE_SIZE)

/* Each client has this additional data */
struct eeprom_data {
	struct mutex update_lock;
	u8 valid;			/* bitfield, bit!=0 if slice is valid */
	unsigned long last_updated[SLICE_NUM];	/* In jiffies, 8 slices */
	u8 data[EEPROM_SIZE];		/* Register values */
};


static void inv_eeprom_update_client(struct i2c_client *client, u8 slice)
{
	struct eeprom_data *data = i2c_get_clientdata(client);
	int i, j;
	int ret;
	int addr;
	

	mutex_lock(&data->update_lock);

	if (!(data->valid & (1 << slice)) ||
	    time_after(jiffies, data->last_updated[slice] + 300 * HZ)) {
		dev_dbg(&client->dev, "Starting eeprom update, slice %u\n", slice);

		addr = slice << SLICE_BITS;

		ret = i2c_smbus_write_byte_data(client, ((u8)addr >> 8) & 0xFF, (u8)addr & 0xFF);
		/* select the eeprom address */
		if (ret < 0) {
			dev_err(&client->dev, "address set failed\n");
			goto exit;
		}

		if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_BYTE)) {
			goto exit;
		}

		for (i = slice << SLICE_BITS; i < (slice + 1) << SLICE_BITS; i+= SLICE_SIZE) {
			for (j = i; j < (i+SLICE_SIZE); j++) {
				int res;

				res = i2c_smbus_read_byte(client);
				if (res < 0) {
					goto exit;
				}

				data->data[j] = res & 0xFF;
			}
		}
		
		data->last_updated[slice] = jiffies;
		data->valid |= (1 << slice);
	}

exit:
	mutex_unlock(&data->update_lock);
}

static ssize_t inv_eeprom_read(struct file *filp, struct kobject *kobj,
			   struct bin_attribute *bin_attr,
			   char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
	struct eeprom_data *data = i2c_get_clientdata(client);
	u8 slice;
	

	if (off > EEPROM_SIZE) {
		return 0;
	}
	if (off + count > EEPROM_SIZE) {
		count = EEPROM_SIZE - off;
	}
	if (count == 0) {
		return 0;
	}

	/* Only refresh slices which contain requested bytes */
	for (slice = off >> SLICE_BITS; slice <= (off + count - 1) >> SLICE_BITS; slice++) {
		inv_eeprom_update_client(client, slice);
	}

	memcpy(buf, &data->data[off], count);

	return count;
}

static struct bin_attribute inv_eeprom_attr = {
	.attr = {
		.name = "eeprom",
		.mode = S_IRUGO,
	},
	.size = EEPROM_SIZE,
	.read = inv_eeprom_read,
};

static int inv_eeprom_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct eeprom_data *data;
	int err;
	
	if (!(data = kzalloc(sizeof(struct eeprom_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	memset(data->data, 0xff, EEPROM_SIZE);
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	/* create the sysfs eeprom file */
	err = sysfs_create_bin_file(&client->dev.kobj, &inv_eeprom_attr);
	if (err) {
		goto exit_kfree;
	}
	
	return 0;

exit_kfree:
	kfree(data);
exit:
	return err;
}

static int inv_eeprom_remove(struct i2c_client *client)
{
	sysfs_remove_bin_file(&client->dev.kobj, &inv_eeprom_attr);
	kfree(i2c_get_clientdata(client));

	return 0;
}

static const struct i2c_device_id inv_eeprom_id[] = {
	{ "inv_eeprom", 0 },
	{ }
};

static struct i2c_driver inv_eeprom_driver = {
	.driver = {
		.name	= "inv_eeprom",
	},
	.probe		= inv_eeprom_probe,
	.remove		= inv_eeprom_remove,
	.id_table	= inv_eeprom_id,
};

module_i2c_driver(inv_eeprom_driver);

MODULE_AUTHOR("Inventec");
MODULE_DESCRIPTION("Inventec D7054 Mother Board EEPROM driver");
MODULE_LICENSE("GPL");


