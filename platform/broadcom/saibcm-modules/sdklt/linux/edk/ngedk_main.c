/*! \file ngedk_main.c
 *
 * EDK support module entry.
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

#include <lkm/lkm.h>
#include <lkm/ngbde_kapi.h>
#include <lkm/ngedk_ioctl.h>
#include <lkm/ngedk_kapi.h>

/*! \cond */
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("EDK Support Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! Maximum number of switch devices supported. */
#ifndef NGEDK_NUM_SWDEV_MAX
#define NGEDK_NUM_SWDEV_MAX     NGBDE_NUM_SWDEV_MAX
#endif

/*! Kernel DMA API (dma_alloc_coherent). */
#define NGEDK_DMA_T_KAPI        1

/*! Page allocator and map to physical address manually. */
#define NGEDK_DMA_T_PGMEM       2

/* Structure to hold info about interrupts handled by EDK */
typedef struct edk_intr_s {

    /*! Unit Level Enable */
    uint32_t enable;

    /*! Active cores */
    uint32_t active_bmp;

    /*! Timer interrupts status offset */
    uint32_t timer_intrc_stat_reg;

    /*! Timer interrupts disable offset */
    uint32_t timer_intrc_disable_reg;

    /*! Timer interrupts mask */
    uint32_t timer_intrc_mask_val;

    /*! Bitmap of cores that asserted SW Programmable Interrupt */
    volatile unsigned long swi_intr_cores;

    /*! EDK interrupt flags */
    uint32_t flags;

} edk_intr_t;

/*! Switch device descriptor. */
typedef struct edk_dev_s {

    /*! Kernel device number (similar to user mode unit number). */
    int kdev;



    /*! Logical address of DMA pool. */
    void *dma_vaddr;

    /*! Physical address of DMA pool. */
    dma_addr_t dma_paddr;

    /*! Bus address of DMA pool. */
    dma_addr_t dma_baddr;

    /*! Size of DMA memory (in bytes). */
    size_t dma_size;

    /*! DMA pool type. KNET or PGMEM */
    int dma_type;

    /*! Linux DMA device associated with DMA pool. */
    struct device *dma_dev;

    /* EDK Interrupt detail */
    struct edk_intr_s edk_intr;

    /*! Wait queue for edk interrupt thread. */
    wait_queue_head_t edk_thread_wq;

    /*! Flag to wake up edk interrupt thread. */
    atomic_t run_edk_thread;

    /*! Number of interrupts processed. */
    unsigned long intr_cnt;

} edk_dev_t;

static edk_dev_t edkdevs[NGBDE_NUM_SWDEV_MAX];

static int
ngedk_dmamem_alloc(edk_dev_t *edkdev, size_t size)
{
    void *vaddr;
    dma_addr_t baddr;

    if (edkdev->dma_vaddr) {
        /* Already allocated */
        return 0;
    }

#ifdef CONFIG_CMA
    vaddr = dma_alloc_coherent(edkdev->dma_dev, size, &baddr,
                               GFP_KERNEL | GFP_DMA32);
    if (vaddr) {
        /* Store allocation information in the edk device */
        edkdev->dma_vaddr = vaddr;
        edkdev->dma_paddr = virt_to_phys(vaddr);
        edkdev->dma_baddr = baddr;
        edkdev->dma_size = size;
        edkdev->dma_type = NGEDK_DMA_T_KAPI;
        return 0;
    }
#endif
    vaddr = ngbde_kapi_dma_alloc(size);
    if (vaddr) {
        /* Store allocation information in the edk device */
        edkdev->dma_vaddr = vaddr;
        edkdev->dma_paddr = virt_to_phys(vaddr);
        edkdev->dma_size = size;
        edkdev->dma_type = NGEDK_DMA_T_PGMEM;
        baddr = dma_map_single(edkdev->dma_dev, edkdev->dma_vaddr,
                               edkdev->dma_size, DMA_BIDIRECTIONAL);
        if (dma_mapping_error(edkdev->dma_dev, baddr)) {
            edkdev->dma_baddr = 0;
            printk("EDK: Failed to map PGMEM memory\n");
        } else {
            edkdev->dma_baddr = baddr;
        }
        return 0;
    }

    edkdev->dma_vaddr = NULL;
    return -1;
}

static void
ngedk_dmamem_free(edk_dev_t *edkdev)
{
    if (edkdev->dma_type == NGEDK_DMA_T_KAPI) {
        dma_free_coherent(edkdev->dma_dev, edkdev->dma_size,
                          edkdev->dma_vaddr, edkdev->dma_baddr);
    } else if (edkdev->dma_type == NGEDK_DMA_T_PGMEM) {

        if (edkdev->dma_baddr) {
            dma_unmap_single(edkdev->dma_dev, edkdev->dma_baddr,
                             edkdev->dma_size, DMA_BIDIRECTIONAL);
        }
        ngbde_kapi_dma_free(edkdev->dma_vaddr);
    }
    edkdev->dma_type = 0;
    edkdev->dma_vaddr = 0;
}

void *
ngedk_dmamem_map_p2v(dma_addr_t paddr)
{
    struct edk_dev_s *ed;
    ed = &edkdevs[0];
    if ((paddr >= ed->dma_baddr) &&
        (paddr < (ed->dma_baddr + ed->dma_size))) {
        return (ed->dma_vaddr + (paddr - ed->dma_baddr));
    } else {
        return NULL;
    }
}
EXPORT_SYMBOL(ngedk_dmamem_map_p2v);

static int
ngedk_intr_wait(int kdev, uint32_t *uc_bmp)
{
    int32_t core;
    struct edk_dev_s *ed = &edkdevs[kdev];

    wait_event_interruptible(ed->edk_thread_wq,
                             atomic_read(&ed->run_edk_thread) != 0);
    atomic_set(&ed->run_edk_thread, 0);

    *uc_bmp = 0;
    if (ed->edk_intr.swi_intr_cores) {
        /* We got a SW Interrupt */
        for (core = 0; core < MCS_NUM_UC; core++) {
            if (test_and_clear_bit(core, &(ed->edk_intr.swi_intr_cores))) {

               *uc_bmp |= (1 << core);
            }
        }
    }
    return 0;
}

static int
ngedk_swdev_init(int kdev)
{
    struct device *edk_dev;
    struct edk_dev_s *ed = &edkdevs[kdev];

    edk_dev = ngbde_kapi_dma_dev_get(kdev);
    if (edk_dev) {
        printk(KERN_INFO "Found EDK dev %d\n", kdev);
        ed->kdev = kdev;
        ed->dma_dev = edk_dev;
        ed->edk_intr.swi_intr_cores = 0;
        init_waitqueue_head(&ed->edk_thread_wq);
        atomic_set(&ed->run_edk_thread, 0);
    }

    return 0;
}

static int
ngedk_swdev_cleanup(int kdev)
{
    struct edk_dev_s *ed = &edkdevs[kdev];

    if (ed->dma_dev) {
        printk(KERN_INFO "Clean up EDK dev %d\n", kdev);
        /* Wake up edk thread */
        atomic_set(&ed->run_edk_thread, 1);
        wake_up_interruptible(&ed->edk_thread_wq);
    }
    if (ed->dma_vaddr) {
        ngedk_dmamem_free(ed);
    }
    memset(ed, 0, sizeof(*ed));

    return 0;
}

static int
ngedk_attach(void)
{
    int kdev;

    for (kdev = 0; kdev < NGEDK_NUM_SWDEV_MAX; kdev++) {
        ngedk_swdev_init(kdev);
    }

    return 0;
}

static int
ngedk_detach(void)
{
    int kdev;

    for (kdev = 0; kdev < NGEDK_NUM_SWDEV_MAX; kdev++) {
        ngedk_swdev_cleanup(kdev);
    }

    return 0;
}

static int
proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Broadcom EDK Support (%s)\n", NGEDK_MODULE_NAME);

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

static struct proc_ops proc_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        proc_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     proc_release,
};

static int
ngedk_procfs_init(void)
{
    struct proc_dir_entry *entry;

    PROC_CREATE(entry, NGEDK_MODULE_NAME, 0666, NULL, &proc_fops);

    if (entry == NULL) {
        printk(KERN_ERR "ngedk: proc_create failed\n");
        return -1;
    }

    return 0;
}

static int
ngedk_procfs_cleanup(void)
{
    remove_proc_entry(NGEDK_MODULE_NAME, NULL);

    return 0;
}

/*!
 * Generic module functions
 */

static int
ngedk_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int
ngedk_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long
ngedk_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ngedk_ioc_cmd_s ioc;
    struct edk_dev_s *ed;

    if (copy_from_user(&ioc, (void *)arg, sizeof(ioc))) {
        return -EFAULT;
    }

    ioc.rc = NGEDK_IOC_SUCCESS;

    switch (cmd) {
    case NGEDK_IOC_MOD_INFO:
        ioc.op.mod_info.version = NGEDK_IOC_VERSION;
        break;
    case NGEDK_IOC_ATTACH_INST:
        ed = &edkdevs[ioc.devid];
        if (ngedk_dmamem_alloc(ed, ioc.op.attach_inst.size_mb) < 0) {
            printk(KERN_WARNING "Unable to allocate DMA pool for EDK\n");
            ioc.rc = NGEDK_IOC_FAIL;
            break;
        }
        printk(KERN_INFO "ngedk: Attach unit %d hram %d\n",
                          ioc.devid, ioc.op.attach_inst.size_mb);
        break;
    case NGEDK_IOC_GET_DMA_INFO:
        ed = &edkdevs[ioc.devid];
        ioc.op.dma_info.vaddr = (uintptr_t)ed->dma_vaddr;
        ioc.op.dma_info.paddr = ed->dma_paddr;
        ioc.op.dma_info.baddr = ed->dma_baddr;
        ioc.op.dma_info.size = ed->dma_size;
        break;
    case NGEDK_IOC_INTR_WAIT:
        if (ngedk_intr_wait(ioc.devid, &ioc.op.edk_intr.sw_intr_cores) < 0) {
            ioc.rc = NGEDK_IOC_FAIL;
        }
        break;
    case NGEDK_IOC_INTR_ENABLE:
        ed = &edkdevs[ioc.devid];
        ed->edk_intr.enable = 1;
        break;
    case NGEDK_IOC_INTR_DISABLE:
        ed = &edkdevs[ioc.devid];
        ed->edk_intr.enable = 0;
        break;
    case NGEDK_IOC_INTR_SET:
        ed = &edkdevs[ioc.devid];

        /* Active uCs */
        ed->edk_intr.active_bmp = ioc.op.edk_intr.active_bmp;
        /* To check if a timer interrupt is asserted */
        ed->edk_intr.timer_intrc_stat_reg = ioc.op.edk_intr.timer_intrc_stat_reg;
        ed->edk_intr.timer_intrc_disable_reg = ioc.op.edk_intr.timer_intrc_disable_reg;
        ed->edk_intr.timer_intrc_mask_val = ioc.op.edk_intr.timer_intrc_mask_val;
        ed->edk_intr.flags = ioc.op.edk_intr.flags;
        break;
    case NGEDK_IOC_TIMER_INTR:
        ed = &edkdevs[ioc.devid];
        /* Two cores may use one interrupt. Leave it to the EDK Host to Identify */
        atomic_set(&ed->run_edk_thread, 1);
        wake_up_interruptible(&ed->edk_thread_wq);
        break;
    case NGEDK_IOC_SW_INTR:
        ed = &edkdevs[ioc.devid];
        set_bit(ioc.op.sw_intr.uc, &(ed->edk_intr.swi_intr_cores));
        atomic_set(&ed->run_edk_thread, 1);
        wake_up_interruptible(&ed->edk_thread_wq);
        break;
    default:
        printk(KERN_ERR "ngedk: invalid ioctl (%08x)\n", cmd);
        ioc.rc = NGEDK_IOC_FAIL;
        break;
    }

    if (copy_to_user((void *)arg, &ioc, sizeof(ioc))) {
        return -EFAULT;
    }

    return 0;
}

static int
ngedk_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long paddr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    struct edk_dev_s *edkdev;
    int kdev;
    int range_valid = 0;
    int map_noncached = 0;

    /* Check for valid range */
    for (kdev = 0; kdev < NGEDK_NUM_SWDEV_MAX; kdev++) {
        edkdev = &edkdevs[kdev];
        if (paddr >= edkdev->dma_paddr &&
            (paddr + size) <= (edkdev->dma_paddr + edkdev->dma_size)) {
            if (edkdev->dma_type == NGEDK_DMA_T_KAPI) {
                map_noncached = 1;
            }
            range_valid = 1;
            break;
        }
    }

    if (!range_valid) {
       printk("NGEDK: Invalid mmap range 0x%08lx/0x%lx\n", paddr, size);
       return -EINVAL;
    }

    if (map_noncached) {
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    }

    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                        size, vma->vm_page_prot)) {
        printk("EDK: Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
               paddr, paddr + size, vma->vm_start, vma->vm_end);
        return -EAGAIN;
    }

    return 0;
}

static struct file_operations ngedk_fops = {
    .open = ngedk_open,
    .release = ngedk_release,
    .unlocked_ioctl = ngedk_ioctl,
    .compat_ioctl = ngedk_ioctl,
    .mmap = ngedk_mmap,
};

static void __exit
ngedk_exit_module(void)
{
    ngedk_detach();
    ngedk_procfs_cleanup();
    unregister_chrdev(NGEDK_MODULE_MAJOR, NGEDK_MODULE_NAME);
    printk(KERN_INFO "Broadcom NGEDK unloaded successfully\n");
}

static int __init
ngedk_init_module(void)
{
    int rv;

    rv = register_chrdev(NGEDK_MODULE_MAJOR, NGEDK_MODULE_NAME, &ngedk_fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               NGEDK_MODULE_NAME, NGEDK_MODULE_MAJOR);
        return rv;
    }

    rv = ngedk_procfs_init();
    if (rv < 0) {
        printk(KERN_WARNING "%s: Unable to initialize proc files\n",
               NGEDK_MODULE_NAME);
        return rv;
    }

    ngedk_attach();

    printk(KERN_INFO "Broadcom NGEDK loaded successfully\n");
    return 0;
}

module_exit(ngedk_exit_module);
module_init(ngedk_init_module);
