/*! \file ngknet_extra.h
 *
 * Generic data structure definitions for NGKNET enhancement.
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

#ifndef NGKNET_EXTRA_H
#define NGKNET_EXTRA_H

#include <lkm/ngknet_kapi.h>

/*!
 * \brief Filter control.
 */
struct filt_ctrl {
    /*! List head */
    struct list_head list;

    /*! Device number */
    int dev_no;

    /*! Number of hits */
    uint64_t hits;

    /*! Filter description */
    ngknet_filter_t filt;

    /*! Filter callback */
    ngknet_filter_cb_f filter_cb;

    /*! Filter create callback */
    ngknet_filter_create_cb_f create_cb;

    /*! Filter destroy callback */
    ngknet_filter_destroy_cb_f destroy_cb;
};

/*!
 * \brief Create filter.
 *
 * \param [in] dev Device structure point.
 * \param [in] filter Filter structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_filter_create(struct ngknet_dev *dev, ngknet_filter_t *filter);

/*!
 * \brief Destroy filter.
 *
 * \param [in] dev Device structure point.
 * \param [in] id Filter ID.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_filter_destroy(struct ngknet_dev *dev, int id);

/*!
 * \brief Destroy all the filters.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_filter_destroy_all(struct ngknet_dev *dev);

/*!
 * \brief Get filter.
 *
 * \param [in] dev Device structure point.
 * \param [in] id Filter ID.
 * \param [out] filter Filter structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_filter_get(struct ngknet_dev *dev, int id, ngknet_filter_t *filter);

/*!
 * \brief Get the next filter.
 *
 * \param [in] dev Device structure point.
 * \param [out] filter Filter structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_filter_get_next(struct ngknet_dev *dev, ngknet_filter_t *filter);

/*!
 * \brief Filter packet.
 *
 * \param [in] dev Device structure point.
 * \param [in] skb Rx packet SKB.
 *
 * \retval Matched network interface.
 * \retval NULL No matched network interface.
 */
extern int
ngknet_rx_pkt_filter(struct ngknet_dev *dev, struct sk_buff *skb);

/*!
 * \brief Filter frame.
 *
 * \param [in] dev Device structure point.
 * \param [in] frame Data frame.
 * \param [out] ndev Network interface.
 *
 * \retval Matched network interface.
 * \retval NULL No matched network interface.
 */
extern int
ngknet_rx_xdp_filter(struct ngknet_dev *dev, void *frame,
                     struct net_device **ndev);

/*!
 * \brief Rx rate limit control.
 *
 * This contains all the control information for Rx rate limit such as
 * the number of Rx packets, status related to Rx rate limit, etc.
 *
 * The rate limit is kernel-oriented, i.e. all the Rx packets from any
 * device/channel will be accounted for. Once the received packets reach
 * the limit value in an 1-Sec interval, the driver API XXXX_rx_suspend()
 * will be called to suspend Rx. The 1-Sec basis timer will call the driver
 * API XXXX_rx_resume() to resume Rx and reset rate-related status/counters
 * at the begin of the next 1-Sec interval.
 *
 * The NGKNET module parameter 'rx_rate_limit' is used to decide the maximum
 * Rx rate. Disable Rx rate limit if set 0. It can be set when inserting
 * NGKNET module or modified using its SYSFS attributions.
 */
struct ngknet_rl_ctrl {
    /*! Rx packets */
    int rx_pkts;

    /*! Rx overruns */
    int rx_overruns;

    /*! Rx ticks */
    int rx_ticks;

    /*! Active devices under rate control */
    int dev_active[NUM_PDMA_DEV_MAX];

    /*! Paused devices due to no Rx credit */
    int dev_paused[NUM_PDMA_DEV_MAX];

    /*! Rate limit timer */
    struct timer_list timer;

    /*! Rate limit lock */
    spinlock_t lock;

    /*! Devices */
    struct ngknet_dev *devs;

    /*! Rate limit status indicator */
    int started;
};

/*!
 * \brief Initialize Rx rate limit.
 *
 * \param [in] devs Devices array.
 */
extern void
ngknet_rx_rate_limit_init(struct ngknet_dev *devs);

/*!
 * \brief Cleanup Rx rate limit.
 */
extern void
ngknet_rx_rate_limit_cleanup(void);

/*!
 * \brief Get Rx rate limit state.
 */
extern int
ngknet_rx_rate_limit_started(void);

/*!
 * \brief Start Rx rate limit.
 *
 * \param [in] dev Device structure point.
 */
extern void
ngknet_rx_rate_limit_start(struct ngknet_dev *dev);

/*!
 * \brief Stop Rx rate limit.
 *
 * \param [in] dev Device structure point.
 */
extern void
ngknet_rx_rate_limit_stop(struct ngknet_dev *dev);

/*!
 * \brief Limit Rx rate.
 *
 * \param [in] dev Device structure point.
 */
extern void
ngknet_rx_rate_limit(struct ngknet_dev *dev, int limit);

/*!
 * \brief Schedule Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 */
extern void
ngknet_tx_queue_schedule(struct ngknet_dev *dev, struct pkt_buf *pkb, int *queue);

/*!
 * \brief Dump packet content.
 *
 * \param [in] data Packet data.
 * \param [in] len Data length.
 */
extern void
ngknet_pkt_dump(uint8_t *data, int len);

/*!
 * \brief Packet statistics.
 *
 * \param [in] pdev Packet device data structure.
 * \param [in] dir Packet direction.
 */
extern void
ngknet_pkt_stats(struct pdma_dev *pdev, int dir);

#endif /* NGKNET_EXTRA_H */

