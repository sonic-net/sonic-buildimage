/*
    ciena_raw_chardev.c - Driver for raw access to a regmap client

    Copyright (C) 2022 Ciena Corporation

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.
*/

#include <linux/device.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kobject.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include "ciena_raw_chardev.h"

#define CIENA_MAX_CHARDEV_ENV 128

struct ciena_raw_chardev_priv {
	struct bin_attribute  raw_attr;
	struct regmap        *io_regmap;
	unsigned              io_offset;
	const char           *chardev_env;
};

/* ------------------------------------------------------------------------- */
static int ciena_raw_chardev_do_edge(struct ciena_raw_chardev_priv *priv,
				     struct device                 *dev,
				     char                          *buf,
				     loff_t                         off,
				     size_t                         count,
				     bool                           do_write)
{
	unsigned stride = regmap_get_reg_stride(priv->io_regmap);
	unsigned skip   = off % stride;
	unsigned nbytes = stride - skip;
	unsigned all_1s = ~0U;
	unsigned mask   = 0;
	unsigned val    = 0;
	int      rc;

	if (count < nbytes) nbytes = count;

	if (do_write) {
		memcpy(((void *) &val) + skip, buf, nbytes);
		memcpy(((void *) &mask) + skip, &all_1s, nbytes);
		rc = regmap_write_bits(priv->io_regmap, off - skip,
				       mask, val);
	}
	else {
		rc = regmap_read(priv->io_regmap, off - skip, &val);
		memcpy(buf, ((void *) &val) + skip, nbytes);
	}

	if (rc) {
		dev_dbg(dev, "%s %s failed off=%lld count=%zu, (%d)\n",
			__func__, do_write ? "wr" : "rd", off, count, rc);
		return rc;
	}

	return nbytes;
}

/* ------------------------------------------------------------------------- */
static ssize_t ciena_raw_chardev_xfer(struct file*          filp,
				      struct kobject*       kobj,
				      struct bin_attribute* bin_attr,
				      char*                 buf,
				      loff_t                off,
				      size_t                count,
				      bool                  do_write)
{
	struct ciena_raw_chardev_priv *priv;
	struct device                 *dev;
	unsigned                       stride;
	unsigned                       val;
	size_t                         remain;
	int                            rc = 0;

	dev    = container_of(kobj, struct device, kobj);
	priv   = dev_get_drvdata(dev);
	stride = regmap_get_reg_stride(priv->io_regmap);

	off += priv->io_offset;

	if (off % stride) {
		rc = ciena_raw_chardev_do_edge(priv, dev, buf, off,
					       count, do_write);
		if (0 > rc) return rc;
	}

	off    += rc;
	buf    += rc;
	remain  = count - rc;

	while (stride <= remain) {
		if (do_write) {
			memcpy(&val, buf, stride);
			rc = regmap_write(priv->io_regmap, off, val);
		}
		else {
			rc = regmap_read(priv->io_regmap, off, &val);
			memcpy(buf, &val, stride);
		}

		if (rc) {
			dev_dbg(dev, "%s failed off=%lld count=%zu, (%d)\n",
				do_write ? "wr" : "rd", off, remain, rc);
			return rc;
		}

		off    += stride;
		buf    += stride;
		remain -= stride;
	}

	if (remain) {
		rc = ciena_raw_chardev_do_edge(priv, dev, buf, off,
					       remain, do_write);
		if (0 > rc) return rc;
	}

	return count;
}

/* ------------------------------------------------------------------------- */
static ssize_t ciena_raw_chardev_read(struct file*          filp,
				      struct kobject*       kobj,
				      struct bin_attribute* bin_attr,
				      char*                 buf,
				      loff_t                off,
				      size_t                count)
{
	return ciena_raw_chardev_xfer(filp, kobj, bin_attr,
				      buf, off, count, false);
}

/* ------------------------------------------------------------------------- */
static ssize_t ciena_raw_chardev_write(struct file*          filp,
				       struct kobject*       kobj,
				       struct bin_attribute* bin_attr,
				       char*                 buf,
				       loff_t                off,
				       size_t                count)
{
	return ciena_raw_chardev_xfer(filp, kobj, bin_attr,
				      buf, off, count, true);
}

/* ------------------------------------------------------------------------- */
static int ciena_raw_chardev_probe(struct platform_device *pdev)
{
	struct ciena_raw_chardev_priv  *priv;
	struct resource                *regs;
	struct device                  *dev    = &pdev->dev;
	struct ciena_raw_chardev_pdata *pdata  = dev->platform_data;
        char                            envs[CIENA_MAX_CHARDEV_ENV];
        char                           *envp[] = { envs, NULL };
	int                             rc;

	if (!pdata) {
		dev_err(dev, "missing platform data\n");
		return -EINVAL;
	}

	if (pdata->chardev_env) {
		if (sizeof(envs) <= snprintf(envs, sizeof(envs), "%s",
					     pdata->chardev_env)) {
			dev_err(dev, "%s too long\n", pdata->chardev_env);
			return -EINVAL;
		}
	}

	regs = platform_get_resource(pdev, IORESOURCE_REG, 0);
	if (!regs) {
		dev_err(dev, "missing register resource\n");
		return -ENXIO;
	}

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "no memory for private data\n");
		return -ENOMEM;
	}

	sysfs_bin_attr_init(&priv->raw_attr);

	priv->raw_attr.attr.name = "raw_chardev";
	priv->raw_attr.attr.mode = S_IRUGO | S_IWUSR;
	priv->raw_attr.size      = resource_size(regs);
	priv->raw_attr.read      = ciena_raw_chardev_read;
	priv->raw_attr.write     = ciena_raw_chardev_write;
	priv->io_regmap          = pdata->chardev_regmap;
	priv->io_offset          = regs->start;
	priv->chardev_env        = pdata->chardev_env;

	rc = device_create_bin_file(dev, &priv->raw_attr);
	if (rc) {
		dev_err(dev, "cannot create bin file (%d)\n", rc);
		return rc;
	}

	dev_set_drvdata(dev, priv);

	/* nudge a uevent from the device, because adding files under
	 * the platform device will not */
	if (priv->chardev_env) kobject_uevent_env(&dev->kobj, KOBJ_ADD, envp);
	else kobject_uevent(&dev->kobj, KOBJ_ADD);

	return 0;
}

/* ------------------------------------------------------------------------- */
static void ciena_raw_chardev_remove(struct platform_device *pdev)
{
	struct device                  *dev  = &pdev->dev;
	struct ciena_raw_chardev_priv  *priv = dev_get_drvdata(dev);
        char                            envs[CIENA_MAX_CHARDEV_ENV];
        char                           *envp[] = { envs, NULL };

	if (priv->chardev_env) {
		snprintf(envs, sizeof(envs), "%s", priv->chardev_env);
		kobject_uevent_env(&dev->kobj, KOBJ_REMOVE, envp);
	}
	else kobject_uevent(&dev->kobj, KOBJ_REMOVE);

	device_remove_bin_file(dev, &priv->raw_attr);
}

/* ------------------------------------------------------------------------- */
static const struct platform_device_id ciena_raw_chardev_platform_ids[] = {
	{ .name = CIENA_RAW_CHARDEV_NAME },
	{ },
};
MODULE_DEVICE_TABLE(platform, ciena_raw_chardev_platform_ids);

static struct platform_driver ciena_raw_chardev_driver = {
	.probe    = ciena_raw_chardev_probe,
	.remove   = ciena_raw_chardev_remove,
	.id_table = ciena_raw_chardev_platform_ids,
	.driver   = {
		.name  = CIENA_RAW_CHARDEV_NAME,
	},
};

module_platform_driver(ciena_raw_chardev_driver);

MODULE_AUTHOR("Marc St-Amand <mstamand@ciena.com>");
MODULE_DESCRIPTION("raw character driver for regmap areas");
MODULE_LICENSE("GPL");
