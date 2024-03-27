/*
 * redstone_xp_cpld.c - Driver for Redstone XP CPLD
 *
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
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>

#define DRIVER_NAME "redstone_xp_cpld"

#define CPLD1_VERSION_ADDR 0x100
#define CPLD2_VERSION_ADDR 0x200
#define CPLD3_VERSION_ADDR 0x280
#define CPLD4_VERSION_ADDR 0x300
#define CPLD5_VERSION_ADDR 0x380

#define FRONT_LED_STAT  0x303

// SFP
#define ABS0108             0x259
#define ABS0916             0x25A
#define ABS1718             0x25B
#define ABS1926             0x2D9
#define ABS2734             0x2DA
#define ABS3536             0x2DB
#define ABS3744             0x3D6
#define ABS4548             0x3D7

// QSFP Options
#define ABS4954             0x362
#define ABS_INT4954         0x363
#define RESET4954           0x360
#define LPMOD4954           0x361
#define INT4954             0x305

#define CPLD4_INT0          0x309
#define CPLD4_INT0_MSK      0x30C

#define CPLD_RAM_ADDR_HIGH_BYTE_REG (0x130)
#define CPLD_RAM_ADDR_LOW_BYTE_REG 	(0x131)
#define CPLD_RAM_READ_REG 			(0x132)

struct redstone_xp_cpld_data {
        struct mutex cpld_lock;
        uint16_t     read_addr;
};

struct redstone_xp_cpld_data *cpld_data;

static ssize_t getreg_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) {
    uint16_t addr;
    char *last;

    addr = (uint16_t)strtoul(buf,&last,16);
    if(addr == 0 && buf == last){
        return -EINVAL;
    }
    cpld_data->read_addr = addr;
    return count;
};


static ssize_t getreg_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int len = 0;

    mutex_lock(&cpld_data->cpld_lock);
    len = sprintf(buf, "0x%2.2x\n",inb(cpld_data->read_addr));
    mutex_unlock(&cpld_data->cpld_lock);
    return len;
};


static ssize_t setreg_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) {
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
};

static DEVICE_ATTR_RW(getreg);
static DEVICE_ATTR_WO(setreg);


static ssize_t get_presence(struct device *dev, struct device_attribute *devattr, char *buf) {
        unsigned long present;

        mutex_lock(&cpld_data->cpld_lock);

        present = 0;

        present = inb(ABS4954) & 0x3F;
        present = (present << 4) | (inb(ABS4548) & 0x0F);
        present = (present << 8) | inb(ABS3744);
        present = (present << 2) | (inb(ABS3536) & 0x03);
        present = (present << 8) | inb(ABS2734);
        present = (present << 8) | inb(ABS1926);
        present = (present << 2) | (inb(ABS1718) & 0x03);
        present = (present << 8) | inb(ABS0916);
        present = (present << 8) | inb(ABS0108);

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%14.14lx\n", present);
};

static DEVICE_ATTR(port_presence, S_IRUGO, get_presence, NULL);


static ssize_t get_modirq(struct device *dev, struct device_attribute *devattr, char *buf) {
        unsigned long irq;

        mutex_lock(&cpld_data->cpld_lock);

        irq = inb(INT4954);

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%2.2lx\n", irq & 0x3F);
};

static DEVICE_ATTR(qsfp_modirq, S_IRUGO, get_modirq, NULL);


static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) {
        unsigned long reset = 0;

        mutex_lock(&cpld_data->cpld_lock);

        reset = inb(RESET4954);

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%2.2lx\n", reset & 0x3F);
};

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) {
        unsigned long reset;
        int err;

        mutex_lock(&cpld_data->cpld_lock);

        err = kstrtoul(buf, 16, &reset);
        if (err)
        {
                mutex_unlock(&cpld_data->cpld_lock);
                return err;
        }

        outb(((reset >> 0)  & 0x3F), RESET4954);


        mutex_unlock(&cpld_data->cpld_lock);

        return count;
};

static DEVICE_ATTR(qsfp_reset, S_IRUGO | S_IWUSR, get_reset, set_reset);


// QSFP Low Power mode
static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) {
        unsigned long lpmod = 0;

        mutex_lock(&cpld_data->cpld_lock);

        lpmod = inb(LPMOD4954);

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%2.2lx\n", lpmod & 0x3F);
};

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) {
        unsigned long lpmod;
        int err;

        mutex_lock(&cpld_data->cpld_lock);

        err = kstrtoul(buf, 16, &lpmod);
        if (err)
        {
                mutex_unlock(&cpld_data->cpld_lock);
                return err;
        }

        outb(((lpmod >> 0) & 0x3F), LPMOD4954);

        mutex_unlock(&cpld_data->cpld_lock);

        return count;
};

static DEVICE_ATTR(qsfp_lpmode, S_IRUGO | S_IWUSR, get_lpmode, set_lpmode);


// CPLD4 INT0
static ssize_t get_cpld4_int0(struct device *dev, struct device_attribute *devattr, char *buf) {
        unsigned char int0 = 0;

        mutex_lock(&cpld_data->cpld_lock);

        int0 = inb(CPLD4_INT0);

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%2.2x\n", int0 & 0xff);
};

static DEVICE_ATTR(cpld4_int0, S_IRUGO, get_cpld4_int0, NULL);


static ssize_t get_cpld4_int0_msk(struct device *dev, struct device_attribute *devattr, char *buf) {
        unsigned char int0_msk = 0;

        mutex_lock(&cpld_data->cpld_lock);

        int0_msk = inb(CPLD4_INT0_MSK);

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%2.2x\n", int0_msk & 0xff);
};

static ssize_t set_cpld4_int0_msk(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) {
        unsigned long int0_msk;
        int err;

        mutex_lock(&cpld_data->cpld_lock);

        err = kstrtoul(buf, 16, &int0_msk);
        if (err)
        {
                mutex_unlock(&cpld_data->cpld_lock);
                return err;
        }

        outb(int0_msk & 0x3f, CPLD4_INT0_MSK);

        mutex_unlock(&cpld_data->cpld_lock);

        return count;
};

static DEVICE_ATTR(cpld4_int0_msk, S_IRUGO | S_IWUSR, get_cpld4_int0_msk, set_cpld4_int0_msk);


static ssize_t status_led_get(struct device *dev, struct device_attribute *attr, char *buf) {
    unsigned int reg;

    mutex_lock(&cpld_data->cpld_lock);

    reg = 255; // default
    reg = inb(FRONT_LED_STAT); // get register value
    // 0 - Off
    // 1 - Fast Blink
    // 2 - Slow Blink
    // 3 - Solid On

    reg &= 0x03;
    reg ^= 0x03;

    mutex_unlock(&cpld_data->cpld_lock);

    return sprintf(buf, "%u\n", reg);
};

static ssize_t status_led_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    unsigned long mode;
    int err;

    mutex_lock(&cpld_data->cpld_lock);

    err = kstrtoul(buf, 16, &mode);
    if (err) {
        mutex_unlock(&cpld_data->cpld_lock);
        return err;
    }

    outb(((inb(FRONT_LED_STAT) >> 2) << 2) | (mode ^ 3), FRONT_LED_STAT);

    mutex_unlock(&cpld_data->cpld_lock);
    return count;
};

static DEVICE_ATTR(status_led, S_IWUSR | S_IRUGO, status_led_get, status_led_set);


static ssize_t psu1_get(struct device *dev, struct device_attribute *attr, char *buf){
    unsigned int reg;

    mutex_lock(&cpld_data->cpld_lock);

    reg = 255; // default
    reg = inb(FRONT_LED_STAT); // get register value

    reg = reg >> 2;
    reg &= 0x01;
    reg ^= 0x01;

    mutex_unlock(&cpld_data->cpld_lock);

    return sprintf(buf, "%u\n", reg);
};

static ssize_t psu1_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    unsigned long mode;
    int err;

    mutex_lock(&cpld_data->cpld_lock);

    err = kstrtoul(buf, 16, &mode);
    if (err) {
        mutex_unlock(&cpld_data->cpld_lock);
        return err;
    }

    mode = mode > 0 ? 1 : 0;

    outb((((inb(FRONT_LED_STAT)) & ~(0x01 << 2) | ((0x01 ^ mode)) << 2)), FRONT_LED_STAT);

    mutex_unlock(&cpld_data->cpld_lock);
    return count;
};

static DEVICE_ATTR(psu1_led, S_IRUGO | S_IWUSR, psu1_get, psu1_set);


static ssize_t psu2_get(struct device *dev, struct device_attribute *attr, char *buf){
    unsigned int reg;

    mutex_lock(&cpld_data->cpld_lock);

    reg = 255;
    reg = inb(FRONT_LED_STAT);

    reg = reg >> 3;
    reg &= 0x01;
    reg ^= 0x01;

    mutex_unlock(&cpld_data->cpld_lock);

    return sprintf(buf, "%u\n", reg);
};

static ssize_t psu2_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    unsigned long mode;
    int err;

    mutex_lock(&cpld_data->cpld_lock);

    err = kstrtoul(buf, 16, &mode);
    if (err) {
        mutex_unlock(&cpld_data->cpld_lock);
        return err;
    }

    mode = mode > 0 ? 1 : 0;

    outb((((inb(FRONT_LED_STAT)) & ~(0x01 << 3) | ((0x01 ^ mode)) << 3)), FRONT_LED_STAT);

    mutex_unlock(&cpld_data->cpld_lock);
    return count;
};

static DEVICE_ATTR(psu2_led, S_IRUGO | S_IWUSR, psu2_get, psu2_set);


static struct attribute *redstone_xp_cpld_attrs[] = {
        &dev_attr_getreg.attr,
        &dev_attr_setreg.attr,
        &dev_attr_port_presence.attr,
        &dev_attr_qsfp_modirq.attr,
        &dev_attr_qsfp_reset.attr,
        &dev_attr_qsfp_lpmode.attr,
        &dev_attr_cpld4_int0.attr,
        &dev_attr_cpld4_int0_msk.attr,
        &dev_attr_status_led.attr,
        &dev_attr_psu1_led.attr,
        &dev_attr_psu2_led.attr,
        NULL,
};

static struct attribute_group redstone_xp_cpld_attr_grp = {
        .attrs = redstone_xp_cpld_attrs,
};

static struct resource redstone_xp_cpld_resources[] = {
        {
                .flags  = IORESOURCE_IO,
        },
};

static void redstone_xp_cpld_dev_release( struct device * dev) {
        return;
};

static struct platform_device redstone_xp_cpld_dev = {
        .name           = DRIVER_NAME,
        .id             = -1,
        .num_resources  = ARRAY_SIZE(redstone_xp_cpld_resources),
        .resource       = redstone_xp_cpld_resources,
        .dev = {
                        .release = redstone_xp_cpld_dev_release,
        }
};

static int redstone_xp_cpld_drv_probe(struct platform_device *pdev)
{
        struct resource *res;
        int ret = 0;

        cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct redstone_xp_cpld_data),
                        GFP_KERNEL);
        if (!cpld_data)
                return -ENOMEM;

        mutex_init(&cpld_data->cpld_lock);
        cpld_data->read_addr = CPLD1_VERSION_ADDR;

        res = platform_get_resource(pdev, IORESOURCE_IO, 0);
        if (unlikely(!res)) {
                printk(KERN_ERR " Specified Resource Not Available...\n");
                return -1;
        }

        ret = sysfs_create_group(&pdev->dev.kobj, &redstone_xp_cpld_attr_grp);
        if (ret) {
                printk(KERN_ERR "Cannot create sysfs\n");
        }

        /* Enable INT0 interrupt register */
        outb(inb(CPLD4_INT0_MSK) & 0xf8, CPLD4_INT0_MSK);

        return 0;
};

static int redstone_xp_cpld_drv_remove(struct platform_device *pdev) {

        sysfs_remove_group(&pdev->dev.kobj, &redstone_xp_cpld_attr_grp);

        return 0;
};

static struct platform_driver redstone_xp_cpld_drv = {
        .probe  = redstone_xp_cpld_drv_probe,
        .remove = __exit_p(redstone_xp_cpld_drv_remove),
        .driver = {
        .name   = DRIVER_NAME,
        },
};

int redstone_xp_cpld_init(void) {
        platform_device_register(&redstone_xp_cpld_dev);
        platform_driver_register(&redstone_xp_cpld_drv);

        return 0;
};

void redstone_xp_cpld_exit(void) {
        platform_driver_unregister(&redstone_xp_cpld_drv);
        platform_device_unregister(&redstone_xp_cpld_dev);
};

module_init(redstone_xp_cpld_init);
module_exit(redstone_xp_cpld_exit);

MODULE_AUTHOR("Derek Vance <dacrepair@gmail.com>");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Celestica Redstone XP D2060 CPLD Driver");
MODULE_LICENSE("GPL");