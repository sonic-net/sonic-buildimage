// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/msi.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_debugfs.h"

#define IONIC_DEV_BAR         0
#define IONIC_INTR_CTRL_BAR   1
#define IONIC_MSIX_CFG_BAR    2
#define IONIC_DOORBELL_BAR    3
#define IONIC_TSTAMP_BAR      4

#define IONIC_REQUIRED_BARS   4
#define IONIC_NUM_OF_BAR      5

#define IONIC_INTR_MSIXCFG_STRIDE     0x10

struct ionic_intr_msixcfg {
	__le64 msgaddr;
	__le32 msgdata;
	__le32 vector_ctrl;
};

static void *ionic_intr_msixcfg_addr(struct device *mnic_dev, const int intr)
{
	struct ionic_dev *idev = (struct ionic_dev *) mnic_dev->platform_data;

	dev_info(mnic_dev, "msix_cfg_base: %p\n", idev->msix_cfg_base);
	return (idev->msix_cfg_base + (intr * IONIC_INTR_MSIXCFG_STRIDE));
}

static void ionic_intr_msixcfg(struct device *mnic_dev,
			       const int intr, const u64 msgaddr,
			       const u32 msgdata, const int vctrl)
{
	void *pa = ionic_intr_msixcfg_addr(mnic_dev, intr);

	writeq(msgaddr, (pa + offsetof(struct ionic_intr_msixcfg, msgaddr)));
	writel(msgdata, (pa + offsetof(struct ionic_intr_msixcfg, msgdata)));
	writel(vctrl, (pa + offsetof(struct ionic_intr_msixcfg, vector_ctrl)));
}

/* Resources can only be mapped once at a time.  A second mapping will fail.
 * For resources that are shared by multiple devices, we avoid using devm,
 * because the mapping will not be used exclusively by one device, and if
 * devices are unregistered in any order, the mapping must not be destroyed
 * when the first device is unregistered, when other devices may still be using
 * it.  ionic_shared_resource just maintains a refcount for mapping a shared
 * resource for use by multiple ionic devices.
 */
struct ionic_shared_resource {
	struct mutex lock;
	void __iomem *base;
	int refs;
};

#define IONIC_SHARED_RESOURCE_INITIALIZER(shres) { .lock = __MUTEX_INITIALIZER(shres.lock) }

static void __iomem *ionic_ioremap_shared_resource(struct ionic_shared_resource *shres,
						   struct resource *res)
{
	void __iomem *base;

	mutex_lock(&shres->lock);

	if (shres->refs) {
		base = shres->base;
		++shres->refs;
	} else {
		if (!request_mem_region(res->start, resource_size(res), res->name ?: KBUILD_MODNAME)) {
			base = IOMEM_ERR_PTR(-EBUSY);
		} else {
			base = ioremap(res->start, resource_size(res));
			if (!IS_ERR_OR_NULL(base)) {
				shres->base = base;
				++shres->refs;
			}
		}
	}

	mutex_unlock(&shres->lock);

	return base;
}

static void ionic_iounmap_shared_resource(struct ionic *ionic,
					  struct ionic_shared_resource *shres,
					  void __iomem *vaddr,
					  resource_size_t start,
					  resource_size_t n)
{
	mutex_lock(&shres->lock);

	if (!shres->refs) {
		dev_warn(ionic->dev, "%s: no references exist for shared resource\n",
			 __func__);
		mutex_unlock(&shres->lock);
		return;
	}

	--shres->refs;

	if (!shres->refs) {
		iounmap(vaddr);
		release_mem_region(start, n);
	}

	mutex_unlock(&shres->lock);
}

static struct ionic_shared_resource tstamp_res =
	IONIC_SHARED_RESOURCE_INITIALIZER(tstamp_res);

int ionic_bus_get_irq(struct ionic *ionic, unsigned int num)
{
	struct msi_desc *desc;
	int i = 0;

	msi_for_each_desc(desc, ionic->dev, MSI_DESC_ALL) {
		if (i == num) {
			pr_info("[i = %d] msi_entry: %d.%d\n",
				i, desc->msi_index,
				desc->irq);

			return desc->irq;
		}
		i++;
	}

	return -1; //return error if user is asking more irqs than allocated
}

const char *ionic_bus_info(struct ionic *ionic)
{
	return ionic->pfdev->name;
}

static void ionic_mnic_set_msi_msg(struct msi_desc *desc, struct msi_msg *msg)
{
	dev_dbg(desc->dev, "msi_index: [%d] (msi_addr hi_lo): %x_%x msi_data: %x\n",
		desc->msi_index, msg->address_hi,
		msg->address_lo, msg->data);

	ionic_intr_msixcfg(desc->dev, desc->msi_index,
		     (((u64)msg->address_hi << 32) | msg->address_lo),
		     msg->data, 0/*vctrl*/);
}

int ionic_bus_alloc_irq_vectors(struct ionic *ionic, unsigned int nintrs)
{
    struct device *dev = ionic->dev;
    struct irq_domain *domain;
    int err;

    /* 1. Explicitly associate the Platform MSI domain if not already set */
    if (!dev_get_msi_domain(dev)) {
        domain = of_msi_get_domain(dev, dev->of_node, DOMAIN_BUS_PLATFORM_MSI);
        if (domain) {
            dev_set_msi_domain(dev, domain);
        } else {
            dev_err(dev, "Failed to find Platform MSI domain\n");
            return -EINVAL;
        }
    }

    /* 2. Modern allocation helper with the required callback */
    err = platform_device_msi_init_and_alloc_irqs(dev, nintrs, ionic_mnic_set_msi_msg);
    if (err) {
        dev_err(dev, "Platform MSI allocation failed: %d\n", err);
        return err;
    }

    return nintrs;
}

void ionic_bus_free_irq_vectors(struct ionic *ionic)
{
	platform_device_msi_free_irqs_all(ionic->dev);
}

struct net_device *ionic_alloc_netdev(struct ionic *ionic)
{
	struct net_device *netdev = NULL;
	struct ionic_lif *lif;

	netdev = alloc_netdev_mqs(sizeof(struct ionic_lif), ionic->pfdev->name,
				  NET_NAME_USER, ether_setup,
				  ionic->ntxqs_per_lif, ionic->ntxqs_per_lif);
	if (!netdev)
		return netdev;

	lif = netdev_priv(netdev);

	/* lif name is used for naming the interrupt handler so better
	 * to name them differently for mnic
	 */
	snprintf(lif->name, sizeof(lif->name), "%s-", ionic->pfdev->name);

	return netdev;
}

static int ionic_mnic_dev_setup(struct ionic *ionic)
{
	unsigned int num_bars = ionic->num_bars;
	struct ionic_dev *idev = &ionic->idev;
	u32 sig;

	if (num_bars < IONIC_REQUIRED_BARS)
		return -EFAULT;

	idev->dev_info_regs = ionic->bars[IONIC_DEV_BAR].vaddr;
	idev->dev_cmd_regs = ionic->bars[IONIC_DEV_BAR].vaddr +
					offsetof(union ionic_dev_regs, devcmd);
	idev->intr_ctrl = ionic->bars[IONIC_INTR_CTRL_BAR].vaddr;
	idev->msix_cfg_base = ionic->bars[IONIC_MSIX_CFG_BAR].vaddr;
	if (num_bars > IONIC_TSTAMP_BAR)
		idev->hwstamp_regs = ionic->bars[IONIC_TSTAMP_BAR].vaddr;
	else
		idev->hwstamp_regs = NULL;

	/* save the idev into dev->platform_data so we can use it later */
	ionic->dev->platform_data = idev;

	sig = ioread32(&idev->dev_info_regs->signature);
	if (sig != IONIC_DEV_INFO_SIGNATURE)
		return -EFAULT;

	ionic_init_devinfo(ionic);
	ionic_watchdog_init(ionic);

	idev->db_pages = ionic->bars[IONIC_DOORBELL_BAR].vaddr;
	idev->phy_db_pages = ionic->bars[IONIC_DOORBELL_BAR].bus_addr;

	ionic_debugfs_add_dev_cmd(ionic);

	return 0;
}

static int ionic_map_bars(struct ionic *ionic)
{
	struct platform_device *pfdev = ionic->pfdev;
	struct ionic_dev_bar *bars = ionic->bars;
	struct device *dev = ionic->dev;
	struct resource *res;
	unsigned int i, j;
	void *base;

	ionic->num_bars = 0;
	for (i = 0, j = 0; i < IONIC_BARS_MAX; i++) {
		res = platform_get_resource(pfdev, IORESOURCE_MEM, i);
		if (!res)
			continue;
		if (i == IONIC_TSTAMP_BAR)
			base = ionic_ioremap_shared_resource(&tstamp_res, res);
		else
			base = devm_ioremap_resource(dev, res);
		if (IS_ERR(base)) {
			dev_err(dev, "Cannot memory-map BAR %d, aborting\n", j);
			return -ENODEV;
		}
		bars[j].len = res->end - res->start + 1;
		bars[j].vaddr = base;
		bars[j].bus_addr = res->start;
		ionic->num_bars++;
		j++;
	}

	ionic_debugfs_add_bars(ionic);

	return 0;
}

static void ionic_unmap_bars(struct ionic *ionic)
{
	struct ionic_dev_bar *bars = ionic->bars;
	struct device *dev = ionic->dev;
	unsigned int i;

	for (i = 0; i < IONIC_BARS_MAX; i++)
		if (bars[i].vaddr) {
			dev_info(dev, "Unmapping BAR %d @%p, bus_addr: %llx\n",
				 i, bars[i].vaddr, bars[i].bus_addr);
			if (i == IONIC_TSTAMP_BAR) {
				ionic_iounmap_shared_resource(ionic, &tstamp_res, bars[i].vaddr,
							      bars[i].bus_addr, bars[i].len);
			} else {
				devm_iounmap(dev, bars[i].vaddr);
				devm_release_mem_region(dev, bars[i].bus_addr, bars[i].len);
			}
		}
}

void __iomem *ionic_bus_map_dbpage(struct ionic *ionic, int page_num)
{
	return ionic->idev.db_pages;
}

void ionic_bus_unmap_dbpage(struct ionic *ionic, void __iomem *page)
{
}

phys_addr_t ionic_bus_phys_dbpage(struct ionic *ionic, int page_num)
{
	return ionic->idev.phy_db_pages;
}

int ionic_probe(struct platform_device *pfdev)
{
	struct device *dev = &pfdev->dev;
	struct device_node *np;
	struct ionic *ionic;
	int err;

	ionic = devm_kzalloc(dev, sizeof(*ionic), GFP_KERNEL);
	if (!ionic)
		return -ENOMEM;

	ionic->pfdev = pfdev;
	platform_set_drvdata(pfdev, ionic);
	ionic->dev = dev;
	mutex_init(&ionic->dev_cmd_lock);

	np = dev->of_node;
	if (!np) {
		dev_err(dev, "No device tree node\n");
		return -EINVAL;
	}

	err = of_reserved_mem_device_init_by_idx(dev, np, 0);
	if (err != 0 && err != -ENODEV) {
		dev_err(dev, "Failed to init reserved memory region\n");
		return err;
	}

	err = ionic_set_dma_mask(ionic);
	if (err) {
		dev_err(dev, "Cannot set DMA mask, aborting\n");
		return err;
	}

	ionic_debugfs_add_dev(ionic);

	/* Setup platform device */
	err = ionic_map_bars(ionic);
	if (err)
		goto err_out_unmap_bars;

	/* Discover ionic dev resources */
	err = ionic_mnic_dev_setup(ionic);
	if (err) {
		dev_err(dev, "Cannot setup device, aborting\n");
		goto err_out_unmap_bars;
	}

	err = ionic_identify(ionic);
	if (err) {
		dev_err(dev, "Cannot identify device, aborting\n");
		goto err_out_unmap_bars;
	}
	ionic_debugfs_add_ident(ionic);

	err = ionic_init(ionic);
	if (err) {
		dev_err(dev, "Cannot init device, aborting\n");
		goto err_out_unmap_bars;
	}

	/* Configure the ports */
	err = ionic_port_identify(ionic);
	if (err) {
		dev_err(dev, "Cannot identify port: %d, aborting\n", err);
		goto err_out_unmap_bars;
	}

	if (ionic->ident.port.type == IONIC_ETH_HOST_MGMT ||
	    ionic->ident.port.type == IONIC_ETH_MNIC_INTERNAL_MGMT)
		ionic->is_mgmt_nic = true;

	err = ionic_port_init(ionic);
	if (err) {
		dev_err(dev, "Cannot init port: %d, aborting\n", err);
		goto err_out_unmap_bars;
	}

	/* Allocate and init the LIF */
	err = ionic_lif_size(ionic);
	if (err) {
		dev_err(dev, "Cannot size LIF: %d, aborting\n", err);
		goto err_out_unmap_bars;
	}

	err = ionic_lif_alloc(ionic);
	if (err) {
		dev_err(dev, "Cannot allocate LIF: %d, aborting\n", err);
		goto err_out_free_irqs;
	}

	err = ionic_lif_init(ionic->lif);
	if (err) {
		dev_err(dev, "Cannot init LIF: %d, aborting\n", err);
		goto err_out_free_lifs;
	}

	err = ionic_lif_register(ionic->lif);
	if (err) {
		dev_err(dev, "Cannot register LIF: %d, aborting\n", err);
		goto err_out_deinit_lifs;
	}

	mod_timer(&ionic->watchdog_timer,
		  round_jiffies(jiffies + ionic->watchdog_period));

	return 0;

err_out_deinit_lifs:
	ionic_lif_deinit(ionic->lif);
err_out_free_lifs:
	ionic_lif_free(ionic->lif);
	ionic->lif = NULL;
err_out_free_irqs:
	ionic_bus_free_irq_vectors(ionic);
err_out_unmap_bars:
	ionic_unmap_bars(ionic);
	ionic_debugfs_del_dev(ionic);
	mutex_destroy(&ionic->dev_cmd_lock);
	platform_set_drvdata(pfdev, NULL);

	return err;
}
EXPORT_SYMBOL_GPL(ionic_probe);

void ionic_remove(struct platform_device *pfdev)
{
	struct ionic *ionic = platform_get_drvdata(pfdev);

	if (ionic) {
		del_timer_sync(&ionic->watchdog_timer);
		ionic_lif_unregister(ionic->lif);
		ionic_lif_deinit(ionic->lif);
		ionic_lif_free(ionic->lif);
		ionic->lif = NULL;
		ionic_port_reset(ionic);
		ionic_reset(ionic);
		ionic_bus_free_irq_vectors(ionic);
		ionic_unmap_bars(ionic);
		ionic_debugfs_del_dev(ionic);

		mutex_destroy(&ionic->dev_cmd_lock);

		dev_info(ionic->dev, "removed\n");
	}

	return;
}
EXPORT_SYMBOL_GPL(ionic_remove);

void ionic_reset_prepare(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "reset_prepare not supported\n");
}

void ionic_reset_done(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "reset_done not supported\n");
}

static const struct of_device_id mnic_of_match[] = {
		{.compatible = "pensando,ionic-mnic"},
			{/* end of table */}
};

static struct platform_driver ionic_driver = {
	.probe = ionic_probe,
	.remove = ionic_remove,
	.driver = {
		.name = "ionic-mnic",
		.owner = THIS_MODULE,
		.of_match_table = mnic_of_match,
	},
};

int ionic_bus_register_driver(void)
{
	return platform_driver_register(&ionic_driver);
}

void ionic_bus_unregister_driver(void)
{
	platform_driver_unregister(&ionic_driver);
}

