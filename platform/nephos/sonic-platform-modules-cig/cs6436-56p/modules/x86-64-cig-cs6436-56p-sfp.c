/*
 * A hwmon driver for the CIG cs6436-56P SFP Module
 *
 * Copyright (C) 2018 Cambridge, Inc.
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
#include <linux/delay.h>
#include "i2c-algo-lpc.h"

#define DRIVER_NAME 	"cs6436_56p_sfp" /* Platform dependent */

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printk (KERN_INFO "%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#define EEPROM_NAME				"sfp_eeprom"
#define EEPROM_SIZE				256	/*	256 byte eeprom */
#define BIT_INDEX(i)			(1ULL << (i))
#define USE_I2C_BLOCK_READ 		1 /* Platform dependent */
#define I2C_RW_RETRY_COUNT		3
#define I2C_RW_RETRY_INTERVAL	100 /* ms */

#define SFP_EEPROM_A0_I2C_ADDR (0xA0 >> 1)
#define SFP_EEPROM_A2_I2C_ADDR (0xA2 >> 1)

#define SFF8024_PHYSICAL_DEVICE_ID_ADDR		0x0
#define SFF8024_DEVICE_ID_SFP				0x3
#define SFF8024_DEVICE_ID_QSFP				0xC
#define SFF8024_DEVICE_ID_QSFP_PLUS			0xD
#define SFF8024_DEVICE_ID_QSFP28			0x11

#define SFF8472_DIAG_MON_TYPE_ADDR			92
#define SFF8472_DIAG_MON_TYPE_DDM_MASK		0x40
#define SFF8472_10G_ETH_COMPLIANCE_ADDR		0x3
#define SFF8472_10G_BASE_MASK				0xF0

#define SFF8436_RX_LOS_ADDR					3
#define SFF8436_TX_FAULT_ADDR				4
#define SFF8436_TX_DISABLE_ADDR				86
#define QSFP_RESET_ADDR                     0x1b
#define QSFP_INTER_ADDR                     0x1a
#define QSFP_LPMODE_ADDR                    0x1c

static ssize_t show_port_number(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_port_type(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_present(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_show_tx_rx_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_show_tx_rx_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_set_tx_disable(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_set_tx_disable(struct device *dev, struct device_attribute *da, const char *buf, size_t count);;
static ssize_t sfp_show_ddm_implemented(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_eeprom_read(struct i2c_client *, u8, u8 *,int);
static ssize_t sfp_eeprom_write(struct i2c_client *, u8 , const char *,int);
extern int cig_cpld_read_register(u8 reg_off, u8 *val);
extern int cig_cpld_write_register(u8 reg_off, u8 val);

static ssize_t qsfp_reset_read(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_reset_write(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_inter_read(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_lpmode_read(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_lpmode_write(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

enum sfp_sysfs_attributes {
	PRESENT,
	PRESENT_ALL,
	PORT_NUMBER,
	PORT_TYPE,
	DDM_IMPLEMENTED,
	TX_FAULT,
	TX_FAULT1,
	TX_FAULT2,
	TX_FAULT3,
	TX_FAULT4,
	TX_DISABLE,
	TX_DISABLE1,
	TX_DISABLE2,
	TX_DISABLE3,
	TX_DISABLE4,
	RX_LOS,
	RX_LOS1,
	RX_LOS2,
	RX_LOS3,
	RX_LOS4,
	RX_LOS_ALL,
	QSFPRESET,
	QSFPINT,
    QSFPLPMODE
};

/* SFP/QSFP common attributes for sysfs */
static SENSOR_DEVICE_ATTR(sfp_port_number, S_IRUGO, show_port_number, NULL, PORT_NUMBER);
static SENSOR_DEVICE_ATTR(sfp_port_type, S_IRUGO, show_port_type, NULL, PORT_TYPE);
static SENSOR_DEVICE_ATTR(sfp_is_present,  S_IRUGO, show_present, NULL, PRESENT);
static SENSOR_DEVICE_ATTR(sfp_is_present_all,  S_IRUGO, show_present, NULL, PRESENT_ALL);
static SENSOR_DEVICE_ATTR(sfp_rx_los,  S_IRUGO, sfp_show_tx_rx_status, NULL, RX_LOS);
static SENSOR_DEVICE_ATTR(sfp_tx_disable,  S_IWUSR | S_IRUGO, sfp_show_tx_rx_status, sfp_set_tx_disable, TX_DISABLE);
static SENSOR_DEVICE_ATTR(sfp_tx_fault,	 S_IRUGO, sfp_show_tx_rx_status, NULL, TX_FAULT);

/* QSFP attributes for sysfs */
static SENSOR_DEVICE_ATTR(sfp_rx_los1, S_IRUGO, qsfp_show_tx_rx_status, NULL, RX_LOS1);
static SENSOR_DEVICE_ATTR(sfp_rx_los2, S_IRUGO, qsfp_show_tx_rx_status, NULL, RX_LOS2);
static SENSOR_DEVICE_ATTR(sfp_rx_los3, S_IRUGO, qsfp_show_tx_rx_status, NULL, RX_LOS3);
static SENSOR_DEVICE_ATTR(sfp_rx_los4, S_IRUGO, qsfp_show_tx_rx_status, NULL, RX_LOS4);
static SENSOR_DEVICE_ATTR(sfp_tx_disable1, S_IWUSR | S_IRUGO, qsfp_show_tx_rx_status, qsfp_set_tx_disable, TX_DISABLE1);
static SENSOR_DEVICE_ATTR(sfp_tx_disable2, S_IWUSR | S_IRUGO, qsfp_show_tx_rx_status, qsfp_set_tx_disable, TX_DISABLE2);
static SENSOR_DEVICE_ATTR(sfp_tx_disable3, S_IWUSR | S_IRUGO, qsfp_show_tx_rx_status, qsfp_set_tx_disable, TX_DISABLE3);
static SENSOR_DEVICE_ATTR(sfp_tx_disable4, S_IWUSR | S_IRUGO, qsfp_show_tx_rx_status, qsfp_set_tx_disable, TX_DISABLE4);
static SENSOR_DEVICE_ATTR(sfp_tx_fault1, S_IRUGO, qsfp_show_tx_rx_status, NULL, TX_FAULT1);
static SENSOR_DEVICE_ATTR(sfp_tx_fault2, S_IRUGO, qsfp_show_tx_rx_status, NULL, TX_FAULT2);
static SENSOR_DEVICE_ATTR(sfp_tx_fault3, S_IRUGO, qsfp_show_tx_rx_status, NULL, TX_FAULT3);
static SENSOR_DEVICE_ATTR(sfp_tx_fault4, S_IRUGO, qsfp_show_tx_rx_status, NULL, TX_FAULT4);
static SENSOR_DEVICE_ATTR(sfp_reset, S_IWUSR | S_IRUGO, qsfp_reset_read, qsfp_reset_write, QSFPRESET);
static SENSOR_DEVICE_ATTR(sfp_inter, S_IRUGO, qsfp_inter_read, NULL, QSFPINT);
static SENSOR_DEVICE_ATTR(sfp_lpmode, S_IWUSR | S_IRUGO, qsfp_lpmode_read, qsfp_lpmode_write, QSFPLPMODE);
static struct attribute *qsfp_attributes[] = {
	&sensor_dev_attr_sfp_port_number.dev_attr.attr,
	&sensor_dev_attr_sfp_port_type.dev_attr.attr,
	&sensor_dev_attr_sfp_is_present.dev_attr.attr,
	&sensor_dev_attr_sfp_is_present_all.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los1.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los2.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los3.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los4.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_disable.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_disable1.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_disable2.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_disable3.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_disable4.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_fault.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_fault1.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_fault2.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_fault3.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_fault4.dev_attr.attr,
    &sensor_dev_attr_sfp_reset.dev_attr.attr,
    &sensor_dev_attr_sfp_inter.dev_attr.attr,
    &sensor_dev_attr_sfp_lpmode.dev_attr.attr,
	NULL
};

/* SFP msa attributes for sysfs */
static SENSOR_DEVICE_ATTR(sfp_ddm_implemented,	 S_IRUGO, sfp_show_ddm_implemented, NULL, DDM_IMPLEMENTED);
static SENSOR_DEVICE_ATTR(sfp_rx_los_all,  S_IRUGO, sfp_show_tx_rx_status, NULL, RX_LOS_ALL);
static struct attribute *sfp_msa_attributes[] = {
	&sensor_dev_attr_sfp_port_number.dev_attr.attr,
	&sensor_dev_attr_sfp_port_type.dev_attr.attr,
	&sensor_dev_attr_sfp_is_present.dev_attr.attr,
	&sensor_dev_attr_sfp_is_present_all.dev_attr.attr,
	&sensor_dev_attr_sfp_ddm_implemented.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_fault.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los.dev_attr.attr,
	&sensor_dev_attr_sfp_rx_los_all.dev_attr.attr,
	&sensor_dev_attr_sfp_tx_disable.dev_attr.attr,
	NULL
};

/* SFP ddm attributes for sysfs */
static struct attribute *sfp_ddm_attributes[] = {
	NULL
};

/* Platform dependent +++ */
#define CPLD_PORT_TO_FRONT_PORT(port)  (port+1)

enum port_numbers {
cs6436_56p_sfp1,  cs6436_56p_sfp2,  cs6436_56p_sfp3,  cs6436_56p_sfp4,
cs6436_56p_sfp5,  cs6436_56p_sfp6,  cs6436_56p_sfp7,  cs6436_56p_sfp8,
cs6436_56p_sfp9,  cs6436_56p_sfp10, cs6436_56p_sfp11, cs6436_56p_sfp12,
cs6436_56p_sfp13, cs6436_56p_sfp14, cs6436_56p_sfp15, cs6436_56p_sfp16,
cs6436_56p_sfp17, cs6436_56p_sfp18, cs6436_56p_sfp19, cs6436_56p_sfp20,
cs6436_56p_sfp21, cs6436_56p_sfp22, cs6436_56p_sfp23, cs6436_56p_sfp24,
cs6436_56p_sfp25, cs6436_56p_sfp26, cs6436_56p_sfp27, cs6436_56p_sfp28,
cs6436_56p_sfp29, cs6436_56p_sfp30, cs6436_56p_sfp31, cs6436_56p_sfp32,
cs6436_56p_sfp33, cs6436_56p_sfp34, cs6436_56p_sfp35, cs6436_56p_sfp36,
cs6436_56p_sfp37, cs6436_56p_sfp38, cs6436_56p_sfp39, cs6436_56p_sfp40,
cs6436_56p_sfp41, cs6436_56p_sfp42, cs6436_56p_sfp43, cs6436_56p_sfp44,
cs6436_56p_sfp45, cs6436_56p_sfp46, cs6436_56p_sfp47, cs6436_56p_sfp48,
cs6436_56p_sfp49, cs6436_56p_sfp50, cs6436_56p_sfp51, cs6436_56p_sfp52,
cs6436_56p_sfp53, cs6436_56p_sfp54, cs6436_56p_sfp55, cs6436_56p_sfp56
};

#define I2C_DEV_ID(x) { #x, x}

static const struct i2c_device_id sfp_device_id[] = {
I2C_DEV_ID(cs6436_56p_sfp1),
I2C_DEV_ID(cs6436_56p_sfp2),
I2C_DEV_ID(cs6436_56p_sfp3),
I2C_DEV_ID(cs6436_56p_sfp4),
I2C_DEV_ID(cs6436_56p_sfp5),
I2C_DEV_ID(cs6436_56p_sfp6),
I2C_DEV_ID(cs6436_56p_sfp7),
I2C_DEV_ID(cs6436_56p_sfp8),
I2C_DEV_ID(cs6436_56p_sfp9),
I2C_DEV_ID(cs6436_56p_sfp10),
I2C_DEV_ID(cs6436_56p_sfp11),
I2C_DEV_ID(cs6436_56p_sfp12),
I2C_DEV_ID(cs6436_56p_sfp13),
I2C_DEV_ID(cs6436_56p_sfp14),
I2C_DEV_ID(cs6436_56p_sfp15),
I2C_DEV_ID(cs6436_56p_sfp16),
I2C_DEV_ID(cs6436_56p_sfp17),
I2C_DEV_ID(cs6436_56p_sfp18),
I2C_DEV_ID(cs6436_56p_sfp19),
I2C_DEV_ID(cs6436_56p_sfp20),
I2C_DEV_ID(cs6436_56p_sfp21),
I2C_DEV_ID(cs6436_56p_sfp22),
I2C_DEV_ID(cs6436_56p_sfp23),
I2C_DEV_ID(cs6436_56p_sfp24),
I2C_DEV_ID(cs6436_56p_sfp25),
I2C_DEV_ID(cs6436_56p_sfp26),
I2C_DEV_ID(cs6436_56p_sfp27),
I2C_DEV_ID(cs6436_56p_sfp28),
I2C_DEV_ID(cs6436_56p_sfp29),
I2C_DEV_ID(cs6436_56p_sfp30),
I2C_DEV_ID(cs6436_56p_sfp31),
I2C_DEV_ID(cs6436_56p_sfp32),
I2C_DEV_ID(cs6436_56p_sfp33),
I2C_DEV_ID(cs6436_56p_sfp34),
I2C_DEV_ID(cs6436_56p_sfp35),
I2C_DEV_ID(cs6436_56p_sfp36),
I2C_DEV_ID(cs6436_56p_sfp37),
I2C_DEV_ID(cs6436_56p_sfp38),
I2C_DEV_ID(cs6436_56p_sfp39),
I2C_DEV_ID(cs6436_56p_sfp40),
I2C_DEV_ID(cs6436_56p_sfp41),
I2C_DEV_ID(cs6436_56p_sfp42),
I2C_DEV_ID(cs6436_56p_sfp43),
I2C_DEV_ID(cs6436_56p_sfp44),
I2C_DEV_ID(cs6436_56p_sfp45),
I2C_DEV_ID(cs6436_56p_sfp46),
I2C_DEV_ID(cs6436_56p_sfp47),
I2C_DEV_ID(cs6436_56p_sfp48),
I2C_DEV_ID(cs6436_56p_sfp49),
I2C_DEV_ID(cs6436_56p_sfp50),
I2C_DEV_ID(cs6436_56p_sfp51),
I2C_DEV_ID(cs6436_56p_sfp52),
I2C_DEV_ID(cs6436_56p_sfp53),
I2C_DEV_ID(cs6436_56p_sfp54),
I2C_DEV_ID(cs6436_56p_sfp55),
I2C_DEV_ID(cs6436_56p_sfp56),
{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, sfp_device_id);

/*
 * list of valid port types
 * note OOM_PORT_TYPE_NOT_PRESENT to indicate no
 * module is present in this port
 */
typedef enum oom_driver_port_type_e {
	OOM_DRIVER_PORT_TYPE_INVALID,
	OOM_DRIVER_PORT_TYPE_NOT_PRESENT,
	OOM_DRIVER_PORT_TYPE_SFP,
	OOM_DRIVER_PORT_TYPE_SFP_PLUS,
	OOM_DRIVER_PORT_TYPE_QSFP,
	OOM_DRIVER_PORT_TYPE_QSFP_PLUS,
	OOM_DRIVER_PORT_TYPE_QSFP28
} oom_driver_port_type_t;

enum driver_type_e {
	DRIVER_TYPE_SFP_MSA,
	DRIVER_TYPE_SFP_DDM,
	DRIVER_TYPE_QSFP
};

/* Each client has this additional data
 */
struct eeprom_data {
	char				 valid;			/* !=0 if registers are valid */
	unsigned long		 last_updated;	/* In jiffies */
	struct bin_attribute bin;			/* eeprom data */
};

struct sfp_msa_data {
	char			valid;			/* !=0 if registers are valid */
	unsigned long	last_updated;	/* In jiffies */
	u64				status[6];		/* bit0:port0, bit1:port1 and so on */
									/* index 0 => tx_fail
											 1 => tx_disable
											 2 => rx_loss
											 3 => device id
											 4 => 10G Ethernet Compliance Codes
												  to distinguish SFP or SFP+
											 5 => DIAGNOSTIC MONITORING TYPE */
	struct eeprom_data				eeprom;
};

struct sfp_ddm_data {
	struct eeprom_data				eeprom;
};

struct qsfp_data {
	char			valid;			/* !=0 if registers are valid */
	unsigned long	last_updated;	/* In jiffies */
	u8				status[3];		/* bit0:port0, bit1:port1 and so on */
									/* index 0 => tx_fail
											 1 => tx_disable
											 2 => rx_loss */

	u8					device_id;
	struct eeprom_data	eeprom;
};

struct sfp_port_data {
	struct mutex		   update_lock;
	enum driver_type_e	   driver_type;
	int					   port;		/* CPLD port index */
	oom_driver_port_type_t port_type;
	u64					   present;		/* present status, bit0:port0, bit1:port1 and so on */

	struct sfp_msa_data	  *msa;
	struct sfp_ddm_data	  *ddm;
	struct qsfp_data	  *qsfp;

	struct i2c_client	  *client;
};

static ssize_t show_port_number(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);
	return sprintf(buf, "%d\n", CPLD_PORT_TO_FRONT_PORT(data->port));
}



static int cig_cpld_write_sfp_register(u8 sfp_reg_addr, u8 sfp_write_reg_data)
{
	u8 sfp_read_status = 0;
	u8 wait_time_out = WAIT_TIME_OUT_COUNT;

	cig_cpld_write_register(ADDR_REG_SFP_STATUS_ADDR, sfp_reg_addr << 1);
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_TX, sfp_write_reg_data);
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_COMMAND, 0x80);
	do{
		cig_cpld_read_register(ADDR_REG_SFP_STATUS_STATUS, &sfp_read_status);
		udelay(60);
		wait_time_out--;
		if(wait_time_out == 0)
			break;
	}while(sfp_read_status != 0x02);

	if(wait_time_out == 0)
		return -1;

	return 1;
}


static int cig_cpld_read_sfp_register(u8 sfp_reg_addr, u8 *sfp_read_reg_data)
{
	u8 sfp_read_status = 0;
	u8 wait_time_out = WAIT_TIME_OUT_COUNT;

	cig_cpld_write_register(ADDR_REG_SFP_STATUS_ADDR, sfp_reg_addr << 1 | 1);
	cig_cpld_write_register(ADDR_REG_SFP_STATUS_COMMAND, 0x80);
	do{
		udelay(60);
		cig_cpld_read_register(ADDR_REG_SFP_STATUS_STATUS, &sfp_read_status);
		wait_time_out--;
		if(wait_time_out == 0)
			break;
	}while(sfp_read_status != 0x01);

	cig_cpld_read_register(ADDR_REG_SFP_STATUS_RX,sfp_read_reg_data);

	if(wait_time_out == 0)
		return -1;

	return 1;
}






/* Platform dependent +++ */
static struct sfp_port_data *sfp_update_present(struct i2c_client *client)
{
	int i = 0, j = 0, status = -1;
	unsigned char cpld_reg_data = 0,cpld_reg_addr = 0;
	struct sfp_port_data *data = i2c_get_clientdata(client);

	DEBUG_PRINT("Starting sfp present status update");
	mutex_lock(&data->update_lock);
	data->present = 0;

	udelay(6000);

	/* Read present status of port 1~48(SFP port) */
	for (i = 0; i < 6; i++) {
			cpld_reg_addr 	= 1 + i;

			status = cig_cpld_read_sfp_register(cpld_reg_addr, &cpld_reg_data);

			if (unlikely(status < 0)) {
				dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_reg_addr, cpld_reg_data, status);
				goto exit;
			}

			data->present |= (u64)cpld_reg_data << (i*8);

			DEBUG_PRINT("Present status = 0x%lx\r\n", data->present);
	}

	/* Read present status of port 49-56(QSFP port) */
	cpld_reg_addr = 25;
	status 	  = cig_cpld_read_sfp_register(cpld_reg_addr, &cpld_reg_data);
	if (unlikely(status < 0)) {
		dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_reg_addr, cpld_reg_data, status);
		goto exit;
	}
	else {
		data->present |= (u64)cpld_reg_data << 48;
	}

	DEBUG_PRINT("Present status = 0x%lx", data->present);
exit:
	mutex_unlock(&data->update_lock);
	return (status < 0) ? ERR_PTR(status) : data;
}

static struct sfp_port_data *sfp_update_tx_rx_status(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);
	int i = 0, j = 0;
	int status = -1;
	unsigned char cpld_reg_data = 0,cpld_reg_addr = 0;

	if (time_before(jiffies, data->msa->last_updated + HZ + HZ / 2) && data->msa->valid) {
        return data;
	}

	DEBUG_PRINT("Starting cs6436_56p sfp tx rx status update");
	mutex_lock(&data->update_lock);
	data->msa->valid = 0;
	memset(data->msa->status, 0, sizeof(data->msa->status));

	udelay(6000);

	/* Read status of port 1~48(SFP port) */
	for (i = 0; i < 6; i++) {
			cpld_reg_addr = 13+i;

			status	= cig_cpld_read_sfp_register(cpld_reg_addr, &cpld_reg_data);
			if (unlikely(status < 0)) {
				dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_reg_addr, cpld_reg_data, status);
				goto exit;
			}

			data->msa->status[0] |= (u64)cpld_reg_data << (i * 8);

			DEBUG_PRINT("tx rx status[0] = 0x%lx\r\n", data->msa->status[0]);
	}


	for (i = 0; i < 6; i++) {
			cpld_reg_addr = 19+i;

			status	= cig_cpld_read_sfp_register(cpld_reg_addr, &cpld_reg_data);
			if (unlikely(status < 0)) {
				dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_reg_addr, cpld_reg_data, status);
				goto exit;
			}

			data->msa->status[1] |= (u64)cpld_reg_data << (i * 8);

			DEBUG_PRINT("tx rx status[1] = 0x%lx\r\n", data->msa->status[1]);
	}

	for (i = 0; i < 6; i++) {
			cpld_reg_addr = 7+i;

			status	= cig_cpld_read_sfp_register(cpld_reg_addr, &cpld_reg_data);
			if (unlikely(status < 0)) {
				dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_reg_addr, cpld_reg_data, status);
				goto exit;
			}

			data->msa->status[2] |= (u64)cpld_reg_data << (i * 8);

			DEBUG_PRINT("tx rx status[2] = 0x%lx\r\n", data->msa->status[2]);
	}

	data->msa->valid = 1;
	data->msa->last_updated = jiffies;

exit:
	mutex_unlock(&data->update_lock);
    return (status < 0) ? ERR_PTR(status) : data;
}

static ssize_t sfp_set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);
	unsigned char cpld_reg_data = 0,cpld_reg_addr = 0,cpld_reg_bit = 0,cpld_reg_val = 0;
	long disable;
	int error;

	if (data->driver_type == DRIVER_TYPE_QSFP) {
		return qsfp_set_tx_disable(dev, da, buf, count);
	}

	error = kstrtol(buf, 10, &disable);
	if (error) {
		return error;
	}

	mutex_lock(&data->update_lock);

	udelay(6000);

	if(data->port <= 48) {
		cpld_reg_addr = 19 + data->port / 8;
		cpld_reg_bit = 1 << ((data->port) % 8);
	}

	/* Read current status */
	error = cig_cpld_read_sfp_register(cpld_reg_addr, &cpld_reg_data);

	/* Update tx_disable status */
	if (disable) {
		data->msa->status[1] |= BIT_INDEX(data->port);
		cpld_reg_data |= cpld_reg_bit;
	}
	else {
		data->msa->status[1] &= ~ BIT_INDEX(data->port);
		cpld_reg_data &= ~cpld_reg_bit;
	}

	error = cig_cpld_write_sfp_register(cpld_reg_addr,cpld_reg_data);

	mutex_unlock(&data->update_lock);
	return count;
}
/* Platform dependent --- */

static int sfp_is_port_present(struct i2c_client *client, int port)
{
	struct sfp_port_data *data = i2c_get_clientdata(client);

	data = sfp_update_present(client);
	if (IS_ERR(data)) {
		return PTR_ERR(data);
	}

	return !(data->present & BIT_INDEX(data->port)); /* Platform dependent */
}

/* Platform dependent +++ */
static ssize_t show_present(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);

	if (PRESENT_ALL == attr->index) {
		int i;
		u8 values[7]  = {0};
		struct sfp_port_data *data = sfp_update_present(client);

		if (IS_ERR(data)) {
			return PTR_ERR(data);
		}

		for (i = 0; i < ARRAY_SIZE(values); i++) {
			values[i] = ~(u8)(data->present >> (i * 8));
		}

        /* Return values 1 -> 56 in order */
        return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
                       values[0], values[1], values[2],
                       values[3], values[4], values[5],
                       values[6]);
	}
	else {
		struct sfp_port_data *data = i2c_get_clientdata(client);
		int present = sfp_is_port_present(client, data->port);

		if (IS_ERR_VALUE(present)) {
			return present;
		}

		/* PRESENT */
		return sprintf(buf, "%d\n", present);
	}
}
/* Platform dependent --- */

static struct sfp_port_data *sfp_update_port_type(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);
	u8 buf = 0;
	int status;

	mutex_lock(&data->update_lock);

	switch (data->driver_type) {
		case DRIVER_TYPE_SFP_MSA:
		{
			status = sfp_eeprom_read(client, SFF8024_PHYSICAL_DEVICE_ID_ADDR, &buf, sizeof(buf));
			if (unlikely(status < 0)) {
				data->port_type = OOM_DRIVER_PORT_TYPE_INVALID;
				break;
			}

			if (buf != SFF8024_DEVICE_ID_SFP) {
				data->port_type = OOM_DRIVER_PORT_TYPE_INVALID;
				break;
			}

			status = sfp_eeprom_read(client, SFF8472_10G_ETH_COMPLIANCE_ADDR, &buf, sizeof(buf));
			if (unlikely(status < 0)) {
				data->port_type = OOM_DRIVER_PORT_TYPE_INVALID;
				break;
			}

			DEBUG_PRINT("sfp port type (0x3) data = (0x%x)", buf);
			data->port_type = buf & SFF8472_10G_BASE_MASK ? OOM_DRIVER_PORT_TYPE_SFP_PLUS : OOM_DRIVER_PORT_TYPE_SFP;
			break;
		}
		case DRIVER_TYPE_QSFP:
		{
			status = sfp_eeprom_read(client, SFF8024_PHYSICAL_DEVICE_ID_ADDR, &buf, sizeof(buf));
			if (unlikely(status < 0)) {
				data->port_type = OOM_DRIVER_PORT_TYPE_INVALID;
				break;
			}

			DEBUG_PRINT("qsfp port type (0x0) buf = (0x%x)", buf);
			switch (buf) {
			case SFF8024_DEVICE_ID_QSFP:
				data->port_type = OOM_DRIVER_PORT_TYPE_QSFP;
				break;
			case SFF8024_DEVICE_ID_QSFP_PLUS:
				data->port_type = OOM_DRIVER_PORT_TYPE_QSFP_PLUS;
				break;
			case SFF8024_DEVICE_ID_QSFP28:
				data->port_type = OOM_DRIVER_PORT_TYPE_QSFP_PLUS;
				break;
			default:
				data->port_type = buf;
				break;
			}

			break;
		}
		default:
			break;
	}

	mutex_unlock(&data->update_lock);
	return data;
}

static ssize_t show_port_type(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);
	int present = sfp_is_port_present(client, data->port);

	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (!present) {
		/* port is not present */
		return sprintf(buf, "%d\n", OOM_DRIVER_PORT_TYPE_NOT_PRESENT);
	}

	sfp_update_port_type(dev);
	return sprintf(buf, "%d\n", data->port_type);
}

static struct sfp_port_data *qsfp_update_tx_rx_status(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);
	int i, status = -1;
	u8 buf = 0;
	u8 reg[] = {SFF8436_TX_FAULT_ADDR, SFF8436_TX_DISABLE_ADDR, SFF8436_RX_LOS_ADDR};

	DEBUG_PRINT("");
	if (time_before(jiffies, data->qsfp->last_updated + HZ + HZ / 2) && data->qsfp->valid) {
		return data;
	}

	DEBUG_PRINT("Starting sfp tx rx status update");
	mutex_lock(&data->update_lock);
	data->qsfp->valid = 0;
	memset(data->qsfp->status, 0, sizeof(data->qsfp->status));

	DEBUG_PRINT("");
	/* Notify device to update tx fault/ tx disable/ rx los status */
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		status = sfp_eeprom_read(client, reg[i], &buf, sizeof(buf));
		if (unlikely(status < 0)) {
			DEBUG_PRINT("");
			goto exit;
		}
	}
	msleep(200);
	DEBUG_PRINT("");

	/* Read actual tx fault/ tx disable/ rx los status */
	for (i = 0; i < ARRAY_SIZE(reg); i++) {
		status = sfp_eeprom_read(client, reg[i], &buf, sizeof(buf));
		if (unlikely(status < 0)) {
			DEBUG_PRINT("");
			goto exit;
		}

		DEBUG_PRINT("qsfp reg(0x%x) status = (0x%x)", reg[i], data->qsfp->status[i]);
		data->qsfp->status[i] = (buf & 0xF);
	}

	DEBUG_PRINT("");
	data->qsfp->valid = 1;
	data->qsfp->last_updated = jiffies;

exit:
	DEBUG_PRINT("");
	mutex_unlock(&data->update_lock);
	return (status < 0) ? ERR_PTR(status) : data;
}

static ssize_t qsfp_inter_read(struct device *dev, struct device_attribute *da, char *buf)
{
	int present;
	int status;
	u8 val = 0;
	int ret = 0;
	u8 cpld_reg_data = 0;
	u8 index = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	present = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
        return -ENODEV;
	}

	mutex_lock(&data->update_lock);

	udelay(6000);
	/* Read current status */
	ret = cig_cpld_read_sfp_register(QSFP_INTER_ADDR, &cpld_reg_data);
	index = data->port - 48;
	index = 1 << index;
    val = (cpld_reg_data & index) > 0 ? 1 : 0;

	printk("inter read:data->port = %d, index = %hhu, cpld_reg_data = %hhu\n", data->port, index, cpld_reg_data);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", val);
}


static ssize_t qsfp_reset_read(struct device *dev, struct device_attribute *da, char *buf)
{
	int present;
	int status;
	u8 val = 0;
	int ret = 0;
	u8 cpld_reg_data = 0;
	u8 index = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	present = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
        return -ENODEV;
	}

	mutex_lock(&data->update_lock);

	udelay(6000);
	/* Read current status */
	ret = cig_cpld_read_sfp_register(QSFP_RESET_ADDR, &cpld_reg_data);
	index = data->port - 48;
	index = 1 << index;
    val = (cpld_reg_data & index) > 0 ? 1 : 0;

	printk("reset read:data->port = %d, index = %hhu, cpld_reg_data = %hhu\n", data->port, index, cpld_reg_data);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", val);
}

static ssize_t qsfp_reset_write(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	int present;
	int status;
	u8 val = 0;
	int ret = 0;
	u8 cpld_reg_data = 0;
	u8 index = 0;
	long usrdata;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	present = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
        return -ENODEV;
	}

	ret = kstrtol(buf, 10, &usrdata);
	if (ret) {
		return ret;
	}

    usrdata = usrdata > 0 ? 1 : 0;
	index = data->port - 48;

    DEBUG_PRINT("usrdata = %u, index = %hhu\n", usrdata, index);

	mutex_lock(&data->update_lock);

	udelay(6000);
	/* Read current status */
	ret = cig_cpld_read_sfp_register(QSFP_RESET_ADDR, &cpld_reg_data);
	if (ret == 1)
	{

        DEBUG_PRINT("cpld_reg_data = %x\n", cpld_reg_data);
        cpld_reg_data &= ~(1 << index);
        cpld_reg_data |= usrdata << index;

        DEBUG_PRINT("cpld_reg_data = %x\n", cpld_reg_data);
        ret = cig_cpld_write_sfp_register(QSFP_RESET_ADDR, cpld_reg_data);
        if (1 != ret)
        {
            DEBUG_PRINT("write failed\n");
        }
	}
	else
	{
        DEBUG_PRINT("read failed\n");
	}

	mutex_unlock(&data->update_lock);

    if (ret != 1)
        return -1;

    return count;
}




static ssize_t qsfp_lpmode_read(struct device *dev, struct device_attribute *da, char *buf)
{
	int present;
	int status;
	u8 val = 0;
	int ret = 0;
	u8 cpld_reg_data = 0;
	u8 index = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	present = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
        return -ENODEV;
	}

	mutex_lock(&data->update_lock);

	udelay(6000);
	/* Read current status */
	ret = cig_cpld_read_sfp_register(QSFP_LPMODE_ADDR, &cpld_reg_data);
	index = data->port - 48;
	index = 1 << index;
    val = (cpld_reg_data & index) > 0 ? 1 : 0;

	printk("lpmode read:data->port = %d, index = %hhu, cpld_reg_data = %hhu\n", data->port, index, cpld_reg_data);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", val);
}

static ssize_t qsfp_lpmode_write(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	int present;
	int status;
	u8 val = 0;
	int ret = 0;
	u8 cpld_reg_data = 0;
	u8 index = 0;
	long usrdata;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	present = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
        return -ENODEV;
	}

	ret = kstrtol(buf, 10, &usrdata);
	if (ret) {
		return ret;
	}

    usrdata = usrdata > 0 ? 1 : 0;
	index = data->port - 48;

    DEBUG_PRINT("usrdata = %u, index = %hhu\n", usrdata, index);

	mutex_lock(&data->update_lock);

	udelay(6000);
	/* Read current status */
	ret = cig_cpld_read_sfp_register(QSFP_LPMODE_ADDR, &cpld_reg_data);
	if (ret == 1)
	{

        DEBUG_PRINT("cpld_reg_data = %x\n", cpld_reg_data);
        cpld_reg_data &= ~(1 << index);
        cpld_reg_data |= usrdata << index;

        DEBUG_PRINT("cpld_reg_data = %x\n", cpld_reg_data);
        ret = cig_cpld_write_sfp_register(QSFP_LPMODE_ADDR, cpld_reg_data);
        if (1 != ret)
        {
            DEBUG_PRINT("write failed\n");
        }
	}
	else
	{
        DEBUG_PRINT("read failed\n");
	}

	mutex_unlock(&data->update_lock);

    if (ret != 1)
        return -1;

    return count;
}



static ssize_t qsfp_show_tx_rx_status(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int present;
	u8 val = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	DEBUG_PRINT("");
	present = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	DEBUG_PRINT("");
	if (present == 0) {
		/* port is not present */
        return -ENODEV;
	}

	DEBUG_PRINT("");
	data = qsfp_update_tx_rx_status(dev);
	DEBUG_PRINT("");
	if (IS_ERR(data)) {
		return PTR_ERR(data);
	}

	DEBUG_PRINT("");
	switch (attr->index) {
	case TX_FAULT:
		val = !!(data->qsfp->status[2] & 0xF);
		break;
	case TX_FAULT1:
	case TX_FAULT2:
	case TX_FAULT3:
	case TX_FAULT4:
		val = !!(data->qsfp->status[2] & BIT_INDEX(attr->index - TX_FAULT1));
		break;
	case TX_DISABLE:
		val = data->qsfp->status[1] & 0xF;
		break;
	case TX_DISABLE1:
	case TX_DISABLE2:
	case TX_DISABLE3:
	case TX_DISABLE4:
		val = !!(data->qsfp->status[1] & BIT_INDEX(attr->index - TX_DISABLE1));
		break;
	case RX_LOS:
		val = !!(data->qsfp->status[0] & 0xF);
		break;
	case RX_LOS1:
	case RX_LOS2:
	case RX_LOS3:
	case RX_LOS4:
		val = !!(data->qsfp->status[0] & BIT_INDEX(attr->index - RX_LOS1));
		break;
	default:
		break;
	}

	DEBUG_PRINT("");
	return sprintf(buf, "%d\n", val);
}

static ssize_t qsfp_set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	long disable;
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	status = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(status)) {
		return status;
	}

	if (!status) {
		/* port is not present */
		return -ENXIO;
	}

	status = kstrtol(buf, 10, &disable);
	if (status) {
		return status;
	}

	data = qsfp_update_tx_rx_status(dev);
	if (IS_ERR(data)) {
		return PTR_ERR(data);
	}

	mutex_lock(&data->update_lock);

	if (attr->index == TX_DISABLE) {
		data->qsfp->status[1] = disable & 0xF;
	}
	else {/* TX_DISABLE1 ~ TX_DISABLE4*/
		if (disable) {
			data->qsfp->status[1] |= (1 << (attr->index - TX_DISABLE1));
		}
		else {
			data->qsfp->status[1] &= ~(1 << (attr->index - TX_DISABLE1));
		}
	}

	DEBUG_PRINT("index = (%d), status = (0x%x)", attr->index, data->qsfp->status[1]);
	status = sfp_eeprom_write(data->client, SFF8436_TX_DISABLE_ADDR, &data->qsfp->status[1], sizeof(data->qsfp->status[1]));
	if (unlikely(status < 0)) {
		count = status;
	}

	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t sfp_show_ddm_implemented(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	char ddm;
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	status = sfp_is_port_present(client, data->port);
	if (IS_ERR_VALUE(status)) {
		return status;
	}

	if (status == 0) {
		/* port is not present */
		return -ENODEV;
	}

	status = sfp_eeprom_read(client, SFF8472_DIAG_MON_TYPE_ADDR, &ddm, sizeof(ddm));
	if (unlikely(status < 0)) {
		return status;
	}

	return sprintf(buf, "%d\n", !!(ddm & SFF8472_DIAG_MON_TYPE_DDM_MASK));
}

/* Platform dependent +++ */
static ssize_t sfp_show_tx_rx_status(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u8 val = 0, index = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct sfp_port_data *data = i2c_get_clientdata(client);

	DEBUG_PRINT("driver type = (%d)", data->driver_type);
	if (data->driver_type == DRIVER_TYPE_QSFP) {
		DEBUG_PRINT("");
		return qsfp_show_tx_rx_status(dev, da, buf);
	}

	DEBUG_PRINT("");
	data = sfp_update_tx_rx_status(dev);
	if (IS_ERR(data)) {
		return PTR_ERR(data);
	}

	if(attr->index == RX_LOS_ALL) {
		int i = 0;
		u8 values[6] = {0};

		for (i = 0; i < ARRAY_SIZE(values); i++) {
			values[i] = (u8)(data->msa->status[2] >> (i * 8));
		}

		/** Return values 1 -> 48 in order */
		return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x\n",
						values[0], values[1], values[2],
						values[3], values[4], values[5]);
	}

	switch (attr->index) {
	case TX_FAULT:
		index = 0;
		break;
	case TX_DISABLE:
		index = 1;
		break;
	case RX_LOS:
		index = 2;
		break;
	default:
		break;
	}

    val = !!(data->msa->status[index] & BIT_INDEX(data->port));
    return sprintf(buf, "%d\n", val);
}
/* Platform dependent --- */
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
	int present;
	struct sfp_port_data *data;
	DEBUG_PRINT("%s(%d) offset = (%d), count = (%d)", off, count);
	data = dev_get_drvdata(container_of(kobj, struct device, kobj));

	present = sfp_is_port_present(data->client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
		return -ENODEV;
	}

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
	int present;
	struct sfp_port_data *data;
	DEBUG_PRINT("offset = (%d), count = (%d)", off, count);
	data = dev_get_drvdata(container_of(kobj, struct device, kobj));

	present = sfp_is_port_present(data->client, data->port);
	if (IS_ERR_VALUE(present)) {
		return present;
	}

	if (present == 0) {
		/* port is not present */
		return -ENODEV;
	}

	return sfp_port_read(data, buf, off, count);
}

static int sfp_sysfs_eeprom_init(struct kobject *kobj, struct bin_attribute *eeprom)
{
	int err;

	sysfs_bin_attr_init(eeprom);
	eeprom->attr.name = EEPROM_NAME;
	eeprom->attr.mode = S_IWUSR | S_IRUGO;
	eeprom->read	  = sfp_bin_read;
	eeprom->write	  = sfp_bin_write;
	eeprom->size	  = EEPROM_SIZE;

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

static const struct attribute_group sfp_msa_group = {
	.attrs = sfp_msa_attributes,
};

static int sfp_i2c_check_functionality(struct i2c_client *client)
{
#if USE_I2C_BLOCK_READ
	return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK);
#else
	return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA);
#endif
}

static int sfp_msa_probe(struct i2c_client *client, const struct i2c_device_id *dev_id,
							   struct sfp_msa_data **data)
{
	int status;
	struct sfp_msa_data *msa;

	if (!sfp_i2c_check_functionality(client)) {
		status = -EIO;
		goto exit;
	}

	msa = kzalloc(sizeof(struct sfp_msa_data), GFP_KERNEL);
	if (!msa) {
		status = -ENOMEM;
		goto exit;
	}

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &sfp_msa_group);
	if (status) {
		goto exit_free;
	}

	/* init eeprom */
	status = sfp_sysfs_eeprom_init(&client->dev.kobj, &msa->eeprom.bin);
	if (status) {
		goto exit_remove;
	}

	*data = msa;
	dev_info(&client->dev, "sfp msa '%s'\n", client->name);

	cs6436_56p_sysfs_add_client(client);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &sfp_msa_group);
exit_free:
	kfree(msa);
exit:

	return status;
}

static const struct attribute_group sfp_ddm_group = {
	.attrs = sfp_ddm_attributes,
};

static int sfp_ddm_probe(struct i2c_client *client, const struct i2c_device_id *dev_id,
							   struct sfp_ddm_data **data)
{
	int status;
	struct sfp_ddm_data *ddm;

	if (!sfp_i2c_check_functionality(client)) {
		status = -EIO;
		goto exit;
	}

	ddm = kzalloc(sizeof(struct sfp_ddm_data), GFP_KERNEL);
	if (!ddm) {
		status = -ENOMEM;
		goto exit;
	}

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &sfp_ddm_group);
	if (status) {
		goto exit_free;
	}

	/* init eeprom */
	status = sfp_sysfs_eeprom_init(&client->dev.kobj, &ddm->eeprom.bin);
	if (status) {
		goto exit_remove;
	}

	*data = ddm;
	dev_info(&client->dev, "sfp ddm '%s'\n", client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &sfp_ddm_group);
exit_free:
	kfree(ddm);
exit:

	return status;
}

static const struct attribute_group qsfp_group = {
	.attrs = qsfp_attributes,
};

static int qsfp_probe(struct i2c_client *client, const struct i2c_device_id *dev_id,
						  struct qsfp_data **data)
{
	int status;
	struct qsfp_data *qsfp;

	if (!sfp_i2c_check_functionality(client)) {
		status = -EIO;
		goto exit;
	}

	qsfp = kzalloc(sizeof(struct qsfp_data), GFP_KERNEL);
	if (!qsfp) {
		status = -ENOMEM;
		goto exit;
	}

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &qsfp_group);
	if (status) {
		goto exit_free;
	}

	/* init eeprom */
	status = sfp_sysfs_eeprom_init(&client->dev.kobj, &qsfp->eeprom.bin);
	if (status) {
		goto exit_remove;
	}

	/* Bring QSFPs out of reset */
	//cig_lpc_write(0x62, 0x15, 0x3F);

	*data = qsfp;
	dev_info(&client->dev, "qsfp '%s'\n", client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &qsfp_group);
exit_free:
	kfree(qsfp);
exit:

	return status;
}

/* Platform dependent +++ */
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

	if (dev_id->driver_data >= cs6436_56p_sfp1 && dev_id->driver_data <= cs6436_56p_sfp48) {
		if (client->addr == SFP_EEPROM_A0_I2C_ADDR) {
			data->driver_type = DRIVER_TYPE_SFP_MSA;
			return sfp_msa_probe(client, dev_id, &data->msa);
		}
		else if (client->addr == SFP_EEPROM_A2_I2C_ADDR) {
			data->driver_type = DRIVER_TYPE_SFP_DDM;
			return sfp_ddm_probe(client, dev_id, &data->ddm);
		}
	}
	else { /* cs6436_56p_sfp49 ~ cs6436_56p_sfp56 */
		if (client->addr == SFP_EEPROM_A0_I2C_ADDR) {
			data->driver_type = DRIVER_TYPE_QSFP;
			return qsfp_probe(client, dev_id, &data->qsfp);
		}
	}

	return -ENODEV;
}
/* Platform dependent --- */

static int sfp_msa_remove(struct i2c_client *client, struct sfp_msa_data *data)
{
	sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &data->eeprom.bin);
	sysfs_remove_group(&client->dev.kobj, &sfp_msa_group);
	kfree(data);
	return 0;
}

static int sfp_ddm_remove(struct i2c_client *client, struct sfp_ddm_data *data)
{
	sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &data->eeprom.bin);
	sysfs_remove_group(&client->dev.kobj, &sfp_ddm_group);
	kfree(data);
	return 0;
}

static int qfp_remove(struct i2c_client *client, struct qsfp_data *data)
{
	sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &data->eeprom.bin);
	sysfs_remove_group(&client->dev.kobj, &qsfp_group);
	kfree(data);
	return 0;
}

static int sfp_device_remove(struct i2c_client *client)
{
	struct sfp_port_data *data = i2c_get_clientdata(client);

	cs6436_56p_sysfs_remove_client(client);
	switch (data->driver_type) {
		case DRIVER_TYPE_SFP_MSA:
			return sfp_msa_remove(client, data->msa);
		case DRIVER_TYPE_SFP_DDM:
			return sfp_ddm_remove(client, data->ddm);
		case DRIVER_TYPE_QSFP:
			return qfp_remove(client, data->qsfp);
	}

	return 0;
}

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static struct i2c_driver cs6436_56p_sfp_driver = {
	.driver = {
		.name	  = DRIVER_NAME,
	},
	.probe		  = sfp_device_probe,
	.remove		  = sfp_device_remove,
    .id_table     = sfp_device_id,
	.address_list = normal_i2c,
};

module_i2c_driver(cs6436_56p_sfp_driver);


MODULE_AUTHOR("Zhang Peng <zhangpeng@cigtech.com>");
MODULE_DESCRIPTION("cs6436_56p_sfp driver");
MODULE_LICENSE("GPL");

