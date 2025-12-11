/*
 * $Id: lkm.h,v 1.22 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

#ifndef __COMMON_LINUX_KRN_LKM_H__
#define __COMMON_LINUX_KRN_LKM_H__

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/init.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
#error Kernel too old
#endif
#include <linux/kconfig.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#if defined(INCLUDE_KNET) && LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#ifdef CONFIG_NF_CONNTRACK_MODULE
#include <linux/netfilter.h>
#endif
#endif
#include <linux/slab.h>
#endif
#include <linux/module.h>

/* Helper defines for multi-version kernel  support */
#define LKM_2_6

#include <linux/kernel.h>   /* printk() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/signal.h>
#endif
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/hardirq.h>
#include <asm/uaccess.h>

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

#define PROC_INTERFACE_KERN_VER_3_10 (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))

/* Compatibility Macros */

#define LKM_MOD_PARAM(n,ot,nt,d) module_param(n,nt,d)
#define LKM_MOD_PARAM_ARRAY(n,ot,nt,c,d) module_param_array(n,nt,c,d)
#define LKM_EXPORT_SYM(s) EXPORT_SYMBOL(s)
#define _free_netdev free_netdev

#ifndef list_for_each_safe
#define list_for_each_safe(l,t,i) t = 0; list_for_each((l),(i))
#endif

#ifndef reparent_to_init
#define reparent_to_init()
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(str)
#endif

#ifndef EXPORT_NO_SYMBOLS
#define EXPORT_NO_SYMBOLS
#endif

#ifndef DEFINE_SPINLOCK
#define DEFINE_SPINLOCK(_lock) spinlock_t _lock = SPIN_LOCK_UNLOCKED
#endif

#ifndef __SPIN_LOCK_UNLOCKED
#define __SPIN_LOCK_UNLOCKED(_lock) SPIN_LOCK_UNLOCKED
#endif

#ifndef lock_kernel
#ifdef preempt_disable
#define lock_kernel() preempt_disable()
#else
#define lock_kernel()
#endif
#endif

#ifndef unlock_kernel
#ifdef preempt_enable
#define unlock_kernel() preempt_enable()
#else
#define unlock_kernel()
#endif
#endif

#ifndef init_MUTEX_LOCKED
#define init_MUTEX_LOCKED(_sem) sema_init(_sem, 0)
#endif

#ifdef CONFIG_BCM98245
#define CONFIG_BMW
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,8,0)
#define DMA_FORCE_CONTIGUOUS NULL
#else
#define DMA_FORCE_CONTIGUOUS DMA_ATTR_FORCE_CONTIGUOUS
#endif

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
#endif

#if PROC_INTERFACE_KERN_VER_3_10
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0)
#define strscpy strlcpy
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
#define kernel_ethtool_ts_info ethtool_ts_info
#endif

#endif /* __COMMON_LINUX_KRN_LKM_H__ */
