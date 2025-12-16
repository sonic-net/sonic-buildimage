/*! \file ngbde_procfs.c
 *
 * <description>
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

static int
proc_show(struct seq_file *m, void *v)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx, irq;
    struct ngbde_dmamem_s *dmamem;
    unsigned int pool;
    unsigned int dma_pools;
    char *dma_str;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    seq_printf(m, "Broadcom Device Enumerator (%s)\n", MOD_NAME);
#ifdef LKM_BUILD_INFO
    seq_printf(m, "%s\n", LKM_BUILD_INFO);
#endif
    seq_printf(m, "Found %d switch device(s):\n", num_swdev);
    for (idx = 0; idx < num_swdev; idx++) {
        if (swdev->inactive) {
            seq_printf(m, "%d:removed\n", idx);
            continue;
        }
        seq_printf(m, "%d:%04x:%04x:%02x,%s(%d", idx,
                   swdev->vendor_id, swdev->device_id, swdev->revision,
                   swdev->use_msi ? "MSI" : "IRQ",
                   swdev->intr_ctrl[0].irq_vect);
        for (irq = 1; irq < swdev->irq_max; irq++) {
            seq_printf(m, ",%d", swdev->intr_ctrl[irq].irq_vect);
        }
        seq_printf(m, ")\n");
    }

    seq_printf(m, "DMA pools:\n");
    for (idx = 0; idx < num_swdev; idx++) {
        seq_printf(m, "%d", idx);
        dma_pools = 0;
        for (pool = 0; pool < NGBDE_NUM_DMAPOOL_MAX; pool++) {
            dmamem = &swdev[idx].dmapool[pool].dmamem;
            dma_str = "unknown";
            if (dmamem->type == NGBDE_DMA_T_NONE) {
                /* Skip empty DMA pools */
                continue;
            } else  if (dmamem->type == NGBDE_DMA_T_KAPI) {
                dma_str = "kapi";
            } else  if (dmamem->type == NGBDE_DMA_T_PGMEM) {
                dma_str = "pgmem";
            }
            seq_printf(m, ":%dMB@0x%08lx(%s)",
                       (int)(dmamem->size / ONE_MB),
                       (unsigned long)dmamem->baddr, dma_str);
            dma_pools++;
        }
        if (dma_pools == 0) {
            seq_printf(m, ":none");
        }
        seq_printf(m, "\n");
    }

    return 0;
}

static int
proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

static int
proc_release(struct inode *inode, struct file *file)
{
    return single_release(inode, file);
}

struct proc_ops proc_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_release,
};

int
ngbde_procfs_init(void)
{
    struct proc_dir_entry *entry;

    PROC_CREATE(entry, MOD_NAME, 0666, NULL, &proc_fops);

    if (entry == NULL) {
        printk(KERN_ERR "ngbde: proc_create failed\n");
        return -1;
    }

    return 0;
}

int
ngbde_procfs_cleanup(void)
{
    remove_proc_entry(MOD_NAME, NULL);

    return 0;
}
