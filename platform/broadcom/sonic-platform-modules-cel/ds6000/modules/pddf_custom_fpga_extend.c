/*
 * pddf_custom_fpga_extend.c
 *
 * This module provides sysfs interfaces (getreg, setreg, version, scratch)
 * for the DS6000 platform's FPGA, using the ptr_fpgapci_read/write accessors.
 *
 * Copyright (C) 2026 Celestica Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>

/* Function pointers for FPGA access, provided by another module */
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);

#define DRV_NAME "pddf_custom_fpga_extend"

#define FPGA_VERSION_REG 0x0000
#define FPGA_SCRATCH_REG 0x0004
#define FPGA_TH_MIN_TEMP_REG 0x0030
#define FPGA_TH_MAX_TEMP_REG 0x0034

struct fpga_extend_priv {
	struct mutex lock;
	uint32_t read_addr; /* For getreg */
};

/**
 * getreg_show - Spefical function to show the register value.
 * The register address is specified by getreg_store function.
 *
 * Return: size of data, or negative error code.
 */
static ssize_t getreg_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	uint32_t data;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	mutex_lock(&priv->lock);
	data = ptr_fpgapci_read(priv->read_addr);
	mutex_unlock(&priv->lock);

	return sysfs_emit(buf, "0x%08x\n", data);
}

/**
 * getreg_store - Spefical function to set the register address to read.
 * Return: size of data received, or negative error code.
 */
static ssize_t getreg_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	unsigned long addr;
	int ret;

	ret = kstrtoul(buf, 16, &addr);
	if (ret)
		return ret;

	/* The address offset must be 32-bit aligned for fpga register */
	if (addr & 0x3) {
		dev_err(dev, "Error: address 0x%lx is not 32-bit aligned\n",
			addr);
		return -EINVAL;
	}

	mutex_lock(&priv->lock);
	priv->read_addr = addr;
	mutex_unlock(&priv->lock);

	return count;
}

/*
 * sysfs 'scratch' show/store functions
 */
static ssize_t scratch_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	uint32_t data;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	data = ptr_fpgapci_read(FPGA_SCRATCH_REG);

	return sysfs_emit(buf, "0x%08x\n", data);
}

static ssize_t scratch_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	unsigned long data;
	int ret;

	if (!ptr_fpgapci_write) {
		dev_err(dev, "ptr_fpgapci_write is not available\n");
		return -ENODEV;
	}

	ret = kstrtoul(buf, 16, &data);
	if (ret)
		return ret;

	mutex_lock(&priv->lock);
	ptr_fpgapci_write(FPGA_SCRATCH_REG, data);
	mutex_unlock(&priv->lock);

	return count;
}

/**
 * setreg_store - Special function to set the register value.
 *
 * @dev: kerenl device
 * @devattr: kernel device attribute
 * @buf: The buffer contains address and value.
 *       The format is "<address> <value>"
 * @count: size of buf
 *
 * Return: size of data, or negative error code.
 */
static ssize_t setreg_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	unsigned long addr, value;
	char *clone, *pclone, *tok;
	int ret = 0;

	if (!ptr_fpgapci_write) {
		dev_err(dev, "ptr_fpgapci_write is not available\n");
		return -ENODEV;
	}

	clone = kstrndup(buf, count, GFP_KERNEL);
	if (!clone)
		return -ENOMEM;
	pclone = clone;

	tok = strsep(&pclone, " ");
	if (!tok) {
		ret = -EINVAL;
		goto out;
	}
	ret = kstrtoul(tok, 16, &addr);
	if (ret)
		goto out;

	tok = strsep(&pclone, " ");
	if (!tok) {
		ret = -EINVAL;
		goto out;
	}
	ret = kstrtoul(tok, 16, &value);
	if (ret)
		goto out;

	mutex_lock(&priv->lock);
	ptr_fpgapci_write(addr, value);
	mutex_unlock(&priv->lock);

out:
	kfree(clone);
	return ret ? ret : count;
}

/*
 * sysfs 'version' show function
 */
static ssize_t version_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	uint32_t data;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	data = ptr_fpgapci_read(FPGA_VERSION_REG);

	return sysfs_emit(buf, "0x%08x\n", data);
}

/**
 * th6_reg_to_temp - Convert the raw register value to temperature in milli-Celsius
 * @reg_val: The raw register value, in 16-bit format.
 *
 * Return: The temperature in milli-Celsius, or negative error code.
 */
static int32_t th6_reg_to_temp(uint32_t reg_val)
{
	int64_t temp_mC;

	if (reg_val == 0 || reg_val == 0xFFFFFFFF)
		return -ERANGE;

	/* Calculate temperature in milli-Celsius */
	temp_mC = (int16_t)(reg_val & GENMASK(15, 0));
	temp_mC *= 1000;

	return temp_mC;
}

static ssize_t TH6_max_temp_raw_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	uint32_t reg;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	mutex_lock(&priv->lock);
	reg = ptr_fpgapci_read(FPGA_TH_MAX_TEMP_REG);
	mutex_unlock(&priv->lock);

	return sysfs_emit(buf, "0x%x\n", reg);
}

static ssize_t TH6_max_temp_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	uint32_t reg, temp_milli_c;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	mutex_lock(&priv->lock);
	reg = ptr_fpgapci_read(FPGA_TH_MAX_TEMP_REG);
	mutex_unlock(&priv->lock);

	temp_milli_c = th6_reg_to_temp(reg);
	return sysfs_emit(buf, "%d\n", temp_milli_c);
}

static ssize_t TH6_min_temp_raw_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	uint32_t reg;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	mutex_lock(&priv->lock);
	reg = ptr_fpgapci_read(FPGA_TH_MIN_TEMP_REG);
	mutex_unlock(&priv->lock);

	return sysfs_emit(buf, "0x%x\n", reg);
}

static ssize_t TH6_min_temp_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fpga_extend_priv *priv = dev_get_drvdata(dev);
	uint32_t reg, temp_milli_c;

	if (!ptr_fpgapci_read) {
		dev_err(dev, "ptr_fpgapci_read is not available\n");
		return -ENODEV;
	}

	mutex_lock(&priv->lock);
	reg = ptr_fpgapci_read(FPGA_TH_MIN_TEMP_REG);
	mutex_unlock(&priv->lock);

	temp_milli_c = th6_reg_to_temp(reg);
	return sysfs_emit(buf, "%d\n", temp_milli_c);
}

/* Attribute definitions */
static DEVICE_ATTR_RW(getreg);
static DEVICE_ATTR_RW(scratch);
static DEVICE_ATTR_WO(setreg);
static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RO(TH6_max_temp_raw);
static DEVICE_ATTR_RO(TH6_max_temp);
static DEVICE_ATTR_RO(TH6_min_temp_raw);
static DEVICE_ATTR_RO(TH6_min_temp);

static struct attribute *fpga_ext_attrs[] = {
	&dev_attr_getreg.attr,
	&dev_attr_scratch.attr,
	&dev_attr_setreg.attr,
	&dev_attr_version.attr,
	&dev_attr_TH6_max_temp_raw.attr,
	&dev_attr_TH6_max_temp.attr,
	&dev_attr_TH6_min_temp_raw.attr,
	&dev_attr_TH6_min_temp.attr,
	NULL,
};

static const struct attribute_group fpga_ext_attr_group = {
	.attrs = fpga_ext_attrs,
};

/*
 * Platform driver probe/remove functions
 */
static int fpga_extend_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fpga_extend_priv *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mutex_init(&priv->lock);
	dev_set_drvdata(dev, priv);

	ret = sysfs_create_group(&dev->kobj, &fpga_ext_attr_group);
	if (ret) {
		dev_err(dev, "failed to create sysfs group\n");
		return ret;
	}

	dev_info(dev, "driver loaded\n");
	return 0;
}

static void fpga_extend_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &fpga_ext_attr_group);
	dev_info(&pdev->dev, "driver unloaded\n");
}

static struct platform_driver pddf_custom_fpga_extend_driver = {
	.probe = fpga_extend_probe,
	.remove = fpga_extend_remove,
	.driver = {
		.name = DRV_NAME,
	},
};

static struct platform_device *pddf_fpga_extend_pdev;

/* Module initialization and exit functions */
static int __init fpga_extend_init(void)
{
	int ret;

	pddf_fpga_extend_pdev =
		platform_device_register_simple(DRV_NAME, -1, NULL, 0);
	if (IS_ERR(pddf_fpga_extend_pdev)) {
		pr_err("%s: failed to register platform device\n", DRV_NAME);
		return PTR_ERR(pddf_fpga_extend_pdev);
	}

	ret = platform_driver_register(&pddf_custom_fpga_extend_driver);
	if (ret) {
		pr_err("%s: failed to register platform driver\n", DRV_NAME);
		platform_device_unregister(pddf_fpga_extend_pdev);
		return ret;
	}

	return 0;
}

static void __exit fpga_extend_exit(void)
{
	platform_driver_unregister(&pddf_custom_fpga_extend_driver);
	platform_device_unregister(pddf_fpga_extend_pdev);
}

module_init(fpga_extend_init);
module_exit(fpga_extend_exit);

MODULE_DESCRIPTION("DS6000 Custom FPGA Extension Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
