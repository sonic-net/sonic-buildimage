/*! \file ngknet_dev.h
 *
 * NGKNET device definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 * IMPORTANT!
 * All shared structures must be properly 64-bit aligned.
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

#ifndef NGKNET_DEV_H
#define NGKNET_DEV_H

#include <lkm/ngbde_kapi.h>

/*! Maximum number of devices supported */
#ifdef NGBDE_NUM_SWDEV_MAX
#define NUM_PDMA_DEV_MAX        NGBDE_NUM_SWDEV_MAX
#else
#define NUM_PDMA_DEV_MAX        16
#endif

/*! Device name length */
#define NGKNET_DEV_NAME_MAX     16

/*! Maximum number of virtual network devices */
#ifndef NGKNET_NETIF_MAX
#define NUM_VDEV_MAX            128
#else
#define NUM_VDEV_MAX            NGKNET_NETIF_MAX
#endif

/*! Maximum number of filters */
#ifndef NGKNET_FILTER_MAX
#define NUM_FILTER_MAX          128
#else
#define NUM_FILTER_MAX          NGKNET_FILTER_MAX
#endif

/*!
 * \brief System network interface
 *
 * Network interface types:
 *
 *  NGKNET_NETIF_T_VLAN
 *  Transmits to this interface will go to ingress PIPE of switch
 *  CPU port using specified VLAN ID. Packet will be switched.
 *
 *  NGKNET_NETIF_T_PORT
 *  Transmits to this interface will go to unmodified to specified
 *  physical switch port. All switching logic is bypassed. Meta data
 *  should be provided when this interface is created.
 *
 *  NGKNET_NETIF_T_META
 *  Transmits to this interface will be done using raw meta data
 *  as DMA descriptors.
 *
 * Network interface flags:
 *
 *  NGKNET_NETIF_F_RCPU_ENCAP
 *  Use RCPU encapsulation for packets that enter and exit this
 *  interface.
 *
 *  NGKNET_NETIF_F_ADD_TAG
 *  Add VLAN tag to packets sent directly to physical port.
 *
 *  NGKNET_NETIF_F_BIND_CHAN
 *  Bind this interface to a Rx channel.
 */
/*! Max network interface name length */
#define NGKNET_NETIF_NAME_MAX       16
/*! Max network interface meta bytes */
#define NGKNET_NETIF_META_MAX       32
/*! Max netif user data in bytes */
#define NGKNET_NETIF_USER_DATA      64

/*! Send packets to switch */
#define NGKNET_NETIF_T_VLAN         0
/*! Send packets to port */
#define NGKNET_NETIF_T_PORT         1
/*! Send packets with matadata attached */
#define NGKNET_NETIF_T_META         2

/*! Send packets with RCPU encapsulation */
#define NGKNET_NETIF_F_RCPU_ENCAP   (1U << 0)
/*! Send packets with vlan tag */
#define NGKNET_NETIF_F_ADD_TAG      (1U << 1)
/*! Bind network interface to Rx channel */
#define NGKNET_NETIF_F_BIND_CHAN    (1U << 2)
/*! Create network interface with specified ID */
#define NGKNET_NETIF_F_WITH_ID      (1U << 3)

/*!
 * \brief Network interface description.
 */
typedef struct ngknet_netif_s {
    /*! This network interface ID */
    uint16_t id;

    /*! Next network interface ID */
    uint16_t next;

    /*! Network interface type */
    uint16_t type;

    /*! Network interface flags */
    uint16_t flags;

    /*! Network interface VLAN ID */
    uint16_t vlan;

    /*! Network interface MAC address */
    uint8_t macaddr[6];

    /*! Network interface MTU */
    uint32_t mtu;

    /*! Network interface bound to channel */
    uint32_t chan;

    /*! Network interface port */
    uint32_t port;

    /*! Network interface name */
    char name[NGKNET_NETIF_NAME_MAX];

    /*! Metadata offset from Ethernet header */
    uint16_t meta_off;

    /*! Metadata length */
    uint16_t meta_len;

    /*! Metadata used to send packets to physical port */
    uint8_t meta_data[NGKNET_NETIF_META_MAX];

    /*! User data gotten back through callbacks */
    uint8_t user_data[NGKNET_NETIF_USER_DATA];
} ngknet_netif_t;

/*!
 * \brief Packet filters
 *
 * Filters work like software TCAMs where a mask is applied to the
 * source data, and the result is then compared to the filter data.
 *
 * Filters are checked in priority order with the lowest priority
 * values being checked first (i.e. 0 is the highest priority).
 *
 * Filter types:
 *
 *  NGKNET_FILTER_T_RX_PKT
 *  Filter data and mask are applied to the Rx DMA control block
 *  as well as to the Rx packet contents.
 *
 * Destination types:
 *
 *  NGKNET_FILTER_DEST_T_NULL
 *  Packet is dropped.
 *
 *  NGKNET_FILTER_DEST_T_NETIF
 *  Packet is sent to network interface with ID <dest_id>.
 *
 *  NGKNET_FILTER_DEST_T_VNET
 *  Packet is sent to VNET in user space.
 *
 *  NGKNET_FILTER_DEST_T_CB
 *  Packet is sent to kernel filter call-back function for further filtering.
 *
 * Filter flags:
 *
 *  NGKNET_FILTER_F_ANY_DATA
 *  When this flags is set the filter will match any packet on
 *  the associated unit.
 *
 *  NGKNET_FILTER_F_STRIP_TAG
 *  Strip VLAN tag before packet is sent to destination.
 */
/*! Roundup to word */
#define NGKNET_BYTES2WORDS(bytes)   ((bytes + 3) / 4)

/*! Max filter description length */
#define NGKNET_FILTER_DESC_MAX      32
/*! Max filter bytes size */
#define NGKNET_FILTER_BYTES_MAX     256
/*! Max filter words size */
#define NGKNET_FILTER_WORDS_MAX     NGKNET_BYTES2WORDS(NGKNET_FILTER_BYTES_MAX)
/*! Max filter user data in bytes */
#define NGKNET_FILTER_USER_DATA     64

/*! Filter to Rx */
#define NGKNET_FILTER_T_RX_PKT      1

/*! Drop packet */
#define NGKNET_FILTER_DEST_T_NULL   0
/*! Send packet to netif */
#define NGKNET_FILTER_DEST_T_NETIF  1
/*! Send packet to VNET */
#define NGKNET_FILTER_DEST_T_VNET   2
/*! Send packet to kernel filter call-back function */
#define NGKNET_FILTER_DEST_T_CB     3

/*! Match any data */
#define NGKNET_FILTER_F_ANY_DATA    (1U << 0)
/*! Strip vlan tag */
#define NGKNET_FILTER_F_STRIP_TAG   (1U << 1)
/*! Match Rx channel */
#define NGKNET_FILTER_F_MATCH_CHAN  (1U << 2)
/*! Filter created with raw metadata */
#define NGKNET_FILTER_F_RAW_PMD     (1U << 15)

/*!
 * \brief Filter description.
 */
typedef struct ngknet_filter_s {
    /*! This filter ID */
    uint16_t id;

    /*! Next filter ID */
    uint16_t next;

    /*! Filter type. Refer to \ref NGKNET_FILTER_T_XXX. */
    uint16_t type;

    /*! Filter flags. Refer to \ref NGKNET_FILTER_F_XXX. */
    uint16_t flags;

    /*! Filter priority */
    uint32_t priority;

    /*! Filter belong to */
    uint32_t chan;

    /*! Filter description */
    char desc[NGKNET_FILTER_DESC_MAX];

    /*! Destination type. Refer to \ref NGKNET_FILTER_DEST_T_XXX. */
    uint16_t dest_type;

    /*! Destination network interface ID */
    uint16_t dest_id;

    /*! Destination network interface protocol type */
    uint16_t dest_proto;

    /*! Mirror type */
    uint16_t mirror_type;

    /*! Mirror network interface ID */
    uint16_t mirror_id;

    /*! Mirror network interface protocol type */
    uint16_t mirror_proto;

    /*! Out band data offset */
    uint16_t oob_data_offset;

    /*! Out band data size */
    uint16_t oob_data_size;

    /*! Packet data offset */
    uint16_t pkt_data_offset;

    /*! Packet data size */
    uint16_t pkt_data_size;

    /*! Filtering data */
    union {
        uint8_t b[NGKNET_FILTER_BYTES_MAX];
        uint32_t w[NGKNET_FILTER_WORDS_MAX];
    } data;

    /*! Filtering mask */
    union {
        uint8_t b[NGKNET_FILTER_BYTES_MAX];
        uint32_t w[NGKNET_FILTER_WORDS_MAX];
    } mask;

    /*! User data gotten back through callbacks */
    uint8_t user_data[NGKNET_FILTER_USER_DATA];
} ngknet_filter_t;

/*!
 * \brief Device information.
 */
typedef struct ngknet_dev_info_s {
    /*! Device number (from BDE) */
    int dev_no;

    /*! Device ID */
    uint32_t dev_id;

    /*! Device type string */
    char type_str[NGKNET_DEV_NAME_MAX];

    /*! Device variant string */
    char var_str[NGKNET_DEV_NAME_MAX];

    /*! Virtual network devices, pointer to ngknet_dev.vdev[] */
    struct net_device **vdev;
} ngknet_dev_info_t;

/*!
 * \brief Device configuration structure.
 */
typedef struct ngknet_dev_cfg_s {
    /*! Device name */
    char name[NGKNET_DEV_NAME_MAX];

    /*! Device type string */
    char type_str[NGKNET_DEV_NAME_MAX];

    /*! Device variant string */
    char var_str[NGKNET_DEV_NAME_MAX];

    /*! Device ID */
    uint32_t dev_id;

    /*! Device mode */
    int mode;

    /*! Number of groups */
    uint32_t nb_grp;

    /*! Bitmap of groups */
    uint32_t bm_grp;

    /*! Rx packet header size */
    uint32_t rx_ph_size;

    /*! Tx packet header size */
    uint32_t tx_ph_size;

    /*! Base network interface */
    ngknet_netif_t base_netif;

    /*! Configuration flags */
    uint32_t flags;
    /*! Rx polling for single queue */
#define NGKNET_RX_POLL_SQ       (1 << 0)
} ngknet_dev_cfg_t;

/*!
 * \brief Channel configure structure.
 */
typedef struct ngknet_chan_cfg_s {
    /*! Channel number */
    int chan;

    /*! Number of descriptors */
    uint32_t nb_desc;

    /*! Rx buffer size */
    uint32_t rx_buf_size;

    /*! Channel control */
    uint32_t chan_ctrl;
    /*! Packet_byte_swap */
#define NGKNET_PKT_BYTE_SWAP    (1 << 0)
    /*! Non packet_byte_swap */
#define NGKNET_OTH_BYTE_SWAP    (1 << 1)
    /*! Header_byte_swap */
#define NGKNET_HDR_BYTE_SWAP    (1 << 2)

    /*! Rx or Tx */
    int dir;
    /*! Rx channel */
#define NGKNET_RX_CHAN          PDMA_Q_RX
    /*! Tx channel */
#define NGKNET_TX_CHAN          PDMA_Q_TX

    /*! Pipe specified for Rx/Tx */
    int pipe;
} ngknet_chan_cfg_t;

/*!
 * \brief RCPU header structure.
 */
struct ngknet_rcpu_hdr {
    /*! Destination MAC address */
    uint8_t dst_mac[6];

    /*! Source MAC address */
    uint8_t src_mac[6];

    /*! VLAN TPID */
    uint16_t vlan_tpid;

    /*! VLAN TCI */
    uint16_t vlan_tci;

    /*! Ethernet type */
    uint16_t eth_type;

    /*! Packet signature */
    uint16_t pkt_sig;

    /*! Operation code */
    uint8_t op_code;

    /*! Flags */
    uint8_t flags;

    /*! Transaction number */
    uint16_t trans_id;

    /*! Packet data length */
    uint16_t data_len;

    /*! Header profile */
    uint16_t hdr_prof;

    /*! packet meta data length */
    uint8_t meta_len;

    /*! Transmission queue number */
    uint8_t queue_id;

    /*! Reserved must be 0 */
    uint16_t rsvd;
};

/*! RCPU Rx operation */
#define RCPU_OPCODE_RX          0x10
/*! RCPU Tx operation */
#define RCPU_OPCODE_TX          0x20

/*! RCPU purge flag */
#define RCPU_FLAG_PURGE         (1 << 0)
/*! RCPU pause flag */
#define RCPU_FLAG_PAUSE         (1 << 1)
/*! RCPU modhdr flag */
#define RCPU_FLAG_MODHDR        (1 << 2)
/*! RCPU bind queue flag */
#define RCPU_FLAG_BIND_QUE      (1 << 3)
/*! RCPU no pad flag */
#define RCPU_FLAG_NO_PAD        (1 << 4)
/*! RCPU keep FCS flag */
#define RCPU_FLAG_KEEP_FCS      (1 << 5)

#endif /* NGKNET_DEV_H */

