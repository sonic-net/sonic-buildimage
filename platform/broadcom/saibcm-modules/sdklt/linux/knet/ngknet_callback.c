/*! \file ngknet_callback.c
 *
 * Utility routines for NGKNET callbacks.
 *
 */
/*
 * $Copyright: Copyright 2018-2022 Broadcom. All rights reserved.
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
 * be found in the LICENSES folder.$
 */

#include "ngknet_callback.h"

static struct ngknet_callback_ctrl callback_ctrl;

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
    if (callback_ctrl.dev_init_cb != NULL) {
        return -1;
    }
    callback_ctrl.dev_init_cb = dev_init_cb;

    return 0;
}

int
ngknet_dev_init_cb_unregister(ngknet_dev_init_cb_f dev_init_cb)
{
    if (dev_init_cb == NULL || callback_ctrl.dev_init_cb != dev_init_cb) {
        return -1;
    }
    callback_ctrl.dev_init_cb = NULL;

    return 0;
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
    if (callback_ctrl.netif_create_cb != NULL) {
        return -1;
    }
    callback_ctrl.netif_create_cb = netif_cb;

    return 0;
}

int
ngknet_netif_create_cb_unregister(ngknet_netif_cb_f netif_cb)
{
    if (netif_cb == NULL || callback_ctrl.netif_create_cb != netif_cb) {
        return -1;
    }
    callback_ctrl.netif_create_cb = NULL;

    return 0;
}

int
ngknet_netif_destroy_cb_register(ngknet_netif_cb_f netif_cb)
{
    if (callback_ctrl.netif_destroy_cb != NULL) {
        return -1;
    }
    callback_ctrl.netif_destroy_cb = netif_cb;

    return 0;
}

int
ngknet_netif_destroy_cb_unregister(ngknet_netif_cb_f netif_cb)
{
    if (netif_cb == NULL || callback_ctrl.netif_destroy_cb != netif_cb) {
        return -1;
    }
    callback_ctrl.netif_destroy_cb = NULL;

    return 0;
}

int
ngknet_filter_cb_register(ngknet_filter_cb_f filter_cb)
{
    if (callback_ctrl.filter_cb != NULL) {
        return -1;
    }
    callback_ctrl.filter_cb = filter_cb;

    return 0;
}

int
ngknet_filter_cb_unregister(ngknet_filter_cb_f filter_cb)
{
    if (filter_cb == NULL || callback_ctrl.filter_cb != filter_cb) {
        return -1;
    }
    callback_ctrl.filter_cb = NULL;

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

