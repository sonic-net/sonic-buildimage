/*! \file ngknet_linux.h
 *
 * Data structure and macro definitions for Linux kernel APIs abstraction.
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

#ifndef NGKNET_LINUX_H
#define NGKNET_LINUX_H

#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>

/*!
 * Kernel abstraction
 */

#define MODULE_PARAM(n, t, p)   module_param(n, t, p)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
#define NGKNET_XDP_NATIVE
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,19,0))
#define kal_netif_napi_add(_dev, _napi, _poll, _weight) \
            netif_napi_add(_dev, _napi, _poll, _weight)
#else
#define kal_netif_napi_add(_dev, _napi, _poll, _weight) \
            netif_napi_add_weight(_dev, _napi, _poll, _weight)
#endif

/*
 * The eth_hw_addr_set was added in Linux 5.15, but later backported
 * to various longterm releases, so we need a more advanced check with
 * the option to override the default.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0) && \
     LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,188))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0) && \
     LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,251))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0) &&     \
     LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,291))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#ifndef KERNEL_HAS_ETH_HW_ADDR_SET
#define KERNEL_HAS_ETH_HW_ADDR_SET 0
#endif

#if (KERNEL_HAS_ETH_HW_ADDR_SET == 0)
static inline void
eth_hw_addr_set(struct net_device *dev, const u8 *addr)
{
    memcpy(dev->dev_addr, addr, ETH_ALEN);
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
#define NGKNET_ETHTOOL_LINK_SETTINGS 1
#else
#define NGKNET_ETHTOOL_LINK_SETTINGS 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define kal_vlan_hwaccel_put_tag(skb, proto, tci) \
    __vlan_hwaccel_put_tag(skb, tci)
#define NETIF_F_HW_VLAN_CTAG_RX NETIF_F_HW_VLAN_RX
#define NETIF_F_HW_VLAN_CTAG_TX NETIF_F_HW_VLAN_TX
#else
#define kal_vlan_hwaccel_put_tag(skb, proto, tci) \
    __vlan_hwaccel_put_tag(skb, htons(proto), tci)
#endif /* KERNEL_VERSION(3,10,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
static inline struct page *
kal_dev_alloc_pages(unsigned int order)
{
    return alloc_pages(GFP_ATOMIC | __GFP_ZERO | __GFP_COLD |
                       __GFP_COMP | __GFP_MEMALLOC, order);
}
#else
static inline struct page *
kal_dev_alloc_pages(unsigned int order)
{
    return dev_alloc_pages(order);
}
#endif /* KERNEL_VERSION(3,19,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0)
static inline struct sk_buff *
kal_build_skb(void *data, unsigned int frag_size)
{
    return NULL;
}
#else
static inline struct sk_buff *
kal_build_skb(void *data, unsigned int frag_size)
{
    return build_skb(data, frag_size);
}
#endif /* KERNEL_VERSION(3,5,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,2,0)
static inline bool
kal_page_is_pfmemalloc(struct page *page)
{
    return false;
}
#else
static inline bool
kal_page_is_pfmemalloc(struct page *page)
{
    return page_is_pfmemalloc(page);
}
#endif /* KERNEL_VERSION(4,2,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)
static inline void
kal_netif_trans_update(struct net_device *dev)
{
    dev->trans_start = jiffies;
}
#else
static inline void
kal_netif_trans_update(struct net_device *dev)
{
    netif_trans_update(dev);
}
#endif /* KERNEL_VERSION(4,7,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
static inline dma_addr_t
kal_dma_map_page_attrs(struct device *dev, struct page *page,
                       size_t offset, size_t size, enum dma_data_direction dir,
                       unsigned long attrs)
{
    return dma_map_page(dev, page, offset, size, dir);
}
static inline void
kal_dma_unmap_page_attrs(struct device *dev, dma_addr_t addr,
                         size_t size, enum dma_data_direction dir,
                         unsigned long attrs)
{
    dma_unmap_page(dev, addr, size, dir);
}
#else
static inline dma_addr_t
kal_dma_map_page_attrs(struct device *dev, struct page *page,
                       size_t offset, size_t size, enum dma_data_direction dir,
                       unsigned long attrs)
{
    return dma_map_page_attrs(dev, page, offset, size, dir, attrs);
}
static inline void
kal_dma_unmap_page_attrs(struct device *dev, dma_addr_t addr,
                         size_t size, enum dma_data_direction dir,
                         unsigned long attrs)
{
    dma_unmap_page_attrs(dev, addr, size, dir, attrs);
}
#endif /* KERNEL_VERSION(4,10,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
static inline s64
kal_time_usecs(void)
{
    struct timeval tv;
    do_gettimeofday(&tv);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
#else
static inline s64
kal_time_usecs(void)
{
    struct timespec64 ts;
    ktime_get_real_ts64(&ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
#endif /* KERNEL_VERSION(3,17,0) */

static inline unsigned long
kal_copy_from_user(void *to, const void __user *from,
                   unsigned int dl, unsigned int sl)
{
    unsigned int len = dl;

    if (unlikely(len != sl)) {
        printk(KERN_WARNING "Unmatched linux_ngknet.ko, please use the latest.\n");
        len = min(dl, sl);
    }

    return copy_from_user(to, from, len);
}

static inline unsigned long
kal_copy_to_user(void __user *to, const void *from,
                 unsigned int dl, unsigned int sl)
{
    unsigned int len = dl;

    if (unlikely(len != sl)) {
        printk(KERN_WARNING "Unmatched linux_ngknet.ko, please use the latest.\n");
        len = min(dl, sl);
    }

    return copy_to_user(to, from, len);
}

/*!
 * Atomic bit operations
 */

static inline void
at_set_bit(int nr, void *addr, void *lock)
{
    set_bit(nr, addr);
}

static inline void
at_clear_bit(int nr, void *addr, void *lock)
{
    clear_bit(nr, addr);
}

static inline int
at_test_set_bit(int nr, void *addr, void *lock)
{
    return test_and_set_bit(nr, addr);
}

static inline int
at_test_clear_bit(int nr, void *addr, void *lock)
{
    return test_and_clear_bit(nr, addr);
}

/*!
 * System abstraction
 */

static inline void *
sal_alloc(unsigned int sz, char *s)
{
    return kmalloc(sz, GFP_KERNEL);
}

static inline void
sal_free(void *addr)
{
    kfree(addr);
}

static inline void *
sal_memset(void *dest, int c, size_t cnt)
{
    return memset(dest, c, cnt);
}

static inline void *
sal_memcpy(void *dest, const void *src, size_t cnt)
{
    return memcpy(dest, src, cnt);
}

static inline char *
sal_strncpy(char *dest, const char *src, size_t cnt)
{
    return strncpy(dest, src, cnt);
}

/*!
 * Time
 */

extern unsigned long
sal_time_usecs(void);

extern void
sal_usleep(unsigned long usec);

/*!
 * Synchronization
 */

typedef struct sal_sem_s {
    char semaphore_opaque_type;
} *sal_sem_t;

typedef struct sal_spinlock_s {
    char spinlock_opaque_type;
} *sal_spinlock_t;

#define SAL_SEM_FOREVER         -1
#define SAL_SEM_BINARY          1
#define SAL_SEM_COUNTING        0

extern sal_sem_t
sal_sem_create(char *desc, int binary, int count);

extern void
sal_sem_destroy(sal_sem_t sem);

extern int
sal_sem_take(sal_sem_t sem, int usec);

extern int
sal_sem_give(sal_sem_t sem);

extern sal_spinlock_t
sal_spinlock_create(char *desc);

extern void
sal_spinlock_destroy(sal_spinlock_t lock);

extern int
sal_spinlock_lock(sal_spinlock_t lock);

extern int
sal_spinlock_unlock(sal_spinlock_t lock);

#endif /* NGKNET_LINUX_H */

