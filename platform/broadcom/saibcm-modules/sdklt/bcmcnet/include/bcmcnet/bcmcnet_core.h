/*! \file bcmcnet_core.h
 *
 * Generic data structure definitions and APIs for BCMCNET driver.
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

#ifndef BCMCNET_CORE_H
#define BCMCNET_CORE_H

#include <bcmcnet/bcmcnet_types.h>
#include <bcmcnet/bcmcnet_internal.h>

/*!
 * \brief Packet header structure.
 */
struct pkt_hdr {
    /*! Meta data or outer header */
    uint8_t meta_data[16];

    /*! Reserved */
    uint16_t rsvd0;

    /*! Packet signature */
    uint16_t pkt_sig;

    /*! Reserved */
    uint32_t rsvd1;

    /*! Data length */
    uint16_t data_len;

    /*! Header profile */
    uint16_t hdr_prof;

    /*! Meta length */
    uint8_t meta_len;

    /*! Queue index */
    uint8_t queue_id;

    /*! Attributes */
    uint16_t attrs;
    /*! Tx higig packet */
#define PDMA_TX_HIGIG_PKT   (1 << 0)
    /*! Tx pause packet */
#define PDMA_TX_PAUSE_PKT   (1 << 1)
    /*! Tx purge packet */
#define PDMA_TX_PURGE_PKT   (1 << 2)
    /*! Tx queue number */
#define PDMA_TX_BIND_QUE    (1 << 3)
    /*! Tx cookded header */
#define PDMA_TX_HDR_COOKED  (1 << 4)
    /*! Tx no pad */
#define PDMA_TX_NO_PAD      (1 << 5)
    /*! Tx to HNET */
#define PDMA_TX_TO_HNET     (1 << 6)
    /*! Tx XDP frame */
#define PDMA_TX_XDP_FRM     (1 << 7)
    /*! Tx XDP action */
#define PDMA_TX_XDP_ACT     (1 << 8)
    /*! Tx XSK ZC frame */
#define PDMA_TX_XSK_ZC      (1 << 9)
    /*! Rx to VNET */
#define PDMA_RX_TO_VNET     (1 << 10)
    /*! Rx strip vlan tag */
#define PDMA_RX_STRIP_TAG   (1 << 11)
    /*! Rx set protocol type */
#define PDMA_RX_SET_PROTO   (1 << 12)
    /*! Rx IP checksum */
#define PDMA_RX_IP_CSUM     (1 << 13)
    /*! Rx TCPUDP checksum */
#define PDMA_RX_TU_CSUM     (1 << 14)
};

/*! Packet header size */
#define PKT_HDR_SIZE        sizeof(struct pkt_hdr)

/*!
 * \brief Packet buffer structure.
 */
struct pkt_buf {
    /*! Packet header */
    struct pkt_hdr pkh;

    /*! Packet data */
    uint8_t data;
};

/*!
 * \brief Interrupt handle.
 */
struct intr_handle {
    /*! Device number */
    int unit;

    /*! Group number */
    int group;

    /*! Channel number */
    int chan;

    /*! Queue number */
    int queue;

    /*! Direction */
    int dir;

    /*! Polling budget */
    int budget;

    /*! Device point */
    void *dev;

    /*! Private point */
    void *priv;

    /*! Interrupt number */
    int inum;

    /*! Interrupt flags */
    uint32_t intr_flags;

    /*! Extra polling after queue is empty */
    bool extra_poll;
};

/*!
 * \brief Queue group structure.
 */
struct queue_group {
    /*! Pointer to the device control structure */
    struct dev_ctrl *ctrl;

    /*! Interrupt handles */
    struct intr_handle intr_hdl[NUM_Q_PER_GRP];

    /*! Rx queue pointers */
    void *rx_queue[NUM_Q_PER_GRP];

    /*! Tx queue pointers */
    void *tx_queue[NUM_Q_PER_GRP];

    /*! Virtual Rx queue pointers */
    void *vnet_rxq[NUM_Q_PER_GRP];

    /*! Virtual Tx queue pointers */
    void *vnet_txq[NUM_Q_PER_GRP];

    /*! Bitmap for Rx queues at work */
    uint32_t bm_rxq;

    /*! Bitmap for Tx queues at work */
    uint32_t bm_txq;

    /*! Number of Rx queues at work */
    uint32_t nb_rxq;

    /*! Number of Tx queues at work */
    uint32_t nb_txq;

    /*! Number of descriptors */
    uint32_t nb_desc[NUM_Q_PER_GRP];

    /*! Rx buffer size */
    uint32_t rx_size[NUM_Q_PER_GRP];

    /*! Queue mode */
    uint32_t que_ctrl[NUM_Q_PER_GRP];
    /*! Packet_byte_swap */
#define PDMA_PKT_BYTE_SWAP  (1 << 0)
    /*! Non packet_byte_swap */
#define PDMA_OTH_BYTE_SWAP  (1 << 1)
    /*! Header_byte_swap */
#define PDMA_HDR_BYTE_SWAP  (1 << 2)

    /*! Pipe interfaces */
    int pipe[NUM_Q_PER_GRP];

    /*! Group ID */
    int id;

    /*! Queues need to poll */
    uint32_t poll_queues;

    /*! Active IRQs for DMA control */
    uint32_t irq_mask;

    /*! Indicating the group is attached */
    bool attached;
};

/*!
 * \brief Device control structure.
 */
struct dev_ctrl {
    /*! Pointer to the device structure */
    struct pdma_dev *dev;

    /*! Pointer to hardware-specific data */
    void *hw;

    /*! HW base address */
    volatile void *hw_addr;

    /*! Queue groups */
    struct queue_group grp[NUM_GRP_MAX];

    /*! Pointers to Rx queues */
    void *rx_queue[NUM_Q_MAX];

    /*! Pointers to Tx queues */
    void *tx_queue[NUM_Q_MAX];

    /*! Pointers to virtual Rx queues */
    void *vnet_rxq[NUM_Q_MAX];

    /*! Pointers to virtual Tx queues */
    void *vnet_txq[NUM_Q_MAX];

    /*! Pointer to buffer manager */
    void *buf_mngr;

    /*! VNET sync data */
    vnet_sync_t vsync;

    /*! Bitmap of groups at work */
    uint32_t bm_grp;

    /*! Bitmap of Rx queues at work */
    uint32_t bm_rxq;

    /*! Bitmap of Tx queues at work */
    uint32_t bm_txq;

    /*! Number of groups at work */
    uint32_t nb_grp;

    /*! Number of Rx queues at work */
    uint32_t nb_rxq;

    /*! Number of Tx queues at work */
    uint32_t nb_txq;

    /*! Number of descriptors for a queue */
    uint32_t nb_desc;

    /*! Budget for once queue processing */
    uint32_t budget;

    /*! Common Rx buffer size for all queues */
    uint32_t rx_buf_size;

    /*! Rx descriptor size */
    uint32_t rx_desc_size;

    /*! Tx descriptor size */
    uint32_t tx_desc_size;

    /*! Bitmap of Rx queues in busy state */
    uint64_t bm_rxq_busy;

    /*! Bitmap of Tx queues in busy state */
    uint64_t bm_txq_busy;

    /*! Device resource lock */
    sal_spinlock_t lock;
};

/*!
 * Configure device.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] bm_rxq Rx queue bitmap.
 * \param [in] bm_txq Tx queue bitmap.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_config_f)(struct pdma_dev *dev, uint32_t bm_rxq, uint32_t bm_txq);

/*!
 * Start device.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_start_f)(struct pdma_dev *dev);

/*!
 * Stop device.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_stop_f)(struct pdma_dev *dev);

/*!
 * Close device.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_close_f)(struct pdma_dev *dev);

/*!
 * Suspend device.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_suspend_f)(struct pdma_dev *dev);

/*!
 * Resume device.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_resume_f)(struct pdma_dev *dev);

/*!
 * Get device information.
 *
 * \param [in] dev Pointer to device structure.
 */
typedef void (*pdma_dev_info_get_f)(struct pdma_dev *dev);

/*!
 * Get device statistics.
 *
 * \param [in] dev Pointer to device structure.
 */
typedef void (*pdma_dev_stats_get_f)(struct pdma_dev *dev);

/*!
 * Reset device statistics.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] dir Direction of packets specified to reset statistics.
 */
typedef void (*pdma_dev_stats_reset_f)(struct pdma_dev *dev, pdma_dir_t dir);

/*!
 * Convert logic queue to physical queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Logic queue number.
 * \param [in] dir Transmit direction.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_lq2pq_f)(struct pdma_dev *dev, int queue, int dir, int *chan);

/*!
 * Convert physical queue to logic queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] chan Channel number.
 * \param [in] queue Logic queue number.
 * \param [in] dir Transmit direction.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_dev_pq2lq_f)(struct pdma_dev *dev, int chan, int *queue, int *dir);

/*!
 * Start queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_queue_start_f)(struct pdma_dev *dev, int queue);

/*!
 * Stop queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_queue_stop_f)(struct pdma_dev *dev, int queue);

/*!
 * Set up queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_queue_setup_f)(struct pdma_dev *dev, int queue);

/*!
 * Release queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_queue_release_f)(struct pdma_dev *dev, int queue);

/*!
 * Restore queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_queue_restore_f)(struct pdma_dev *dev, int queue);

/*!
 * Enable queue interrupt.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_intr_enable_f)(struct pdma_dev *dev, int queue);

/*!
 * Disable queue interrupt.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_intr_disable_f)(struct pdma_dev *dev, int queue);

/*!
 * Acknowledge queue interrupt.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_intr_ack_f)(struct pdma_dev *dev, int queue);

/*!
 * Query queue interrupt.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_intr_query_f)(struct pdma_dev *dev, int queue);

/*!
 * Check queue interrupt.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_intr_check_f)(struct pdma_dev *dev, int queue);

/*!
 * Suspend Rx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_rx_queue_suspend_f)(struct pdma_dev *dev, int queue);

/*!
 * Resume Rx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_rx_queue_resume_f)(struct pdma_dev *dev, int queue);

/*!
 * Wake up Tx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_tx_queue_wakeup_f)(struct pdma_dev *dev, int queue);

/*!
 * Poll Rx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 * \param [in] budget Max number of descriptor to poll.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_rx_queue_poll_f)(struct pdma_dev *dev, int queue, int budget);

/*!
 * Poll Tx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 * \param [in] budget Max number of descriptor to poll.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_tx_queue_poll_f)(struct pdma_dev *dev, int queue, int budget);

/*!
 * Poll queue group.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Queue number.
 * \param [in] budget Max number of descriptor to poll.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pdma_group_poll_f)(struct pdma_dev *dev, int group, int budget);

/*!
 * \brief Exported functions structure.
 */
struct dev_ops {
    /*! Configure device */
    pdma_dev_config_f           dev_config;

    /*! Start device */
    pdma_dev_start_f            dev_start;

    /*! Stop device */
    pdma_dev_stop_f             dev_stop;

    /*! Close device */
    pdma_dev_close_f            dev_close;

    /*! Suspend device */
    pdma_dev_suspend_f          dev_suspend;

    /*! Resume device */
    pdma_dev_resume_f           dev_resume;

    /*! Get device information */
    pdma_dev_info_get_f         dev_info_get;

    /*! Get device statistics */
    pdma_dev_stats_get_f        dev_stats_get;

    /*! Reset device statistics */
    pdma_dev_stats_reset_f      dev_stats_reset;

    /*! Logic queue to physical queue */
    pdma_dev_lq2pq_f            dev_lq_to_pq;

    /*! Physical queue to logic queue */
    pdma_dev_pq2lq_f            dev_pq_to_lq;

    /*! Start Rx for a queue */
    pdma_queue_start_f          rx_queue_start;

    /*! Stop Rx for a queue */
    pdma_queue_stop_f           rx_queue_stop;

    /*! Start Tx for a queue */
    pdma_queue_start_f          tx_queue_start;

    /*! Stop Tx for a queue */
    pdma_queue_stop_f           tx_queue_stop;

    /*! Set up Rx queue */
    pdma_queue_setup_f          rx_queue_setup;

    /*! Release Rx queue */
    pdma_queue_release_f        rx_queue_release;

    /*! Restore stopped Rx queue */
    pdma_queue_restore_f        rx_queue_restore;

    /*! Set up virtual Rx queue */
    pdma_queue_setup_f          rx_vqueue_setup;

    /*! Release virtual Rx queue */
    pdma_queue_release_f        rx_vqueue_release;

    /*! Set up Tx queue */
    pdma_queue_setup_f          tx_queue_setup;

    /*! Release Tx queue */
    pdma_queue_release_f        tx_queue_release;

    /*! Restore stopped Tx queue */
    pdma_queue_restore_f        tx_queue_restore;

    /*! Set up virtual Tx queue */
    pdma_queue_setup_f          tx_vqueue_setup;

    /*! Release virtual Tx queue */
    pdma_queue_release_f        tx_vqueue_release;

    /*! Enable Rx queue interrupt */
    pdma_intr_enable_f          rx_queue_intr_enable;

    /*! Disable Rx queue interrupt */
    pdma_intr_disable_f         rx_queue_intr_disable;

    /*! Acknowledge interrupt for Rx queue */
    pdma_intr_ack_f             rx_queue_intr_ack;

    /*! Query interrupt status for Rx queue */
    pdma_intr_query_f           rx_queue_intr_query;

    /*! Check interrupt validity for Rx queue */
    pdma_intr_check_f           rx_queue_intr_check;

    /*! Enable Tx queue interrupt */
    pdma_intr_enable_f          tx_queue_intr_enable;

    /*! Disable Tx queue interrupt */
    pdma_intr_disable_f         tx_queue_intr_disable;

    /*! Acknowledge interrupt for Tx queue */
    pdma_intr_ack_f             tx_queue_intr_ack;

    /*! Query interrupt status for Tx queue */
    pdma_intr_query_f           tx_queue_intr_query;

    /*! Check interrupt validity for Tx queue */
    pdma_intr_check_f           tx_queue_intr_check;

    /*! Suspend a Rx queue */
    pdma_rx_queue_suspend_f     rx_queue_suspend;

    /*! Resume a Rx queue */
    pdma_rx_queue_resume_f      rx_queue_resume;

    /*! Wake up a Tx queue to transmit */
    pdma_tx_queue_wakeup_f      tx_queue_wakeup;

    /*! Poll for a Rx queue */
    pdma_rx_queue_poll_f        rx_queue_poll;

    /*! Poll for a Tx queue */
    pdma_tx_queue_poll_f        tx_queue_poll;

    /*! Poll for a group */
    pdma_group_poll_f           group_poll;
};

/*!
 * Read 32-bit device register.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] addr Register address.
 * \param [in] data Pointer to read data.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*reg32_read_f)(struct pdma_dev *dev, uint32_t addr, uint32_t *data);

/*!
 * Write 32-bit device register.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] addr Register address.
 * \param [in] data Data to write.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*reg32_write_f)(struct pdma_dev *dev, uint32_t addr, uint32_t data);

/*!
 * Receive packet.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Rx queue number.
 * \param [in] buf Pointer to packet buffer.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*pdma_rx_f)(struct pdma_dev *dev, int queue, void *buf);

/*!
 * Transmit packet.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Tx queue number.
 * \param [in] buf Pointer to packet buffer.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*pdma_tx_f)(struct pdma_dev *dev, int queue, void *buf);

/*!
 * Network device detach.
 *
 * \param [in] dev Pointer to device structure.
 */
typedef void (*sys_ndev_detach_f)(struct pdma_dev *dev);

/*!
 * Network device attach.
 *
 * \param [in] dev Pointer to device structure.
 */
typedef void (*sys_ndev_attach_f)(struct pdma_dev *dev);

/*!
 * Suspend Tx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Tx queue number.
 */
typedef void (*sys_tx_suspend_f)(struct pdma_dev *dev, int queue);

/*!
 * Resume Tx queue.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] queue Tx queue number.
 */
typedef void (*sys_tx_resume_f)(struct pdma_dev *dev, int queue);

/*!
 * Enable interrupts.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] group Channel group number.
 * \param [in] chan Channel number.
 * \param [in] reg Interrupt enable register.
 * \param [in] val Interrupt enable register value.
 */
typedef void (*sys_intr_unmask_f)(struct pdma_dev *dev, int group, int chan,
                                  uint32_t reg, uint32_t val);

/*!
 * Disable interrupts.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] group Channel group number.
 * \param [in] chan Channel number.
 * \param [in] reg Interrupt disable register.
 * \param [in] val Interrupt disable register value.
 */
typedef void (*sys_intr_mask_f)(struct pdma_dev *dev, int group, int chan,
                                uint32_t reg, uint32_t val);

/*!
 * Wait for notification from the other side.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*xnet_wait_f)(struct pdma_dev *dev);

/*!
 * Wake up the other side.
 *
 * \param [in] dev Pointer to device structure.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*xnet_wake_f)(struct pdma_dev *dev);

/*!
 * Convert physical address to virtual address.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] paddr Physical address.
 *
 * \retval Virtual address.
 */
typedef void *(*sys_p2v_f)(struct pdma_dev *dev, uint64_t paddr);

/*!
 * Convert virtual address to physical address.
 *
 * \param [in] dev Pointer to device structure.
 * \param [in] vaddr Virtual address.
 *
 * \retval Physical address.
 */
typedef uint64_t (*sys_v2p_f)(struct pdma_dev *dev, void *vaddr);

/*!
 * \brief Device structure.
 */
struct pdma_dev {
    /*! Device name */
    char name[DEV_NAME_LEN_MAX];

    /*! Device ID */
    uint32_t dev_id;

    /*! Device type */
    uint32_t dev_type;

    /*! Device Number */
    int unit;

    /*! Device control structure */
    struct dev_ctrl ctrl;

    /*! Pointer to the exported funtions structure */
    struct dev_ops *ops;

    /*! Device information */
    struct bcmcnet_dev_info info;

    /*! Device statistics data */
    struct bcmcnet_dev_stats stats;

    /*! Device statistics base data */
    struct bcmcnet_dev_stats stats_base;

    /*! Private data */
    void *priv;

    /*! Read 32-bit device register */
    reg32_read_f dev_read32;

    /*! Write 32-bit device register */
    reg32_write_f dev_write32;

    /*! Packet reception */
    pdma_rx_f pkt_recv;

    /*! Packet transmission */
    pdma_tx_f pkt_xmit;

    /*! Network device detach */
    sys_ndev_detach_f ndev_detach;

    /*! Network device attach */
    sys_ndev_attach_f ndev_attach;

    /*! Tx suspend */
    sys_tx_suspend_f tx_suspend;

    /*! Tx resume */
    sys_tx_resume_f tx_resume;

    /*! Enable a set of interrupts */
    sys_intr_unmask_f intr_unmask;

    /*! Disable a set of interrupts */
    sys_intr_mask_f intr_mask;

    /*! Virtual network wait for */
    xnet_wait_f xnet_wait;

    /*! Virtual network wake up */
    xnet_wake_f xnet_wake;

    /*! Physical address to virtual address */
    sys_p2v_f sys_p2v;

    /*! Virtual address to physical address */
    sys_v2p_f sys_v2p;

    /*! Maximum number of groups */
    int num_groups;

    /*! Maximum number of group queues */
    int grp_queues;

    /*! Maximum number of queues */
    int num_queues;

    /*! Rx packet header size */
    uint32_t rx_ph_size;

    /*! Tx packet header size */
    uint32_t tx_ph_size;

    /*! Flags */
    uint32_t flags;
    /*! Interrupt processing per group */
#define PDMA_GROUP_INTR     (1 << 0)
    /*! Tx polling mode */
#define PDMA_TX_POLLING     (1 << 1)
    /*! Rx batch refilling */
#define PDMA_RX_BATCHING    (1 << 2)
    /*! DMA chain mode */
#define PDMA_CHAIN_MODE     (1 << 3)
    /*! Descriptor prefetch mode */
#define PDMA_DESC_PREFETCH  (1 << 4)
    /*! VNET is docked */
#define PDMA_VNET_DOCKED    (1 << 5)
    /*! Abort PDMA mode for suspend and resume */
#define PDMA_ABORT          (1 << 6)
    /*! No FCS for Rx/Tx packets */
#define PDMA_NO_FCS         (1 << 7)

    /*! Extra poll time in microseconds */
    int extra_poll_time;

    /*! Device mode */
    dev_mode_t mode;

    /*! Device is started */
    bool started;

    /*! Device is started but suspended */
    bool suspended;

    /*! Device is initialized and HMI driver is attached */
    bool attached;
};

/*!
 * \brief Initialize device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_init(struct pdma_dev *dev);

/*!
 * \brief Clean up device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_cleanup(struct pdma_dev *dev);

/*!
 * \brief Start device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_start(struct pdma_dev *dev);

/*!
 * \brief Stop device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_stop(struct pdma_dev *dev);

/*!
 * \brief Suspend device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_suspend(struct pdma_dev *dev);

/*!
 * \brief Resume device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_resume(struct pdma_dev *dev);

/*!
 * \brief Suspend device Rx.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_rx_suspend(struct pdma_dev *dev);

/*!
 * \brief Resume device Rx.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_rx_resume(struct pdma_dev *dev);

/*!
 * \brief Dock device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_dock(struct pdma_dev *dev);

/*!
 * \brief Undock device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_undock(struct pdma_dev *dev);

/*!
 * \brief Get device information.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_info_get(struct pdma_dev *dev);

/*!
 * \brief Get device statistics.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_stats_get(struct pdma_dev *dev);

/*!
 * \brief Reset device statistics.
 *
 * \param [in] dev Device structure point.
 * \param [in] dir Direction of packets specified to reset statistics.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_stats_reset(struct pdma_dev *dev, pdma_dir_t dir);

/*!
 * \brief Change queue number to channel number.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Queue number.
 * \param [in] dir Transmit direction.
 * \param [out] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_queue_to_chan(struct pdma_dev *dev, int queue, int dir, int *chan);

/*!
 * \brief Change channel number to queue number.
 *
 * \param [in] dev Device structure point.
 * \param [in] chan Channel number.
 * \param [out] queue Queue number.
 * \param [out] dir Transmit direction.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_dev_chan_to_queue(struct pdma_dev *dev, int chan, int *queue, int *dir);

/*!
 * \brief Enable Rx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_rx_queue_intr_enable(struct pdma_dev *dev, int queue);

/*!
 * \brief Disable Rx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_rx_queue_intr_disable(struct pdma_dev *dev, int queue);

/*!
 * \brief Acknowledge Rx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_rx_queue_intr_ack(struct pdma_dev *dev, int queue);

/*!
 * \brief Check Rx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_rx_queue_intr_check(struct pdma_dev *dev, int queue);

/*!
 * \brief Enable Tx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_tx_queue_intr_enable(struct pdma_dev *dev, int queue);

/*!
 * \brief Disable Tx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_tx_queue_intr_disable(struct pdma_dev *dev, int queue);

/*!
 * \brief Acknowledge Tx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_tx_queue_intr_ack(struct pdma_dev *dev, int queue);

/*!
 * \brief Check Tx queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_tx_queue_intr_check(struct pdma_dev *dev, int queue);

/*!
 * \brief Enable queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] hdl Queue interrupt handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_queue_intr_enable(struct pdma_dev *dev, struct intr_handle *hdl);

/*!
 * \brief Disable queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] hdl Queue interrupt handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_queue_intr_disable(struct pdma_dev *dev, struct intr_handle *hdl);

/*!
 * \brief Acknowledge queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] hdl Queue interrupt handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_queue_intr_ack(struct pdma_dev *dev, struct intr_handle *hdl);

/*!
 * \brief Check queue interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] hdl Queue interrupt handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_queue_intr_check(struct pdma_dev *dev, struct intr_handle *hdl);

/*!
 * \brief Enable group interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] group Group number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_group_intr_enable(struct pdma_dev *dev, int group);

/*!
 * \brief Disable group interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] group Group number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_group_intr_disable(struct pdma_dev *dev, int group);

/*!
 * \brief Acknowledge group interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] group Group number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_group_intr_ack(struct pdma_dev *dev, int group);

/*!
 * \brief Check group interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] group Group number.
 *
 * \retval true Interrupt is active.
 * \retval false Interrupt is not active.
 */
extern bool
bcmcnet_group_intr_check(struct pdma_dev *dev, int group);

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
bcmcnet_rx_queue_poll(struct pdma_dev *dev, int queue, int budget);

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
bcmcnet_tx_queue_poll(struct pdma_dev *dev, int queue, int budget);

/*!
 * \brief Poll queue.
 *
 * \param [in] dev Device structure point.
 * \param [in] hdl Queue interrupt handle.
 * \param [in] budget Poll budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_queue_poll(struct pdma_dev *dev, struct intr_handle *hdl, int budget);

/*!
 * \brief Poll group.
 *
 * \param [in] dev Device structure point.
 * \param [in] group Group number.
 * \param [in] budget Poll budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_group_poll(struct pdma_dev *dev, int group, int budget);

#endif /* BCMCNET_CORE_H */

