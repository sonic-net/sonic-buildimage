/*! \file ngbde_pci_probe.c
 *
 * NG BDE probe for PCI devices.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#include <ngbde.h>

/*! \cond */
static int use_msi = NGBDE_MSI_T_MSI;
module_param(use_msi, int, S_IRUSR);
MODULE_PARM_DESC(use_msi,
"Use MSI (1) or MSI-X (2) interrupts if supported by the kernel (default 1).");
/*! \endcond */

/*! \cond */
static int pci_debug = 0;
module_param(pci_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(pci_debug,
"PCI debug output enable (default 0).");
/*! \endcond */

/*!
 * Use BCMDRD_DEVLIST_ENTRY macro to generate a device list based on
 * supported/installed devices.
 */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    { _vn, _dv, PCI_ANY_ID, PCI_ANY_ID },

/*! Include all chip variants in the list of supported devices. */
#define BCMDRD_DEVLIST_INCLUDE_ALL

static struct pci_device_id pci_id_table[] = {
#include <bcmdrd/bcmdrd_devlist.h>
    { BROADCOM_VENDOR_ID, 0xb524, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, 0xb684, PCI_ANY_ID, PCI_ANY_ID },
    { 0, 0, 0, 0 }
};
MODULE_DEVICE_TABLE(pci, pci_id_table);

static int
pci_probe(struct pci_dev *pci_dev, const struct pci_device_id *ent)
{
    int rv;
    int bdx;
    int cmic_bar = 0;
    uint8_t rev;
    struct ngbde_dev_s *nd = NULL;
    int domain_no = pci_dev->bus ? pci_domain_nr(pci_dev->bus) : 0;
    int bus_no = pci_dev->bus ? pci_dev->bus->number : 0;
    int slot_no = PCI_SLOT(pci_dev->devfn);

    if (PCI_FUNC(pci_dev->devfn) > 0) {
        return 0;
    }

    if (pci_debug) {
        printk("PCI: pci_probe: slot=%04d:%02d:%02d dev=%04x:%04x\n",
               domain_no, bus_no, slot_no,
               pci_dev->vendor, pci_dev->device);
    }

    nd = kmalloc(sizeof(*nd), GFP_KERNEL);
    if (nd == NULL) {
        return -ENOMEM;
    }
    memset(nd, 0, sizeof(*nd));
    nd->pci_dev = pci_dev;
    nd->dma_dev = &pci_dev->dev;
    nd->vendor_id = pci_dev->vendor;
    nd->device_id = pci_dev->device;
    nd->domain_no = domain_no;
    nd->bus_no = bus_no;
    nd->slot_no = slot_no;

    /* PCI revision must extracted "manually" */
    pci_read_config_byte(pci_dev, PCI_REVISION_ID, &rev);
    nd->revision = rev;

    if (pci_enable_device(pci_dev)) {
        printk(KERN_WARNING "%s: Cannot enable PCI device: "
               "vendor_id = %x, device_id = %x\n",
               MOD_NAME, pci_dev->vendor, pci_dev->device);
    }
    pci_set_master(pci_dev);

    /* IRQ number is only valid if PCI device is enabled */
    nd->irq_line = pci_dev->irq;

    /* Check for iProc */
    if (pci_resource_len(pci_dev, 2)) {
        nd->iowin[1].addr = pci_resource_start(pci_dev, 0);
        nd->iowin[1].size = pci_resource_len(pci_dev, 0);
        cmic_bar = 2;
    }
    nd->iowin[0].addr = pci_resource_start(pci_dev, cmic_bar);
    nd->iowin[0].size = pci_resource_len(pci_dev, cmic_bar);

    /* Verify basic I/O access by reading first word of each BAR window */
    for (bdx = 0; bdx < 2; bdx++) {
        if (nd->iowin[bdx].size == 0) {
            continue;
        }
        if (ngbde_pio_map(nd, nd->iowin[bdx].addr, nd->iowin[bdx].size)) {
            if (pci_debug) {
                printk("PCI: BAR %d adddress 0 = 0x%x\n",
                       bdx, (unsigned int)ngbde_pio_read32(nd, 0));
            }
            ngbde_pio_unmap(nd);
        } else {
            printk(KERN_WARNING "%s: Cannot map PCI BAR %d: "
                   "start = %08lx, len = %lx\n",
                   MOD_NAME, bdx,
                   (unsigned long)nd->iowin[bdx].addr,
                   (unsigned long)nd->iowin[bdx].size);
        }
    }

    spin_lock_init(&nd->lock);

    /* Get MSI configuration preference from module parameter */
    nd->use_msi = use_msi;

    rv = ngbde_swdev_add(nd);

    if (rv == 0) {
        /* Update DMA pools for all devices */
        rv = ngbde_dma_init();
        if (rv < 0) {
            printk(KERN_WARNING "%s: Error initializing DMA memory\n",
                   MOD_NAME);
            /* Mark device as inactive */
            nd->inactive = 1;
        }
    }

    kfree(nd);

    return rv;
}

static void
pci_remove(struct pci_dev* pci_dev)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx;
    int domain_no = pci_dev->bus ? pci_domain_nr(pci_dev->bus) : 0;
    int bus_no = pci_dev->bus ? pci_dev->bus->number : 0;
    int slot_no = PCI_SLOT(pci_dev->devfn);

    if (pci_debug) {
        printk("PCI: pci_remove: slot=%04d:%02d:%02d dev=%04x:%04x\n",
               domain_no, bus_no, slot_no,
               pci_dev->vendor, pci_dev->device);
    }

    ngbde_swdev_get_all(&swdev, &num_swdev);
    for (idx = 0; idx < num_swdev; idx++) {
        if (swdev[idx].knet_func) {
            swdev[idx].knet_func(idx, NGBDE_EVENT_DEV_REMOVE,
                                 swdev->knet_data);
        }
        if (swdev[idx].bus_no == bus_no &&
            swdev[idx].slot_no == slot_no) {
            if (swdev[idx].inactive) {
                printk(KERN_WARNING "%s: Device already removed\n",
                       MOD_NAME);
            }
            /* Mark device as inactive (not present) */
            swdev[idx].inactive = 1;
        }
    }

    /* Update DMA pools for all devices */
    ngbde_dma_cleanup();
}

static struct pci_driver pci_driver = {
    .name = MOD_NAME,
    .probe = pci_probe,
    .remove = pci_remove,
    .id_table = pci_id_table,
    /* The rest are dynamic */
};

int
ngbde_pci_probe(void)
{
    if (pci_register_driver(&pci_driver) < 0) {
        return -ENODEV;
    }

    return 0;
}

int
ngbde_pci_cleanup(void)
{
    pci_unregister_driver(&pci_driver);

    return 0;
}
