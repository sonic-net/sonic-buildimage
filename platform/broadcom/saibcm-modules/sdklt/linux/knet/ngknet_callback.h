/*! \file ngknet_callback.h
 *
 * Data structure definitions for NGKNET callbacks.
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

#ifndef NGKNET_CALLBACK_H
#define NGKNET_CALLBACK_H

#include <lkm/ngknet_kapi.h>

typedef struct dev_cb_s {
    /*! List head */
    struct list_head list;

    /*! Device callback */
    ngknet_dev_init_cb_f cb;
} dev_cb_t;

typedef struct netif_cb_s {
    /*! List head */
    struct list_head list;

    /*! Handle Netif creation or destruction */
    ngknet_netif_cb_f cb;
} netif_cb_t;

typedef struct filter_cb_s {
    /*! List head */
    struct list_head list;

    /*! Filter description */
    char desc[NGKNET_FILTER_DESC_MAX];

    /*! Handle Filter callback */
    ngknet_filter_cb_f cb;

    /*! Handle Filter create callback */
    ngknet_filter_create_cb_f create_cb;

    /*! Handle Filter destroy callback */
    ngknet_filter_destroy_cb_f destroy_cb;
} filter_cb_t;

/*!
 * \brief NGKNET callback control.
 */
struct ngknet_callback_ctrl {
    /*! Device initialization callback list */
    struct list_head dev_init_cb_list;

    /*! Handle Rx packet */
    ngknet_rx_cb_f rx_cb;

    /*! Handle Tx packet */
    ngknet_tx_cb_f tx_cb;

    /*! Netif creation list */
    struct list_head netif_create_cb_list;

    /*! Netif destruction list */
    struct list_head netif_destroy_cb_list;

    /*! Filter callback list */
    struct list_head filter_cb_list;

    /*! Handle filter callback */
    ngknet_filter_cb_f filter_cb;

    /*! PTP Rx config set */
    ngknet_ptp_config_set_cb_f ptp_rx_config_set_cb;

    /*! PTP Tx config set */
    ngknet_ptp_config_set_cb_f ptp_tx_config_set_cb;

    /*! PTP Rx HW timestamp get */
    ngknet_ptp_hwts_get_cb_f ptp_rx_hwts_get_cb;

    /*! PTP Tx HW timestamp get */
    ngknet_ptp_hwts_get_cb_f ptp_tx_hwts_get_cb;

    /*! PTP Tx meta set */
    ngknet_ptp_meta_set_cb_f ptp_tx_meta_set_cb;

    /*! PTP PHC index get */
    ngknet_ptp_phc_index_get_cb_f ptp_phc_index_get_cb;

    /*! PTP device control */
    ngknet_ptp_dev_ctrl_cb_f ptp_dev_ctrl_cb;

    /*! PTP Rx pre processing */
    ngknet_ptp_rx_pre_process_cb_f ptp_rx_pre_process_cb;

    /*! Devices */
    struct ngknet_dev *devs;
};

/*!
 * \brief Initialize callback control.
 *
 * \param [in] devs Devices array.
 */
extern void
ngknet_callback_init(struct ngknet_dev *devs);

/*!
 * \brief Cleanup callback control.
 *
 */
extern void
ngknet_callback_cleanup(void);

/*!
 * \brief Get callback control.
 *
 * \param [in] cbc Pointer to callback control.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_callback_control_get(struct ngknet_callback_ctrl **cbc);

#endif /* NGKNET_CALLBACK_H */
