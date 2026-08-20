#ifndef _GENERIC_CIC_COMMON_H
#define _GENERIC_CIC_COMMON_H

#include "generic_cic_config.h"
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>
#include <linux/sysfs.h>
#include <linux/types.h>

struct cic_gpio_export {
	struct device    *dev;
	const char       *label;
	bool              internal;
	struct gpio_desc *desc;
};

static inline bool cic_gpio_do_export(struct cic_gpio_export *exp)
{
	struct gpio_desc *desc;
	struct device    *dev      = exp->dev;
	const char       *label    = exp->label;
	bool              internal = exp->internal;
	int               rc;

	desc = gpiod_get(dev, label, GPIOD_ASIS);
	if (IS_ERR_OR_NULL(desc)) {
		dev_err(dev, "cannot get %s/%s (%ld)\n",
			dev_name(dev), label, PTR_ERR(desc));
		return false;
	}

	exp->desc = desc;

	/* Internal interrupts stay in the kernel. */
	if (internal) return true;

	rc = gpiod_export(desc, false);
	if (rc < 0) dev_warn(dev, "invalid gpio_export(%s/%s) = %d\n",
			     dev_name(dev), label, rc);

	rc = gpiod_export_link(exp->dev, label, desc);
	if (rc < 0) dev_warn(dev, "gpiod_export_link(%s/%s) = %d\n",
			     dev_name(dev), label, rc);

	return true;
}

static inline void cic_gpio_do_unexport(struct cic_gpio_export *exp)
{
	struct gpio_desc *desc     = exp->desc;
	struct device    *dev      = exp->dev;
	const char       *label    = exp->label;

	if (!desc) {
		dev_warn(dev, "%s: NULL descriptor for %s, skipping\n",
			 __func__, label ? label : "(null)");
		return;
	}

	if (!IS_ERR(desc)) {
		gpiod_unexport(desc);
		gpiod_put(desc);
	}
}

#endif /* _GENERIC_CIC_COMMON_H */
