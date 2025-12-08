/*! \file ngknet_xsk.c
 *
 * NGKNET AF_XDP Zero-copy driver.
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
#include "ngknet_extra.h"
#include "ngknet_xdp.h"
#include "ngknet_xsk.h"

#ifdef NGKNET_XDP_NATIVE

/* Some older kernels do not free buffer on error */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,6,15)
#define XSK_RCV_ZC_NO_FREE_ON_ERR
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,7,0) && \
     LINUX_VERSION_CODE <= KERNEL_VERSION(6,7,2))
#define XSK_RCV_ZC_NO_FREE_ON_ERR
#endif

#define NGKNET_RX_DMA_ATTR  (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

extern int xsk_napi_tx;

static atomic_t xsk_active;
static wait_queue_head_t xsk_wq;
static struct task_struct *xsk_task;

static inline void
ngknet_xsk_pkt_peek(struct net_device *ndev, void *data, int len, int dir)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    int debug = ngknet_debug_level_get();

    DBG_VERB(("XDP ZC %s packet (%d bytes).\n",
              dir == PDMA_Q_TX ? "Tx" : "Rx", len));

    if (debug & DBG_LVL_PDMP) {
        ngknet_pkt_dump(data, len);
    }

    if (debug & DBG_LVL_RATE) {
        ngknet_pkt_stats(pdev, dir);
    }
}

static int
ngknet_xsk_rx_frame_process(struct net_device *ndev, struct xdp_buff *xdp, uint32_t *mlen)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pkt_hdr *pkh = (struct pkt_hdr *)(xdp->data - PKT_HDR_SIZE);
    struct ngknet_rcpu_hdr *rch = (struct ngknet_rcpu_hdr *)pkh;
    uint8_t meta_len = pkh->meta_len;

    if (priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) {
        /* Set up RCPU header */
        memcpy(rch, xdp->data + meta_len, 2 * ETH_ALEN);
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
        xdp->data = rch;
    } else {
        /* Remove meta data */
        xdp->data += meta_len;
    }

    xdp->data_meta = xdp->data;
    *mlen = meta_len;

    return SHR_E_NONE;
}

static int
ngknet_xsk_tx_frame_process(struct net_device *ndev, struct xsk_frame *xskf, bool act)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct pkt_hdr *pkh = (struct pkt_hdr *)xskf->data;
    struct ngknet_rcpu_hdr *rch = (struct ngknet_rcpu_hdr *)pkh;
    unsigned char *data;
    uint32_t meta_len, data_len, pkt_len, tag_len;
    uint16_t fcs_len = pdev->flags & PDMA_NO_FCS ? 0 : ETH_FCS_LEN;
    uint16_t tpid;

    /* Set up packet header */
    if ((priv->netif.flags & NGKNET_NETIF_F_RCPU_ENCAP) && !act) {
        /* RCPU encapsulation packet */
        data_len = pkh->attrs & PDMA_TX_HDR_COOKED ?
                   pkh->data_len : ntohs(rch->data_len);
        pkt_len = PKT_HDR_SIZE + rch->meta_len + data_len;
        if (xskf->len != pkt_len || xskf->len < (PKT_HDR_SIZE + ETH_HLEN)) {
            printk(KERN_ERR "Invalid packet header\n");
            /* Let HW drop the packet */
            data_len = xskf->len - sizeof(struct ngknet_rcpu_hdr);
            rch->meta_len = 0;
            rch->flags = RCPU_FLAG_MODHDR;
        } else if (dev->rcpu_ctrl.pkt_sig && dev->rcpu_ctrl.pkt_sig != ntohs(rch->pkt_sig)) {
            printk(KERN_ERR "Invalid packet signature\n");
        }
        if (pkh->attrs & PDMA_TX_HDR_COOKED) {
            /* Resumed packet */
            return SHR_E_NONE;
        }

        /* Populate internal packet header */
        xskf->len += fcs_len;
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
        data_len = pkh->data_len;
        pkt_len = PKT_HDR_SIZE + pkh->meta_len + data_len;
        if (xskf->len == pkt_len && pkh->attrs & PDMA_TX_HDR_COOKED &&
            pkh->pkt_sig == dev->rcpu_ctrl.pkt_sig) {
            /* Resumed packet */
            return SHR_E_NONE;
        }

        /* Populate internal packet header and meta data if used */
        meta_len = 0;
        if (priv->netif.type == NGKNET_NETIF_T_PORT ||
            priv->netif.type == NGKNET_NETIF_T_META) {
            meta_len = priv->netif.meta_len;
            if (!meta_len) {
                printk(KERN_ERR "Tx abort: no metadata\n");
                return SHR_E_UNAVAIL;
            }
        }
        memmove(xskf->data + PKT_HDR_SIZE + meta_len, xskf->data, xskf->len);
        xskf->len += PKT_HDR_SIZE + meta_len + fcs_len;
        memset(xskf->data, 0, PKT_HDR_SIZE + meta_len);
        pkh->data_len = xskf->len - PKT_HDR_SIZE - meta_len;
        pkh->meta_len = meta_len;
        if (meta_len) {
            /* Send to physical port using netif metadata */
            if (priv->netif.meta_off) {
                memmove(xskf->data + PKT_HDR_SIZE,
                        xskf->data + PKT_HDR_SIZE + meta_len,
                        priv->netif.meta_off);
            }
            memcpy(xskf->data + PKT_HDR_SIZE + priv->netif.meta_off,
                   priv->netif.meta_data, priv->netif.meta_len);
            pkh->attrs |= PDMA_TX_HIGIG_PKT;
        }
        pkh->pkt_sig = dev->rcpu_ctrl.pkt_sig;
    }

    data = xskf->data + PKT_HDR_SIZE + pkh->meta_len;
    tpid = data[12] << 8 | data[13];
    tag_len = (tpid == ETH_P_8021Q || tpid == ETH_P_8021AD) ? VLAN_HLEN : 0;

    /* Need to add VLAN tag if packet is untagged */
    if (tag_len == 0 && (priv->netif.vlan & 0xfff) != 0 &&
        (!(pkh->attrs & PDMA_TX_HIGIG_PKT) ||
         priv->netif.flags & NGKNET_NETIF_F_ADD_TAG)) {
        memmove(&data[16], &data[12], pkh->data_len - ETH_ALEN * 2);
        data[12] = 0x81;
        data[13] = 0x00;
        data[14] = priv->netif.vlan >> 8 & 0xf;
        data[15] = priv->netif.vlan & 0xff;
        pkh->data_len += VLAN_HLEN;
        xskf->len += VLAN_HLEN;
    }

    /* Packet header done here */
    pkh->attrs |= PDMA_TX_XSK_ZC | PDMA_TX_HDR_COOKED;
    if (act) {
        pkh->attrs |= PDMA_TX_XDP_ACT;
    }

    return SHR_E_NONE;
}

static int
ngknet_xsk_frame_xmit(struct net_device *ndev, void *desc, bool act)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct xdp_frame *xdpf;
    struct xdp_desc *xdpd;
    struct pkt_buf *pkb;
    struct xsk_frame xskf;
    int rv, qi = -1;

    /* Convert buffer format to support both XSK Tx and XDP action Tx */
    if (act) {
        xdpf = (struct xdp_frame *)desc;
        xskf.data = xdpf->data;
        xskf.len = xdpf->len;
        xskf.desc = desc;
    } else {
        xdpd = (struct xdp_desc *)desc;
        xskf.data = xsk_buff_raw_get_data(dev->xsk_pool, xdpd->addr);
        xskf.len = xdpd->len;
        xskf.desc = xdpd;
    }

    /* Pre-process the Tx packet */
    rv = ngknet_xsk_tx_frame_process(ndev, &xskf, act);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    /* Peek the packet for debugging */
    ngknet_xsk_pkt_peek(ndev, xskf.data, xskf.len, PDMA_Q_TX);

    if (act) {
        xdpf->len = xskf.len;
    } else {
        xdpd->len = xskf.len;
    }

    /* Do Tx */
    pkb = (struct pkt_buf *)xskf.data;
    ngknet_tx_queue_schedule(dev, pkb, &qi);
    if (qi >= 0) {
        return pdev->pkt_xmit(pdev, qi, &xskf.data);
    }

    return pdev->pkt_xmit(pdev, dev->xsk_queue, &xskf.data);
}

static bool
ngknet_xsk_frame_get(struct xsk_buff_pool *pool, struct xdp_desc *desc)
{
    static struct xdp_desc desc_c, desc_n;
    static bool pending = false;

    if (!pending) {
        if (!xsk_tx_peek_desc(pool, &desc_c)) {
            if (xsk_uses_need_wakeup(pool)) {
                xsk_set_tx_need_wakeup(pool);
            }
            return false;
        }
    }

    if (!xsk_tx_peek_desc(pool, &desc_n)) {
        if (xsk_uses_need_wakeup(pool)) {
            xsk_set_tx_need_wakeup(pool);
        }
        pending = false;
    } else {
        pending = true;
    }

    *desc = desc_c;

    if (pending) {
        desc_c = desc_n;
    }

    return true;
}

static int
ngknet_xsk_xmit(void *data)
{
    struct net_device *ndev = (struct net_device *)data;
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    static struct xdp_desc desc = {0};
    int rv;

    while (!kthread_should_stop()) {
        wait_event_interruptible(xsk_wq, atomic_read(&xsk_active) != 0);
        atomic_set(&xsk_active, 0);
        if (!dev->xsk_pool) {
            break;
        }

        while (desc.len || ngknet_xsk_frame_get(dev->xsk_pool, &desc)) {
            rv = ngknet_xsk_frame_xmit(ndev, &desc, false);
            if (SHR_FAILURE(rv)) {
                if (rv == SHR_E_BUSY) {
                    break;
                } else {
                    xsk_tx_completed(dev->xsk_pool, 1);
                    printk(KERN_ERR "Tx drop: XSK Tx error %d\n", rv);
                }
            }
            desc.len = 0;
            xsk_tx_release(dev->xsk_pool);
        }
    }

    return 0;
}

static int
ngknet_xsk_thread_create(struct net_device *ndev)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct cpumask mask;

    atomic_set(&xsk_active, 0);
    init_waitqueue_head(&xsk_wq);
    xsk_task = kthread_run(ngknet_xsk_xmit, ndev, pdev->name);
    if (IS_ERR(xsk_task)) {
        return -EINVAL;
    }

    cpumask_clear(&mask);
    cpumask_set_cpu(num_online_cpus() - 1, &mask);
    set_cpus_allowed_ptr(xsk_task, &mask);

    return 0;
}

static void
ngknet_xsk_thread_destroy(void)
{
    atomic_set(&xsk_active, 1);
    wake_up_interruptible(&xsk_wq);
    kthread_stop(xsk_task);
    xsk_task = NULL;
}

static int
ngknet_xsk_buffer_deploy(struct net_device *ndev, bool enable)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    int rv;

    /* Suspend DMA operation for switching buffer mode */
    pdev->flags |= PDMA_ABORT;
    rv = bcmcnet_pdma_dev_suspend(pdev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    if (enable) {
        dev->flags |= NGKNET_XSK_ZC;
        pdev->flags |= PDMA_RX_BATCHING;
    } else {
        pdev->flags &= ~PDMA_RX_BATCHING;
        dev->flags &= ~NGKNET_XSK_ZC;
    }

    /* Resume DMA operation and use new buffer mode */
    rv = bcmcnet_pdma_dev_resume(pdev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    return rv;
}

static int
ngknet_xsk_pool_enable(struct net_device *ndev, struct xsk_buff_pool *pool,
                       uint32_t queue)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    int rv;

    if (dev->xsk_pool) {
        /*
         * In KNET, all DMA channels are shared by all virtual netdevs,
         * and only one common DMA buffer pool can be used. The buffer
         * pool should be enabled only on one netdev/queue pair.
         */
        printk(KERN_ERR "Only one netdev/queue can work in XSK zero-copy mode\n");
        return -EINVAL;
    }

    if (queue >= pdev->ctrl.nb_rxq || queue >= pdev->ctrl.nb_txq) {
        return -EINVAL;
    }

    rv = xsk_pool_dma_map(pool, dev->dev, NGKNET_RX_DMA_ATTR);
    if (rv) {
        return rv;
    }

    dev->xsk_pool = pool;
    dev->xsk_queue = queue;
    priv->xsk_zc = true;

    /* Create helper thread for Tx */
    if (!xsk_napi_tx) {
        rv = ngknet_xsk_thread_create(ndev);
        if (rv < 0) {
            xsk_pool_dma_unmap(pool, NGKNET_RX_DMA_ATTR);
            dev->xsk_pool = NULL;
            return -EINVAL;
        }
    }

    /* Deploy new buffer pool for XSK ZC */
    rv = ngknet_xsk_buffer_deploy(ndev, true);
    if (SHR_FAILURE(rv)) {
        xsk_pool_dma_unmap(pool, NGKNET_RX_DMA_ATTR);
        dev->xsk_pool = NULL;
        return -EINVAL;
    }

    /* Register XDP Rx info */
    if (!xdp_rxq_info_is_reg(&priv->xri)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,0))
        if (xdp_rxq_info_reg(&priv->xri, ndev, 0, 0) < 0) {
#else
        if (xdp_rxq_info_reg(&priv->xri, ndev, 0) < 0) {
#endif
            ngknet_xsk_thread_destroy();
            xsk_pool_dma_unmap(pool, NGKNET_RX_DMA_ATTR);
            dev->xsk_pool = NULL;
            printk(KERN_ERR "XDP Rx info register failed\n");
            return -EINVAL;
        }
    }

    xdp_rxq_info_unreg_mem_model(&priv->xri);
    xdp_rxq_info_reg_mem_model(&priv->xri, MEM_TYPE_XSK_BUFF_POOL, NULL);
    xsk_pool_set_rxq_info(pool, &priv->xri);

    printk(KERN_CRIT "XSK buffer pool enabled.\n");

    return 0;
}

static int
ngknet_xsk_pool_disable(struct net_device *ndev, uint32_t queue)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct xsk_buff_pool *pool;

    if (!dev->xsk_pool) {
        return 0;
    }

    pool = xsk_get_pool_from_qid(ndev, queue);
    if (!pool || pool != dev->xsk_pool) {
        return -EINVAL;
    }

    /* Restore old buffer mode */
    ngknet_xsk_buffer_deploy(ndev, false);

    xsk_pool_dma_unmap(pool, NGKNET_RX_DMA_ATTR);

    if (priv->xdp_prog) {
        xdp_rxq_info_unreg_mem_model(&priv->xri);
        xdp_rxq_info_reg_mem_model(&priv->xri, MEM_TYPE_PAGE_SHARED, NULL);
    } else {
        xdp_rxq_info_unreg(&priv->xri);
    }

    dev->xsk_pool = NULL;
    priv->xsk_zc = false;

    /* Destroy Tx helper thread */
    if (!xsk_napi_tx) {
        ngknet_xsk_thread_destroy();
    }

    printk(KERN_CRIT "XSK buffer pool disabled.\n");

    return 0;
}

int
ngknet_xsk_pool_setup(struct net_device *ndev, struct xsk_buff_pool *pool,
                      uint32_t queue)
{
    return pool ? ngknet_xsk_pool_enable(ndev, pool, queue) :
                  ngknet_xsk_pool_disable(ndev, queue);
}

int
ngknet_xsk_wakeup(struct net_device *ndev, uint32_t queue, uint32_t flags)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct intr_handle *hdl;
    struct napi_struct *napi;
    unsigned long qbm;
    int qi;

    if (xsk_uses_need_wakeup(dev->xsk_pool)) {
        xsk_clear_rx_need_wakeup(dev->xsk_pool);
        xsk_clear_tx_need_wakeup(dev->xsk_pool);
    }

    /* Schedule NAPI to Tx or fill Rx buffers */
    qbm = pdev->ctrl.grp[0].bm_rxq | pdev->ctrl.grp[0].bm_txq;
    for (qi = 0; qi < pdev->grp_queues; qi++) {
        if (!(pdev->flags & PDMA_GROUP_INTR) && !(1 << qi & qbm)) {
            continue;
        }
        hdl = &pdev->ctrl.grp[0].intr_hdl[qi];
        napi = (struct napi_struct *)hdl->priv;
        if (!napi_if_scheduled_mark_missed(napi) && napi_schedule_prep(napi)) {
            local_bh_disable();
            __napi_schedule(napi);
            local_bh_enable();
        }
        if (pdev->flags & PDMA_GROUP_INTR) {
            break;
        }
    }

    /* Schedule helper thread for Tx */
    if (!xsk_napi_tx) {
        atomic_set(&xsk_active, 1);
        wake_up_interruptible(&xsk_wq);
    }

    return 0;
}

int
ngknet_xsk_napi_tx(struct ngknet_dev *dev, struct intr_handle *hdl, int budget)
{
    struct xsk_buff_pool *pool = dev->xsk_pool;
    struct pdma_dev *pdev = &dev->pdma_dev;
    static struct xdp_desc desc = {0};
    int queue = pdev->flags & PDMA_GROUP_INTR ? 0 : dev->xsk_queue;
    int rv, done = 0;

    if (hdl->queue != queue || hdl->dir != PDMA_Q_TX) {
        return 0;
    }

    if (!desc.len) {
        if (xsk_uses_need_wakeup(pool)) {
            xsk_set_tx_need_wakeup(pool);
        }
    }

    while (done < budget) {
        if (!desc.len && !xsk_tx_peek_desc(pool, &desc)) {
            break;
        }
        rv = ngknet_xsk_frame_xmit(dev->net_dev, &desc, false);
        if (SHR_FAILURE(rv) && rv != SHR_E_BUSY) {
            xsk_tx_completed(pool, 1);
            desc.len = 0;
            printk(KERN_ERR "Tx drop: XSK Tx error %d\n", rv);
            continue;
        } else if (rv == SHR_E_BUSY) {
            return done;
        }
        desc.len = 0;
        done++;
    }

    if (done) {
        xsk_tx_release(pool);
    }

    return done;
}

int
ngknet_run_xdp_zc(struct net_device *ndev, struct xdp_buff *xdp)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct bpf_prog *xdp_prog = priv->xdp_prog;
    struct xdp_frame *xdpf;
    void *data = xdp->data;
    uint32_t mlen;
    uint32_t act, xv;
    unsigned long flags;
    int rv;

    if (!xdp_prog) {
        return NGKNET_XDP_PASS;
    }

    /* Pre-process the Rx packet */
    ngknet_xsk_rx_frame_process(ndev, xdp, &mlen);

    /* Peek the packet for debugging */
    ngknet_xsk_pkt_peek(ndev, xdp->data, xdp->data_end - xdp->data, PDMA_Q_RX);

    /* Run XDP program and take the action */
    act = bpf_prog_run_xdp(xdp_prog, xdp);
    switch (act) {
    case XDP_PASS:
        xdp->data = data;
        xv = NGKNET_XDP_PASS;
        break;
    case XDP_TX:
        xdp->data = data + mlen;
        xdp->data_meta = xdp->data;
        xdpf = xdp_convert_buff_to_frame(xdp);
        if (unlikely(!xdpf)) {
            xv = NGKNET_XDP_DROP;
            printk(KERN_ERR "Tx drop: XDP frame convert failed\n");
            goto fail;
        }
        xv = NGKNET_XDP_TX;
        rv = ngknet_xsk_frame_xmit(ndev, xdpf, true);
        if (SHR_FAILURE(rv)) {
            printk(KERN_ERR "Tx drop: XDP action Tx error %d\n", rv);
            xdp_return_frame(xdpf);
            goto fail;
        }
        break;
    case XDP_REDIRECT:
        rv = xdp_do_redirect(ndev, xdp, xdp_prog);
        if (rv) {
            if (xsk_uses_need_wakeup(dev->xsk_pool) &&
                (rv == -ENOBUFS || rv == -ENOSPC)) {
                xdp->data = data;
#ifdef XSK_RCV_ZC_NO_FREE_ON_ERR
                xv = NGKNET_XDP_BUSY;
#else
                xv = NGKNET_XDP_EXIT;
                printk(KERN_INFO "Redir drop: Rx ring is full\n");
#endif
            } else {
                xv = NGKNET_XDP_DROP;
                printk(KERN_ERR "Redir drop: error %d\n", rv);
            }
            goto fail;
        }
        xdp_do_flush();
        xv = NGKNET_XDP_REDIR;
        break;
    case XDP_DROP:
        xv = NGKNET_XDP_DROP;
        break;
    default:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
        bpf_warn_invalid_xdp_action(ndev, xdp_prog, act);
#else
        bpf_warn_invalid_xdp_action(act);
#endif
        fallthrough;
    case XDP_ABORTED:
        xv = NGKNET_XDP_DROP;
fail:
        trace_xdp_exception(ndev, xdp_prog, act);
    }

    if (xv != NGKNET_XDP_PASS) {
        spin_lock_irqsave(&dev->lock, flags);
        priv->users--;
        if (!priv->users && priv->wait) {
            wake_up(&dev->wq);
        }
        spin_unlock_irqrestore(&dev->lock, flags);
    }

    return xv;
}

#endif /* NGKNET_XDP_NATIVE */
