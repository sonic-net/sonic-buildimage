/*! \file ngknet_main.h
 *
 * Data structure and macro definitions for NGKNET kernel module.
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

#ifndef NGKNET_MAIN_H
#define NGKNET_MAIN_H

#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <lkm/lkm.h>
#include <lkm/ngknet_dev.h>
#include <bcmcnet/bcmcnet_core.h>

#ifdef NGKNET_XDP_NATIVE
#include <net/xdp.h>
#include <net/xdp_sock_drv.h>
#include <trace/events/xdp.h>
#endif

/*!
 * Debug levels
 */
#define DBG_LVL_VERB        0x0001
#define DBG_LVL_PKT         0x0002
#define DBG_LVL_CMD         0x0004
#define DBG_LVL_IRQ         0x0008
#define DBG_LVL_NAPI        0x0010
#define DBG_LVL_NDEV        0x0020
#define DBG_LVL_FILT        0x0040
#define DBG_LVL_RCPU        0x0080
#define DBG_LVL_WARN        0x0100
#define DBG_LVL_PDMP        0x0200
#define DBG_LVL_RATE        0x0400
#define DBG_LVL_LINK        0x0800

#define DBG_VERB(_s)        do { if (debug & DBG_LVL_VERB) printk _s; } while (0)
#define DBG_PKT(_s)         do { if (debug & DBG_LVL_PKT)  printk _s; } while (0)
#define DBG_CMD(_s)         do { if (debug & DBG_LVL_CMD)  printk _s; } while (0)
#define DBG_IRQ(_s)         do { if (debug & DBG_LVL_IRQ)  printk _s; } while (0)
#define DBG_NAPI(_s)        do { if (debug & DBG_LVL_NAPI) printk _s; } while (0)
#define DBG_NDEV(_s)        do { if (debug & DBG_LVL_NDEV) printk _s; } while (0)
#define DBG_FILT(_s)        do { if (debug & DBG_LVL_FILT) printk _s; } while (0)
#define DBG_RCPU(_s)        do { if (debug & DBG_LVL_RCPU) printk _s; } while (0)
#define DBG_WARN(_s)        do { if (debug & DBG_LVL_WARN) printk _s; } while (0)
#define DBG_PDMP(_s)        do { if (debug & DBG_LVL_PDMP) printk _s; } while (0)
#define DBG_RATE(_s)        do { if (debug & DBG_LVL_RATE) printk _s; } while (0)
#define DBG_LINK(_s)        do { if (debug & DBG_LVL_LINK) printk _s; } while (0)

/* Take over the control of SKB and send packet to network interface. */
typedef void (*ngknet_pkt_recv_f)(struct net_device *ndev, struct sk_buff *skb);

#define SAI_FIXUP           1
#define KNET_SVTAG_HOTFIX   1
/*!
 * Device description
 */
struct ngknet_dev {
    /* Device information */
    ngknet_dev_info_t dev_info;

    /*! Base address for PCI register access */
    volatile void *base_addr;

    /*! Required for DMA memory control */
    struct device *dev;

    /*! Required for PCI memory control */
    struct pci_dev *pci_dev;

    /*! Base network device */
    struct net_device *net_dev;

    /*! PDMA device */
    struct pdma_dev pdma_dev;

    /*! Virtual network devices, 0 is used for max ID number. */
    struct net_device *vdev[NUM_VDEV_MAX + 1];

    /*! Virtual network devices bound to queue */
    struct net_device *bdev[NUM_Q_MAX];

    /*! Filter list */
    struct list_head filt_list;

    /*! Filter control, 0 is reserved */
    void *fc[NUM_FILTER_MAX + 1];

    /*! Callback control */
    struct ngknet_callback_ctrl *cbc;

    /*! RCPU control */
    struct ngknet_rcpu_hdr rcpu_ctrl;

    /*! NGKNET lock */
    spinlock_t lock;

    /*! NGKNET wait queue */
    wait_queue_head_t wq;

    /*! VNET wait queue */
    wait_queue_head_t vnet_wq;

    /*! VNET is active */
    atomic_t vnet_active;

    /*! HNET wait queue */
    wait_queue_head_t hnet_wq;

    /*! HNET is active */
    atomic_t hnet_active;

    /*! HNET deamon */
    struct task_struct *hnet_task;

    /*! HNET work */
    struct work_struct hnet_work;

    /*! PTP Tx queue */
    struct sk_buff_head ptp_tx_queue;

    /*! PTP Tx work */
    struct work_struct ptp_tx_work;

    /*! NGKNET work queue for link process */
    struct workqueue_struct *link_wq;

#ifdef NGKNET_XDP_NATIVE
    /*! XSK buffer pool */
    struct xsk_buff_pool *xsk_pool;

    /*! XSK Tx queue */
    int xsk_queue;

    /* XDP program number */
    int xprog_num;
#endif

    /*! Flags */
    int flags;
    /*! NGKNET device is active */
#define NGKNET_DEV_ACTIVE      (1 << 0)
    /*! NGKNET AF_XDP in Zero-copy mode */
#define NGKNET_XSK_ZC          (1 << 1)
};

/*!
 * Network interface specific private data
 */
struct ngknet_private {
    /*! Network device */
    struct net_device *net_dev;

    /*! Network stats */
    struct net_device_stats stats;

    /*! NGKNET device */
    struct ngknet_dev *bkn_dev;

    /*! Network interface */
    ngknet_netif_t netif;

#ifdef NGKNET_XDP_NATIVE
    /*! XDP program */
    struct bpf_prog *xdp_prog;

    /*! XDP Rx info */
    struct xdp_rxq_info xri;

    /*! XSK ZC mode */
    bool xsk_zc;
#endif

    /*! Packet receive callback */
    ngknet_pkt_recv_f pkt_recv;

    /*! Link work */
    struct work_struct link_work;

    /*! Users of this network interface */
    int users;

    /*! Wait for this network interface free */
    int wait;

    /*! HW timestamp Rx filter */
    int hwts_rx_filter;

    /*! HW timestamp Tx type */
    int hwts_tx_type;

#if NGKNET_ETHTOOL_LINK_SETTINGS
    /* Link settings */
    struct ethtool_link_settings link_settings;
#endif
#if SAI_FIXUP && KNET_SVTAG_HOTFIX  /* SONIC-76482 */
    uint8_t svtag[4];
#endif
};

/*!
 * \brief Create network interface.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] netif Network interface structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_netif_create(struct ngknet_dev *dev, ngknet_netif_t *netif);

/*!
 * \brief Destroy network interface.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] id Network interface ID.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_netif_destroy(struct ngknet_dev *dev, int id);

/*!
 * \brief Get network interface.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] id Network interface ID.
 * \param [out] netif Network interface structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_netif_get(struct ngknet_dev *dev, int id, ngknet_netif_t *netif);

/*!
 * \brief Get the next network interface.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [out] netif Network interface structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
ngknet_netif_get_next(struct ngknet_dev *dev, ngknet_netif_t *netif);

/*!
 * \brief Get debug level.
 *
 * \retval Current debug level.
 */
extern int
ngknet_debug_level_get(void);

/*!
 * \brief Set debug level.
 *
 * \param [in] debug_level Debug level to be set.
 */
extern void
ngknet_debug_level_set(int debug_level);

/*!
 * \brief Get Rx rate limit.
 *
 * \retval Current Rx rate limit.
 */
extern int
ngknet_rx_rate_limit_get(void);

/*!
 * \brief Set Rx rate limit.
 *
 * \param [in] rate_limit Rx rate limit to be set.
 */
extern void
ngknet_rx_rate_limit_set(int rate_limit);

/*!
 * \brief Get page buffer mode.
 *
 * \retval Current page buffer mode.
 */
extern int
ngknet_page_buffer_mode_get(void);

#endif /* NGKNET_MAIN_H */

