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
#include <linux/pci.h>
#include <linux/stddef.h>
#include <linux/uio_driver.h>

/* -------------------------------------------------------------------------- */
/* Resource adjustments required by platform sub-devices.
 */
static resource_size_t ciena_siril_dev_offset(struct device *dev);
static struct resource ciena_siril_default_irq_resource(struct device *dev);
static void            ciena_siril_default_irq_deresource(struct device *dev);

/* -------------------------------------------------------------------------- */
/* Default sub-device resource type. Individual drivers can override.
 */
#define CIENA_SIRIL_I2C_RESTYPE IORESOURCE_MEM
#define CIENA_SIRIL_SPI_RESTYPE IORESOURCE_MEM
#define CIENA_SIRIL_CIC_RESTYPE IORESOURCE_MEM
#define CIENA_SIRIL_RST_RESTYPE IORESOURCE_MEM
#define CIENA_SIRIL_WDG_RESTYPE IORESOURCE_MEM
#define CIENA_SIRIL_URT_RESTYPE IORESOURCE_MEM
#define CIENA_SIRIL_LED_RESTYPE IORESOURCE_REG

#include "sirilx_platform.h"

static const unsigned long ciena_siril_uio_resource_type[] = {
	[devtype_i2c]      = CIENA_SIRIL_I2C_RESTYPE,
	[devtype_spi]      = CIENA_SIRIL_SPI_RESTYPE,
	[devtype_cic]      = CIENA_SIRIL_CIC_RESTYPE,
	[devtype_reset]    = CIENA_SIRIL_RST_RESTYPE,
	[devtype_watchdog] = CIENA_SIRIL_WDG_RESTYPE,
	[devtype_uart]     = CIENA_SIRIL_URT_RESTYPE,
	[devtype_led]      = CIENA_SIRIL_LED_RESTYPE,
};

/* -------------------------------------------------------------------------- */
/* Resource adjustments: PCI implementation.
 */
static resource_size_t ciena_siril_dev_offset(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	int hwbar = CIENA_SIRIL_BAR;

	return pci_resource_start(pdev, hwbar);
}

static struct resource ciena_siril_default_irq_resource(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct resource res = {};

	res.start = res.end = pdev->irq;
	res.flags = IORESOURCE_IRQ;

	return res;
}

static void ciena_siril_default_irq_deresource(struct device *dev)
{
}

/* -------------------------------------------------------------------------- */
static int ciena_siril_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device        *dev = &pdev->dev;
	struct siril_priv    *priv;
	uint32_t              dev_index = 0;
	int                   nvec = 1;
	int                   bar;
	int                   rc;
	int                   hwbar;

	/*
	 * Set up resource types
	 */
	ciena_siril_resource_type = ciena_siril_uio_resource_type;

	priv = devm_kzalloc(dev, sizeof(struct siril_priv), GFP_KERNEL);

	if (priv == 0)
		return -ENOMEM;

	snprintf(priv->dev_name, sizeof(priv->dev_name), "%s#%u", UIO_NAME, dev_index);

	if ((rc = pcim_enable_device(pdev))) {
		dev_err(dev, "%s: pcim_enable_device() failed\n", __func__);
		goto out_free;
	}

	if (pci_request_regions(pdev, "Siril Registers"))
		goto out_free;

	/* Set up PCI device memory. */
	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		struct regmap_config rconfig = CIENA_REGMAP_CONFIG;
		char                 rname[128];

		hwbar = bar + CIENA_SIRIL_BAR;
		priv->info.mem[bar].addr = pci_resource_start(pdev, hwbar);
		if (!priv->info.mem[bar].addr)
			goto out_unmap;

		priv->info.mem[bar].internal_addr = pci_ioremap_bar(pdev, hwbar);
		if (!priv->info.mem[bar].internal_addr)
			goto out_unmap;

		priv->info.mem[bar].size    = pci_resource_len(pdev, hwbar);
		priv->info.mem[bar].memtype = UIO_MEM_PHYS;
		priv->info.mem[bar].name    = ciena_siril_bar_names[bar];

		dev_info(dev, "BAR[%d] addr=0x%llx internal_addr=%p size=0x%llx name=%s\n",
			 bar, priv->info.mem[bar].addr, priv->info.mem[bar].internal_addr,
			 priv->info.mem[bar].size, priv->info.mem[bar].name);

		/* Avoid name clashes. */
		if (0 != bar) {
			snprintf(rname, sizeof(rname), "bar-%d", bar);
			rconfig.name = rname;
		}

		priv->regs[bar] = devm_regmap_init_mmio(dev,
							priv->info.mem[bar].internal_addr,
							&rconfig);
		if (IS_ERR(priv->regs[bar])) {
			dev_warn(dev, "Couldn't create the regmap\n");
			goto out_unmap;
		}
	}

	dev_set_drvdata(dev, priv);

	/* As soon as we're able (device is mapped) apply some fixups so
	 * that we come up in a clean state. */
	if (sirilx_pdata.sirilx_apply_early_fixups)
		sirilx_pdata.sirilx_apply_early_fixups(priv->regs[0]);

	priv->info.name           = priv->dev_name;
	priv->info.version        = CIENA_SIRIL_DRIVER_VERSION;

#ifdef CONFIG_PCI_MSI
	if (pdev->msi_cap) {
		nvec = pci_msi_vec_count(pdev);
		rc   = pci_alloc_irq_vectors(pdev, 1, nvec, PCI_IRQ_MSI);
		if (rc < 0) {
			dev_err(dev, "failed to enable MSI [%d]\n", rc);
			goto out_unmap;
		}
		dev_info(dev, "MSI: %d advertised, %d allocated\n", nvec, rc);
		nvec = rc;

		if (CIENA_SIRIL_MSI_INDEX >= nvec) {
			dev_err(dev, "MSI index %d >= %d\n",
				CIENA_SIRIL_MSI_INDEX, nvec);
			rc = -ENODEV;
			goto out_disable_msi;
		}

		pdev->irq = pci_irq_vector(pdev, CIENA_SIRIL_MSI_INDEX);

		/* Enable bus mastering for this device (needed for MSI) */
		pci_set_master(pdev);

	} else
#endif
	{
		/* Legacy PCI interrupts are always shared. */
		priv->info.irq_flags = IRQF_SHARED;
	}

	if ((rc = uio_register_device(dev, &priv->info))) {
		dev_err(dev, "%s: uio_register_device() failed (%s)\n", __func__,
			priv->dev_name);
		goto out_disable_msi;
	}

	/* AER is enabled automatically by the PCI core since 6.12 */

	rc = ciena_siril_create_sub_devices(priv, dev);
	if (rc) goto out_unregister_uio;
	else ciena_siril_post_functions(dev, true);

	return 0;

out_unregister_uio:
	uio_unregister_device(&(priv->info));

out_disable_msi:
#ifdef CONFIG_PCI_MSI
	if (pdev->msi_cap)
		pci_free_irq_vectors(pdev);
#endif
out_unmap:
	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		if (priv->info.mem[bar].internal_addr)
			iounmap(priv->info.mem[bar].internal_addr);
	}
	pci_release_regions(pdev);
	pci_disable_device(pdev);
out_free:
	devm_kfree(dev, priv);
	return rc;
}


/* -------------------------------------------------------------------------- */
static void ciena_siril_shutdown(struct pci_dev *pdev)
{
	struct device     *dev = &pdev->dev;
	struct siril_priv *priv = dev_get_drvdata(dev);
	int bar;

	/* Remove the driver and all its children on shutdowns. This
	 * will revert FPGA selections (e.g. i2c, interrupts) back to
	 * their default states. Default states protect drivers which
	 * assume that all controllers are clean on the way back up.
	 */
	dev_info(&pdev->dev, "shutting down\n");

	/* bail out if shutdown/removal already happened */
	if (NULL == priv) return;

	ciena_siril_post_functions(dev, false);

	ciena_siril_destroy_sub_devices(dev);

	uio_unregister_device(&(priv->info));

#ifdef CONFIG_PCI_MSI
	if (pdev->msi_cap)
		pci_free_irq_vectors(pdev);
#endif

	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		iounmap(priv->info.mem[bar].internal_addr);
	}

	pci_release_regions(pdev);

	pci_disable_device(pdev);
	dev_set_drvdata(dev, NULL);

	devm_kfree(dev, priv);

	return;
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_remove (struct pci_dev *pdev)
{
	pr_info("%s: %s\n", pci_name(pdev), __func__);
	ciena_siril_shutdown(pdev);
}

/* --------------------------------------------------------------------------
 * PCI device infrastructure
 */


static struct pci_driver ciena_siril_pci_device_driver = {
	.name     = CIENA_SIRIL_DRIVER_NAME,
	.id_table = ciena_siril_device_id_table,
	.probe    = ciena_siril_probe,
	.remove   = ciena_siril_remove,
	.shutdown = ciena_siril_shutdown,
};

/* --------------------------------------------------------------------------
 * Kernel module infrastructure
 */

static int __init ciena_siril_init_module(void)
{
	int rc;

	rc = pci_register_driver(&ciena_siril_pci_device_driver);
	if (rc) {
		pr_err("%s: pci_register_driver failed, rc=%d\n", __func__, rc);
		return rc;
	}

	return rc;
}

static void __exit ciena_siril_exit_module(void)
{
	pci_unregister_driver(&ciena_siril_pci_device_driver);
}

module_init(ciena_siril_init_module);
module_exit(ciena_siril_exit_module);

MODULE_DESCRIPTION("UIO driver for the " MOD_NAME " FPGA");
MODULE_AUTHOR("Marc St-Amand <mstamand@ciena.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" CIENA_SIRIL_DRIVER_NAME);
MODULE_DEVICE_TABLE(pci, ciena_siril_device_id_table);
