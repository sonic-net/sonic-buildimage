/*! \file bcmcnet_core.c
 *
 * Utility routines for BCMCNET driver.
 *
 */
/*
 * Copyright 2018-2024 Broadcom. All rights reserved.
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

/*!
 * Initialize a device
 */
int
bcmcnet_pdma_dev_init(struct pdma_dev *dev)
{
    int rv;

    /* Open the device */
    rv = bcmcnet_pdma_open(dev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    dev->attached = true;

    return SHR_E_NONE;
}

/*!
 * Clean up a device
 */
int
bcmcnet_pdma_dev_cleanup(struct pdma_dev *dev)
{
    if (!dev->attached) {
        return SHR_E_NONE;
    }

    dev->ops->dev_close(dev);
    dev->ops = NULL;

    dev->attached = false;

    return SHR_E_NONE;
}

/*!
 * Start a device
 */
int
bcmcnet_pdma_dev_start(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;
    int rv;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    if (dev->started) {
        return SHR_E_NONE;
    }

    rv = dev->ops->dev_config(dev, ctrl->bm_rxq, ctrl->bm_txq);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    dev->started = true;

    /* Start all the Rx queues */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rv = dev->ops->rx_queue_setup(dev, qi);
        if (SHR_FAILURE(rv)) {
            return rv;
        }
        dev->ops->rx_queue_intr_enable(dev, qi);
        dev->ops->rx_queue_start(dev, qi);
    }

    /* Start all the Tx queues */
    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        rv = dev->ops->tx_queue_setup(dev, qi);
        if (SHR_FAILURE(rv)) {
            return rv;
        }
        dev->ops->tx_queue_intr_enable(dev, qi);
        dev->ops->tx_queue_start(dev, qi);
    }

    bcmcnet_pdma_dev_info_get(dev);

    return SHR_E_NONE;
}

/*!
 * Stop a device
 */
int
bcmcnet_pdma_dev_stop(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    if (!dev->started) {
        return SHR_E_NONE;
    }

    /* Stop all the Rx queues */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        dev->ops->rx_queue_stop(dev, qi);
        dev->ops->rx_queue_release(dev, qi);
    }

    /* Stop all the Tx queues */
    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        dev->ops->tx_queue_stop(dev, qi);
        dev->ops->tx_queue_release(dev, qi);
    }

    dev->started = false;

    /* Disable all the Rx interrupts */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        dev->ops->rx_queue_intr_disable(dev, qi);
    }

    /* Disable all the Tx interrupts */
    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        dev->ops->tx_queue_intr_disable(dev, qi);
    }

    return SHR_E_NONE;
}

/*!
 * Suspend a device
 */
int
bcmcnet_pdma_dev_suspend(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;
    int rv;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    dev->suspended = true;

    rv = dev->ops->dev_suspend(dev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    if (dev->flags & PDMA_ABORT) {
        /* Abort all the Tx queues */
        for (qi = 0; qi < ctrl->nb_txq; qi++) {
            dev->ops->tx_queue_stop(dev, qi);
        }
        /* Abort all the Rx queues */
        for (qi = 0; qi < ctrl->nb_rxq; qi++) {
            dev->ops->rx_queue_stop(dev, qi);
        }
    }

    return SHR_E_NONE;
}

/*!
 * Resume a device
 */
int
bcmcnet_pdma_dev_resume(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;
    int rv;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    dev->suspended = false;

    if (dev->flags & PDMA_ABORT) {
        /*
         * H/W configuration of Packet DMA is gone in the FFB apply phase,
         * so we need to program it again.
         */
        dev->ops->dev_config(dev, ctrl->bm_rxq, ctrl->bm_txq);

        /* Restart all the Rx queues */
        for (qi = 0; qi < ctrl->nb_rxq; qi++) {
            dev->ops->rx_queue_release(dev, qi);
            dev->ops->rx_queue_setup(dev, qi);
            dev->ops->rx_queue_intr_enable(dev, qi);
            dev->ops->rx_queue_start(dev, qi);
        }
        /* Restart all the Tx queues */
        for (qi = 0; qi < ctrl->nb_txq; qi++) {
            dev->ops->tx_queue_release(dev, qi);
            dev->ops->tx_queue_setup(dev, qi);
            dev->ops->tx_queue_intr_enable(dev, qi);
            dev->ops->tx_queue_start(dev, qi);
        }
    }

    rv = dev->ops->dev_resume(dev);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    if (dev->flags & PDMA_ABORT) {
        dev->flags &= ~PDMA_ABORT;
    }

    return rv;
}

/*!
 * Suspend Rx
 */
int
bcmcnet_pdma_dev_rx_suspend(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    /* Suspend all the Rx queues */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        dev->ops->rx_queue_suspend(dev, qi);
    }

    return SHR_E_NONE;
}

/*!
 * Resume Rx
 */
int
bcmcnet_pdma_dev_rx_resume(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    /* Resume all the Rx queues */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        dev->ops->rx_queue_resume(dev, qi);
    }

    return SHR_E_NONE;
}

/*!
 * Dock to HNET
 */
int
bcmcnet_pdma_dev_dock(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;
    int rv;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    /* Set up all the virtual Rx queues */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rv = dev->ops->rx_vqueue_setup(dev, qi);
        if (SHR_FAILURE(rv)) {
            return rv;
        }
    }

    /* Set up all the virtual Tx queues */
    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        rv = dev->ops->tx_vqueue_setup(dev, qi);
        if (SHR_FAILURE(rv)) {
            return rv;
        }
    }

    return SHR_E_NONE;
}

/*!
 * Undock from HNET
 */
int
bcmcnet_pdma_dev_undock(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;

    if (!dev->attached) {
        return SHR_E_UNAVAIL;
    }

    /* Release all the virtual Rx queues */
    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        dev->ops->rx_vqueue_release(dev, qi);
    }

    /* Release all the virtual Tx queues */
    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        dev->ops->tx_vqueue_release(dev, qi);
    }

    return SHR_E_NONE;
}

/*!
 * Get device information
 */
int
bcmcnet_pdma_dev_info_get(struct pdma_dev *dev)
{
    if (!dev->ops || !dev->ops->dev_info_get) {
        return SHR_E_INTERNAL;
    }

    dev->ops->dev_info_get(dev);

    return SHR_E_NONE;
}

/*!
 * Get device statistics
 */
int
bcmcnet_pdma_dev_stats_get(struct pdma_dev *dev)
{
    if (!dev->ops || !dev->ops->dev_stats_get) {
        return SHR_E_INTERNAL;
    }

    dev->ops->dev_stats_get(dev);

    return SHR_E_NONE;
}

/*!
 * Reset device statistics
 */
int
bcmcnet_pdma_dev_stats_reset(struct pdma_dev *dev, pdma_dir_t dir)
{
    if (!dev->ops || !dev->ops->dev_stats_reset) {
        return SHR_E_INTERNAL;
    }

    dev->ops->dev_stats_reset(dev, dir);

    return SHR_E_NONE;
}

/*!
 * Convert a queue index to channel index
 */
int
bcmcnet_pdma_dev_queue_to_chan(struct pdma_dev *dev, int queue, int dir, int *chan)
{
    struct dev_ctrl *ctrl = &dev->ctrl;

    if (dir == PDMA_Q_RX) {
        if ((uint32_t)queue >= ctrl->nb_rxq || chan == NULL) {
            return SHR_E_PARAM;
        }
    } else {
        if ((uint32_t)queue >= ctrl->nb_txq || chan == NULL) {
            return SHR_E_PARAM;
        }
    }

    if (!dev->ops || !dev->ops->dev_lq_to_pq) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->dev_lq_to_pq(dev, queue, dir, chan);
}

/*!
 * Convert a channel index to queue index
 */
int
bcmcnet_pdma_dev_chan_to_queue(struct pdma_dev *dev, int chan, int *queue, int *dir)
{
    if (chan < 0 || chan >= dev->num_queues || queue == NULL || dir == NULL) {
        return SHR_E_PARAM;
    }

    if (!dev->ops || !dev->ops->dev_pq_to_lq) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->dev_pq_to_lq(dev, chan, queue, dir);
}

/*!
 * Enable interrupt for a Rx queue
 */
int
bcmcnet_rx_queue_intr_enable(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->rx_queue_intr_enable) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->rx_queue_intr_enable(dev, queue);
}

/*!
 * Disable interrupt for a Rx queue
 */
int
bcmcnet_rx_queue_intr_disable(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->rx_queue_intr_disable) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->rx_queue_intr_disable(dev, queue);
}

/*!
 * Acknowledge interrupt for a Rx queue
 */
int
bcmcnet_rx_queue_intr_ack(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->rx_queue_intr_ack) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->rx_queue_intr_ack(dev, queue);
}

/*!
 * Check interrupt for a Rx queue
 */
int
bcmcnet_rx_queue_intr_check(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->rx_queue_intr_check) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->rx_queue_intr_check(dev, queue);
}

/*!
 * Enable interrupt for a Tx queue
 */
int
bcmcnet_tx_queue_intr_enable(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->tx_queue_intr_enable) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->tx_queue_intr_enable(dev, queue);
}

/*!
 * Disable interrupt for a Tx queue
 */
int
bcmcnet_tx_queue_intr_disable(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->tx_queue_intr_disable) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->tx_queue_intr_disable(dev, queue);
}

/*!
 * Acknowledge interrupt for a Tx queue
 */
int
bcmcnet_tx_queue_intr_ack(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->tx_queue_intr_ack) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->tx_queue_intr_ack(dev, queue);
}

/*!
 * Check interrupt for a Tx queue
 */
int
bcmcnet_tx_queue_intr_check(struct pdma_dev *dev, int queue)
{
    if (!dev->ops || !dev->ops->tx_queue_intr_check) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->tx_queue_intr_check(dev, queue);
}

/*!
 * Enable interrupt for a queue
 */
int
bcmcnet_queue_intr_enable(struct pdma_dev *dev, struct intr_handle *hdl)
{
    if (hdl->dir == PDMA_Q_RX) {
        return bcmcnet_rx_queue_intr_enable(dev, hdl->queue);
    } else {
        return bcmcnet_tx_queue_intr_enable(dev, hdl->queue);
    }
}

/*!
 * Disable interrupt for a queue
 */
int
bcmcnet_queue_intr_disable(struct pdma_dev *dev, struct intr_handle *hdl)
{
    if (hdl->dir == PDMA_Q_RX) {
        return bcmcnet_rx_queue_intr_disable(dev, hdl->queue);
    } else {
        return bcmcnet_tx_queue_intr_disable(dev, hdl->queue);
    }
}

/*!
 * Acknowledge interrupt for a queue
 */
int
bcmcnet_queue_intr_ack(struct pdma_dev *dev, struct intr_handle *hdl)
{
    if (hdl->dir == PDMA_Q_RX) {
        return bcmcnet_rx_queue_intr_ack(dev, hdl->queue);
    } else {
        return bcmcnet_tx_queue_intr_ack(dev, hdl->queue);
    }
}

/*!
 * Check interrupt for a queue
 */
int
bcmcnet_queue_intr_check(struct pdma_dev *dev, struct intr_handle *hdl)
{
    if (hdl->dir == PDMA_Q_RX) {
        return bcmcnet_rx_queue_intr_check(dev, hdl->queue);
    } else {
        return bcmcnet_tx_queue_intr_check(dev, hdl->queue);
    }
}

/*!
 * Enable interrupt for a queue group
 */
int
bcmcnet_group_intr_enable(struct pdma_dev *dev, int group)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct queue_group *grp = &ctrl->grp[group];
    int queue, dir;
    int i;

    if (!dev->ops) {
        return SHR_E_INTERNAL;
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_rxq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            dev->ops->rx_queue_intr_enable(dev, queue);
        }
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_txq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            dev->ops->tx_queue_intr_enable(dev, queue);
        }
    }

    return SHR_E_NONE;
}

/*!
 * Disable interrupt for a queue group
 */
int
bcmcnet_group_intr_disable(struct pdma_dev *dev, int group)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct queue_group *grp = &ctrl->grp[group];
    int queue, dir;
    int i;

    if (!dev->ops) {
        return SHR_E_INTERNAL;
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_rxq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            dev->ops->rx_queue_intr_disable(dev, queue);
        }
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_txq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            dev->ops->tx_queue_intr_disable(dev, queue);
        }
    }

    return SHR_E_NONE;
}

/*!
 * Acknowledge interrupt for a queue group
 */
int
bcmcnet_group_intr_ack(struct pdma_dev *dev, int group)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct queue_group *grp = &ctrl->grp[group];
    int queue, dir;
    int i;

    if (!dev->ops) {
        return SHR_E_INTERNAL;
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_rxq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            dev->ops->rx_queue_intr_ack(dev, queue);
        }
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_txq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            dev->ops->tx_queue_intr_ack(dev, queue);
        }
    }

    return SHR_E_NONE;
}

/*!
 * Check interrupt for a queue group
 */
bool
bcmcnet_group_intr_check(struct pdma_dev *dev, int group)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct queue_group *grp = &ctrl->grp[group];
    int queue, dir;
    int i;

    if (!dev->ops) {
        return false;
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_rxq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            if (dev->ops->rx_queue_intr_check(dev, queue)) {
                return true;
            }
        }
    }

    for (i = 0; i < dev->grp_queues; i++) {
        if (1 << i & grp->bm_txq) {
            dev->ops->dev_pq_to_lq(dev, i + group * dev->grp_queues, &queue, &dir);
            if (dev->ops->tx_queue_intr_check(dev, queue)) {
                return true;
            }
        }
    }

    return false;
}

/*!
 * Poll a Rx queue
 */
int
bcmcnet_rx_queue_poll(struct pdma_dev *dev, int queue, int budget)
{
    if (!dev->started) {
        return SHR_E_NONE;
    }

    if (!dev->ops || !dev->ops->rx_queue_poll) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->rx_queue_poll(dev, queue, budget);
}

/*!
 * Poll a Tx queue
 */
int
bcmcnet_tx_queue_poll(struct pdma_dev *dev, int queue, int budget)
{
    if (!dev->started) {
        return SHR_E_NONE;
    }

    if (!dev->ops || !dev->ops->tx_queue_poll) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->tx_queue_poll(dev, queue, budget);
}

/*!
 * Poll a queue
 */
int
bcmcnet_queue_poll(struct pdma_dev *dev, struct intr_handle *hdl, int budget)
{
    if (!dev->started) {
        return SHR_E_NONE;
    }

    if (hdl->dir == PDMA_Q_RX) {
        return bcmcnet_rx_queue_poll(dev, hdl->queue, budget);
    } else {
        return bcmcnet_tx_queue_poll(dev, hdl->queue, budget);
    }
}

/*!
 * Poll a queue group
 */
int
bcmcnet_group_poll(struct pdma_dev *dev, int group, int budget)
{
    if (!dev->started) {
        return SHR_E_NONE;
    }

    if (!dev->ops || !dev->ops->group_poll) {
        return SHR_E_INTERNAL;
    }

    return dev->ops->group_poll(dev, group, budget);
}

