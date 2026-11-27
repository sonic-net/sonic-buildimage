/*
 * cpld_b .c - The CPLD driver for the Base Board of ds2010
 * The driver implement sysfs to access CPLD register on the baseboard of ds2010 via LPC bus.
 * Copyright (C) 2025 Celestica Corp.
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
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>

#define DRIVER_NAME "sys_cpld1"
/**
 * CPLD1 register address for read and write.
 */
#define VERSION_ADDR 0xA200
#define SCRATCH_ADDR 0xA201
#define BMC_PRESENT_ADDR 0xA206
#define SYS_LED_ADDR 0xA240
#define ALARM_LED_ADDR 0xA244
#define FAN_LED_ADDR 0xA245
#define QSFP_PRESENT_ADDR 0xA2A1
#define QSFP_LPMODE_ADDR 0xA2A2
#define QSFP_RESET_ADDR 0xA22F
#define QSFP_INTR_STATUS_ADDR 0xA225

#define CPLD_REGISTER_SIZE 0xFF

struct cpld_b_data {
    struct mutex       cpld_lock;
    uint16_t           read_addr;
};

struct cpld_b_data *cpld_data;

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
    unsigned char value = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    value = inb(VERSION_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return sprintf(buf, "%d.%d\n", (value >> 4) & 0x0F, value & 0x0F);
}
static DEVICE_ATTR_RO(version);


/* BMC Present Status */
static ssize_t bmc_present_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char value = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    value = inb(BMC_PRESENT_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    value = (value & 0x04) ? 0 : 1;
    return sprintf(buf, "%d\n", value);
}
static DEVICE_ATTR_RO(bmc_present);


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
    int len = 0;
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
    char *clone, *pclone;
    char *last;

    clone = kmalloc(count + 1, GFP_KERNEL);
    if (!clone) {
        return -ENOMEM;
    }

    strscpy(clone, buf, count + 1);
    pclone = clone;

    mutex_lock(&cpld_data->cpld_lock);
    tok = strsep(&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(clone);
        return -EINVAL;
    }
    addr = (uint16_t)strtoul(tok,&last,16);
    if(addr == 0 && tok == last){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(clone);
        return -EINVAL;
    }

    tok = strsep(&pclone, " ");
    if(tok == NULL){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(clone);
        return -EINVAL;
    }
    value = (uint8_t)strtoul(tok,&last,16);
    if(value == 0 && tok == last){
        mutex_unlock(&cpld_data->cpld_lock);
        kfree(clone);
        return -EINVAL;
    }

    outb(value,addr);
    mutex_unlock(&cpld_data->cpld_lock);
    kfree(clone);
    return count;
}
static DEVICE_ATTR_WO(setreg);

/**
 * Read all CPLD register in binary mode.
 * @return number of byte read.
 */
static ssize_t dump_read(struct file *filp, struct kobject *kobj,
                struct bin_attribute *attr, char *buf,
                loff_t off, size_t count)
{
    unsigned long i=0;

    mutex_lock(&cpld_data->cpld_lock);

   for(i = 0; i < count; i++){
        buf[i] = inb(VERSION_ADDR + off + i);
        msleep(1);
    }
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static BIN_ATTR_RO(dump, CPLD_REGISTER_SIZE);

/**
 * Show system led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         system led color (string).
 */
static ssize_t sys_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    char *led_color = "unknown";
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = (inb(SYS_LED_ADDR)) & 0x33;
    mutex_unlock(&cpld_data->cpld_lock);

    if (data == 0x33) {
        led_color = "off";
    } else if (data == 0x10) {
        led_color = "amber";
    } else if (data == 0x20) {
        led_color = "green";
    } else if (data == 0x01) {
        led_color = "alternate_blink_1hz";
    } else if (data == 0x02) {
        led_color = "alternate_blink_4hz";
    } else if (data == 0x11) {
        led_color = "amber_blink_1hz";
    } else if (data == 0x12) {
        led_color = "amber_blink_4hz";
    } else if (data == 0x21) {
        led_color = "green_blink_1hz";
    } else if (data == 0x22) {
        led_color = "green_blink_4hz";
    }

    return sprintf(buf, "%s\n", led_color);
}

/**
 * Set the system led
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
    if (sysfs_streq(buf, "off")) {
        led_status = 0x33;
    } else if (sysfs_streq(buf, "amber")) {
        led_status = 0x10;
    } else if (sysfs_streq(buf, "green")) {
        led_status = 0x20;
    } else if (sysfs_streq(buf, "alternate_blink_1hz")) {
        led_status = 0x01;
    } else if (sysfs_streq(buf, "alternate_blink_4hz")) {
        led_status = 0x02;
    } else if (sysfs_streq(buf, "amber_blink_1hz")) {
        led_status = 0x11;
    } else if (sysfs_streq(buf, "amber_blink_4hz")) {
        led_status = 0x12;
    } else if (sysfs_streq(buf, "green_blink_1hz")) {
        led_status = 0x21;
    } else if (sysfs_streq(buf, "green_blink_4hz")) {
        led_status = 0x22;
    } else {
        count = -EINVAL;
        return count;
    }

    mutex_lock(&cpld_data->cpld_lock);
    data = (inb(SYS_LED_ADDR)) & 0x33;
    if (data != led_status) {
        outb(led_status, SYS_LED_ADDR);
    }

    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(sys_led);

/**
 * Show alarm led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         alarm led color (string).
 */
static ssize_t alarm_led_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    char *led_color = "unknown";
    unsigned char data = 0;
    mutex_lock(&cpld_data->cpld_lock);
    data = (inb(ALARM_LED_ADDR)) & 0x33;
    mutex_unlock(&cpld_data->cpld_lock);

    if (data == 0x10) {
        led_color = "amber";
    } else if (data == 0x20) {
        led_color = "green";
    } else if (data == 0x11) {
        led_color = "amber_blink_1hz";
    } else if (data == 0x12) {
        led_color = "amber_blink_4hz";
    }else {
        led_color = "off";
    }

    return sprintf(buf, "%s\n", led_color);
}

/**
 * Set the alarm led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t alarm_led_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    unsigned char led_status,data;
    if (sysfs_streq(buf, "off")) {
        led_status = 0x33;
    } else if (sysfs_streq(buf, "amber")) {
        led_status = 0x10;
    } else if (sysfs_streq(buf, "green")) {
        led_status = 0x20;
    } else if (sysfs_streq(buf, "amber_blink_1hz")) {
        led_status = 0x11;
    } else if (sysfs_streq(buf, "amber_blink_4hz")) {
        led_status = 0x12;
    } else {
        count = -EINVAL;
        return count;
    }

    mutex_lock(&cpld_data->cpld_lock);
    data = (inb(ALARM_LED_ADDR)) & 0x33;
    if (data != led_status) {
        outb(led_status, ALARM_LED_ADDR);
    }

    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}
static DEVICE_ATTR_RW(alarm_led);

/**
 * Show the front panel fan led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer for get value
 * @return         fan led color (string).
 */
static ssize_t fan_led_show(struct device *dev,
				struct device_attribute *devattr,
				char *buf)
{
    unsigned char data = 0;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    if ((data & 0x10) != 0)
        return sprintf(buf, "%s\n", "auto");
    data = data & 0x3;
    return sprintf(buf, "%s\n", data == 0x2 ? "green" :
				 data == 0x1 ? "amber" : "off");
}

/**
 * Set the front panel fan led
 * @param  dev     kernel device
 * @param  devattr kernel device attribute
 * @param  buf     buffer of set value
 * @param  count   number of bytes in buffer
 * @return         number of bytes written, or error code < 0.
 */
static ssize_t fan_led_store(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
    unsigned char led_status, data;

    if (sysfs_streq(buf, "off"))
        led_status = 0x3;
    else if (sysfs_streq(buf, "green"))
        led_status = 0x2;
    else if (sysfs_streq(buf, "amber"))
        led_status = 0x1;
    else if (sysfs_streq(buf, "auto"))
        led_status = 0x10;
    else {
        count = -EINVAL;
        return count;
    }
    mutex_lock(&cpld_data->cpld_lock);
    data = inb(FAN_LED_ADDR);
    /* Set bit 4 as 0 to control fan led by software */
    data = data & ~(0x13);
    data = data | led_status;
    outb(data, FAN_LED_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);

    return count;
}
static DEVICE_ATTR_RW(fan_led);

static ssize_t qsfp_present_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    int index = s_attr->index;
    unsigned char value = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    value = inb(QSFP_PRESENT_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    value = (value >> (index - 1)) & 0x1;
    return sprintf(buf, "%d\n", value);
}

static ssize_t qsfp_lpmode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    int index = s_attr->index;
    unsigned char value = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    value = inb(QSFP_LPMODE_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    value = (value >> (index - 1)) & 0x1;
    return sprintf(buf, "%d\n", value);
}


static ssize_t qsfp_lpmode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    int index = s_attr->index;
    long value;
    int ret;
    unsigned char data;

   ret = kstrtol(buf, 0, &value);
    if (ret)
        return ret;

    if (value != 0 && value != 1)
        return -EINVAL;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(QSFP_LPMODE_ADDR);
    if (value)
        data |= (1 << (index - 1));
    else
        data &= ~(1 << (index - 1));
    outb(data, QSFP_LPMODE_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}

static ssize_t qsfp_reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    int index = s_attr->index;
    unsigned char value = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    value = inb(QSFP_RESET_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    value = (value >> (index - 1)) & 0x1;
    return sprintf(buf, "%d\n", value);
}

static ssize_t qsfp_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    int index = s_attr->index;
    long value;
    int ret;
    unsigned char data;

   ret = kstrtol(buf, 0, &value);
    if (ret)
        return ret;

    if (value != 0 && value != 1)
        return -EINVAL;

    mutex_lock(&cpld_data->cpld_lock);
    data = inb(QSFP_RESET_ADDR);
    if (value)
        data |= (1 << (index - 1));
    else
        data &= ~(1 << (index - 1));
    outb(data, QSFP_RESET_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    return count;
}

static ssize_t qsfp_intr_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    int index = s_attr->index;
    unsigned char value = 0;
    // CPLD register is one byte
    mutex_lock(&cpld_data->cpld_lock);
    value = inb(QSFP_INTR_STATUS_ADDR);
    mutex_unlock(&cpld_data->cpld_lock);
    value = (value >> (index - 1)) & 0x1;
    return sprintf(buf, "%d\n", value);
}

#define QSFP_PRESENT_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(qsfp##index##_present, S_IRUGO,  qsfp_present_show, NULL, index);
#define QSFP_LPMODE_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(qsfp##index##_lpmode, S_IWUSR | S_IRUGO,  qsfp_lpmode_show, qsfp_lpmode_store, index);
#define QSFP_RESET_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(qsfp##index##_reset, S_IWUSR | S_IRUGO,  qsfp_reset_show, qsfp_reset_store, index);
#define QSFP_INTR_STATUS_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(qsfp##index##_intr_status, S_IRUGO,  qsfp_intr_status_show, NULL, index);

#define FOR_EACH_QSFP_PORT(X) \
    X(1) X(2) X(3) X(4) X(5) X(6)

FOR_EACH_QSFP_PORT(QSFP_PRESENT_DEVICE_ATTR);
FOR_EACH_QSFP_PORT(QSFP_LPMODE_DEVICE_ATTR);
FOR_EACH_QSFP_PORT(QSFP_RESET_DEVICE_ATTR);
FOR_EACH_QSFP_PORT(QSFP_INTR_STATUS_DEVICE_ATTR);

#define LIST_QSFP_PRESENT_ATTR(index) &sensor_dev_attr_qsfp##index##_present.dev_attr.attr,
#define LIST_QSFP_LPMODE_ATTR(index) &sensor_dev_attr_qsfp##index##_lpmode.dev_attr.attr,
#define LIST_QSFP_RESET_ATTR(index) &sensor_dev_attr_qsfp##index##_reset.dev_attr.attr,
#define LIST_QSFP_INTR_STATUS_ATTR(index) &sensor_dev_attr_qsfp##index##_intr_status.dev_attr.attr,

static struct attribute *cpld_b_attrs[] = {
    &dev_attr_bmc_present.attr,
    &dev_attr_version.attr,
    &dev_attr_scratch.attr,
    &dev_attr_getreg.attr,
    &dev_attr_setreg.attr,
    &dev_attr_sys_led.attr,
    &dev_attr_alarm_led.attr,
    &dev_attr_fan_led.attr,
    FOR_EACH_QSFP_PORT(LIST_QSFP_PRESENT_ATTR)
    FOR_EACH_QSFP_PORT(LIST_QSFP_LPMODE_ATTR)
    FOR_EACH_QSFP_PORT(LIST_QSFP_RESET_ATTR)
    FOR_EACH_QSFP_PORT(LIST_QSFP_INTR_STATUS_ATTR)
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
        .start  = 0xA200,
        .end    = 0xA2FF,
        .flags  = IORESOURCE_IO,
    },
};

static void cpld_b_dev_release( struct device * dev)
{
    return;
}

static struct platform_device cpld_b_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cpld_b_resources),
    .resource       = cpld_b_resources,
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
        printk(KERN_ERR "Specified Resource Not Available...\n");
        return -ENODEV;
    }

    err = sysfs_create_group(&pdev->dev.kobj, &cpld_b_attrs_grp);
    if (err) {
        printk(KERN_ERR "Cannot create sysfs for baseboard CPLD\n");
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
MODULE_DESCRIPTION("Celestica DS2010 LPC Baseboard CPLD Driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
