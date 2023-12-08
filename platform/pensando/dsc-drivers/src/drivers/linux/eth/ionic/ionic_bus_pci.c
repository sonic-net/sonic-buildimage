// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/pci.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_debugfs.h"

/* Supported devices */
static const struct pci_device_id ionic_id_table[] = {
	{ PCI_VDEVICE(PENSANDO, PCI_DEVICE_ID_PENSANDO_IONIC_ETH_PF) },
	{ PCI_VDEVICE(PENSANDO, PCI_DEVICE_ID_PENSANDO_IONIC_ETH_VF) },
	{ PCI_VDEVICE(PENSANDO, PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT) },
	{ 0, }	/* end of table */
};
MODULE_DEVICE_TABLE(pci, ionic_id_table);

int ionic_bus_get_irq(struct ionic *ionic, unsigned int num)
{
#ifdef HAVE_PCI_IRQ_API
	return pci_irq_vector(ionic->pdev, num);
#else
	return ionic->msix[num].vector;
#endif
}

const char *ionic_bus_info(struct ionic *ionic)
{
	return pci_name(ionic->pdev);
}

int ionic_bus_alloc_irq_vectors(struct ionic *ionic, unsigned int nintrs)
{
#ifdef HAVE_PCI_IRQ_API
	return pci_alloc_irq_vectors(ionic->pdev, nintrs, nintrs,
				     PCI_IRQ_MSIX);
#else
	int err;
	int i;

	if (ionic->msix)
		return -EBUSY;

	ionic->msix = devm_kzalloc(ionic->dev,
				   sizeof(*ionic->msix) * nintrs, GFP_KERNEL);
	if (!ionic->msix)
		return -ENOMEM;
	for (i = 0; i < nintrs; i++)
		ionic->msix[i].entry = i;
	err = pci_enable_msix_exact(ionic->pdev, ionic->msix, nintrs);
	if (err < 0) {
		devm_kfree(ionic->dev, ionic->msix);
		ionic->msix = NULL;
		return err;
	}
	return nintrs;
#endif
}

void ionic_bus_free_irq_vectors(struct ionic *ionic)
{
	if (!ionic->nintrs)
		return;

#ifdef HAVE_PCI_IRQ_API
	pci_free_irq_vectors(ionic->pdev);
#else
	pci_disable_msix(ionic->pdev);
	devm_kfree(ionic->dev, ionic->msix);
	ionic->msix = NULL;
#endif
}

struct net_device *ionic_alloc_netdev(struct ionic *ionic)
{
	dev_dbg(ionic->dev, "nxqs=%d nlifs=%d nintrs=%d\n",
		ionic->ntxqs_per_lif, ionic->nlifs, ionic->nintrs);

	return alloc_etherdev_mqs(sizeof(struct ionic_lif),
				  ionic->ntxqs_per_lif, ionic->ntxqs_per_lif);
}

static int ionic_map_bars(struct ionic *ionic)
{
	struct pci_dev *pdev = ionic->pdev;
	struct device *dev = ionic->dev;
	struct ionic_dev_bar *bars;
	unsigned int i, j;

	bars = ionic->bars;
	ionic->num_bars = 0;

	for (i = 0, j = 0; i < IONIC_BARS_MAX; i++) {
		if (!(pci_resource_flags(pdev, i) & IORESOURCE_MEM))
			continue;
		bars[j].len = pci_resource_len(pdev, i);

		/* only map the whole bar 0 */
		if (j > 0) {
			bars[j].vaddr = NULL;
		} else {
			bars[j].vaddr = pci_iomap(pdev, i, bars[j].len);
			if (!bars[j].vaddr) {
				dev_err(dev,
					"Cannot memory-map BAR %d, aborting\n",
					i);
				return -ENODEV;
			}
		}

		bars[j].bus_addr = pci_resource_start(pdev, i);
		bars[j].res_index = i;
		ionic->num_bars++;
		j++;
	}

	ionic_debugfs_add_bars(ionic);

	return 0;
}

static void ionic_unmap_bars(struct ionic *ionic)
{
	struct ionic_dev_bar *bars = ionic->bars;
	unsigned int i;

	for (i = 0; i < IONIC_BARS_MAX; i++) {
		if (bars[i].vaddr) {
			iounmap(bars[i].vaddr);
			bars[i].bus_addr = 0;
			bars[i].vaddr = NULL;
			bars[i].len = 0;
		}
	}
}

void __iomem *ionic_bus_map_dbpage(struct ionic *ionic, int page_num)
{
#ifdef HAVE_PCI_IOMAP_RANGE
	return pci_iomap_range(ionic->pdev,
			       ionic->bars[IONIC_PCI_BAR_DBELL].res_index,
			       (u64)page_num << PAGE_SHIFT, PAGE_SIZE);
#else
	int bar = ionic->bars[IONIC_PCI_BAR_DBELL].res_index;
	phys_addr_t start = pci_resource_start(ionic->pdev, bar);
	phys_addr_t offset = start + ((phys_addr_t)page_num << PAGE_SHIFT);

	return ioremap(offset, PAGE_SIZE);
#endif /* HAVE_PCI_IOMAP_RANGE */
}

void ionic_bus_unmap_dbpage(struct ionic *ionic, void __iomem *page)
{
	iounmap(page);
}

phys_addr_t ionic_bus_phys_dbpage(struct ionic *ionic, int page_num)
{
	return ionic->bars[IONIC_PCI_BAR_DBELL].bus_addr +
		((phys_addr_t)page_num << PAGE_SHIFT);
}

static void ionic_vf_dealloc_locked(struct ionic *ionic)
{
	struct ionic_vf_setattr_cmd vfc = { .attr = IONIC_VF_ATTR_STATSADDR };
	struct ionic_vf *v;
	int i;

	if (!ionic->vfs)
		return;

	for (i = ionic->num_vfs - 1; i >= 0; i--) {
		v = &ionic->vfs[i];

		if (v->stats_pa) {
			vfc.stats_pa = 0;
			(void)ionic_set_vf_config(ionic, i, &vfc);
			dma_unmap_single(ionic->dev, v->stats_pa,
					 sizeof(v->stats), DMA_FROM_DEVICE);
			v->stats_pa = 0;
		}
	}

	kfree(ionic->vfs);
	ionic->vfs = NULL;
	ionic->num_vfs = 0;
}

static void ionic_vf_dealloc(struct ionic *ionic)
{
	down_write(&ionic->vf_op_lock);
	ionic_vf_dealloc_locked(ionic);
	up_write(&ionic->vf_op_lock);
}

static int ionic_vf_alloc(struct ionic *ionic, int num_vfs)
{
	struct ionic_vf_setattr_cmd vfc = { .attr = IONIC_VF_ATTR_STATSADDR };
	struct ionic_vf *v;
	int err = 0;
	int i;

	down_write(&ionic->vf_op_lock);

	ionic->vfs = kcalloc(num_vfs, sizeof(struct ionic_vf), GFP_KERNEL);
	if (!ionic->vfs) {
		err = -ENOMEM;
		goto out;
	}

	for (i = 0; i < num_vfs; i++) {
		v = &ionic->vfs[i];
		v->stats_pa = dma_map_single(ionic->dev, &v->stats,
					     sizeof(v->stats), DMA_FROM_DEVICE);
		if (dma_mapping_error(ionic->dev, v->stats_pa)) {
			dev_err(ionic->dev, "DMA mapping failed for vf[%d] stats\n", i);
			v->stats_pa = 0;
			err = -ENODEV;
			goto out;
		}

		ionic->num_vfs++;

		/* ignore failures from older FW, we just won't get stats */
		vfc.stats_pa = cpu_to_le64(v->stats_pa);
		(void)ionic_set_vf_config(ionic, i, &vfc);
	}

out:
	if (err)
		ionic_vf_dealloc_locked(ionic);
	up_write(&ionic->vf_op_lock);
	return err;
}

static int ionic_sriov_configure(struct pci_dev *pdev, int num_vfs)
{
	struct ionic *ionic = pci_get_drvdata(pdev);
	struct device *dev = ionic->dev;
	int ret = 0;

	if (ionic->lif &&
	    test_bit(IONIC_LIF_F_FW_RESET, ionic->lif->state))
		return -EBUSY;

	if (num_vfs > 0) {
		ret = pci_enable_sriov(pdev, num_vfs);
		if (ret) {
			dev_err(dev, "Cannot enable SRIOV: %d\n", ret);
			goto out;
		}

		ret = ionic_vf_alloc(ionic, num_vfs);
		if (ret) {
			dev_err(dev, "Cannot alloc VFs: %d\n", ret);
			pci_disable_sriov(pdev);
			goto out;
		}

		ret = num_vfs;
	} else {
		pci_disable_sriov(pdev);
		ionic_vf_dealloc(ionic);
	}

out:
	return ret;
}

static int ionic_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct device *dev = &pdev->dev;
	struct ionic *ionic;
	int num_vfs;
	int err;

	ionic = ionic_devlink_alloc(dev);
	if (!ionic)
		return -ENOMEM;

	ionic->pdev = pdev;
	ionic->dev = dev;
	pci_set_drvdata(pdev, ionic);
	mutex_init(&ionic->dev_cmd_lock);

	ionic->is_mgmt_nic =
		ent->device == PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT;
	ionic->pfdev = NULL;

	err = ionic_set_dma_mask(ionic);
	if (err) {
		dev_err(dev, "Cannot set DMA mask: %d, aborting\n", err);
		goto err_out_clear_drvdata;
	}

	ionic_debugfs_add_dev(ionic);

	/* Setup PCI device */
	err = pci_enable_device_mem(pdev);
	if (err) {
		dev_err(dev, "Cannot enable PCI device: %d, aborting\n", err);
		goto err_out_debugfs_del_dev;
	}

	err = pci_request_regions(pdev, IONIC_DRV_NAME);
	if (err) {
		dev_err(dev, "Cannot request PCI regions: %d, aborting\n", err);
		goto err_out_pci_disable_device;
	}
	pcie_print_link_status(pdev);

	err = ionic_map_bars(ionic);
	if (err)
		goto err_out_pci_release_regions;

	/* Configure the device */
	err = ionic_setup(ionic);
	if (err) {
		dev_err(dev, "Cannot setup device: %d, aborting\n", err);
		goto err_out_unmap_bars;
	}
	pci_set_master(pdev);

	err = ionic_identify(ionic);
	if (err) {
		dev_err(dev, "Cannot identify device: %d, aborting\n", err);
		goto err_out_teardown;
	}
	ionic_debugfs_add_ident(ionic);

	err = ionic_init(ionic);
	if (err) {
		dev_err(dev, "Cannot init device: %d, aborting\n", err);
		goto err_out_teardown;
	}

	/* Configure the ports */
	err = ionic_port_identify(ionic);
	if (err) {
		dev_err(dev, "Cannot identify port: %d, aborting\n", err);
		goto err_out_reset;
	}

	err = ionic_port_init(ionic);
	if (err) {
		dev_err(dev, "Cannot init port: %d, aborting\n", err);
		goto err_out_reset;
	}

	/* Allocate and init the LIF */
	err = ionic_lif_size(ionic);
	if (err) {
		dev_err(dev, "Cannot size LIF: %d, aborting\n", err);
		goto err_out_port_reset;
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

	init_rwsem(&ionic->vf_op_lock);
	num_vfs = pci_num_vf(pdev);
	if (num_vfs) {
		dev_info(dev, "%d VFs found already enabled\n", num_vfs);
		err = ionic_vf_alloc(ionic, num_vfs);
		if (err)
			dev_err(dev, "Cannot enable existing VFs: %d\n", err);
	}

	err = ionic_devlink_register(ionic);
	if (err) {
		dev_err(dev, "Cannot register devlink: %d\n", err);
		goto err_out_deinit_lifs;
	}

	err = ionic_lif_register(ionic->lif);
	if (err) {
		dev_err(dev, "Cannot register LIF: %d, aborting\n", err);
		goto err_out_deregister_devlink;
	}

	mod_timer(&ionic->watchdog_timer,
		  round_jiffies(jiffies + ionic->watchdog_period));

	return 0;

err_out_deregister_devlink:
	ionic_devlink_unregister(ionic);
err_out_deinit_lifs:
	ionic_vf_dealloc(ionic);
	ionic_lif_deinit(ionic->lif);
err_out_free_lifs:
	ionic_lif_free(ionic->lif);
	ionic->lif = NULL;
err_out_free_irqs:
	ionic_bus_free_irq_vectors(ionic);
err_out_port_reset:
	ionic_port_reset(ionic);
err_out_reset:
	ionic_reset(ionic);
err_out_teardown:
	ionic_dev_teardown(ionic);
	pci_clear_master(pdev);
	/* Don't fail the probe for these errors, keep
	 * the hw interface around for inspection
	 */
	return 0;

err_out_unmap_bars:
	ionic_unmap_bars(ionic);
err_out_pci_release_regions:
	pci_release_regions(pdev);
err_out_pci_disable_device:
	pci_disable_device(pdev);
err_out_debugfs_del_dev:
	ionic_debugfs_del_dev(ionic);
err_out_clear_drvdata:
	mutex_destroy(&ionic->dev_cmd_lock);
	ionic_devlink_free(ionic);
	pci_set_drvdata(pdev, NULL);

	return err;
}

static void ionic_remove(struct pci_dev *pdev)
{
	struct ionic *ionic = pci_get_drvdata(pdev);

	if (!ionic)
		return;

	del_timer_sync(&ionic->watchdog_timer);

	if (ionic->lif) {
		ionic_lif_unregister(ionic->lif);
		ionic_devlink_unregister(ionic);
		ionic_lif_deinit(ionic->lif);
		ionic_lif_free(ionic->lif);
		ionic->lif = NULL;
		ionic_bus_free_irq_vectors(ionic);
	}

	ionic_port_reset(ionic);
	ionic_reset(ionic);
	ionic_dev_teardown(ionic);
	pci_clear_master(pdev);
	ionic_unmap_bars(ionic);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	ionic_debugfs_del_dev(ionic);
	mutex_destroy(&ionic->dev_cmd_lock);
	ionic_devlink_free(ionic);
}

static struct pci_driver ionic_driver = {
	.name = IONIC_DRV_NAME,
	.id_table = ionic_id_table,
	.probe = ionic_probe,
	.remove = ionic_remove,
	.sriov_configure = ionic_sriov_configure,
};

int ionic_bus_register_driver(void)
{
	return pci_register_driver(&ionic_driver);
}

void ionic_bus_unregister_driver(void)
{
	pci_unregister_driver(&ionic_driver);
}
