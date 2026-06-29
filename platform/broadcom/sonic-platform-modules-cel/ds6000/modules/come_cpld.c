/*
 * The CPLD driver for the COMe of DS6000.
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

#define DRIVER_NAME "come_cpld"

/* CPLD register address for read and write */
#define VERSION_ADDR 0xAE0
#define SCRATCH_ADDR 0xAE1
#define WDT_TIME_DELAY_ADDR 0xAE2
#define INT_SRC_MASK_ADDR 0xAE3
#define INT_CLEAR_ADDR 0xAE4
#define INT_SRC_STATUS_ADDR 0xAE5
#define EMMC_RESET_ADDR 0xAE6
#define EEPROM_WP_ADDR 0xAE7


#define CPLD_REGISTER_SIZE 0x8


struct cpld_data {
	struct mutex cpld_lock;
	uint16_t read_addr;
};

struct cpld_data *priv;

static ssize_t scratch_show(struct device *dev,
			    struct device_attribute *devattr, char *buf)
{
	unsigned char data = 0;

	mutex_lock(&priv->cpld_lock);
	data = inb(SCRATCH_ADDR);
	mutex_unlock(&priv->cpld_lock);

	return sysfs_emit(buf, "0x%2.2x\n", data);
}

static ssize_t scratch_store(struct device *dev,
			     struct device_attribute *devattr, const char *buf,
			     size_t count)
{
	int ret = 0;
	unsigned long data;

	mutex_lock(&priv->cpld_lock);
	ret = kstrtoul(buf, 0, &data);
	if (ret != 0) {
		mutex_unlock(&priv->cpld_lock);
		return ret;
	}
	outb(data, SCRATCH_ADDR);
	mutex_unlock(&priv->cpld_lock);

	return count;
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int len = 0;
	unsigned char value = 0;

	mutex_lock(&priv->cpld_lock);
	value = inb(VERSION_ADDR);
	len = sysfs_emit(buf, "%d.%d\n", value >> 4, value & 0x0F);
	mutex_unlock(&priv->cpld_lock);

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
	priv->read_addr = addr;

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

	mutex_lock(&priv->cpld_lock);
	len = sysfs_emit(buf, "0x%2.2x\n", inb(priv->read_addr));
	mutex_unlock(&priv->cpld_lock);

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

	mutex_lock(&priv->cpld_lock);
	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&priv->cpld_lock);
		kfree(clone);
		return -EINVAL;
	}
	ret = kstrtoul(tok, 0, &addr);
	if (ret != 0) {
		mutex_unlock(&priv->cpld_lock);
		kfree(clone);
		return ret;
	}

	tok = strsep((char **)&pclone, " ");
	if (tok == NULL) {
		mutex_unlock(&priv->cpld_lock);
		kfree(clone);
		return -EINVAL;
	}
	ret = kstrtoul(tok, 0, &value);
	if (ret != 0) {
		mutex_unlock(&priv->cpld_lock);
		kfree(clone);
		return ret;
	}

	outb(value, addr);
	mutex_unlock(&priv->cpld_lock);
	kfree(clone);

	return count;
}

static DEVICE_ATTR_RW(getreg);
static DEVICE_ATTR_WO(setreg);
static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RW(scratch);

static struct attribute *cpld_attrs[] = {
	&dev_attr_scratch.attr,
	&dev_attr_getreg.attr,
	&dev_attr_version.attr,
	&dev_attr_setreg.attr,
	NULL,
};

static struct attribute_group cpld_attrs_grp = {
	.attrs = cpld_attrs,
};

static struct resource cpld_resource[] = {
	{
		.start = 0xAE0,
		.end = 0xAE7,
		.flags = IORESOURCE_IO,
	},
};

static void cpld_dev_release(struct device *dev)
{
}

static struct platform_device cpld_dev = {
	.name = DRIVER_NAME,
	.id	= PLATFORM_DEVID_NONE,
	.num_resources = ARRAY_SIZE(cpld_resource),
	.resource = cpld_resource,
	.dev = {
		.release = cpld_dev_release,
	}
};

static int cpld_drv_probe(struct platform_device *pdev)
{
	struct resource *res;
	int err = 0;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct cpld_data),
				 GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mutex_init(&priv->cpld_lock);

	priv->read_addr = VERSION_ADDR;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (unlikely(!res)) {
		dev_err(&pdev->dev, "Specified Resource Not Available...\n");
		return -ENODEV;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &cpld_attrs_grp);
	if (err) {
		dev_err(&pdev->dev, "Cannot create sysfs for COMe CPLD\n");
		return err;
	}
	return 0;
}

static void cpld_drv_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &cpld_attrs_grp);

	return;
}

static struct platform_driver cpld_drv = {
	.probe	= cpld_drv_probe,
	.remove = __exit_p(cpld_drv_remove),
	.driver = {
		.name	= DRIVER_NAME,
	},
};

int cpld_init(void)
{
	// Register platform device and platform driver
	platform_device_register(&cpld_dev);
	platform_driver_register(&cpld_drv);

	return 0;
}

void cpld_exit(void)
{
	// Unregister platform device and platform driver
	platform_driver_unregister(&cpld_drv);
	platform_device_unregister(&cpld_dev);
}

module_init(cpld_init);
module_exit(cpld_exit);

MODULE_AUTHOR("Celestica Inc.");
MODULE_DESCRIPTION("LPC CPLD COMe driver");
MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL");
