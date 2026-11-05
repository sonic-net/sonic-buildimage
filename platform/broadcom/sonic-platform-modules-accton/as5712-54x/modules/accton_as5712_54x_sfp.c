/*
 * An hwmon driver for accton as5712_54x sfp
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
#if 0
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define NUM_OF_SFF_PORT 54
#define SFP_PORT_MAX    48
#define I2C_ADDR_CPLD1	0x60
#define I2C_ADDR_CPLD2	0x61
#define I2C_ADDR_CPLD3	0x62
#define CPLD3_OFFSET_QSFP_MOD_RST 0x15
#define CPLD3_OFFSET_QSFP_LPMODE  0x16


#define BIT_INDEX(i) (1ULL << (i))

#if 0
static ssize_t show_status(struct device *dev, struct device_attribute *da,char *buf);
static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_port_number(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_eeprom(struct device *dev, struct device_attribute *da, char *buf);
static int as5712_54x_sfp_read_block(struct i2c_client *client, u8 command, u8 *data,int data_len);
extern int as5712_54x_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as5712_54x_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);
#endif

/* Addresses scanned
 */
static const unsigned short normal_i2c[] = { 0x50, I2C_CLIENT_END };

/* Each client has this additional data
 */
struct as5712_54x_sfp_data {
    struct device      *hwmon_dev;
    struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
    int                 port;            /* Front port index */
    char                eeprom[256];     /* eeprom data */
    u64                 status[4];       /* bit0:port0, bit1:port1 and so on */
                                         /* index 0 => is_present
                                                  1 => tx_fail
                                                  2 => tx_disable
                                                  3 => rx_loss */
};

/* The table maps active port to cpld port.
 * Array index 0 is for active port 1,
 * index 1 for active port 2, and so on.
 * The array content implies cpld port index.
 */
static const u8 cpld_to_front_port_table[] =
{ 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
 49, 52, 50, 53, 51, 54};

#define CPLD_PORT_TO_FRONT_PORT(port)  (cpld_to_front_port_table[port])

static struct as5712_54x_sfp_data *as5712_54x_sfp_update_device(struct device *dev, int update_eeprom);
static ssize_t show_port_number(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_status(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t show_eeprom(struct device *dev, struct device_attribute *da, char *buf);
static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t get_lp_mode(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_lp_mode(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);		
static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);								
extern int as5712_54x_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int as5712_54x_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);

enum as5712_54x_sfp_sysfs_attributes {
    SFP_IS_PRESENT,
    SFP_TX_FAULT,
    SFP_TX_DISABLE,
    SFP_RX_LOSS,
    SFP_PORT_NUMBER,
    SFP_EEPROM,
    SFP_RX_LOS_ALL,
    SFP_IS_PRESENT_ALL,
    SFP_LP_MODE,
    SFP_MOD_RST,
};

/* sysfs attributes for hwmon
 */
static SENSOR_DEVICE_ATTR(sfp_is_present,  S_IRUGO, show_status, NULL, SFP_IS_PRESENT);
static SENSOR_DEVICE_ATTR(sfp_tx_fault,    S_IRUGO, show_status, NULL, SFP_TX_FAULT);
static SENSOR_DEVICE_ATTR(sfp_tx_disable,  S_IWUSR | S_IRUGO, show_status, set_tx_disable, SFP_TX_DISABLE);
static SENSOR_DEVICE_ATTR(sfp_rx_loss,     S_IRUGO, show_status,NULL, SFP_RX_LOSS);
static SENSOR_DEVICE_ATTR(sfp_port_number,  S_IRUGO, show_port_number, NULL, SFP_PORT_NUMBER);
static SENSOR_DEVICE_ATTR(sfp_eeprom,      S_IRUGO, show_eeprom, NULL, SFP_EEPROM);
static SENSOR_DEVICE_ATTR(sfp_rx_los_all, S_IRUGO, show_status,NULL, SFP_RX_LOS_ALL);
static SENSOR_DEVICE_ATTR(sfp_is_present_all, S_IRUGO, show_status,NULL, SFP_IS_PRESENT_ALL);
static SENSOR_DEVICE_ATTR(sfp_lp_mode,     S_IWUSR | S_IRUGO, get_lp_mode, set_lp_mode, SFP_LP_MODE);
static SENSOR_DEVICE_ATTR(sfp_mod_rst,     S_IWUSR | S_IRUGO, get_mode_reset, set_mode_reset, SFP_MOD_RST);

static struct attribute *as5712_54x_sfp_attributes[] = {
    &sensor_dev_attr_sfp_is_present.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_fault.dev_attr.attr,
    &sensor_dev_attr_sfp_rx_loss.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_disable.dev_attr.attr,
    &sensor_dev_attr_sfp_eeprom.dev_attr.attr,
    &sensor_dev_attr_sfp_port_number.dev_attr.attr,
    &sensor_dev_attr_sfp_rx_los_all.dev_attr.attr,
    &sensor_dev_attr_sfp_is_present_all.dev_attr.attr,
    &sensor_dev_attr_sfp_lp_mode.dev_attr.attr,
    &sensor_dev_attr_sfp_mod_rst.dev_attr.attr,
    NULL
};

static ssize_t show_port_number(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", CPLD_PORT_TO_FRONT_PORT(data->port));
}

static ssize_t show_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct as5712_54x_sfp_data *data;
    u8 val;
    int values[7];

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

    if(attr->index == SFP_RX_LOS_ALL) {
        /*
         * Report the RX_LOS status for all ports.
         * This does not depend on the currently active SFP selector.
         */

        /* RX_LOS Ports 1-8 */
        VALIDATED_READ(buf, values[0], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2, 0x0F), 0);
        /* RX_LOS Ports 9-16 */
        VALIDATED_READ(buf, values[1], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2, 0x10), 0);
        /* RX_LOS Ports 17-24 */
        VALIDATED_READ(buf, values[2], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2, 0x11), 0);
        /* RX_LOS Ports 25-32 */
        VALIDATED_READ(buf, values[3], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x0F), 0);
        /* RX_LOS Ports 33-40 */
        VALIDATED_READ(buf, values[4], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x10), 0);
        /* RX_LOS Ports 41-48 */
        VALIDATED_READ(buf, values[5], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x11), 0);

        /** Return values 1 -> 48 in order */
        return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x\n",
                       values[0], values[1], values[2],
                       values[3], values[4], values[5]);
    }

    if(attr->index == SFP_IS_PRESENT_ALL) {
        /*
         * Report the SFP_PRESENCE status for all ports.
         * This does not depend on the currently active SFP selector.
         */

        /* SFP_PRESENT Ports 1-8 */
        VALIDATED_READ(buf, values[0], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2, 0x6), 1);
        /* SFP_PRESENT Ports 9-16 */
        VALIDATED_READ(buf, values[1], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2, 0x7), 1);
        /* SFP_PRESENT Ports 17-24 */
        VALIDATED_READ(buf, values[2], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2, 0x8), 1);
        /* SFP_PRESENT Ports 25-32 */
        VALIDATED_READ(buf, values[3], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x6), 1);
        /* SFP_PRESENT Ports 33-40 */
        VALIDATED_READ(buf, values[4], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x7), 1);
        /* SFP_PRESENT Ports 41-48 */
        VALIDATED_READ(buf, values[5], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x8), 1);
        /* QSFP_PRESENT Ports 49-54 */
        VALIDATED_READ(buf, values[6], as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x14), 1);

        /* Return values 1 -> 54 in order */
        return sprintf(buf, "%.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
                       values[0], values[1], values[2],
                       values[3], values[4], values[5],
                       values[6] & 0x3F);
    }
    /*
     * The remaining attributes are gathered on a per-selected-sfp basis.
     */
    data = as5712_54x_sfp_update_device(dev, 0);
    if (attr->index == SFP_IS_PRESENT) {
        val = (data->status[attr->index] & BIT_INDEX(data->port)) ? 0 : 1;
    }
    else {
        val = (data->status[attr->index] & BIT_INDEX(data->port)) ? 1 : 0;
    }

    return sprintf(buf, "%d", val);
}

static ssize_t get_lp_mode(struct device *dev, struct device_attribute *da,
			char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);
    u8  cpld_val = 0;
    int port_bit;
    int status = -EINVAL;    
    
    /* Low power mode is not supported for SFP ports(1-48) */
    if (data->port < SFP_PORT_MAX) {
        return -EINVAL;
    }
    mutex_lock(&data->update_lock);
    
    port_bit = data->port - SFP_PORT_MAX;
    cpld_val = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, CPLD3_OFFSET_QSFP_LPMODE);
    cpld_val = cpld_val & 0x3F;
    cpld_val = cpld_val & BIT_INDEX(port_bit);
    status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", cpld_val>>port_bit);

    mutex_unlock(&data->update_lock);

    return status;
}

static ssize_t set_lp_mode(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);
    u8 cpld_val = 0;
    long mode;
    int error, port_bit;

    /* Tx disable is not supported for QSFP ports(49-54) */
    if (data->port < SFP_PORT_MAX) {
        return -EINVAL;
    }
    port_bit = data->port - SFP_PORT_MAX;
    error = kstrtol(buf, 10, &mode);
    if (error) {
        return error;
    }
    mutex_lock(&data->update_lock);
    
    cpld_val = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, CPLD3_OFFSET_QSFP_LPMODE);
    /* Update lp_mode status */
    if (mode)
    {       
        cpld_val |= BIT_INDEX(port_bit);
    }
    else
    {       
        cpld_val &=~BIT_INDEX(port_bit);
    }
    as5712_54x_i2c_cpld_write(I2C_ADDR_CPLD3, CPLD3_OFFSET_QSFP_LPMODE, cpld_val);

    mutex_unlock(&data->update_lock);

    return count;
}


static ssize_t get_mode_reset(struct device *dev, struct device_attribute *da,
			char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);
    u8  cpld_val = 0;
    int port_bit;
    int status = -EINVAL;    
    
    /* Low power mode is not supported for SFP ports(1-48) */
    if (data->port < SFP_PORT_MAX) {
        return -EINVAL;
    }
    mutex_lock(&data->update_lock);
    
    port_bit = data->port - SFP_PORT_MAX;
    cpld_val = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, CPLD3_OFFSET_QSFP_MOD_RST);
    cpld_val = cpld_val & 0x3F;
    cpld_val = cpld_val & BIT_INDEX(port_bit);
    status = snprintf(buf, PAGE_SIZE - 1, "%d\r\n", cpld_val>>port_bit);

    mutex_unlock(&data->update_lock);

    return status;
}

static ssize_t set_mode_reset(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);
    u8 cpld_val = 0;
    long reset;
    int error, port_bit;

    /* Tx disable is not supported for QSFP ports(49-54) */
    if (data->port < SFP_PORT_MAX) {
        return -EINVAL;
    }
    port_bit = data->port - SFP_PORT_MAX;
    error = kstrtol(buf, 10, &reset);
    if (error) {
        return error;
    }
    mutex_lock(&data->update_lock);
    
    cpld_val = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, CPLD3_OFFSET_QSFP_MOD_RST);
    /* Update lp_mode status */
    if (reset)
    {       
        cpld_val |= BIT_INDEX(port_bit);
    }
    else
    {       
        cpld_val &=~BIT_INDEX(port_bit);
    }
    as5712_54x_i2c_cpld_write(I2C_ADDR_CPLD3, CPLD3_OFFSET_QSFP_MOD_RST, cpld_val);

    mutex_unlock(&data->update_lock);

    return count;
}


static ssize_t set_tx_disable(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);
    unsigned short cpld_addr = 0;
    u8 cpld_reg = 0, cpld_val = 0, cpld_bit = 0;
    long disable;
    int error;

    /* Tx disable is not supported for QSFP ports(49-54) */
    if (data->port >= SFP_PORT_MAX) {
        return -EINVAL;
    }

    error = kstrtol(buf, 10, &disable);
    if (error) {
        return error;
    }

    mutex_lock(&data->update_lock);

    if(data->port < 24) {
        cpld_addr = I2C_ADDR_CPLD2;
        cpld_reg = 0xC + data->port / 8;
        cpld_bit = 1 << (data->port % 8);
    }
    else {
        cpld_addr = I2C_ADDR_CPLD3;
        cpld_reg = 0xC + (data->port - 24) / 8;
        cpld_bit = 1 << (data->port % 8);
    }

    cpld_val = as5712_54x_i2c_cpld_read(cpld_addr, cpld_reg);

    /* Update tx_disable status */
    if (disable) {
        data->status[SFP_TX_DISABLE] |= BIT_INDEX(data->port);
        cpld_val |= cpld_bit;
    }
    else {
        data->status[SFP_TX_DISABLE] &= ~BIT_INDEX(data->port);
        cpld_val &= ~cpld_bit;
    }

    as5712_54x_i2c_cpld_write(cpld_addr, cpld_reg, cpld_val);

    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_eeprom(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct as5712_54x_sfp_data *data = as5712_54x_sfp_update_device(dev, 1);

    if (!data->valid) {
        return 0;
    }

    if ((data->status[SFP_IS_PRESENT] & BIT_INDEX(data->port)) != 0) {
        return 0;
    }

    memcpy(buf, data->eeprom, sizeof(data->eeprom));

    return sizeof(data->eeprom);
}

static const struct attribute_group as5712_54x_sfp_group = {
    .attrs = as5712_54x_sfp_attributes,
};

static int as5712_54x_sfp_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct as5712_54x_sfp_data *data;
    int status;

    extern int platform_accton_as5712_54x(void);
    if(!platform_accton_as5712_54x()) {
        return -ENODEV;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct as5712_54x_sfp_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    mutex_init(&data->update_lock);
    data->port = dev_id->driver_data;
    i2c_set_clientdata(client, data);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &as5712_54x_sfp_group);
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
    sysfs_remove_group(&client->dev.kobj, &as5712_54x_sfp_group);
exit_free:
    kfree(data);
exit:

    return status;
}

static void as5712_54x_sfp_remove(struct i2c_client *client)
{
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &as5712_54x_sfp_group);
    kfree(data);

}

enum port_numbers {
as5712_54x_sfp1,  as5712_54x_sfp2,  as5712_54x_sfp3, as5712_54x_sfp4,
as5712_54x_sfp5,  as5712_54x_sfp6,  as5712_54x_sfp7, as5712_54x_sfp8,
as5712_54x_sfp9,  as5712_54x_sfp10, as5712_54x_sfp11,as5712_54x_sfp12,
as5712_54x_sfp13, as5712_54x_sfp14, as5712_54x_sfp15,as5712_54x_sfp16,
as5712_54x_sfp17, as5712_54x_sfp18, as5712_54x_sfp19,as5712_54x_sfp20,
as5712_54x_sfp21, as5712_54x_sfp22, as5712_54x_sfp23,as5712_54x_sfp24,
as5712_54x_sfp25, as5712_54x_sfp26, as5712_54x_sfp27,as5712_54x_sfp28,
as5712_54x_sfp29, as5712_54x_sfp30, as5712_54x_sfp31,as5712_54x_sfp32,
as5712_54x_sfp33, as5712_54x_sfp34, as5712_54x_sfp35,as5712_54x_sfp36,
as5712_54x_sfp37, as5712_54x_sfp38, as5712_54x_sfp39,as5712_54x_sfp40,
as5712_54x_sfp41, as5712_54x_sfp42, as5712_54x_sfp43,as5712_54x_sfp44,
as5712_54x_sfp45, as5712_54x_sfp46, as5712_54x_sfp47,as5712_54x_sfp48,
as5712_54x_sfp49, as5712_54x_sfp52, as5712_54x_sfp50,as5712_54x_sfp53,
as5712_54x_sfp51, as5712_54x_sfp54
};

static const struct i2c_device_id as5712_54x_sfp_id[] = {
{ "as5712_54x_sfp1",  as5712_54x_sfp1 },  { "as5712_54x_sfp2",  as5712_54x_sfp2 },
{ "as5712_54x_sfp3",  as5712_54x_sfp3 },  { "as5712_54x_sfp4",  as5712_54x_sfp4 },
{ "as5712_54x_sfp5",  as5712_54x_sfp5 },  { "as5712_54x_sfp6",  as5712_54x_sfp6 },
{ "as5712_54x_sfp7",  as5712_54x_sfp7 },  { "as5712_54x_sfp8",  as5712_54x_sfp8 },
{ "as5712_54x_sfp9",  as5712_54x_sfp9 },  { "as5712_54x_sfp10", as5712_54x_sfp10 },
{ "as5712_54x_sfp11", as5712_54x_sfp11 }, { "as5712_54x_sfp12", as5712_54x_sfp12 },
{ "as5712_54x_sfp13", as5712_54x_sfp13 }, { "as5712_54x_sfp14", as5712_54x_sfp14 },
{ "as5712_54x_sfp15", as5712_54x_sfp15 }, { "as5712_54x_sfp16", as5712_54x_sfp16 },
{ "as5712_54x_sfp17", as5712_54x_sfp17 }, { "as5712_54x_sfp18", as5712_54x_sfp18 },
{ "as5712_54x_sfp19", as5712_54x_sfp19 }, { "as5712_54x_sfp20", as5712_54x_sfp20 },
{ "as5712_54x_sfp21", as5712_54x_sfp21 }, { "as5712_54x_sfp22", as5712_54x_sfp22 },
{ "as5712_54x_sfp23", as5712_54x_sfp23 }, { "as5712_54x_sfp24", as5712_54x_sfp24 },
{ "as5712_54x_sfp25", as5712_54x_sfp25 }, { "as5712_54x_sfp26", as5712_54x_sfp26 },
{ "as5712_54x_sfp27", as5712_54x_sfp27 }, { "as5712_54x_sfp28", as5712_54x_sfp28 },
{ "as5712_54x_sfp29", as5712_54x_sfp29 }, { "as5712_54x_sfp30", as5712_54x_sfp30 },
{ "as5712_54x_sfp31", as5712_54x_sfp31 }, { "as5712_54x_sfp32", as5712_54x_sfp32 },
{ "as5712_54x_sfp33", as5712_54x_sfp33 }, { "as5712_54x_sfp34", as5712_54x_sfp34 },
{ "as5712_54x_sfp35", as5712_54x_sfp35 }, { "as5712_54x_sfp36", as5712_54x_sfp36 },
{ "as5712_54x_sfp37", as5712_54x_sfp37 }, { "as5712_54x_sfp38", as5712_54x_sfp38 },
{ "as5712_54x_sfp39", as5712_54x_sfp39 }, { "as5712_54x_sfp40", as5712_54x_sfp40 },
{ "as5712_54x_sfp41", as5712_54x_sfp41 }, { "as5712_54x_sfp42", as5712_54x_sfp42 },
{ "as5712_54x_sfp43", as5712_54x_sfp43 }, { "as5712_54x_sfp44", as5712_54x_sfp44 },
{ "as5712_54x_sfp45", as5712_54x_sfp45 }, { "as5712_54x_sfp46", as5712_54x_sfp46 },
{ "as5712_54x_sfp47", as5712_54x_sfp47 }, { "as5712_54x_sfp48", as5712_54x_sfp48 },
{ "as5712_54x_sfp49", as5712_54x_sfp49 }, { "as5712_54x_sfp50", as5712_54x_sfp50 },
{ "as5712_54x_sfp51", as5712_54x_sfp51 }, { "as5712_54x_sfp52", as5712_54x_sfp52 },
{ "as5712_54x_sfp53", as5712_54x_sfp53 }, { "as5712_54x_sfp54", as5712_54x_sfp54 },

{}
};
MODULE_DEVICE_TABLE(i2c, as5712_54x_sfp_id);

static struct i2c_driver as5712_54x_sfp_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "as5712_54x_sfp",
    },
    .probe        = as5712_54x_sfp_probe,
    .remove       = as5712_54x_sfp_remove,
    .id_table     = as5712_54x_sfp_id,
    .address_list = normal_i2c,
};

static int as5712_54x_sfp_read_byte(struct i2c_client *client, u8 command, u8 *data)
{
    int result = i2c_smbus_read_byte_data(client, command);

    if (unlikely(result < 0)) {
        dev_dbg(&client->dev, "sfp read byte data failed, command(0x%2x), data(0x%2x)\r\n", command, result);
        goto abort;
    }

    *data = (u8)result;
    result = 0;

abort:
    return result;
}

#define ALWAYS_UPDATE_DEVICE 1

static struct as5712_54x_sfp_data *as5712_54x_sfp_update_device(struct device *dev, int update_eeprom)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (ALWAYS_UPDATE_DEVICE || time_after(jiffies, data->last_updated + HZ + HZ / 2)
        || !data->valid) {
        int status = -1;
        int i = 0, j = 0;

        data->valid = 0;
        //dev_dbg(&client->dev, "Starting as5712_54x sfp status update\n");
        memset(data->status, 0, sizeof(data->status));

        /* Read status of port 1~48(SFP port) */
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 12; j++) {
                status = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2+i, 0x6+j);

                if (status < 0) {
                    dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD2+i, 0x6+j, status);
                    goto exit;
                }

                data->status[j/3] |= (u64)status << ((i*24) + (j%3)*8);
            }
        }

        /*
         * Bring QSFPs out of reset,
         * This is a temporary fix until the QSFP+_MOD_RST register
         * can be exposed through the driver.
         */
        as5712_54x_i2c_cpld_write(I2C_ADDR_CPLD3, 0x15, 0x3F);

        /* Read present status of port 49-54(QSFP port) */
        status = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x14);

        if (status < 0) {
            dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD2+i, 0x6+j, status);
        }
        else {
            data->status[SFP_IS_PRESENT] |= (u64)status << 48;
        }

        if (update_eeprom) {
            /* Read eeprom data based on port number */
            memset(data->eeprom, 0, sizeof(data->eeprom));

            /* Check if the port is present */
            if ((data->status[SFP_IS_PRESENT] & BIT_INDEX(data->port)) == 0) {
                /* read eeprom */
                for (i = 0; i < sizeof(data->eeprom); i++) {
                    status = as5712_54x_sfp_read_byte(client, i, data->eeprom + i);

                    if (status < 0) {
                        dev_dbg(&client->dev, "unable to read eeprom from port(%d)\n",
                                              CPLD_PORT_TO_FRONT_PORT(data->port));
                        goto exit;
                    }
                }
            }
        }

        data->valid = 1;
        data->last_updated = jiffies;
    }

exit:
    mutex_unlock(&data->update_lock);

    return data;
}

module_i2c_driver(as5712_54x_sfp_driver);

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("accton as5712_54x_sfp driver");
MODULE_LICENSE("GPL");

#if 0
        int i = 0, j = 0;

        data->valid = 0;
        //dev_dbg(&client->dev, "Starting as5712_54x sfp update\n");
        memset(data->status, 0, sizeof(data->status));

        /* Read status of port 1~48(SFP port) */
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 12; j++) {
                status = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2+i, 0x6+j);

                if (status < 0) {
                    dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD2+i, 0x6+j, status);
                    continue;
                }

                data->status[j/3] |= (u64)status << ((i*24) + (j%3)*8);
            }
        }

        /* Read present status of port 49-54(QSFP port) */
        status = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x14);

        if (status < 0) {
            dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD2+i, 0x6+j, status);
        }
        else {
            data->status[SFP_IS_PRESENT] |= (u64)status << 48;
        }
#endif

/* Reserver to prevent from CPLD port mapping is changed
 */
#if 0
BIT_INDEX(port_present_index[data->port])
/* The bit index of is_present field read from CPLD
 * Array index 0 is for as5712_54x_sfp1,
 *       index 1 is for as5712_54x_sfp2, and so on.
 */
static const int port_present_index[] = {
 4,  5,  6,  7,  9,  8, 11, 10,
 0,  1,  2,  3, 12, 13, 14, 15,
16, 17, 18, 19, 28, 29, 30, 31,
20, 21, 22, 23, 24, 25, 26, 27
};
#endif

#if 0
static struct as5712_54x_sfp_data *as5712_54x_sfp_update_status(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct as5712_54x_sfp_data *data = i2c_get_clientdata(client);
    int status = -1;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->status_last_updated + HZ + HZ / 2)
        || !data->status_valid) {
        int status = -1;
        int i = 0, j = 0;

        data->status_valid = 0;
        //dev_dbg(&client->dev, "Starting as5712_54x sfp status update\n");
        memset(data->status, 0, sizeof(data->status));

        /* Read status of port 1~48(SFP port) */
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 12; j++) {
                status = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD2+i, 0x6+j);

                if (status < 0) {
                    dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD2+i, 0x6+j, status);
                    goto exit;
                }

                data->status[j/3] |= (u64)status << ((i*24) + (j%3)*8);
            }
        }

        /*
         * Bring QSFPs out of reset,
         * This is a temporary fix until the QSFP+_MOD_RST register
         * can be exposed through the driver.
         */
        as5712_54x_i2c_cpld_write(I2C_ADDR_CPLD3, 0x15, 0x3F);

        /* Read present status of port 49-54(QSFP port) */
        status = as5712_54x_i2c_cpld_read(I2C_ADDR_CPLD3, 0x14);

        if (status < 0) {
            dev_dbg(&client->dev, "cpld(0x%x) reg(0x%x) err %d\n", I2C_ADDR_CPLD2+i, 0x6+j, status);
        }
        else {
            data->status[SFP_IS_PRESENT] |= (u64)status << 48;
        }

        data->status_valid = 1;
        data->status_last_updated = jiffies;
    }

exit:
    mutex_unlock(&data->update_lock);

    return data;
}

static struct as5712_54x_sfp_data *as5712_54x_sfp_update_eeprom(struct device *dev)
{
    struct as5712_54x_sfp_data *data = NULL;

    data = as5712_54x_sfp_update_status(dev);

    if (data == NULL || data->status_valid == 0) {
        data->eeprom_valid = 0;
        return data;
    }

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->eeprom_last_updated + HZ + HZ / 2)
        || !data->eeprom_valid) {
        int status = -1;
        int i = 0;

        /* Read eeprom data based on port number */
        memset(data->eeprom, 0, sizeof(data->eeprom));

        /* Check if the port is present */
        if ((data->status[SFP_IS_PRESENT] & BIT_INDEX(data->port)) == 0) {
            /* read eeprom */
            for (i = 0; i < sizeof(data->eeprom)/I2C_SMBUS_BLOCK_MAX; i++) {
                status = as5712_54x_sfp_read_block(client, i*I2C_SMBUS_BLOCK_MAX,
                                                   data->eeprom+(i*I2C_SMBUS_BLOCK_MAX),
                                                   I2C_SMBUS_BLOCK_MAX);
                if (status < 0) {
                    dev_dbg(&client->dev, "unable to read eeprom from port(%d)\n",
                                          CPLD_PORT_TO_FRONT_PORT(data->port));
                    goto exit;
                }
            }
        }

        data->eeprom_last_updated = jiffies;
        data->eeprom_valid = 1;
    }

exit:
    mutex_unlock(&data->update_lock);

    return data;
}
#endif
