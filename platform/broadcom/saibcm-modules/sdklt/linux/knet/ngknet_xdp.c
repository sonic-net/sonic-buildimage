/*! \file ngknet_xdp.c
 *
 * NGKNET XDP_NATIVE driver.
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

static inline void
ngknet_xdp_pkt_peek(struct net_device *ndev, void *data, int len, int dir)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    int debug = ngknet_debug_level_get();

    DBG_VERB(("XDP %s packet (%d bytes).\n",
              dir == PDMA_Q_TX ? "Tx" : "Rx", len));

    if (debug & DBG_LVL_PDMP) {
        ngknet_pkt_dump(data, len);
    }

    if (debug & DBG_LVL_RATE) {
        ngknet_pkt_stats(pdev, dir);
    }
}

static int
ngknet_xdp_rx_frame_process(struct net_device *ndev, struct xdp_buff *xdp, uint32_t *mlen)
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
ngknet_xdp_tx_frame_process(struct net_device *ndev, struct xdp_frame *xdpf, bool act)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct pkt_hdr *pkh = (struct pkt_hdr *)xdpf->data;
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
        if (xdpf->len != pkt_len || xdpf->len < (PKT_HDR_SIZE + ETH_HLEN)) {
            printk(KERN_ERR "Tx drop: Invalid packet length\n");
            return SHR_E_PARAM;
        }
        if (dev->rcpu_ctrl.pkt_sig && dev->rcpu_ctrl.pkt_sig != ntohs(rch->pkt_sig)) {
            printk(KERN_ERR "Tx drop: Invalid packet signature\n");
            return SHR_E_PARAM;
        }
        if (pkh->attrs & PDMA_TX_HDR_COOKED) {
            /* Resumed packet */
            return SHR_E_NONE;
        }

        /* Populate internal packet header */
        xdpf->len += fcs_len;
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
        if (xdpf->len == pkt_len && pkh->attrs & PDMA_TX_HDR_COOKED &&
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
        memmove(xdpf->data + PKT_HDR_SIZE + meta_len, xdpf->data, xdpf->len);
        xdpf->len += PKT_HDR_SIZE + meta_len + fcs_len;
        memset(xdpf->data, 0, PKT_HDR_SIZE + meta_len);
        pkh->data_len = xdpf->len - PKT_HDR_SIZE - meta_len;
        pkh->meta_len = meta_len;
        if (meta_len) {
            /* Send to physical port using netif metadata */
            if (priv->netif.meta_off) {
                memmove(xdpf->data + PKT_HDR_SIZE,
                        xdpf->data + PKT_HDR_SIZE + meta_len,
                        priv->netif.meta_off);
            }
            memcpy(xdpf->data + PKT_HDR_SIZE + priv->netif.meta_off,
                   priv->netif.meta_data, priv->netif.meta_len);
            pkh->attrs |= PDMA_TX_HIGIG_PKT;
        }
        pkh->pkt_sig = dev->rcpu_ctrl.pkt_sig;
    }

    data = xdpf->data + PKT_HDR_SIZE + pkh->meta_len;
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
        xdpf->len += VLAN_HLEN;
    }

    /* Packet header done here */
    pkh->attrs |= PDMA_TX_XDP_FRM | PDMA_TX_HDR_COOKED;
    if (act) {
        pkh->attrs |= PDMA_TX_XDP_ACT;
    }

    return SHR_E_NONE;
}

static int
ngknet_xdp_frame_xmit(struct net_device *ndev, struct xdp_frame *frame, bool act)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct pkt_buf *pkb;
    int rv, qi = -1;

    /* Pre-process the Tx packet */
    rv = ngknet_xdp_tx_frame_process(ndev, frame, act);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    /* Peek the packet for debugging */
    ngknet_xdp_pkt_peek(ndev, frame->data, frame->len, PDMA_Q_TX);

    /* Do Tx */
    pkb = (struct pkt_buf *)frame->data;
    ngknet_tx_queue_schedule(dev, pkb, &qi);
    if (qi >= 0) {
        return pdev->pkt_xmit(pdev, qi, &frame->data);
    }

    for (qi = 0; qi < pdev->ctrl.nb_txq; qi++) {
        rv = pdev->pkt_xmit(pdev, qi, &frame->data);
        if (SHR_SUCCESS(rv)) {
            break;
        }
    }

    return rv;
}

static int
ngknet_xdp_prog_setup(struct net_device *ndev, struct bpf_prog *prog)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    struct bpf_prog *old_prog;

    if (!ngknet_page_buffer_mode_get() && !priv->xsk_zc) {
        printk(KERN_ERR "PAGE buffer mode is not enabled for XSK non-ZC\n");
        return -EINVAL;
    }

    old_prog = xchg(&priv->xdp_prog, prog);
    if (old_prog) {
        bpf_prog_put(old_prog);
    }

    if (!!prog != !!old_prog) {
        if (prog) {
            /* Register XDP Rx info */
            if (!xdp_rxq_info_is_reg(&priv->xri)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,0))
                if (xdp_rxq_info_reg(&priv->xri, ndev, 0, 0) < 0) {
#else
                if (xdp_rxq_info_reg(&priv->xri, ndev, 0) < 0) {
#endif
                    printk(KERN_ERR "XDP Rx info register failed\n");
                    return -EINVAL;
                }
            }
            if (!priv->xsk_zc) {
                xdp_rxq_info_unreg_mem_model(&priv->xri);
                xdp_rxq_info_reg_mem_model(&priv->xri, MEM_TYPE_PAGE_SHARED, NULL);
            }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0))
            xdp_features_set_redirect_target(ndev, true);
#endif
            dev->xprog_num++;
            printk(KERN_CRIT "XDP program attached to ndev%d.\n", priv->netif.id);
        } else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0))
            xdp_features_clear_redirect_target(ndev);
#endif
            if (!priv->xsk_zc) {
                xdp_rxq_info_unreg(&priv->xri);
            }
            dev->xprog_num--;
            printk(KERN_CRIT "XDP program detached from ndev%d.\n", priv->netif.id);
        }
    }

    return 0;
}

int
ngknet_xdp_setup(struct net_device *ndev, struct netdev_bpf *bpf)
{
    switch (bpf->command) {
    case XDP_SETUP_PROG:
        return ngknet_xdp_prog_setup(ndev, bpf->prog);
    case XDP_SETUP_XSK_POOL:
        return ngknet_xsk_pool_setup(ndev, bpf->xsk.pool, bpf->xsk.queue_id);
    default:
        return -EINVAL;
    }

    return 0;
}

int
ngknet_xdp_xmit(struct net_device *ndev, int n,
                struct xdp_frame **frames, uint32_t flags)
{
    struct xdp_frame *frame;
    int fn, rv;

    if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK)) {
        return -EINVAL;
    }

    for (fn = 0; fn < n; fn++) {
        frame = frames[fn];
        rv = ngknet_xdp_frame_xmit(ndev, frame, false);
        if (SHR_FAILURE(rv)) {
            break;
        }
    }

    return fn;
}

int
ngknet_run_xdp(struct net_device *ndev, struct xdp_buff *xdp)
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
    ngknet_xdp_rx_frame_process(ndev, xdp, &mlen);

    /* Peek the packet for debugging */
    ngknet_xdp_pkt_peek(ndev, xdp->data, xdp->data_end - xdp->data, PDMA_Q_RX);

    /* Run XDP program and take the action */
    act = bpf_prog_run_xdp(xdp_prog, xdp);
    switch (act) {
    case XDP_PASS:
        xdp->data = data;
        xv = NGKNET_XDP_PASS;
        break;
    case XDP_TX:
        xdp->data = data;
        xdp->data += mlen;
        xdp->data_meta = xdp->data;
        xdpf = xdp_convert_buff_to_frame(xdp);
        if (unlikely(!xdpf)) {
            goto fail;
        }
        rv = ngknet_xdp_frame_xmit(ndev, xdpf, true);
        if (SHR_FAILURE(rv)) {
            printk(KERN_ERR "Tx drop: XDP action Tx error %d\n", rv);
            goto fail;
        }
        xv = NGKNET_XDP_TX;
        break;
    case XDP_REDIRECT:
        rv = xdp_do_redirect(ndev, xdp, xdp_prog);
        if (rv) {
            goto fail;
        }
        xdp_do_flush();
        xv = NGKNET_XDP_REDIR;
        break;
    default:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
        bpf_warn_invalid_xdp_action(ndev, xdp_prog, act);
#else
        bpf_warn_invalid_xdp_action(act);
#endif
        fallthrough;
    case XDP_ABORTED:
fail:
        trace_xdp_exception(ndev, xdp_prog, act);
        fallthrough;
    case XDP_DROP:
        xv = NGKNET_XDP_DROP;
        break;
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
