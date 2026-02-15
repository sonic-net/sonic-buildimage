/*! \file ngbde_ioctl.c
 *
 * NGBDE IOCTL interface.
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

#include <lkm/ngbde_ioctl.h>

#include <ngbde.h>

long
ngbde_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ngbde_ioc_cmd_s ioc;
    struct ngbde_dev_s *swdev;
    struct ngbde_irq_reg_s ireg;
    struct ngbde_intr_ack_reg_s ackreg;
    phys_addr_t addr, size;
    unsigned int num_swdev;
    unsigned int rsrc_type, rsrc_idx;
    unsigned int irq_num, intr_cmd;
    int rv;
    uint32_t mreg, mval;

    if (copy_from_user(&ioc, (void *)arg, sizeof(ioc))) {
        return -EFAULT;
    }

    ioc.rc = NGBDE_IOC_SUCCESS;

    switch (cmd) {
    case NGBDE_IOC_MOD_INFO:
        ioc.op.mod_info.version = NGBDE_IOC_VERSION;
        ioc.op.mod_info.compat = NGBDE_COMPAT_IRQ_INIT;
        break;
    case NGBDE_IOC_PROBE_INFO:
        ngbde_swdev_get_all(NULL, &num_swdev);
        ioc.op.probe_info.num_swdev = num_swdev;
        break;
    case NGBDE_IOC_DEV_INFO:
        swdev = ngbde_swdev_get(ioc.devid);
        if (!swdev) {
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        ioc.op.dev_info.device_type = 0;
        ioc.op.dev_info.bus_type = NGBDE_DEV_BT_AXI;
        if (swdev->pci_dev) {
            ioc.op.dev_info.bus_type = NGBDE_DEV_BT_PCI;
        }
        ioc.op.dev_info.flags = 0;
        if (swdev->use_msi) {
            ioc.op.dev_info.flags |= NGBDE_DEV_F_MSI;
        }
        if (swdev->inactive) {
            ioc.op.dev_info.flags |= NGBDE_DEV_F_INACTIVE;
        }
        ioc.op.dev_info.vendor_id = swdev->vendor_id;
        ioc.op.dev_info.device_id = swdev->device_id;
        ioc.op.dev_info.revision = swdev->revision;
        ioc.op.dev_info.model = swdev->model;
        break;
    case NGBDE_IOC_PHYS_ADDR:
        swdev = ngbde_swdev_get(ioc.devid);
        if (!swdev) {
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        rsrc_type = ioc.op.rsrc_id.type;
        rsrc_idx = ioc.op.rsrc_id.inst;
        switch (rsrc_type) {
        case NGBDE_IO_RSRC_DEV_IO:
            if (rsrc_idx >= NGBDE_NUM_IOWIN_MAX) {
                printk(KERN_WARNING
                       "ngbde: invalid resource index (%d)\n",
                       rsrc_idx);
                ioc.rc = NGBDE_IOC_FAIL;
                break;
            }
            ioc.op.phys_addr.addr = swdev->iowin[rsrc_idx].addr;
            ioc.op.phys_addr.size = swdev->iowin[rsrc_idx].size;
            break;
        case NGBDE_IO_RSRC_DMA_MEM:
            if (rsrc_idx >= NGBDE_NUM_DMAPOOL_MAX) {
                printk(KERN_WARNING
                       "ngbde: invalid resource index (%d)\n",
                       rsrc_idx);
                ioc.rc = NGBDE_IOC_FAIL;
                break;
            }
            ioc.op.phys_addr.addr = swdev->dmapool[rsrc_idx].dmamem.paddr;
            ioc.op.phys_addr.size = swdev->dmapool[rsrc_idx].dmactrl.size;
            break;
        case NGBDE_IO_RSRC_DMA_BUS:
            if (rsrc_idx >= NGBDE_NUM_DMAPOOL_MAX) {
                printk(KERN_WARNING
                       "ngbde: invalid resource index (%d)\n",
                       rsrc_idx);
                ioc.rc = NGBDE_IOC_FAIL;
                break;
            }
            ioc.op.phys_addr.addr = swdev->dmapool[rsrc_idx].dmamem.baddr;
            ioc.op.phys_addr.size = swdev->dmapool[rsrc_idx].dmactrl.size;
            break;
        default:
            printk(KERN_WARNING
                   "ngbde: unknown resource type (%d)\n",
                   rsrc_type);
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        break;
    case NGBDE_IOC_INTR_CTRL:
        irq_num = ioc.op.intr_ctrl.irq_num;
        intr_cmd = ioc.op.intr_ctrl.cmd;
        switch (intr_cmd) {
        case NGBDE_ICTL_INTR_CONN:
            if (ngbde_intr_connect(ioc.devid, irq_num) < 0) {
                ioc.rc = NGBDE_IOC_FAIL;
            }
            break;
        case NGBDE_ICTL_INTR_DISC:
            if (ngbde_intr_disconnect(ioc.devid, irq_num) < 0) {
                ioc.rc = NGBDE_IOC_FAIL;
            }
            break;
        case NGBDE_ICTL_INTR_WAIT:
            if (ngbde_intr_wait(ioc.devid, irq_num) < 0) {
                ioc.rc = NGBDE_IOC_FAIL;
            }
            break;
        case NGBDE_ICTL_INTR_STOP:
            if (ngbde_intr_stop(ioc.devid, irq_num) < 0) {
                ioc.rc = NGBDE_IOC_FAIL;
            }
            break;
        case NGBDE_ICTL_REGS_CLR:
            if (ngbde_intr_regs_clr(ioc.devid, irq_num) < 0) {
                ioc.rc = NGBDE_IOC_FAIL;
            }
            break;
        default:
            printk(KERN_WARNING
                   "%s: unknown interrupt control command (%d)\n",
                   MOD_NAME, intr_cmd);
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        break;
    case NGBDE_IOC_IRQ_REG_ADD:
        irq_num = ioc.op.irq_reg_add.irq_num;
        ireg.status_reg = ioc.op.irq_reg_add.status_reg;
        ireg.mask_reg = ioc.op.irq_reg_add.mask_reg;
        ireg.umask = 0;
        ireg.kmask = 0;
        ireg.kmask_valid = false;
        if (ioc.op.irq_reg_add.flags & NGBDE_IRQ_REG_F_KMASK) {
            ireg.kmask = ioc.op.irq_reg_add.kmask;
            ireg.kmask_valid = true;
        }
        if (ioc.op.irq_reg_add.flags & NGBDE_IRQ_REG_F_UMASK) {
            ireg.umask = ioc.op.irq_reg_add.umask;
        } else {
            /*
             * Assign non-kernel bits to user mode driver. Note that
             * this functionality is intended to provide backward
             * compatibility.
             */
            ireg.umask = ~ioc.op.irq_reg_add.kmask;
        }
        ireg.status_is_masked = false;
        if (ioc.op.irq_reg_add.flags & NGBDE_IRQ_REG_F_MASKED) {
            ireg.status_is_masked = true;
        }
        ireg.mask_w1tc = false;
        if (ioc.op.irq_reg_add.flags & NGBDE_IRQ_REG_F_W1TC) {
            ireg.mask_w1tc = true;
        }
        if (ngbde_intr_reg_add(ioc.devid, irq_num, &ireg) < 0) {
            printk(KERN_WARNING
                   "%s: Unable to add interrupt register\n",
                   MOD_NAME);
            ioc.rc = NGBDE_IOC_FAIL;
        }
        break;
    case NGBDE_IOC_IACK_REG_ADD:
        irq_num = ioc.op.iack_reg_add.irq_num;
        ackreg.ack_valid = true;
        ackreg.ack_domain = NGBDE_INTR_ACK_IO_DEV;
        if (ioc.op.iack_reg_add.flags & NGBDE_IACK_REG_F_PAXB) {
            ackreg.ack_domain = NGBDE_INTR_ACK_IO_PAXB;
        }
        ackreg.ack_reg = ioc.op.iack_reg_add.ack_reg;
        ackreg.ack_val = ioc.op.iack_reg_add.ack_val;
        if (ngbde_intr_ack_reg_add(ioc.devid, irq_num, &ackreg) < 0) {
            printk(KERN_WARNING
                   "%s: Unable to add interrupt ack register\n",
                   MOD_NAME);
            ioc.rc = NGBDE_IOC_FAIL;
        }
        break;
    case NGBDE_IOC_IRQ_MASK_WR:
        irq_num = ioc.op.irq_mask_wr.irq_num;
        mreg = ioc.op.irq_mask_wr.offs;
        mval = ioc.op.irq_mask_wr.val;
        if (ngbde_intr_mask_write(ioc.devid, irq_num, 0, mreg, mval) < 0) {
            printk(KERN_WARNING
                   "%s: Unable to write shared register\n",
                   MOD_NAME);
            ioc.rc = NGBDE_IOC_FAIL;
        }
        break;
    case NGBDE_IOC_IRQ_INIT:
        rv = ngbde_intr_alloc(ioc.devid, ioc.op.irq_init.irq_max);
        if (rv < 0) {
            ioc.rc = NGBDE_IOC_FAIL;
        } else {
            ioc.op.irq_init.irq_max = rv;
        }
        break;
    case NGBDE_IOC_PIO_WIN_MAP:
        swdev = ngbde_swdev_get(ioc.devid);
        if (!swdev) {
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        addr = ioc.op.pio_win.addr;
        size = ioc.op.pio_win.size;
        if (ngbde_pio_map(swdev, addr, size) == NULL) {
            ioc.rc = NGBDE_IOC_FAIL;
        }
        break;
    case NGBDE_IOC_IIO_WIN_MAP:
        swdev = ngbde_swdev_get(ioc.devid);
        if (!swdev) {
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        addr = ioc.op.pio_win.addr;
        size = ioc.op.pio_win.size;
        if (ngbde_iio_map(swdev, addr, size) == NULL) {
            ioc.rc = NGBDE_IOC_FAIL;
        }
        break;
    case NGBDE_IOC_PAXB_WIN_MAP:
        swdev = ngbde_swdev_get(ioc.devid);
        if (!swdev) {
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        addr = ioc.op.pio_win.addr;
        size = ioc.op.pio_win.size;
        if (ngbde_paxb_map(swdev, addr, size) == NULL) {
            ioc.rc = NGBDE_IOC_FAIL;
        }
        break;
    case NGBDE_IOC_SLOT_INFO:
        swdev = ngbde_swdev_get(ioc.devid);
        if (!swdev) {
            ioc.rc = NGBDE_IOC_FAIL;
            break;
        }
        ioc.op.slot_info.domain_no = swdev->domain_no;
        ioc.op.slot_info.bus_no = swdev->bus_no;
        ioc.op.slot_info.slot_no = swdev->slot_no;
        ioc.op.slot_info.func_no = 0; /* unused */
        break;
    default:
        printk(KERN_ERR "ngbde: invalid ioctl (%08x)\n", cmd);
        ioc.rc = NGBDE_IOC_FAIL;
        break;
    }

    if (copy_to_user((void *)arg, &ioc, sizeof(ioc))) {
        return -EFAULT;
    }

    return 0;
}
