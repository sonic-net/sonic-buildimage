/*! \file ngknet_callback.c
 *
 * Utility routines for NGKNET callbacks.
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

#include "ngknet_main.h"
#include "ngknet_callback.h"
#include "ngknet_extra.h"

static struct ngknet_callback_ctrl callback_ctrl;

void
ngknet_callback_init(struct ngknet_dev *devs)
{
    INIT_LIST_HEAD(&callback_ctrl.dev_init_cb_list);
    INIT_LIST_HEAD(&callback_ctrl.netif_create_cb_list);
    INIT_LIST_HEAD(&callback_ctrl.netif_destroy_cb_list);
    INIT_LIST_HEAD(&callback_ctrl.filter_cb_list);
    callback_ctrl.devs = devs;
}

void
ngknet_callback_cleanup(void)
{
    dev_cb_t *dev_cb;
    netif_cb_t *netif_cb;
    filter_cb_t *filter_cb;

    while (!list_empty(&callback_ctrl.dev_init_cb_list)) {
        dev_cb = list_entry(callback_ctrl.dev_init_cb_list.next,
                            dev_cb_t, list);
        list_del(&dev_cb->list);
        kfree(dev_cb);
    }
    while (!list_empty(&callback_ctrl.netif_create_cb_list)) {
        netif_cb = list_entry(callback_ctrl.netif_create_cb_list.next,
                              netif_cb_t, list);
        list_del(&netif_cb->list);
        kfree(netif_cb);
    }
    while (!list_empty(&callback_ctrl.netif_destroy_cb_list)) {
        netif_cb = list_entry(callback_ctrl.netif_destroy_cb_list.next,
                              netif_cb_t, list);
        list_del(&netif_cb->list);
        kfree(netif_cb);
    }
    while (!list_empty(&callback_ctrl.filter_cb_list)) {
        filter_cb = list_entry(callback_ctrl.filter_cb_list.next,
                               filter_cb_t, list);
        list_del(&filter_cb->list);
        kfree(filter_cb);
    }
}

int
ngknet_callback_control_get(struct ngknet_callback_ctrl **cbc)
{
    *cbc = &callback_ctrl;

    return 0;
}

/*!
 * Call-back interfaces for other Linux kernel drivers.
 */

int
ngknet_dev_init_cb_register(ngknet_dev_init_cb_f dev_init_cb)
{
    struct list_head *list;
    dev_cb_t *dev_cb;

    if (dev_init_cb == NULL) {
        return -1;
    }

    list_for_each(list, &callback_ctrl.dev_init_cb_list) {
        dev_cb = list_entry(list, dev_cb_t, list);
        if (dev_cb->cb == dev_init_cb) {
            return -1;
        }
    }

    dev_cb = kmalloc(sizeof(*dev_cb), GFP_KERNEL);
    if (dev_cb == NULL) {
        return -1;
    }
    dev_cb->cb = dev_init_cb;
    list_add_tail(&dev_cb->list, &callback_ctrl.dev_init_cb_list);

    return 0;
}

int
ngknet_dev_init_cb_unregister(ngknet_dev_init_cb_f dev_init_cb)
{
    struct list_head *list, *list_next;
    dev_cb_t *dev_cb;

    if (dev_init_cb == NULL) {
        return -1;
    }

    list_for_each_safe(list, list_next, &callback_ctrl.dev_init_cb_list) {
        dev_cb = list_entry(list, dev_cb_t, list);
        if (dev_cb->cb == dev_init_cb) {
            list_del(list);
            kfree(dev_cb);
            return 0;
        }
    }

    return -1;
}

int
ngknet_rx_cb_register(ngknet_rx_cb_f rx_cb)
{
    if (callback_ctrl.rx_cb != NULL) {
        return -1;
    }
    callback_ctrl.rx_cb = rx_cb;

    return 0;
}

int
ngknet_rx_cb_unregister(ngknet_rx_cb_f rx_cb)
{
    if (rx_cb == NULL || callback_ctrl.rx_cb != rx_cb) {
        return -1;
    }
    callback_ctrl.rx_cb = NULL;

    return 0;
}

int
ngknet_tx_cb_register(ngknet_tx_cb_f tx_cb)
{
    if (callback_ctrl.tx_cb != NULL) {
        return -1;
    }
    callback_ctrl.tx_cb = tx_cb;

    return 0;
}

int
ngknet_tx_cb_unregister(ngknet_tx_cb_f tx_cb)
{
    if (tx_cb == NULL || callback_ctrl.tx_cb != tx_cb) {
        return -1;
    }
    callback_ctrl.tx_cb = NULL;

    return 0;
}

int
ngknet_netif_create_cb_register(ngknet_netif_cb_f netif_cb)
{
    struct list_head *list;
    netif_cb_t *netif_create_cb;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each(list, &callback_ctrl.netif_create_cb_list) {
        netif_create_cb = list_entry(list, netif_cb_t, list);
        if (netif_create_cb->cb == netif_cb) {
            return -1;
        }
    }
    netif_create_cb = kmalloc(sizeof(*netif_create_cb), GFP_KERNEL);
    if (netif_create_cb == NULL) {
        return -1;
    }
    netif_create_cb->cb = netif_cb;
    list_add_tail(&netif_create_cb->list, &callback_ctrl.netif_create_cb_list);

    return 0;
}

int
ngknet_netif_create_cb_unregister(ngknet_netif_cb_f netif_cb)
{
    struct list_head *list, *list_next;
    netif_cb_t *netif_create_cb;
    int found = 0;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each_safe(list, list_next, &callback_ctrl.netif_create_cb_list) {
        netif_create_cb = list_entry(list, netif_cb_t, list);
        if (netif_create_cb->cb == netif_cb) {
            found = 1;
            list_del(list);
            break;
        }
    }
    if (!found) {
        return -1;
    }
    kfree(netif_create_cb);

    return 0;
}

int
ngknet_netif_destroy_cb_register(ngknet_netif_cb_f netif_cb)
{
    struct list_head *list;
    netif_cb_t *netif_destroy_cb;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each(list, &callback_ctrl.netif_destroy_cb_list) {
        netif_destroy_cb = list_entry(list, netif_cb_t, list);
        if (netif_destroy_cb->cb == netif_cb) {
            return -1;
        }
    }
    netif_destroy_cb = kmalloc(sizeof(*netif_destroy_cb), GFP_KERNEL);
    if (netif_destroy_cb == NULL) {
        return -1;
    }
    netif_destroy_cb->cb = netif_cb;
    list_add_tail(&netif_destroy_cb->list, &callback_ctrl.netif_destroy_cb_list);

    return 0;
}

int
ngknet_netif_destroy_cb_unregister(ngknet_netif_cb_f netif_cb)
{
    struct list_head *list, *list_next;
    netif_cb_t *netif_destroy_cb;
    int found = 0;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each_safe(list, list_next, &callback_ctrl.netif_destroy_cb_list) {
        netif_destroy_cb = list_entry(list, netif_cb_t, list);
        if (netif_destroy_cb->cb == netif_cb) {
            found = 1;
            list_del(list);
            break;
        }
    }
    if (!found) {
        return -1;
    }
    kfree(netif_destroy_cb);

    return 0;
}

int
ngknet_filter_cb_register(ngknet_filter_cb_f filter_cb)
{
    return ngknet_filter_cb_attr_register(filter_cb, NULL);
}

int
ngknet_filter_cb_register_by_name(ngknet_filter_cb_f filter_cb,
                                  const char *desc)
{
    ngknet_filter_cb_attr_t filter_cb_attr;

    memset(&filter_cb_attr, 0, sizeof(filter_cb_attr));
    filter_cb_attr.name = desc;
    return ngknet_filter_cb_attr_register(filter_cb, &filter_cb_attr);
}

int ngknet_filter_cb_attr_register(ngknet_filter_cb_f filter_cb,
                                   ngknet_filter_cb_attr_t *filter_cb_attr)
{
    struct ngknet_dev *dev;
    struct list_head *list;
    struct filt_ctrl *fc = NULL;
    filter_cb_t *fcb;
    unsigned long flags;
    int idx;
    const char *desc;

    if (filter_cb_attr == NULL || filter_cb_attr->name == NULL) {
        if (callback_ctrl.filter_cb != NULL) {
            return -1;
        }
        callback_ctrl.filter_cb = filter_cb;
        return 0;
    }

    desc = filter_cb_attr->name;
    if (desc[0] == '\0' || strlen(desc) >= NGKNET_FILTER_DESC_MAX) {
        return -1;
    }

    list_for_each(list, &callback_ctrl.filter_cb_list) {
        fcb = list_entry(list, filter_cb_t, list);
        if (strcmp(fcb->desc, desc) == 0) {
            return -1;
        }
    }
    fcb = kmalloc(sizeof(*fcb), GFP_KERNEL);
    if (fcb == NULL) {
        return -1;
    }
    fcb->cb = filter_cb;
    fcb->create_cb = filter_cb_attr->create_cb;
    fcb->destroy_cb = filter_cb_attr->destroy_cb;
    strscpy(fcb->desc, desc, sizeof(fcb->desc));
    list_add_tail(&fcb->list, &callback_ctrl.filter_cb_list);

    /* Check if any existing filter matches the registered name */
    for (idx = 0; idx < NUM_PDMA_DEV_MAX; idx++) {
        dev = &callback_ctrl.devs[idx];
        if (!(dev->flags & NGKNET_DEV_ACTIVE) ||
            list_empty(&dev->filt_list)) {
            continue;
        }
        spin_lock_irqsave(&dev->lock, flags);
        list_for_each(list, &dev->filt_list) {
            fc = (struct filt_ctrl *)list;
            if (fc &&
                fc->filt.dest_type == NGKNET_FILTER_DEST_T_CB &&
                fc->filt.desc[0] != '\0') {
                if (strcmp(fc->filt.desc, desc) == 0) {
                    fc->filter_cb = fcb->cb;
                    fc->create_cb = fcb->create_cb;
                    fc->destroy_cb = fcb->destroy_cb;
                }
            }
        }
        spin_unlock_irqrestore(&dev->lock, flags);
    }
    return 0;
}

int
ngknet_filter_cb_unregister(ngknet_filter_cb_f filter_cb)
{
    struct ngknet_dev *dev;
    struct list_head *list, *list2;
    struct filt_ctrl *fc = NULL;
    filter_cb_t *fcb;
    unsigned long flags;
    int found = 0, idx;

    if (filter_cb == NULL) {
        return -1;
    }

    /* Check if the any existing filter-specific callback matches */

    /* Remove from list */
    list_for_each_safe(list, list2, &callback_ctrl.filter_cb_list) {
        fcb = list_entry(list, filter_cb_t, list);
        if (fcb->cb == filter_cb) {
            found = 1;
            list_del(&fcb->list);
            kfree(fcb);
            break;
        }
    }
    /* Check if the callback is set to filters */
    if (found) {
        for (idx = 0; idx < NUM_PDMA_DEV_MAX; idx++) {
            dev = &callback_ctrl.devs[idx];
            if (!(dev->flags & NGKNET_DEV_ACTIVE) ||
                list_empty(&dev->filt_list)) {
                continue;
            }
            spin_lock_irqsave(&dev->lock, flags);
            list_for_each(list, &dev->filt_list) {
                fc = (struct filt_ctrl *)list;
                if (fc &&
                    fc->filt.dest_type == NGKNET_FILTER_DEST_T_CB &&
                    fc->filter_cb == filter_cb) {
                    fc->filter_cb = NULL;
                    fc->create_cb = NULL;
                    fc->destroy_cb = NULL;
                }
            }
            spin_unlock_irqrestore(&dev->lock, flags);
        }
    }

    if (!found && filter_cb != callback_ctrl.filter_cb) {
        return -1;
    }
    if (!found || filter_cb == callback_ctrl.filter_cb) {
        callback_ctrl.filter_cb = NULL;
    }
    return 0;
}

int
ngknet_ptp_rx_config_set_cb_register(ngknet_ptp_config_set_cb_f ptp_rx_config_set_cb)
{
    if (callback_ctrl.ptp_rx_config_set_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_rx_config_set_cb = ptp_rx_config_set_cb;

    return 0;
}

int
ngknet_ptp_rx_config_set_cb_unregister(ngknet_ptp_config_set_cb_f ptp_rx_config_set_cb)
{
    if (ptp_rx_config_set_cb == NULL ||
        callback_ctrl.ptp_rx_config_set_cb != ptp_rx_config_set_cb) {
        return -1;
    }
    callback_ctrl.ptp_rx_config_set_cb = NULL;

    return 0;
}

int
ngknet_ptp_tx_config_set_cb_register(ngknet_ptp_config_set_cb_f ptp_tx_config_set_cb)
{
    if (callback_ctrl.ptp_tx_config_set_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_tx_config_set_cb = ptp_tx_config_set_cb;

    return 0;
}

int
ngknet_ptp_tx_config_set_cb_unregister(ngknet_ptp_config_set_cb_f ptp_tx_config_set_cb)
{
    if (ptp_tx_config_set_cb == NULL ||
        callback_ctrl.ptp_tx_config_set_cb != ptp_tx_config_set_cb) {
        return -1;
    }
    callback_ctrl.ptp_tx_config_set_cb = NULL;

    return 0;
}

int
ngknet_ptp_rx_hwts_get_cb_register(ngknet_ptp_hwts_get_cb_f ptp_rx_hwts_get_cb)
{
    if (callback_ctrl.ptp_rx_hwts_get_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_rx_hwts_get_cb = ptp_rx_hwts_get_cb;

    return 0;
}

int
ngknet_ptp_rx_hwts_get_cb_unregister(ngknet_ptp_hwts_get_cb_f ptp_rx_hwts_get_cb)
{
    if (ptp_rx_hwts_get_cb == NULL ||
        callback_ctrl.ptp_rx_hwts_get_cb != ptp_rx_hwts_get_cb) {
        return -1;
    }
    callback_ctrl.ptp_rx_hwts_get_cb = NULL;

    return 0;
}

int
ngknet_ptp_tx_hwts_get_cb_register(ngknet_ptp_hwts_get_cb_f ptp_tx_hwts_get_cb)
{
    if (callback_ctrl.ptp_tx_hwts_get_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_tx_hwts_get_cb = ptp_tx_hwts_get_cb;

    return 0;
}

int
ngknet_ptp_tx_hwts_get_cb_unregister(ngknet_ptp_hwts_get_cb_f ptp_tx_hwts_get_cb)
{
    if (ptp_tx_hwts_get_cb == NULL ||
        callback_ctrl.ptp_tx_hwts_get_cb != ptp_tx_hwts_get_cb) {
        return -1;
    }
    callback_ctrl.ptp_tx_hwts_get_cb = NULL;

    return 0;
}

int
ngknet_ptp_tx_meta_set_cb_register(ngknet_ptp_meta_set_cb_f ptp_tx_meta_set_cb)
{
    if (callback_ctrl.ptp_tx_meta_set_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_tx_meta_set_cb = ptp_tx_meta_set_cb;

    return 0;
}

int
ngknet_ptp_tx_meta_set_cb_unregister(ngknet_ptp_meta_set_cb_f ptp_tx_meta_set_cb)
{
    if (ptp_tx_meta_set_cb == NULL ||
        callback_ctrl.ptp_tx_meta_set_cb != ptp_tx_meta_set_cb) {
        return -1;
    }
    callback_ctrl.ptp_tx_meta_set_cb = NULL;

    return 0;
}

int
ngknet_ptp_phc_index_get_cb_register(ngknet_ptp_phc_index_get_cb_f ptp_phc_index_get_cb)
{
    if (callback_ctrl.ptp_phc_index_get_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_phc_index_get_cb = ptp_phc_index_get_cb;

    return 0;
}

int
ngknet_ptp_phc_index_get_cb_unregister(ngknet_ptp_phc_index_get_cb_f ptp_phc_index_get_cb)
{
    if (ptp_phc_index_get_cb == NULL ||
        callback_ctrl.ptp_phc_index_get_cb != ptp_phc_index_get_cb) {
        return -1;
    }
    callback_ctrl.ptp_phc_index_get_cb = NULL;

    return 0;
}

int
ngknet_ptp_dev_ctrl_cb_register(ngknet_ptp_dev_ctrl_cb_f ptp_dev_ctrl_cb)
{
    if (callback_ctrl.ptp_dev_ctrl_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_dev_ctrl_cb = ptp_dev_ctrl_cb;

    return 0;
}

int
ngknet_ptp_dev_ctrl_cb_unregister(ngknet_ptp_dev_ctrl_cb_f ptp_dev_ctrl_cb)
{
    if (ptp_dev_ctrl_cb == NULL ||
        callback_ctrl.ptp_dev_ctrl_cb != ptp_dev_ctrl_cb) {
        return -1;
    }
    callback_ctrl.ptp_dev_ctrl_cb = NULL;

    return 0;
}

int
ngknet_ptp_rx_pre_process_cb_register(ngknet_ptp_rx_pre_process_cb_f ptp_rx_pre_process_cb)
{
    if (callback_ctrl.ptp_rx_pre_process_cb != NULL) {
        return -1;
    }
    callback_ctrl.ptp_rx_pre_process_cb = ptp_rx_pre_process_cb;

    return 0;
}

int
ngknet_ptp_rx_pre_process_cb_unregister(ngknet_ptp_rx_pre_process_cb_f ptp_rx_pre_process_cb)
{
    if (ptp_rx_pre_process_cb == NULL ||
        callback_ctrl.ptp_rx_pre_process_cb != ptp_rx_pre_process_cb) {
        return -1;
    }
    callback_ctrl.ptp_rx_pre_process_cb = NULL;

    return 0;
}

EXPORT_SYMBOL(ngknet_dev_init_cb_register);
EXPORT_SYMBOL(ngknet_dev_init_cb_unregister);
EXPORT_SYMBOL(ngknet_rx_cb_register);
EXPORT_SYMBOL(ngknet_rx_cb_unregister);
EXPORT_SYMBOL(ngknet_tx_cb_register);
EXPORT_SYMBOL(ngknet_tx_cb_unregister);
EXPORT_SYMBOL(ngknet_netif_create_cb_register);
EXPORT_SYMBOL(ngknet_netif_create_cb_unregister);
EXPORT_SYMBOL(ngknet_netif_destroy_cb_register);
EXPORT_SYMBOL(ngknet_netif_destroy_cb_unregister);
EXPORT_SYMBOL(ngknet_filter_cb_register);
EXPORT_SYMBOL(ngknet_filter_cb_register_by_name);
EXPORT_SYMBOL(ngknet_filter_cb_attr_register);
EXPORT_SYMBOL(ngknet_filter_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_rx_config_set_cb_register);
EXPORT_SYMBOL(ngknet_ptp_rx_config_set_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_tx_config_set_cb_register);
EXPORT_SYMBOL(ngknet_ptp_tx_config_set_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_rx_hwts_get_cb_register);
EXPORT_SYMBOL(ngknet_ptp_rx_hwts_get_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_tx_hwts_get_cb_register);
EXPORT_SYMBOL(ngknet_ptp_tx_hwts_get_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_tx_meta_set_cb_register);
EXPORT_SYMBOL(ngknet_ptp_tx_meta_set_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_phc_index_get_cb_register);
EXPORT_SYMBOL(ngknet_ptp_phc_index_get_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_dev_ctrl_cb_register);
EXPORT_SYMBOL(ngknet_ptp_dev_ctrl_cb_unregister);
EXPORT_SYMBOL(ngknet_ptp_rx_pre_process_cb_register);
EXPORT_SYMBOL(ngknet_ptp_rx_pre_process_cb_unregister);

