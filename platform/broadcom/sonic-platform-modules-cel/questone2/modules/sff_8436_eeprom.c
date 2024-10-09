/*
 * sff_8436_eeprom.c - handle most SFF-8436 based QSFP EEPROMs
 *
 * Copyright (C) 2014 Cumulus networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Freeoftware Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * 	Description:
 * 	a) SFF 8436 based qsfp read/write transactions are just like the at24 eeproms
 * 	b) The register/memory layout is up to 5 128 byte pages defined by a "pages valid"
 * 		register and switched via a "page select" register as explained in below diagram.
 * 	c) 256 bytes are mapped at a time. page 0 is always mapped to the first 128 bytes and
 * 	   the other 4 pages are selectively mapped to the second 128 bytes
 *
 *	                    SFF 8436 based QSFP Memory Map
 *
 *	                    2-Wire Serial Address: 1010000x
 *
 *	                    Lower Page 00h (128 bytes)
 *	                    =====================
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |                     |
 *	                   |Page Select Byte(127)|
 *	                    =====================
 *	                              |
 *	                              |
 *	                              |
 *	                              |
 *	                              V
 *	     -----------------------------------------------------------------
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    |                  |                    |                         |
 *	    V                  V                    V                         V
 *	 -------------   ----------------      -----------------     --------------
 *	|             | |                |    |                 |   |              |
 *	|   Upper     | |     Upper      |    |     Upper       |   |    Upper     |
 *	|  Page 00h   | |    Page 01h    |    |    Page 02h     |   |   Page 03h   |
 *	|             | |   (Optional)   |    |   (Optional)    |   |  (Optional   |
 *	|             | |                |    |                 |   |   for Cable  |
 *	|             | |                |    |                 |   |  Assemblies) |
 *	|    ID       | |     AST        |    |      User       |   |              |
 *	|  Fields     | |    Table       |    |   EEPROM Data   |   |              |
 *	|             | |                |    |                 |   |              |
 *	|             | |                |    |                 |   |              |
 *	|             | |                |    |                 |   |              |
 *	 -------------   ----------------      -----------------     --------------
 *
 *
 **/

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
#include "sff-8436.h"

#include <linux/types.h>
#include <linux/memory.h>

#define SFF_8436_EEPROM_SIZE       5*128
#define SFF_8436_MAX_PAGE_COUNT    5
#define SFF_8436_MMAP_SIZE         256
#define SFF_8436_PAGE_SELECT_REG   0x7F

#define  SFF_8436_OPTION_4_OFFSET           0xC3
#define   SFF_8436_PAGE_02_PRESENT          (1 << 7) /* Memory Page 02 present */
#define   SFF_8436_PAGE_01_PRESENT          (1 << 6) /* Memory Page 01 present */
#define  SFF_8436_STATUS_2_OFFSET           0x02
#define   SFF_8436_STATUS_PAGE_03_PRESENT_L  (1 << 2) /* Flat Memory:0- Paging, 1- Page 0 only */

struct sff_8436_data {
	struct sff_8436_platform_data chip;
	//struct memory_accessor macc;
	int use_smbus;

	/*
	 * Lock protects against activities from other Linux tasks,
	 * but not from changes by other I2C masters.
	 */
	struct mutex lock;
	struct bin_attribute bin;

	u8 *writebuf;
	unsigned write_max;

	unsigned num_addresses;

	u8  data[SFF_8436_EEPROM_SIZE];

	struct i2c_client *client[];
};

typedef enum qsfp_opcode {
    QSFP_READ_OP = 0,
    QSFP_WRITE_OP = 1
} qsfp_opcode_e;

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

/*
 *pecs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned write_timeout = 25;

#define SFF_8436_PAGE_SIZE 128
#define SFF_8436_SIZE_BYTELEN 5
#define SFF_8436_SIZE_FLAGS 8

#define SFF_8436_BITMASK(x) (BIT(x) - 1)


/* create non-zero magic value for given eeprom parameters */
#define SFF_8436_DEVICE_MAGIC(_len, _flags) 		\
	((1 << SFF_8436_SIZE_FLAGS | (_flags)) 		\
	    << SFF_8436_SIZE_BYTELEN | ilog2(_len))

static const struct i2c_device_id sff8436_ids[] = {
	{ "sff8436",SFF_8436_DEVICE_MAGIC(2048 / 8, 0) },
	{ /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(i2c, sff8436_ids);

/*-------------------------------------------------------------------------*/
/*
 * This routine computes the addressing information to be used for a given r/w request.
 * Assumes that sanity checks for offset happened at sysfs-layer.
 * Offset within Lower Page 00h and Upper Page 00h are not recomputed
 */
static uint8_t sff_8436_translate_offset(struct sff_8436_data *sff_8436,
		loff_t *offset)
{
	unsigned page = 0;

	if (*offset < SFF_8436_MMAP_SIZE) {
		return 0;
	}

	page = (*offset >> 7)-1;

	if (page > 0 ) {
		*offset = 0x80 + (*offset & 0x7f);
	} else {
		*offset &= 0xff;
	}

	return page;
}

static int sff_8436_read_reg(struct sff_8436_data *sff_8436,
                  uint8_t reg, uint8_t *val)
{
	int count = 1, i = 0;
	struct i2c_client *client = sff_8436->client[0];
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	ssize_t status;
	unsigned long timeout, read_time;

	memset(msg, 0, sizeof(msg));

	/*
	 * Writes fail if the previous one didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		read_time = jiffies;
		switch (sff_8436->use_smbus) {
		case I2C_SMBUS_I2C_BLOCK_DATA:
			status = i2c_smbus_read_i2c_block_data(client,
					reg, count, val);
			break;
		case I2C_SMBUS_WORD_DATA:
			status = i2c_smbus_read_word_data(client, reg);

			if (status >= 0) {
				*val = status & 0xff;
				status = count;
			}
			break;
		case I2C_SMBUS_BYTE_DATA:
			status = i2c_smbus_read_byte_data(client, reg);

			if (status >= 0) {
				*val = status;
				status = count;
			}
			break;

		default:
            i = 0;
			msgbuf[i++] = reg;

			msg[0].addr = client->addr;
			msg[0].buf = msgbuf;
			msg[0].len = i;

			msg[1].addr = client->addr;
			msg[1].flags = I2C_M_RD;
			msg[1].buf = val;
			msg[1].len = count;

			status = i2c_transfer(client->adapter, msg, 2);
			if (status == 2)
				status = count;
			break;
		}
		dev_dbg(&client->dev, "read (using smbus %d) %d@%d --> %zd (%ld)\n",
				sff_8436->use_smbus, count, reg, status, jiffies);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(read_time, timeout));

	return -ETIMEDOUT;
}

static int sff_8436_write_reg(struct sff_8436_data *sff_8436,
                  uint8_t reg, uint8_t val)
{
	uint8_t data[2] = { reg, val };
	int count = 1;
	struct i2c_client *client = sff_8436->client[0];
	struct i2c_msg msg;
	ssize_t status;
	unsigned long timeout, write_time;

	/*
	 * Writes fail if the previous one didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		write_time = jiffies;
		switch (sff_8436->use_smbus) {
		case I2C_SMBUS_I2C_BLOCK_DATA:
			status = i2c_smbus_write_i2c_block_data(client,
					reg, count, &val);
			if (status == 0)
				status = count;
			break;
		case I2C_SMBUS_WORD_DATA:
		case I2C_SMBUS_BYTE_DATA:
			status = i2c_smbus_write_byte_data(client, reg,  val);

			if (status == 0)
				status = count;
			break;
		default:
			msg.addr = client->addr;
			msg.flags = 0;
			msg.len = sizeof(data);
			msg.buf = (char *) data;

			status = i2c_transfer(client->adapter, &msg, 1);
			if (status == 1)
				status = count;
			break;
		}
		dev_dbg(&client->dev, "write (using smbus %d) %d@%d --> %zd (%ld)\n",
				sff_8436->use_smbus, count, reg, status, jiffies);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(write_time, timeout));

	return -ETIMEDOUT;
}

static int sff_8436_write_page_reg(struct sff_8436_data *sff_8436,
                               uint8_t val)
{
	return sff_8436_write_reg(sff_8436, SFF_8436_PAGE_SELECT_REG, val);
}

static ssize_t sff_8436_eeprom_read(struct sff_8436_data *sff_8436, char *buf,
		    unsigned offset, size_t count)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	struct i2c_client *client = sff_8436->client[0];
	unsigned long timeout, read_time;
	int status, i;

	memset(msg, 0, sizeof(msg));

	switch (sff_8436->use_smbus) {
	case I2C_SMBUS_I2C_BLOCK_DATA:
		/*smaller eeproms can work given some SMBus extension calls */
		if (count > I2C_SMBUS_BLOCK_MAX)
			count = I2C_SMBUS_BLOCK_MAX;
		break;
	case I2C_SMBUS_WORD_DATA:
		/* Check for odd length transaction */
		count = (count == 1) ? 1 : 2;
		break;
	case I2C_SMBUS_BYTE_DATA:
		count = 1;
		break;
	default:
		/*
		 * When we have a better choice than SMBus calls, use a
		 * combined I2C message. Write address; then read up to
		 * io_limit data bytes. Note that read page rollover helps us
		 * here (unlike writes). msgbuf is u8 and will cast to our
		 * needs.
		 */
		i = 0;
		msgbuf[i++] = offset;

		msg[0].addr = client->addr;
		msg[0].buf = msgbuf;
		msg[0].len = i;

		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].buf = buf;
		msg[1].len = count;
	}

	/*
	 * Reads fail if the previous write didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		read_time = jiffies;

		switch (sff_8436->use_smbus) {
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

		dev_dbg(&client->dev, "eeprom read %zu@%d --> %d (%ld)\n",
				count, offset, status, jiffies);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(read_time, timeout));

	return -ETIMEDOUT;
}

static ssize_t sff_8436_eeprom_write(struct sff_8436_data *sff_8436, const char *buf,
		    unsigned offset, size_t count)
{
	struct i2c_client *client = sff_8436->client[0];
	struct i2c_msg msg;
	unsigned long timeout, write_time;
	unsigned next_page;
	int status, i = 0;

	/* write max is at most a page */
	if (count > sff_8436->write_max)
		count = sff_8436->write_max;

		/* Never roll over backwards, to the start of this page */
	next_page = roundup(offset + 1, SFF_8436_PAGE_SIZE);
	if (offset + count > next_page)
		count = next_page - offset;

	switch (sff_8436->use_smbus) {
	case I2C_SMBUS_I2C_BLOCK_DATA:
		/*smaller eeproms can work given some SMBus extension calls */
		if (count > I2C_SMBUS_BLOCK_MAX)
			count = I2C_SMBUS_BLOCK_MAX;
		break;
	case I2C_SMBUS_WORD_DATA:
		/* Check for odd length transaction */
		count = (count == 1) ? 1 : 2;
		break;
	case I2C_SMBUS_BYTE_DATA:
		count = 1;
		break;
	default:
		/* If we'll use I2C calls for I/O, set up the message */
		msg.addr = client->addr;
		msg.flags = 0;

		/* msg.buf is u8 and casts will mask the values */
		msg.buf = sff_8436->writebuf;

		msg.buf[i++] = offset;
		memcpy(&msg.buf[i], buf, count);
		msg.len = i + count;
		break;
	}

	/*
	 * Reads fail if the previous write didn't complete yet. We may
	 * loop a few times until this one succeeds, waiting at least
	 * long enough for one entire page write to work.
	 */
	timeout = jiffies + msecs_to_jiffies(write_timeout);
	do {
		write_time = jiffies;

		switch (sff_8436->use_smbus) {
		case I2C_SMBUS_I2C_BLOCK_DATA:
			status = i2c_smbus_write_i2c_block_data(client,
			              offset, count, buf);
			if (status == 0)
				status = count;
			break;
		case I2C_SMBUS_WORD_DATA:
			if (count == 2) {
				status = i2c_smbus_write_word_data(
				            client,offset,(u16)((buf[0]) |
				            (buf[1] << 8)));
			} else {
					/* count = 1 */
					status = i2c_smbus_write_byte_data(
					           client, offset, buf[0]);
			}
			if (status == 0)
				status = count;
			break;
		case I2C_SMBUS_BYTE_DATA:
			status = i2c_smbus_write_byte_data(client, offset,  buf[0]);
			if (status == 0)
				status = count;
			break;
		default:
			status = i2c_transfer(client->adapter, &msg, 1);
			if (status == 1)
				status = count;
			break;
		}

		dev_dbg(&client->dev, "eeprom write %zu@%d --> %d (%ld)\n",
				count, offset, status, jiffies);

		if (status == count)
			return count;

		/* REVISIT: at HZ=100, this is sloooow */
		msleep(1);
	} while (time_before(write_time, timeout));

	return -ETIMEDOUT;
}

static ssize_t sff_8436_eeprom_update_client(struct sff_8436_data *sff_8436,
                                loff_t off, size_t count, qsfp_opcode_e opcode)
{
	struct i2c_client *client = sff_8436->client[0];
	ssize_t retval = 0;
	u8 page = 0;
	loff_t phy_offset = off;
	int ret = 0;

	page = sff_8436_translate_offset(sff_8436, &phy_offset);

	dev_dbg(&client->dev,
					"sff_8436_eeprom_update_client off %lld  page:%d phy_offset:%lld, count:%zu, opcode:%d\n",
					off, page, phy_offset, count, opcode);
	if (page > 0) {
		ret = sff_8436_write_page_reg(sff_8436, page);
		if (ret < 0) {
			dev_err(&client->dev,
					"sff_8436_write_page_reg for page %d failed ret:%d!\n",
					page, ret);
			return ret;
		}
	}

	while (count) {
		ssize_t	status;

		if (opcode == QSFP_READ_OP) {
			status =  sff_8436_eeprom_read(sff_8436, (char *)(&sff_8436->data[off]), phy_offset, count);
		} else {
			status =  sff_8436_eeprom_write(sff_8436, (char *)(&sff_8436->data[off]), phy_offset, count);
		}
		if (status <= 0) {
			if (retval == 0)
				retval = status;
			break;
		}
		phy_offset += status;
		off += status;
		count -= status;
		retval += status;
	}


	if (page > 0) {
		ret = sff_8436_write_page_reg(sff_8436, 0);
		if (ret < 0) {
			dev_err(&client->dev,
					"sff_8436_write_page_reg for page 0 failed ret:%d!\n", ret);
			return ret;
		}
	}
	return retval;
}

static ssize_t sff_8436_read_write(struct sff_8436_data *sff_8436,
		char *buf, loff_t off, size_t len, qsfp_opcode_e opcode)
{
	struct i2c_client *client = sff_8436->client[0];
	u8 page;
	u8 refresh_page = 0;
	int ret = 0;
	u8 val = 0;
	int err_timeout = 0;
	size_t pending_len = 0, page_len = 0;
	loff_t page_offset = 0, page_start_offset = 0;

	if (unlikely(!len))
		return len;

	if (off > SFF_8436_EEPROM_SIZE)
		return 0;

	if (off + len > SFF_8436_EEPROM_SIZE)
		len = SFF_8436_EEPROM_SIZE - off;

	if (opcode == QSFP_READ_OP) {
		memset(sff_8436->data, 0xff, SFF_8436_EEPROM_SIZE);
	} else if (opcode == QSFP_WRITE_OP) {
		memcpy(&sff_8436->data[off], buf, len);
	}

	/*
	 * Read data from chip, protecting against concurrent updates
	 * from this host, but not from other I2C masters.
	 */
	mutex_lock(&sff_8436->lock);

	/*
	 * Refresh pages which covers the requested data
	 * from offset to  off + len
	 * Only refresh pages which contain requested bytes
	 *
	 */

	pending_len = len;

	for (page = off >> 7; page <= (off + len - 1) >> 7; page++) {
		refresh_page = 0;
		switch (page) {
		case 0:
			/* Lower page 00h */
			refresh_page = 1;
			err_timeout = 1;
			break;
		case 1:
			/* Upper page 00h */
			refresh_page = 1;
			err_timeout = 1;
			break;
		case 2:
			/* Upper page 01h */
			ret = sff_8436_read_reg(sff_8436, SFF_8436_OPTION_4_OFFSET, &val);
			if (ret < 0)  {
				dev_dbg(&client->dev,
				"sff_8436_read_reg for page 01h status failed %d!\n", ret);
				goto err;
			}
			if (val & SFF_8436_PAGE_01_PRESENT) {
				refresh_page = 1;
			}
			break;
		case 3:
			/* Upper page 02h */
			ret = sff_8436_read_reg(sff_8436, SFF_8436_OPTION_4_OFFSET, &val);
			if (ret < 0)  {
				dev_dbg(&client->dev,
				"sff_8436_read_reg for page 02h status failed %d!\n", ret);
				goto err;
			}
			if (val & SFF_8436_PAGE_02_PRESENT) {
				refresh_page = 1;
			}
			break;
		case 4:
			/* Upper page 03h */
			ret = sff_8436_read_reg(sff_8436, SFF_8436_STATUS_2_OFFSET, &val);
			if (ret < 0)  {
				dev_dbg(&client->dev,
				"sff_8436_read_reg for page 03h status failed %d!\n", ret);
				goto err;
			}
			if (!(val & SFF_8436_STATUS_PAGE_03_PRESENT_L)) {
				refresh_page = 1;
			}
			break;
		default:
			/* Invalid page index */
			dev_err(&client->dev, "Invalid page %d!\n", page);
			ret = -EINVAL;
			goto err;
		}

		if (!refresh_page) {
			/* if page is not valid or already refreshed */
			continue;
		}

		/*
		 * Compute the offset and number of bytes to be read/write
		 * w.r.t requested page
		 *
		 * 1. start at offset 0 (within the page), and read/write the entire page
		 * 2. start at offset 0 (within the page) and read/write less than entire page
		 * 3. start at an offset not equal to 0 and read/write the rest of the page
		 * 4. start at an offset not equal to 0 and read/write less than (end of page - offset)
		 *
		 */
		page_start_offset = page * SFF_8436_PAGE_SIZE;

		if (page_start_offset < off) {
			page_offset = off;
			if (off + pending_len < page_start_offset + SFF_8436_PAGE_SIZE) {
				page_len = pending_len;
			} else {
				page_len = SFF_8436_PAGE_SIZE - off;
			}
		} else {
			page_offset = page_start_offset;
			if (pending_len > SFF_8436_PAGE_SIZE) {
				page_len = SFF_8436_PAGE_SIZE;
			} else {
				page_len = pending_len;
			}
		}

		pending_len = pending_len - page_len;

		dev_dbg(&client->dev,
				"sff_read off %lld len %zu page_start_offset %lld page_offset %lld page_len %zu pending_len %zu\n",
				off, len, page_start_offset, page_offset, page_len, pending_len);

		/* Refresh the data from offset for specified len */
		ret = sff_8436_eeprom_update_client(sff_8436, page_offset, page_len, opcode);
		if (ret != page_len) {
			if (err_timeout) {
				dev_dbg(&client->dev, "sff_8436_update_client for %s page %d page_offset %lld page_len %zu failed %d!\n",
							(page ? "Upper" : "Lower"), (page ? (page-1) : page), page_offset, page_len, ret);
				goto err;
			} else {
				dev_err(&client->dev, "sff_8436_update_client for %s page %d page_offset %lld page_len %zu failed %d!\n",
							(page ? "Upper" : "Lower"), (page ? (page-1) : page), page_offset, page_len, ret);
			}
		}
	}
	mutex_unlock(&sff_8436->lock);

	if (opcode == QSFP_READ_OP) {
		memcpy(buf, &sff_8436->data[off], len);
	}
	return len;

err:
	mutex_unlock(&sff_8436->lock);

	return ret;
}

static ssize_t sff_8436_bin_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
	struct sff_8436_data *sff_8436 = i2c_get_clientdata(client);

	return sff_8436_read_write(sff_8436, buf, off, count, QSFP_READ_OP);
}


static ssize_t sff_8436_bin_write(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr,
		char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
	struct sff_8436_data *sff_8436 = i2c_get_clientdata(client);

	return sff_8436_read_write(sff_8436, buf, off, count, QSFP_WRITE_OP);
}
/*-------------------------------------------------------------------------*/

/*
 * This lets other kernel code access the eeprom data. For example, it
 * might hold a board's Ethernet address, or board-specific calibration
 * data generated on the manufacturing floor.
 */

// static ssize_t sff_8436_macc_read(struct memory_accessor *macc, char *buf,
// 			 off_t offset, size_t count)
// {
// 	struct sff_8436_data *sff_8436 = container_of(macc, struct sff_8436_data, macc);

// 	return sff_8436_read_write(sff_8436, buf, offset, count, QSFP_READ_OP);
// }

// static ssize_t sff_8436_macc_write(struct memory_accessor *macc, const char *buf,
//              off_t offset, size_t count)
// {
// 	struct sff_8436_data *sff_8436 = container_of(macc, struct sff_8436_data, macc);

// 	return sff_8436_read_write(sff_8436, buf, offset, count, QSFP_WRITE_OP);
// }

/*-------------------------------------------------------------------------*/

static int sff_8436_remove(struct i2c_client *client)
{
	struct sff_8436_data *sff_8436;

	sff_8436 = i2c_get_clientdata(client);
	sysfs_remove_bin_file(&client->dev.kobj, &sff_8436->bin);

	kfree(sff_8436->writebuf);
	kfree(sff_8436);
	return 0;
}
static int sff_8436_eeprom_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;
	int use_smbus = 0;
	struct sff_8436_platform_data chip;
	struct sff_8436_data *sff_8436;
	kernel_ulong_t magic;

	if (client->dev.platform_data) {
		chip = *(struct sff_8436_platform_data *)client->dev.platform_data;
	} else {
		/*
		 * SFF-8436 MMAP is 256 bytes long
		 */
		magic = SFF_8436_DEVICE_MAGIC(2048 / 8, 0);
		chip.byte_len = BIT(magic & SFF_8436_BITMASK(SFF_8436_SIZE_BYTELEN));
		magic >>= SFF_8436_SIZE_BYTELEN;
		chip.flags = magic & SFF_8436_BITMASK(SFF_8436_SIZE_FLAGS);
		/*
		 * This is slow, but we can't know all eeproms, so we better
		 * play safe.pecifying custom eeprom-types via platform_data
		 * is recommended anyhow.
		 */
		chip.page_size = 1;

		//chip.setup = NULL;
		//chip.context = NULL;
	}

	if (!is_power_of_2(chip.byte_len))
		dev_warn(&client->dev,
			"byte_len looks suspicious (no power of 2)!\n");

	if (!chip.page_size) {
		dev_err(&client->dev, "page_size must not be 0!\n");
		err = -EINVAL;
		goto exit;
	}
	if (!is_power_of_2(chip.page_size))
		dev_warn(&client->dev,
			"page_size looks suspicious (no power of 2)!\n");

	/* Use I2C operations unless we're stuck with SMBus extensions. */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		if (i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
			use_smbus = I2C_SMBUS_I2C_BLOCK_DATA;
		} else if (i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			use_smbus = I2C_SMBUS_WORD_DATA;
		} else if (i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			use_smbus = I2C_SMBUS_BYTE_DATA;
		} else {
			err = -EPFNOSUPPORT;
			goto exit;
		}
	}

	if (!(sff_8436 = kzalloc(sizeof(struct sff_8436_data) +  sizeof(struct i2c_client *), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	mutex_init(&sff_8436->lock);
	sff_8436->use_smbus = use_smbus;
	sff_8436->chip = chip;

	/*
	 * Export the EEPROM bytes through sysfs, since that's convenient.
	 * By default, only root should see the data (maybe passwords etc)
	 */
	sysfs_bin_attr_init(&sff_8436->bin);
	sff_8436->bin.attr.name = "eeprom";
	sff_8436->bin.attr.mode = SFF_8436_FLAG_IRUGO;
	sff_8436->bin.read = sff_8436_bin_read;
	sff_8436->bin.size = SFF_8436_EEPROM_SIZE;

	//sff_8436->macc.read = sff_8436_macc_read;

	if (!use_smbus ||
			(i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) ||
			i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_WRITE_WORD_DATA) ||
			i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
		//unsigned write_max = chip.page_size;
		/*
		 * NOTE: AN-2079
		 * Finisar recommends that the host implement 1 byte writes only,
		 * since this module only supports 32 byte page boundaries.
		 * 2 byte writes are acceptable for PE and Vout changes per
		 * Application Note AN-2071.
		 */
		unsigned write_max = 1;

		//sff_8436->macc.write = sff_8436_macc_write;

		sff_8436->bin.write = sff_8436_bin_write;
		sff_8436->bin.attr.mode |= S_IWUSR;

		if (write_max > io_limit)
			write_max = io_limit;
		if (use_smbus && write_max > I2C_SMBUS_BLOCK_MAX)
			write_max = I2C_SMBUS_BLOCK_MAX;
		sff_8436->write_max = write_max;

		/* buffer (data + address at the beginning) */
		sff_8436->writebuf = kmalloc(write_max + 2, GFP_KERNEL);
		if (!sff_8436->writebuf) {
			err = -ENOMEM;
			goto exit_kfree;
		}
	} else {
			dev_warn(&client->dev,
				"cannot write due to controller restrictions.");
	}

	memset(sff_8436->data, 0xff, SFF_8436_EEPROM_SIZE);

	sff_8436->client[0] = client;

	/* create the sysfs eeprom file */
	err = sysfs_create_bin_file(&client->dev.kobj, &sff_8436->bin);
	if (err)
		goto err_struct;

	i2c_set_clientdata(client, sff_8436);

	dev_info(&client->dev, "%zu byte %s EEPROM, %s\n",
		sff_8436->bin.size, client->name,
		"read-only");

	if (use_smbus == I2C_SMBUS_WORD_DATA ||
	    use_smbus == I2C_SMBUS_BYTE_DATA) {
		dev_notice(&client->dev, "Falling back to %s reads, "
			   "performance will suffer\n", use_smbus ==
			   I2C_SMBUS_WORD_DATA ? "word" : "byte");
	}

	//if (chip.setup)
		//chip.setup(&sff_8436->macc, chip.context);

	return 0;

err_sysfs_cleanup:
	sysfs_remove_bin_file(&client->dev.kobj, &sff_8436->bin);
err_struct:
	kfree(sff_8436->writebuf);
exit_kfree:
	kfree(sff_8436);
exit:
	dev_dbg(&client->dev, "probe error %d\n", err);

	return err;
}

/*-------------------------------------------------------------------------*/

static struct i2c_driver sff_8436_driver = {
	.driver = {
		.name = "sff8436",
		.owner = THIS_MODULE,
	},
	.probe = sff_8436_eeprom_probe,
	.remove = sff_8436_remove,
	.id_table = sff8436_ids,
};

static int __init sff_8436_init(void)
{
	if (!io_limit) {
		pr_err("sff_8436: io_limit must not be 0!\n");
		return -EINVAL;
	}

	io_limit = rounddown_pow_of_two(io_limit);
	return i2c_add_driver(&sff_8436_driver);
}
module_init(sff_8436_init);

static void __exit sff_8436_exit(void)
{
	i2c_del_driver(&sff_8436_driver);
}
module_exit(sff_8436_exit);

MODULE_DESCRIPTION("Driver for SFF-8436 based QSFP EEPROMs");
MODULE_AUTHOR("VIDYA RAVIPATI <vidya@cumulusnetworks.com>");
MODULE_LICENSE("GPL");
