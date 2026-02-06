/*! \file lkm.h
 *
 * Linux compatibility macros.
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

#ifndef LKM_H
#define LKM_H

#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
#error Kernel too old
#endif
#include <linux/kconfig.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#include <linux/slab.h>
#endif
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fcntl.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

#include <asm/io.h>
#include <asm/hardirq.h>

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

/* Compatibility Macros */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
#define PROC_OWNER(_m)
#else
#define PROC_OWNER(_m) .owner = _m,
#define proc_ops file_operations
#define proc_open open
#define proc_read read
#define proc_write write
#define proc_lseek llseek
#define proc_release release
#define proc_ioctl unlocked_ioctl
#define proc_compat_ioctl compat_ioctl
#define proc_mmap mmap
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#define PROC_CREATE(_entry, _name, _acc, _path, _fops)                  \
    do {                                                                \
        _entry = proc_create(_name, _acc, _path, _fops);                \
    } while (0)

#define PROC_CREATE_DATA(_entry, _name, _acc, _path, _fops, _data)      \
    do {                                                                \
        _entry = proc_create_data(_name, _acc, _path, _fops, _data);    \
    } while (0)

#define PROC_PDE_DATA(_node) PDE_DATA(_node)
#else
#define PROC_CREATE(_entry, _name, _acc, _path, _fops)                  \
    do {                                                                \
        _entry = create_proc_entry(_name, _acc, _path);                 \
        if (_entry) {                                                   \
            _entry->proc_fops = _fops;                                  \
        }                                                               \
    } while (0)

#define PROC_CREATE_DATA(_entry, _name, _acc, _path, _fops, _data)      \
    do {                                                                \
        _entry = create_proc_entry(_name, _acc, _path);                 \
        if (_entry) {                                                   \
            _entry->proc_fops = _fops;                                  \
            _entry->data=_data;                                         \
        }                                                               \
    } while (0)

#define PROC_PDE_DATA(_node) PROC_I(_node)->pde->data
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)
#define timer_arg(var, context, timer_fieldname) \
    (typeof(var))(context)
#define timer_context_t unsigned long
#else
#define timer_context_t struct timer_list *
#define timer_arg(var, context, timer_fieldname) \
    from_timer(var, context, timer_fieldname)
#endif

#ifndef setup_timer
#define setup_timer(timer, fn, data) \
    timer_setup(timer, fn, 0)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
static inline void page_ref_inc(struct page *page)
{
    atomic_inc(&page->_count);
}

static inline void page_ref_dec(struct page *page)
{
    atomic_dec(&page->_count);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,8,0)
#define DMA_FORCE_CONTIGUOUS NULL
#else
#define DMA_FORCE_CONTIGUOUS DMA_ATTR_FORCE_CONTIGUOUS
#endif

#ifndef PCI_IRQ_MSI
/* Emulate new IRQ API if not available */
#define PCI_IRQ_INTX          (1 << 0)
#define PCI_IRQ_MSI     (1 << 1)
#define PCI_IRQ_MSIX    (1 << 2)
static inline int
pci_alloc_irq_vectors(struct pci_dev *dev, unsigned int min_vecs,
                      unsigned int max_vecs, unsigned int flags)
{
    /* We do not attempt to support MSI-X via old API */
    if (flags & PCI_IRQ_MSI) {
        if (pci_enable_msi(dev) == 0) {
            return 1;
        }
    }
    if (flags & PCI_IRQ_INTX) {
        return 1;
    }
    return 0;
}
static inline void
pci_free_irq_vectors(struct pci_dev *dev)
{
    pci_disable_msi(dev);
}
static inline int
pci_irq_vector(struct pci_dev *dev, unsigned int nr)
{
    return dev->irq;
}
#endif

/* Renamed in 6.8 */
#ifndef PCI_IRQ_INTX
#define PCI_IRQ_INTX    PCI_IRQ_LEGACY
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
#define strscpy strlcpy
#endif

#ifndef MAX_PAGE_ORDER
#define MAX_PAGE_ORDER  MAX_ORDER
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
#define kernel_ethtool_ts_info ethtool_ts_info
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
#define PLATFORM_DRIVER_REMOVE_RETURN_TYPE int
#define PLATFORM_DRIVER_REMOVE_RETURN_VALUE(_v) return (_v)
#endif

#ifndef PLATFORM_DRIVER_REMOVE_RETURN_TYPE
#define PLATFORM_DRIVER_REMOVE_RETURN_TYPE void
#endif

#ifndef PLATFORM_DRIVER_REMOVE_RETURN_VALUE
#define PLATFORM_DRIVER_REMOVE_RETURN_VALUE(_v)
#endif

#endif /* LKM_H */
