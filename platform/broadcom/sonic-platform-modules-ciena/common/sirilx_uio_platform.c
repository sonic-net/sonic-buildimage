/*
 * UIO driver for the Siril FPGA
 * *
 * Copyright (C) 2013  Ciena Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <asm/byteorder.h>
#include <linux/aer.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/uio_driver.h>
#include <linux/acpi.h>
#include <linux/irq.h>

/* -------------------------------------------------------------------------- */
/* Resource adjustments required by platform sub-devices.
 */
static resource_size_t ciena_siril_dev_offset(struct device *dev);
static struct resource ciena_siril_default_irq_resource(struct device *dev);
static void            ciena_siril_default_irq_deresource(struct device *dev);

#include "sirilx_platform.h"

static const unsigned long ciena_siril_platform_res_type[] = {
	[devtype_i2c]      = IORESOURCE_REG,
	[devtype_spi]      = IORESOURCE_MEM,
	[devtype_cic]      = IORESOURCE_REG,
	[devtype_reset]    = IORESOURCE_MEM,
	[devtype_watchdog] = IORESOURCE_MEM,
	[devtype_uart]     = IORESOURCE_MEM,
	[devtype_led]      = IORESOURCE_REG,
};

/* -------------------------------------------------------------------------- */
/* Resource adjustments: ACPI implementation.
 */
static resource_size_t ciena_siril_dev_offset(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	return mem->start;
}

static struct resource ciena_siril_default_irq_resource(struct device *dev)
{
	struct siril_priv  *priv = dev_get_drvdata(dev);
	struct acpi_device *adev = priv->info.priv;
	struct resource     res  = {};

	if (adev) {
		res.start = res.end = acpi_dev_gpio_irq_get(adev, 0);
		res.flags = IORESOURCE_IRQ;
	}

	return res;
}

static void ciena_siril_default_irq_deresource(struct device *dev)
{
}

/* -------------------------------------------------------------------------- */
static int ciena_siril_probe(struct platform_device *pdev)
{
	struct device        *dev = &pdev->dev;
	struct siril_priv    *priv;
	uint32_t              dev_index = 0;
	struct resource      *mem;
	struct uio_mem       *um;
	int                   bar;
	int                   rc;

	/*
	 * Set up resource types
	 */
	ciena_siril_resource_type = ciena_siril_platform_res_type;

	priv = devm_kzalloc(dev, sizeof(struct siril_priv), GFP_KERNEL);
	if (priv == 0)
		return -ENOMEM;

	snprintf(priv->dev_name, sizeof(priv->dev_name), "%s#%u",
		 UIO_NAME, dev_index);

#ifdef CONFIG_ACPI
	{
		acpi_handle         handle = ACPI_HANDLE(dev);
		struct acpi_device *acpi_device;

		acpi_device = acpi_fetch_acpi_dev(handle);
		if (!acpi_device)
			return -ENODEV;

		priv->info.priv = acpi_device;
	}
#endif

	/* Set up platform device memory. */
	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		um  = &priv->info.mem[bar];
		mem = platform_get_resource(pdev, IORESOURCE_MEM, bar);

		um->addr = mem->start;
		if (!um->addr) {
			pr_err("%s: no addr\n", __func__);
			goto out_unmap;
		}

		um->size    = resource_size(mem);
		um->memtype = UIO_MEM_PHYS;
		um->name    = ciena_siril_bar_names[bar];

		request_mem_region(um->addr, um->size, pdev->name);

		um->internal_addr = devm_ioremap(&pdev->dev,
						 mem->start,
						 resource_size(mem));
		if (!um->internal_addr) {
			pr_err("%s: no internal_addr\n", __func__);
			goto out_unmap;
		}

		priv->regs[bar] = devm_regmap_init_mmio(&pdev->dev,
							um->internal_addr,
							&CIENA_REGMAP_CONFIG);
		if (IS_ERR(priv->regs[bar])) {
			dev_warn(&pdev->dev, "Couldn't create the regmap\n");
			goto out_unmap;
		}
	}

	dev_set_drvdata(dev, priv);

	/* As soon as we're able (device is mapped) apply some fixups so
	 * that we come up in a clean state. */
	if (sirilx_pdata.sirilx_apply_early_fixups)
		sirilx_pdata.sirilx_apply_early_fixups(priv->regs[0]);

	priv->info.name    = priv->dev_name;
	priv->info.version = CIENA_SIRIL_DRIVER_VERSION;

	if ((rc = uio_register_device(dev, &priv->info))) {
		dev_err(dev, "%s: uio_register_device() failed "
			"(%s), rc %d\n", __func__, priv->dev_name, rc);
		goto out_unmap;
	}

	rc = ciena_siril_create_sub_devices(priv, dev);
	if (rc) goto out_unregister_uio;
	else ciena_siril_post_functions(dev, true);

	return 0;

out_unregister_uio:
	uio_unregister_device(&(priv->info));

out_unmap:
	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		release_mem_region(priv->info.mem[bar].addr,
				   priv->info.mem[bar].size);
	}
	return rc;
}


/* -------------------------------------------------------------------------- */
static void ciena_siril_remove (struct platform_device *pdev)
{
	struct device     *dev = &pdev->dev;
	struct siril_priv *priv = dev_get_drvdata(dev);
	int bar;

	ciena_siril_post_functions(dev, false);

	ciena_siril_destroy_sub_devices(dev);

	uio_unregister_device(&(priv->info));

	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		release_mem_region(priv->info.mem[bar].addr,
				   priv->info.mem[bar].size);
	}

	dev_set_drvdata(dev, NULL);
}

/* --------------------------------------------------------------------------
 * Kernel module infrastructure
 */

static struct platform_driver ciena_platform_device_driver = {
	.probe		= ciena_siril_probe,
	.remove		= ciena_siril_remove,
#ifdef CIENA_HAS_OF_MATCH
	.driver = {
		.name           = CIENA_SIRIL_DRIVER_NAME,
		.owner          = THIS_MODULE,
		.of_match_table = ciena_siril_device_id_table,
	},
#endif
};

module_platform_driver(ciena_platform_device_driver);

MODULE_DESCRIPTION("UIO driver for the " MOD_NAME " FPGA");
MODULE_AUTHOR("Shaokong Kao <skao@ciena.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" CIENA_SIRIL_DRIVER_NAME);
#ifdef CIENA_HAS_OF_MATCH
MODULE_DEVICE_TABLE(of, ciena_siril_device_id_table);
#endif
