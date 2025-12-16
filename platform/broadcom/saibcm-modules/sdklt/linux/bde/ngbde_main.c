/*! \file ngbde_main.c
 *
 * NGBDE module entry.
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
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("NG BDE Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! \cond */
static int mmap_debug = 0;
module_param(mmap_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(mmap_debug,
"MMAP debug output enable (default 0).");
/*! \endcond */

/*!
 * \brief Remap user space DMA memory to non-cached area.
 *
 * Since we cannot flush and invalidate DMA memory from user space,
 * the DMA memory pools need to be cache-coherent, even if this means
 * that we need to remap the DMA memory as non-cached.
 *
 * If undefined, we set this value according to kernel configuration.
 */
#ifndef REMAP_DMA_NONCACHED
#  ifdef CONFIG_DMA_NONCOHERENT
#    define REMAP_DMA_NONCACHED 1
#  else
#    define REMAP_DMA_NONCACHED 0
#  endif
#endif

static int
ngbde_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int
ngbde_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*!
 * \brief Check if memory range is within existing DMA memory pools.
 *
 * \param [in] paddr Physical start address of memory range.
 * \param [in] size Size of memory range.
 *
 * \retval true Range is valid.
 * \retval false Range is not valid.
 */
static bool
ngbde_dma_range_valid(unsigned long paddr, unsigned long size)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx;
    struct ngbde_dmamem_s *dmamem;
    unsigned int pool;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        for (pool = 0; pool < NGBDE_NUM_DMAPOOL_MAX; pool++) {
            dmamem = &swdev[idx].dmapool[pool].dmamem;
            if (paddr >= dmamem->paddr &&
                (paddr + size) <= (dmamem->paddr + dmamem->size)) {
                return true;
            }
        }
    }
    return false;
}

/*!
 * \brief Check if memory range is within device I/O ranges.
 *
 * \param [in] paddr Physical start address of I/O memory range.
 * \param [in] size Size of memory range.
 *
 * \retval true Range is valid.
 * \retval false Range is not valid.
 */
static bool
ngbde_pio_range_valid(unsigned long paddr, unsigned long size)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx;
    struct ngbde_memwin_s *iowin;
    unsigned long iowin_size;
    unsigned int wdx;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        for (wdx = 0; wdx < NGBDE_NUM_IOWIN_MAX; wdx++) {
            iowin = &swdev[idx].iowin[wdx];
            iowin_size = iowin->size;
            if (iowin_size < PAGE_SIZE) {
                iowin_size = PAGE_SIZE;
            }
            if (mmap_debug) {
                printk("MMAP: Check 0x%08lx/0x%08lx against "
                       "0x%08lx/0x%08lx(0x%08lx)\n",
                       paddr, size,
                       (unsigned long)iowin->addr,
                       (unsigned long)iowin->size, iowin_size);
            }
            if (paddr >= iowin->addr &&
                (paddr + size) <= (iowin->addr + iowin_size)) {
                return true;
            }
        }
    }
    return false;
}

/*!
 * \brief Match incomplete address with device base addresses.
 *
 * Use for physical addresses larger than 44 bits.
 *
 * \param [in] paddr Physical address from user space.
 *
 * \return Matched device base addess or 0 if no match.
 */
static unsigned long
ngbde_pio_base_match(unsigned long paddr)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx;
    struct ngbde_memwin_s *iowin;
    unsigned int wdx;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        for (wdx = 0; wdx < NGBDE_NUM_IOWIN_MAX; wdx++) {
            iowin = &swdev[idx].iowin[wdx];
            if (((paddr ^ iowin->addr) & 0xfffffffffffULL) == 0) {
                if (mmap_debug) {
                    printk("MMAP: Matched 0x%08lx to 0x%08lx\n",
                           (unsigned long)paddr,
                           (unsigned long)iowin->addr);
                }
                return iowin->addr;
            }
        }
    }
    return 0;
}

/*
 * Some kernels are configured to prevent mapping of kernel RAM memory
 * into user space via the /dev/mem device.
 *
 * The function below provides a backdoor to mapping the DMA pool to
 * user space via the BDE device file.
 */
static const struct vm_operations_struct ngbde_vma_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
    .access = generic_access_phys,
#endif
};

static int
ngbde_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long paddr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    int map_noncached = REMAP_DMA_NONCACHED;
    int range_valid = 0;

    if (mmap_debug) {
        printk("MMAP: Mapping %lu Kbytes at 0x%08lx (0x%lx)\n",
               size / 1024, paddr, vma->vm_pgoff);
    }

    if (ngbde_dma_range_valid(paddr, size)) {
        range_valid = 1;
    } else {
        map_noncached = 1;
        if (ngbde_pio_range_valid(paddr, size)) {
            range_valid = 1;
        } else {
            paddr = ngbde_pio_base_match(paddr);
            if (ngbde_pio_range_valid(paddr, size)) {
                range_valid = 1;
            }
        }
    }

    if (!range_valid) {
        printk("BDE: Invalid mmap range 0x%08lx/0x%lx\n", paddr, size);
        return -EINVAL;
    }

    /* Support debug access to the mapping (works for PGMEM DMA only) */
    vma->vm_ops = &ngbde_vma_ops;

    if (map_noncached) {
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    }

    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                        size, vma->vm_page_prot)) {
        printk("BDE: Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
               paddr, paddr + size, vma->vm_start, vma->vm_end);
        return -EAGAIN;
    }

    return 0;
}

static struct file_operations fops = {
    .open = ngbde_open,
    .release = ngbde_release,
    .unlocked_ioctl = ngbde_ioctl,
    .compat_ioctl = ngbde_ioctl,
    .mmap = ngbde_mmap,
};

/*!
 * \brief Standard module cleanup.
 *
 * \return Nothing.
 */
static void __exit
ngbde_exit_module(void)
{
    ngbde_intr_cleanup();
    ngbde_iio_cleanup();
    ngbde_paxb_cleanup();
    ngbde_pio_cleanup();
    ngbde_dma_cleanup();
    ngbde_procfs_cleanup();
    unregister_chrdev(MOD_MAJOR, MOD_NAME);
    ngbde_pci_cleanup();
    ngbde_iproc_cleanup();
    printk(KERN_INFO "Broadcom NGBDE unloaded successfully\n");
}

/*!
 * \brief Standard module initialization.
 *
 * \return Nothing.
 */
static int __init
ngbde_init_module(void)
{
    int rv;

    rv = register_chrdev(MOD_MAJOR, MOD_NAME, &fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               MOD_NAME, MOD_MAJOR);
        return rv;
    }

    rv = ngbde_iproc_probe();
    if (rv < 0) {
        printk(KERN_WARNING "%s: Error probing for AXI bus devices.\n",
               MOD_NAME);
        return rv;
    }

    rv = ngbde_pci_probe();
    if (rv < 0) {
        printk(KERN_WARNING "%s: Error probing for PCI bus devices.\n",
               MOD_NAME);
        return rv;
    }

    rv = ngbde_procfs_init();
    if (rv < 0) {
        printk(KERN_WARNING "%s: Unable to initialize proc files\n",
               MOD_NAME);
        return rv;
    }

    printk(KERN_INFO "Broadcom NGBDE loaded successfully\n");
    return 0;
}

module_exit(ngbde_exit_module);
module_init(ngbde_init_module);
