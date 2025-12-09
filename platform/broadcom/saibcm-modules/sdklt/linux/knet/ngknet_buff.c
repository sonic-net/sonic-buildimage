/*! \file ngknet_buff.c
 *
 * Utility routines for NGKNET packet buffer management in Linux kernel mode.
 *
 * These callbacks in the standalone buffer management will be called by pktio
 * driver for various buffer operations:
 *   - Set buffer mode based on the pktio working mode, see \ref enum buf_mode.
 *   - Allocate and free DCBs, which should be cache coherent.
 *   - Allocate and free packet buffers, which should use appropriate APIs
 *     based on buffer mode, and should be mapped/unmapped for DMA operation.
 *   - Get DMA address for filling DCB.
 *   - Validate buffer and reusing it.
 *   - Get buffer information such as packet length, meta data length and so on.
 *   - Put buffer if it can be reused.
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

#include <bcmcnet/bcmcnet_core.h>
#include <bcmcnet/bcmcnet_dev.h>
#include <bcmcnet/bcmcnet_rxtx.h>
#include "ngknet_main.h"
#include "ngknet_extra.h"
#include "ngknet_xdp.h"
#include "ngknet_xsk.h"
#include "ngknet_buff.h"

/*
 * Macros for meta data offset calculation when DMA buffer is mapped for
 * hardware or DMAed data length is set.
 * The adjustment for meta data length indicated by "adj" should be set in
 * pktio driver. In Rx, it is the length of meta data. In Tx, it is a bool
 * value to indicate if the meta data length should be considered.
 * It only needs to set for legacy devices on which meta data exists in DCB
 * and can not be sent/received along with packet payload. Instead, it is
 * always 0 for modern LT devices.
 */
#define RX_BUFF_RSV(pbuf)   (PDMA_RXB_RESV + pbuf->adj)
#define RX_BUFF_DMA(pbuf)   (&pbuf->pkb->data + pbuf->adj)
#define TX_META_LEN(pbuf)   (pbuf->adj ? 0 : pbuf->pkb->pkh.meta_len)
#define TX_BUFF_DMA(pbuf)   (&pbuf->pkb->data + \
                             (pbuf->adj ? pbuf->pkb->pkh.meta_len : 0))

#define NGKNET_RX_DMA_ATTR  (DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

/*!
 * Allocate coherent memory
 */
static void *
ngknet_ring_buf_alloc(struct pdma_dev *dev, uint32_t size, dma_addr_t *dma)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;

    return dma_alloc_coherent(kdev->dev, size, dma, GFP_KERNEL);
}

/*!
 * Free coherent memory
 */
static void
ngknet_ring_buf_free(struct pdma_dev *dev, uint32_t size, void *addr, dma_addr_t dma)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;

    dma_free_coherent(kdev->dev, size, addr, dma);
}

/*!
 * Allocate Rx buffer
 */
static int
ngknet_rx_buf_alloc(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                    struct pdma_rx_buf *pbuf)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    dma_addr_t dma;

#ifdef NGKNET_XDP_NATIVE
    if (rxq->buf_mode == PDMA_BUF_MODE_MAPPED) {
        struct xdp_buff *xdp = xsk_buff_alloc(kdev->xsk_pool);
        if (!xdp) {
            if (xsk_uses_need_wakeup(kdev->xsk_pool)) {
                xsk_set_rx_need_wakeup(kdev->xsk_pool);
            }
            return SHR_E_MEMORY;
        }
        pbuf->dma = xsk_buff_xdp_get_dma(xdp);
        pbuf->skb = (struct sk_buff *)xdp;
    } else
#endif
    if (rxq->buf_mode == PDMA_BUF_MODE_PAGE) {
        struct page *page = kal_dev_alloc_pages(rxq->page_order);
        if (unlikely(!page)) {
            return SHR_E_MEMORY;
        }
        dma = kal_dma_map_page_attrs(kdev->dev, page, 0, rxq->page_size,
                                     DMA_FROM_DEVICE, NGKNET_RX_DMA_ATTR);
        if (unlikely(dma_mapping_error(kdev->dev, dma))) {
            __free_pages(page, rxq->page_order);
            return SHR_E_MEMORY;
        }
        pbuf->dma = dma;
        pbuf->page = page;
        pbuf->page_offset = 0;
        dma_sync_single_range_for_device(kdev->dev, pbuf->dma, pbuf->page_offset,
                                         rxq->page_size >> 1, DMA_FROM_DEVICE);
    } else {
        struct sk_buff *skb;
        skb = netdev_alloc_skb(kdev->net_dev, RX_BUFF_RSV(pbuf) + rxq->buf_size);
        if (unlikely(!skb)) {
            return SHR_E_MEMORY;
        }
        skb_reserve(skb, PDMA_RXB_ALIGN - ((unsigned long)skb->data &
                                           (PDMA_RXB_ALIGN - 1)));
        pbuf->pkb = (struct pkt_buf *)skb->data;
        dma = dma_map_single(kdev->dev, RX_BUFF_DMA(pbuf), rxq->buf_size,
                             DMA_FROM_DEVICE);
        if (unlikely(dma_mapping_error(kdev->dev, dma))) {
            dev_kfree_skb_any(skb);
            return SHR_E_MEMORY;
        }
        pbuf->dma = dma;
        pbuf->skb = skb;
    }

    return SHR_E_NONE;
}

/*!
 * Get Rx buffer DMA address
 */
static void
ngknet_rx_buf_dma(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                  struct pdma_rx_buf *pbuf, dma_addr_t *addr)
{
    if (!pbuf->dma) {
        *addr = 0;
        return;
    }

#ifdef NGKNET_XDP_NATIVE
    if (rxq->buf_mode == PDMA_BUF_MODE_MAPPED) {
        *addr = pbuf->dma + RX_BUFF_RSV(pbuf);
    } else
#endif
    if (rxq->buf_mode == PDMA_BUF_MODE_PAGE) {
        *addr = pbuf->dma + pbuf->page_offset + RX_BUFF_RSV(pbuf);
    } else {
        *addr = pbuf->dma;
    }
}

/*!
 * Check Rx buffer
 */
static bool
ngknet_rx_buf_avail(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                    struct pdma_rx_buf *pbuf)
{
    if (rxq->buf_mode == PDMA_BUF_MODE_PAGE) {
        pbuf->skb = NULL;
    }

    return (pbuf->dma != 0);
}

/*!
 * Reuse the pages
 */
static void
ngknet_rx_page_reuse(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                     struct pdma_rx_buf *pbuf)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;

    if (unlikely(page_count(pbuf->page) != 1) ||
        kal_page_is_pfmemalloc(pbuf->page) ||
        page_to_nid(pbuf->page) != numa_mem_id()) {
        kal_dma_unmap_page_attrs(kdev->dev, pbuf->dma, rxq->page_size,
                                 DMA_FROM_DEVICE, NGKNET_RX_DMA_ATTR);
        pbuf->dma = 0;
    } else {
        pbuf->page_offset ^= rxq->page_size >> 1;
        page_ref_inc(pbuf->page);
        dma_sync_single_range_for_device(kdev->dev, pbuf->dma, pbuf->page_offset,
                                         rxq->page_size >> 1, DMA_FROM_DEVICE);
    }
}

#ifdef NGKNET_XDP_NATIVE
/*!
 * Run Rx XDP in zero-copy mode
 */
static int
ngknet_rx_run_xdp_zc(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                     struct pdma_rx_buf *pbuf, int len)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    struct net_device *ndev;
    struct xdp_buff *xdp = (struct xdp_buff *)pbuf->skb;
    struct pdma_hw *hw = (struct pdma_hw *)dev->ctrl.hw;
    uint32_t meta_len = hw->info.rx_ph_size;
    uint8_t fcs = dev->flags & PDMA_NO_FCS ? 0 : ETH_FCS_LEN;
    void *frame;
    struct pkt_hdr *pkh;
    uint32_t xv;
    int rv;

    frame = xdp->data - PKT_HDR_SIZE;
    pkh = &((struct pkt_buf *)frame)->pkh;
    pkh->data_len = len - meta_len;
    pkh->meta_len = meta_len;
    pkh->queue_id = rxq->queue_id;

    rv = ngknet_rx_xdp_filter(kdev, frame, &ndev);
    if (SHR_SUCCESS(rv) && ndev) {
        xdp->data_end = xdp->data + len - fcs;
        xdp->data_meta = xdp->data;
        xv = ngknet_run_xdp_zc(ndev, xdp);
        if (xv) {
            if (xv & (NGKNET_XDP_REDIR | NGKNET_XDP_TX | NGKNET_XDP_EXIT)) {
                return SHR_E_UNAVAIL;
            } else if (xv == NGKNET_XDP_BUSY) {
                return SHR_E_BUSY;
            } else {
                xsk_buff_free(xdp);
                return SHR_E_UNAVAIL;
            }
        }
    } else if (rv != SHR_E_NO_HANDLER) {
        xsk_buff_free(xdp);
        return SHR_E_UNAVAIL;
    }

    return SHR_E_NONE;
}

/*!
 * Run Rx XDP
 */
static int
ngknet_rx_run_xdp(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                  struct pdma_rx_buf *pbuf, int len)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    struct net_device *ndev;
    struct ngknet_private *priv;
    struct xdp_buff xdp;
    struct pdma_hw *hw = (struct pdma_hw *)dev->ctrl.hw;
    uint32_t meta_len = hw->info.rx_ph_size;
    uint8_t fcs = dev->flags & PDMA_NO_FCS ? 0 : ETH_FCS_LEN;
    void *frame;
    struct pkt_hdr *pkh;
    uint32_t xv = NGKNET_XDP_PASS;
    int rv;

    frame = page_address(pbuf->page) + pbuf->page_offset + PDMA_RXB_ALIGN;
    pkh = &((struct pkt_buf *)frame)->pkh;
    pkh->data_len = len - meta_len;
    pkh->meta_len = meta_len;
    pkh->queue_id = rxq->queue_id;

    rv = ngknet_rx_xdp_filter(kdev, frame, &ndev);
    if (SHR_SUCCESS(rv) && ndev) {
        priv = netdev_priv(ndev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,12,0))
        xdp_init_buff(&xdp, rxq->page_size >> 1, &priv->xri);
        xdp_prepare_buff(&xdp, page_address(pbuf->page) + pbuf->page_offset,
                         PDMA_RXB_RESV, len - fcs, true);
#else
        xdp.frame_sz = rxq->page_size >> 1;
        xdp.rxq = &priv->xri;
        xdp.data_hard_start = page_address(pbuf->page) + pbuf->page_offset;
        xdp.data = xdp.data_hard_start + PDMA_RXB_RESV;
        xdp.data_end = xdp.data + len - fcs;
        xdp.data_meta = xdp.data;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0))
        xdp_buff_clear_frags_flag(&xdp);
#endif
        xv = ngknet_run_xdp(ndev, &xdp);
    }

    if ((rv && rv != SHR_E_NO_HANDLER) ||
        (xv && !(xv & (NGKNET_XDP_REDIR | NGKNET_XDP_TX)))) {
        dma_sync_single_range_for_device(kdev->dev, pbuf->dma, pbuf->page_offset,
                                         rxq->page_size >> 1, DMA_FROM_DEVICE);
        return SHR_E_UNAVAIL;
    }

    if (xv & (NGKNET_XDP_REDIR | NGKNET_XDP_TX)) {
        ngknet_rx_page_reuse(dev, rxq, pbuf);
        return SHR_E_UNAVAIL;
    }

    return SHR_E_NONE;
}
#endif

/*!
 * Get Rx buffer
 */
static int
ngknet_rx_buf_get(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                  struct pdma_rx_buf *pbuf, int len)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    struct sk_buff *skb;

#ifdef NGKNET_XDP_NATIVE
    int rv;
    if (rxq->buf_mode == PDMA_BUF_MODE_MAPPED) {
        struct xdp_buff *xdp = (struct xdp_buff *)pbuf->skb;
        if (pbuf->dma) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,10,0))
            xsk_buff_dma_sync_for_cpu(xdp);
#else
            xsk_buff_dma_sync_for_cpu(xdp, kdev->xsk_pool);
#endif
            pbuf->dma = 0;
            xdp->data += RX_BUFF_RSV(pbuf);
        }
        if (kdev->xprog_num > 0) {
            rv = ngknet_rx_run_xdp_zc(dev, rxq, pbuf, len);
            if (SHR_FAILURE(rv)) {
                return rv;
            }
        }
        skb = netdev_alloc_skb(kdev->net_dev, RX_BUFF_RSV(pbuf) + rxq->buf_size);
        if (unlikely(!skb)) {
            return SHR_E_MEMORY;
        }
        memcpy(skb->data + RX_BUFF_RSV(pbuf), xdp->data, len);
        skb_reserve(skb, PDMA_RXB_ALIGN);
        xsk_buff_free(xdp);
        pbuf->skb = skb;
        pbuf->pkb = (struct pkt_buf *)skb->data;
    } else
#endif
    if (rxq->buf_mode == PDMA_BUF_MODE_PAGE) {
        if (pbuf->skb) {
            return SHR_E_NONE;
        }
        dma_sync_single_range_for_cpu(kdev->dev, pbuf->dma, pbuf->page_offset,
                                      rxq->page_size >> 1, DMA_FROM_DEVICE);
#ifdef NGKNET_XDP_NATIVE
        if (kdev->xprog_num > 0) {
            rv = ngknet_rx_run_xdp(dev, rxq, pbuf, len);
            if (SHR_FAILURE(rv)) {
                return rv;
            }
        }
#endif
        skb = kal_build_skb(page_address(pbuf->page) + pbuf->page_offset,
                            PDMA_RXB_SIZE(rxq->buf_size + pbuf->adj));
        if (unlikely(!skb)) {
            return SHR_E_MEMORY;
        }
        skb_reserve(skb, PDMA_RXB_ALIGN);
        pbuf->skb = skb;
        pbuf->pkb = (struct pkt_buf *)skb->data;
        ngknet_rx_page_reuse(dev, rxq, pbuf);
    } else {
        if (!pbuf->dma) {
            return SHR_E_NONE;
        }
        skb = pbuf->skb;
        dma_unmap_single(kdev->dev, pbuf->dma, rxq->buf_size, DMA_FROM_DEVICE);
        pbuf->dma = 0;
    }

    skb_put(skb, PKT_HDR_SIZE + pbuf->adj + len);

    return SHR_E_NONE;
}

/*!
 * Put Rx buffer
 */
static int
ngknet_rx_buf_put(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                  struct pdma_rx_buf *pbuf, int len)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    struct sk_buff *skb = pbuf->skb;

#ifdef NGKNET_XDP_NATIVE
    if (rxq->buf_mode == PDMA_BUF_MODE_MAPPED) {
        dev_kfree_skb_any(skb);
        pbuf->skb = NULL;
    } else
#endif
    if (rxq->buf_mode == PDMA_BUF_MODE_PAGE) {
        dev_kfree_skb_any(skb);
    } else {
        if (pbuf->pkb != (struct pkt_buf *)skb->data) {
            dev_kfree_skb_any(skb);
            pbuf->dma = 0;
            pbuf->skb = NULL;
            return SHR_E_NONE;
        }
        pbuf->dma = dma_map_single(kdev->dev, RX_BUFF_DMA(pbuf), rxq->buf_size,
                                   DMA_FROM_DEVICE);
        if (unlikely(dma_mapping_error(kdev->dev, pbuf->dma))) {
            dev_kfree_skb_any(skb);
            pbuf->dma = 0;
            pbuf->skb = NULL;
            return SHR_E_MEMORY;
        }
        skb_trim(skb, 0);
    }

    return SHR_E_NONE;
}

/*!
 * Free Rx buffer
 */
static void
ngknet_rx_buf_free(struct pdma_dev *dev, struct pdma_rx_queue *rxq,
                   struct pdma_rx_buf *pbuf)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;

#ifdef NGKNET_XDP_NATIVE
    if (rxq->buf_mode == PDMA_BUF_MODE_MAPPED) {
        struct xdp_buff *xdp = (struct xdp_buff *)pbuf->skb;
        if (!xdp) {
            return;
        }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,10,0))
        xsk_buff_dma_sync_for_cpu(xdp);
#else
        xsk_buff_dma_sync_for_cpu(xdp, kdev->xsk_pool);
#endif
        xsk_buff_free(xdp);
    } else
#endif
    if (rxq->buf_mode == PDMA_BUF_MODE_PAGE) {
        if (!pbuf->page) {
            return;
        }
        kal_dma_unmap_page_attrs(kdev->dev, pbuf->dma, rxq->page_size,
                                 DMA_FROM_DEVICE, NGKNET_RX_DMA_ATTR);
        __free_pages(pbuf->page, rxq->page_order);
    } else {
        if (!pbuf->skb) {
            return;
        }
        dma_unmap_single(kdev->dev, pbuf->dma, rxq->buf_size, DMA_FROM_DEVICE);
        dev_kfree_skb_any(pbuf->skb);
    }

    pbuf->dma = 0;
    pbuf->page = NULL;
    pbuf->page_offset = 0;
    pbuf->skb = NULL;
    pbuf->pkb = NULL;
    pbuf->adj = 0;
}

/*!
 * Get Rx buffer mode
 */
static enum buf_mode
ngknet_rx_buf_mode(struct pdma_dev *dev, struct pdma_rx_queue *rxq)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    uint32_t len, pgsz = PAGE_SIZE;
    int order = 0;

    if (kdev->flags & NGKNET_XSK_ZC) {
        return PDMA_BUF_MODE_MAPPED;
    } else if (ngknet_page_buffer_mode_get() == 0) {
        return PDMA_BUF_MODE_SKB;
    }

    len = dev->rx_ph_size ? rxq->buf_size : rxq->buf_size + PDMA_RXB_META;
    do {
        if (PDMA_RXB_SIZE(len) * 2 <= pgsz) {
            rxq->page_order = order;
            rxq->page_size = pgsz;
            break;
        }
        order++;
        pgsz *= 2;
    } while (1);

    return PDMA_BUF_MODE_PAGE;
}

/*!
 * Get Tx buffer
 */
static int
ngknet_tx_buf_get(struct pdma_dev *dev, struct pdma_tx_queue *txq,
                  struct pdma_tx_buf *pbuf, void *buf)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    struct pkt_buf *pkb = (struct pkt_buf *)*(unsigned long *)buf;
    struct sk_buff *skb;
    dma_addr_t dma;

#ifdef NGKNET_XDP_NATIVE
    if (pkb->pkh.attrs & PDMA_TX_XSK_ZC) {
        struct xsk_frame *xskf = container_of(buf, struct xsk_frame, data);
        struct xdp_desc *xdpd = xskf->desc;
        skb = (struct sk_buff *)xdpd;
        pbuf->pkb = pkb;
        pbuf->len = pkb->pkh.data_len + TX_META_LEN(pbuf);
        if (pkb->pkh.attrs & PDMA_TX_XDP_ACT) {
            dma = dma_map_single(kdev->dev, TX_BUFF_DMA(pbuf), pbuf->len,
                                 DMA_TO_DEVICE);
            if (unlikely(dma_mapping_error(kdev->dev, dma))) {
                return SHR_E_MEMORY;
            }
        } else {
            dma = xsk_buff_raw_get_dma(kdev->xsk_pool, xdpd->addr + PKT_HDR_SIZE);
            xsk_buff_raw_dma_sync_for_device(kdev->xsk_pool, dma, pbuf->len);
        }
    } else if (pkb->pkh.attrs & PDMA_TX_XDP_FRM) {
        struct xdp_frame *xdpf = container_of(buf, struct xdp_frame, data);
        skb = (struct sk_buff *)xdpf;
        pbuf->pkb = pkb;
        pbuf->len = pkb->pkh.data_len + TX_META_LEN(pbuf);
        dma = dma_map_single(kdev->dev, TX_BUFF_DMA(pbuf), pbuf->len,
                             DMA_TO_DEVICE);
        if (unlikely(dma_mapping_error(kdev->dev, dma))) {
            return SHR_E_MEMORY;
        }
    } else
#endif
    {
        skb = container_of(buf, struct sk_buff, data);
        pbuf->pkb = pkb;
        pbuf->len = pkb->pkh.data_len + TX_META_LEN(pbuf);
        dma = dma_map_single(kdev->dev, TX_BUFF_DMA(pbuf), pbuf->len,
                             DMA_TO_DEVICE);
        if (unlikely(dma_mapping_error(kdev->dev, dma))) {
            dev_kfree_skb_any(skb);
            return SHR_E_MEMORY;
        }
    }
    pbuf->dma = dma;
    pbuf->skb = skb;

    return SHR_E_NONE;
}

/*!
 * Get Tx buffer DMA address
 */
static void
ngknet_tx_buf_dma(struct pdma_dev *dev, struct pdma_tx_queue *txq,
                  struct pdma_tx_buf *pbuf, dma_addr_t *addr)
{
    *addr = pbuf->dma;
}

/*!
 * Free Tx buffer
 */
static void
ngknet_tx_buf_free(struct pdma_dev *dev, struct pdma_tx_queue *txq,
                   struct pdma_tx_buf *pbuf)
{
    struct ngknet_dev *kdev = (struct ngknet_dev *)dev->priv;
    struct pkt_buf *pkb = pbuf->pkb;
    struct sk_buff *skb = pbuf->skb;

    if (!skb || !pkb) {
        return;
    }

#ifdef NGKNET_XDP_NATIVE
    if (pkb->pkh.attrs & PDMA_TX_XSK_ZC) {
        if (pkb->pkh.attrs & PDMA_TX_XDP_ACT) {
            struct xdp_frame *xdpf = (struct xdp_frame *)skb;
            dma_unmap_single(kdev->dev, pbuf->dma, pbuf->len, DMA_TO_DEVICE);
            xdp_return_frame(xdpf);
        } else {
            xsk_tx_completed(kdev->xsk_pool, 1);
        }
    } else if (pkb->pkh.attrs & PDMA_TX_XDP_FRM) {
        struct xdp_frame *xdpf = (struct xdp_frame *)skb;
        dma_unmap_single(kdev->dev, pbuf->dma, pbuf->len, DMA_TO_DEVICE);
        if (pkb->pkh.attrs & PDMA_TX_XDP_ACT) {
            page_frag_free(xdpf->data);
        } else {
            xdp_return_frame(xdpf);
        }
    } else
#endif
    {
        dma_unmap_single(kdev->dev, pbuf->dma, pbuf->len, DMA_TO_DEVICE);
        dev_kfree_skb_any(skb);
    }

    pbuf->dma = 0;
    pbuf->len = 0;
    pbuf->skb = NULL;
    pbuf->pkb = NULL;
    pbuf->adj = 0;
}

static const struct pdma_buf_mngr buf_mngr = {
    .ring_buf_alloc     = ngknet_ring_buf_alloc,
    .ring_buf_free      = ngknet_ring_buf_free,
    .rx_buf_alloc       = ngknet_rx_buf_alloc,
    .rx_buf_dma         = ngknet_rx_buf_dma,
    .rx_buf_avail       = ngknet_rx_buf_avail,
    .rx_buf_get         = ngknet_rx_buf_get,
    .rx_buf_put         = ngknet_rx_buf_put,
    .rx_buf_free        = ngknet_rx_buf_free,
    .rx_buf_mode        = ngknet_rx_buf_mode,
    .tx_buf_get         = ngknet_tx_buf_get,
    .tx_buf_dma         = ngknet_tx_buf_dma,
    .tx_buf_free        = ngknet_tx_buf_free,
};

/*!
 * Open a device
 */
void
bcmcnet_buf_mngr_init(struct pdma_dev *dev)
{
    dev->ctrl.buf_mngr = (struct pdma_buf_mngr *)&buf_mngr;
}

