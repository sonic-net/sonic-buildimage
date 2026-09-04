/*
 * Copyright 2016 Ciena Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include <linux/reset-controller.h>

#include "reset-sysfs.h"

/*
 * This is a simple driver that exposes the reset-controller driver to userspace
 * via sysfs. We bind to a device tree node that has 'resets' and 'reset-names'
 * property and let the reset-controller driver do all of the heavy lifting.
 *
 * The reset-controller driver will find the given name in the 'reset-names'
 * property and then trigger the equivalent reset from the 'resets' property.
 */

struct sysfs_reset_private {
	struct reset_control_lookup *lkup;
	unsigned lkup_count;
};

static ssize_t sysfs_reset_control(struct device *dev, const char *buf,
				   int (*f)(struct reset_control *))
{
	struct reset_control *rstc;
	char  name[64];
	ssize_t size;
	int rc;

	size = strchrnul(buf, '\n') - buf + 1;

	if (size > sizeof(name))
		return -EINVAL;

	strscpy(name, buf, size);

	dev_dbg(dev, "name:%s size:%zd\n", name, size);

	rstc = reset_control_get_exclusive(dev, name);
	if (IS_ERR(rstc)) {
		dev_dbg(dev, "%s: failed to get %s (%ld)\n", __func__,
			name, PTR_ERR(rstc));
		return PTR_ERR(rstc);
	}

	rc = f(rstc);

	reset_control_put(rstc);

	return rc ? rc : size;
}


static ssize_t assert_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	return sysfs_reset_control(dev, buf, reset_control_assert);
}
static DEVICE_ATTR_WO(assert);


static ssize_t deassert_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return sysfs_reset_control(dev, buf, reset_control_deassert);
}
static DEVICE_ATTR_WO(deassert);


static ssize_t reset_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	return sysfs_reset_control(dev, buf, reset_control_reset);
}
static DEVICE_ATTR_WO(reset);

static int reset_status_show(struct device *dev, const char *name,
			     char *buf, int size)
{
	struct reset_control *rstc;
	int status;
	int count;

	rstc = reset_control_get_shared(dev, name);
	if (IS_ERR(rstc)) {
		dev_dbg(dev, "%s: failed to get %s (%ld)\n",
			__func__, name, PTR_ERR(rstc));
		return 0;
	}

	status = reset_control_status(rstc);

	count = scnprintf(buf, size, "%s\t%s%d\n", name,
			  (8 > strlen(name)) ? "\t" : "", status);

	reset_control_put(rstc);

	return count;
}

static ssize_t status_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct ciena_sysfs_reset_pdata *pdata = dev->platform_data;
	const struct ciena_sysfs_reset *reset;
	struct property *prop;
	const char *name;
	int count = 0;

	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "# 0 - deasserted, 1 - asserted\n");

	if (pdata) {
		reset = pdata->resets;
		while (reset && reset->reset_name) {
			count += reset_status_show(dev, reset->reset_name,
						   buf + count,
						   PAGE_SIZE - count);
			reset++;
		}
	}
	else {
		of_property_for_each_string(dev->of_node, "reset-names",
					    prop, name)
			count += reset_status_show(dev, name, buf + count,
						   PAGE_SIZE - count);
	}

	return count;
}
static DEVICE_ATTR_RO(status);

static int ciena_sysfs_reset_lkup(struct platform_device *pdev,
				  struct sysfs_reset_private *priv)
{
	struct ciena_sysfs_reset_pdata *pdata = pdev->dev.platform_data;
	const struct ciena_sysfs_reset *reset = pdata->resets;
	struct reset_control_lookup *lkup;
	struct reset_control_lookup *pos;
	unsigned count = 0;

	while (reset && reset->reset_name) {
		count++;
		reset++;
	}

	if (!count) {
		dev_err(&pdev->dev, "must have at least one reset\n");
		return -EINVAL;
	}

	lkup = devm_kzalloc(&pdev->dev, count * sizeof(*lkup), GFP_KERNEL);
	if (!lkup) {
		dev_err(&pdev->dev, "cannot allocate %u * %zu bytes\n",
			count, sizeof(*lkup));
		return -EINVAL;
	}

	pos = &lkup[count - 1];
	do {
		reset--;
		pos->provider = pdata->controller_name;
		if (reset->value) pos->index = reset->value;
		else pos->index = reset->bit_offset | reset->bit_flags;
		pos->dev_id = dev_name(&pdev->dev);
		pos->con_id = reset->reset_name;
		dev_dbg(&pdev->dev, "%s: provider=%s index=%u dev_id=%s\n",
			__func__, pos->provider, pos->index, pos->dev_id);
		pos--;
	} while(reset != pdata->resets);

	reset_controller_add_lookup(lkup, count);

	priv->lkup = lkup;
	priv->lkup_count = count;

	return 0;
}

static int ciena_sysfs_reset_probe(struct platform_device *pdev)
{
	struct sysfs_reset_private *priv;
	struct property *prop;
	const char *pos = NULL;
	int rc;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "cannot get %zu bytes\n", sizeof(*priv));
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, priv);

	if (pdev->dev.platform_data) {
		rc = ciena_sysfs_reset_lkup(pdev, priv);
		if (rc) return rc;
	}
	else {
		prop = of_find_property(pdev->dev.of_node,
					"reset-names", NULL);
		if (NULL == prop) {
			dev_err(&pdev->dev, "%s missing reset-names property\n",
				pdev->dev.of_node->full_name);
			return -EINVAL;
		}
		while (NULL != (pos = of_prop_next_string(prop, pos)))
			priv->lkup_count++;
	}

	device_create_file(&pdev->dev, &dev_attr_reset);
	device_create_file(&pdev->dev, &dev_attr_assert);
	device_create_file(&pdev->dev, &dev_attr_deassert);
	device_create_file(&pdev->dev, &dev_attr_status);

	/* nudge a uevent from the device, because adding files under
	 * the platform device will not */
	kobject_uevent(&pdev->dev.kobj, KOBJ_ADD);

	dev_info(&pdev->dev, "exporting %u reset%s\n", priv->lkup_count,
		 1 < priv->lkup_count ? "s" : "");

	return 0;
}

static void ciena_sysfs_reset_remove(struct platform_device *pdev)
{
#ifdef CONFIG_CIENA_RESET_CONTROLLER_DEL_LOOKUP
	struct sysfs_reset_private *priv = platform_get_drvdata(pdev);

	if (priv->lkup)
		reset_controller_del_lookup(priv->lkup, priv->lkup_count);
#endif

	kobject_uevent(&pdev->dev.kobj, KOBJ_REMOVE);

	device_remove_file(&pdev->dev, &dev_attr_reset);
	device_remove_file(&pdev->dev, &dev_attr_assert);
	device_remove_file(&pdev->dev, &dev_attr_deassert);
	device_remove_file(&pdev->dev, &dev_attr_status);
}

static const struct of_device_id ciena_sysfs_reset_dt_ids[] = {
	{
		.compatible = "ciena,sysfs-reset",
		.type       = "reset-client",
	},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ciena_sysfs_reset_dt_ids);

static struct platform_driver ciena_sysfs_reset_driver = {
	.probe	  = ciena_sysfs_reset_probe,
	.remove	  = ciena_sysfs_reset_remove,
	.driver   = {
		.name		= RESET_SYSFS_DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= ciena_sysfs_reset_dt_ids,
	},
};
module_platform_driver(ciena_sysfs_reset_driver);

MODULE_AUTHOR("Dell Drummond");
MODULE_DESCRIPTION("Ciena sysfs reset interface");
MODULE_LICENSE("GPL");
