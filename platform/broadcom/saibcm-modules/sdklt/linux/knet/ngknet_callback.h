/*! \file ngknet_callback.h
 *
 * Data structure definitions for NGKNET callbacks.
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

#ifndef NGKNET_CALLBACK_H
#define NGKNET_CALLBACK_H

#include <lkm/ngknet_kapi.h>

/*!
 * \brief NGKNET callback control.
 */
struct ngknet_callback_ctrl {
    /*! Handle TX/RX callback initialization. */
    ngknet_dev_init_cb_f dev_init_cb;

    /*! Handle Rx packet */
    ngknet_rx_cb_f rx_cb;

    /*! Handle Tx packet */
    ngknet_tx_cb_f tx_cb;

    /*! Handle Netif creation */
    ngknet_netif_cb_f netif_create_cb;

    /*! Handle Netif destruction */
    ngknet_netif_cb_f netif_destroy_cb;

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
};

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

