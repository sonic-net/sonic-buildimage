/*! \file bcmcnet_types.h
 *
 * BCMCNET public data structure and macro definitions.
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

#ifndef BCMCNET_TYPES_H
#define BCMCNET_TYPES_H

#include <bcmcnet/bcmcnet_dep.h>

/*! Maximum length of device name */
#define DEV_NAME_LEN_MAX    16

/*! Maximum number of groups supported each device */
#define NUM_GRP_MAX         4

/*! Maximum number of queues supported each group */
#define NUM_Q_PER_GRP       16

/*! Maximum number of queues supported each device */
#define NUM_Q_MAX           (NUM_GRP_MAX * NUM_Q_PER_GRP)

/*! Maximum length of jumbo frame */
#define JUMBO_FRAME_LEN_MAX 0xffff

/*! Maximum Rx buffer size */
#define RX_BUF_SIZE_MAX     JUMBO_FRAME_LEN_MAX

/*! Minimum Rx buffer size */
#define RX_BUF_SIZE_MIN     68

/*! Default Rx buffer size */
#define RX_BUF_SIZE_DFLT    9216

/*!
 * \brief Transmission direction.
 */
enum pdma_dir {
    PDMA_Q_RX = 0,
    PDMA_Q_TX
};

/*!
 * \brief Device information.
 */
typedef struct bcmcnet_dev_info {
    /*! Device name */
    char dev_name[DEV_NAME_LEN_MAX];

    /*! Device ID */
    uint32_t dev_id;

    /*! Device type */
    uint32_t dev_type;

    /*! Maximum number of groups */
    uint32_t max_groups;

    /*! Maximum number of queues */
    uint32_t max_queues;

    /*! Bitmap of groups at work */
    uint32_t bm_groups;

    /*! Bitmap of Rx queues at work */
    uint32_t bm_rx_queues;

    /*! Bitmap of Tx queues at work */
    uint32_t bm_tx_queues;

    /*! Number of groups at work */
    uint32_t nb_groups;

    /*! Number of Rx queues at work */
    uint32_t nb_rx_queues;

    /*! Number of Tx queues at work */
    uint32_t nb_tx_queues;

    /*! Rx descriptor size */
    uint32_t rx_desc_size;

    /*! Tx descriptor size */
    uint32_t tx_desc_size;

    /*! Rx packet header size */
    uint32_t rx_ph_size;

    /*! Tx packet header size */
    uint32_t tx_ph_size;

    /*! Rx buffer size */
    uint32_t rx_buf_dflt;

    /*! Number of descriptors for a queue */
    uint32_t nb_desc_dflt;

    /*! Rx buffer size per queue */
    uint32_t rx_buf_size[NUM_Q_MAX];

    /*! Number of Rx descriptors per queue */
    uint32_t nb_rx_desc[NUM_Q_MAX];

    /*! State of Rx queues */
    uint32_t rxq_state[NUM_Q_MAX];

    /*! Number of Tx descriptors per queue */
    uint32_t nb_tx_desc[NUM_Q_MAX];

    /*! State of Tx queues */
    uint32_t txq_state[NUM_Q_MAX];
} bcmcnet_dev_info_t;

/*!
 * \brief Device statistics.
 */
typedef struct bcmcnet_dev_stats {
    /*! Number of successfully received packets */
    uint64_t rx_packets;

    /*! Number of successfully received bytes */
    uint64_t rx_bytes;

    /*! Number of dropped packets */
    uint64_t rx_dropped;

    /*! Number of erroneous received packets */
    uint64_t rx_errors;

    /*! Number of error head packets */
    uint64_t rx_head_errors;

    /*! Number of error data packets */
    uint64_t rx_data_errors;

    /*! Number of error cell packets */
    uint64_t rx_cell_errors;

    /*! Number of RX pktbuf allocation failures */
    uint64_t rx_nomems;

    /*! Number of successfully transmitted packets */
    uint64_t tx_packets;

    /*! Number of successfully transmitted bytes */
    uint64_t tx_bytes;

    /*! Number of dropped packets */
    uint64_t tx_dropped;

    /*! Number of failed transmitted packets */
    uint64_t tx_errors;

    /*! Number of suspended transmission */
    uint64_t tx_xoffs;

    /*! Number of interrupts */
    uint64_t intrs;

    /*! Number of successfully received packets per queue */
    uint64_t rxq_packets[NUM_Q_MAX];

    /*! Number of successfully received bytes per queue */
    uint64_t rxq_bytes[NUM_Q_MAX];

    /*! Number of dropped packets per queue */
    uint64_t rxq_dropped[NUM_Q_MAX];

    /*! Number of erroneous received packets per queue */
    uint64_t rxq_errors[NUM_Q_MAX];

    /*! Number of error head packets per queue */
    uint64_t rxq_head_errors[NUM_Q_MAX];

    /*! Number of error data packets per queue */
    uint64_t rxq_data_errors[NUM_Q_MAX];

    /*! Number of error cell packets per queue */
    uint64_t rxq_cell_errors[NUM_Q_MAX];

    /*! Number of RX pktbuf allocation failures per queue */
    uint64_t rxq_nomems[NUM_Q_MAX];

    /*! Number of successfully transmitted bytes per queue */
    uint64_t txq_packets[NUM_Q_MAX];

    /*! Number of successfully transmitted bytes per queue */
    uint64_t txq_bytes[NUM_Q_MAX];

    /*! Number of dropped packets per queue */
    uint64_t txq_dropped[NUM_Q_MAX];

    /*! Number of failed transmitted packets per queue */
    uint64_t txq_errors[NUM_Q_MAX];

    /*! Number of suspended transmission per queue */
    uint64_t txq_xoffs[NUM_Q_MAX];
} bcmcnet_dev_stats_t;

/*!
 * \brief Device modes.
 */
typedef enum dev_mode_e {
    /*!
     * User network mode.
     * The standalone CNET works in user space.
     */
    DEV_MODE_UNET = 0,

    /*!
     * Kernel network mode.
     * Combined with KNET module, CNET works in kernel space.
     */
    DEV_MODE_KNET,

    /*!
     * Virtual network mode.
     * CNET works in user space as a virtual network.
     * The hypervisor must be deployed in KNET module.
     */
    DEV_MODE_VNET,

    /*!
     * Hyper network mode.
     * Combined with KNET module, CNET works in kernel space as a hypervisor.
     * The virtual network is not neccessary in this mode.
     */
    DEV_MODE_HNET,

    /*! Maximum number of mode */
    DEV_MODE_MAX
} dev_mode_t;

/*!
 * \brief VNET sync data.
 */
typedef struct vnet_sync_s {
    /*! Rx ring address */
    uint64_t rx_ring_addr[NUM_Q_MAX];

    /*! Rx ring size */
    uint32_t rx_ring_size[NUM_Q_MAX];

    /*! Tx ring address */
    uint64_t tx_ring_addr[NUM_Q_MAX];

    /*! Tx ring size */
    uint32_t tx_ring_size[NUM_Q_MAX];
} vnet_sync_t;

#endif /* BCMCNET_TYPES_H */

