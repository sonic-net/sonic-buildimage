// SPDX-License-Identifier: GPL-2.0+
/*
 * The CPLD driver for the Base Board of DS6000.
 * The driver implement sysfs to access CPLD register via LPC bus.
 *
 * Copyright (C) 2026 Celestica Inc.
 *
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DRIVER_NAME "sys_cpld"

/* CPLD register address for read and write */
#define VERSION_ADDR 0xA00
#define SCRATCH_ADDR 0xA01
#define CARD_PRS_ADDR 0xA08
#define PSU_LED_ADDR 0xA61
#define SYS_LED_ADDR 0xA62
#define ALARM_LED_ADDR 0xA63
#define FAN_LED_ADDR 0xA65

#define CPLD_REGISTER_SIZE 0xAA

/* LED CTRL */
enum FAN_LED {
	fan_led_amb = 1,
	fan_led_grn,
	fan_led_off,
	fan_led_auto = 0x10
} fan_led;

enum SYS_LED {
	sys_led_on = 0,
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
	alarm_led_on = 0,
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

#define LED_OFF "off"
#define LED_GREEN "green"
#define LED_AMBER "amber"
#define LED_HZ_GBNK "grn_bnk_1hz"
#define LED_HZ_ABNK "amb_bnk_1hz"
#define LED_QHZ_GBNK "grn_bnk_4hz"
#define LED_QHZ_ABNK "amb_bnk_4hz"
#define LED_HZ_GABNK "grn_amb_1hz"
#define LED_QHZ_GABNK "grn_amb_4hz"

struct cpld_b_data {
	struct mutex cpld_lock;
	uint16_t read_addr;
};

struct cpld_b_data *cpld_data;

static ssize_t scratch_show(struct device *dev,
			    struct device_attribute *devattr, char *buf)
{
	unsigned char data = 0;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(SCRATCH_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);

	return sysfs_emit(buf, "0x%2.2x\n", data);
}

static ssize_t scratch_store(struct device *dev,
			     struct device_attribute *devattr, const char *buf,
			     size_t count)
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

static ssize_t version_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int len = 0;
	unsigned char value = 0;

	mutex_lock(&cpld_data->cpld_lock);
	value = inb(VERSION_ADDR);
	len = sysfs_emit(buf, "%d.%d\n", value >> 4, value & 0x0F);
	mutex_unlock(&cpld_data->cpld_lock);

	return len;
}

/**
 * getreg_store - Spefical function to get the CPLD register address to read.
 *
 * @dev: kerenl device
 * @devattr: kernel device attribute
 * @buf: The address to read
 * @count: size of buf
 *
 * Return: size of data received, or negative error code.
 */
static ssize_t getreg_store(struct device *dev,
			    struct device_attribute *devattr, const char *buf,
			    size_t count)
{
	int ret = 0;
	unsigned long addr;

	ret = kstrtoul(buf, 0, &addr);
	if (ret != 0)
		return ret;
	cpld_data->read_addr = addr;

	return count;
}

/**
 * getreg_show - Spefical function to show the CPLD register value.
 *
 * The register address is specified by getreg_store function.
 *
 * @dev: kerenl device
 * @devattr: kernel device attribute
 * @buf: The buffer to store read value to user space
 *
 * Return: size of data, or negative error code.
 */
static ssize_t getreg_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	int len = 0;

	mutex_lock(&cpld_data->cpld_lock);
	len = sysfs_emit(buf, "0x%2.2x\n", inb(cpld_data->read_addr));
	mutex_unlock(&cpld_data->cpld_lock);

	return len;
}

/**
 * setreg_store - Special function to set the CPLD register value.
 *
 * @dev: kerenl device
 * @devattr: kernel device attribute
 * @buf: The buffer contains address and value.
 *       The format is "<address> <value>"
 * @count: size of buf
 *
 * Return: size of data, or negative error code.
 */
static ssize_t setreg_store(struct device *dev,
			    struct device_attribute *devattr, const char *buf,
			    size_t count)
{
	int ret = 0;
	unsigned long addr;
	unsigned long value;
	char *tok;
	char *clone;
	char *pclone;

	clone = kstrndup(buf, count, GFP_KERNEL);
	if (!clone)
		return -ENOMEM;

	pclone = clone;

	mutex_lock(&cpld_data->cpld_lock);
	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&cpld_data->cpld_lock);
		kfree(clone);
		return -EINVAL;
	}
	ret = kstrtoul(tok, 0, &addr);
	if (ret != 0) {
		mutex_unlock(&cpld_data->cpld_lock);
		kfree(clone);
		return ret;
	}

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&cpld_data->cpld_lock);
		kfree(clone);
		return -EINVAL;
	}
	ret = kstrtoul(tok, 0, &value);
	if (ret != 0) {
		mutex_unlock(&cpld_data->cpld_lock);
		kfree(clone);
		return ret;
	}

	outb(value, addr);
	mutex_unlock(&cpld_data->cpld_lock);
	kfree(clone);

	return count;
}

static ssize_t sys_led_show(struct device *dev,
			    struct device_attribute *devattr, char *buf)
{
	unsigned char data = 0;
	unsigned char color = 0;
	unsigned char control = 0;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(SYS_LED_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);
	color = (data & 0x30) >> 4;
	control = (data & 0x3);

	switch (color) {
	case sys_led_on:
		if (control == led_blk_1hz)
			return sysfs_emit(buf, "%s\n", LED_HZ_GABNK);
		else if (control == led_blk_4hz)
			return sysfs_emit(buf, "%s\n", LED_QHZ_GABNK);
		break;
	case sys_led_amb:
		if (control == led_blk_1hz)
			return sysfs_emit(buf, "%s\n", LED_HZ_ABNK);
		else if (control == led_blk_4hz)
			return sysfs_emit(buf, "%s\n", LED_QHZ_ABNK);
		else if (control == led_on)
			return sysfs_emit(buf, "%s\n", LED_AMBER);
		break;
	case sys_led_grn:
		if (control == led_blk_1hz)
			return sysfs_emit(buf, "%s\n", LED_HZ_GBNK);
		else if (control == led_blk_4hz)
			return sysfs_emit(buf, "%s\n", LED_QHZ_GBNK);
		else if (control == led_on)
			return sysfs_emit(buf, "%s\n", LED_GREEN);
		break;
	default:
		break;
	}

	return sysfs_emit(buf, "%s\n", LED_OFF);
}

static ssize_t sys_led_store(struct device *dev,
			     struct device_attribute *devattr, const char *buf,
			     size_t count)
{
	unsigned char data;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(SYS_LED_ADDR);
	if (sysfs_streq(buf, LED_OFF)) {
		data &= 0xCF;
		data = sys_led_off << 4;
		data += led_off;
	} else if (sysfs_streq(buf, LED_GREEN)) {
		data &= 0xCF;
		data = sys_led_grn << 4;
		data &= 0xFC;
	} else if (sysfs_streq(buf, LED_AMBER)) {
		data &= 0xCF;
		data = sys_led_amb << 4;
		data &= 0xFC;
	} else if (sysfs_streq(buf, LED_HZ_GBNK)) {
		data &= 0xCF;
		data = sys_led_grn << 4;
		data += led_blk_1hz;
	} else if (sysfs_streq(buf, LED_HZ_ABNK)) {
		data &= 0xCF;
		data = sys_led_amb << 4;
		data += led_blk_1hz;
	} else if (sysfs_streq(buf, LED_QHZ_GBNK)) {
		data &= 0xCF;
		data = sys_led_grn << 4;
		data += led_blk_4hz;
	} else if (sysfs_streq(buf, LED_QHZ_ABNK)) {
		data &= 0xCF;
		data = sys_led_amb << 4;
		data += led_blk_4hz;
	} else if (sysfs_streq(buf, LED_HZ_GABNK)) {
		data &= 0xCF;
		data = sys_led_on << 4;
		data += led_blk_1hz;
	} else if (sysfs_streq(buf, LED_QHZ_GABNK)) {
		data &= 0xCF;
		data = sys_led_on << 4;
		data += led_blk_4hz;
	} else
		count = -EINVAL;

	outb(data, SYS_LED_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);

	return count;
}

static ssize_t alarm_led_show(struct device *dev,
			      struct device_attribute *devattr, char *buf)
{
	unsigned char data = 0;
	unsigned char color = 0;
	unsigned char control = 0;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(ALARM_LED_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);
	color = (data & 0x30) >> 4;
	control = (data & 0x3);

	switch (color) {
	case alarm_led_on:
		if (control == led_blk_1hz)
			return sysfs_emit(buf, "%s\n", LED_HZ_GABNK);
		else if (control == led_blk_4hz)
			return sysfs_emit(buf, "%s\n", LED_QHZ_GABNK);
		break;
	case alarm_led_amb:
		if (control == led_blk_1hz)
			return sysfs_emit(buf, "%s\n", LED_HZ_ABNK);
		else if (control == led_blk_4hz)
			return sysfs_emit(buf, "%s\n", LED_QHZ_ABNK);
		else if (control == led_on)
			return sysfs_emit(buf, "%s\n", LED_AMBER);
		break;
	case alarm_led_grn:
		if (control == led_blk_1hz)
			return sysfs_emit(buf, "%s\n", LED_HZ_GBNK);
		else if (control == led_blk_4hz)
			return sysfs_emit(buf, "%s\n", LED_QHZ_GBNK);
		else if (control == led_on)
			return sysfs_emit(buf, "%s\n", LED_GREEN);
		break;
	default:
		break;
	}

	return sysfs_emit(buf, "%s\n", LED_OFF);
}

static ssize_t alarm_led_store(struct device *dev,
			       struct device_attribute *devattr,
			       const char *buf, size_t count)
{
	unsigned char data;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(ALARM_LED_ADDR);
	if (sysfs_streq(buf, LED_OFF)) {
		data &= 0xCF;
		data = alarm_led_off << 4;
		data += led_off;
	} else if (sysfs_streq(buf, LED_GREEN)) {
		data &= 0xCF;
		data = alarm_led_grn << 4;
		data &= 0xFC;
	} else if (sysfs_streq(buf, LED_AMBER)) {
		data &= 0xCF;
		data = alarm_led_amb << 4;
		data &= 0xFC;
	} else if (sysfs_streq(buf, LED_HZ_GBNK)) {
		data &= 0xCF;
		data = alarm_led_grn << 4;
		data += led_blk_1hz;
	} else if (sysfs_streq(buf, LED_HZ_ABNK)) {
		data &= 0xCF;
		data = alarm_led_amb << 4;
		data += led_blk_1hz;
	} else if (sysfs_streq(buf, LED_QHZ_GBNK)) {
		data &= 0xCF;
		data = alarm_led_grn << 4;
		data += led_blk_4hz;
	} else if (sysfs_streq(buf, LED_QHZ_ABNK)) {
		data &= 0xCF;
		data = alarm_led_amb << 4;
		data += led_blk_4hz;
	} else if (sysfs_streq(buf, LED_HZ_GABNK)) {
		data &= 0xCF;
		data = alarm_led_on << 4;
		data += led_blk_1hz;
	} else if (sysfs_streq(buf, LED_QHZ_GABNK)) {
		data &= 0xCF;
		data = alarm_led_on << 4;
		data += led_blk_4hz;
	} else
		count = -EINVAL;

	outb(data, ALARM_LED_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);

	return count;
}

static ssize_t pwr_led_show(struct device *dev,
			    struct device_attribute *devattr, char *buf)
{
	unsigned char data = 0;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(PSU_LED_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);
	if ((data & 0x10) != 0)
		return sysfs_emit(buf, "%s\n", "auto");
	data = data & 0x3;
	return sysfs_emit(buf, "%s\n",
			  data == pwr_led_grn ? "green" :
			  data == pwr_led_amb ? "amber" :
						"off");
}

static ssize_t pwr_led_store(struct device *dev,
			     struct device_attribute *devattr, const char *buf,
			     size_t count)
{
	unsigned char led_status, data;

	if (sysfs_streq(buf, "off"))
		led_status = pwr_led_off;
	else if (sysfs_streq(buf, "green"))
		led_status = pwr_led_grn;
	else if (sysfs_streq(buf, "amber"))
		led_status = pwr_led_amb;
	else if (sysfs_streq(buf, "auto"))
		led_status = pwr_led_auto;
	else {
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

static ssize_t fan_led_show(struct device *dev,
			    struct device_attribute *devattr, char *buf)
{
	unsigned char data = 0;

	mutex_lock(&cpld_data->cpld_lock);
	data = inb(FAN_LED_ADDR);
	mutex_unlock(&cpld_data->cpld_lock);
	if ((data & 0x10) != 0)
		return sysfs_emit(buf, "%s\n", "auto");
	data = data & 0x3;

	return sysfs_emit(buf, "%s\n",
			  data == fan_led_grn ? "green" :
			  data == fan_led_amb ? "amber" :
						"off");
}

static ssize_t fan_led_store(struct device *dev,
			     struct device_attribute *devattr, const char *buf,
			     size_t count)
{
	unsigned char led_status, data;

	if (sysfs_streq(buf, "off"))
		led_status = fan_led_off;
	else if (sysfs_streq(buf, "green"))
		led_status = fan_led_grn;
	else if (sysfs_streq(buf, "amber"))
		led_status = fan_led_amb;
	else if (sysfs_streq(buf, "auto"))
		led_status = fan_led_auto;
	else {
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

static ssize_t bmc_present_l_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int len = 0;
	unsigned char value = 0;

	mutex_lock(&cpld_data->cpld_lock);
	value = inb(CARD_PRS_ADDR);
	len = sysfs_emit(buf, "%d\n", value & 1U);
	mutex_unlock(&cpld_data->cpld_lock);

	return len;
}

static DEVICE_ATTR_RW(getreg);
static DEVICE_ATTR_WO(setreg);
static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RW(scratch);
static DEVICE_ATTR_RO(bmc_present_l);
static DEVICE_ATTR_RW(sys_led);
static DEVICE_ATTR_RW(alarm_led);
static DEVICE_ATTR_RW(pwr_led);
static DEVICE_ATTR_RW(fan_led);

static struct attribute *cpld_b_attrs[] = {
	&dev_attr_scratch.attr,
	&dev_attr_getreg.attr,
	&dev_attr_version.attr,
	&dev_attr_setreg.attr,
	&dev_attr_bmc_present_l.attr,
	&dev_attr_sys_led.attr,
	&dev_attr_alarm_led.attr,
	&dev_attr_pwr_led.attr,
	&dev_attr_fan_led.attr,
	NULL,
};

static struct attribute_group cpld_b_attrs_grp = {
	.attrs = cpld_b_attrs,
};

static struct resource cpld_b_resources[] = {
	{
		.start = 0xA00,
		.end = 0xAAA,
		.flags = IORESOURCE_IO,
	},
};

static void cpld_b_dev_release(struct device *dev)
{
}

static struct platform_device cpld_b_dev = {
	.name = DRIVER_NAME,
	.id	= PLATFORM_DEVID_NONE,
	.num_resources = ARRAY_SIZE(cpld_b_resources),
	.resource = cpld_b_resources,
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
	.probe	= cpld_b_drv_probe,
	.remove = __exit_p(cpld_b_drv_remove),
	.driver = {
		.name	= DRIVER_NAME,
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
