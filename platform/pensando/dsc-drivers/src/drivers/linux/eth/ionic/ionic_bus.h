/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#ifndef _IONIC_BUS_H_
#define _IONIC_BUS_H_

int ionic_bus_get_irq(struct ionic *ionic, unsigned int num);
const char *ionic_bus_info(struct ionic *ionic);
int ionic_bus_alloc_irq_vectors(struct ionic *ionic, unsigned int nintrs);
void ionic_bus_free_irq_vectors(struct ionic *ionic);
int ionic_bus_register_driver(void);
void ionic_bus_unregister_driver(void);
struct net_device *ionic_alloc_netdev(struct ionic *ionic);
void __iomem *ionic_bus_map_dbpage(struct ionic *ionic, int page_num);
void ionic_bus_unmap_dbpage(struct ionic *ionic, void __iomem *page);
phys_addr_t ionic_bus_phys_dbpage(struct ionic *ionic, int page_num);

void ionic_reset_prepare(struct pci_dev *pdev);
void ionic_reset_done(struct pci_dev *pdev);

static inline bool ionic_bus_dbpage_per_pid(struct ionic *ionic)
{
	return ionic->pdev;
}

#endif /* _IONIC_BUS_H_ */
