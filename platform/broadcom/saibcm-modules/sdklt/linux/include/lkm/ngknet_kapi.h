/*! \file ngknet_kapi.h
 *
 * NGKNET kernel API.
 *
 * This file is intended for use by other kernel modules relying on the KNET.
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

#ifndef NGKNET_KAPI_H
#define NGKNET_KAPI_H

#include <linux/skbuff.h>
#include <lkm/ngknet_dev.h>

/*!
 * \brief NGKNET callback descriptor.
 *
 * The NGKNET module provides several callback functions registration for
 * advanced features support. For callback function that carries packet
 * data (normally with SKB as callback function paramenter), the NGKNET
 * callback descriptor can be passed to callback function via SKB and
 * be accessed via the macro \ref NGKNET_SKB_CB.
 */
struct ngknet_callback_desc {
    /* Device information. */
    ngknet_dev_info_t *dinfo;

    /*! Network interface. */
    ngknet_netif_t *netif;

    /*! Matched filter. */
    ngknet_filter_t *filt;

    /*! Packet meta data. */
    uint8_t *pmd;

    /*! Packet meta data length. */
    int pmd_len;

    /*! Packet data length. */
    int pkt_len;

    /*! Network device. */
    struct net_device *net_dev;
};

/*! SKB callback data */
#define NGKNET_SKB_CB(_skb) ((struct ngknet_callback_desc *)_skb->cb)

/*!
 * PHC specific private data.
 */
struct ngknet_ptp_data {
    /*! Physical port. */
    int phy_port;

    /*! HW timestamp Tx type. */
    int hwts_tx_type;
};

/*! TX/RX callback init. */
typedef void
(*ngknet_dev_init_cb_f)(ngknet_dev_info_t *dinfo);

/*! Handle Rx packet. */
typedef struct sk_buff *
(*ngknet_rx_cb_f)(struct sk_buff *skb);

/*! Handle Tx packet. */
typedef struct sk_buff *
(*ngknet_tx_cb_f)(struct sk_buff *skb);

/*! Handle Netif callback. */
typedef int
(*ngknet_netif_cb_f)(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif);

/*! Handle Filter callback. */
typedef struct sk_buff *
(*ngknet_filter_cb_f)(struct sk_buff *skb, ngknet_filter_t **filt);

/*! Handle Filter create callback. */
typedef int
(*ngknet_filter_create_cb_f)(ngknet_filter_t *filt);

/*! Handle Filter destroy callback. */
typedef int
(*ngknet_filter_destroy_cb_f)(ngknet_filter_t *filt);

/*! Additional attribute associated with the filter callback. */
typedef struct {
    /*! Filter description to be matched. */
    const char *name;

    /*! Callback when the matched filter is created. */
    ngknet_filter_create_cb_f create_cb;

    /*! Callback when the matched filter is destroyed. */
    ngknet_filter_destroy_cb_f destroy_cb;
} ngknet_filter_cb_attr_t;

/*! PTP Rx/Tx config set. */
typedef int
(*ngknet_ptp_config_set_cb_f)(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif, int *value);

/*! PTP Rx/Tx HW timestamp get. */
typedef int
(*ngknet_ptp_hwts_get_cb_f)(struct sk_buff *skb, uint64_t *ts);

/*! PTP Tx meta set. */
typedef int
(*ngknet_ptp_meta_set_cb_f)(struct sk_buff *skb);

/*! PTP PHC index get */
typedef int
(*ngknet_ptp_phc_index_get_cb_f)(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif, int *index);

/*! PTP device control */
typedef int
(*ngknet_ptp_dev_ctrl_cb_f)(ngknet_dev_info_t *dinfo, int cmd, char *data, int len);

/*! PTP RX Preprocessing. */
typedef int
(*ngknet_ptp_rx_pre_process_cb_f)(struct sk_buff *skb, uint32_t *cust_hdr_len);

/*!
 * \brief Register TX/RX callback device initialization callback function.
 *
 * The device initialization callback allows an external module to
 * perform device-specific initialization in preparation for Tx and Rx
 * packet processing.
 *
 * \param [in] dev_init_cb TX/RX callback device initialization callback
 *        function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_dev_init_cb_register(ngknet_dev_init_cb_f dev_init_cb);

/*!
 * \brief Unegister TX/RX callback device initialization callback function.
 *
 * The device initialization callback allows an external module to
 * perform device-specific initialization in preparation for Tx and Rx
 * packet processing.
 *
 * \param [in] dev_init_cb TX/RX callback device initialization callback
 *        function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_dev_init_cb_unregister(ngknet_dev_init_cb_f dev_init_cb);

/*!
 * \brief Register Rx callback.
 *
 * \param [in] rx_cb Rx callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_rx_cb_register(ngknet_rx_cb_f rx_cb);

/*!
 * \brief Unregister Rx callback.
 *
 * \param [in] rx_cb Rx callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_rx_cb_unregister(ngknet_rx_cb_f rx_cb);

/*!
 * \brief Register Tx callback.
 *
 * \param [in] tx_cb Tx callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_tx_cb_register(ngknet_tx_cb_f tx_cb);

/*!
 * \brief Unregister Tx callback.
 *
 * \param [in] tx_cb Tx callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_tx_cb_unregister(ngknet_tx_cb_f tx_cb);

/*!
 * \brief Register callback for network interface creation.
 *
 * Register a function to be called whenever a virtual network interface
 * is created in the KNET kernel module.
 *
 * \param [in] netif_cb Callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_netif_create_cb_register(ngknet_netif_cb_f netif_cb);

/*!
 * \brief Unregister callback for network interface creation.
 *
 * \param [in] netif_cb Callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_netif_create_cb_unregister(ngknet_netif_cb_f netif_cb);

/*!
 * \brief Register callback for network interface destruction.
 *
 * Register a function to be called whenever a virtual network interface
 * is destroyed in the KNET kernel module.
 *
 * \param [in] netif_cb Callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_netif_destroy_cb_register(ngknet_netif_cb_f netif_cb);

/*!
 * \brief Unregister callback for network interface destruction.
 *
 * \param [in] netif_cb Callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_netif_destroy_cb_unregister(ngknet_netif_cb_f netif_cb);

/*!
 * \brief Register filter callback.
 *
 * \param [in] filter_cb Filter callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_filter_cb_register(ngknet_filter_cb_f filter_cb);

/*!
 * \brief Register filter callback by name.
 *
 * \param [in] filter_cb Filter callback function.
 * \param [in] desc Filter description.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_filter_cb_register_by_name(ngknet_filter_cb_f filter_cb, const char *desc);

/*!
 * \brief Register filter callback with additional attribute description.
 *
 * If \c filter_cb_attr is not NULL, this function can register filter-specific
 * callback for filter description matches \c name of \c filter_cb_attr. If
 * \c create_cb or \c destroy_cb of \c filter_cb_attr is not NULL,
 * corresponding callback will be performed when that matched filter is
 * created or destoryed.
 *
 * This function is equivalent to \ref ngknet_filter_cb_register if
 * \c filter_cb_attr is NULL or \c name of \c filter_cb_attr is NULL.
 * This function is equivalent to \ref ngknet_filter_cb_register_by_name if
 * \c name of \c filter_cb_attr is set with NULL values for \c create_cb and
 * \c destroy_cb for \c filter_cb_attr.
 *
 * \param [in] filter_cb Filter callback function.
 * \param [in] filter_cb_attr Additional attribute associated with \c filter_cb.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_filter_cb_attr_register(ngknet_filter_cb_f filter_cb,
                               ngknet_filter_cb_attr_t *filter_cb_attr);

/*!
 * \brief Unregister filter callback.
 *
 * \param [in] filter_cb Filter callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_filter_cb_unregister(ngknet_filter_cb_f filter_cb);

/*!
 * \brief Register PTP Rx config set callback.
 *
 * \param [in] ptp_rx_config_set_cb Rx config set callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_config_set_cb_register(ngknet_ptp_config_set_cb_f ptp_rx_config_set_cb);

/*!
 * \brief Unregister PTP Rx config set callback.
 *
 * \param [in] ptp_rx_config_set_cb Rx config set callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_config_set_cb_unregister(ngknet_ptp_config_set_cb_f ptp_rx_config_set_cb);

/*!
 * \brief Register PTP Tx config set callback.
 *
 * \param [in] ptp_tx_config_set_cb Tx config set callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_config_set_cb_register(ngknet_ptp_config_set_cb_f ptp_tx_config_set_cb);

/*!
 * \brief Unregister PTP Tx config set callback.
 *
 * \param [in] ptp_tx_config_set_cb Tx config set callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_config_set_cb_unregister(ngknet_ptp_config_set_cb_f ptp_tx_config_set_cb);

/*!
 * \brief Register PTP Rx HW timestamp get callback.
 *
 * \param [in] ptp_rx_hwts_get_cb Rx HW timestamp get callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_hwts_get_cb_register(ngknet_ptp_hwts_get_cb_f ptp_rx_hwts_get_cb);

/*!
 * \brief Unregister PTP Rx HW timestamp get callback.
 *
 * \param [in] ptp_rx_hwts_get_cb Rx HW timestamp get callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_hwts_get_cb_unregister(ngknet_ptp_hwts_get_cb_f ptp_rx_hwts_get_cb);

/*!
 * \brief Register PTP Tx HW timestamp get callback.
 *
 * \param [in] ptp_tx_hwts_get_cb Tx HW timestamp get callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_hwts_get_cb_register(ngknet_ptp_hwts_get_cb_f ptp_tx_hwts_get_cb);

/*!
 * \brief Unregister PTP Tx HW timestamp get callback.
 *
 * \param [in] ptp_tx_hwts_get_cb Tx HW timestamp get callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_hwts_get_cb_unregister(ngknet_ptp_hwts_get_cb_f ptp_tx_hwts_get_cb);

/*!
 * \brief Register PTP Tx meta set callback.
 *
 * \param [in] ptp_tx_meta_set_cb Tx meta set callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_meta_set_cb_register(ngknet_ptp_meta_set_cb_f ptp_tx_meta_set_cb);

/*!
 * \brief Unregister PTP Tx meta set callback.
 *
 * \param [in] ptp_tx_meta_set_cb Tx meta set callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_meta_set_cb_unregister(ngknet_ptp_meta_set_cb_f ptp_tx_meta_set_cb);

/*!
 * \brief Register PTP PHC index get callback.
 *
 * \param [in] ptp_phc_index_get_cb PHC index get callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_phc_index_get_cb_register(ngknet_ptp_phc_index_get_cb_f ptp_phc_index_get_cb);

/*!
 * \brief Unregister PTP PHC index get callback.
 *
 * \param [in] ptp_phc_index_get_cb PHC index get callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_phc_index_get_cb_unregister(ngknet_ptp_phc_index_get_cb_f ptp_phc_index_get_cb);

/*!
 * \brief Register PTP device control callback.
 *
 * \param [in] ptp_dev_ctrl_cb Device control callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_dev_ctrl_cb_register(ngknet_ptp_dev_ctrl_cb_f ptp_dev_ctrl_cb);

/*!
 * \brief Unregister PTP device control callback.
 *
 * \param [in] ptp_dev_ctrl_cb Device control callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_dev_ctrl_cb_unregister(ngknet_ptp_dev_ctrl_cb_f ptp_dev_ctrl_cb);

/*!
 * \brief Register PTP RX pre processing callback.
 *
 * \param [in] ptp_rx_pre_process_cb RX pre processing callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_pre_process_cb_register(ngknet_ptp_rx_pre_process_cb_f ptp_rx_pre_process_cb);

/*!
 * \brief Unregister PTP RX pre processing callback.
 *
 * \param [in] ptp_rx_pre_process_cb RX pre processing callback function.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_pre_process_cb_unregister(ngknet_ptp_rx_pre_process_cb_f ptp_rx_pre_process_cb);

#endif /* NGKNET_KAPI_H */

