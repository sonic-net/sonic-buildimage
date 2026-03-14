/*! \file bcmcnet_rxtx.h
 *
 * Generic data structure and macro definitions for BCMCNET Rx/Tx.
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

#ifndef BCMCNET_RXTX_H
#define BCMCNET_RXTX_H

/*! Default timeout value (us) to wait for Tx resource. */
#ifndef BCMCNET_TX_RSRC_WAIT_USEC
#define BCMCNET_TX_RSRC_WAIT_USEC 1000000
#endif

/*! Default descriptor number in each ring. */
#define NUM_RING_DESC       64

/*! Maximum number of packets to be handled in one poll call. */
#define NUM_RXTX_BUDGET     64

/*!
 * \brief Rx buffer mode definitions.
 *
 * Buffer modes used for pktio various work modes:
 *   PDMA_BUF_MODE_PRIV   - Used for UNET DAM buffers mapped to user space.
 *   PDMA_BUF_MODE_SKB    - Legacy SKB buffer for KNET mode in kernel space.
 *   PDMA_BUF_MODE_PAGE   - Pages as DMA buffer for KNET mode in kernel space.
 *   PDMA_BUF_MODE_MAPPED - DMA buffers mapped to user space so zero-copy can
 *                          be supported by kernel for KNET mode.
 */
enum buf_mode {
    /*! Private DMA buffer in user space */
    PDMA_BUF_MODE_PRIV,

    /*! Legacy SKB buffer in Linux kernel */
    PDMA_BUF_MODE_SKB,

    /*! Raw Page buffer in Linux kernel */
    PDMA_BUF_MODE_PAGE,

    /*! Kernel buffer mapped to user space */
    PDMA_BUF_MODE_MAPPED,

    /*! Maximum number of modes */
    PDMA_BUF_MODE_MAX
};

/*!
 * Rx queue structure
 */
struct pdma_rx_queue {
    /*! Group index to which this queue belongs */
    uint32_t group_id;

    /*! Global channel index */
    uint32_t chan_id;

    /*! Queue index */
    uint32_t queue_id;

    /*! Pointer to the device control structure */
    struct dev_ctrl *ctrl;

    /*! Rx packet buffer pointers */
    struct pdma_rx_buf *pbuf;

    /*! Rx ring address */
    void *ring;

    /*! Rx ring DMA address */
    dma_addr_t ring_addr;

    /*! Rx ring DMA halt address */
    dma_addr_t halt_addr;

    /*! Rx buffer size */
    uint32_t buf_size;

    /*! Total number of descriptors */
    uint32_t nb_desc;

    /*! Next free ring entry */
    uint32_t curr;

    /*! Halt ring entry */
    uint32_t halt;

    /*! Max free descriptors to hold */
    uint32_t free_thresh;

    /*! Rx interrupt coalesce value */
    uint32_t ic_val;

    /*! Rx interrupt coalescing */
    int intr_coalescing;

    /*! Queue statistics */
    struct bcmcnet_rxq_stats stats;

    /*! Rx queue spin lock */
    sal_spinlock_t lock;

    /*! Queue state */
    int state;
    /*! Queue is used */
#define PDMA_RX_QUEUE_USED      (1 << 0)
    /*! Queue is setup */
#define PDMA_RX_QUEUE_SETUP     (1 << 1)
    /*! Queue is active */
#define PDMA_RX_QUEUE_ACTIVE    (1 << 2)
    /*! Queue is busy */
#define PDMA_RX_QUEUE_BUSY      (1 << 3)
    /*! Queue in batch refilling mode */
#define PDMA_RX_BATCH_REFILL    (1 << 4)

    /*! Queue status */
    uint32_t status;
    /*! Queue is suspended */
#define PDMA_RX_QUEUE_XOFF      (1 << 0)

    /*! DMA buffer mode */
    enum buf_mode buf_mode;

    /*! Page order in PDMA_BUF_MODE_PAGE mode */
    uint32_t page_order;

    /*! Page size in PDMA_BUF_MODE_PAGE mode */
    uint32_t page_size;
};

/*!
 * \brief Tx queue structure.
 */
struct pdma_tx_queue {
    /*! Group index to which this queue belongs */
    uint32_t group_id;

    /*! Global channel index */
    uint32_t chan_id;

    /*! Queue index */
    uint32_t queue_id;

    /*! pointer to the device control structure */
    struct dev_ctrl *ctrl;

    /*! Tx packet buffer pointers */
    struct pdma_tx_buf *pbuf;

    /*! Tx ring address */
    void *ring;

    /*! Tx ring DMA address */
    dma_addr_t ring_addr;

    /*! Tx ring DMA halt address */
    dma_addr_t halt_addr;

    /*! Total number of descriptors */
    uint32_t nb_desc;

    /*! Next free ring entry */
    uint32_t curr;

    /*! First entry to be transmitted */
    uint32_t dirt;

    /*! Halt ring entry */
    uint32_t halt;

    /*! Max free descriptors to hold in non-intr mode */
    uint32_t free_thresh;

    /*! Tx interrupt coalesce value */
    uint32_t ic_val;

    /*! Tx interrupt coalescing */
    int intr_coalescing;

    /*! Queue statistics */
    struct bcmcnet_txq_stats stats;

    /*! Tx queue spin lock */
    sal_spinlock_t lock;

    /*! Tx mutex spin lock */
    sal_spinlock_t mutex;

    /*! Tx mutex and flow control semaphore */
    sal_sem_t sem;

    /*! Queue state */
    int state;
    /*! Queue is used */
#define PDMA_TX_QUEUE_USED      (1 << 0)
    /*! Queue is setup */
#define PDMA_TX_QUEUE_SETUP     (1 << 1)
    /*! Queue is active */
#define PDMA_TX_QUEUE_ACTIVE    (1 << 2)
    /*! Queue is setup */
#define PDMA_TX_QUEUE_BUSY      (1 << 3)
    /*! Queue in polling mode */
#define PDMA_TX_QUEUE_POLL      (1 << 4)

    /*! Queue status */
    uint32_t status;
    /*! Queue is suspended */
#define PDMA_TX_QUEUE_XOFF      (1 << 0)

    /*! DMA buffer mode */
    enum buf_mode buf_mode;
};

/*!
 * \brief Setup Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_setup(struct pdma_dev *dev, int queue);

/*!
 * \brief Release Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_release(struct pdma_dev *dev, int queue);

/*!
 * \brief Restore Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_restore(struct pdma_dev *dev, int queue);

/*!
 * \brief Setup virtual Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_vqueue_setup(struct pdma_dev *dev, int queue);

/*!
 * \brief Release virtual Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_vqueue_release(struct pdma_dev *dev, int queue);

/*!
 * \brief Setup Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_setup(struct pdma_dev *dev, int queue);

/*!
 * \brief Release Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_release(struct pdma_dev *dev, int queue);

/*!
 * \brief Restore Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_restore(struct pdma_dev *dev, int queue);

/*!
 * \brief Setup virtual Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_vqueue_setup(struct pdma_dev *dev, int queue);

/*!
 * \brief Release virtual Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_vqueue_release(struct pdma_dev *dev, int queue);

/*!
 * \brief Suspend Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_suspend(struct pdma_dev *dev, int queue);

/*!
 * \brief Resume Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_resume(struct pdma_dev *dev, int queue);

/*!
 * \brief Suspend Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_suspend(struct pdma_dev *dev, int queue);

/*!
 * \brief Resume Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_resume(struct pdma_dev *dev, int queue);

/*!
 * \brief Wakeup Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_wakeup(struct pdma_dev *dev, int queue);

/*!
 * \brief Start Tx queue transmission.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 * \param [in] buf Tx packet buffer.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_xmit(struct pdma_dev *dev, int queue, void *buf);

/*!
 * \brief Poll Rx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 * \param [in] budget Poll budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_poll(struct pdma_dev *dev, int queue, int budget);

/*!
 * \brief Poll Tx queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 * \param [in] budget Poll budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_poll(struct pdma_dev *dev, int queue, int budget);

/*!
 * \brief Poll queue group.
 *
 * \param [in] dev Device structure point.
 * \param [in] group Group number.
 * \param [in] budget Poll budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_group_poll(struct pdma_dev *dev, int group, int budget);

/*!
 * \brief Dump Rx ring.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_ring_dump(struct pdma_dev *dev, int queue);

/*!
 * \brief Dump Tx ring.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_ring_dump(struct pdma_dev *dev, int queue);

#endif /* BCMCNET_RXTX_H */

