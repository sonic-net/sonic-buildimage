/*! \file ngknet_main.c
 *
 * NGKNET module entry.
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

/*
 * This module implements a Linux network driver for Broadcom
 * XGS switch devices. The driver simultaneously serves a
 * number of virtual Linux network devices.
 *
 * Packets received from the switch device are sent to a virtual
 * Linux network device based on a set of packet filters.
 *
 * Packets from the virtual Linux network devices are multiplexed
 * with fifo mode if only one Tx queue enabled.
 *
 * A command-based IOCTL interface is used for managing the devices,
 * packet filters and virtual Linux network interfaces.
 *
 * A virtual network interface can be configured to work in RCPU
 * mode, which means that packets from the switch device will
 * be encapsulated with a RCPU header and a block of meta data
 * that basically contains the core DCB information. Likewise,
 * packets received from the Linux network stack are assumed to
 * be RCPU encapsulated when going out on an interface in RCPU
 * mode. If a virtual network interface does not work in RCPU
 * mode and transmits to this interface will unmodified go to
 * specified physical switch port, DCB information should be
 * provided when the interface is created.
 *
 * The module implements basic Rx DMA rate control. The rate is
 * specified in packets per second, and different Rx DMA channels
 * can be configured to use different maximum packet rates.
 * The packet rate can be configure as a module parameter, and
 * it can also be changed dynamically through the proc file
 * system (syntax is described in function header comment).
 *
 * For a list of supported module parameters, please see below.
 */

#include <linux/kconfig.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if.h>
#include <linux/if_vlan.h>
#include <linux/net_tstamp.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/time.h>
#include <linux/random.h>

#include <lkm/ngbde_kapi.h>
#include <lkm/ngknet_dev.h>
#include <lkm/ngknet_ioctl.h>
#include <bcmcnet/bcmcnet_core.h>
#include "ngknet_main.h"
#include "ngknet_extra.h"
#include "ngknet_procfs.h"
#include "ngknet_callback.h"
#include "ngknet_ptp.h"
#include "ngknet_xdp.h"
#include "ngknet_xsk.h"

#if SAI_FIXUP && KNET_SVTAG_HOTFIX  /* SONIC-76482 */
#define NGKNET_IOC_SVTAG_SET            (SIOCDEVPRIVATE + 0)
#define NGKNET_IOC_SVTAG_MAGIC          0x53565447 /* "SVTG" */
#define NGKNET_NETIF_F_DEL_SVTAG        (1U << 15) /* Remove SVTAG from the RX packets */
#define NGKNET_NETIF_F_ADD_SVTAG        (1U << 14) /* Insert SVTAG into the TX packets */
#define NGKNET_SVTAG_PKTYPE_NONMACSEC   0  /* Unsecure data packet (Untag Control Port packet) */
#define NGKNET_SVTAG_PKTYPE_MACSEC      1  /* Secure data packet (Tag Controlled Port packet) */
#define NGKNET_SVTAG_PKTYPE_KAY         2  /* KaY Frame (KaY Uncontrolled Port packet) */
struct ifru_svtag {
    uint32_t magic;
    uint32_t flags;
    uint8_t svtag[4];
};
#endif
/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Network Device Driver Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! \cond */
static int debug = 0;
MODULE_PARAM(debug, int, 0);
MODULE_PARM_DESC(debug,
"Debug level (default 0)");
/*! \endcond */

/*! \cond */
static char *base_dev_name = "bcm";
MODULE_PARAM(base_dev_name, charp, 0);
MODULE_PARM_DESC(base_dev_name,
"Base device name (default bcm0, bcm1, etc.)");
/*! \endcond */

/*! \cond */
static char *mac_addr = NULL;
MODULE_PARAM(mac_addr, charp, 0);
MODULE_PARM_DESC(mac_addr,
"Ethernet MAC address (default 02:10:18:xx:xx:xx)");
/*! \endcond */

/*! \cond */
static int default_mtu = 1500;
MODULE_PARAM(default_mtu, int, 0);
MODULE_PARM_DESC(default_mtu,
"MTU size for KNET network interfaces (default 1500)");
/*! \endcond */

/*! \cond */
static int rx_buffer_size = RX_BUF_SIZE_DFLT;
MODULE_PARAM(rx_buffer_size, int, 0);
MODULE_PARM_DESC(rx_buffer_size,
"RX packet buffer size in bytes (default 9216)");
/*! \endcond */

/*! \cond */
static int rx_rate_limit = -1;
MODULE_PARAM(rx_rate_limit, int, 0);
MODULE_PARM_DESC(rx_rate_limit,
"Rx rate limit in packets per second (default -1 for no limit)");
/*! \endcond */

/*! \cond */
static int tx_polling = 0;
MODULE_PARAM(tx_polling, int, 0);
MODULE_PARM_DESC(tx_polling,
"Enable Tx poll mode (default 0 for interrupt mode)");
/*! \endcond */

/*! \cond */
static int rx_batching = 0;
MODULE_PARAM(rx_batching, int, 0);
MODULE_PARM_DESC(rx_batching,
"Enable Rx batch fill mode (default 0 for single fill mode)");
/*! \endcond */

/*! \cond */
static int page_buffer_mode = 0;
MODULE_PARAM(page_buffer_mode, int, 0);
MODULE_PARM_DESC(page_buffer_mode,
"Enable SKB page buffer mode (default 0 for legacy SKB mode)");
/*! \endcond */

/*! \cond */
int xsk_napi_tx = 0;
MODULE_PARAM(xsk_napi_tx, int, 0);
MODULE_PARM_DESC(xsk_napi_tx,
"Use NAPI to transmit XSK packet (default 0 to use kernel thread)");
/*! \endcond */

typedef int (*drv_ops_attach)(struct pdma_dev *dev);

struct bcmcnet_drv_ops {
    const char         *drv_desc;
    drv_ops_attach      drv_attach;
    drv_ops_attach      drv_detach;
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    static struct bcmcnet_drv_ops _bd##_cnet_drv_ops = { \
        #_bd, \
        _bd##_cnet_pdma_attach, \
        _bd##_cnet_pdma_detach, \
    };
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_cnet_drv_ops,
static struct bcmcnet_drv_ops *drv_ops[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};
static int drv_num = sizeof(drv_ops) / sizeof(drv_ops[0]);

struct ngknet_dev ngknet_devices[NUM_PDMA_DEV_MAX];

/* Default random MAC address has Broadcom OUI with local admin bit set */
static uint8_t ngknet_dev_mac[6] = {0x02, 0x10, 0x18, 0x00, 0x00, 0x00};

/* Interrupt handles */
struct ngknet_intr_handle {
    struct napi_struct napi;
    struct intr_handle *hdl;
    int napi_resched;
    int napi_pending;
};

static struct ngknet_intr_handle priv_hdl[NUM_PDMA_DEV_MAX][NUM_Q_MAX];

/*!
 * Read 32-bit register callback
 */
static int
ngknet_dev_read32(struct pdma_dev *pdev, uint32_t addr, uint32_t *data)
{
    *data = ngbde_kapi_pio_read32(pdev->unit, addr);

    return 0;
}

/*!
 * Write 32-bit register callback
 */
static int
ngknet_dev_write32(struct pdma_dev *pdev, uint32_t addr, uint32_t data)
{
    ngbde_kapi_pio_write32(pdev->unit, addr, data);

    return 0;
}

/*!
 * Set Rx HW timestamping.
 */
static int
ngknet_ptp_rx_hwts_set(struct net_device *ndev, struct sk_buff *skb)
{
    struct skb_shared_hwtstamps *shhwtstamps = skb_hwtstamps(skb);
    uint64_t ts = 0;
    int rv;

    rv = ngknet_ptp_rx_hwts_get(ndev, skb, &ts);
    if (SHR_FAILURE(rv) || !ts) {
        return SHR_E_FAIL;
    }

    memset(shhwtstamps, 0, sizeof(*shhwtstamps));
    shhwtstamps->hwtstamp = ns_to_ktime(ts);

    return SHR_E_NONE;
}

/*!
 * \brief Process Rx packet.
 *
 * Add RCPU encapsulation or strip matadata if needed
 *
 * \param [in] ndev Network device structure point.
 * \param [in] oskb Rx packet SKB.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_rx_frame_process(struct net_device *ndev, struct sk_buff **oskb)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct sk_buff *skb = *oskb;
    struct ngknet_rcpu_hdr *rch = (struct ngknet_rcpu_hdr *)skb->data;
    struct pkt_hdr *pkh = (struct pkt_hdr *)skb->data;
    uint8_t meta_len = pkh->meta_len;
    uint8_t fcs_len = pdev->flags & PDMA_NO_FCS ? 0 : ETH_FCS_LEN;
#if SAI_FIXUP && KNET_SVTAG_HOTFIX
    int offset;
#endif

    /* Remove FCS from packet length */
    skb_trim(skb, skb->len - fcs_len);
    pkh->data_len -= fcs_len;

    if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
        /* Set up RCPU header */
        memcpy(skb->data, skb->data + PKT_HDR_SIZE + meta_len, 2 * ETH_ALEN);
        if (*(uint32_t *)&dev->rcpu_ctrl.dst_mac[0] != 0 ||
            *(uint16_t *)&dev->rcpu_ctrl.dst_mac[4] != 0) {
            memcpy(rch->dst_mac, dev->rcpu_ctrl.dst_mac, ETH_ALEN);
        }
        if (*(uint32_t *)&dev->rcpu_ctrl.src_mac[0] != 0 ||
            *(uint16_t *)&dev->rcpu_ctrl.src_mac[4] != 0) {
            memcpy(rch->src_mac, dev->rcpu_ctrl.src_mac, ETH_ALEN);
        }
        rch->vlan_tpid = htons(dev->rcpu_ctrl.vlan_tpid);
        rch->vlan_tci = htons(dev->rcpu_ctrl.vlan_tci);
        rch->eth_type = htons(dev->rcpu_ctrl.eth_type);
        rch->pkt_sig = htons(dev->rcpu_ctrl.pkt_sig);
        rch->op_code = RCPU_OPCODE_RX;
        rch->flags = RCPU_FLAG_MODHDR;
        rch->trans_id = htons(dev->rcpu_ctrl.trans_id);
        rch->data_len = htons(pkh->data_len);
    } else {
        /* Remove packet header and meta data */
        skb_pull(skb, PKT_HDR_SIZE + meta_len);
    }

    /* Do Rx timestamping */
    if (priv->hwts_rx_filter) {
        ngknet_ptp_rx_hwts_set(ndev, skb);
    }
    BUILD_BUG_ON(sizeof(struct ngknet_callback_desc) > sizeof(skb->cb));
#if SAI_FIXUP && KNET_SVTAG_HOTFIX /* SONIC-76482 */
    if (priv->netif.flags & NGKNET_NETIF_F_DEL_SVTAG) {
        if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
            offset = PKT_HDR_SIZE + meta_len + 2*ETH_ALEN;
            memmove(skb->data + offset, skb->data + offset + 4, skb->len - offset - 4);
            skb_trim(skb, skb->len - 4);
            pkh->data_len -= 4;
            rch->data_len = htons(pkh->data_len);
        } else {
            offset = 2*ETH_ALEN;
            memmove(skb->data + offset, skb->data + offset + 4, skb->len - offset - 4);
            skb_trim(skb, skb->len - 4);
            pkh->data_len -= 4;
        }
    }
#endif

    /* Optional callback handle */
    if (dev->cbc->rx_cb) {
        struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
        cbd->dinfo = &dev->dev_info;
        cbd->netif = &priv->netif;
        cbd->net_dev = priv->net_dev;

        if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
            cbd->pmd = skb->data + PKT_HDR_SIZE;
            cbd->pkt_len = ntohs(rch->data_len);
        } else {
            cbd->pmd = skb->data - meta_len;
            cbd->pkt_len = pkh->data_len;
        }
        cbd->pmd_len = meta_len;
        skb = dev->cbc->rx_cb(skb);
        if (!skb) {
            *oskb = NULL;
            return SHR_E_UNAVAIL;
        }
        if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
            rch = (struct ngknet_rcpu_hdr *)skb->data;
            rch->data_len = htons(skb->len - PKT_HDR_SIZE - meta_len);
        }
    }

    /* Update SKB pointer */
    *oskb = skb;

    return SHR_E_NONE;
}

/*!
 * Get network interface status.
 */
static bool
ngknet_netif_ok(struct net_device *ndev)
{
    return (netif_carrier_ok(ndev) && netif_running(ndev));
}

/*!
 * \brief Network interface Rx function.
 *
 * After processing the packet, send it up to network stack.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] skb Rx packet SKB.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_netif_recv(struct net_device *ndev, struct sk_buff *skb)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pkt_hdr *pkh = (struct pkt_hdr *)skb->data;
    uint16_t proto;
    int rv;

    /* Handle one incoming packet */
    rv = ngknet_rx_frame_process(ndev, &skb);
    if (!skb) {
        return SHR_E_NONE;
    }
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    DBG_VERB(("Rx packet sent up to ndev%d (%d bytes).\n",
              priv->netif.id, skb->len));
    if (debug & DBG_LVL_PDMP) {
        ngknet_pkt_dump(skb->data, skb->len);
    }

    if (ndev->features & NETIF_F_RXCSUM) {
        if ((pkh->attrs & (PDMA_RX_TU_CSUM | PDMA_RX_IP_CSUM)) ==
            (PDMA_RX_TU_CSUM | PDMA_RX_IP_CSUM)) {
            skb->ip_summed = CHECKSUM_UNNECESSARY;
        } else {
            skb_checksum_none_assert(skb);
        }
    }

    proto = eth_type_trans(skb, ndev);
    if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
        skb->protocol = htons(dev->rcpu_ctrl.eth_type);
    } else if (!(pkh->attrs & PDMA_RX_SET_PROTO) || !skb->protocol) {
        skb->protocol = proto;
    }

    skb_record_rx_queue(skb, pkh->queue_id);

    /* Update accounting */
    priv->stats.rx_packets++;
    priv->stats.rx_bytes += skb->len;

    netif_receive_skb(skb);

    /* Rate limit */
    if (rx_rate_limit >= 0) {
        if (!ngknet_rx_rate_limit_started()) {
            ngknet_rx_rate_limit_start(dev);
        }
        ngknet_rx_rate_limit(dev, rx_rate_limit);
    }

    return SHR_E_NONE;
}

/*!
 * \brief Packet Rx callback.
 *
 * Take over the control of SKB and send packet to network interface.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] skb Rx packet SKB.
 */
static void
ngknet_pkt_recv(struct net_device *ndev, struct sk_buff *skb)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    unsigned long flags;

    /* Send the packet to network interface */
    if (ngknet_netif_ok(ndev)) {
        if (SHR_FAILURE(ngknet_netif_recv(ndev, skb))) {
            dev_kfree_skb_any(skb);
            if (!netif_queue_stopped(ndev)) {
                priv->stats.rx_dropped++;
            }
        }
    } else {
        dev_kfree_skb_any(skb);
    }

    spin_lock_irqsave(&dev->lock, flags);
    priv->users--;
    if (!priv->users && priv->wait) {
        wake_up(&dev->wq);
    }
    spin_unlock_irqrestore(&dev->lock, flags);
}

/*!
 * \brief Driver Rx callback.
 *
 * After processing the packet, send it up to network stack.
 *
 * \param [in] pdev Packet DMA device structure point.
 * \param [in] buf Raw Rx buffer.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_frame_recv(struct pdma_dev *pdev, int queue, void *buf)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct sk_buff *skb = (struct sk_buff *)buf;
    int rv;

    DBG_VERB(("Rx packet (%d bytes).\n", skb->len));
    if (debug & DBG_LVL_PDMP) {
        ngknet_pkt_dump(skb->data, skb->len);
    }

    DBG_NDEV(("Valid virtual network devices: %ld.\n", (long)dev->vdev[0]));

    /* Go through the filters and process it. */
    rv = ngknet_rx_pkt_filter(dev, skb);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    /* Measure speed */
    if (debug & DBG_LVL_RATE) {
        ngknet_pkt_stats(pdev, PDMA_Q_RX);
    }

    return rv;
}

/*!
 * Set Tx HW timestamping.
 */
static int
ngknet_ptp_tx_hwts_set(struct net_device *ndev, struct sk_buff *skb)
{
    struct skb_shared_hwtstamps shhwtstamps;
    uint64_t ts = 0;
    int rv;

    rv = ngknet_ptp_tx_hwts_get(ndev, skb, &ts);
    if (SHR_FAILURE(rv) || !ts) {
        return SHR_E_FAIL;
    }

    memset(&shhwtstamps, 0, sizeof(shhwtstamps));
    shhwtstamps.hwtstamp = ns_to_ktime(ts);
    skb_tstamp_tx(skb, &shhwtstamps);

    return SHR_E_NONE;
}

/*!
 * PTP Tx worker.
 */
static void
ngknet_ptp_tx_work(struct work_struct *work)
{
    struct ngknet_dev *dev = container_of(work, struct ngknet_dev, ptp_tx_work);
    struct sk_buff *skb;
    int rv;

    while (skb_queue_len(&dev->ptp_tx_queue)) {
        skb = skb_dequeue(&dev->ptp_tx_queue);
        rv = ngknet_ptp_tx_hwts_set(dev->net_dev, skb);
        if (SHR_FAILURE(rv)) {
            printk("Timestamp value has not been set for current skb.\n");
        }
        dev_kfree_skb_any(skb);
    }
}

/*!
 * Config Tx metadata for HW timestamping.
 */
static int
ngknet_ptp_tx_config(struct net_device *ndev, struct sk_buff *skb)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    uint64_t *tx_ts = (uint64_t *)skb->cb;
    int rv;

    if (priv->netif.type == NGKNET_NETIF_T_PORT ||
        priv->netif.type == NGKNET_NETIF_T_META) {
        rv = ngknet_ptp_tx_meta_set(ndev, skb);
        if (SHR_FAILURE(rv)) {
            return rv;
        }
    } else if (priv->hwts_tx_type != HWTSTAMP_TX_ONESTEP_SYNC) {
        return SHR_E_UNAVAIL;
    }

    /* For 1step meta_set will populate the TX timestamp for
     * the required PTP packets (i.e. DELAY_REQ), only in such
     * case we should schedule ptp_tx_work for the TX timestamp
     * to be sent back on the socket.
     */
    if (priv->hwts_tx_type == HWTSTAMP_TX_ONESTEP_SYNC &&
        *tx_ts == 0) {
        return SHR_E_NONE;
    }

    skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;

    if (priv->hwts_tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        skb_queue_tail(&dev->ptp_tx_queue, skb_get(skb));
        schedule_work(&dev->ptp_tx_work);
    }

    return SHR_E_NONE;
}

/*!
 * \brief Process Tx packet.
 *
 * Strip RCPU encapsulation, setup CNET packet buffer, add vlan tag
 * or pad the packet.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] oskb Tx packet SKB.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_tx_frame_process(struct net_device *ndev, struct sk_buff **oskb)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct sk_buff *skb = *oskb;
    struct ngknet_rcpu_hdr *rch = (struct ngknet_rcpu_hdr *)skb->data;
    struct pkt_hdr *pkh = (struct pkt_hdr *)skb->data;
    struct sk_buff *nskb = NULL;
    char *data = NULL;
    uint32_t copy_len, meta_len, data_len, pkt_len, tag_len, pad_len;
    uint16_t fcs_len = pdev->flags & PDMA_NO_FCS ? 0 : ETH_FCS_LEN;
    uint16_t tpid;

    /* Set up packet header */
    if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
        if (rch->flags & RCPU_FLAG_KEEP_FCS) {
            fcs_len = 0;
        }
        /* RCPU encapsulation packet */
        data_len = pkh->attrs & PDMA_TX_HDR_COOKED ?
                   pkh->data_len - fcs_len : ntohs(rch->data_len);
        pkt_len = PKT_HDR_SIZE + rch->meta_len + data_len;
        if (skb->len != pkt_len || skb->len < (PKT_HDR_SIZE + ETH_HLEN)) {
            DBG_WARN(("Tx drop: Invalid packet length\n"));
            return SHR_E_PARAM;
        }
        if (dev->rcpu_ctrl.pkt_sig && dev->rcpu_ctrl.pkt_sig != ntohs(rch->pkt_sig)) {
            DBG_WARN(("Tx drop: Invalid packet signature\n"));
            return SHR_E_PARAM;
        }
        if (pkh->attrs & PDMA_TX_HDR_COOKED) {
            /* Resumed packet */
            return SHR_E_NONE;
        }
        pkh->data_len = data_len + fcs_len;
        pkh->meta_len = rch->meta_len;
        pkh->attrs = 0;
        if (rch->flags & RCPU_FLAG_MODHDR) {
            pkh->attrs |= PDMA_TX_HIGIG_PKT;
        }
        if (rch->flags & RCPU_FLAG_PAUSE) {
            pkh->attrs |= PDMA_TX_PAUSE_PKT;
        }
        if (rch->flags & RCPU_FLAG_PURGE) {
            pkh->attrs |= PDMA_TX_PURGE_PKT;
        }
        if (rch->flags & RCPU_FLAG_BIND_QUE) {
            pkh->attrs |= PDMA_TX_BIND_QUE;
        }
        if (rch->flags & RCPU_FLAG_NO_PAD) {
            pkh->attrs |= PDMA_TX_NO_PAD;
        }
    } else {
        /* Non-RCPU encapsulation packet */
        data_len = pkh->data_len - fcs_len;
        pkt_len = PKT_HDR_SIZE + pkh->meta_len + data_len;
        if (skb->len == pkt_len && pkh->attrs & PDMA_TX_HDR_COOKED &&
            pkh->pkt_sig == dev->rcpu_ctrl.pkt_sig) {
            /* Resumed packet */
            return SHR_E_NONE;
        }
        meta_len = 0;
        if (priv->netif.type == NGKNET_NETIF_T_PORT ||
            priv->netif.type == NGKNET_NETIF_T_META) {
            meta_len = priv->netif.meta_len;
            if (!meta_len) {
                printk("Tx abort: no metadata\n");
                return SHR_E_UNAVAIL;
            }
        }
        if (skb_header_cloned(skb) ||
            skb_headroom(skb) < (PKT_HDR_SIZE + meta_len + VLAN_HLEN) ||
            skb_tailroom(skb) < fcs_len) {
            nskb = skb_copy_expand(skb, PKT_HDR_SIZE + meta_len + VLAN_HLEN,
                                   fcs_len, GFP_ATOMIC);
            if (!nskb) {
                return SHR_E_MEMORY;
            }
            skb_shinfo(nskb)->tx_flags = skb_shinfo(skb)->tx_flags;
            nskb->sk = skb->sk;
            skb = nskb;
        }
        skb_push(skb, PKT_HDR_SIZE + meta_len);
        memset(skb->data, 0, PKT_HDR_SIZE + meta_len);
        pkh = (struct pkt_hdr *)skb->data;
        pkh->data_len = skb->len - PKT_HDR_SIZE - meta_len + fcs_len;
        pkh->meta_len = meta_len;
        pkh->attrs = 0;
        if (priv->netif.type == NGKNET_NETIF_T_PORT ||
            priv->netif.type == NGKNET_NETIF_T_META) {
            /* Send to physical port using netif metadata */
            if (priv->netif.meta_off) {
                memmove(skb->data + PKT_HDR_SIZE,
                        skb->data + PKT_HDR_SIZE + meta_len,
                        priv->netif.meta_off);
            }
            memcpy(skb->data + PKT_HDR_SIZE + priv->netif.meta_off,
                   priv->netif.meta_data, priv->netif.meta_len);
            pkh->attrs |= PDMA_TX_HIGIG_PKT;
        }
        pkh->pkt_sig = dev->rcpu_ctrl.pkt_sig;
    }

    /* Packet header done here */
    pkh->attrs |= PDMA_TX_HDR_COOKED;

    data = skb->data + PKT_HDR_SIZE + pkh->meta_len;
    tpid = data[12] << 8 | data[13];
    tag_len = (tpid == ETH_P_8021Q || tpid == ETH_P_8021AD) ? VLAN_HLEN : 0;

    /* Need to add VLAN tag if packet is untagged */
    if (tag_len == 0 && (priv->netif.vlan & 0xfff) != 0 &&
        (!(pkh->attrs & PDMA_TX_HIGIG_PKT) ||
         priv->netif.flags & NGKNET_NETIF_F_ADD_TAG)) {
        copy_len = PKT_HDR_SIZE + pkh->meta_len + 2 * ETH_ALEN;
        if (skb_header_cloned(skb) || skb_headroom(skb) < VLAN_HLEN) {
            nskb = skb_copy_expand(skb, VLAN_HLEN, 0, GFP_ATOMIC);
            if (!nskb) {
                return SHR_E_MEMORY;
            }
            skb_shinfo(nskb)->tx_flags = skb_shinfo(skb)->tx_flags;
            nskb->sk = skb->sk;
            skb = nskb;
        }
        skb_push(skb, VLAN_HLEN);
        memmove(skb->data, skb->data + VLAN_HLEN, copy_len);
        pkh = (struct pkt_hdr *)skb->data;
        data = skb->data + PKT_HDR_SIZE + pkh->meta_len;
        data[12] = 0x81;
        data[13] = 0x00;
        data[14] = priv->netif.vlan >> 8 & 0xf;
        data[15] = priv->netif.vlan & 0xff;
        pkh->data_len += VLAN_HLEN;
        tag_len = VLAN_HLEN;
    }
#if SAI_FIXUP && KNET_SVTAG_HOTFIX /* SONIC-76482 */
    if (priv->netif.flags & NGKNET_NETIF_F_ADD_SVTAG) {
        uint16_t ether_type = 0;
        static const uint16_t mgmt_et = 0x888e;
        static const uint8_t mgmt_dst[] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x03};

        copy_len = PKT_HDR_SIZE + pkh->meta_len + 2 * ETH_ALEN;
        if (skb_header_cloned(skb) || skb_headroom(skb) < VLAN_HLEN) {
            nskb = skb_copy_expand(skb, VLAN_HLEN, 0, GFP_ATOMIC);
            if (!nskb) {
                return SHR_E_MEMORY;
            }
            skb_shinfo(nskb)->tx_flags = skb_shinfo(skb)->tx_flags;
            nskb->sk = skb->sk;
            skb = nskb;
        }
        skb_push(skb, VLAN_HLEN);
        memmove(skb->data, skb->data + VLAN_HLEN, copy_len);
        pkh = (struct pkt_hdr *)skb->data;
        data = skb->data + PKT_HDR_SIZE + pkh->meta_len;
        ether_type = ((uint8_t)data[16] << 8) | (uint8_t)data[17];
        data[12] = priv->svtag[0];
        data[13] = priv->svtag[1];
        if (mgmt_et == ether_type && !memcmp(mgmt_dst, data, 6)) {
            if (priv->svtag[2])
                data[14] = NGKNET_SVTAG_PKTYPE_KAY << 2;
            else
                data[14] = NGKNET_SVTAG_PKTYPE_NONMACSEC << 2;
        } else {
            data[14] = priv->svtag[2]; /* secured if configured */
        }
        data[15] = priv->svtag[3];
        pkh->data_len += VLAN_HLEN;
        tag_len += VLAN_HLEN;
        printk(KERN_DEBUG "ether_type: %04x, pktype %d, subport %d\n", ether_type, (data[14] >> 2) & 0xf, data[15]);
    }
#endif
    /* Optional callback handle */
    if (dev->cbc->tx_cb) {
        struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
        cbd->dinfo = &dev->dev_info;
        cbd->netif = &priv->netif;
        cbd->pmd = skb->data + PKT_HDR_SIZE;
        cbd->pmd_len = pkh->meta_len;
        cbd->pkt_len = skb->len - PKT_HDR_SIZE - pkh->meta_len;
        skb = dev->cbc->tx_cb(skb);
        if (!skb) {
            if (!nskb) {
                *oskb = NULL;
            }
            return SHR_E_UNAVAIL;
        }
        pkh = (struct pkt_hdr *)skb->data;
        pkh->data_len = skb->len - PKT_HDR_SIZE - pkh->meta_len + fcs_len;
    }

    /* Pad packet if needed */
    pad_len = ETH_ZLEN + tag_len + fcs_len;
    if (pkh->data_len < pad_len && !(pkh->attrs & PDMA_TX_NO_PAD)) {
        pkh->data_len = pad_len;
        if (skb_padto(skb,
                      PKT_HDR_SIZE + pkh->meta_len + pkh->data_len - fcs_len)) {
            if (!nskb) {
                *oskb = NULL;
            }
            return SHR_E_MEMORY;
        }
    }

    /* Update SKB pointer */
    *oskb = skb;

    return SHR_E_NONE;
}

/*!
 * Network device detach callback
 */
static void
ngknet_ndev_detach(struct pdma_dev *pdev)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct net_device *ndev;
    int vdi;

    netif_tx_lock(dev->net_dev);
    netif_device_detach(dev->net_dev);
    netif_tx_unlock(dev->net_dev);

    for (vdi = 1; vdi <= NUM_VDEV_MAX; vdi++) {
        ndev = dev->vdev[vdi];
        if (!ndev) {
            continue;
        }
        netif_tx_lock(ndev);
        netif_device_detach(ndev);
        netif_tx_unlock(ndev);
    }
}

/*!
 * Network device attach callback
 */
static void
ngknet_ndev_attach(struct pdma_dev *pdev)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct net_device *ndev;
    int vdi;

    netif_tx_lock(dev->net_dev);
    netif_device_attach(dev->net_dev);
    netif_tx_unlock(dev->net_dev);

    for (vdi = 1; vdi <= NUM_VDEV_MAX; vdi++) {
        ndev = dev->vdev[vdi];
        if (!ndev) {
            continue;
        }
        netif_tx_lock(ndev);
        netif_device_attach(ndev);
        netif_tx_unlock(ndev);
#ifdef NGKNET_XDP_NATIVE
        if (((struct ngknet_private *)netdev_priv(ndev))->xsk_zc) {
            int qi;
            for (qi = 0; qi < pdev->ctrl.nb_txq; qi++) {
                ngknet_xsk_wakeup(ndev, qi, XDP_WAKEUP_TX);
            }
        }
#endif
    }
}

/*!
 * Suspend Tx queue callback
 */
static void
ngknet_tx_suspend(struct pdma_dev *pdev, int queue)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct net_device *ndev;
    unsigned long flags;
    int vdi;

    netif_stop_subqueue(dev->net_dev, queue);

    spin_lock_irqsave(&dev->lock, flags);
    for (vdi = 1; vdi <= NUM_VDEV_MAX; vdi++) {
        ndev = dev->vdev[vdi];
        if (!ndev) {
            continue;
        }
        netif_stop_subqueue(ndev, queue);
    }
    spin_unlock_irqrestore(&dev->lock, flags);
}

/*!
 * Resume Tx queue callback
 */
static void
ngknet_tx_resume(struct pdma_dev *pdev, int queue)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct net_device *ndev;
    unsigned long flags;
    static int start_index[NUM_PDMA_DEV_MAX];
    int vdi = start_index[pdev->unit];
    int vdn = 0;

    if (__netif_subqueue_stopped(dev->net_dev, queue)) {
        netif_wake_subqueue(dev->net_dev, queue);
    }

    spin_lock_irqsave(&dev->lock, flags);
    vdi = vdi > 0 ? vdi : NUM_VDEV_MAX;
    while (1) {
        ndev = dev->vdev[vdi];
        if (ndev) {
            if (__netif_subqueue_stopped(ndev, queue)) {
                netif_wake_subqueue(ndev, queue);
            }
#ifdef NGKNET_XDP_NATIVE
            if (((struct ngknet_private *)netdev_priv(ndev))->xsk_zc) {
                ngknet_xsk_wakeup(ndev, queue, XDP_WAKEUP_TX);
            }
#endif
            start_index[pdev->unit] = vdi;
        }

        /*
         * Exits if all devices are done and vdi indexes the last resumed device.
         * As such, every device has the same chance of being waken up first.
         */
        if (++vdn == NUM_VDEV_MAX) {
            break;
        }

        /* Increases vdi by 1 in a circular manner within the specified range */
        vdi = ++vdi > NUM_VDEV_MAX ? 1 : vdi;
    }
    spin_unlock_irqrestore(&dev->lock, flags);

    if (pdev->mode == DEV_MODE_HNET) {
        atomic_set(&dev->hnet_active, 1);
        wake_up_interruptible(&dev->hnet_wq);
    }
}

/*!
 * Enable interrupt callback
 */
static void
ngknet_intr_enable(struct pdma_dev *pdev, int cmc, int chan,
                   uint32_t reg, uint32_t val)
{
    if (val) {
        ngbde_kapi_iio_write32(pdev->unit, reg, val);
    } else {
        ngbde_kapi_intr_mask_write(pdev->unit, 0, reg, pdev->ctrl.grp[cmc].irq_mask);
    }
}

/*!
 * Disable interrupt callback
 */
static void
ngknet_intr_disable(struct pdma_dev *pdev, int cmc, int chan,
                    uint32_t reg, uint32_t val)
{
    if (val) {
        ngbde_kapi_iio_write32(pdev->unit, reg, val);
    } else {
        ngbde_kapi_intr_mask_write(pdev->unit, 0, reg, pdev->ctrl.grp[cmc].irq_mask);
    }
}

/*!
 * NAPI polling function
 */
static int
ngknet_poll(struct napi_struct *napi, int budget)
{
    struct ngknet_intr_handle *kih = (struct ngknet_intr_handle *)napi;
    struct intr_handle *hdl = kih->hdl;
    struct pdma_dev *pdev = (struct pdma_dev *)hdl->dev;
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    unsigned long flags;
    int work_done;

    DBG_NAPI(("Scheduled NAPI on queue %d.\n", hdl->queue));

    kih->napi_pending = 0;

    if (pdev->flags & PDMA_GROUP_INTR) {
        work_done = bcmcnet_group_poll(pdev, hdl->group, budget);
    } else {
        if (!kih->napi_resched) {
            bcmcnet_queue_intr_ack(pdev, hdl);
        }
        work_done = bcmcnet_queue_poll(pdev, hdl, budget);
    }

#ifdef NGKNET_XDP_NATIVE
    if (dev->xsk_pool && xsk_napi_tx) {
        work_done = max(ngknet_xsk_napi_tx(dev, hdl, budget), work_done);
    }
#endif

    if (work_done < budget) {
        kih->napi_resched = 0;
        napi_complete(napi);
        if (kih->napi_pending && napi_schedule_prep(napi)) {
            kih->napi_resched = 1;
            __napi_schedule(napi);
            return work_done;
        }
        spin_lock_irqsave(&dev->lock, flags);
        if (pdev->flags & PDMA_GROUP_INTR) {
            bcmcnet_group_intr_enable(pdev, hdl->group);
        } else {
            bcmcnet_queue_intr_enable(pdev, hdl);
        }
        spin_unlock_irqrestore(&dev->lock, flags);
    }

    return work_done;
}

/*!
 * NGKNET ISR
 */
static int
ngknet_isr(void *isr_data)
{
    struct ngknet_dev *dev = isr_data;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct intr_handle *hdl = NULL;
    struct napi_struct *napi = NULL;
    unsigned long bm_queue;
    unsigned long flags;
    int gi, qi;
    int iv = 0;

    for (gi = 0; gi < pdev->num_groups; gi++) {
        if (!pdev->ctrl.grp[gi].attached) {
            continue;
        }
        bm_queue = pdev->ctrl.grp[gi].bm_rxq | pdev->ctrl.grp[gi].bm_txq;
        for (qi = 0; qi < pdev->grp_queues; qi++) {
            if (!(pdev->flags & PDMA_GROUP_INTR) && !(1 << qi & bm_queue)) {
                continue;
            }
            hdl = &pdev->ctrl.grp[gi].intr_hdl[qi];
            if (pdev->flags & PDMA_GROUP_INTR) {
                if (!bcmcnet_group_intr_check(pdev, gi)) {
                    break;
                }
            } else {
                if (!bcmcnet_queue_intr_check(pdev, hdl)) {
                    continue;
                }
            }
            spin_lock_irqsave(&dev->lock, flags);
            if (pdev->flags & PDMA_GROUP_INTR) {
                bcmcnet_group_intr_disable(pdev, gi);
            } else {
                bcmcnet_queue_intr_disable(pdev, hdl);
            }
            spin_unlock_irqrestore(&dev->lock, flags);
            napi = (struct napi_struct *)hdl->priv;
            if (likely(napi_schedule_prep(napi))) {
                __napi_schedule(napi);
            }
            iv++;
            if (pdev->flags & PDMA_GROUP_INTR) {
                break;
            }
        }
    }

    if (iv) {
        DBG_IRQ(("Got interrupt on device %d.\n", dev->dev_info.dev_no));
        pdev->stats.intrs++;
        return IRQ_HANDLED;
    } else {
        return IRQ_NONE;
    }
}

/*!
 * Hypervisor network work handler
 */
static void
ngknet_dev_hnet_work(struct pdma_dev *pdev)
{
    struct intr_handle *hdl = NULL;
    struct napi_struct *napi = NULL;
    struct ngknet_intr_handle *kih = NULL;
    unsigned long bm_queue;
    int gi, qi;

    for (gi = 0; gi < pdev->num_groups; gi++) {
        if (!pdev->ctrl.grp[gi].attached) {
            continue;
        }
        bm_queue = pdev->ctrl.grp[gi].bm_rxq | pdev->ctrl.grp[gi].bm_txq;
        for (qi = 0; qi < pdev->grp_queues; qi++) {
            if (!(pdev->flags & PDMA_GROUP_INTR) && !(1 << qi & bm_queue)) {
                continue;
            }
            hdl = &pdev->ctrl.grp[gi].intr_hdl[qi];
            napi = (struct napi_struct *)hdl->priv;
            kih = (struct ngknet_intr_handle *)napi;
            kih->napi_pending = 1;
            if (napi_schedule_prep(napi)) {
                kih->napi_resched = 1;
                local_bh_disable();
                __napi_schedule(napi);
                local_bh_enable();
            }
            if (pdev->flags & PDMA_GROUP_INTR) {
                break;
            }
        }
    }
}

/*!
 * Hypervisor network wait handler
 */
static int
ngknet_dev_hnet_wait(struct pdma_dev *pdev)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    uint32_t bmp;
    int budget, qi;

    while (!kthread_should_stop()) {
        wait_event_interruptible(dev->hnet_wq,
                                 atomic_read(&dev->hnet_active) != 0);
        if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
            schedule_timeout(HZ);
            continue;
        }
        atomic_set(&dev->hnet_active, 0);

        schedule_work(&dev->hnet_work);

        do {
            bmp = 0x0;
            for (qi = 0; qi < pdev->ctrl.nb_txq; qi++) {
                bmp |= 1 << qi;
                budget = pdev->ctrl.budget;
                while (budget--) {
                    if (SHR_FAILURE(pdev->pkt_xmit(pdev, qi, 0))) {
                        bmp &= ~(1 << qi);
                        break;
                    }
                }
            }
        } while (bmp);
    }

    return 0;
}

/*!
 * Hypervisor network wake handler
 */
static int
ngknet_dev_vnet_wake(struct pdma_dev *pdev)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;

    if (atomic_read(&dev->vnet_active) != 1) {
        atomic_set(&dev->vnet_active, 1);
        wake_up_interruptible(&dev->vnet_wq);
    }

    return SHR_E_NONE;
}

/*!
 * Hypervisor network process
 */
static int
ngknet_dev_hnet_process(void *data)
{
    return ngknet_dev_hnet_wait((struct pdma_dev *)data);
}

/*!
 * Hypervisor network schedule
 */
static void
ngknet_dev_hnet_schedule(struct work_struct *work)
{
    struct ngknet_dev *dev = container_of(work, struct ngknet_dev, hnet_work);

    ngknet_dev_hnet_work(&dev->pdma_dev);
}

/*!
 * Convert physical address to virtual address
 */
static void *
ngknet_sys_p2v(struct pdma_dev *pdev, uint64_t paddr)
{
    return ngbde_kapi_dma_bus_to_virt(pdev->unit, (dma_addr_t)paddr);
}

/*!
 * Convert virtual address to physical address
 */
static uint64_t
ngknet_sys_v2p(struct pdma_dev *pdev, void *vaddr)
{
    return (uint64_t)ngbde_kapi_dma_virt_to_bus(pdev->unit, vaddr);
}

/*!
 * Open network device
 */
static int
ngknet_enet_open(struct net_device *ndev)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct napi_struct *napi = NULL;
    unsigned long bm_queue;
    int gi, qi;
    int rv;

    if (!pdev->ctrl.bm_rxq || !pdev->ctrl.bm_txq) {
        printk("Not config Rx or Tx queue yet!\n");
        return -EPERM;
    }

    if (priv->netif.id <= 0) {
        /* Register interrupt handler */
        ngbde_kapi_intr_connect(dev->dev_info.dev_no, 0, ngknet_isr, dev);

        /* Start PDMA device */
        rv = bcmcnet_pdma_dev_start(pdev);
        if (SHR_FAILURE(rv)) {
            ngbde_kapi_intr_disconnect(dev->dev_info.dev_no, 0);
            return -EPERM;
        }

        /* Start rate limit */
        if (rx_rate_limit >= 0) {
            ngknet_rx_rate_limit_start(dev);
        }

        /* Notify the stack of the actual queue counts. */
        rv = netif_set_real_num_rx_queues(dev->net_dev, pdev->ctrl.nb_rxq);
        if (rv < 0) {
            ngbde_kapi_intr_disconnect(dev->dev_info.dev_no, 0);
            return rv;
        }
        rv = netif_set_real_num_tx_queues(dev->net_dev, pdev->ctrl.nb_txq);
        if (rv < 0) {
            ngbde_kapi_intr_disconnect(dev->dev_info.dev_no, 0);
            return rv;
        }

        for (gi = 0; gi < pdev->num_groups; gi++) {
            if (!pdev->ctrl.grp[gi].attached) {
                continue;
            }
            bm_queue = pdev->ctrl.grp[gi].bm_rxq | pdev->ctrl.grp[gi].bm_txq;
            for (qi = 0; qi < pdev->grp_queues; qi++) {
                napi = (struct napi_struct *)pdev->ctrl.grp[gi].intr_hdl[qi].priv;
                if (pdev->flags & PDMA_GROUP_INTR) {
                    napi_enable(napi);
                    break;
                }
                if (1 << qi & bm_queue) {
                    napi_enable(napi);
                }
            }
        }
    } else {
        /* Notify the stack of the actual queue counts. */
        rv = netif_set_real_num_rx_queues(ndev, pdev->ctrl.nb_rxq);
        if (rv < 0) {
            return rv;
        }
        rv = netif_set_real_num_tx_queues(ndev, pdev->ctrl.nb_txq);
        if (rv < 0) {
            return rv;
        }
    }

    /* Prevent tx timeout */
    kal_netif_trans_update(ndev);

    netif_tx_wake_all_queues(ndev);

    return 0;
}

/*!
 * Stop network device
 */
static int
ngknet_enet_stop(struct net_device *ndev)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct napi_struct *napi = NULL;
    unsigned long bm_queue;
    int gi, qi;

    netif_tx_stop_all_queues(ndev);

    if (priv->netif.id <= 0) {
        /* Stop rate limit */
        if (rx_rate_limit >= 0) {
            ngknet_rx_rate_limit_stop(dev);
        }

        for (gi = 0; gi < pdev->num_groups; gi++) {
            if (!pdev->ctrl.grp[gi].attached) {
                continue;
            }
            bm_queue = pdev->ctrl.grp[gi].bm_rxq | pdev->ctrl.grp[gi].bm_txq;
            for (qi = 0; qi < pdev->grp_queues; qi++) {
                napi = (struct napi_struct *)pdev->ctrl.grp[gi].intr_hdl[qi].priv;
                if (pdev->flags & PDMA_GROUP_INTR) {
                    napi_disable(napi);
                    break;
                }
                if (1 << qi & bm_queue) {
                    napi_disable(napi);
                }
            }
        }

        /* Stop PDMA device */
        bcmcnet_pdma_dev_stop(pdev);

        /* Unregister interrupt handler */
        ngbde_kapi_intr_disconnect(dev->dev_info.dev_no, 0);
    }

    return 0;
}

/*!
 * Start transmission
 */
static int
ngknet_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct sk_buff *bskb = skb;
    uint32_t len = skb->len;
    int queue;
    int rv;

    DBG_VERB(("Tx packet from ndev%d (%d bytes).\n", priv->netif.id, skb->len));
    if (debug & DBG_LVL_PDMP) {
        ngknet_pkt_dump(skb->data, skb->len);
    }

    /* Do not transmit on base device */
    if (priv->netif.id <= 0) {
        priv->stats.tx_dropped++;
        dev_kfree_skb_any(skb);
        return NETDEV_TX_OK;
    }

    /* Measure speed */
    if (debug & DBG_LVL_RATE) {
        ngknet_pkt_stats(pdev, PDMA_Q_TX);
    }

    queue = skb->queue_mapping;

    /* Handle one outgoing packet */
    rv = ngknet_tx_frame_process(ndev, &skb);
    if (SHR_FAILURE(rv)) {
        priv->stats.tx_dropped++;
        if (skb) {
            dev_kfree_skb_any(skb);
        }
        return NETDEV_TX_OK;
    }

    /* Schedule Tx queue */
    ngknet_tx_queue_schedule(dev, (struct pkt_buf *)skb->data, &queue);
    skb->queue_mapping = queue;

    DBG_VERB(("Tx packet (%d bytes).\n", skb->len));
    if (debug & DBG_LVL_PDMP) {
        ngknet_pkt_dump(skb->data, skb->len);
    }

    /* Do Tx timestamping */
    if (skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) {
        ngknet_ptp_tx_config(ndev, skb);
    }

    skb_tx_timestamp(skb);

    rv = pdev->pkt_xmit(pdev, queue, &skb->data);

    if (rv == SHR_E_BUSY) {
        DBG_WARN(("Tx suspend: DMA device is busy and temporarily "
                  "unavailable.\n"));
        priv->stats.tx_fifo_errors++;
        if (skb != bskb) {
            dev_kfree_skb_any(skb);
        }
        return NETDEV_TX_BUSY;
    } else if (rv != SHR_E_NONE) {
        DBG_WARN(("Tx drop: DMA device not ready or not supported.\n"));
        priv->stats.tx_dropped++;
        if (skb != bskb) {
            dev_kfree_skb_any(skb);
        }
        dev_kfree_skb_any(bskb);
        return NETDEV_TX_OK;
    } else {
        if (skb != bskb) {
            dev_kfree_skb_any(bskb);
        }
    }

    /* Update accounting */
    priv->stats.tx_packets++;
    priv->stats.tx_bytes += len;

    return NETDEV_TX_OK;
}

/*!
 * Get network device stats
 */
static struct net_device_stats *
ngknet_get_stats(struct net_device *ndev)
{
    struct ngknet_private *priv = netdev_priv(ndev);

    return &priv->stats;
}

/*!
 * Set network device MC list
 */
static void
ngknet_set_multicast_list(struct net_device *ndev)
{
    return;
}

/*!
 * Set network device MAC address
 */
static int
ngknet_set_mac_address(struct net_device *ndev, void *addr)
{
    if (!is_valid_ether_addr(((struct sockaddr *)addr)->sa_data)) {
        return -EINVAL;
    }

    netdev_info(ndev, "Setting new MAC address\n");
    eth_hw_addr_set(ndev, ((struct sockaddr *)addr)->sa_data);

    return 0;
}

/*!
 * Change network device MTU
 */
static int
ngknet_change_mtu(struct net_device *ndev, int new_mtu)
{
    int frame_size = new_mtu + ETH_HLEN + VLAN_HLEN + ETH_FCS_LEN;

    if (frame_size < (ETH_ZLEN + ETH_FCS_LEN) || frame_size > rx_buffer_size) {
        return -EINVAL;
    }

    netdev_info(ndev, "Changing MTU from %d to %d\n", ndev->mtu, new_mtu);
    ndev->mtu = new_mtu;

    return 0;
}

/*!
 * Do I/O control
 */
static int
ngknet_do_ioctl(struct net_device *ndev, struct ifreq *ifr, int cmd)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct hwtstamp_config config;
    int rv;

#if SAI_FIXUP && KNET_SVTAG_HOTFIX /* SONIC-76482 */
    if (cmd == NGKNET_IOC_SVTAG_SET) {
        struct ifru_svtag req;
        if (copy_from_user(&req, ifr->ifr_data, sizeof(req)))
            return -EFAULT;
        if (ntohl(req.magic) != NGKNET_IOC_SVTAG_MAGIC)
            return -EINVAL;
        priv->netif.flags &= ~(NGKNET_NETIF_F_ADD_SVTAG | NGKNET_NETIF_F_DEL_SVTAG);
        priv->netif.flags |= req.flags & (NGKNET_NETIF_F_ADD_SVTAG | NGKNET_NETIF_F_DEL_SVTAG);
        memcpy(priv->svtag, req.svtag, 4);
        return 0;
    } else
#endif
    if (cmd == SIOCSHWTSTAMP) {
        if (copy_from_user(&config, ifr->ifr_data, sizeof(config))) {
            return -EFAULT;
        }

        if (priv->netif.type != NGKNET_NETIF_T_PORT &&
            priv->netif.type != NGKNET_NETIF_T_META) {
            return -ENOSYS;
        }

        switch (config.tx_type) {
        case HWTSTAMP_TX_OFF:
            priv->hwts_tx_type = HWTSTAMP_TX_OFF;
            rv = ngknet_ptp_tx_config_set(ndev, priv->hwts_tx_type);
            if (SHR_FAILURE(rv)) {
                return -ENOSYS;
            }
            break;
        case HWTSTAMP_TX_ON:
            priv->hwts_tx_type = HWTSTAMP_TX_ON;
            rv = ngknet_ptp_tx_config_set(ndev, priv->hwts_tx_type);
            if (SHR_FAILURE(rv)) {
                return -ENOSYS;
            }
            break;
        case HWTSTAMP_TX_ONESTEP_SYNC:
            priv->hwts_tx_type = HWTSTAMP_TX_ONESTEP_SYNC;
            rv = ngknet_ptp_tx_config_set(ndev, priv->hwts_tx_type);
            if (SHR_FAILURE(rv)) {
                return -ENOSYS;
            }
            break;
        default:
            return -ERANGE;
        }

        switch (config.rx_filter) {
        case HWTSTAMP_FILTER_NONE:
            rv = ngknet_ptp_rx_config_set(ndev, &config.rx_filter);
            if (SHR_FAILURE(rv)) {
                return -ENOSYS;
            }
            priv->hwts_rx_filter = HWTSTAMP_FILTER_NONE;
            break;
        default:
            rv = ngknet_ptp_rx_config_set(ndev, &config.rx_filter);
            if (SHR_FAILURE(rv)) {
                return -ENOSYS;
            }
            priv->hwts_rx_filter = config.rx_filter;
            break;
        }

        return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
    if (cmd == SIOCGHWTSTAMP) {
        config.flags = 0;
        config.tx_type = priv->hwts_tx_type;
        config.rx_filter = priv->hwts_rx_filter;

        return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
    }
#endif

    return -EINVAL;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*!
 * Poll network device
 */
static void
ngknet_poll_controller(struct net_device *ndev)
{
    struct ngknet_private *priv = netdev_priv(ndev);

    disable_irq(ndev->irq);
    ngknet_isr(priv->bkn_dev);
    enable_irq(ndev->irq);
}
#endif

static const struct net_device_ops ngknet_netdev_ops = {
    .ndo_open            = ngknet_enet_open,
    .ndo_stop            = ngknet_enet_stop,
    .ndo_start_xmit      = ngknet_start_xmit,
    .ndo_get_stats       = ngknet_get_stats,
    .ndo_validate_addr   = eth_validate_addr,
    .ndo_set_rx_mode     = ngknet_set_multicast_list,
    .ndo_set_mac_address = ngknet_set_mac_address,
    .ndo_change_mtu      = ngknet_change_mtu,
    .ndo_set_features    = NULL,
    .ndo_do_ioctl        = ngknet_do_ioctl,
    .ndo_tx_timeout      = NULL,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = ngknet_poll_controller,
#endif
#ifdef NGKNET_XDP_NATIVE
    .ndo_bpf             = ngknet_xdp_setup,
    .ndo_xdp_xmit        = ngknet_xdp_xmit,
    .ndo_xsk_wakeup      = ngknet_xsk_wakeup,
#endif
};

static void
ngknet_get_drvinfo(struct net_device *ndev, struct ethtool_drvinfo *drvinfo)
{
    strscpy(drvinfo->driver, "linux_ngknet", sizeof(drvinfo->driver));
    snprintf(drvinfo->version, sizeof(drvinfo->version), "%d", NGKNET_IOC_VERSION);
    strscpy(drvinfo->fw_version, "N/A", sizeof(drvinfo->fw_version));
    strscpy(drvinfo->bus_info, "N/A", sizeof(drvinfo->bus_info));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
static int
ngknet_get_ts_info(struct net_device *ndev, struct kernel_ethtool_ts_info *info)
{
    int rv;

    info->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
                            SOF_TIMESTAMPING_TX_SOFTWARE |
                            SOF_TIMESTAMPING_RX_HARDWARE |
                            SOF_TIMESTAMPING_RX_SOFTWARE |
                            SOF_TIMESTAMPING_SOFTWARE |
                            SOF_TIMESTAMPING_RAW_HARDWARE;
    info->tx_types =
        (1 << HWTSTAMP_TX_OFF) |
        (1 << HWTSTAMP_TX_ON) |
        (1 << HWTSTAMP_TX_ONESTEP_SYNC);
    info->rx_filters =
        (1 << HWTSTAMP_FILTER_NONE) |
        (1 << HWTSTAMP_FILTER_ALL);
    rv = ngknet_ptp_phc_index_get(ndev, &info->phc_index);
    if (SHR_FAILURE(rv)) {
        info->phc_index = -1;
    }

    return 0;
}
#endif

#if NGKNET_ETHTOOL_LINK_SETTINGS
static int
ngknet_get_link_ksettings(struct net_device *ndev,
                          struct ethtool_link_ksettings *cmd)
{
    struct ngknet_private *priv = netdev_priv(ndev);

    cmd->base.speed = priv->link_settings.speed;
    cmd->base.duplex = priv->link_settings.duplex;

    return 0;
}

static int
ngknet_set_link_ksettings(struct net_device *ndev,
                          const struct ethtool_link_ksettings *cmd)
{
    struct ngknet_private *priv = netdev_priv(ndev);

    priv->link_settings.speed = cmd->base.speed;
    priv->link_settings.duplex = cmd->base.speed ? DUPLEX_FULL : 0;

    return 0;
}
#endif

static const struct ethtool_ops ngknet_ethtool_ops = {
    .get_drvinfo        = ngknet_get_drvinfo,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
    .get_ts_info        = ngknet_get_ts_info,
#endif
#if NGKNET_ETHTOOL_LINK_SETTINGS
    .get_link_ksettings = ngknet_get_link_ksettings,
    .set_link_ksettings = ngknet_set_link_ksettings,
#endif
};

/*!
 * \brief Initialize network device.
 *
 * \param [in] name Network device name.
 * \param [in] mac Network device MAC address.
 * \param [out] nd New registered network device.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_ndev_init(ngknet_netif_t *netif, struct net_device **nd)
{
    struct net_device *ndev = NULL;
    uint8_t *ma;
    int rv;

    if (!netif) {
        DBG_WARN(("Network interface is NULL.\n"));
        return SHR_E_PARAM;
    }
    if (!nd) {
        DBG_WARN(("Network device is NULL.\n"));
        return SHR_E_PARAM;
    }

    ndev = alloc_etherdev_mq(sizeof(struct ngknet_private), NUM_Q_MAX);
    if (!ndev) {
        DBG_WARN(("Error allocating network device.\n"));
        return SHR_E_MEMORY;
    }
    if (!ndev->dev_addr) {
        DBG_WARN(("ndev->dev_addr is NULL\n"));
        free_netdev(ndev);
        return SHR_E_INTERNAL;
    }

    /* Device information -- not available right now */
    ndev->irq = 0;
    ndev->base_addr = 0;

    /* Fill in the dev structure */
    ndev->watchdog_timeo = 5 * HZ;

    /* Default MTU should not exceed MTU of switch front-panel ports */
    ndev->mtu = netif->mtu;
    if (!ndev->mtu) {
        ndev->mtu = default_mtu ? default_mtu : rx_buffer_size;
    }

    /* MTU range: 32 - 9198 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
    ndev->min_mtu = PKT_HDR_SIZE; /* Min 50-byte length of packet with RCPU-encap */
    ndev->max_mtu = rx_buffer_size - (ETH_HLEN + ETH_FCS_LEN);
#endif

    ndev->netdev_ops = &ngknet_netdev_ops;
    ndev->ethtool_ops = &ngknet_ethtool_ops;

    /* Network device name */
    if (netif->name[0] != '\0') {
        strncpy(ndev->name, netif->name, IFNAMSIZ - 1);
    }

    /* Set the device MAC address */
    ma = netif->macaddr;
    if ((ma[0] | ma[1] | ma[2] | ma[3] | ma[4] | ma[5]) == 0) {
        ngknet_dev_mac[5]++;
        if (ngknet_dev_mac[5] == 0) {
            ngknet_dev_mac[4]++;
        }
        ma = ngknet_dev_mac;
    }
    eth_hw_addr_set(ndev, ma);

    /* Initialize the device features */
    ndev->hw_features = NETIF_F_RXCSUM |
                        NETIF_F_HW_VLAN_CTAG_RX |
                        NETIF_F_HW_VLAN_CTAG_TX;
    ndev->features = NETIF_F_RXCSUM |
                     NETIF_F_HIGHDMA |
                     NETIF_F_HW_VLAN_CTAG_RX;

#ifdef NGKNET_XDP_NATIVE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0))
    ndev->xdp_features = NETDEV_XDP_ACT_BASIC |
                         NETDEV_XDP_ACT_REDIRECT |
                         NETDEV_XDP_ACT_NDO_XMIT |
                         NETDEV_XDP_ACT_XSK_ZEROCOPY;
#endif
#endif

    dev_net_set(ndev, current->nsproxy->net_ns);
    /* Register the kernel network device */
    rv = register_netdev(ndev);
    if (rv < 0) {
        DBG_WARN(("Error registering network device %s.\n", ndev->name));
        free_netdev(ndev);
        return SHR_E_FAIL;
    }

    *nd = ndev;

    DBG_VERB(("Created network device %s.\n", ndev->name));

    return SHR_E_NONE;
}

static int
ngknet_dev_remove(int dn);

static int
ngknet_bde_event_handler(int kdev, int event, void *data)
{
    DBG_VERB(("%s: callback from BDE with kdev(%d) event(%d).\n",
              __FUNCTION__, kdev, event));

    if (event == NGBDE_EVENT_DEV_REMOVE) {
        ngknet_dev_remove(kdev);
    }

    return SHR_E_NONE;
}

/*!
 * \brief Initialize Packet DMA device.
 *
 * \param [in] dev NGKNET device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_pdev_init(struct ngknet_dev *dev)
{
    struct pdma_dev *pdev = &dev->pdma_dev;
    int rv;

    /* Initialize PDMA control structure */
    pdev->unit = dev->dev_info.dev_no;
    pdev->priv = dev;
    pdev->ctrl.dev = pdev;
    pdev->ctrl.hw_addr = dev->base_addr;
    pdev->ctrl.rx_buf_size = rx_buffer_size;

    /* Hook callbacks */
    pdev->dev_read32 = ngknet_dev_read32;
    pdev->dev_write32 = ngknet_dev_write32;
    pdev->pkt_recv = ngknet_frame_recv;
    pdev->ndev_detach = ngknet_ndev_detach;
    pdev->ndev_attach = ngknet_ndev_attach;
    pdev->tx_suspend = ngknet_tx_suspend;
    pdev->tx_resume = ngknet_tx_resume;
    pdev->intr_unmask = ngknet_intr_enable;
    pdev->intr_mask = ngknet_intr_disable;
    pdev->xnet_wait = ngknet_dev_hnet_wait;
    pdev->xnet_wake = ngknet_dev_vnet_wake;
    pdev->sys_p2v = ngknet_sys_p2v;
    pdev->sys_v2p = ngknet_sys_v2p;

    if (tx_polling) {
        pdev->flags |= PDMA_TX_POLLING;
    }
    if (rx_batching || pdev->mode == DEV_MODE_HNET) {
        pdev->flags |= PDMA_RX_BATCHING;
    }

    /* Attach PDMA driver */
    rv = drv_ops[pdev->dev_type]->drv_attach(pdev);
    if (SHR_FAILURE(rv)) {
        DBG_WARN(("Attach DMA driver failed.\n"));
        return rv;
    }

    /* Initialize PDMA device */
    rv = bcmcnet_pdma_dev_init(pdev);
    if (SHR_FAILURE(rv)) {
        DBG_WARN(("Init DMA device.failed.\n"));
        return rv;
    }

    DBG_VERB(("Attached DMA device %s.\n", pdev->name));

    return SHR_E_NONE;
}

/*!
 * \brief Get device information from BDE.
 *
 * \param [in] dn Device number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_dev_info_get(int dn)
{
    struct ngknet_dev *dev = &ngknet_devices[dn];

    dev->base_addr = ngbde_kapi_pio_membase(dn);
    dev->dev = ngbde_kapi_dma_dev_get(dn);

    if (!dev->base_addr || !dev->dev) {
        return SHR_E_ACCESS;
    }

    dev->dev_info.dev_no = dn;
    strscpy(dev->dev_info.type_str, drv_ops[dev->pdma_dev.dev_type]->drv_desc,
            sizeof(dev->dev_info.type_str));
    dev->dev_info.vdev = dev->vdev;
    return SHR_E_NONE;
}

/*!
 * \brief Probe device.
 *
 * Get the information from BDE, initialize Packet DMA device,
 * initialize base network device and allocate other resources.
 *
 * \param [in] dn Device number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_dev_probe(int dn, ngknet_netif_t *netif)
{
    struct ngknet_dev *dev = &ngknet_devices[dn];
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct net_device *ndev = NULL;
    struct ngknet_private *priv = NULL;
    struct intr_handle *hdl = NULL;
    struct cpumask mask;
    int gi, qi;
    int rv;

    DBG_VERB(("%s: dev %d\n",__FUNCTION__, dn));

    /* Get device information */
    rv = ngknet_dev_info_get(dn);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    /* Initialize PDMA device */
    rv = ngknet_pdev_init(dev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    /* Get base network device name */
    if (netif->name[0] == '\0') {
        /* Reserve 6 vacancies for base&vitual device number, i.e. nameAB_XYZ */
        if (strlen(base_dev_name) < IFNAMSIZ - 6) {
            snprintf(netif->name, IFNAMSIZ, "%s%d", base_dev_name, dn);
        } else {
            DBG_WARN(("Too long network device name: %s.\n", base_dev_name));
            return SHR_E_PARAM;
        }
    }

    if (netif->chan >= NUM_Q_MAX) {
        DBG_WARN(("Exceed max number of queues : %d.\n", netif->chan));
        return SHR_E_PARAM;
    }

    rv = ngknet_ndev_init(netif, &ndev);
    if (SHR_FAILURE(rv)) {
        bcmcnet_pdma_dev_cleanup(pdev);
        return rv;
    }
    dev->net_dev = ndev;

    /* Initialize private information for base network device */
    priv = netdev_priv(ndev);
    priv->net_dev = ndev;
    priv->bkn_dev = dev;
    priv->pkt_recv = ngknet_pkt_recv;

    netif->id = 0;
    memcpy(netif->macaddr, ndev->dev_addr, ETH_ALEN);
    netif->mtu = ndev->mtu;
    memcpy(netif->name, ndev->name, sizeof(netif->name) - 1);
    memcpy(&priv->netif, netif, sizeof(priv->netif));

    if (priv->netif.flags & NGKNET_NETIF_F_BIND_CHAN) {
        dev->bdev[priv->netif.chan] = ndev;
    }

    /* Register for napi */
    for (gi = 0; gi < pdev->num_groups; gi++) {
        if (!pdev->ctrl.grp[gi].attached) {
            continue;
        }
        for (qi = 0; qi < pdev->grp_queues; qi++) {
            hdl = &pdev->ctrl.grp[gi].intr_hdl[qi];
            priv_hdl[hdl->unit][hdl->chan].hdl = hdl;
            hdl->priv = &priv_hdl[hdl->unit][hdl->chan];
            kal_netif_napi_add(ndev, (struct napi_struct *)hdl->priv,
                               ngknet_poll, pdev->ctrl.budget);
            if (pdev->flags & PDMA_GROUP_INTR) {
                break;
            }
        }
    }

    /* Get callback control */
    ngknet_callback_control_get(&dev->cbc);

    INIT_LIST_HEAD(&dev->filt_list);
    spin_lock_init(&dev->lock);
    init_waitqueue_head(&dev->wq);
    if (pdev->mode == DEV_MODE_HNET) {
        init_waitqueue_head(&dev->vnet_wq);
        atomic_set(&dev->vnet_active, 0);
        init_waitqueue_head(&dev->hnet_wq);
        atomic_set(&dev->hnet_active, 0);
        dev->hnet_task = kthread_run(ngknet_dev_hnet_process, pdev, pdev->name);
        if (IS_ERR(dev->hnet_task)) {
            dev->hnet_task = NULL;
            return SHR_E_INTERNAL;
        }
        cpumask_clear(&mask);
        cpumask_set_cpu(num_online_cpus() / 2, &mask);
        set_cpus_allowed_ptr(dev->hnet_task, &mask);
        INIT_WORK(&dev->hnet_work, ngknet_dev_hnet_schedule);
    }

    skb_queue_head_init(&dev->ptp_tx_queue);
    INIT_WORK(&dev->ptp_tx_work, ngknet_ptp_tx_work);

    dev->link_wq = create_workqueue("ngknet");
    if (!dev->link_wq) {
        return SHR_E_INTERNAL;
    }

    dev->flags |= NGKNET_DEV_ACTIVE;

    DBG_NDEV(("Broadcom NGKNET Attached\n"));
    DBG_NDEV(("MAC: %pM\n", ndev->dev_addr));
    DBG_NDEV(("Running with NAPI enabled\n"));

    /* Register handler for BDE events. */
    ngbde_kapi_knet_connect(dn, ngknet_bde_event_handler, dev);

    return SHR_E_NONE;
}

/*!
 * \brief Remove device.
 *
 * Suspend device firstly, destroy all virtual network devices
 * and filters, clean up Packet DMA device.
 *
 * \param [in] dn Device number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
ngknet_dev_remove(int dn)
{
    struct ngknet_dev *dev = &ngknet_devices[dn];
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct net_device *ndev = NULL;
    struct intr_handle *hdl = NULL;
    int di, gi, qi;
    int rv;

    if (!(dev->flags & NGKNET_DEV_ACTIVE)) {
        ngbde_kapi_knet_disconnect(dn);
        return SHR_E_NONE;
    }

    DBG_VERB(("%s: dev %d\n",__FUNCTION__, dn));

    dev->flags &= ~NGKNET_DEV_ACTIVE;

    if (dev->link_wq) {
        flush_workqueue(dev->link_wq);
        destroy_workqueue(dev->link_wq);
    }

    skb_queue_purge(&dev->ptp_tx_queue);

    if (pdev->mode == DEV_MODE_HNET && dev->hnet_task) {
        atomic_set(&dev->hnet_active, 1);
        wake_up_interruptible(&dev->hnet_wq);
        kthread_stop(dev->hnet_task);
        dev->hnet_task = NULL;
    }

    /* Destroy all the filters */
    ngknet_filter_destroy_all(dev);

    /* Destroy all the virtual devices */
    for (di = 1; di <= NUM_VDEV_MAX; di++) {
        ndev = dev->vdev[di];
        if (ndev) {
            unregister_netdev(ndev);
            free_netdev(ndev);
            dev->vdev[di] = NULL;
        }
    }
    dev->vdev[0] = NULL;

    DBG_VERB(("Removing base network device %s.\n", dev->net_dev->name));

    /* Destroy the base network device */
    ndev = dev->net_dev;
    unregister_netdev(ndev);
    free_netdev(ndev);

    for (qi = 0; qi < NUM_Q_MAX; qi++) {
        dev->bdev[qi] = NULL;
    }

    for (gi = 0; gi < pdev->num_groups; gi++) {
        if (!pdev->ctrl.grp[gi].attached) {
            continue;
        }
        for (qi = 0; qi < pdev->grp_queues; qi++) {
            hdl = &pdev->ctrl.grp[gi].intr_hdl[qi];
            netif_napi_del((struct napi_struct *)hdl->priv);
            priv_hdl[hdl->unit][hdl->chan].hdl = NULL;
            if (pdev->flags & PDMA_GROUP_INTR) {
                break;
            }
        }
    }

    /* Clean up PDMA device */
    bcmcnet_pdma_dev_cleanup(pdev);

    /* Detach PDMA driver */
    rv = drv_ops[pdev->dev_type]->drv_detach(pdev);
    if (SHR_FAILURE(rv)) {
        DBG_WARN(("Detach DMA driver failed.\n"));
    }
    ngbde_kapi_knet_disconnect(dn);

    return rv;
}

static void
ngknet_netif_link_process(struct work_struct *work)
{
    struct ngknet_private *priv = container_of(work, struct ngknet_private,
                                               link_work);
    struct net_device *ndev = priv->net_dev;

    if (netif_carrier_ok(ndev)) {
        netif_carrier_off(ndev);
        netif_tx_stop_all_queues(ndev);
    } else {
        netif_carrier_on(ndev);
        netif_tx_wake_all_queues(ndev);
    }
}

/*!
 * Network interface functions
 */

int
ngknet_netif_create(struct ngknet_dev *dev, ngknet_netif_t *netif)
{
    struct net_device *ndev = NULL;
    struct ngknet_private *priv = NULL;
    unsigned long flags;
    uint16_t id, num;
    int rv;
    struct list_head *list;
    netif_cb_t *netif_create_cb;

    switch (netif->type) {
    case NGKNET_NETIF_T_VLAN:
    case NGKNET_NETIF_T_PORT:
    case NGKNET_NETIF_T_META:
        break;
    default:
        return SHR_E_UNAVAIL;
    }

    /* Get vitual network device name */
    if (netif->name[0] == '\0') {
        /* Reserve 6 vacancies for base&vitual device number, i.e. nameAB_XYZ */
        if (strlen(base_dev_name) < IFNAMSIZ - 6) {
            snprintf(netif->name, IFNAMSIZ, "%s%d%s",
                     base_dev_name, dev->dev_info.dev_no, "_");
            strncat(netif->name, "%d", 3);
        } else {
            DBG_WARN(("Too long network device name: %s.\n", base_dev_name));
            return SHR_E_PARAM;
        }
    }

    if (netif->chan >= NUM_Q_MAX) {
        DBG_WARN(("Exceed max number of queues : %d.\n", netif->chan));
        return SHR_E_PARAM;
    }

    rv = ngknet_ndev_init(netif, &ndev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    spin_lock_irqsave(&dev->lock, flags);

    num = (long)dev->vdev[0];
    id = netif->id;
    if (netif->flags & NGKNET_NETIF_F_WITH_ID) {
        if (id == 0 || id > NUM_VDEV_MAX) {
            rv = SHR_E_PARAM;
        } else {
            /* ID assignment is specifed by user. */
            if (dev->vdev[id]) {
                DBG_WARN(("ID %d is already in use\n", id));
                rv = SHR_E_BUSY;
            }
        }
    } else {
        /* Automatic ID assignment. */
        for (id = 1; id < num + 1; id++) {
            if (!dev->vdev[id]) {
                break;
            }
        }
        if (id > NUM_VDEV_MAX) {
            rv = SHR_E_RESOURCE;
        }
    }
    if (SHR_FAILURE(rv)) {
        spin_unlock_irqrestore(&dev->lock, flags);
        unregister_netdev(ndev);
        free_netdev(ndev);
        return rv;
    }

    dev->vdev[id] = ndev;
    if (id > num) {
        num = id;
    }
    dev->vdev[0] = (struct net_device *)(long)num;

    spin_unlock_irqrestore(&dev->lock, flags);

    priv = netdev_priv(ndev);
    priv->net_dev = ndev;
    priv->bkn_dev = dev;
    priv->pkt_recv = ngknet_pkt_recv;

    netif->id = id;
    memcpy(netif->macaddr, ndev->dev_addr, ETH_ALEN);
    netif->mtu = ndev->mtu;
    memcpy(netif->name, ndev->name, sizeof(netif->name) - 1);
    memcpy(&priv->netif, netif, sizeof(priv->netif));

    if (priv->netif.flags & NGKNET_NETIF_F_BIND_CHAN) {
        dev->bdev[priv->netif.chan] = ndev;
    }

    /* Optional netif create callback handle */
    list_for_each(list, &dev->cbc->netif_create_cb_list) {
        netif_create_cb = list_entry(list, netif_cb_t, list);
        if (netif_create_cb->cb(&dev->dev_info, &priv->netif)) {
            DBG_WARN(("Network interface callback (create) failed for '%s'\n",
                      ndev->name));
        }
    }

    INIT_WORK(&priv->link_work, ngknet_netif_link_process);

    DBG_VERB(("Created virtual network device %s (%d).\n",
              ndev->name, priv->netif.id));

    return SHR_E_NONE;
}

int
ngknet_netif_destroy(struct ngknet_dev *dev, int id)
{
    struct net_device *ndev = NULL;
    struct ngknet_private *priv = NULL;
    unsigned long flags;
    int num;
    struct list_head *list;
    netif_cb_t *netif_destroy_cb;
    DECLARE_WAITQUEUE(wait, current);

    if (id <= 0 || id > NUM_VDEV_MAX) {
        return SHR_E_PARAM;
    }

    spin_lock_irqsave(&dev->lock, flags);

    ndev = dev->vdev[id];
    if (!ndev) {
        spin_unlock_irqrestore(&dev->lock, flags);
        return SHR_E_NOT_FOUND;
    }
    priv = netdev_priv(ndev);

    add_wait_queue(&dev->wq, &wait);

    while (priv->users) {
        priv->wait = 1;
        set_current_state(TASK_INTERRUPTIBLE);
        spin_unlock_irqrestore(&dev->lock, flags);
        schedule();
        spin_lock_irqsave(&dev->lock, flags);
        priv->wait = 0;
        set_current_state(TASK_RUNNING);
    }

    if (priv->netif.flags & NGKNET_NETIF_F_BIND_CHAN) {
        dev->bdev[priv->netif.chan] = NULL;
    }

    dev->vdev[id] = NULL;
    num = (long)dev->vdev[0];
    while (num-- == id--) {
        if (dev->vdev[id]) {
            dev->vdev[0] = (struct net_device *)(long)num;
            break;
        }
    }

    spin_unlock_irqrestore(&dev->lock, flags);

    remove_wait_queue(&dev->wq, &wait);

    /* Optional netif destroy callback handle */
    list_for_each(list, &dev->cbc->netif_destroy_cb_list) {
        netif_destroy_cb = list_entry(list, netif_cb_t, list);
        if (netif_destroy_cb->cb(&dev->dev_info, &priv->netif)) {
            DBG_WARN(("Network interface callback (destroy) failed for '%s'\n",
                      ndev->name));
        }
    }

    DBG_VERB(("Removing virtual network device %s (%d).\n",
              ndev->name, priv->netif.id));

    unregister_netdev(ndev);
    free_netdev(ndev);

    return SHR_E_NONE;
}

int
ngknet_netif_get(struct ngknet_dev *dev, int id, ngknet_netif_t *netif)
{
    struct net_device *ndev = NULL;
    struct ngknet_private *priv = NULL;
    unsigned long flags;
    int num;

    if (id < 0 || id > NUM_VDEV_MAX) {
        return SHR_E_PARAM;
    }

    spin_lock_irqsave(&dev->lock, flags);

    ndev = id == 0 ? dev->net_dev : dev->vdev[id];
    if (!ndev) {
        spin_unlock_irqrestore(&dev->lock, flags);
        return SHR_E_NOT_FOUND;
    }

    priv = netdev_priv(ndev);
    memcpy(netif, &priv->netif, sizeof(*netif));

    num = (long)dev->vdev[0];
    for (id++; id < num + 1; id++) {
        if (dev->vdev[id]) {
            break;
        }
    }
    netif->next = id == (num + 1) ? 0 : id;

    spin_unlock_irqrestore(&dev->lock, flags);

    DBG_VERB(("Got virtual network device %s (%d).\n",
              ndev->name, priv->netif.id));

    return SHR_E_NONE;
}

int
ngknet_netif_get_next(struct ngknet_dev *dev, ngknet_netif_t *netif)
{
    return ngknet_netif_get(dev, netif->next, netif);
}

/*!
 * System control interfaces
 */

int
ngknet_debug_level_get(void)
{
    return debug;
}

void
ngknet_debug_level_set(int debug_level)
{
    debug = debug_level;
}

int
ngknet_rx_rate_limit_get(void)
{
    return rx_rate_limit;
}

void
ngknet_rx_rate_limit_set(int rate_limit)
{
    rx_rate_limit = rate_limit;
}

int
ngknet_page_buffer_mode_get(void)
{
    return page_buffer_mode;
}

/*!
 * Generic module functions
 */

static int
ngknet_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int
ngknet_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long
ngknet_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ngknet_ioctl ioc;
    struct ngknet_dev *dev = NULL;
    struct net_device *ndev = NULL;
    struct ngknet_private *priv = NULL;
    struct pdma_dev *pdev = NULL;
    union {
        ngknet_dev_cfg_t dev_cfg;
        ngknet_chan_cfg_t chan_cfg;
        ngknet_netif_t netif;
        ngknet_filter_t filter;
    } iod;
    ngknet_dev_cfg_t *dev_cfg = &iod.dev_cfg;
    ngknet_chan_cfg_t *chan_cfg = &iod.chan_cfg;
    ngknet_netif_t *netif = &iod.netif;
    ngknet_filter_t *filter = &iod.filter;
    struct list_head *list = NULL;
    dev_cb_t *dev_cb = NULL;
    char *data = NULL;
    int dt, gi, qi;

    if (_IOC_TYPE(cmd) != NGKNET_IOC_MAGIC) {
        DBG_WARN(("Unsupported command (cmd=%d)\n", cmd));
        return -EINVAL;
    }

    if (copy_from_user(&ioc, (void *)arg, sizeof(ioc))) {
        return -EFAULT;
    }

    ioc.rc = SHR_E_NONE;

    if (ioc.unit >= NUM_PDMA_DEV_MAX) {
        ioc.rc = SHR_E_PARAM;
        if (copy_to_user((void *)arg, &ioc, sizeof(ioc))) {
            return -EFAULT;
        }
        return 0;
    }

    dev = &ngknet_devices[ioc.unit];
    pdev = &dev->pdma_dev;

    if (cmd != NGKNET_VERSION_GET &&
        cmd != NGKNET_RX_RATE_LIMIT &&
        cmd != NGKNET_DEV_INIT &&
        !(dev->flags & NGKNET_DEV_ACTIVE)) {
        ioc.rc = SHR_E_UNAVAIL;
        if (copy_to_user((void *)arg, &ioc, sizeof(ioc))) {
            return -EFAULT;
        }
        return 0;
    }

    memset(&iod, 0, sizeof(iod));

    switch (cmd) {
    case NGKNET_VERSION_GET:
        DBG_CMD(("NGKNET_VERSION_GET\n"));
        ioc.op.info.version = NGKNET_IOC_VERSION;
        break;
    case NGKNET_RX_RATE_LIMIT:
        DBG_CMD(("NGKNET_RX_RATE_LIMIT\n"));
        if (ioc.iarg[0]) {
            ngknet_rx_rate_limit_set(ioc.iarg[1]);
        } else {
            ioc.iarg[1] = ngknet_rx_rate_limit_get();
        }
        break;
    case NGKNET_DEV_INIT:
        DBG_CMD(("NGKNET_DEV_INIT\n"));
        if (dev->flags & NGKNET_DEV_ACTIVE) {
            DBG_CMD(("NGKNET_DEV_INIT, retrieve device configurations.\n"));
            strscpy(dev_cfg->name, pdev->name, sizeof(dev_cfg->name));
            dev_cfg->dev_id = pdev->dev_id;
            dev_cfg->nb_grp = pdev->ctrl.nb_grp;
            dev_cfg->bm_grp = pdev->ctrl.bm_grp;
            ioc.rc = ngknet_netif_get(dev, 0, &dev_cfg->base_netif);
            if (SHR_FAILURE((int)ioc.rc)) {
                break;
            }
            if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, dev_cfg,
                                 ioc.op.data.len, sizeof(*dev_cfg))) {
                return -EFAULT;
            }
            break;
        }
        if (kal_copy_from_user(dev_cfg, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*dev_cfg), ioc.op.data.len)) {
            return -EFAULT;
        }
        if (!dev_cfg->bm_grp || dev_cfg->bm_grp >= (1 << NUM_GRP_MAX)) {
            DBG_WARN(("Invalid parameter: name=%s, bm_grp=0x%x\n",
                      dev_cfg->name, dev_cfg->bm_grp));
            ioc.rc = SHR_E_PARAM;
            break;
        }
        if (dev_cfg->name[0] == '\0') {
            snprintf(dev_cfg->name, sizeof(dev_cfg->name), "%s%d",
                     base_dev_name, ioc.unit);
        }
        memset(pdev, 0, sizeof(*pdev));
        strscpy(pdev->name, dev_cfg->name, sizeof(pdev->name));
        pdev->dev_id = dev_cfg->dev_id;
        for (dt = 0; dt < drv_num; dt++) {
            if (!drv_ops[dt]) {
                continue;
            }
            if (!strcasecmp(dev_cfg->type_str, drv_ops[dt]->drv_desc)) {
                pdev->dev_type = dt;
                strscpy(dev->dev_info.var_str, dev_cfg->var_str,
                        sizeof(dev->dev_info.var_str));
                break;
            }
        }
        if (pdev->dev_type <= NGKNET_DEV_T_NONE ||
            pdev->dev_type >= NGKNET_DEV_T_COUNT) {
            ioc.rc = SHR_E_PARAM;
            break;
        }
        dev->dev_info.dev_id = pdev->dev_id;
        pdev->ctrl.bm_grp = dev_cfg->bm_grp;
        for (gi = 0; gi < NUM_GRP_MAX; gi++) {
            if (1 << gi & dev_cfg->bm_grp) {
                pdev->ctrl.nb_grp++;
                pdev->ctrl.grp[gi].attached = true;
                pdev->num_groups = gi + 1;
            }
        }
        pdev->rx_ph_size = dev_cfg->rx_ph_size;
        pdev->tx_ph_size = dev_cfg->tx_ph_size;
        pdev->flags |= PDMA_GROUP_INTR;
        if (dev_cfg->flags & NGKNET_RX_POLL_SQ) {
            pdev->flags &= ~PDMA_GROUP_INTR;
        }
        pdev->mode = dev_cfg->mode;
        if (pdev->mode != DEV_MODE_KNET && pdev->mode != DEV_MODE_HNET) {
            pdev->mode = DEV_MODE_KNET;
        }
        ioc.rc = ngknet_dev_probe(ioc.unit, &dev_cfg->base_netif);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        list_for_each(list, &dev->cbc->dev_init_cb_list) {
            dev_cb = list_entry(list, dev_cb_t, list);
            dev_cb->cb(&dev->dev_info);
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, dev_cfg,
                             ioc.op.data.len, sizeof(*dev_cfg))) {
            return -EFAULT;
        }
        break;
    case NGKNET_DEV_DEINIT:
        DBG_CMD(("NGKNET_DEV_DEINIT\n"));
        if (dev->flags & NGKNET_DEV_ACTIVE) {
            ioc.rc = ngknet_dev_remove(ioc.unit);
        }
        break;
    case NGKNET_QUEUE_CONFIG:
        DBG_CMD(("NGKNET_QUEUE_CONFIG\n"));
        if (kal_copy_from_user(chan_cfg, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*chan_cfg), ioc.op.data.len)) {
            return -EFAULT;
        }
        gi = chan_cfg->chan / pdev->grp_queues;
        if (!(1 << gi & pdev->ctrl.bm_grp)) {
            DBG_WARN(("Invalid parameter: chan=%d (bm_grp=0x%x)\n",
                      chan_cfg->chan, pdev->ctrl.bm_grp));
            ioc.rc = SHR_E_PARAM;
            break;
        }
        if (chan_cfg->dir == PDMA_Q_RX) {
            if (1 << chan_cfg->chan & pdev->ctrl.bm_txq) {
                pdev->ctrl.bm_txq &= ~(1 << chan_cfg->chan);
                pdev->ctrl.nb_txq--;
            }
            if (!(1 << chan_cfg->chan & pdev->ctrl.bm_rxq)) {
                pdev->ctrl.bm_rxq |= 1 << chan_cfg->chan;
                pdev->ctrl.nb_rxq++;
            }
        } else {
            if (1 << chan_cfg->chan & pdev->ctrl.bm_rxq) {
                pdev->ctrl.bm_rxq &= ~(1 << chan_cfg->chan);
                pdev->ctrl.nb_rxq--;
            }
            if (!(1 << chan_cfg->chan & pdev->ctrl.bm_txq)) {
                pdev->ctrl.bm_txq |= 1 << chan_cfg->chan;
                pdev->ctrl.nb_txq++;
            }
        }
        qi = chan_cfg->chan % pdev->grp_queues;
        pdev->ctrl.grp[gi].nb_desc[qi] = chan_cfg->nb_desc;
        pdev->ctrl.grp[gi].rx_size[qi] = chan_cfg->rx_buf_size;
        pdev->ctrl.grp[gi].que_ctrl[qi] &= ~(PDMA_PKT_BYTE_SWAP |
                                             PDMA_OTH_BYTE_SWAP |
                                             PDMA_HDR_BYTE_SWAP);
        if (chan_cfg->chan_ctrl & NGKNET_PKT_BYTE_SWAP) {
            pdev->ctrl.grp[gi].que_ctrl[qi] |= PDMA_PKT_BYTE_SWAP;
        }
        if (chan_cfg->chan_ctrl & NGKNET_OTH_BYTE_SWAP) {
            pdev->ctrl.grp[gi].que_ctrl[qi] |= PDMA_OTH_BYTE_SWAP;
        }
        if (chan_cfg->chan_ctrl & NGKNET_HDR_BYTE_SWAP) {
            pdev->ctrl.grp[gi].que_ctrl[qi] |= PDMA_HDR_BYTE_SWAP;
        }
        pdev->ctrl.grp[gi].pipe[qi] = chan_cfg->pipe;
        break;
    case NGKNET_QUEUE_QUERY:
        DBG_CMD(("NGKNET_QUEUE_QUERY\n"));
        if (kal_copy_from_user(chan_cfg, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*chan_cfg), ioc.op.data.len)) {
            return -EFAULT;
        }
        if (1 << chan_cfg->chan & pdev->ctrl.bm_rxq) {
            chan_cfg->dir = PDMA_Q_RX;
        } else if (1 << chan_cfg->chan & pdev->ctrl.bm_txq) {
            chan_cfg->dir = PDMA_Q_TX;
        } else {
            ioc.rc = SHR_E_UNAVAIL;
            break;
        }
        gi = chan_cfg->chan / pdev->grp_queues;
        qi = chan_cfg->chan % pdev->grp_queues;
        chan_cfg->nb_desc = pdev->ctrl.grp[gi].nb_desc[qi];
        chan_cfg->chan_ctrl = pdev->ctrl.grp[gi].que_ctrl[qi];
        if (chan_cfg->dir == PDMA_Q_RX) {
            chan_cfg->rx_buf_size = pdev->ctrl.grp[gi].rx_size[qi];
        } else {
            chan_cfg->rx_buf_size = 0;
        }
        chan_cfg->pipe = pdev->ctrl.grp[gi].pipe[qi];
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, chan_cfg,
                             ioc.op.data.len, sizeof(*chan_cfg))) {
            return -EFAULT;
        }
        break;
    case NGKNET_DEV_SUSPEND:
        DBG_CMD(("NGKNET_DEV_SUSPEND\n"));
        if (rx_rate_limit >= 0) {
            ngknet_rx_rate_limit_stop(dev);
        }
        if (ioc.iarg[0]) {
            /* Graceful suspend */
            ioc.rc = bcmcnet_pdma_dev_suspend(pdev);
        } else {
            pdev->flags |= PDMA_ABORT;
            ioc.rc = bcmcnet_pdma_dev_suspend(pdev);
        }
        break;
    case NGKNET_DEV_RESUME:
        DBG_CMD(("NGKNET_DEV_RESUME\n"));
        ioc.rc = bcmcnet_pdma_dev_resume(pdev);
        if (rx_rate_limit >= 0) {
            ngknet_rx_rate_limit_start(dev);
        }
        break;
    case NGKNET_DEV_VNET_WAIT:
        DBG_CMD(("NGKNET_DEV_VNET_WAIT\n"));
        if (pdev->mode != DEV_MODE_HNET) {
            ioc.rc = SHR_E_UNAVAIL;
            break;
        }
        wait_event_interruptible(dev->vnet_wq,
                                 atomic_read(&dev->vnet_active) != 0);
        atomic_set(&dev->vnet_active, 0);
        break;
    case NGKNET_DEV_HNET_WAKE:
        DBG_CMD(("NGKNET_DEV_HNET_WAKE\n"));
        if (pdev->mode != DEV_MODE_HNET) {
            ioc.rc = SHR_E_UNAVAIL;
            break;
        }
        if (atomic_read(&dev->hnet_active) != 1) {
            atomic_set(&dev->hnet_active, 1);
            wake_up_interruptible(&dev->hnet_wq);
        }
        break;
    case NGKNET_DEV_VNET_DOCK:
        DBG_CMD(("NGKNET_DEV_VNET_DOCK\n"));
        if (pdev->mode != DEV_MODE_HNET) {
            ioc.rc = SHR_E_UNAVAIL;
            break;
        }
        if (kal_copy_from_user(&pdev->ctrl.vsync, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(pdev->ctrl.vsync), ioc.op.data.len)) {
            return -EFAULT;
        }
        ioc.rc = bcmcnet_pdma_dev_dock(pdev);
        break;
    case NGKNET_DEV_VNET_UNDOCK:
        DBG_CMD(("NGKNET_DEV_VNET_UNDOCK\n"));
        if (pdev->mode != DEV_MODE_HNET) {
            ioc.rc = SHR_E_UNAVAIL;
            break;
        }
        ngknet_dev_vnet_wake(pdev);
        ioc.rc = bcmcnet_pdma_dev_undock(pdev);
        break;
    case NGKNET_RCPU_CONFIG:
        DBG_CMD(("NGKNET_RCPU_CONFIG\n"));
        if (kal_copy_from_user(&dev->rcpu_ctrl, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(dev->rcpu_ctrl), ioc.op.data.len)) {
            return -EFAULT;
        }
        break;
    case NGKNET_RCPU_GET:
        DBG_CMD(("NGKNET_RCPU_GET\n"));
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, &dev->rcpu_ctrl,
                             ioc.op.data.len, sizeof(dev->rcpu_ctrl))) {
            return -EFAULT;
        }
        break;
    case NGKNET_INFO_GET:
        DBG_CMD(("NGKNET_INFO_GET\n"));
        bcmcnet_pdma_dev_info_get(pdev);
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, &pdev->info,
                             ioc.op.data.len, sizeof(pdev->info))) {
            return -EFAULT;
        }
        break;
    case NGKNET_STATS_GET:
        DBG_CMD(("NGKNET_STATS_GET\n"));
        bcmcnet_pdma_dev_stats_get(pdev);
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, &pdev->stats,
                             ioc.op.data.len, sizeof(pdev->stats))) {
            return -EFAULT;
        }
        break;
    case NGKNET_STATS_RESET:
        DBG_CMD(("NGKNET_STATS_RESET\n"));
        bcmcnet_pdma_dev_stats_reset(pdev, ioc.iarg[0]);
        break;
    case NGKNET_NETIF_CREATE:
        DBG_CMD(("NGKNET_NETIF_CREATE\n"));
        if (kal_copy_from_user(netif, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*netif), ioc.op.data.len)) {
            return -EFAULT;
        }
        ioc.rc = ngknet_netif_create(dev, netif);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, netif,
                             ioc.op.data.len, sizeof(*netif))) {
            return -EFAULT;
        }
        break;
    case NGKNET_NETIF_DESTROY:
        DBG_CMD(("NGKNET_NETIF_DESTROY\n"));
        ioc.rc = ngknet_netif_destroy(dev, ioc.iarg[0]);
        break;
    case NGKNET_NETIF_GET:
        DBG_CMD(("NGKNET_NETIF_GET\n"));
        ioc.rc = ngknet_netif_get(dev, ioc.iarg[0], netif);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, netif,
                             ioc.op.data.len, sizeof(*netif))) {
            return -EFAULT;
        }
        break;
    case NGKNET_NETIF_NEXT:
        DBG_CMD(("NGKNET_NETIF_NEXT\n"));
        if (kal_copy_from_user(netif, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*netif), ioc.op.data.len)) {
            return -EFAULT;
        }
        ioc.rc = ngknet_netif_get_next(dev, netif);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, netif,
                             ioc.op.data.len, sizeof(*netif))) {
            return -EFAULT;
        }
        break;
    case NGKNET_NETIF_LINK_SET:
        DBG_CMD(("NGKNET_NETIF_LINK_SET\n"));
        ioc.rc = ngknet_netif_get(dev, ioc.iarg[0], netif);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        ndev = dev->vdev[netif->id];
        priv = netdev_priv(ndev);
        if (ioc.iarg[1]) {
            if (!netif_carrier_ok(ndev)) {
                queue_work(dev->link_wq, &priv->link_work);
                flush_work(&priv->link_work);
                DBG_LINK(("%s: link up\n", netif->name));
            }
        } else {
            if (netif_carrier_ok(ndev)) {
                queue_work(dev->link_wq, &priv->link_work);
                flush_work(&priv->link_work);
                DBG_LINK(("%s: link down\n", netif->name));
            }
        }
        break;
    case NGKNET_FILT_CREATE:
        DBG_CMD(("NGKNET_FILT_CREATE\n"));
        if (kal_copy_from_user(filter, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*filter), ioc.op.data.len)) {
            return -EFAULT;
        }
        ioc.rc = ngknet_filter_create(dev, filter);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, filter,
                             ioc.op.data.len, sizeof(*filter))) {
            return -EFAULT;
        }
        break;
    case NGKNET_FILT_DESTROY:
        DBG_CMD(("NGKNET_FILT_DESTROY\n"));
        ioc.rc = ngknet_filter_destroy(dev, ioc.iarg[0]);
        break;
    case NGKNET_FILT_GET:
        DBG_CMD(("NGKNET_FILT_GET\n"));
        ioc.rc = ngknet_filter_get(dev, ioc.iarg[0], filter);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, filter,
                             ioc.op.data.len, sizeof(*filter))) {
            return -EFAULT;
        }
        break;
    case NGKNET_FILT_NEXT:
        DBG_CMD(("NGKNET_FILT_NEXT\n"));
        if (kal_copy_from_user(filter, (void *)(unsigned long)ioc.op.data.buf,
                               sizeof(*filter), ioc.op.data.len)) {
            return -EFAULT;
        }
        ioc.rc = ngknet_filter_get_next(dev, filter);
        if (SHR_FAILURE((int)ioc.rc)) {
            break;
        }
        if (kal_copy_to_user((void *)(unsigned long)ioc.op.data.buf, filter,
                             ioc.op.data.len, sizeof(*filter))) {
            return -EFAULT;
        }
        break;
    case NGKNET_PTP_DEV_CTRL:
        DBG_CMD(("NGKNET_PTP_DEV_CTRL\n"));
        if (ioc.op.data.len) {
            data = kmalloc(ioc.op.data.len, GFP_ATOMIC);
            if (data == NULL) {
                printk("Fatal error: no memory for PTP device ioctl\n");
                return -EFAULT;
            }
            if (copy_from_user(data, (void *)(unsigned long)ioc.op.data.buf,
                               ioc.op.data.len)) {
                kfree(data);
                return -EFAULT;
            }
        }
        ioc.rc = ngknet_ptp_dev_ctrl(dev, ioc.iarg[0], data, ioc.op.data.len);
        if (SHR_FAILURE((int)ioc.rc)) {
            if (data) {
                kfree(data);
            }
            break;
        }
        if (ioc.op.data.len) {
            if (copy_to_user((void *)(unsigned long)ioc.op.data.buf, data,
                             ioc.op.data.len)) {
                kfree(data);
                return -EFAULT;
            }
            kfree(data);
        }
        break;
    default:
        ioc.rc = SHR_E_UNAVAIL;
        printk("Invalid IOCTL");
        break;
    }

    if (copy_to_user((void *)arg, &ioc, sizeof(ioc))) {
        return -EFAULT;
    }

    return 0;
}

static int
ngknet_mmap(struct file *filp, struct vm_area_struct *vma)
{
    return 0;
}

static struct file_operations ngknet_fops = {
    .open = ngknet_open,
    .release = ngknet_release,
    .unlocked_ioctl = ngknet_ioctl,
    .compat_ioctl = ngknet_ioctl,
    .mmap = ngknet_mmap,
};

static int __init
ngknet_init_module(void)
{
    int idx;
    int rv;

    rv = register_chrdev(NGKNET_MODULE_MAJOR, NGKNET_MODULE_NAME, &ngknet_fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               NGKNET_MODULE_NAME, NGKNET_MODULE_MAJOR);
        return rv;
    }

    /* Randomize lower 3 bytes of the MAC address (TESTING ONLY) */
    get_random_bytes(&ngknet_dev_mac[3], 3);

    /* Check for user-supplied MAC address (recommended) */
    if (mac_addr != NULL && strlen(mac_addr) == 17) {
        for (idx = 0; idx < 6; idx++) {
            ngknet_dev_mac[idx] = simple_strtoul(&mac_addr[idx * 3], NULL, 16);
        }
        /* Do not allow multicast address */
        ngknet_dev_mac[0] &= ~0x01;
    }

    /* Initialize procfs */
    ngknet_procfs_init();

    /* Initialize Rx rate limit */
    ngknet_rx_rate_limit_init(ngknet_devices);

    /* Initialize Callback control */
    ngknet_callback_init(ngknet_devices);

    return 0;
}

static void __exit
ngknet_exit_module(void)
{
    int idx;

    /* Cleanup Callback control */
    ngknet_callback_cleanup();

    /* Cleanup Rx rate limit */
    ngknet_rx_rate_limit_cleanup();

    /* Cleanup procfs */
    ngknet_procfs_cleanup();

    /* Remove all the devices */
    for (idx = 0; idx < NUM_PDMA_DEV_MAX; idx++) {
        ngknet_dev_remove(idx);
    }

    unregister_chrdev(NGKNET_MODULE_MAJOR, NGKNET_MODULE_NAME);
}

module_init(ngknet_init_module);
module_exit(ngknet_exit_module);

