/*
 * S9280-64x PSU driver
 *
 * Copyright (C) 2017 Ingrasys, Inc.
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
#include <linux/device.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include "ingrasys_s9280_64x_platform.h"

static ssize_t show_psu_eeprom(struct device *dev, 
                               struct device_attribute *da, 
                               char *buf);
static struct s9280_psu_data *s9280_psu_update_status(struct device *dev);
static struct s9280_psu_data *s9280_psu_update_eeprom(struct device *dev);
static int s9280_psu_read_block(struct i2c_client *client, 
                                u8 command, 
                                u8 *data,
                                int data_len);


#define DRIVER_NAME "psu"

// Addresses scanned 
static const unsigned short normal_i2c[] = { 0x50, I2C_CLIENT_END };

/* PSU EEPROM SIZE */
#define EEPROM_SZ 256
#define READ_EEPROM 1
#define NREAD_EEPROM 0

static struct i2c_client pca9555_client;

/* pca9555 gpio pin mapping */
#define PSU2_PWROK      0
#define PSU2_PRSNT_L    1
#define PSU2_PWRON_L    2
#define PSU1_PWROK      3
#define PSU1_PRSNT_L    4
#define PSU1_PWRON_L    5
#define TMP_75_INT_L    6

/* Driver Private Data */
struct s9280_psu_data {
    struct mutex    lock;
    char            valid;           /* !=0 if registers are valid */
    unsigned long   last_updated;    /* In jiffies */
    u8  index;                       /* PSU index */
    s32 status;                      /* IO expander value */
    char eeprom[EEPROM_SZ];          /* psu eeprom data */
    char psuABS;                     /* PSU absent */
    char psuPG;                      /* PSU power good */
};

enum psu_index 
{ 
    s9280_psu1, 
    s9280_psu2
};

/*
 * display power good attribute 
 */
static ssize_t 
show_psu_pg(struct device *dev, 
            struct device_attribute *devattr, 
            char *buf)
{
    struct s9280_psu_data *data = s9280_psu_update_status(dev);
    unsigned int value;

    mutex_lock(&data->lock);
    value = data->psuPG;        
    mutex_unlock(&data->lock);

    return sprintf(buf, "%d\n", value);
}

/*
 * display power absent attribute 
 */
static ssize_t 
show_psu_abs(struct device *dev, 
             struct device_attribute *devattr, 
             char *buf)
{
    struct s9280_psu_data *data = s9280_psu_update_status(dev);
    unsigned int value;

    mutex_lock(&data->lock);
    value = data->psuABS;       
    mutex_unlock(&data->lock);

    return sprintf(buf, "%d\n", value);
}


/* 
 * sysfs attributes for psu 
 */
static DEVICE_ATTR(psu_pg, S_IRUGO, show_psu_pg, NULL);
static DEVICE_ATTR(psu_abs, S_IRUGO, show_psu_abs, NULL);
static DEVICE_ATTR(psu_eeprom, S_IRUGO, show_psu_eeprom, NULL);

static struct attribute *s9280_psu_attributes[] = {
    &dev_attr_psu_pg.attr,
    &dev_attr_psu_abs.attr,
    &dev_attr_psu_eeprom.attr,
    NULL
};

/* 
 * display psu eeprom content
 */
static ssize_t 
show_psu_eeprom(struct device *dev, 
                struct device_attribute *da,
                char *buf)
{
    struct s9280_psu_data *data = s9280_psu_update_eeprom(dev);
    
    memcpy(buf, (char *)data->eeprom, EEPROM_SZ);
    return EEPROM_SZ;
}

static const struct attribute_group s9280_psu_group = {
    .attrs = s9280_psu_attributes,
};

/* 
 * check gpio expander is accessible
 */
static int 
pca9555_detect(struct i2c_client *client)
{
    if (i2c_smbus_read_byte_data(client, REG_PORT0_DIR) < 0) {
        return -ENODEV;
    }

    return 0;
}

/* 
 * client init
 */
static void 
i2c_devices_client_address_init(struct i2c_client *client)
{
    pca9555_client = *client;
    
    /* get i2c adapter for the target */
    pca9555_client.adapter = i2c_get_adapter(I2C_BUS_PSU_STATUS);
    
    /* get the i2c addr for the target */
    pca9555_client.addr = I2C_ADDR_PSU_STATUS;
}

static int 
s9280_psu_probe(struct i2c_client *client,
                const struct i2c_device_id *dev_id)
{
    struct s9280_psu_data *data;
    int status, err;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct s9280_psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }
    memset(data, 0, sizeof(struct s9280_psu_data));
    i2c_set_clientdata(client, data);
    data->valid = 0;
    data->index = dev_id->driver_data;
    mutex_init(&data->lock);

    i2c_devices_client_address_init(client);

    err = pca9555_detect(&pca9555_client);
    if (err) {
        return err; 
    }

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &s9280_psu_group);
    if (status) {
        goto exit_free;
    }
    
    return 0;

exit_free:
    kfree(data);
exit:
    
    return status;
}

static int 
s9280_psu_remove(struct i2c_client *client)
{
    struct s9280_psu_data *data = i2c_get_clientdata(client);

    sysfs_remove_group(&client->dev.kobj, &s9280_psu_group);
    kfree(data);
    
    /* free i2c adapter */
    i2c_put_adapter(pca9555_client.adapter);
    
    return 0;
}


/* 
 * psu eeprom read utility
 */
static int 
s9280_psu_read_block(struct i2c_client *client, 
                     u8 command, 
                     u8 *data,
                     int data_len)
{
    int i=0, ret=0;
    int blk_max = 32; //max block read size

    /* read eeprom, 32 * 8 = 256 bytes */
    for (i=0; i < EEPROM_SZ/blk_max; i++) {
        ret = i2c_smbus_read_i2c_block_data(client, (i*blk_max), blk_max, 
                                            data + (i*blk_max));
        if (ret < 0) {
            return ret;
        }
    }
    return ret;
}

/* 
 * update eeprom content
 */
static struct s9280_psu_data 
*s9280_psu_update_eeprom(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct s9280_psu_data *data = i2c_get_clientdata(client);
    s32 status = 0;
    int psu_pwrok = 0;
    int psu_prsnt_l = 0;
    
    mutex_lock(&data->lock);

    if (time_after(jiffies, data->last_updated + 300 * HZ)
        || !data->valid) {

        /* Read psu status */
        
        status = i2c_smbus_read_word_data(&(pca9555_client), REG_PORT0_IN);
        data->status = status;

        /*read psu status from io expander*/

        if (data->index == s9280_psu1) {
            psu_pwrok = PSU1_PWROK;
            psu_prsnt_l = PSU1_PRSNT_L;
        } else {
            psu_pwrok = PSU2_PWROK;
            psu_prsnt_l = PSU2_PRSNT_L;
        }
        data->psuPG = (status >> psu_pwrok) & 0x1;
        data->psuABS = (status >> psu_prsnt_l) & 0x1; 
        
        /* Read eeprom */
        if (!data->psuABS) {
            //clear local eeprom data
            memset(data->eeprom, 0, EEPROM_SZ);

            //read eeprom
            status = s9280_psu_read_block(client, 0, data->eeprom, 
                                               ARRAY_SIZE(data->eeprom));

            if (status < 0) {
                memset(data->eeprom, 0, EEPROM_SZ);
                dev_err(&client->dev, "Read eeprom failed, status=(%d)\n", status);
            } else {
                data->valid = 1;
            }
        } else {
            memset(data->eeprom, 0, EEPROM_SZ);
        }
        data->last_updated = jiffies;
    }

    mutex_unlock(&data->lock);

    return data;
}

/* 
 * update psu status
 */
static struct s9280_psu_data 
*s9280_psu_update_status(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct s9280_psu_data *data = i2c_get_clientdata(client);
    s32 status = 0;
    int psu_pwrok = 0;
    int psu_prsnt_l = 0;
    
    mutex_lock(&data->lock);

    /* Read psu status */
        
    status = i2c_smbus_read_word_data(&(pca9555_client), REG_PORT0_IN);
    data->status = status;

    /*read psu status from io expander*/

    if (data->index == s9280_psu1) {
        psu_pwrok = PSU1_PWROK;
        psu_prsnt_l = PSU1_PRSNT_L;
    } else {
        psu_pwrok = PSU2_PWROK;
        psu_prsnt_l = PSU2_PRSNT_L;
    }
    data->psuPG = (status >> psu_pwrok) & 0x1;
    data->psuABS = (status >> psu_prsnt_l) & 0x1; 

    mutex_unlock(&data->lock);

    return data;
}

static const struct i2c_device_id s9280_psu_id[] = {
    { "psu1", s9280_psu1 },
    { "psu2", s9280_psu2 },
    {}
};

MODULE_DEVICE_TABLE(i2c, s9280_psu_id);

static struct i2c_driver s9280_psu_driver = {
    .driver = {
        .name     = DRIVER_NAME,
    },
    .probe        = s9280_psu_probe,
    .remove       = s9280_psu_remove,
    .id_table     = s9280_psu_id,
    .address_list = normal_i2c,
};

static int __init s9280_psu_init(void)
{
    return i2c_add_driver(&s9280_psu_driver);
}

static void __exit s9280_psu_exit(void)
{
    i2c_del_driver(&s9280_psu_driver);
}

module_init(s9280_psu_init);
module_exit(s9280_psu_exit);

MODULE_AUTHOR("Leo Lin <feng.lee.usa@ingrasys.com>");
MODULE_DESCRIPTION("S9280-64X psu driver");
MODULE_LICENSE("GPL");
