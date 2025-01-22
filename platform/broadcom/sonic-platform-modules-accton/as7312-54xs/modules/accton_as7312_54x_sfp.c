/*
 * SFP driver for accton as7312_54x sfp
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

#define DRIVER_NAME 	"as7312_54x_sfp" /* Platform dependent */

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
#define DEBUG_PRINT(fmt, args...)                                        \
		printk (KERN_INFO "%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#define NUM_OF_PORT 		54
#define SFP_PORT_MAX        48
#define EEPROM_NAME 			"sfp_eeprom"
#define EEPROM_SIZE				256	/*  256 byte eeprom */
#define BIT_INDEX(i) 			(1ULL << (i))
#define USE_I2C_BLOCK_READ 		1 /* Platform dependent */
#define I2C_RW_RETRY_COUNT		10
#define I2C_RW_RETRY_INTERVAL	60 /* ms */

#define SFP_EEPROM_A0_I2C_ADDR (0xA0 >> 1)

#define SFF8024_PHYSICAL_DEVICE_ID_ADDR		0x0
#define SFF8024_DEVICE_ID_SFP				0x3
#define SFF8024_DEVICE_ID_QSFP				0xC
#define SFF8024_DEVICE_ID_QSFP_PLUS			0xD
#define SFF8024_DEVICE_ID_QSFP28			0x11

#define SFF8472_DIAG_MON_TYPE_ADDR        	92
#define SFF8472_DIAG_MON_TYPE_DDM_MASK		0x40
#define SFF8436_RX_LOS_ADDR					3
#define SFF8436_TX_FAULT_ADDR				4
#define SFF8436_TX_DISABLE_ADDR				86

#define MULTIPAGE_SUPPORT		1

#if (MULTIPAGE_SUPPORT == 1)
/* fundamental unit of addressing for SFF_8472/SFF_8436 */
#define SFF_8436_PAGE_SIZE 128
/*
 * The current 8436 (QSFP) spec provides for only 4 supported
 * pages (pages 0-3).
 * This driver is prepared to support more, but needs a register in the
 * EEPROM to indicate how many pages are supported before it is safe
 * to implement more pages in the driver.
 */
#define SFF_8436_SPECED_PAGES 4
#define SFF_8436_EEPROM_SIZE ((1 + SFF_8436_SPECED_PAGES) * SFF_8436_PAGE_SIZE)
#define SFF_8436_EEPROM_UNPAGED_SIZE (2 * SFF_8436_PAGE_SIZE)
/*
 * The current 8472 (SFP) spec provides for only 3 supported
 * pages (pages 0-2).
 * This driver is prepared to support more, but needs a register in the
 * EEPROM to indicate how many pages are supported before it is safe
 * to implement more pages in the driver.
 */
#define SFF_8472_SPECED_PAGES 3
#define SFF_8472_EEPROM_SIZE ((3 + SFF_8472_SPECED_PAGES) * SFF_8436_PAGE_SIZE)
#define SFF_8472_EEPROM_UNPAGED_SIZE (4 * SFF_8436_PAGE_SIZE)

/* a few constants to find our way around the EEPROM */
#define SFF_8436_PAGE_SELECT_REG   0x7F
#define SFF_8436_PAGEABLE_REG 0x02
#define SFF_8436_NOT_PAGEABLE (1<<2)
#define SFF_8472_PAGEABLE_REG 0x40
#define SFF_8472_PAGEABLE (1<<4)

/*
 * This parameter is to help this driver avoid blocking other drivers out
 * of I2C for potentially troublesome amounts of time. With a 100 kHz I2C
 * clock, one 256 byte read takes about 1/43 second which is excessive;
 * but the 1/170 second it takes at 400 kHz may be quite reasonable; and
 * at 1 MHz (Fm+) a 1/430 second delay could easily be invisible.
 *
 * This value is forced to be a power of two so that writes align on pages.
 */
static unsigned io_limit = SFF_8436_PAGE_SIZE;

/*
 * specs often allow 5 msec for a page write, sometimes 20 msec;
 * it's important to recover from write timeouts.
 */
static unsigned write_timeout = 25;

typedef enum qsfp_opcode {
    QSFP_READ_OP = 0,
    QSFP_WRITE_OP = 1
} qsfp_opcode_e;
#endif

/* Platform dependent +++ */
#define I2C_ADDR_CPLD1	0x60
#define I2C_ADDR_CPLD2	0x62
#define I2C_ADDR_CPLD3	0x64

#define CPLD3_OFFSET_QSFP_MOD_RST   0x17
/* Platform dependent --- */
static ssize_t show_port_number(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_present(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_show_tx_rx_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t qsfp_show_tx_rx_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t sfp_set_tx_disable(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
static ssize_t qsfp_set_tx_disable(struct device *dev, struct device_attribute *da, const char *buf, size_t count);;
static ssize_t sfp_eeprom_read(struct i2c_client *, u8, u8 *,int);
static ssize_t sfp_eeprom_write(struct i2c_client *, u8 , const char *,int);
static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da,
                              char *buf);
static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count);
extern int accton_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int accton_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);
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
    SFP_MOD_RST
};

/* SFP/QSFP common attributes for sysfs */
static SENSOR_DEVICE_ATTR(sfp_port_number, S_IRUGO, show_port_number, NULL, PORT_NUMBER);
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
static SENSOR_DEVICE_ATTR(sfp_mod_rst,     S_IWUSR | S_IRUGO, get_mode_reset, set_mode_reset, SFP_MOD_RST);

static struct attribute *qsfp_attributes[] = {
    &sensor_dev_attr_sfp_port_number.dev_attr.attr,
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
    &sensor_dev_attr_sfp_mod_rst.dev_attr.attr,
    NULL
};

/* SFP msa attributes for sysfs */
static SENSOR_DEVICE_ATTR(sfp_rx_los_all,  S_IRUGO, sfp_show_tx_rx_status, NULL, RX_LOS_ALL);
static struct attribute *sfp_msa_attributes[] = {
    &sensor_dev_attr_sfp_port_number.dev_attr.attr,
    &sensor_dev_attr_sfp_is_present.dev_attr.attr,
    &sensor_dev_attr_sfp_is_present_all.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_fault.dev_attr.attr,
    &sensor_dev_attr_sfp_rx_los.dev_attr.attr,
    &sensor_dev_attr_sfp_rx_los_all.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_disable.dev_attr.attr,
    NULL
};

/* Platform dependent +++ */
#define CPLD_PORT_TO_FRONT_PORT(port)  (port+1)

enum port_numbers {
    as7312_54x_port1,  as7312_54x_port2,  as7312_54x_port3,  as7312_54x_port4,
    as7312_54x_port5,  as7312_54x_port6,  as7312_54x_port7,  as7312_54x_port8,
    as7312_54x_port9,  as7312_54x_port10, as7312_54x_port11, as7312_54x_port12,
    as7312_54x_port13, as7312_54x_port14, as7312_54x_port15, as7312_54x_port16,
    as7312_54x_port17, as7312_54x_port18, as7312_54x_port19, as7312_54x_port20,
    as7312_54x_port21, as7312_54x_port22, as7312_54x_port23, as7312_54x_port24,
    as7312_54x_port25, as7312_54x_port26, as7312_54x_port27, as7312_54x_port28,
    as7312_54x_port29, as7312_54x_port30, as7312_54x_port31, as7312_54x_port32,
    as7312_54x_port33, as7312_54x_port34, as7312_54x_port35, as7312_54x_port36,
    as7312_54x_port37, as7312_54x_port38, as7312_54x_port39, as7312_54x_port40,
    as7312_54x_port41, as7312_54x_port42, as7312_54x_port43, as7312_54x_port44,
    as7312_54x_port45, as7312_54x_port46, as7312_54x_port47, as7312_54x_port48,
    as7312_54x_port49, as7312_54x_port52, as7312_54x_port50, as7312_54x_port53,
    as7312_54x_port51, as7312_54x_port54
};

#define I2C_DEV_ID(x) { #x, x}

static const struct i2c_device_id sfp_device_id[] = {
    I2C_DEV_ID(as7312_54x_port1),
    I2C_DEV_ID(as7312_54x_port2),
    I2C_DEV_ID(as7312_54x_port3),
    I2C_DEV_ID(as7312_54x_port4),
    I2C_DEV_ID(as7312_54x_port5),
    I2C_DEV_ID(as7312_54x_port6),
    I2C_DEV_ID(as7312_54x_port7),
    I2C_DEV_ID(as7312_54x_port8),
    I2C_DEV_ID(as7312_54x_port9),
    I2C_DEV_ID(as7312_54x_port10),
    I2C_DEV_ID(as7312_54x_port11),
    I2C_DEV_ID(as7312_54x_port12),
    I2C_DEV_ID(as7312_54x_port13),
    I2C_DEV_ID(as7312_54x_port14),
    I2C_DEV_ID(as7312_54x_port15),
    I2C_DEV_ID(as7312_54x_port16),
    I2C_DEV_ID(as7312_54x_port17),
    I2C_DEV_ID(as7312_54x_port18),
    I2C_DEV_ID(as7312_54x_port19),
    I2C_DEV_ID(as7312_54x_port20),
    I2C_DEV_ID(as7312_54x_port21),
    I2C_DEV_ID(as7312_54x_port22),
    I2C_DEV_ID(as7312_54x_port23),
    I2C_DEV_ID(as7312_54x_port24),
    I2C_DEV_ID(as7312_54x_port25),
    I2C_DEV_ID(as7312_54x_port26),
    I2C_DEV_ID(as7312_54x_port27),
    I2C_DEV_ID(as7312_54x_port28),
    I2C_DEV_ID(as7312_54x_port29),
    I2C_DEV_ID(as7312_54x_port30),
    I2C_DEV_ID(as7312_54x_port31),
    I2C_DEV_ID(as7312_54x_port32),
    I2C_DEV_ID(as7312_54x_port33),
    I2C_DEV_ID(as7312_54x_port34),
    I2C_DEV_ID(as7312_54x_port35),
    I2C_DEV_ID(as7312_54x_port36),
    I2C_DEV_ID(as7312_54x_port37),
    I2C_DEV_ID(as7312_54x_port38),
    I2C_DEV_ID(as7312_54x_port39),
    I2C_DEV_ID(as7312_54x_port40),
    I2C_DEV_ID(as7312_54x_port41),
    I2C_DEV_ID(as7312_54x_port42),
    I2C_DEV_ID(as7312_54x_port43),
    I2C_DEV_ID(as7312_54x_port44),
    I2C_DEV_ID(as7312_54x_port45),
    I2C_DEV_ID(as7312_54x_port46),
    I2C_DEV_ID(as7312_54x_port47),
    I2C_DEV_ID(as7312_54x_port48),
    I2C_DEV_ID(as7312_54x_port49),
    I2C_DEV_ID(as7312_54x_port50),
    I2C_DEV_ID(as7312_54x_port51),
    I2C_DEV_ID(as7312_54x_port52),
    I2C_DEV_ID(as7312_54x_port53),
    I2C_DEV_ID(as7312_54x_port54),
    { /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, sfp_device_id);
/* Platform dependent --- */

enum driver_type_e {
    DRIVER_TYPE_SFP_MSA,
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
#if (MULTIPAGE_SUPPORT == 1)
    struct i2c_client	   *ddm_client;	/* dummy client instance for 0xA2 */
#endif
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
    u64					   present;		/* present status, bit0:port0, bit1:port1 and so on */

    struct sfp_msa_data	  *msa;
    struct qsfp_data	  *qsfp;

    struct i2c_client	  *client;
#if (MULTIPAGE_SUPPORT == 1)
    int use_smbus;
    u8 *writebuf;
    unsigned write_max;
#endif
};

#if (MULTIPAGE_SUPPORT == 1)
static ssize_t sfp_port_read_write(struct sfp_port_data *port_data,
                                   char *buf, loff_t off, size_t len, qsfp_opcode_e opcode);
#endif
static ssize_t show_port_number(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sfp_port_data *data = i2c_get_clientdata(client);
    return sprintf(buf, "%d\n", CPLD_PORT_TO_FRONT_PORT(data->port));
}

/* Platform dependent +++ */
static struct sfp_port_data *sfp_update_present(struct i2c_client *client)
{
    int i = 0, j = 0, status = -1;
    u8 reg;
    unsigned short cpld_addr;
    struct sfp_port_data *data = i2c_get_clientdata(client);

    DEBUG_PRINT("Starting sfp present status update");
    mutex_lock(&data->update_lock);
    data->present = 0;

    /* Read present status of port 1~48(SFP port) */
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            cpld_addr 	= I2C_ADDR_CPLD2 + i*2;
            reg	   		= 0x9+j;
            status		= accton_i2c_cpld_read(cpld_addr, reg);

            if (unlikely(status < 0)) {
                dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_addr, reg, status);
                goto exit;
            }

            DEBUG_PRINT("Present status = 0x%lx\r\n", data->present);
            data->present |= (u64)status << ((i*24) + (j%3)*8);
        }
    }

    /* Read present status of port 49-52(QSFP port) */
    cpld_addr = I2C_ADDR_CPLD2;
    reg 	  = 0x18;
    status 	  = accton_i2c_cpld_read(cpld_addr, reg);

    if (unlikely(status < 0)) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_addr, reg, status);
        goto exit;
    }
    else {
        data->present |= (u64)(status & 0xF) << SFP_PORT_MAX;
    }

    /* Read present status of port 53-54(QSFP port) */
    cpld_addr = I2C_ADDR_CPLD3;
    reg 	  = 0x18;
    status 	  = accton_i2c_cpld_read(cpld_addr, reg);

    if (unlikely(status < 0)) {
        dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_addr, reg, status);
        goto exit;
    }
    else {
        data->present |= (u64)(status & 0x3) << 52;
    }

    DEBUG_PRINT("Present status = 0x%lx", data->present);
exit:
    mutex_unlock(&data->update_lock);
    return (status < 0) ? ERR_PTR(status) : data;
}

static struct sfp_port_data* sfp_update_tx_rx_status(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sfp_port_data *data = i2c_get_clientdata(client);
    int i = 0, j = 0;
    int status = -1;

    if (time_before(jiffies, data->msa->last_updated + HZ + HZ / 2) && data->msa->valid) {
        return data;
    }

    DEBUG_PRINT("Starting as7312_54x sfp tx rx status update");
    mutex_lock(&data->update_lock);
    data->msa->valid = 0;
    memset(data->msa->status, 0, sizeof(data->msa->status));

    /* Read status of port 1~48(SFP port) */
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 9; j++) {
            u8 reg;
            unsigned short cpld_addr;
            reg 	  = 0xc+j;
            cpld_addr = I2C_ADDR_CPLD2 + i*2;

            status	= accton_i2c_cpld_read(cpld_addr, reg);
            if (unlikely(status < 0)) {
                dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", cpld_addr, reg, status);
                goto exit;
            }

            data->msa->status[j/3] |= (u64)status << ((i*24) + (j%3)*8);
        }
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
    unsigned short cpld_addr = 0;
    u8 cpld_reg = 0, cpld_val = 0, cpld_bit = 0;
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

    if(data->port < 24) {
        cpld_addr = I2C_ADDR_CPLD2;
        cpld_reg  = 0xF + data->port / 8;
        cpld_bit  = 1 << (data->port % 8);
    }
    else { /* port 24 ~ 48 */
        cpld_addr = I2C_ADDR_CPLD3;
        cpld_reg  = 0xF + (data->port - 24) / 8;
        cpld_bit  = 1 << (data->port % 8);
    }

    /* Read current status */
    cpld_val = accton_i2c_cpld_read(cpld_addr, cpld_reg);

    /* Update tx_disable status */
    if (disable) {
        data->msa->status[1] |= BIT_INDEX(data->port);
        cpld_val |= cpld_bit;
    }
    else {
        data->msa->status[1] &= ~BIT_INDEX(data->port);
        cpld_val &= ~cpld_bit;
    }

    accton_i2c_cpld_write(cpld_addr, cpld_reg, cpld_val);
    mutex_unlock(&data->update_lock);
    return count;
}

static int sfp_is_port_present(struct i2c_client *client, int port)
{
    struct sfp_port_data *data = i2c_get_clientdata(client);

    data = sfp_update_present(client);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

    return (data->present & BIT_INDEX(data->port)) ? 0 : 1; /* Platform dependent */
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

        /* Return values 1 -> 54 in order */
        return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
                       values[0], values[1], values[2],
                       values[3], values[4], values[5],
                       values[6] & 0x3F);
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

static struct sfp_port_data *qsfp_update_tx_rx_status(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sfp_port_data *data = i2c_get_clientdata(client);
    int i, status = -1;
    u8 buf = 0;
    u8 reg[] = {SFF8436_TX_FAULT_ADDR, SFF8436_TX_DISABLE_ADDR, SFF8436_RX_LOS_ADDR};

    if (time_before(jiffies, data->qsfp->last_updated + HZ + HZ / 2) && data->qsfp->valid) {
        return data;
    }

    DEBUG_PRINT("Starting sfp tx rx status update");
    mutex_lock(&data->update_lock);
    data->qsfp->valid = 0;
    memset(data->qsfp->status, 0, sizeof(data->qsfp->status));

    /* Notify device to update tx fault/ tx disable/ rx los status */
    for (i = 0; i < ARRAY_SIZE(reg); i++) {
        status = sfp_eeprom_read(client, reg[i], &buf, sizeof(buf));
        if (unlikely(status < 0)) {
            goto exit;
        }
    }
    msleep(200);

    /* Read actual tx fault/ tx disable/ rx los status */
    for (i = 0; i < ARRAY_SIZE(reg); i++) {
        status = sfp_eeprom_read(client, reg[i], &buf, sizeof(buf));
        if (unlikely(status < 0)) {
            goto exit;
        }

        DEBUG_PRINT("qsfp reg(0x%x) status = (0x%x)", reg[i], data->qsfp->status[i]);
        data->qsfp->status[i] = (buf & 0xF);
    }

    data->qsfp->valid = 1;
    data->qsfp->last_updated = jiffies;

exit:
    mutex_unlock(&data->update_lock);
    return (status < 0) ? ERR_PTR(status) : data;
}

static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da,
                              char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sfp_port_data *data = i2c_get_clientdata(client);
    u8  cpld_val = 0;
    int port_bit;
    int status = -EINVAL;
    u8 cpld_addr[] = {I2C_ADDR_CPLD2, I2C_ADDR_CPLD3};

    /* Low power mode is not supported for SFP ports(1-48) */
    if (data->port < SFP_PORT_MAX) {
        return -EINVAL;
    }
    mutex_lock(&data->update_lock);

    port_bit = data->port - SFP_PORT_MAX;
    cpld_val = accton_i2c_cpld_read(cpld_addr[port_bit/4], CPLD3_OFFSET_QSFP_MOD_RST);

    pr_err("[ROY]%s#%d, %x from %x\n", __func__, __LINE__, cpld_val, cpld_addr[port_bit/4]);

    cpld_val = cpld_val & 0x0F;
    cpld_val = cpld_val & BIT_INDEX(port_bit%4);

    pr_err("[ROY]%s#%d, %x of bit %d\n", __func__, __LINE__, cpld_val, port_bit);

    status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", cpld_val>>(port_bit%4));

    mutex_unlock(&data->update_lock);

    return status;
}

static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
                              const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sfp_port_data *data = i2c_get_clientdata(client);
    u8 cpld_val = 0;
    long reset;
    int error, port_bit;
    u8 cpld_addr[] = {I2C_ADDR_CPLD2, I2C_ADDR_CPLD3};

    pr_err("[ROY]%s#%d, port:%d\n", __func__, __LINE__, data->port);

    /* Tx disable is not supported for QSFP ports(49-54) */
    if (data->port < SFP_PORT_MAX) {
        return -EINVAL;
    }
    port_bit = data->port - SFP_PORT_MAX;
    error = kstrtol(buf, 10, &reset);

    pr_err("[ROY]%s#%d, %s == %d\n", __func__, __LINE__, buf, error);
    if (error) {
        return error;
    }
    mutex_lock(&data->update_lock);

    cpld_val = accton_i2c_cpld_read(cpld_addr[port_bit/4], CPLD3_OFFSET_QSFP_MOD_RST);
    pr_err("[ROY]%s#%d, %x\n", __func__, __LINE__, cpld_val);
    /* Update lp_mode status */
    if (reset)
    {
        cpld_val |= BIT_INDEX(port_bit%4);
    }
    else
    {
        cpld_val &= ~BIT_INDEX(port_bit%4);
    }
    pr_err("[ROY]%s#%d, %x to %x\n", __func__, __LINE__, cpld_val, cpld_addr[port_bit/4]);

    accton_i2c_cpld_write(cpld_addr[port_bit/4], CPLD3_OFFSET_QSFP_MOD_RST, cpld_val);

    mutex_unlock(&data->update_lock);

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

    present = sfp_is_port_present(client, data->port);
    if (IS_ERR_VALUE(present)) {
        return present;
    }

    if (present == 0) {
        /* port is not present */
        return -ENXIO;
    }

    data = qsfp_update_tx_rx_status(dev);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

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
        if (disable) {
            data->qsfp->status[1] |= 0xF;
        }
        else {
            data->qsfp->status[1] &= ~0xF;
        }
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

/* Platform dependent +++ */
static ssize_t sfp_show_tx_rx_status(struct device *dev, struct device_attribute *da,
                                     char *buf)
{
    u8 val = 0, index = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct sfp_port_data *data = i2c_get_clientdata(client);

    if (data->driver_type == DRIVER_TYPE_QSFP) {
        return qsfp_show_tx_rx_status(dev, da, buf);
    }

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
        return 0;
    }

    val = (data->msa->status[index] & BIT_INDEX(data->port)) ? 1 : 0;
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

#if (MULTIPAGE_SUPPORT == 0)
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
#endif

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

#if (MULTIPAGE_SUPPORT == 1)
    return sfp_port_read_write(data, buf, off, count, QSFP_WRITE_OP);
#else
    return sfp_port_write(data, buf, off, count);
#endif
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

#if (MULTIPAGE_SUPPORT == 1)
/*-------------------------------------------------------------------------*/
/*
 * This routine computes the addressing information to be used for
 * a given r/w request.
 *
 * Task is to calculate the client (0 = i2c addr 50, 1 = i2c addr 51),
 * the page, and the offset.
 *
 * Handles both SFP and QSFP.
 *     For SFP, offset 0-255 are on client[0], >255 is on client[1]
 *     Offset 256-383 are on the lower half of client[1]
 *     Pages are accessible on the upper half of client[1].
 *     Offset >383 are in 128 byte pages mapped into the upper half
 *
 *     For QSFP, all offsets are on client[0]
 *     offset 0-127 are on the lower half of client[0] (no paging)
 *     Pages are accessible on the upper half of client[1].
 *     Offset >127 are in 128 byte pages mapped into the upper half
 *
 *     Callers must not read/write beyond the end of a client or a page
 *     without recomputing the client/page.  Hence offset (within page)
 *     plus length must be less than or equal to 128.  (Note that this
 *     routine does not have access to the length of the call, hence
 *     cannot do the validity check.)
 *
 * Offset within Lower Page 00h and Upper Page 00h are not recomputed
 */
static uint8_t sff_8436_translate_offset(struct sfp_port_data *port_data,
        loff_t *offset, struct i2c_client **client)
{
    unsigned page = 0;

    *client = port_data->client;

    /* if SFP style, offset > 255, shift to i2c addr 0x51 */
    if (port_data->driver_type == DRIVER_TYPE_SFP_MSA) {
        if (*offset > 255) {
            /* like QSFP, but shifted to client[1] */
            *client = port_data->msa->ddm_client;
            *offset -= 256;
        }
    }

    /*
     * if offset is in the range 0-128...
     * page doesn't matter (using lower half), return 0.
     * offset is already correct (don't add 128 to get to paged area)
     */
    if (*offset < SFF_8436_PAGE_SIZE)
        return page;

    /* note, page will always be positive since *offset >= 128 */
    page = (*offset >> 7)-1;
    /* 0x80 places the offset in the top half, offset is last 7 bits */
    *offset = SFF_8436_PAGE_SIZE + (*offset & 0x7f);

    return page;  /* note also returning client and offset */
}

static ssize_t sff_8436_eeprom_read(struct sfp_port_data *port_data,
                                    struct i2c_client *client,
                                    char *buf, unsigned offset, size_t count)
{
    struct i2c_msg msg[2];
    u8 msgbuf[2];
    unsigned long timeout, read_time;
    int status, i;

    memset(msg, 0, sizeof(msg));

    switch (port_data->use_smbus) {
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
         * io_limit data bytes.  msgbuf is u8 and will cast to our
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

        switch (port_data->use_smbus) {
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

        if (status == count)  /* happy path */
            return count;

        if (status == -ENXIO) /* no module present */
            return status;

        /* REVISIT: at HZ=100, this is sloooow */
        msleep(1);
    } while (time_before(read_time, timeout));

    return -ETIMEDOUT;
}

static ssize_t sff_8436_eeprom_write(struct sfp_port_data *port_data,
                                     struct i2c_client *client,
                                     const char *buf,
                                     unsigned offset, size_t count)
{
    struct i2c_msg msg;
    ssize_t status;
    unsigned long timeout, write_time;
    unsigned next_page_start;
    int i = 0;

    /* write max is at most a page
     * (In this driver, write_max is actually one byte!)
     */
    if (count > port_data->write_max)
        count = port_data->write_max;

    /* shorten count if necessary to avoid crossing page boundary */
    next_page_start = roundup(offset + 1, SFF_8436_PAGE_SIZE);
    if (offset + count > next_page_start)
        count = next_page_start - offset;

    switch (port_data->use_smbus) {
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
        msg.buf = port_data->writebuf;

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

        switch (port_data->use_smbus) {
        case I2C_SMBUS_I2C_BLOCK_DATA:
            status = i2c_smbus_write_i2c_block_data(client,
                                                    offset, count, buf);
            if (status == 0)
                status = count;
            break;
        case I2C_SMBUS_WORD_DATA:
            if (count == 2) {
                status = i2c_smbus_write_word_data(client,
                                                   offset, (u16)((buf[0])|(buf[1] << 8)));
            } else {
                /* count = 1 */
                status = i2c_smbus_write_byte_data(client,
                                                   offset, buf[0]);
            }
            if (status == 0)
                status = count;
            break;
        case I2C_SMBUS_BYTE_DATA:
            status = i2c_smbus_write_byte_data(client, offset,
                                               buf[0]);
            if (status == 0)
                status = count;
            break;
        default:
            status = i2c_transfer(client->adapter, &msg, 1);
            if (status == 1)
                status = count;
            break;
        }

        dev_dbg(&client->dev, "eeprom write %zu@%d --> %ld (%lu)\n",
                count, offset, (long int) status, jiffies);

        if (status == count)
            return count;

        /* REVISIT: at HZ=100, this is sloooow */
        msleep(1);
    } while (time_before(write_time, timeout));

    return -ETIMEDOUT;
}


static ssize_t sff_8436_eeprom_update_client(struct sfp_port_data *port_data,
        char *buf, loff_t off,
        size_t count, qsfp_opcode_e opcode)
{
    struct i2c_client *client;
    ssize_t retval = 0;
    u8 page = 0;
    loff_t phy_offset = off;
    int ret = 0;

    page = sff_8436_translate_offset(port_data, &phy_offset, &client);

    dev_dbg(&client->dev,
            "sff_8436_eeprom_update_client off %lld  page:%d phy_offset:%lld, count:%ld, opcode:%d\n",
            off, page, phy_offset, (long int) count, opcode);
    if (page > 0) {
        ret = sff_8436_eeprom_write(port_data, client, &page,
                                    SFF_8436_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            dev_dbg(&client->dev,
                    "Write page register for page %d failed ret:%d!\n",
                    page, ret);
            return ret;
        }
    }

    while (count) {
        ssize_t	status;

        if (opcode == QSFP_READ_OP) {
            status =  sff_8436_eeprom_read(port_data, client,
                                           buf, phy_offset, count);
        } else {
            status =  sff_8436_eeprom_write(port_data, client,
                                            buf, phy_offset, count);
        }
        if (status <= 0) {
            if (retval == 0)
                retval = status;
            break;
        }
        buf += status;
        phy_offset += status;
        count -= status;
        retval += status;
    }


    if (page > 0) {
        /* return the page register to page 0 (why?) */
        page = 0;
        ret = sff_8436_eeprom_write(port_data, client, &page,
                                    SFF_8436_PAGE_SELECT_REG, 1);
        if (ret < 0) {
            dev_err(&client->dev,
                    "Restore page register to page %d failed ret:%d!\n",
                    page, ret);
            return ret;
        }
    }
    return retval;
}


/*
 * Figure out if this access is within the range of supported pages.
 * Note this is called on every access because we don't know if the
 * module has been replaced since the last call.
 * If/when modules support more pages, this is the routine to update
 * to validate and allow access to additional pages.
 *
 * Returns updated len for this access:
 *     - entire access is legal, original len is returned.
 *     - access begins legal but is too long, len is truncated to fit.
 *     - initial offset exceeds supported pages, return -EINVAL
 */
static ssize_t sff_8436_page_legal(struct sfp_port_data *port_data,
                                   loff_t off, size_t len)
{
    struct i2c_client *client = port_data->client;
    u8 regval;
    int status;
    size_t maxlen;

    if (off < 0) return -EINVAL;
    if (port_data->driver_type == DRIVER_TYPE_SFP_MSA) {
        /* SFP case */
        if ((off + len) <= 256) return len;
        /* if no pages needed, we're good */
        //if ((off + len) <= SFF_8472_EEPROM_UNPAGED_SIZE) return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= SFF_8472_EEPROM_SIZE) return -EINVAL;

        /* Check if ddm is supported */
        status = sff_8436_eeprom_read(port_data, client, &regval,
                                      SFF8472_DIAG_MON_TYPE_ADDR, 1);
        if (status < 0) return status;  /* error out (no module?) */
        if (!(regval & SFF8472_DIAG_MON_TYPE_DDM_MASK)) {
            if (off >= 256) return -EINVAL;
            maxlen = 256 - off;
        }
        else {
            /* in between, are pages supported? */
            status = sff_8436_eeprom_read(port_data, client, &regval,
                                          SFF_8472_PAGEABLE_REG, 1);
            if (status < 0) return status;  /* error out (no module?) */
            if (regval & SFF_8472_PAGEABLE) {
                /* Pages supported, trim len to the end of pages */
                maxlen = SFF_8472_EEPROM_SIZE - off;
            } else {
                /* pages not supported, trim len to unpaged size */
                if (off >= SFF_8472_EEPROM_UNPAGED_SIZE) return -EINVAL;
                maxlen = SFF_8472_EEPROM_UNPAGED_SIZE - off;
            }
        }
        len = (len > maxlen) ? maxlen : len;
        dev_dbg(&client->dev,
                "page_legal, SFP, off %lld len %ld\n",
                off, (long int) len);
    }
    else if (port_data->driver_type == DRIVER_TYPE_QSFP) {
        /* QSFP case */
        /* if no pages needed, we're good */
        if ((off + len) <= SFF_8436_EEPROM_UNPAGED_SIZE) return len;
        /* if offset exceeds possible pages, we're not good */
        if (off >= SFF_8436_EEPROM_SIZE) return -EINVAL;
        /* in between, are pages supported? */
        status = sff_8436_eeprom_read(port_data, client, &regval,
                                      SFF_8436_PAGEABLE_REG, 1);
        if (status < 0) return status;  /* error out (no module?) */
        if (regval & SFF_8436_NOT_PAGEABLE) {
            /* pages not supported, trim len to unpaged size */
            if (off >= SFF_8436_EEPROM_UNPAGED_SIZE) return -EINVAL;
            maxlen = SFF_8436_EEPROM_UNPAGED_SIZE - off;
        } else {
            /* Pages supported, trim len to the end of pages */
            maxlen = SFF_8436_EEPROM_SIZE - off;
        }
        len = (len > maxlen) ? maxlen : len;
        dev_dbg(&client->dev,
                "page_legal, QSFP, off %lld len %ld\n",
                off, (long int) len);
    }
    else {
        return -EINVAL;
    }
    return len;
}


static ssize_t sfp_port_read_write(struct sfp_port_data *port_data,
                                   char *buf, loff_t off, size_t len, qsfp_opcode_e opcode)
{
    struct i2c_client *client = port_data->client;
    int chunk;
    int status = 0;
    ssize_t retval;
    size_t pending_len = 0, chunk_len = 0;
    loff_t chunk_offset = 0, chunk_start_offset = 0;

    if (unlikely(!len))
        return len;

    /*
     * Read data from chip, protecting against concurrent updates
     * from this host, but not from other I2C masters.
     */
    mutex_lock(&port_data->update_lock);

    /*
     * Confirm this access fits within the device suppored addr range
     */
    len = sff_8436_page_legal(port_data, off, len);
    if (len < 0) {
        status = len;
        goto err;
    }

    /*
     * For each (128 byte) chunk involved in this request, issue a
     * separate call to sff_eeprom_update_client(), to
     * ensure that each access recalculates the client/page
     * and writes the page register as needed.
     * Note that chunk to page mapping is confusing, is different for
     * QSFP and SFP, and never needs to be done.  Don't try!
     */
    pending_len = len; /* amount remaining to transfer */
    retval = 0;  /* amount transferred */
    for (chunk = off >> 7; chunk <= (off + len - 1) >> 7; chunk++) {

        /*
         * Compute the offset and number of bytes to be read/write
         *
         * 1. start at offset 0 (within the chunk), and read/write
         *    the entire chunk
         * 2. start at offset 0 (within the chunk) and read/write less
         *    than entire chunk
         * 3. start at an offset not equal to 0 and read/write the rest
         *    of the chunk
         * 4. start at an offset not equal to 0 and read/write less than
         *    (end of chunk - offset)
         */
        chunk_start_offset = chunk * SFF_8436_PAGE_SIZE;

        if (chunk_start_offset < off) {
            chunk_offset = off;
            if ((off + pending_len) < (chunk_start_offset +
                                       SFF_8436_PAGE_SIZE))
                chunk_len = pending_len;
            else
                chunk_len = (chunk+1)*SFF_8436_PAGE_SIZE - off;/*SFF_8436_PAGE_SIZE - off;*/
        } else {
            chunk_offset = chunk_start_offset;
            if (pending_len > SFF_8436_PAGE_SIZE)
                chunk_len = SFF_8436_PAGE_SIZE;
            else
                chunk_len = pending_len;
        }

        dev_dbg(&client->dev,
                "sff_r/w: off %lld, len %ld, chunk_start_offset %lld, chunk_offset %lld, chunk_len %ld, pending_len %ld\n",
                off, (long int) len, chunk_start_offset, chunk_offset,
                (long int) chunk_len, (long int) pending_len);

        /*
         * note: chunk_offset is from the start of the EEPROM,
         * not the start of the chunk
         */
        status = sff_8436_eeprom_update_client(port_data, buf,
                                               chunk_offset, chunk_len, opcode);
        if (status != chunk_len) {
            /* This is another 'no device present' path */
            dev_dbg(&client->dev,
                    "sff_8436_update_client for chunk %d chunk_offset %lld chunk_len %ld failed %d!\n",
                    chunk, chunk_offset, (long int) chunk_len, status);
            goto err;
        }
        buf += status;
        pending_len -= status;
        retval += status;
    }
    mutex_unlock(&port_data->update_lock);

    return retval;

err:
    mutex_unlock(&port_data->update_lock);

    return status;
}

#else
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
#endif

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

#if (MULTIPAGE_SUPPORT == 1)
    return sfp_port_read_write(data, buf, off, count, QSFP_READ_OP);
#else
    return sfp_port_read(data, buf, off, count);
#endif
}

#if (MULTIPAGE_SUPPORT == 1)
static int sfp_sysfs_eeprom_init(struct kobject *kobj, struct bin_attribute *eeprom, size_t size)
#else
static int sfp_sysfs_eeprom_init(struct kobject *kobj, struct bin_attribute *eeprom)
#endif
{
    int err;

    sysfs_bin_attr_init(eeprom);
    eeprom->attr.name = EEPROM_NAME;
    eeprom->attr.mode = S_IWUSR | S_IRUGO;
    eeprom->read	  = sfp_bin_read;
    eeprom->write	  = sfp_bin_write;
#if (MULTIPAGE_SUPPORT == 1)
    eeprom->size	  = size;
#else
    eeprom->size	  = EEPROM_SIZE;
#endif

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


#if (MULTIPAGE_SUPPORT == 0)
static int sfp_i2c_check_functionality(struct i2c_client *client)
{
#if USE_I2C_BLOCK_READ
    return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK);
#else
    return i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA);
#endif
}
#endif

static const struct attribute_group sfp_msa_group = {
    .attrs = sfp_msa_attributes,
};

static int sfp_msa_probe(struct i2c_client *client, const struct i2c_device_id *dev_id,
                         struct sfp_msa_data **data)
{
    int status;
    struct sfp_msa_data *msa;

#if (MULTIPAGE_SUPPORT == 0)
    if (!sfp_i2c_check_functionality(client)) {
        status = -EIO;
        goto exit;
    }
#endif

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
#if (MULTIPAGE_SUPPORT == 1)
    status = sfp_sysfs_eeprom_init(&client->dev.kobj, &msa->eeprom.bin, SFF_8436_EEPROM_SIZE);
#else
    status = sfp_sysfs_eeprom_init(&client->dev.kobj, &msa->eeprom.bin);
#endif
    if (status) {
        goto exit_remove;
    }

#if (MULTIPAGE_SUPPORT == 1)
    msa->ddm_client = i2c_new_dummy(client->adapter, client->addr + 1);
    if (!msa->ddm_client) {
        dev_err(&client->dev, "address 0x%02x unavailable\n", client->addr + 1);
        status = -EADDRINUSE;
        goto exit_eeprom;
    }
#endif

    *data = msa;
    dev_info(&client->dev, "sfp msa '%s'\n", client->name);

    return 0;

exit_eeprom:
    sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &msa->eeprom.bin);
exit_remove:
    sysfs_remove_group(&client->dev.kobj, &sfp_msa_group);
exit_free:
    kfree(msa);
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

#if (MULTIPAGE_SUPPORT == 0)
    if (!sfp_i2c_check_functionality(client)) {
        status = -EIO;
        goto exit;
    }
#endif

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
#if (MULTIPAGE_SUPPORT == 1)
    status = sfp_sysfs_eeprom_init(&client->dev.kobj, &qsfp->eeprom.bin, SFF_8436_EEPROM_SIZE);
#else
    status = sfp_sysfs_eeprom_init(&client->dev.kobj, &qsfp->eeprom.bin);
#endif
    if (status) {
        goto exit_remove;
    }

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
    int ret = 0;
    struct sfp_port_data *data = NULL;

    if (client->addr != SFP_EEPROM_A0_I2C_ADDR) {
        return -ENODEV;
    }

    if (dev_id->driver_data < as7312_54x_port1 || dev_id->driver_data > as7312_54x_port54) {
        return -ENXIO;
    }

    data = kzalloc(sizeof(struct sfp_port_data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

#if (MULTIPAGE_SUPPORT == 1)
    data->use_smbus = 0;

    /* Use I2C operations unless we're stuck with SMBus extensions. */
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        if (i2c_check_functionality(client->adapter,
                                    I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
            data->use_smbus = I2C_SMBUS_I2C_BLOCK_DATA;
        } else if (i2c_check_functionality(client->adapter,
                                           I2C_FUNC_SMBUS_READ_WORD_DATA)) {
            data->use_smbus = I2C_SMBUS_WORD_DATA;
        } else if (i2c_check_functionality(client->adapter,
                                           I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
            data->use_smbus = I2C_SMBUS_BYTE_DATA;
        } else {
            ret = -EPFNOSUPPORT;
            goto exit_kfree;
        }
    }

    if (!data->use_smbus ||
            (i2c_check_functionality(client->adapter,
                                     I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) ||
            i2c_check_functionality(client->adapter,
                                    I2C_FUNC_SMBUS_WRITE_WORD_DATA) ||
            i2c_check_functionality(client->adapter,
                                    I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
        /*
         * NOTE: AN-2079
         * Finisar recommends that the host implement 1 byte writes
         * only since this module only supports 32 byte page boundaries.
         * 2 byte writes are acceptable for PE and Vout changes per
         * Application Note AN-2071.
         */
        unsigned write_max = 1;

        if (write_max > io_limit)
            write_max = io_limit;
        if (data->use_smbus && write_max > I2C_SMBUS_BLOCK_MAX)
            write_max = I2C_SMBUS_BLOCK_MAX;
        data->write_max = write_max;

        /* buffer (data + address at the beginning) */
        data->writebuf = kmalloc(write_max + 2, GFP_KERNEL);
        if (!data->writebuf) {
            ret = -ENOMEM;
            goto exit_kfree;
        }
    } else {
        dev_warn(&client->dev,
                 "cannot write due to controller restrictions.");
    }

    if (data->use_smbus == I2C_SMBUS_WORD_DATA ||
            data->use_smbus == I2C_SMBUS_BYTE_DATA) {
        dev_notice(&client->dev, "Falling back to %s reads, "
                   "performance will suffer\n", data->use_smbus ==
                   I2C_SMBUS_WORD_DATA ? "word" : "byte");
    }
#endif

    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->port	 = dev_id->driver_data;
    data->client = client;

    if (dev_id->driver_data >= as7312_54x_port1 && dev_id->driver_data <= as7312_54x_port48) {
        data->driver_type = DRIVER_TYPE_SFP_MSA;
        ret = sfp_msa_probe(client, dev_id, &data->msa);
    }
    else { /* as7312_54x_portsfp49 ~ as7312_54x_portsfp54 */
        data->driver_type = DRIVER_TYPE_QSFP;
        ret =  qsfp_probe(client, dev_id, &data->qsfp);
    }

    if (ret < 0) {
        goto exit_kfree_buf;
    }


    return ret;

exit_kfree_buf:
#if (MULTIPAGE_SUPPORT == 1)
    if (data->writebuf) kfree(data->writebuf);
#endif

exit_kfree:
    kfree(data);
    return ret;
}
/* Platform dependent --- */

static int sfp_msa_remove(struct i2c_client *client, struct sfp_msa_data *data)
{
    sfp_sysfs_eeprom_cleanup(&client->dev.kobj, &data->eeprom.bin);
#if (MULTIPAGE_SUPPORT == 1)
    i2c_unregister_device(data->ddm_client);
#endif
    sysfs_remove_group(&client->dev.kobj, &sfp_msa_group);
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

static void sfp_device_remove(struct i2c_client *client)
{
    int ret = 0;
    struct sfp_port_data *data = i2c_get_clientdata(client);

    switch (data->driver_type) {
    case DRIVER_TYPE_SFP_MSA:
        return sfp_msa_remove(client, data->msa);
    case DRIVER_TYPE_QSFP:
        return qfp_remove(client, data->qsfp);
    }

#if (MULTIPAGE_SUPPORT == 1)
    if (data->writebuf)
        kfree(data->writebuf);
#endif
    kfree(data);
}

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static struct i2c_driver sfp_driver = {
    .driver = {
        .name	  = DRIVER_NAME,
    },
    .probe		  = sfp_device_probe,
    .remove		  = sfp_device_remove,
    .id_table	  = sfp_device_id,
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
MODULE_DESCRIPTION("accton as7312_54x_sfp driver");
MODULE_LICENSE("GPL");

module_init(sfp_init);
module_exit(sfp_exit);

