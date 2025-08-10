/*
 * switchboard.c - Celestica switchboard CPLD I2C driver.
 * Copyright (C) 2019 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


// TODO: User regmap for more descriptive register access. See MFD
// TODO: Add support of legacy i2c bus and smbus_emulated bus.


#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/hwmon.h>

/**
 * CPLD register address for read and write.
 */
#define VERSION_ADDR	0x00
#define SCRATCH_ADDR	0x01
#define PORT_LED_MOD_ADDR	0x09
#define PORT_LED_COLOR_ADDR	0x0A
#define PORT_SL_ADDR	0x10
#define PORT_CR_ADDR	0x11
#define PORT_SR_ADDR	0x12
#define PORT_INT_STAT   0x13


#define CTRL_RST     4
#define CTRL_LPMD    0
#define SR_MODPRS    4  
#define SR_INTN      0
#define INT_STAT_LOS      0

/* One switchboard CPLD control 16 QSFP ports*/
#define QSFP_PORT_NUM 16 

#define SWCPLD1_I2C_ADDR 0x30
#define SWCPLD2_I2C_ADDR 0x31
#define SWCPLD3_I2C_ADDR 0x32
#define SWCPLD4_I2C_ADDR 0x33

#define SWCPLD_NUM 4


/* Private data for switchboard CPLD */
struct switchboard_data {
	struct device *sff_parent_dev;
	struct device *sff_devices[QSFP_PORT_NUM];
	struct regmap *regmap;
	struct i2c_client *client;
	struct class* class;
	uint16_t read_addr;
	struct mutex lock;
};

struct sff_device_data{
    int portid;
	struct i2c_client *client;
    struct mutex *lock;
};


/* CPLD attributes */
static ssize_t version_show(struct device *dev, 
			    struct device_attribute *attr, 
			    char *buf)
{
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	value = i2c_smbus_read_byte_data(client, VERSION_ADDR);
	if(value < 0)
		return value;

	return sprintf(buf, "%d.%d\n", value >> 4, value & 0x0F);
}

static ssize_t scratch_show(struct device *dev, 
			    struct device_attribute *attr, 
			    char *buf)
{
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	value = i2c_smbus_read_byte_data(client, SCRATCH_ADDR);
	if(value < 0)
		return value;

	return sprintf(buf, "0x%.2x\n", value);
}

static ssize_t scratch_store(struct device *dev, 
			     struct device_attribute *attr, 
			     const char *buf, size_t count)
{
	u8 value;
	ssize_t status;
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

	status = kstrtou8(buf, 0, &value);
	if(status != 0)
		return status;
	status = i2c_smbus_write_byte_data(client, SCRATCH_ADDR, value);
	if(status == 0)
		status = count;
	return status;
}

static ssize_t getreg_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    uint16_t addr;
    char *last;
	struct switchboard_data *data = dev_get_drvdata(dev);

    addr = (uint16_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        return -EINVAL;
    }
    data->read_addr = addr;
    return count;
}

static ssize_t getreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	value = i2c_smbus_read_byte_data(client, data->read_addr);
	if(value < 0)
		return value;

	return sprintf(buf, "0x%.2x\n", value);
	
}


static ssize_t setreg_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    // CPLD register is one byte
    uint16_t addr;
    uint8_t value;
    char *tok;
    char clone[count];
    char *pclone = clone;
    char *last;
    struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;

    strcpy(clone, buf);

    mutex_lock(&data->lock);
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&data->lock);
        return -EINVAL;
    }
    addr = (uint16_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        mutex_unlock(&data->lock);
        return -EINVAL;
    }

    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&data->lock);
        return -EINVAL;
    }
    value = (uint8_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        mutex_unlock(&data->lock);
        return -EINVAL;
    }
	
    i2c_smbus_write_byte_data(client, addr, value);
    mutex_unlock(&data->lock);
    return count;
}

static ssize_t port_led_mode_show(struct device *dev, 
			    struct device_attribute *attr, 
			    char *buf)
{
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	value = i2c_smbus_read_byte_data(client, PORT_LED_MOD_ADDR);
	if(value < 0)
		return value;
	
	value = value & 0x01;
	return sprintf(buf, "%s\n",
				value == 0x00 ? "normal" : "test");
}

static ssize_t port_led_mode_store(struct device *dev, 
					 struct device_attribute *attr, 
					 const char *buf, size_t count)
{
    unsigned char led_mode;
	ssize_t status;
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	
    if(sysfs_streq(buf, "normal")){
        led_mode = 0x00;
    }else if(sysfs_streq(buf, "test")){
        led_mode = 0x01;
    }else{
        count = -EINVAL;
        return count;
    }
	mutex_lock(&data->lock);
	status = i2c_smbus_write_byte_data(client, PORT_LED_MOD_ADDR, led_mode);
	if(status == 0)
		status = count;
	mutex_unlock(&data->lock);
	return status;
}

static ssize_t port_led_color_show(struct device *dev, 
			    struct device_attribute *attr, 
			    char *buf)
{
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int value;

	value = i2c_smbus_read_byte_data(client, PORT_LED_COLOR_ADDR);
	if(value < 0)
		return value;
	
	value = value & 0x07;
	return sprintf(buf, "%s\n",
				value == 0x00 ? "white" : value == 0x01 ? "yellow" : value == 0x05 ? "green" : value == 0x06 ? "blue" : "off");
}

static ssize_t port_led_color_store(struct device *dev, 
					 struct device_attribute *attr, 
					 const char *buf, size_t count)
{
    unsigned char led_color;
	ssize_t status;
	struct switchboard_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	
    if(sysfs_streq(buf, "white")){
        led_color = 0x00;
    }else if(sysfs_streq(buf, "yellow")){
        led_color = 0x01;
	}else if(sysfs_streq(buf, "green")){
		led_color = 0x05;
	}else if(sysfs_streq(buf, "blue")){
		led_color = 0x06;
	}else if(sysfs_streq(buf, "off")){
		led_color = 0x07;
    }else{
        count = -EINVAL;
        return count;
    }
	mutex_lock(&data->lock);
	status = i2c_smbus_write_byte_data(client, PORT_LED_COLOR_ADDR, led_color);
	if(status == 0)
		status = count;
	mutex_unlock(&data->lock);
	return status;
}


DEVICE_ATTR_RO(version);
DEVICE_ATTR_RW(scratch);
DEVICE_ATTR_RW(getreg);
DEVICE_ATTR_WO(setreg);
DEVICE_ATTR_RW(port_led_mode);
DEVICE_ATTR_RW(port_led_color);


/* QSPF attributes */
static ssize_t qsfp_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 data;
	int len = 0;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;
	
    mutex_lock(drvdata->lock);
	i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
	data = i2c_smbus_read_byte_data(client, PORT_CR_ADDR);
    len = sprintf(buf, "%x\n",(data >> CTRL_RST) & 0x01);
    mutex_unlock(drvdata->lock);
    return len;
}

static ssize_t qsfp_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    ssize_t status;
    long value;
    u8 data;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;

    mutex_lock(drvdata->lock);
    status = kstrtol(buf, 0, &value);
    if (status == 0) {
		i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
        // if value is 0, reset signal is low
        data = i2c_smbus_read_byte_data(client, PORT_CR_ADDR);
        if (!value)
            data = data & ~( (u8)0x1 << CTRL_RST);
        else
            data = data | ((u8)0x1 << CTRL_RST);
		i2c_smbus_write_byte_data(client, PORT_CR_ADDR, data);
        status = size;
    }
    mutex_unlock(drvdata->lock);
    return status;
}

static ssize_t qsfp_lpmode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int value;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;

    mutex_lock(drvdata->lock);
	i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
	value = i2c_smbus_read_byte_data(client, PORT_CR_ADDR);
	if(value < 0)
		return value;
    mutex_unlock(drvdata->lock);
    return sprintf(buf, "%d\n",(value >> CTRL_LPMD) & 0x01);

}
static ssize_t qsfp_lpmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    ssize_t status;
    long value;
    u8 data;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;

    mutex_lock(drvdata->lock);
    status = kstrtol(buf, 0, &value);
    if (status == 0) {
		i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
        // if value is 0, reset signal is low
        data = i2c_smbus_read_byte_data(client, PORT_CR_ADDR);
        if (!value)
            data = data & ~( (u8)0x1 << CTRL_LPMD);
        else
            data = data | ((u8)0x1 << CTRL_LPMD);
		i2c_smbus_write_byte_data(client, PORT_CR_ADDR, data);
        status = size;
    }
    mutex_unlock(drvdata->lock);
    return status;

}

static ssize_t qsfp_modprs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
	int len = 0;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;
	
    mutex_lock(drvdata->lock);
	i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
	data = i2c_smbus_read_byte_data(client, PORT_SR_ADDR);
    len = sprintf(buf, "%x\n",(data >> SR_MODPRS) & 0x01);
    mutex_unlock(drvdata->lock);
    return len;

}

static ssize_t qsfp_modirq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
	int len = 0;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;
	
    mutex_lock(drvdata->lock);
	i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
	data = i2c_smbus_read_byte_data(client, PORT_SR_ADDR);
    len = sprintf(buf, "%x\n",(data >> SR_INTN) & 0x01);
    mutex_unlock(drvdata->lock);
    return len;

}

static ssize_t qsfp_modintl_show(struct device *dev, struct device_attribute *attr, char *buf){
	u32 data;
	int len = 0;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;
	
    mutex_lock(drvdata->lock);
	i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
	data = i2c_smbus_read_byte_data(client, PORT_INT_STAT);
    len = sprintf(buf, "%x\n",(data >> INT_STAT_LOS) & 0x01);
    mutex_unlock(drvdata->lock);
    return len;

}


#if 0

static ssize_t qsfp_modintl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size){
	ssize_t status;
    long value;
    u8 data;
    struct sff_device_data *drvdata = dev_get_drvdata(dev);
	struct i2c_client *client = drvdata->client;
	u8 portid = drvdata->portid;

    mutex_lock(&drvdata->lock);
    status = kstrtol(buf, 0, &value);
    if (status == 0) {
		i2c_smbus_write_byte_data(client, PORT_SL_ADDR, portid);
        // if value is 0, reset signal is low
        data = i2c_smbus_read_byte_data(client, PORT_INT_MASk);
        if (!value)
            data = data & ~( (u8)0x1 << INT_MASk_LOS);
        else
            data = data | ((u8)0x1 << INT_MASk_LOS);
		i2c_smbus_write_byte_data(client, PORT_INT_MASk, data);
        status = size;
    }
    mutex_unlock(&drvdata->lock);
    return status;

}

#endif

DEVICE_ATTR_RW(qsfp_reset);
DEVICE_ATTR_RW(qsfp_lpmode);
DEVICE_ATTR_RO(qsfp_modprs);
DEVICE_ATTR_RO(qsfp_modirq);
DEVICE_ATTR_RO(qsfp_modintl);


static struct attribute *switchboard_attrs[] = {
	&dev_attr_version.attr,
	&dev_attr_scratch.attr,
	&dev_attr_getreg.attr,
    &dev_attr_setreg.attr,
    &dev_attr_port_led_mode.attr,
    &dev_attr_port_led_color.attr,
	NULL,
};

static struct attribute *sff_attrs[] = {	
	&dev_attr_qsfp_modirq.attr,
	&dev_attr_qsfp_modprs.attr,
	&dev_attr_qsfp_lpmode.attr,
	&dev_attr_qsfp_reset.attr,
	&dev_attr_qsfp_modintl.attr,
	NULL,
};


static struct attribute_group switchboard_attrs_grp = {
    .attrs = switchboard_attrs,
};

static struct attribute_group sff_attr_grp = {
    .attrs = sff_attrs,
};
static const struct attribute_group *sff_attr_grps[] = {
    &sff_attr_grp,
    NULL
};


static struct device * cloverstone_dp_sff_init(struct device *dev, int portid) {

	struct switchboard_data *data = dev_get_drvdata(dev);
	struct sff_device_data *new_data;
    struct device *new_device;
	int device_id = 0;
	char port_map[4][16] = {
			{1, 2, 33, 34, 3, 4, 35, 36, 5, 6, 37, 38, 7, 8, 39, 40}, 
			{9, 10, 41, 42, 11, 12, 43, 44, 13, 14, 45, 46, 15, 16, 47, 48}, 
			{17, 18, 49, 50, 19, 20, 51, 52, 21, 22, 53, 54, 23, 24, 55, 56}, 
			{25, 26, 57, 58, 27, 28, 59, 60, 29, 30, 61, 62, 31, 32, 63, 64}
		};
	
    new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
    if (!new_data) {
        printk(KERN_ALERT "Cannot alloc sff device data @port%d", portid);
        return NULL;
    } 
    /* The QSFP port ID start from 1 */
    new_data->portid = portid + 1;
	new_data->client = data->client;
	new_data->lock = &(data->lock);
	switch(data->client->addr){
		case SWCPLD1_I2C_ADDR:{
			device_id = 0;
		}
		break;

		case SWCPLD2_I2C_ADDR:{
			device_id = 1;
		}
		break;

		case SWCPLD3_I2C_ADDR:{
			device_id = 2;
		}
		break;

		case SWCPLD4_I2C_ADDR:{
			device_id = 3;
		}
		break;
	}
	
	/* CPLD1(0x30) control QSFP(1-16) and CPLD1(0x32) control QSFP(17-32) and SFP(1-2) */
    new_device = device_create_with_groups(data->class, 
                                           NULL,
                                           MKDEV(0, 0), 
                                           new_data, 
                                           sff_attr_grps, 
                                           "%s%d", 
                                           "QSFP",  port_map[device_id][portid]);
	
	if (IS_ERR(new_device)) {
        printk(KERN_ALERT "Cannot create sff device @port%d", port_map[device_id][portid]);
        kfree(new_data);
        return NULL;
    }
	
    printk(KERN_INFO "Create sff device @port%d", port_map[device_id][portid]);
    return new_device;
}


static int switchboard_probe(struct i2c_client *client, 
			   const struct i2c_device_id *id)
{
	int err, ret = 0;
	struct device *dev;
	struct switchboard_data *data;
	static struct class* class;
	u8 portid = 0;
	struct sff_device_data *new_data;
	
	dev = &client->dev;

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	data = devm_kzalloc(dev, sizeof(struct switchboard_data), 
		GFP_KERNEL);

	if (!data){
		err = -ENOMEM;
		goto fail_alloc_switchboard_data;
	}
	
	dev_set_drvdata(dev, data);
	mutex_init(&data->lock);	
	data->client = client;

	/*create CPLD sysfs*/
	ret = sysfs_create_group(&dev->kobj, &switchboard_attrs_grp);
    if (ret != 0) {
        goto fail_alloc_switchboard_data;
    }

	if (class == NULL){
		class = class_create(THIS_MODULE, "SFF");
		data->class = class;
		if (IS_ERR(data->class)) {
	        printk(KERN_ALERT "Failed to register device class\n");
	        err = PTR_ERR(data->class);
	        goto fail_sysfs_create_group;
	    }
	}else{
		data->class = class;
	}
    /* create 32 QSFP sysfs */
    for (portid = 0; portid < QSFP_PORT_NUM; portid++) {
        data->sff_devices[portid] = cloverstone_dp_sff_init(dev, portid);
		if (IS_ERR(data->sff_devices[portid])){
			printk(KERN_ALERT "Failed to register device\n");
			err = PTR_ERR(data->sff_devices[portid]);
			goto fail_register_sff_device;
		}
    }
	return 0;
	
fail_register_sff_device:	
	for (portid = 0; portid < QSFP_PORT_NUM; portid++) {
		if (data->sff_devices[portid] != NULL){
			new_data = dev_get_drvdata(data->sff_devices[portid]);
			device_unregister(data->sff_devices[portid]);
			put_device(data->sff_devices[portid]);
            kfree(new_data);
		}
	}
	device_destroy(data->class, MKDEV(0, 0));
	class_unregister(data->class);
	class_destroy(data->class);
fail_sysfs_create_group:
	sysfs_remove_group(&dev->kobj, &switchboard_attrs_grp);
fail_alloc_switchboard_data:
	return err;
}

static int switchboard_remove(struct i2c_client *client)
{
	u8 portid = 0;
	struct sff_device_data *new_data;
	struct device *dev = &client->dev;
	struct switchboard_data *data = dev_get_drvdata(dev);
	static u8 index = 1;
	
	for (portid = 0; portid < QSFP_PORT_NUM; portid++) {
		if (data->sff_devices[portid] != NULL){
			new_data = dev_get_drvdata(data->sff_devices[portid]);
			device_unregister(data->sff_devices[portid]);
			put_device(data->sff_devices[portid]);
	        kfree(new_data);
		}
	}
	if (index == SWCPLD_NUM){
		device_destroy(data->class, MKDEV(0, 0));
		class_unregister(data->class);
		class_destroy(data->class);
	}
	sysfs_remove_group(&dev->kobj, &switchboard_attrs_grp);
	index++;
	return 0;
}

static const struct i2c_device_id switchboard_ids[] = {
	{ "switchboard", 0 },
	{ /* END OF List */ }
};
MODULE_DEVICE_TABLE(i2c, switchboard_ids);

struct i2c_driver switchboard_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = "switchboard",
		.owner = THIS_MODULE,
	},
	.probe = switchboard_probe,
	.remove = switchboard_remove,
	.id_table = switchboard_ids,
};

module_i2c_driver(switchboard_driver);

MODULE_AUTHOR("Celestica Inc.");
MODULE_DESCRIPTION("Celestica CPLD switchboard driver");
MODULE_VERSION("2.0.0");
MODULE_LICENSE("GPL");
