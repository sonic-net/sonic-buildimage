/*
 * baseboard_cpld.c - driver for Seastone2 Base Board CPLD
 * This driver implement sysfs for CPLD register access using LPC bus.
 * Copyright (C) 2019 Celestica Corp.
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

#define DRIVER_NAME "baseboard"
/**
 * CPLD register address for read and write.
 */
#define VERSION_ADDR 0xA100
#define SCRATCH_ADDR 0xA101
#define SYS_LED_ADDR 0xA162

#define CPLD_REGISTER_SIZE 0x77

struct baseboard_cpld_data {
    struct mutex       cpld_lock;
    uint16_t           read_addr;
};

struct baseboard_cpld_data *cpld_data;

/**
 * Read the value from scratch register as hex string.
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         Hex string read from scratch register.
 */
static ssize_t scratch_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SCRATCH_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf,"0x%2.2x\n", data);
}

/**
 * Set scratch register with specific hex string.
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t scratch_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned long data;
    char *last;

    mutex_lock(&cpld_data->cpld_lock);
    data = (uint16_t)strtoul(buf,&last,16);
    if(data == 0 && buf == last){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }
    outb(data, SCRATCH_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(scratch);


/* CPLD version attributes */
static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len = -EIO;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    len = sprintf(buf, "0x%2.2x\n",inb(VERSION_ADDR));
    mutex_unlock(&cpld_data->cpld_lock);
    return len;
}
static DEVICE_ATTR_RO(version);


static ssize_t getreg_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    // CPLD register is one byte
    uint16_t addr;
    char *last;

    addr = (uint16_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        return -EINVAL;
    }
    cpld_data->read_addr = addr;
    return count;
}

static ssize_t getreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len = -EIO;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    len = sprintf(buf, "0x%2.2x\n",inb(cpld_data->read_addr));
    mutex_unlock(&cpld_data->cpld_lock);
    return len;
}
static DEVICE_ATTR_RW(getreg);

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

    strcpy(clone, buf);

    mutex_lock(&cpld_data->cpld_lock);
    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }
    addr = (uint16_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }

    tok = strsep((char**)&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }
    value = (uint8_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        mutex_unlock(&cpld_data->cpld_lock);
        return -EINVAL;
    }

    outb(value,addr);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_WO(setreg);

/**
 * Show system led status - on/off/1hz/4hz
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         Hex string read from scratch register.
 */
static ssize_t sys_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    data = data & 0x3;
    return sprintf(buf, "%s\n",
            data == 0x03 ? "off" : data == 0x02 ? "4hz" : data ==0x01 ? "1hz": "on");
}

/**
 * Set the status of system led - on/off/1hz/4hz
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t sys_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char led_status,data;
    if(sysfs_streq(buf, "off")){
        led_status = 0x03;
    }else if(sysfs_streq(buf, "4hz")){
        led_status = 0x02;
    }else if(sysfs_streq(buf, "1hz")){
        led_status = 0x01;
    }else if(sysfs_streq(buf, "on")){
        led_status = 0x00;
    }else{
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    data = data & ~(0x3);
    data = data | led_status;
    outb(data, SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(sys_led);

/**
 * Show system led color - both/green/yellow/none
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         Hex string read from scratch register.
 */
static ssize_t sys_led_color_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    data = (data >> 4) & 0x3;
    return sprintf(buf, "%s\n",
            data == 0x03 ? "off" : data == 0x02 ? "yellow" : data ==0x01 ? "green": "both");
}

/**
 * Set the color of system led - both/green/yellow/none
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t sys_led_color_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char led_status,data;
    if(sysfs_streq(buf, "off")){
        led_status = 0x03;
    }else if(sysfs_streq(buf, "yellow")){
        led_status = 0x02;
    }else if(sysfs_streq(buf, "green")){
        led_status = 0x01;
    }else if(sysfs_streq(buf, "both")){
        led_status = 0x00;
    }else{
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(SYS_LED_ADDR);
    data = data & ~( 0x3 << 4);
    data = data | (led_status << 4);
    outb(data, SYS_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(sys_led_color);

static struct attribute *baseboard_cpld_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_scratch.attr,
    &dev_attr_getreg.attr,
    &dev_attr_setreg.attr,
    &dev_attr_sys_led.attr,
    &dev_attr_sys_led_color.attr,
    NULL,
};

static struct attribute_group baseboard_cpld_attrs_grp = {
    .attrs = baseboard_cpld_attrs,
};

static struct resource baseboard_cpld_resources[] = {
    {
        .start  = 0xA100,
        .end    = 0xA1FF,
        .flags  = IORESOURCE_IO,
    },
};

static void baseboard_cpld_dev_release( struct device * dev)
{
    return;
}

static struct platform_device baseboard_cpld_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(baseboard_cpld_resources),
    .resource       = baseboard_cpld_resources,
    .dev = {
        .release = baseboard_cpld_dev_release,
    }
};

static int baseboard_cpld_drv_probe(struct platform_device *pdev)
{
    struct resource *res;
    int ret =0;

    cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct baseboard_cpld_data),
        GFP_KERNEL);
    if (!cpld_data)
        return -ENOMEM;

    mutex_init(&cpld_data->cpld_lock);

    cpld_data->read_addr = VERSION_ADDR;

    res = platform_get_resource(pdev, IORESOURCE_IO, 0);
    if (unlikely(!res)) {
        printk(KERN_ERR "Specified Resource Not Available...\n");
        return -1;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &baseboard_cpld_attrs_grp);
    if (ret) {
        printk(KERN_ERR "Cannot create sysfs for baseboard CPLD\n");
    }
    return 0;
}

static int baseboard_cpld_drv_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &baseboard_cpld_attrs_grp);
    return 0;
}

static struct platform_driver baseboard_cpld_drv = {
    .probe  = baseboard_cpld_drv_probe,
    .remove = __exit_p(baseboard_cpld_drv_remove),
    .driver = {
        .name   = DRIVER_NAME,
    },
};

int baseboard_cpld_init(void)
{
    // Register platform device and platform driver
    platform_device_register(&baseboard_cpld_dev);
    platform_driver_register(&baseboard_cpld_drv);
    return 0;
}

void baseboard_cpld_exit(void)
{
    // Unregister platform device and platform driver
    platform_driver_unregister(&baseboard_cpld_drv);
    platform_device_unregister(&baseboard_cpld_dev);
}

module_init(baseboard_cpld_init);
module_exit(baseboard_cpld_exit);

MODULE_AUTHOR("Pradchaya Phucharoen  <pphuchar@celestica.com>");
MODULE_DESCRIPTION("Celestica Seastone2 Baseboard CPLD Driver");
MODULE_LICENSE("GPL");