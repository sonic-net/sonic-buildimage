/*
 * A SFP driver for the porsche platform
 *
 * Copyright (C) 2018 Pegatron Corporation.
 * Peter5_Lin <Peter5_Lin@pegatroncorp.com.tw>
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/i2c.h>

#undef PEGA_DEBUG
/*#define PEGA_DEBUG*/
#ifdef PEGA_DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif /* DEBUG */

#define SFP_EEPROM_SIZE		256
#define SFP_EEPROM_A0_ADDR 	0x50
#define SFP_EEPROM_A2_ADDR 	0x51
#define SFP_EEPROM_BUS_TYPE	I2C_SMBUS_I2C_BLOCK_DATA
#define CPLDA_SFP_NUM               24
#define CPLDB_SFP_NUM               12
#define CPLDC_SFP_NUM               18
#define CPLDA_ADDRESS               0x74
#define CPLDB_ADDRESS               0x75
#define CPLDC_ADDRESS               0x76
#define SFP_13_36_SCL_BASE          0x4
#define SFP_1_12_SCL_BASE           0x2
#define SFP_37_54_SCL_BASE          0x5
#define QSFP_I2C_ENABLE_BASE		0x17
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)

enum cpld_croups { cpld_group_a, cpld_group_b, cpld_group_c};

static const unsigned short normal_i2c[] = { SFP_EEPROM_A0_ADDR, SFP_EEPROM_A2_ADDR, I2C_CLIENT_END };
static char *SFP_CPLD_GROUPA_MAPPING[CPLDA_SFP_NUM][16]={0};
static char *SFP_CPLD_GROUPB_MAPPING[CPLDB_SFP_NUM][16]={0};
static char *SFP_CPLD_GROUPC_MAPPING[CPLDC_SFP_NUM][16]={0};

/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
static unsigned io_limit = 128;
module_param(io_limit, uint, 0);
MODULE_PARM_DESC(io_limit, "Maximum bytes per I/O (default 128)");

/*
 * Specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned write_timeout = 25;
module_param(write_timeout, uint, 0);
MODULE_PARM_DESC(write_timeout, "Time (in ms) to try writes (default 25)");


struct porsche_sfp_data {
	struct mutex lock;
	struct bin_attribute bin;
	int use_smbus;
	kernel_ulong_t driver_data;

	struct i2c_client *client;
};

extern int pegatron_porsche_cpld_read(unsigned short cpld_addr, u8 reg);
extern int pegatron_porsche_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

static ssize_t porsche_sfp_eeprom_read(struct porsche_sfp_data *data, char *buf,
		unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	struct i2c_client *client = data->client;
	unsigned long timeout, read_time;
	int status;

	memset(msg, 0, sizeof(msg));

	if (count > io_limit)
		count = io_limit;

		/* Smaller eeproms can work given some SMBus extension calls */
		if (count > I2C_SMBUS_BLOCK_MAX)
			count = I2C_SMBUS_BLOCK_MAX;

	/*
	 * Reads fail if the previous write didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		read_time = jiffies;
		switch (data->use_smbus) {
		case I2C_SMBUS_I2C_BLOCK_DATA:
			status = i2c_smbus_read_i2c_block_data(client, offset,
					count, buf);
			break;
		case I2C_SMBUS_WORD_DATA:
			status = i2c_smbus_read_word_data(client, offset);
			if (status >= 0) {
				buf[0] = status & 0xff;
				if (count == 2)
					buf[1] = status >> 8;
				status = count;
			}
			break;
		case I2C_SMBUS_BYTE_DATA:
			status = i2c_smbus_read_byte_data(client, offset);
			if (status >= 0) {
				buf[0] = status;
				status = count;
			}
			break;
		default:
			status = i2c_transfer(client->adapter, msg, 2);
			if (status == 2)
				status = count;
		}
		dev_dbg(&client->dev, "read %zu@%d --> %d (%ld)\n",
				count, offset, status, jiffies);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(read_time, timeout));

	return -ETIMEDOUT;
}

static ssize_t porsche_sfp_read(struct porsche_sfp_data *data,
		char *buf, loff_t off, size_t count)
{
	ssize_t retval = 0;

	if (unlikely(!count))
		return count;

	/*
	 * Read data from chip, protecting against concurrent updates
	 * from this host, but not from other I2C masters.
	 */
	mutex_lock(&data->lock);

	while (count) {
		ssize_t	status;

		status = porsche_sfp_eeprom_read(data, buf, off, count);
		if (status <= 0) {
			if (retval == 0)
				retval = status;
			break;
		}
		buf += status;
		off += status;
		count -= status;
		retval += status;
	}

	mutex_unlock(&data->lock);

	return retval;
}

static ssize_t 
porsche_sfp_bin_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	int i;
	u8 cpldData = 0;
	struct porsche_sfp_data *data;

	/*SFP 1-12*/
	for(i=0; i<CPLDB_SFP_NUM; i++)
	{
		if(!strcmp(attr->attr.name, SFP_CPLD_GROUPB_MAPPING[i]))
		{
			pegatron_porsche_cpld_write(CPLDB_ADDRESS, SFP_1_12_SCL_BASE, i+1);
			goto check_done;
		}
	}
	/*SFP 13-36*/
	for(i=0; i<CPLDA_SFP_NUM; i++)
	{
		if(!strcmp(attr->attr.name, SFP_CPLD_GROUPA_MAPPING[i]))
		{
			pegatron_porsche_cpld_write(CPLDA_ADDRESS, SFP_13_36_SCL_BASE, i+1);
			goto check_done;
		}
	}

	/*SFP 37-54*/
	for(i=0; i<CPLDC_SFP_NUM; i++)
	{
		if(!strcmp(attr->attr.name, SFP_CPLD_GROUPC_MAPPING[i]))
		{
			/* Enable QSFP i2c function */
			if(i >= 12)
			{
				cpldData = 0xff;
				cpldData = pegatron_porsche_cpld_read(CPLDC_ADDRESS, QSFP_I2C_ENABLE_BASE);
				CLEAR_BIT(cpldData, i-12);
				pegatron_porsche_cpld_write(CPLDC_ADDRESS, QSFP_I2C_ENABLE_BASE, cpldData);
			}
			pegatron_porsche_cpld_write(CPLDC_ADDRESS, SFP_37_54_SCL_BASE, i+1);
			goto check_done;
		}
	}

check_done:
	data = dev_get_drvdata(container_of(kobj, struct device, kobj));

	return porsche_sfp_read(data, buf, off, count);
}

#define SFP_EEPROM_ATTR(_num)    \
        static struct bin_attribute sfp##_num##_eeprom_attr = {   \
                .attr = {   \
                        .name =  __stringify(sfp##_num##_eeprom),  \
                        .mode = S_IRUGO\
                },  \
                .size = SFP_EEPROM_SIZE,    \
                .read = porsche_sfp_bin_read,   \
                }

SFP_EEPROM_ATTR(1);SFP_EEPROM_ATTR(2);SFP_EEPROM_ATTR(3);SFP_EEPROM_ATTR(4);SFP_EEPROM_ATTR(5);SFP_EEPROM_ATTR(6);SFP_EEPROM_ATTR(7);SFP_EEPROM_ATTR(8);SFP_EEPROM_ATTR(9);
SFP_EEPROM_ATTR(10);SFP_EEPROM_ATTR(11);SFP_EEPROM_ATTR(12);SFP_EEPROM_ATTR(13);SFP_EEPROM_ATTR(14);SFP_EEPROM_ATTR(15);SFP_EEPROM_ATTR(16);SFP_EEPROM_ATTR(17);SFP_EEPROM_ATTR(18);
SFP_EEPROM_ATTR(19);SFP_EEPROM_ATTR(20);SFP_EEPROM_ATTR(21);SFP_EEPROM_ATTR(22);SFP_EEPROM_ATTR(23);SFP_EEPROM_ATTR(24);SFP_EEPROM_ATTR(25);SFP_EEPROM_ATTR(26);SFP_EEPROM_ATTR(27);
SFP_EEPROM_ATTR(28);SFP_EEPROM_ATTR(29);SFP_EEPROM_ATTR(30);SFP_EEPROM_ATTR(31);SFP_EEPROM_ATTR(32);SFP_EEPROM_ATTR(33);SFP_EEPROM_ATTR(34);SFP_EEPROM_ATTR(35);SFP_EEPROM_ATTR(36);
SFP_EEPROM_ATTR(37);SFP_EEPROM_ATTR(38);SFP_EEPROM_ATTR(39);SFP_EEPROM_ATTR(40);SFP_EEPROM_ATTR(41);SFP_EEPROM_ATTR(42);SFP_EEPROM_ATTR(43);SFP_EEPROM_ATTR(44);SFP_EEPROM_ATTR(45);
SFP_EEPROM_ATTR(46);SFP_EEPROM_ATTR(47);SFP_EEPROM_ATTR(48);SFP_EEPROM_ATTR(49);SFP_EEPROM_ATTR(50);SFP_EEPROM_ATTR(51);SFP_EEPROM_ATTR(52);SFP_EEPROM_ATTR(53);SFP_EEPROM_ATTR(54);

static struct bin_attribute *porsche_cpldA_sfp_epprom_attributes[] = {
    &sfp13_eeprom_attr, &sfp14_eeprom_attr, &sfp15_eeprom_attr, &sfp16_eeprom_attr, &sfp17_eeprom_attr, &sfp18_eeprom_attr, &sfp19_eeprom_attr, &sfp20_eeprom_attr,
    &sfp21_eeprom_attr, &sfp22_eeprom_attr, &sfp23_eeprom_attr, &sfp24_eeprom_attr, &sfp25_eeprom_attr, &sfp26_eeprom_attr, &sfp27_eeprom_attr, &sfp28_eeprom_attr,
    &sfp29_eeprom_attr, &sfp30_eeprom_attr, &sfp31_eeprom_attr, &sfp32_eeprom_attr, &sfp33_eeprom_attr, &sfp34_eeprom_attr, &sfp35_eeprom_attr, &sfp36_eeprom_attr,
    NULL
};

static struct bin_attribute *porsche_cpldB_sfp_epprom_attributes[] = {
    &sfp1_eeprom_attr, &sfp2_eeprom_attr, &sfp3_eeprom_attr, &sfp4_eeprom_attr, &sfp5_eeprom_attr, &sfp6_eeprom_attr, &sfp7_eeprom_attr, &sfp8_eeprom_attr,
    &sfp9_eeprom_attr, &sfp10_eeprom_attr, &sfp11_eeprom_attr, &sfp12_eeprom_attr,
    NULL
};

static struct bin_attribute *porsche_cpldC_sfp_epprom_attributes[] = {
    &sfp37_eeprom_attr, &sfp38_eeprom_attr, &sfp39_eeprom_attr, &sfp40_eeprom_attr, &sfp41_eeprom_attr, &sfp42_eeprom_attr, &sfp43_eeprom_attr, &sfp44_eeprom_attr,
    &sfp45_eeprom_attr, &sfp46_eeprom_attr, &sfp47_eeprom_attr, &sfp48_eeprom_attr, &sfp49_eeprom_attr, &sfp50_eeprom_attr, &sfp51_eeprom_attr, &sfp52_eeprom_attr,
    &sfp53_eeprom_attr, &sfp54_eeprom_attr,
    NULL
};

static const struct attribute_group porsche_sfpA_group = { .bin_attrs = porsche_cpldA_sfp_epprom_attributes};
static const struct attribute_group porsche_sfpB_group = { .bin_attrs = porsche_cpldB_sfp_epprom_attributes};
static const struct attribute_group porsche_sfpC_group = { .bin_attrs = porsche_cpldC_sfp_epprom_attributes};

static int porsche_sfp_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
	int use_smbus = SFP_EEPROM_BUS_TYPE;
	struct porsche_sfp_data *data;
	int err, i;
	unsigned num_addresses;
	kernel_ulong_t magic;

	data = kzalloc(sizeof(struct porsche_sfp_data) , GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	mutex_init(&data->lock);
	data->use_smbus = use_smbus;
	/*
	 * Export the EEPROM bytes through sysfs, since that's convenient.
	 * By default, only root should see the data (maybe passwords etc)
	 */

	data->client = client;
	data->driver_data = dev_id->driver_data;

	sysfs_bin_attr_init(&data->bin);

	switch(dev_id->driver_data)
	{
		case cpld_group_a:
			err = sysfs_create_group(&client->dev.kobj, &porsche_sfpA_group);
			if (err)
				goto err_clients;
			break;
		case cpld_group_b:
			err = sysfs_create_group(&client->dev.kobj, &porsche_sfpB_group);
			if (err)
				goto err_clients;
			break;
		case cpld_group_c:
			err = sysfs_create_group(&client->dev.kobj, &porsche_sfpC_group);
			if (err)
				goto err_clients;
			break;
		default:
			printk(KERN_ALERT "i2c_check_CPLD failed\n");
	        err = -EIO;
			break;
	}

	i2c_set_clientdata(client, data);

	return 0;

err_clients:
	kfree(data);
	return err;	
}

static int porsche_sfp_device_remove(struct i2c_client *client)
{
	struct porsche_sfp_data *data;
	int i;

	data = i2c_get_clientdata(client);

	switch(data->driver_data)
    {
        case cpld_group_a:
            sysfs_remove_group(&client->dev.kobj, &porsche_sfpA_group);
            break;
        case cpld_group_b:
            sysfs_remove_group(&client->dev.kobj, &porsche_sfpB_group);
            break;
        case cpld_group_c:
            sysfs_remove_group(&client->dev.kobj, &porsche_sfpC_group);
            break;
        default:
            dev_dbg(&client->dev, "i2c_remove_CPLD failed (0x%x)\n", client->addr);
            break;
    }


	return 0;
}

static const struct i2c_device_id porsche_sfp_id[] = {
    { "porsche_sfpA", cpld_group_a },
    { "porsche_sfpB", cpld_group_b },
    { "porsche_sfpC", cpld_group_c },
    {}
};
MODULE_DEVICE_TABLE(i2c, porsche_sfp_id);

static struct i2c_driver porsche_sfp_driver = {
    .driver = {
        .name     = "pegatron_porsche_sfp",
    },
    .probe        = porsche_sfp_device_probe,
    .remove       = porsche_sfp_device_remove,
    .id_table     = porsche_sfp_id,
    .address_list = normal_i2c,
};

static int __init porsche_sfp_init(void)
{
	int i;

	/*SFP 1-12*/
	for(i=0; i<CPLDB_SFP_NUM; i++)
	{
		sprintf(SFP_CPLD_GROUPB_MAPPING[i], "sfp%d_eeprom", i+1);
	}
	/*SFP 13-36*/
	for(i=0; i<CPLDA_SFP_NUM; i++)
	{
		sprintf(SFP_CPLD_GROUPA_MAPPING[i], "sfp%d_eeprom", i+1+CPLDB_SFP_NUM);
	}

	/*SFP 37-54*/
	for(i=0; i<CPLDC_SFP_NUM; i++)
	{
		sprintf(SFP_CPLD_GROUPC_MAPPING[i], "sfp%d_eeprom",i+1+CPLDA_SFP_NUM+CPLDB_SFP_NUM);
	}

	return i2c_add_driver(&porsche_sfp_driver);
}

static void __exit porsche_sfp_exit(void)
{
	i2c_del_driver(&porsche_sfp_driver);
}

MODULE_AUTHOR("Peter5 Lin <Peter5_Lin@pegatroncorp.com.tw>");
MODULE_DESCRIPTION("porsche_cpld_mux driver");
MODULE_LICENSE("GPL");

module_init(porsche_sfp_init);
module_exit(porsche_sfp_exit);

