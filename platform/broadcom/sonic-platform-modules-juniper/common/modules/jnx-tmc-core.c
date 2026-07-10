/*
 * Juniper Networks TMC-FPGA MFD Core driver for qfx platform
 *
 * Copyright (c) 2020, Juniper Networks
 * Author: Ashish Bhensdadia <bashish@juniper.net>
 *
 * This driver implement the resource publish for below devices
 * - I2C
 * - GPIO
 * - RE FPGA
 * - PSU
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <linux/device.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/irqdomain.h>
#include <linux/mfd/core.h>
#include "jnx-tmc.h"

#define TMC_DO_SCRATCH_TEST	1

/*
 * TMC FPGA Device IDs
 */
#define PCI_VENDOR_ID_JUNIPER	      0x1304

#define PCI_DEVICE_ID_JNX_TMC_CHD     0x007B
#define PCI_DEVICE_ID_JNX_TMC_PFE     0x007C

/*
 * TMC resources
 */
static struct resource tmc_resource_i2c[] = {
	/* I2C AUTOMATION Block */
	{
		.name  = "i2c-tmc",
		.start = TMC_I2C_AUTOMATION_I2C_CONTROL_START,
		.end   = TMC_I2C_AUTOMATION_I2C_CONTROL_END,
		.flags = IORESOURCE_MEM,
	},

	/* I2C DPMEM */
	{
		.name  = "i2c-tmc-mem",
		.start = TMC_I2C_DPMEM_ENTRY_START,
		.end   = TMC_I2C_DPMEM_ENTRY_END,
		.flags = IORESOURCE_MEM,
	},
};

#define TMC_RES_I2C_NR	ARRAY_SIZE(tmc_resource_i2c)

/*
 * LED resources
 */
static struct resource tmc_resource_leds[] = {
	{
		.name  = "leds-tmc",
		.start = TMC_LED_CONTROL_START,
		.end   = TMC_LED_CONTROL_END,
		.flags = IORESOURCE_MEM,
	},
};

#define TMC_RES_LEDS_NR	ARRAY_SIZE(tmc_resource_leds)

/*
 * TMC RE-FPGA devices
 */
static struct resource tmc_resource_refpga[] = {
	{
		.name  = "refpga-tmc",
		.start = TMC_REFPGA_ACCESS_START,
		.end   = TMC_REFPGA_ACCESS_END,
		.flags = IORESOURCE_MEM,
	},
};

#define TMC_RES_REFPGA_NR  ARRAY_SIZE(tmc_resource_refpga)

static struct resource tmc_resource_gpioslave0[] = {
	/* SLAVE0 Block */
	{
		.name  = "gpioslave-tmc",
		.start = TMC_GPIO_SLAVE0_START,
		.end   = TMC_GPIO_SLAVE0_END,
		.flags = IORESOURCE_MEM,
	}
};

#define TMC_RES_GPIOSLAVE0_NR	ARRAY_SIZE(tmc_resource_gpioslave0)

static struct resource tmc_resource_gpioslave1[] = {
	/* SLAVE1 Block */
	{
		.name  = "gpioslave-tmc",
		.start = TMC_GPIO_SLAVE1_START,
		.end   = TMC_GPIO_SLAVE1_END,
		.flags = IORESOURCE_MEM,
	}
};

#define TMC_RES_GPIOSLAVE1_NR	ARRAY_SIZE(tmc_resource_gpioslave1)

static struct resource tmc_resource_psu[] = {
	/* PSU Block */
	{
		.name  = "psu-tmc",
		.start = TMC_PSU_START,
		.end   = TMC_PSU_END,
		.flags = IORESOURCE_MEM,
	}
};

#define TMC_RES_PSU_NR	ARRAY_SIZE(tmc_resource_psu)

/*
 * CHASSISD TMC MFD devices
 */
static struct mfd_cell chassisd_tmc_mfd_devs[] = {
	{
		.name = "i2c-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_i2c),
		.resources = &tmc_resource_i2c[0],
		.of_compatible = "jnx,i2c-tmc",
		.id = 0,
	},
	{
		.name = "leds-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_leds),
		.resources = &tmc_resource_leds[0],
		.of_compatible = "jnx,leds-tmc",
		.id = 0,
	},
	{
		.name = "refpga-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_refpga),
		.resources = &tmc_resource_refpga[0],
		.of_compatible = "jnx,refpga-tmc",
		.id = 0,
	},
	{
		.name = "psu-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_psu),
		.resources = &tmc_resource_psu[0],
		.of_compatible = "jnx,psu-tmc",
		.id = 0,
	},
};

/*
 * PFE TMC MFD devices
 */
static struct mfd_cell pfe_tmc_mfd_devs[] = {
	{
		.name = "i2c-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_i2c),
		.resources = &tmc_resource_i2c[0],
		.of_compatible = "jnx,i2c-tmc",
		.id = 1,
	},
	{
		.name = "gpioslave-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_gpioslave0),
		.resources = &tmc_resource_gpioslave0[0],
		.of_compatible = "jnx,gpioslave-tmc",
		.id = 0,
	},
	{
		.name = "gpioslave-tmc",
		.num_resources = ARRAY_SIZE(tmc_resource_gpioslave1),
		.resources = &tmc_resource_gpioslave1[0],
		.of_compatible = "jnx,gpioslave-tmc",
		.id = 1,
	},
};


struct tmc_fpga_data {
	void __iomem *membase;
	struct pci_dev *pdev;

	u32 major;   /* Device id & Major version*/
	u32 minor;	/* Minor version */

	u32 optic_cpld_major; /* optic cpld major version */
	u32 optic_cpld_minor; /* optic cpld minor version */
	u32 optic_cpld_devid; /* optic cpld device id */
};

/* sysfs entries */
static ssize_t major_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct tmc_fpga_data *tmc = dev_get_drvdata(dev);

	return sprintf(buf, "0x%02X_%06X\n",
		       (tmc->major >> 24) & 0xff,
		       tmc->major & 0xffffff);
}

static ssize_t minor_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct tmc_fpga_data *tmc = dev_get_drvdata(dev);

	return sprintf(buf, "%02X\n", (tmc->minor) & 0xff);
}

static ssize_t optic_cpld_major_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tmc_fpga_data *tmc = dev_get_drvdata(dev);

	return sprintf(buf, "%01X\n", tmc->optic_cpld_major & 0xf);
}

static ssize_t optic_cpld_devid_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tmc_fpga_data *tmc = dev_get_drvdata(dev);

	return sprintf(buf, "%01X\n",
		       (tmc->optic_cpld_major >> 4) & 0xf);
}

static ssize_t optic_cpld_minor_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tmc_fpga_data *tmc = dev_get_drvdata(dev);

	return sprintf(buf, "%02X\n", tmc->optic_cpld_minor & 0xff);
}

static ssize_t set_sys_shutdown(struct device *dev,
				struct device_attribute *devattr,
				const char *buf,
				size_t len)
{

	struct tmc_fpga_data *tmc = dev_get_drvdata(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	if (val != 1)
		return -EINVAL;

	/* Unlock the shutdown register */
	iowrite32(0x12345678, tmc->membase + TMC_SYS_SHUTDOWN_LOCK);
	iowrite32(0x1, tmc->membase + TMC_SYS_SHUTDOWN);

	return len;
}


static DEVICE_ATTR(major, S_IRUGO, major_show, NULL);
static DEVICE_ATTR(minor, S_IRUGO, minor_show, NULL);
static DEVICE_ATTR(optic_cpld_major, S_IRUGO, optic_cpld_major_show, NULL);
static DEVICE_ATTR(optic_cpld_devid, S_IRUGO, optic_cpld_devid_show, NULL);
static DEVICE_ATTR(optic_cpld_minor, S_IRUGO, optic_cpld_minor_show, NULL);
static DEVICE_ATTR(shutdown, S_IWUSR, NULL, set_sys_shutdown);

static struct attribute *tmc_attrs[] = {
	&dev_attr_major.attr,
	&dev_attr_minor.attr,
	&dev_attr_optic_cpld_major.attr,
	&dev_attr_optic_cpld_devid.attr,
	&dev_attr_optic_cpld_minor.attr,
	&dev_attr_shutdown.attr,
	NULL,
};

static struct attribute_group tmc_attr_group = {
	.attrs  = tmc_attrs,
};

#if defined TMC_DO_SCRATCH_TEST
/* Do a quick scratch access test */
static int tmc_do_test_scratch(struct tmc_fpga_data *tmc)
{
	struct pci_dev *pdev = tmc->pdev;
	struct device *dev = &pdev->dev;
	int offset = TMC_SCRATCH;
	u32 acc, val = 0xdeadbeaf;

	/*
	 * Check rw register access -> use the scratch reg.
	 */
	iowrite32(val, tmc->membase + offset);
	acc = ioread32(tmc->membase + offset);
	if (acc != val) {
		dev_err(dev, "Tmc scratch(0x%x) failed: %08x.%08x!\n",
			offset, val, acc);
		return -EIO;
	}

	for (val = 0; val < 0xf0000000; val += 0x01010101) {
		iowrite32(val, tmc->membase + offset);
		acc = ioread32(tmc->membase + offset);
		if (acc != val) {
			dev_err(dev, "Tmc scratch(0x%x) failed: %08x.%08x!\n",
				offset, val, acc);
			return -EIO;
		}
	}

	/*
	 * Write a sig before leaving..
	 */
	val = 0xcafebabe;
	iowrite32(val, tmc->membase + offset);
	dev_dbg(dev, "Tmc scratch result: 0x%08x\n",
		 ioread32(tmc->membase + offset));

	return 0;
}
#endif /* TMC_DO_SCRATCH_TEST */

static int tmc_fpga_probe(struct pci_dev *pdev,
			       const struct pci_device_id *id)
{
	int err;
	struct tmc_fpga_data *tmc;
	struct device *dev = &pdev->dev;

	dev_dbg(dev, "Tmc FPGA Probe called\n");

	tmc = devm_kzalloc(dev, sizeof(*tmc), GFP_KERNEL);
	if (!tmc)
		return -ENOMEM;

	err = pcim_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "Failed to enable device %d\n", err);
		return err;
	}

	err = pcim_iomap_regions(pdev, 1 << 0, "tmc-core");
	if (err) {
		dev_err(&pdev->dev, "Failed to iomap regions %d\n", err);
		goto err_disable;
	}

	tmc->membase = pcim_iomap_table(pdev)[0];
	if (IS_ERR(tmc->membase)) {
		dev_err(dev, "pci_ioremap_bar() failed\n");
		err = -ENOMEM;
		goto err_release;
	}

	tmc->pdev = pdev;
	pci_set_drvdata(pdev, tmc);

	/* All Tmc uses MSI interrupts - enable bus mastering */
	pci_set_master(pdev);

#if defined TMC_DO_SCRATCH_TEST
	/* Check IO before proceeding */
	dev_dbg(dev, "Tmc FPGA starting scratch test\n");
	err = tmc_do_test_scratch(tmc);
	if (err)
		goto err_unmap;

	dev_dbg(dev, "Tmc FPGA scratch test passed !!!\n");
#endif /* TMC_DO_SCRATCH_TEST */

	switch (id->device) {
	case PCI_DEVICE_ID_JNX_TMC_CHD:
		err = mfd_add_devices(dev, pdev->bus->number,
				&chassisd_tmc_mfd_devs[0],
				ARRAY_SIZE(chassisd_tmc_mfd_devs),
				&pdev->resource[0],
				0, NULL /* tmc->irq_domain */);
		break;
	case PCI_DEVICE_ID_JNX_TMC_PFE:
		err = mfd_add_devices(dev, pdev->bus->number,
					&pfe_tmc_mfd_devs[0],
					ARRAY_SIZE(pfe_tmc_mfd_devs),
					&pdev->resource[0],
					0, NULL /* tmc->irq_domain */);
		break;
	default:
		dev_err(&pdev->dev, "Invalid PCI Device ID id:%d\n",
				id->device);
		goto err_unmap;
	}

	if (err < 0) {
		dev_err(&pdev->dev, "Failed to add mfd devices %d\n", err);
		goto err_unmap;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &tmc_attr_group);
	if (err) {
		sysfs_remove_group(&pdev->dev.kobj, &tmc_attr_group);
		dev_err(&pdev->dev, "Failed to create attr group\n");
		goto err_remove_mfd;
	}

	tmc->major = ioread32(tmc->membase + TMC_REVISION);
	tmc->minor = ioread32(tmc->membase + TMC_MINOR);

	tmc->optic_cpld_major = ioread32(tmc->membase + TMC_OPTIC_CPLD_MAJOR);
	tmc->optic_cpld_minor = ioread32(tmc->membase + TMC_OPTIC_CPLD_MINOR);

	dev_info(dev, "Tmc FPGA Revision: 0x%02X_%06X, Minor: %02X\n",
		 (tmc->major >> 24) & 0xff,
		 tmc->major & 0xffffff,
		 (tmc->minor) & 0xff);
	dev_info(dev, "Tmc FPGA optic cpld Major: 0x%01X, Minor: 0x%02X "
		 "Devid: 0x%01X\n", (tmc->optic_cpld_major) & 0xf,
		 (tmc->optic_cpld_minor) & 0xff,
		 (tmc->optic_cpld_major >> 4) & 0xf);
	dev_info(dev, "Tmc FPGA mem:0x%lx\n",
		 (unsigned long)tmc->membase);

	return 0;

err_remove_mfd:
	mfd_remove_devices(dev);
err_unmap:
	pci_iounmap(pdev, tmc->membase);
err_release:
	pci_release_regions(pdev);
err_disable:
	pci_disable_device(pdev);

	return err;
}

static void tmc_fpga_remove(struct pci_dev *pdev)
{
	struct tmc_fpga_data *tmc = dev_get_drvdata(&pdev->dev);

	sysfs_remove_group(&pdev->dev.kobj, &tmc_attr_group);
	mfd_remove_devices(&pdev->dev);
}

static const struct pci_device_id tmc_fpga_id_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_JUNIPER, PCI_DEVICE_ID_JNX_TMC_CHD) },
	{ PCI_DEVICE(PCI_VENDOR_ID_JUNIPER, PCI_DEVICE_ID_JNX_TMC_PFE) },
	{ }
};
MODULE_DEVICE_TABLE(pci, tmc_fpga_id_tbl);

static struct pci_driver tmc_fpga_driver = {
	.name		= "tmc-core",
	.id_table	= tmc_fpga_id_tbl,
	.probe		= tmc_fpga_probe,
	.remove		= tmc_fpga_remove,
};

module_pci_driver(tmc_fpga_driver);

MODULE_DESCRIPTION("Juniper Networks TMC FPGA MFD core driver");
MODULE_AUTHOR("Ashish Bhensdadia <bashish@juniper.net>");
MODULE_LICENSE("GPL");
