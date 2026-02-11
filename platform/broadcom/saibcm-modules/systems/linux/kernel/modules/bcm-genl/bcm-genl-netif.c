/*
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

/*
 * Middle-driver for communication between Linux KNET driver and
 * drivers support Generic Netlink channel.
 *
 * This code is to handle the NET interfaces from the create and destroy
 * call-back functios of Linux KNET driver.
 */

#include <gmodule.h> /* Must be included first */
#include <kcom.h>
#include <bcm-knet.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include "bcm-genl-netif.h"

/* generic netlink interface info */
typedef struct {
    struct list_head netif_list;
    int netif_count;
    spinlock_t lock;
} genl_netif_info_t;
static genl_netif_info_t g_netif_info;

typedef struct {
    struct list_head list;
    bcmgenl_netif_t netif;
} genl_netif_t;

static uint32 g_sample_rate;
static uint32 g_sample_size;

static int
knet_netif_create_cb(struct net_device *dev, int dev_no, kcom_netif_t *netif)
{
    int found = 0;
    struct list_head *list_ptr;
    genl_netif_t *genl_netif, *lgenl_netif;
    bcmgenl_netif_t *bcmgenl_netif;
    unsigned long flags;

    if ((genl_netif = kmalloc(sizeof(genl_netif_t), GFP_ATOMIC)) == NULL) {
        gprintk("%s: failed to alloc genl-netif memory for netif '%s'\n",
                __func__, dev->name);
        return -1;
    }

    bcmgenl_netif = &genl_netif->netif;
    bcmgenl_netif->dev = dev;
    bcmgenl_netif->id = netif->id;
    bcmgenl_netif->port = netif->port;
    bcmgenl_netif->vlan = netif->vlan;
    bcmgenl_netif->qnum = netif->qnum;
    bcmgenl_netif->sample_rate = g_sample_rate;
    bcmgenl_netif->sample_size = g_sample_size;

    /* insert netif sorted by ID similar to bkn_knet_netif_create() */
    spin_lock_irqsave(&g_netif_info.lock, flags);

    list_for_each(list_ptr, &g_netif_info.netif_list) {
        lgenl_netif = list_entry(list_ptr, genl_netif_t, list);
        bcmgenl_netif = &lgenl_netif->netif;
        if (netif->id < bcmgenl_netif->id) {
            found = 1;
            break;
        }
    }
    if (found) {
        /* Replace previously removed interface */
        list_add_tail(&genl_netif->list, &lgenl_netif->list);
    } else {
        /* No holes - add to end of list */
        list_add_tail(&genl_netif->list, &g_netif_info.netif_list);
    }
    g_netif_info.netif_count++;

    spin_unlock_irqrestore(&g_netif_info.lock, flags);

    return 0;
}

static int
knet_netif_destroy_cb(struct net_device *dev, int dev_no, kcom_netif_t *netif)
{
    int found = 0;
    struct list_head *list_ptr, *list_next;
    genl_netif_t *genl_netif;
    bcmgenl_netif_t *bcmgenl_netif;
    unsigned long flags;

    if (!netif || !dev) {
        gprintk("%s: netif or net_device is NULL\n", __func__);
        return -1;
    }

    spin_lock_irqsave(&g_netif_info.lock, flags);
    list_for_each_safe(list_ptr, list_next, &g_netif_info.netif_list) {
        genl_netif = list_entry(list_ptr, genl_netif_t, list);
        bcmgenl_netif = &genl_netif->netif;
        if (netif->id == bcmgenl_netif->id) {
            found = 1;
            list_del(list_ptr);
            g_netif_info.netif_count--;
            break;
        }
    }
    spin_unlock_irqrestore(&g_netif_info.lock, flags);

    if (!found) {
        gprintk("%s: netif ID %d not found!\n", __func__, netif->id);
        return -1;
    }
    kfree(genl_netif);
    return 0;
}

int
bcmgenl_netif_search(char *dev_name,
                     bcmgenl_netif_search_f cb, void *cb_data)
{
    struct list_head *list_ptr;
    genl_netif_t *genl_netif;
    bcmgenl_netif_t *bcmgenl_netif;
    unsigned long flags;
    int rv;
    int cnt = 0;

    if (cb == NULL) {
        return -1;
    }

    spin_lock_irqsave(&g_netif_info.lock, flags);
    list_for_each(list_ptr, &g_netif_info.netif_list) {
        genl_netif = list_entry(list_ptr, genl_netif_t, list);
        bcmgenl_netif = &genl_netif->netif;
        if (dev_name && strcmp(bcmgenl_netif->dev->name, dev_name) != 0) {
            continue;
        }
        rv = cb(cb_data, bcmgenl_netif);
        if (rv < 0) {
            spin_unlock_irqrestore(&g_netif_info.lock, flags);
            return rv;
        }
        cnt++;
    }
    spin_unlock_irqrestore(&g_netif_info.lock, flags);

    return cnt;
}

int
bcmgenl_netif_num_get(void)
{
    int num = 0;
    unsigned long flags;

    spin_lock_irqsave(&g_netif_info.lock, flags);
    num = g_netif_info.netif_count;
    spin_unlock_irqrestore(&g_netif_info.lock, flags);

    return num;
}

int
bcmgenl_netif_get_by_ifindex(int ifindex, bcmgenl_netif_t *bcmgenl_netif)
{
    struct list_head *list_ptr;
    genl_netif_t *genl_netif;
    bcmgenl_netif_t *netif;
    unsigned long flags;

    if (bcmgenl_netif == NULL) {
        return -1;
    }

    /* look for port from list of available net_devices */
    spin_lock_irqsave(&g_netif_info.lock, flags);
    list_for_each(list_ptr, &g_netif_info.netif_list) {
        genl_netif = list_entry(list_ptr, genl_netif_t, list);
        netif = &genl_netif->netif;
        if (netif->dev->ifindex == ifindex) {
            memcpy(bcmgenl_netif, netif, sizeof(*bcmgenl_netif));
            spin_unlock_irqrestore(&g_netif_info.lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&g_netif_info.lock, flags);

    return -1;
}

int
bcmgenl_netif_get_by_port(int port, bcmgenl_netif_t *bcmgenl_netif)
{
    struct list_head *list_ptr;
    genl_netif_t *genl_netif;
    bcmgenl_netif_t *netif;
    unsigned long flags;

    if (bcmgenl_netif == NULL) {
        return -1;
    }

    /* look for port from list of available net_devices */
    spin_lock_irqsave(&g_netif_info.lock, flags);
    list_for_each(list_ptr, &g_netif_info.netif_list) {
        genl_netif = list_entry(list_ptr, genl_netif_t, list);
        netif = &genl_netif->netif;
        if (netif->port == port) {
            memcpy(bcmgenl_netif, netif, sizeof(bcmgenl_netif_t));
            spin_unlock_irqrestore(&g_netif_info.lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&g_netif_info.lock, flags);

    return -1;
}

int
bcmgenl_netif_default_sample_set(int sample_rate, int sample_size)
{
    if (sample_rate >= 0) {
        g_sample_rate = sample_rate;
    }
    if (sample_size >= 0) {
        g_sample_size = sample_size;
    }

    return 0;
}

int
bcmgenl_netif_init(void)
{
    memset(&g_netif_info, 0, sizeof(genl_netif_info_t));

    INIT_LIST_HEAD(&g_netif_info.netif_list);
    spin_lock_init(&g_netif_info.lock);

    bkn_netif_create_cb_register(knet_netif_create_cb);
    bkn_netif_destroy_cb_register(knet_netif_destroy_cb);

    return 0;
}

int
bcmgenl_netif_cleanup(void)
{
    genl_netif_t *genl_netif;

    bkn_netif_create_cb_unregister(knet_netif_create_cb);
    bkn_netif_destroy_cb_unregister(knet_netif_destroy_cb);

    while (!list_empty(&g_netif_info.netif_list)) {
        genl_netif = list_entry(g_netif_info.netif_list.next,
                                genl_netif_t, list);
        list_del(&genl_netif->list);
        kfree(genl_netif);
    }

    return 0;
}

