// SPDX-License-Identifier: GPL-2.0+
/*
 * lpc_basecpld.c - The CPLD driver for the Base Board of ds2000
 * The driver implement sysfs to access CPLD register on the baseboard of ds2000 via LPC bus.
 *
 * Author: Nicholas Wu <nicwu@celestica.com>/Oscar Xu <ohsu@celestica.com>
 * Copyright (C) 2022-2024 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>
#include <linux/string.h>

#define DRIVER_NAME "sys_cpld"
/**
 * CPLD register address for read and write.
 */
#define VERSION_ADDR 0xA100
#define SCRATCH_ADDR 0xA101
#define PSU_LED_ADDR 0xA161
#define SYS_LED_ADDR 0xA162
#define ALARM_LED_ADDR 0xA163
#define FAN_LED_ADDR 0xA165
#define COME_CPLD_VER_ADDR 0xA1E0

#define CPLD_REGISTER_SIZE 0xFF

/* LED CTRL */

enum FAN_LED {
    fan_led_amb = 1,
    fan_led_grn,
    fan_led_off,
    fan_led_auto = 0x10
} fan_led;

enum SYS_LED {
    sys_led_both = 0,
    sys_led_grn,
    sys_led_amb,
    sys_led_off
} sys_led;

enum PWR_LED {
    pwr_led_amb = 1,
	pwr_led_grn,
    pwr_led_off,
    pwr_led_auto = 0x10
} pwr_led;

enum ALARM_LED {
    alarm_led_both = 0,
    alarm_led_grn,
    alarm_led_amb,
    alarm_led_off
} alarm_led;

enum LED_CTRL {
    led_on = 0,
    led_blk_1hz,
    led_blk_4hz,
    led_off
} led_ctrl;

#define LED_OFF        "off"
#define LED_GREEN      "green"
#define LED_AMBER      "amber"
#define LED_HZ_GBNK    "grn_bnk_1hz"
#define LED_HZ_ABNK    "amb_bnk_1hz"
#define LED_QHZ_GBNK   "grn_bnk_4hz"
#define LED_QHZ_ABNK   "amb_bnk_4hz"
#define LED_HZ_GABNK   "grn_amb_1hz"
#define LED_QHZ_GABNK  "grn_amb_4hz"

struct cpld_b_data {
	struct mutex	   cpld_lock;
	uint16_t		   read_addr;
};

struct cpld_b_data *cpld_data;

/**
 * Read the value from scratch register as hex string.
 * @param  dev	 kernel device
 * @param  devattr kernel device attribute
 * @param  buf	 buffer for get value
 * @return		 Hex string read from scratch register.
 */
static ssize_t scratch_show(struct device *dev,
							struct device_attribute *devattr,
							char *buf)
{
	unsigned char data = 0;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(SCRATCH_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);

	return sprintf(buf, "0x%2.2x\n", data);
}

/**
 * Set scratch register with specific hex string.
 * @param  dev	 kernel device
 * @param  devattr kernel device attribute
 * @param  buf	 buffer of set value
 * @param  count   number of bytes in buffer
 * @return		 number of bytes written, or error code < 0.
 */
static ssize_t scratch_store(struct device *dev,
								struct device_attribute *devattr,
								const char *buf, size_t count)
{
	int ret = 0;
	unsigned long data;

	mutex_lock(&cpld_data->cpld_lock);
	ret = kstrtoul(buf, 0, &data);
	if (ret != 0) {
		mutex_unlock(&cpld_data->cpld_lock);
		return ret;
	}
	outb(data, SCRATCH_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);

	return count;
}

/* CPLD version attributes */
static ssize_t version_show(struct device *dev,
							struct device_attribute *attr,
							char *buf)
{
	int len = 0;
	unsigned char value = 0;

	// CPLD register is one byte
	mutex_lock(&cpld_data->cpld_lock);
	value = inb(VERSION_ADDR);
	len = sprintf(buf, "%d.%d\n", value >> 4, value & 0x0F);
	mutex_unlock(&cpld_data->cpld_lock);

	return len;
}


static ssize_t getreg_store(struct device *dev,
							struct device_attribute *devattr,
							const char *buf,
							size_t count)
{
	int ret = 0;
	unsigned long addr;

	ret = kstrtoul(buf, 0, &addr);
	if (ret != 0) {
		return ret;
	}
	cpld_data->read_addr = addr;

	return count;
}

static ssize_t getreg_show(struct device *dev,
							struct device_attribute *attr,
							char *buf)
{
	int len = 0;

	// CPLD register is one byte
	mutex_lock(&cpld_data->cpld_lock);
	len = sprintf(buf, "0x%2.2x\n", inb(cpld_data->read_addr));
	mutex_unlock(&cpld_data->cpld_lock);

	return len;
}

static ssize_t setreg_store(struct device *dev, struct device_attribute *devattr,
				const char *buf, size_t count)
{
	int ret = 0;
	unsigned long addr;
	unsigned long value;
	char *tok;
	char clone[20];
	char *pclone = clone;

	strscpy(clone, buf,sizeof(clone));

	mutex_lock(&cpld_data->cpld_lock);
	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&cpld_data->cpld_lock);
		return -EINVAL;
	}
	ret = kstrtoul(tok, 0, &addr);
	if (ret != 0) {
		mutex_unlock(&cpld_data->cpld_lock);
		return ret;
	}

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&cpld_data->cpld_lock);
		return -EINVAL;
	}
	ret = kstrtoul(tok, 0, &value);
	if (ret != 0) {
		mutex_unlock(&cpld_data->cpld_lock);
		return ret;
	}
	outb(value, addr);
	mutex_unlock(&cpld_data->cpld_lock);

	return count;
}

/**
 * Read all CPLD register in binary mode.
 * @return number of byte read.
 */
static ssize_t dump_read(struct file *filp, struct kobject *kobj,
				struct bin_attribute *attr, char *buf,
				loff_t off, size_t count)
{
	unsigned long i = 0;
	ssize_t status;

	mutex_lock(&cpld_data->cpld_lock);
begin:
	if (i < count) {
		buf[i++] = inb(VERSION_ADDR + off);
		off++;
		msleep(1);
		goto begin;
	}
	status = count;
	mutex_unlock(&cpld_data->cpld_lock);

	return status;
}

/**
 * @brief          Show status led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         led state - off/on/blink
 */
static ssize_t sys_led_show(struct device *dev, struct device_attribute *devattr,
                               char *buf)
{
    unsigned char data = 0;
    unsigned char color = 0;
    unsigned char control = 0;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    color = (data & 0x30) >> 4;
    control = (data & 0x3);

    switch(color){
        case sys_led_both:
            if ( control == led_blk_1hz){
                return sprintf(buf, "%s\n", LED_HZ_GABNK);
            }else if(control == led_blk_4hz){
                return sprintf(buf, "%s\n", LED_QHZ_GABNK);
            }else{
                break;
            }
        case sys_led_amb:
            if ( control == led_blk_1hz){
                return sprintf(buf, "%s\n", LED_HZ_ABNK);
            }else if(control == led_blk_4hz){
                return sprintf(buf, "%s\n", LED_QHZ_ABNK);
            }else if(control == led_on){
                return sprintf(buf, "%s\n", LED_AMBER);
            }else{
                break;
            }
        case sys_led_grn:
            if ( control == led_blk_1hz){
                return sprintf(buf, "%s\n", LED_HZ_GBNK);
            }else if(control == led_blk_4hz){
                return sprintf(buf, "%s\n", LED_QHZ_GBNK);
            }else if(control == led_on){
                return sprintf(buf, "%s\n", LED_GREEN);
            }else{
                break;
            }
        default:
            break;
    }

    return sprintf(buf, "%s\n", LED_OFF);
}

/**
 * @brief          Set the status led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value - off/on/blink
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t sys_led_store(struct device *dev, struct device_attribute *devattr,
                                const char *buf, size_t count)
{
    unsigned char data;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    if(sysfs_streq(buf, LED_OFF)){
        data &= 0xCC;
        data |= sys_led_off << 4;
        data |= led_off;
    }else if(sysfs_streq(buf, LED_GREEN)){
        data &= 0xCC;
        data |= sys_led_grn << 4;
        data |= led_on;
    }else if(sysfs_streq(buf, LED_AMBER)){
        data &= 0xCC;
        data |= sys_led_amb << 4;
        data |= led_on;
    }else if(sysfs_streq(buf, LED_HZ_GBNK)){
        data &= 0xCC;
        data |= sys_led_grn << 4;
        data |= led_blk_1hz;
    }else if(sysfs_streq(buf, LED_HZ_ABNK)){
        data &= 0xCC;
        data |= sys_led_amb << 4;
        data |= led_blk_1hz;
    }else if(sysfs_streq(buf, LED_QHZ_GBNK)){
        data &= 0xCC;
        data |= sys_led_grn << 4;
        data |= led_blk_4hz;
    }else if(sysfs_streq(buf, LED_QHZ_ABNK)){
        data &= 0xCC;
        data |= sys_led_amb << 4;
        data |= led_blk_4hz;
    }else if(sysfs_streq(buf, LED_HZ_GABNK)){
        data &= 0xCC;
        data |= sys_led_both << 4;
        data |= led_blk_1hz;
    }else if(sysfs_streq(buf, LED_QHZ_GABNK)){
        data &= 0xCC;
        data |= sys_led_both << 4;
        data |= led_blk_4hz;
    }else{
        count = -EINVAL;
    }

    outb(data, SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}

/**
 * @brief          Show alarm led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         led state - off/on/blink
 */
static ssize_t alarm_led_show(struct device *dev, struct device_attribute *devattr,
                               char *buf)
{
    unsigned char data = 0;
    unsigned char color = 0;
    unsigned char control = 0;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(ALARM_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    color = (data & 0x30) >> 4;
    control = (data & 0x3);

    switch(color){
        case alarm_led_both:
            if ( control == led_blk_1hz){
                return sprintf(buf, "%s\n", LED_HZ_GABNK);
            }else if(control == led_blk_4hz){
                return sprintf(buf, "%s\n", LED_QHZ_GABNK);
            }else{
                break;
            }
        case alarm_led_amb:
            if ( control == led_blk_1hz){
                return sprintf(buf, "%s\n", LED_HZ_ABNK);
            }else if(control == led_blk_4hz){
                return sprintf(buf, "%s\n", LED_QHZ_ABNK);
            }else if(control == led_on){
                return sprintf(buf, "%s\n", LED_AMBER);
            }else{
                break;
            }
        case alarm_led_grn:
            if ( control == led_blk_1hz){
                return sprintf(buf, "%s\n", LED_HZ_GBNK);
            }else if(control == led_blk_4hz){
                return sprintf(buf, "%s\n", LED_QHZ_GBNK);
            }else if(control == led_on){
                return sprintf(buf, "%s\n", LED_GREEN);
            }else{
                break;
            }
        default:
            break;
    }

    return sprintf(buf, "%s\n", LED_OFF);
}

/**
 * @brief          Set the alarm led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value - off/on/blink
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t alarm_led_store(struct device *dev, struct device_attribute *devattr,
                                const char *buf, size_t count)
{
    unsigned char data;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(ALARM_LED_ADDR);
    if(sysfs_streq(buf, LED_OFF)){
        data &= 0xCC;
        data |= alarm_led_off << 4;
        data |= led_off;
    }else if(sysfs_streq(buf, LED_GREEN)){
        data &= 0xCC;
        data |= alarm_led_grn << 4;
        data |= led_on;
    }else if(sysfs_streq(buf, LED_AMBER)){
        data &= 0xCC;
        data |= alarm_led_amb << 4;
        data |= led_on;
    }else if(sysfs_streq(buf, LED_HZ_GBNK)){
        data &= 0xCC;
        data |= alarm_led_grn << 4;
        data |= led_blk_1hz;
    }else if(sysfs_streq(buf, LED_HZ_ABNK)){
        data &= 0xCC;
        data |= alarm_led_amb << 4;
        data |= led_blk_1hz;
    }else if(sysfs_streq(buf, LED_QHZ_GBNK)){
        data &= 0xCC;
        data |= alarm_led_grn << 4;
        data |= led_blk_4hz;
    }else if(sysfs_streq(buf, LED_QHZ_ABNK)){
        data &= 0xCC;
        data |= alarm_led_amb << 4;
        data |= led_blk_4hz;
    }else if(sysfs_streq(buf, LED_HZ_GABNK)){
        data &= 0xCC;
        data |= alarm_led_both << 4;
        data |= led_blk_1hz;
    }else if(sysfs_streq(buf, LED_QHZ_GABNK)){
        data &= 0xCC;
        data |= alarm_led_both << 4;
        data |= led_blk_4hz;

    }else{
        count = -EINVAL;
    }

    outb(data, ALARM_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}

static ssize_t pwr_led_show(struct device *dev, struct device_attribute *devattr,
                               char *buf)
{
    unsigned char data = 0;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    if ((data & 0x10) != 0)
    {
        return sprintf(buf, "%s\n", "auto");
    }
    data = data & 0x3;
    return sprintf(buf, "%s\n",
                   data == pwr_led_grn ? "green" : data == pwr_led_amb ? "amber" : "off");
}


static ssize_t pwr_led_store(struct device *dev, struct device_attribute *devattr,
                                const char *buf, size_t count)
{
    unsigned char led_status, data;

    if (sysfs_streq(buf, "off")) {
        led_status = pwr_led_off;
    } else if (sysfs_streq(buf, "green")) {
        led_status = pwr_led_grn;
    } else if (sysfs_streq(buf, "amber")) {
        led_status = pwr_led_amb;
    } else if (sysfs_streq(buf, "auto")) {
        led_status = pwr_led_auto;
    } else {
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(PSU_LED_ADDR);
    /* Set bit 4 as 0 to control pwrled by software */
    data = data & ~(0x13);
    data = data | led_status;
    outb(data, PSU_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}

static ssize_t fan_led_show(struct device *dev, struct device_attribute *devattr,
                               char *buf)
{
    unsigned char data = 0;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    if ((data & 0x10) != 0)
    {
        return sprintf(buf, "%s\n", "auto");
    }
    data = data & 0x3;
    return sprintf(buf, "%s\n",
                   data == fan_led_grn ? "green" : data == fan_led_amb ? "amber" : "off");
}


static ssize_t fan_led_store(struct device *dev, struct device_attribute *devattr,
                                const char *buf, size_t count)
{
    unsigned char led_status, data;

    if (sysfs_streq(buf, "off")) {
        led_status = fan_led_off;
    } else if (sysfs_streq(buf, "green")) {
        led_status = fan_led_grn;
    } else if (sysfs_streq(buf, "amber")) {
        led_status = fan_led_amb;
    } else if (sysfs_streq(buf, "auto")) {
        led_status = fan_led_auto;
    } else {
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN_LED_ADDR);
    /* Set bit 4 as 0 to control fanled by software */
    data = data & ~(0x13);
    data = data | led_status;
    outb(data, FAN_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}

/* COME CPLD version attributes */
static ssize_t come_cpld_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len = -EIO;
    // COME CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    len = sprintf(buf, "0x%2.2x\n",inb(COME_CPLD_VER_ADDR));
    mutex_unlock(&cpld_data->cpld_lock);
    return len;
}

static BIN_ATTR_RO(dump, CPLD_REGISTER_SIZE);
static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RW(scratch);
static DEVICE_ATTR_RW(getreg);
static DEVICE_ATTR_WO(setreg);
static DEVICE_ATTR_RW(sys_led);
static DEVICE_ATTR_RW(alarm_led);
static DEVICE_ATTR_RW(pwr_led);
static DEVICE_ATTR_RW(fan_led);
static DEVICE_ATTR_RO(come_cpld_version);

static struct attribute *cpld_b_attrs[] = {
	&dev_attr_version.attr,
	&dev_attr_scratch.attr,
	&dev_attr_getreg.attr,
	&dev_attr_setreg.attr,
    &dev_attr_sys_led.attr,
    &dev_attr_alarm_led.attr,
    &dev_attr_pwr_led.attr,
	&dev_attr_fan_led.attr,
    &dev_attr_come_cpld_version.attr,
	NULL,
};

static struct bin_attribute *cpld_b_bin_attrs[] = {
	&bin_attr_dump,
	NULL,
};

static struct attribute_group cpld_b_attrs_grp = {
	.attrs = cpld_b_attrs,
	.bin_attrs = cpld_b_bin_attrs,
};

static struct resource cpld_b_resources[] = {
	{
		.start  = 0xA100,
		.end	= 0xA1FF,
		.flags  = IORESOURCE_IO,
	},
};

static void cpld_b_dev_release(struct device *dev)
{
	return;
}

static struct platform_device cpld_b_dev = {
	.name			= DRIVER_NAME,
	.id				= -1,
	.num_resources	= ARRAY_SIZE(cpld_b_resources),
	.resource		= cpld_b_resources,
	.dev = {
		.release = cpld_b_dev_release,
	}
};

static int cpld_b_drv_probe(struct platform_device *pdev)
{
	struct resource *res;
	int err = 0;

	cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct cpld_b_data),
		GFP_KERNEL);
	if (!cpld_data)
		return -ENOMEM;

	mutex_init(&cpld_data->cpld_lock);

	cpld_data->read_addr = VERSION_ADDR;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (unlikely(!res)) {
		dev_err(&pdev->dev, "Specified Resource Not Available...\n");
		return -ENODEV;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &cpld_b_attrs_grp);
	if (err) {
		dev_err(&pdev->dev, "Cannot create sysfs for baseboard CPLD\n");
		return err;
	}
	return 0;
}

static void cpld_b_drv_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &cpld_b_attrs_grp);
}

static struct platform_driver cpld_b_drv = {
	.probe  = cpld_b_drv_probe,
	.remove = __exit_p(cpld_b_drv_remove),
	.driver = {
		.name   = DRIVER_NAME,
	},
};

int cpld_b_init(void)
{
	// Register platform device and platform driver
	platform_device_register(&cpld_b_dev);
	platform_driver_register(&cpld_b_drv);
	return 0;
}

void cpld_b_exit(void)
{
	// Unregister platform device and platform driver
	platform_driver_unregister(&cpld_b_drv);
	platform_device_unregister(&cpld_b_dev);
}

module_init(cpld_b_init);
module_exit(cpld_b_exit);


MODULE_AUTHOR("Celestica Inc.");
MODULE_DESCRIPTION("LPC CPLD baseboard driver");
MODULE_VERSION("0.0.2");
MODULE_LICENSE("GPL");

