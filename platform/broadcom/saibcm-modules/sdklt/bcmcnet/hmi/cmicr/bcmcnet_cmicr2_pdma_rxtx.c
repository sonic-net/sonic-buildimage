/*! \file bcmcnet_cmicr2_pdma_rxtx.c
 *
 * Utility routines for BCMCNET hardware (CMICr2) specific Tx.
 * All others leverage CMICr stuffs.
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
#include <bcmcnet/bcmcnet_buff.h>
#include <bcmcnet/bcmcnet_cmicr2.h>

#define TX_DCB_STAT_SET(r, f)           TX_DCB_SET(r, 3, f)
#define TX_DCB_CTRL_HGf_SET(r)          TX_DCB_HGf_SET(r, 1)
#define TX_DCB_CTRL_HGf_CLR(r)          TX_DCB_HGf_SET(r, 0)
#define TX_DCB_CTRL_PURGEf_SET(r)       (r).tx_dcb[2] |= 1 << 23
#define TX_DCB_CTRL_PURGEf_CLR(r)       (r).tx_dcb[2] &= ~(1 << 23)
#define TX_DCB_CTRL_PROFf_SET(r, f)     (r).tx_dcb[2] = ((r).tx_dcb[2] & ~(0x7 << 20)) | (((f) & 0x7) << 20)

/*!
 * Configure Tx descriptor
 */
static inline void
cmicr2_tx_desc_config(volatile TX_DCB_t *td, uint64_t addr, uint32_t len,
                      uint32_t prof, uint16_t flags)
{
    TX_DCB_ADDR_LOf_SET(*td, addr);
    TX_DCB_ADDR_HIf_SET(*td, DMA_TO_BUS_HI(addr >> 32));
    TX_DCB_STAT_SET(*td, 0);

    if (flags & PDMA_TX_HIGIG_PKT) {
        TX_DCB_CTRL_HGf_SET(*td);
    } else {
        TX_DCB_CTRL_HGf_CLR(*td);
    }
    if (flags & PDMA_TX_PURGE_PKT) {
        TX_DCB_CTRL_PURGEf_SET(*td);
    } else {
        TX_DCB_CTRL_PURGEf_CLR(*td);
    }
    TX_DCB_CTRL_PROFf_SET(*td, prof);
    TX_DCB_BYTE_COUNTf_SET(*td, len);

    MEMORY_BARRIER;
}

/*!
 * Chain Tx descriptor
 */
static inline void
cmicr2_tx_desc_chain(volatile TX_DCB_t *td, int chain)
{
    if (chain) {
        TX_DCB_CHAINf_SET(*td, 1);
    } else {
        TX_DCB_CHAINf_SET(*td, 0);
    }

    MEMORY_BARRIER;
}

/*!
 * Get unused descriptors in a Tx ring
 */
static inline int
cmicr2_pdma_tx_ring_unused(struct pdma_tx_queue *txq)
{
    /* Leave one descriptor unused so as not to halt */
    return (txq->nb_desc + txq->dirt - txq->curr - 1) % txq->nb_desc;
}

/*!
 * Fetch Tx vring
 */
static int
cmicr2_pdma_tx_vring_fetch(struct pdma_hw *hw, struct pdma_tx_queue *txq,
                           struct pdma_tx_buf *pbuf)
{
    struct pdma_dev *dev = hw->dev;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    struct pdma_tx_queue *vtxq = NULL;
    volatile TX_DCB_t *vring = NULL;
    uint32_t rm;

    vtxq = (struct pdma_tx_queue *)dev->ctrl.vnet_txq[txq->queue_id];
    vring = (volatile TX_DCB_t *)vtxq->ring;
    if (!vring || !TX_DCB_BYTE_COUNTf_GET(vring[vtxq->curr])) {
        return SHR_E_UNAVAIL;
    }

    /* Fetch vring descriptor */
    rm = TX_DCB_DESC_REMAINf_GET(ring[txq->curr]);
    TX_DCB_SET(ring[txq->curr], 0, TX_DCB_GET(vring[vtxq->curr], 0));
    TX_DCB_SET(ring[txq->curr], 1, TX_DCB_GET(vring[vtxq->curr], 1));
    TX_DCB_SET(ring[txq->curr], 2, TX_DCB_GET(vring[vtxq->curr], 2));
    TX_DCB_SET(ring[txq->curr], 3, TX_DCB_GET(vring[vtxq->curr], 3));
    TX_DCB_DESC_REMAINf_SET(ring[txq->curr], rm);
    TX_DCB_BYTE_COUNTf_SET(vring[vtxq->curr], 0);

    MEMORY_BARRIER;

    pbuf->dma = TX_DCB_ADDR_LOf_GET(vring[vtxq->curr]);
    pbuf->len = TX_DCB_BYTE_COUNTf_GET(ring[txq->curr]);
    vtxq->curr = (vtxq->curr + 1) % vtxq->nb_desc;

    return SHR_E_NONE;
}

/*!
 * Check Tx ring
 */
static inline int
cmicr2_pdma_tx_ring_check(struct pdma_hw *hw, struct pdma_tx_queue *txq)
{
    struct pdma_dev *dev = hw->dev;

    if (dev->suspended) {
        txq->stats.xoffs++;
        if (dev->tx_suspend) {
            dev->tx_suspend(dev, txq->queue_id);
            return SHR_E_BUSY;
        }
        if (!(txq->state & PDMA_TX_QUEUE_POLL)) {
            return SHR_E_BUSY;
        }
    }

    if (cmicr2_pdma_tx_ring_unused(txq)) {
        return SHR_E_NONE;
    }

    sal_spinlock_lock(txq->lock);
    if (!cmicr2_pdma_tx_ring_unused(txq)) {
        txq->status |= PDMA_TX_QUEUE_XOFF;
        txq->stats.xoffs++;
        if (dev->tx_suspend) {
            dev->tx_suspend(dev, txq->queue_id);
        }
        sal_spinlock_unlock(txq->lock);
        return SHR_E_BUSY;
    }
    sal_spinlock_unlock(txq->lock);

    return SHR_E_NONE;
}

/*!
 * \brief Start packet transmission
 *
 * \param [in] hw HW structure point.
 * \param [in] txq Tx queue structure point.
 * \param [in] buf Tx packet buffer.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
cmicr2_pdma_pkt_xmit(struct pdma_hw *hw, struct pdma_tx_queue *txq, void *buf)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    struct pdma_tx_buf *pbuf = NULL;
    struct pkt_hdr *pkh = NULL;
    dma_addr_t addr;
    uint32_t curr;
    int retry = 5000000;
    int rv;

    if (dev->tx_suspend) {
        sal_spinlock_lock(txq->mutex);
    } else {
        rv = sal_sem_take(txq->sem, BCMCNET_TX_RSRC_WAIT_USEC);
        if (rv == -1) {
            CNET_ERROR(hw->unit, "Timeout waiting for Tx resources\n");
            return SHR_E_TIMEOUT;
        }
    }

    /* Check Tx resource */
    if (dev->tx_suspend) {
        /* Suspend Tx if no resource */
        rv = cmicr2_pdma_tx_ring_check(hw, txq);
        if (SHR_FAILURE(rv)) {
            sal_spinlock_unlock(txq->mutex);
            return rv;
        }
    } else {
        /* Abort Tx if a fatal error happened */
        if (txq->status & PDMA_TX_QUEUE_XOFF) {
            sal_sem_give(txq->sem);
            return SHR_E_RESOURCE;
        }
    }

    /* Setup the new descriptor */
    curr = txq->curr;
    pbuf = &txq->pbuf[curr];
    if (dev->mode == DEV_MODE_HNET && !buf) {
        rv = cmicr2_pdma_tx_vring_fetch(hw, txq, pbuf);
        if (SHR_FAILURE(rv)) {
            sal_spinlock_unlock(txq->mutex);
            return SHR_E_EMPTY;
        }
        txq->state |= PDMA_TX_QUEUE_BUSY;
    } else {
        rv = bm->tx_buf_get(dev, txq, pbuf, buf);
        if (SHR_FAILURE(rv)) {
            txq->stats.dropped++;
            if (dev->tx_suspend) {
                sal_spinlock_unlock(txq->mutex);
            } else {
                sal_sem_give(txq->sem);
            }
            return SHR_E_RESOURCE;
        }
        bm->tx_buf_dma(dev, txq, pbuf, &addr);
        pkh = &pbuf->pkb->pkh;
        cmicr2_tx_desc_config(&ring[curr], addr, pbuf->len, pkh->hdr_prof, pkh->attrs);
    }

    /* Notify HNET to process if needed */
    if (dev->mode == DEV_MODE_VNET) {
        if (!TX_DCB_BYTE_COUNTf_GET(ring[(curr + txq->nb_desc - 1) % txq->nb_desc])) {
            dev->xnet_wake(dev);
        }
    }

    /* Update the indicators */
    curr = (curr + 1) % txq->nb_desc;
    txq->curr = curr;

    /* Start DMA if in chain mode */
    if (dev->flags & PDMA_CHAIN_MODE) {
        if (txq->state & PDMA_TX_QUEUE_POLL) {
            do {
                rv = hw->dops.tx_ring_clean(hw, txq, txq->nb_desc - 1);
                if (rv != (int)txq->nb_desc - 1) {
                    break;
                }
                sal_usleep(1);
            } while (retry--);
            if (retry < 0) {
                CNET_ERROR(hw->unit, "Last Tx could not get done in given time\n");
            }
        }
        sal_spinlock_lock(txq->lock);
        if (txq->dirt == txq->halt && txq->dirt != curr) {
            hw->hdls.chan_stop(hw, txq->chan_id);
            cmicr2_tx_desc_chain(&ring[(curr + txq->nb_desc - 1) % txq->nb_desc], 0);
            hw->hdls.chan_setup(hw, txq->chan_id,
                                txq->ring_addr + sizeof(TX_DCB_t) * txq->halt);
            hw->hdls.chan_start(hw, txq->chan_id);
            txq->halt = curr;
        }
        sal_spinlock_unlock(txq->lock);
    }

    /* Kick off DMA */
    txq->halt_addr = txq->ring_addr + sizeof(TX_DCB_t) * curr;
    hw->hdls.chan_goto(hw, txq->chan_id, txq->halt_addr);

    /* Count the packets/bytes */
    txq->stats.packets++;
    txq->stats.bytes += pbuf->len;

    /* Clean up ring if in polling mode */
    if (txq->state & PDMA_TX_QUEUE_POLL &&
        cmicr2_pdma_tx_ring_unused(txq) <= (int)txq->free_thresh) {
        hw->dops.tx_ring_clean(hw, txq, dev->ctrl.budget);
    }

    /* Suspend Tx if no resource */
    rv = cmicr2_pdma_tx_ring_check(hw, txq);
    if (SHR_FAILURE(rv)) {
        if (dev->mode == DEV_MODE_VNET) {
            dev->xnet_wake(dev);
        }

        if (txq->state & PDMA_TX_QUEUE_POLL) {
            /* In polling mode, must wait till the ring is available */
            do {
                hw->dops.tx_ring_clean(hw, txq, dev->ctrl.budget);
                if (!(txq->status & PDMA_TX_QUEUE_XOFF) ||
                    !(txq->state & PDMA_TX_QUEUE_ACTIVE)) {
                    break;
                }
                sal_usleep(1);
            } while (retry--);
            if (retry < 0) {
                CNET_ERROR(hw->unit, "Fatal error: Tx ring is full, packets can not been transmitted\n");
                if (!dev->tx_suspend) {
                    sal_sem_give(txq->sem);
                    return SHR_E_RESOURCE;
                }
            }
        } else {
            /* In interrupt mode, the handle thread will wake up Tx */
            if (!dev->tx_suspend) {
                return SHR_E_NONE;
            }
        }
    }

    if (dev->tx_suspend) {
        sal_spinlock_unlock(txq->mutex);
    } else {
        sal_sem_give(txq->sem);
    }

    return SHR_E_NONE;
}

/*!
 * Attach device driver
 */
int
bcmcnet_cmicr2_pdma_driver_attach(struct pdma_dev *dev)
{
    struct pdma_hw *hw = NULL;

    /* Allocate memory for HW data */
    hw = sal_alloc(sizeof(*hw), "bcmcnetPdmaHw");
    if (!hw) {
        return SHR_E_MEMORY;
    }
    sal_memset(hw, 0, sizeof(*hw));
    hw->unit = dev->unit;
    hw->dev = dev;
    dev->ctrl.hw = hw;

    bcmcnet_cmicr_pdma_hw_hdls_init(hw);
    bcmcnet_cmicr_pdma_desc_ops_init(hw);

    hw->dops.pkt_xmit = cmicr2_pdma_pkt_xmit;

    dev->flags |= PDMA_NO_FCS;

    return SHR_E_NONE;
}

/*!
 * Detach device driver
 */
int
bcmcnet_cmicr2_pdma_driver_detach(struct pdma_dev *dev)
{
    if (dev->ctrl.hw) {
        sal_free(dev->ctrl.hw);
    }
    dev->ctrl.hw = NULL;

    return SHR_E_NONE;
}
