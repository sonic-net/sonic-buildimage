/*! \file bcmcnet_rxtx.c
 *
 * Utility routines for BCMCNET Rx/Tx.
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

/*!
 * Free a Rx ring
 */
static void
bcn_rx_ring_free(struct pdma_rx_queue *rxq)
{
    struct dev_ctrl *ctrl = rxq->ctrl;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)ctrl->buf_mngr;

    if (rxq->lock) {
        sal_spinlock_destroy(rxq->lock);
        rxq->lock = NULL;
    }

    if (rxq->ring) {
        bm->ring_buf_free(ctrl->dev, ctrl->rx_desc_size * (rxq->nb_desc + 1),
                          rxq->ring, rxq->ring_addr);
        rxq->ring = NULL;
    }

    if (rxq->pbuf) {
        sal_free(rxq->pbuf);
        rxq->pbuf = NULL;
    }
}

/*!
 * Allocate a Rx ring
 */
static int
bcn_rx_ring_alloc(struct pdma_rx_queue *rxq)
{
    struct dev_ctrl *ctrl = rxq->ctrl;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)ctrl->buf_mngr;

    /* Setup pktbuf ring */
    rxq->pbuf = sal_alloc(sizeof(*rxq->pbuf) * rxq->nb_desc, "bcmcnetRxBufRing");
    if (!rxq->pbuf) {
        goto cleanup;
    }
    sal_memset(rxq->pbuf, 0, sizeof(*rxq->pbuf) * rxq->nb_desc);

    /* Allocate memory for descriptors */
    rxq->ring = bm->ring_buf_alloc(ctrl->dev, ctrl->rx_desc_size * (rxq->nb_desc + 1),
                                   &rxq->ring_addr);
    if (!rxq->ring) {
        goto cleanup;
    }
    sal_memset(rxq->ring, 0, ctrl->rx_desc_size * (rxq->nb_desc + 1));

    rxq->lock = sal_spinlock_create("bcmcnetRxQueueLock");
    if (!rxq->lock) {
        goto cleanup;
    }

    return SHR_E_NONE;

cleanup:
    bcn_rx_ring_free(rxq);

    return SHR_E_MEMORY;
}

/*!
 * Free a Tx ring
 */
static void
bcn_tx_ring_free(struct pdma_tx_queue *txq)
{
    struct dev_ctrl *ctrl = txq->ctrl;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)ctrl->buf_mngr;

    if (txq->mutex) {
        sal_spinlock_destroy(txq->mutex);
        txq->mutex = NULL;
    }

    if (txq->lock) {
        sal_spinlock_destroy(txq->lock);
        txq->lock = NULL;
    }

    if (txq->ring) {
        bm->ring_buf_free(ctrl->dev, ctrl->tx_desc_size * (txq->nb_desc + 1),
                          txq->ring, txq->ring_addr);
        txq->ring = NULL;
    }

    if (txq->pbuf) {
        sal_free(txq->pbuf);
        txq->pbuf = NULL;
    }
}

/*!
 * Allocate a Tx ring
 */
static int
bcn_tx_ring_alloc(struct pdma_tx_queue *txq)
{
    struct dev_ctrl *ctrl = txq->ctrl;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)ctrl->buf_mngr;

    /* Setup pktbuf ring */
    txq->pbuf = sal_alloc(sizeof(*txq->pbuf) * txq->nb_desc, "bcmcnetTxBufRing");
    if (!txq->pbuf) {
        goto cleanup;
    }
    sal_memset(txq->pbuf, 0, sizeof(*txq->pbuf) * txq->nb_desc);

    /* Allocate memory for descriptors */
    txq->ring = bm->ring_buf_alloc(ctrl->dev, ctrl->tx_desc_size * (txq->nb_desc + 1),
                                   &txq->ring_addr);
    if (!txq->ring) {
        goto cleanup;
    }
    sal_memset(txq->ring, 0, ctrl->tx_desc_size * (txq->nb_desc + 1));

    txq->lock = sal_spinlock_create("bcmcnetTxQueueLock");
    if (!txq->lock) {
        goto cleanup;
    }

    txq->mutex = sal_spinlock_create("bcmcnetTxMutexLock");
    if (!txq->mutex) {
        goto cleanup;
    }

    return SHR_E_NONE;

cleanup:
    bcn_tx_ring_free(txq);

    return SHR_E_MEMORY;
}

/*!
 * Rx polling
 */
static int
bcn_rx_poll(struct pdma_rx_queue *rxq, int budget)
{
    struct dev_ctrl *ctrl = rxq->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    int rv;

    if (at_test_set_bit(rxq->queue_id, &ctrl->bm_rxq_busy, ctrl->lock)) {
        return 0;
    }

    rv = hw->dops.rx_ring_clean(hw, rxq, budget);

    at_clear_bit(rxq->queue_id, &ctrl->bm_rxq_busy, ctrl->lock);

    return rv;
}

/*!
 * Tx polling
 */
static int
bcn_tx_poll(struct pdma_tx_queue *txq, int budget)
{
    struct dev_ctrl *ctrl = txq->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    int rv;

    if (at_test_set_bit(txq->queue_id, &ctrl->bm_txq_busy, ctrl->lock)) {
        return 0;
    }

    rv = hw->dops.tx_ring_clean(hw, txq, budget);

    at_clear_bit(txq->queue_id, &ctrl->bm_txq_busy, ctrl->lock);

    return rv;
}

/*!
 * Setup a Rx queue
 */
int
bcmcnet_pdma_rx_queue_setup(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;
    int rv;

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    if (rxq->state & PDMA_RX_QUEUE_SETUP) {
        return SHR_E_NONE;
    }

    rv = bcn_rx_ring_alloc(rxq);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    rv = hw->dops.rx_desc_init(hw, rxq);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    if (dev->mode == DEV_MODE_VNET) {
        ctrl->vsync.rx_ring_addr[rxq->chan_id] = rxq->ring_addr;
        ctrl->vsync.rx_ring_size[rxq->chan_id] = rxq->nb_desc;
    }

    rxq->state |= PDMA_RX_QUEUE_SETUP;

    return SHR_E_NONE;
}

/*!
 * Release a Rx queue
 */
int
bcmcnet_pdma_rx_queue_release(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    if (rxq->state & PDMA_RX_QUEUE_SETUP) {
        hw->dops.rx_desc_clean(hw, rxq);
        bcn_rx_ring_free(rxq);
        rxq->state &= ~PDMA_RX_QUEUE_SETUP;
    }

    return SHR_E_NONE;
}

/*!
 * Restore a Rx queue
 */
int
bcmcnet_pdma_rx_queue_restore(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    if (rxq->state & PDMA_RX_QUEUE_SETUP) {
        hw->dops.rx_desc_init(hw, rxq);
    }

    return SHR_E_NONE;
}

/*!
 * Set up a virtual Rx queue
 */
int
bcmcnet_pdma_rx_vqueue_setup(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *vrxq = NULL;

    vrxq = (struct pdma_rx_queue *)ctrl->vnet_rxq[queue];
    if (vrxq->state & PDMA_RX_QUEUE_SETUP) {
        return SHR_E_NONE;
    }

    if (dev->ctrl.vsync.rx_ring_addr[vrxq->chan_id]) {
        vrxq->curr = 0;
        vrxq->nb_desc = dev->ctrl.vsync.rx_ring_size[vrxq->chan_id];
        vrxq->ring_addr = dev->ctrl.vsync.rx_ring_addr[vrxq->chan_id];
        vrxq->ring = dev->sys_p2v(dev, vrxq->ring_addr);
        vrxq->state |= PDMA_RX_QUEUE_SETUP;
    }

    return SHR_E_NONE;
}

/*!
 * Release a virtual Rx queue
 */
int
bcmcnet_pdma_rx_vqueue_release(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *vrxq = NULL;

    vrxq = (struct pdma_rx_queue *)ctrl->vnet_rxq[queue];
    if (vrxq->state & PDMA_RX_QUEUE_SETUP) {
        vrxq->state &= ~PDMA_RX_QUEUE_SETUP;
        vrxq->ring = NULL;
    }

    return SHR_E_NONE;
}

/*!
 * Setup a Tx queue
 */
int
bcmcnet_pdma_tx_queue_setup(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;
    int rv;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (txq->state & PDMA_TX_QUEUE_SETUP) {
        return SHR_E_NONE;
    }

    rv = bcn_tx_ring_alloc(txq);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    rv = hw->dops.tx_desc_init(hw, txq);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    if (dev->mode == DEV_MODE_VNET) {
        ctrl->vsync.tx_ring_addr[txq->chan_id] = txq->ring_addr;
        ctrl->vsync.tx_ring_size[txq->chan_id] = txq->nb_desc;
    }

    txq->state |= PDMA_TX_QUEUE_SETUP;

    return SHR_E_NONE;
}

/*!
 * Release a Tx queue
 */
int
bcmcnet_pdma_tx_queue_release(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (txq->state & PDMA_TX_QUEUE_SETUP) {
        hw->dops.tx_desc_clean(hw, txq);
        bcn_tx_ring_free(txq);
        txq->state &= ~PDMA_TX_QUEUE_SETUP;
    }

    return SHR_E_NONE;
}

/*!
 * Restore a Tx queue
 */
int
bcmcnet_pdma_tx_queue_restore(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (txq->state & PDMA_TX_QUEUE_SETUP) {
        hw->dops.tx_desc_init(hw, txq);
    }

    return SHR_E_NONE;
}

/*!
 * Set up a virtual Tx queue
 */
int
bcmcnet_pdma_tx_vqueue_setup(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *vtxq = NULL;

    vtxq = (struct pdma_tx_queue *)ctrl->vnet_txq[queue];
    if (vtxq->state & PDMA_TX_QUEUE_SETUP) {
        return SHR_E_NONE;
    }

    if (dev->ctrl.vsync.tx_ring_addr[vtxq->chan_id]) {
        vtxq->curr = 0;
        vtxq->dirt = 0;
        vtxq->nb_desc = dev->ctrl.vsync.tx_ring_size[vtxq->chan_id];
        vtxq->ring_addr = dev->ctrl.vsync.tx_ring_addr[vtxq->chan_id];
        vtxq->ring = dev->sys_p2v(dev, vtxq->ring_addr);
        vtxq->state |= PDMA_TX_QUEUE_SETUP;
    }

    return SHR_E_NONE;
}

/*!
 * Release a virtual Tx queue
 */
int
bcmcnet_pdma_tx_vqueue_release(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *vtxq = NULL;

    vtxq = (struct pdma_tx_queue *)ctrl->vnet_txq[queue];
    if (vtxq->state & PDMA_TX_QUEUE_SETUP) {
        vtxq->state &= ~PDMA_TX_QUEUE_SETUP;
        vtxq->ring = NULL;
    }

    return SHR_E_NONE;
}

/*!
 * Suspend a Rx queue
 */
int
bcmcnet_pdma_rx_queue_suspend(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    if (!rxq || !(rxq->state & PDMA_RX_QUEUE_ACTIVE)) {
        return SHR_E_UNAVAIL;
    }

    return hw->dops.rx_suspend(hw, rxq);
}

/*!
 * Resume a Rx queue
 */
int
bcmcnet_pdma_rx_queue_resume(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    if (!rxq || !(rxq->state & PDMA_RX_QUEUE_ACTIVE)) {
        return SHR_E_UNAVAIL;
    }

    return hw->dops.rx_resume(hw, rxq);
}

/*!
 * Suspend a Tx queue
 */
int
bcmcnet_pdma_tx_queue_suspend(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (!txq || !(txq->state & PDMA_TX_QUEUE_ACTIVE)) {
        return SHR_E_UNAVAIL;
    }

    if (txq->sem) {
        sal_sem_take(txq->sem, SAL_SEM_FOREVER);
    }
    if (dev->tx_suspend) {
        dev->tx_suspend(dev, txq->queue_id);
    }

    return SHR_E_NONE;
}

/*!
 * Resume a Tx queue
 */
int
bcmcnet_pdma_tx_queue_resume(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (!txq || !(txq->state & PDMA_TX_QUEUE_ACTIVE)) {
        return SHR_E_UNAVAIL;
    }

    if (txq->sem) {
        sal_sem_give(txq->sem);
    }
    if (dev->tx_resume) {
        dev->tx_resume(dev, txq->queue_id);
    }

    return SHR_E_NONE;
}

/*!
 * Wake up a Tx queue
 */
int
bcmcnet_pdma_tx_queue_wakeup(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (txq->sem) {
        sal_sem_give(txq->sem);
    }

    return SHR_E_NONE;
}

/*!
 * Transmit a outputing packet
 */
int
bcmcnet_pdma_tx_queue_xmit(struct pdma_dev *dev, int queue, void *buf)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (!txq || (!(txq->state & PDMA_TX_QUEUE_ACTIVE) && !dev->suspended)) {
        return SHR_E_DISABLED;
    }

    return hw->dops.pkt_xmit(hw, txq, buf);
}

/*!
 * Poll a Rx queues
 */
int
bcmcnet_pdma_rx_queue_poll(struct pdma_dev *dev, int queue, int budget)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = ctrl->rx_queue[queue];

    return bcn_rx_poll(rxq, budget);
}

/*!
 * Poll a Tx queues
 */
int
bcmcnet_pdma_tx_queue_poll(struct pdma_dev *dev, int queue, int budget)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = ctrl->tx_queue[queue];

    return bcn_tx_poll(txq, budget);
}

/*!
 * Poll for Rx/Tx queues in a group
 */
int
bcmcnet_pdma_group_poll(struct pdma_dev *dev, int group, int budget)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;
    struct queue_group *grp = &ctrl->grp[group];
    uint32_t intr_actives = 0;
    int rx_done = 0, tx_done = 0, done_que, budget_que;
    int i;

    /* Acknowledge the interrupts */
    for (i = 0; i < dev->grp_queues; i++) {
        rxq = grp->rx_queue[i];
        if (rxq->state & PDMA_RX_QUEUE_ACTIVE) {
            if (hw->hdls.chan_intr_query(hw, rxq->chan_id)) {
                hw->hdls.chan_clear(hw, rxq->chan_id);
                grp->poll_queues |= 1 << i;
                intr_actives |= 1 << i;
            }
            if (rxq->state & PDMA_RX_QUEUE_BUSY) {
                rxq->state &= ~PDMA_RX_QUEUE_BUSY;
                grp->poll_queues |= 1 << i;
            }
            continue;
        }
        txq = grp->tx_queue[i];
        if (txq->state & PDMA_TX_QUEUE_ACTIVE) {
            if (hw->hdls.chan_intr_query(hw, txq->chan_id)) {
                hw->hdls.chan_clear(hw, txq->chan_id);
                grp->poll_queues |= 1 << i;
                intr_actives |= 1 << i;
            }
            if (txq->state & PDMA_TX_QUEUE_BUSY) {
                txq->state &= ~PDMA_TX_QUEUE_BUSY;
                grp->poll_queues |= 1 << i;
            }
        }
    }

    /* Calculate per queue budget */
    if (!grp->poll_queues) {
        grp->poll_queues = grp->bm_rxq | grp->bm_txq;
        budget_que = budget / grp->nb_rxq;
    } else {
        budget_que = 0;
        for (i = 0; i < dev->grp_queues; i++) {
            if (1 << i & grp->bm_rxq & grp->poll_queues) {
                budget_que++;
            }
        }
        if (budget_que) {
            budget_que = budget / budget_que;
        }
    }

    /* Poll Rx queues */
    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_rxq & grp->poll_queues) {
            rxq = grp->rx_queue[i];
            done_que = bcn_rx_poll(rxq, budget_que);
            if (done_que >= budget_que ||
                (done_que == 0 && (1 << i & intr_actives))) {
                continue;
            }
            grp->poll_queues &= ~(1 << i);
            rx_done += done_que;
        }
    }

    /* Poll Tx queues */
    for (i = 0; i < dev->grp_queues; i++) {
        txq = grp->tx_queue[i];
        if (1 << i & grp->bm_txq & grp->poll_queues && !txq->free_thresh) {
            done_que = bcn_tx_poll(txq, budget);
            if (done_que >= budget ||
                (done_que == 0 && (1 << i & intr_actives))) {
                continue;
            }
            grp->poll_queues &= ~(1 << i);
            tx_done += done_que;
        }
    }

    /* Reschedule the poll if not completed */
    if (grp->poll_queues) {
        return budget;
    }

    /* Check channel status before exits */
    if (hw->hdls.chan_check) {
        for (i = 0; i < dev->grp_queues; i++) {
            rxq = grp->rx_queue[i];
            if (rxq->state & PDMA_RX_QUEUE_ACTIVE) {
                if (hw->hdls.chan_check(hw, rxq->chan_id)) {
                    hw->hdls.chan_clear(hw, rxq->chan_id);
                    grp->poll_queues |= 1 << i;
                }
                continue;
            }
            txq = grp->tx_queue[i];
            if (txq->state & PDMA_TX_QUEUE_ACTIVE) {
                if (hw->hdls.chan_check(hw, txq->chan_id)) {
                    hw->hdls.chan_clear(hw, txq->chan_id);
                    grp->poll_queues |= 1 << i;
                }
            }
        }
        return grp->poll_queues ? budget : rx_done;
    } else {
        return (rx_done + tx_done) ? budget : 0;
    }
}

/*!
 * Dump a Rx ring
 */
int
bcmcnet_pdma_rx_ring_dump(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= ctrl->nb_rxq) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    if (rxq->state & PDMA_RX_QUEUE_ACTIVE) {
        hw->dops.rx_ring_dump(hw, rxq);
    }
    if (dev->mode == DEV_MODE_HNET) {
        rxq = (struct pdma_rx_queue *)ctrl->vnet_rxq[queue];
        if (rxq->state & PDMA_RX_QUEUE_SETUP) {
            hw->dops.rx_ring_dump(hw, rxq);
        }
    }

    return SHR_E_NONE;
}

/*!
 * Dump a Tx ring
 */
int
bcmcnet_pdma_tx_ring_dump(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    if ((uint32_t)queue >= ctrl->nb_txq) {
        return SHR_E_PARAM;
    }

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    if (txq->state & PDMA_TX_QUEUE_ACTIVE) {
        hw->dops.tx_ring_dump(hw, txq);
    }
    if (dev->mode == DEV_MODE_HNET) {
        txq = (struct pdma_tx_queue *)ctrl->vnet_txq[queue];
        if (txq->state & PDMA_TX_QUEUE_SETUP) {
            hw->dops.tx_ring_dump(hw, txq);
        }
    }

    return SHR_E_NONE;
}

