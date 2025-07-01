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
typedef enum pdma_dir_e {
    PDMA_DIR_RX = 0,
    PDMA_DIR_TX,
    PDMA_DIR_RXTX
} pdma_dir_t;

/*! Channel in Rx direction */
#define PDMA_Q_RX           PDMA_DIR_RX

/*! Channel in Tx direction */
#define PDMA_Q_TX           PDMA_DIR_TX

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
 * \brief Rx queue statistics.
 */
typedef struct bcmcnet_rxq_stats {
    /*! Number of received packets */
    uint64_t packets;

    /*! Number of received bytes */
    uint64_t bytes;

    /*! Number of dropped packets */
    uint64_t dropped;

    /*! Number of errors */
    uint64_t errors;

    /*! Number of head errors */
    uint64_t head_errors;

    /*! Number of data errors */
    uint64_t data_errors;

    /*! Number of cell errors */
    uint64_t cell_errors;

    /*! Number of failed allocation */
    uint64_t nomems;
} bcmcnet_rxq_stats_t;

/*!
 * \brief Tx queue statistics.
 */
typedef struct bcmcnet_txq_stats {
    /*! Number of sent packets */
    uint64_t packets;

    /*! Number of sent bytes */
    uint64_t bytes;

    /*! Number of dropped packets */
    uint64_t dropped;

    /*! Number of errors */
    uint64_t errors;

    /*! Number of suspends */
    uint64_t xoffs;
} bcmcnet_txq_stats_t;

/*!
 * \brief Device statistics.
 */
typedef struct bcmcnet_dev_stats {
    /*! Queue statistics for Rx */
    bcmcnet_rxq_stats_t rxq[NUM_Q_MAX];

    /*! Global statistics for all Rx queues */
    bcmcnet_rxq_stats_t rxqs;

    /*! Queue statistics for Tx */
    bcmcnet_txq_stats_t txq[NUM_Q_MAX];

    /*! Global statistics for all Tx queues */
    bcmcnet_txq_stats_t txqs;

    /*! Number of interrupts */
    uint64_t intrs;
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

