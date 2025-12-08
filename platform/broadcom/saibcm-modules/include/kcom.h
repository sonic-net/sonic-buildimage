/*
 * $Id: kcom.h,v 1.9 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * File:    kcom.h
 * Purpose: User/Kernel message definitions
 */

#ifndef _KCOM_H
#define _KCOM_H

#include <sal/types.h>

#define KCOM_CHAN_KNET          "KCOM_KNET"

/*
 * Message types
 */
#define KCOM_MSG_TYPE_CMD       1  /* Command */
#define KCOM_MSG_TYPE_RSP       2  /* Command response */
#define KCOM_MSG_TYPE_EVT       3  /* Unsolicited event */

/*
 * Message opcodes
 */
#define KCOM_M_NONE             0  /* Should not be used */
#define KCOM_M_VERSION          1  /* Protocol version */
#define KCOM_M_STRING           2  /* For debug messages */
#define KCOM_M_HW_RESET         3  /* H/W not ready */
#define KCOM_M_HW_INIT          4  /* H/W initialized */
#define KCOM_M_ETH_HW_CONFIG    5  /* ETH HW config*/
#define KCOM_M_DETACH           6  /* Detach kernel module */
#define KCOM_M_REPROBE          7  /* Reprobe device */
#define KCOM_M_HW_INFO          8  /* Send the HW info to kernel module */
#define KCOM_M_NETIF_CREATE     11 /* Create network interface */
#define KCOM_M_NETIF_DESTROY    12 /* Destroy network interface */
#define KCOM_M_NETIF_LIST       13 /* Get list of network interface IDs */
#define KCOM_M_NETIF_GET        14 /* Get network interface info */
#define KCOM_M_NETIF_REPLACE    15 /* Replace network interface with ID */
#define KCOM_M_FILTER_CREATE    21 /* Create Rx filter */
#define KCOM_M_FILTER_DESTROY   22 /* Destroy Rx filter */
#define KCOM_M_FILTER_LIST      23 /* Get list of Rx filter IDs */
#define KCOM_M_FILTER_GET       24 /* Get Rx filter info */
#define KCOM_M_DMA_INFO         31 /* Tx/Rx DMA info */
#define KCOM_M_DBGPKT_SET       41 /* Enbale debug packet function */
#define KCOM_M_DBGPKT_GET       42 /* Get debug packet function info */
#define KCOM_M_WB_CLEANUP       51 /* Clean up for warmbooting */
#define KCOM_M_CLOCK_CMD        52 /* Clock Commands */
#define KCOM_M_PCIE_LINK_STATUS 53 /* PCIe link status */

#define KCOM_VERSION            24 /* Protocol version */

/*
 * Message status codes
 */
#define KCOM_E_NONE             0  /* No errors */
#define KCOM_E_PARAM            1  /* Invalid/unsupported parameter */
#define KCOM_E_RESOURCE         2  /* Out of memory or other resource */
#define KCOM_E_NOT_FOUND        3  /* Requested object not found */

typedef struct kcom_msg_hdr_s {
    uint8 type;
    uint8 opcode;
    uint8 seqno;
    uint8 status;
    uint8 unit;
    uint8 reserved;
    uint16 id;
} kcom_msg_hdr_t;

/*
 * Object types
 */

/*
 * System network interface
 *
 * Network interface types:
 *
 *  KCOM_NETIF_T_VLAN
 *  Transmits to this interface will go to ingress PIPE of switch
 *  CPU port using specified VLAN ID. Packet will be switched.
 *
 *  KCOM_NETIF_T_PORT
 *  Transmits to this interface will go to unmodified to specified
 *  physical switch port. All switching logic is bypassed.
 *
 *  KCOM_NETIF_T_META
 *  Transmits to this interface will be done using raw meta data
 *  as DMA descriptors. Currently used for RCPU mode only.
 *
 * Network interface flags:
 *
 *  KCOM_NETIF_F_ADD_TAG
 *  Add VLAN tag to packets sent directly to physical port.
 *
 *  KCOM_NETIF_F_RCPU_ENCAP
 *  Use RCPU encapsulation for packets that enter and exit this
 *  interface.
 */
#define KCOM_NETIF_T_VLAN       0
#define KCOM_NETIF_T_PORT       1
#define KCOM_NETIF_T_META       2

#define KCOM_NETIF_F_ADD_TAG           (1U << 0)
#define KCOM_NETIF_F_RCPU_ENCAP        (1U << 1)
/* If a netif has this flag, the packet sent to the netif can't be stripped tag or added tag */
#define KCOM_NETIF_F_KEEP_RX_TAG       (1U << 2)
#define KCOM_NETIF_F_USE_SHARED_NDEV   (1U << 3)

#define KCOM_NETIF_NAME_MAX     16

/*
 * Max size of Sand System Headers
 * For DNX, Module Header(16B) + PTCH(2B/3B) + ITMH(5B)
 * For DPP, PTCH(2B) + ITMH(4B)
 */
#define KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX     64

typedef struct kcom_netif_s {
    uint32 cb_user_data;
    uint16 id;
    uint16 port;
    uint16 vlan;
    uint16 qnum;
    uint16 phys_port;
    uint8 type;
    uint8 flags;
    uint8 macaddr[6];
    uint8 system_headers[KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX];
    uint8 system_headers_size;
    char name[KCOM_NETIF_NAME_MAX];
} kcom_netif_t;

/*
 * Packet filters
 *
 * Filters work like software TCAMs where a mask is applied to the
 * source data, and the result is then compared to the filter data.
 *
 * Filters are checked in priority order with the lowest priority
 * values being checked first (i.e. 0 is the highest priority).
 *
 * Filter types:
 *
 *  KCOM_FILTER_T_RX_PKT
 *  Filter data and mask are applied to the Rx DMA control block
 *  as well as to the Rx packet contents.
 *
 * Destination types:
 *
 *  KCOM_DEST_T_NULL
 *  Packet is dropped.
 *
 *  KCOM_DEST_T_NETIF
 *  Packet is sent to network interface with ID <dest_id>.
 *
 *  KCOM_DEST_T_API
 *  Packet is sent to Rx API through queue <dest_id>.
 *
 *  KCOM_DEST_T_CB
 *  Packet destination is obtained from kernel call-back function.
 *
 * Filter flags:
 *
 *  KCOM_FILTER_F_ANY_DATA
 *  When this flags is set the filter will match any packet on
 *  the associated unit.
 *
 *  KCOM_FILTER_F_STRIP_TAG
 *  Strip VLAN tag before packet is sent to destination.
 *  This flag only applies to KCOM_DEST_T_NETIF.
 *
 */
#define KCOM_FILTER_BYTES_MAX   256
#define KCOM_FILTER_WORDS_MAX   BYTES2WORDS(KCOM_FILTER_BYTES_MAX)

#define KCOM_FILTER_T_RX_PKT    1

#define KCOM_DEST_T_NULL        0
#define KCOM_DEST_T_NETIF       1
#define KCOM_DEST_T_API         2
#define KCOM_DEST_T_CB          3

#define KCOM_FILTER_F_ANY_DATA  (1U << 0)
#define KCOM_FILTER_F_STRIP_TAG (1U << 1)

#define KCOM_FILTER_DESC_MAX    32

typedef struct kcom_filter_s {
    uint16 id;
    uint8 type;
    uint8 priority;
    char desc[KCOM_FILTER_DESC_MAX];
    uint32 flags;
    uint32 cb_user_data;
    uint16 dest_type;
    uint16 dest_id;
    uint16 dest_proto;
    uint16 mirror_type;
    uint16 mirror_id;
    uint16 mirror_proto;
    uint16 oob_data_offset;
    uint16 oob_data_size;
    uint16 pkt_data_offset;
    uint16 pkt_data_size;
    union {
        uint8 b[KCOM_FILTER_BYTES_MAX];
        uint32 w[KCOM_FILTER_WORDS_MAX];
    } data;
    union {
        uint8 b[KCOM_FILTER_BYTES_MAX];
        uint32 w[KCOM_FILTER_WORDS_MAX];
    } mask;
    /** Mark to match source modid and modport */
    uint8  is_src_modport;
    uint8  spa_unit;
} kcom_filter_t;

/*
 * DMA buffer information
 *
 * Cookie field is reserved use by application (32/64-bit pointer).
 *
 * For Tx operation the application will submit the start address of
 * the Tx DCB chain which is queued for transfer by the kernel module.
 * Once DMA is done a DMA event is returned to the application with an
 * optional sequence number.
 *
 * For Rx operation the application will submit the start address of
 * the Rx DCB chain which should be use for packet reception by the
 * kernel module. Once DMA is done a DMA event is returned to the
 * application with an optional sequence number.
 *
 * Cookie field is reserved use by application (32/64-bit pointer).
 *
 * Packet info types:
 *
 *  KCOM_DMA_INFO_T_TX_DCB
 *  Data is physical start address of Tx DCB chain.
 *
 *  KCOM_DMA_INFO_T_RX_DCB
 *  Data is physical start address of Rx DCB chain.
 *
 * Packet info flags:
 *
 *  KCOM_DMA_INFO_F_TX_DONE
 *  This flag is set by the kernel module and means that one or more
 *  packets have been sent.
 *
 *  KCOM_DMA_INFO_F_RX_DONE
 *  This flag is set by the kernel module and means that one or more
 *  Rx buffers contain valid packet data.
 */
#define KCOM_DMA_INFO_T_TX_DCB  1
#define KCOM_DMA_INFO_T_RX_DCB  2

#define KCOM_DMA_INFO_F_TX_DONE (1U << 0)
#define KCOM_DMA_INFO_F_RX_DONE (1U << 1)

typedef struct kcom_dma_info_s {
    uint8 type;
    uint8 cnt;
    uint16 size;
    uint16 chan;
    uint16 flags;
    union {
        uint64 dcb_start;
        struct {
            uint32 tx;
            uint32 rx;
        } seqno;
    } data;
    union {
        void *p;
        uint8 b[8];
    } cookie;
} kcom_dma_info_t;

/* Default channel configuration */
#define KCOM_DMA_TX_CHAN        0
#define KCOM_DMA_RX_CHAN        1


#define KCOM_ETH_HW_T_RESET     1
#define KCOM_ETH_HW_T_INIT      2
#define KCOM_ETH_HW_T_OTHER     3

#define KCOM_ETH_HW_C_ALL       0xff

#define KCOM_ETH_HW_RESET_F_TX              (1U << 0)
#define KCOM_ETH_HW_RESET_F_RX              (1U << 1)
#define KCOM_ETH_HW_RESET_F_TX_RECLAIM      (1U << 2)
#define KCOM_ETH_HW_RESET_F_RX_RECLAIM      (1U << 3)

#define KCOM_ETH_HW_INIT_F_TX               (1U << 0)
#define KCOM_ETH_HW_INIT_F_RX               (1U << 1)
#define KCOM_ETH_HW_INIT_F_RX_FILL          (1U << 2)

#define KCOM_ETH_HW_OTHER_F_FIFO_LOOPBACK   (1U << 0)
#define KCOM_ETH_HW_OTHER_F_INTERRUPT       (1U << 1)

typedef struct kcom_eth_hw_config_s {
    uint8 type;
    uint8 chan;
    uint32 flags;
    uint32 value;
} kcom_eth_hw_config_t;

#ifndef KCOM_HW_INFO_OAMP_PORT_MAX
#define KCOM_HW_INFO_OAMP_PORT_MAX     8
#endif

/*
 * Send the OAMP information to Kernel module.
 */
typedef struct kcom_oamp_info_s {
    uint32 oamp_port_number;
    uint32 oamp_ports[KCOM_HW_INFO_OAMP_PORT_MAX];
} kcom_oamp_info_t;

/*
 * Message types
 */

/*
 * Request KCOM interface version of kernel module.
 */
typedef struct kcom_msg_version_s {
    kcom_msg_hdr_t hdr;
    uint32 version;
    uint32 netif_max;
    uint32 filter_max;
    uint32 module_reload;
} kcom_msg_version_t;

/*
 * Request KCOM interface clock info.
 */
#define KSYNC_M_HW_INIT                 0
#define KSYNC_M_HW_DEINIT               1
#define KSYNC_M_VERSION                 2
#define KSYNC_M_HW_TS_DISABLE           3
#define KSYNC_M_MTP_TS_UPDATE_ENABLE    4
#define KSYNC_M_MTP_TS_UPDATE_DISABLE   5
#define KSYNC_M_DNX_JR2DEVS_SYS_CONFIG  6
#define KSYNC_M_BS_CONFIG_SET           7
#define KSYNC_M_BS_CONFIG_CLEAR         8
#define KSYNC_M_BS_STATUS               9
#define KSYNC_M_PTP_TOD_OFFSET_SET      10
#define KSYNC_M_PTP_TOD_OFFSET_GET      11
#define KSYNC_M_NTP_TOD_OFFSET_SET      12
#define KSYNC_M_NTP_TOD_OFFSET_GET      13
#define KSYNC_M_PTP_TOD_GET             14
#define KSYNC_M_NTP_TOD_GET             15
#define KSYNC_M_GPIO_CONFIG_SET         16
#define KSYNC_M_GPIO_CONFIG_GET         17
#define KSYNC_M_LEAP_SEC_SET            18
#define KSYNC_M_LEAP_SEC_GET            19
#define KSYNC_M_BS_PHASE_OFFSET_SET     20

typedef struct kcom_clock_info_s {
    uint8 cmd;
    int32 data[12];
} kcom_clock_info_t;

/*
 * Send literal string to/from kernel module.
 * Mainly for debugging purposes.
 */
#define KCOM_MSG_STRING_MAX     128

typedef struct kcom_msg_string_s {
    kcom_msg_hdr_t hdr;
    uint32 len;
    char val[KCOM_MSG_STRING_MAX];
} kcom_msg_string_t;

/*
 * Indicate that eth hardware is about to be reset. Active
 * DMA operations should be aborted and DMA and interrupts
 * should be disabled.
 */
/*
 * Indicate that eth hardware has been properly initialized
 * for DMA operation to commence.
 */ 
typedef struct kcom_msg_eth_hw_config_s {
    kcom_msg_hdr_t hdr;
    kcom_eth_hw_config_t config;
} kcom_msg_eth_hw_config_t;

/*
 * Indicate that switch hardware is about to be reset. Active
 * DMA operations should be aborted and DMA and interrupts
 * should be disabled.
 */
typedef struct kcom_msg_hw_reset_s {
    kcom_msg_hdr_t hdr;
    uint32 channels;
} kcom_msg_hw_reset_t;

/*
 * Indicate that switch hardware has been properly initialized
 * for DMA operation to commence.
 */
typedef struct kcom_msg_hw_init_s {
    kcom_msg_hdr_t hdr;
    uint8 cmic_type;
    uint8 dcb_type;
    uint8 dcb_size;
    uint8 pkt_hdr_size;
    uint32 dma_hi;
    uint32 cdma_channels;
    /*
     * Bitmap of DMA channels reserved for the user mode network driver.
     * These channels cannot be used by the kernel network driver (KNET).
     */
    uint32 unet_channels;
} kcom_msg_hw_init_t;

/*
 * Release blocked IOCTL threads and clean up as necessary.
 */
typedef struct kcom_msg_detach_s {
    kcom_msg_hdr_t hdr;
    uint32 flags;
} kcom_msg_detach_t;

/*
 * Reprobe switch device.
 */
typedef struct kcom_msg_reprobe_s {
    kcom_msg_hdr_t hdr;
    uint32 flags;
} kcom_msg_reprobe_t;

/*
 * Enable/Disable debugging packet function.
 */
typedef struct kcom_msg_dbg_pkt_set_s {
    kcom_msg_hdr_t hdr;
    int enable;
} kcom_msg_dbg_pkt_set_t;

/*
 * Get debugging packet function info.
 */
typedef struct kcom_msg_dbg_pkt_get_s {
    kcom_msg_hdr_t hdr;
    int value;
} kcom_msg_dbg_pkt_get_t;

/*
 * Clean up warmboot-related resources.
 */
typedef struct kcom_msg_wb_cleanup_s {
    kcom_msg_hdr_t hdr;
    uint32 flags;
} kcom_msg_wb_cleanup_t;

/* PCIE Link status */
#define PCIE_LINK_STATUS_UP             0x0
#define PCIE_LINK_STATUS_DOWN           0x1

/*
 * Update PCIe link status.
 */
typedef struct kcom_msg_pcie_link_status_s {
    kcom_msg_hdr_t hdr;
    int pcie_link_status;
} kcom_msg_pcie_link_status_t;

/*
 * Create new system network interface. The network interface will
 * be associated with the specified switch unit number.
 * The interface id and name will be assigned by the kernel module.
 */
typedef struct kcom_msg_netif_create_s {
    kcom_msg_hdr_t hdr;
    kcom_netif_t netif;
} kcom_msg_netif_create_t;

/*
 * Destroy system network interface.
 */
typedef struct kcom_msg_netif_destroy_s {
    kcom_msg_hdr_t hdr;
} kcom_msg_netif_destroy_t;

/*
 * Destroy system network interface.
 */
typedef struct kcom_msg_clock_s{
    kcom_msg_hdr_t hdr;
    kcom_clock_info_t clock_info;
} kcom_msg_clock_cmd_t;

/*
 * Get list of currently defined system network interfaces.
 */
#ifndef KCOM_NETIF_MAX
#define KCOM_NETIF_MAX          128
#endif

typedef struct kcom_msg_netif_list_s {
    kcom_msg_hdr_t hdr;
    uint32 ifcnt;
    uint16 id[KCOM_NETIF_MAX];
} kcom_msg_netif_list_t;

/*
 * Get detailed network interface information.
 */
typedef struct kcom_msg_netif_get_s {
    kcom_msg_hdr_t hdr;
    kcom_netif_t netif;
} kcom_msg_netif_get_t;

/*
 * Create new packet filter.
 * The filter id will be assigned by the kernel module.
 */
typedef struct kcom_msg_filter_create_s {
    kcom_msg_hdr_t hdr;
    kcom_filter_t filter;
} kcom_msg_filter_create_t;

/*
 * Destroy packet filter.
 */
typedef struct kcom_msg_filter_destroy_s {
    kcom_msg_hdr_t hdr;
} kcom_msg_filter_destroy_t;

/*
 * Get list of currently defined packet filters.
 */
#ifndef KCOM_FILTER_MAX
#define KCOM_FILTER_MAX         128
#endif

typedef struct kcom_msg_filter_list_s {
    kcom_msg_hdr_t hdr;
    uint32 fcnt;
    uint16 id[KCOM_FILTER_MAX];
} kcom_msg_filter_list_t;

/*
 * Get detailed packet filter information.
 */
typedef struct kcom_msg_filter_get_s {
    kcom_msg_hdr_t hdr;
    kcom_filter_t filter;
} kcom_msg_filter_get_t;

/*
 * DMA info
 */
typedef struct kcom_msg_dma_info_s {
    kcom_msg_hdr_t hdr;
    kcom_dma_info_t dma_info;
} kcom_msg_dma_info_t;

/*
 * HW info
 */
typedef struct kcom_msg_hw_info_s {
    kcom_msg_hdr_t hdr;
    kcom_oamp_info_t oamp_info;
    /*
     * Information to parse Dune system headers
     */
    uint32 ftmh_lb_key_ext_size;
    uint32 ftmh_stacking_ext_size;
    uint32 pph_base_size;
    uint32 pph_lif_ext_size[8];
    uint32 udh_length_type[4];
    uint32 udh_size;
    uint32 oamp_punted;
    uint32 jr_mode_udh_size_compensation;
    uint32 jr_mode_add_udh_base;
    uint32 enet_channels;
    uint8 no_skip_udh_check;
    uint8 oam_dm_tod_exist;
    uint8 system_headers_mode;
    uint8 udh_enable;
    uint16 up_mep_ingress_cpu_trap_id1;
    uint16 up_mep_ingress_cpu_trap_id2;
    uint8 spa_mode;
} kcom_msg_hw_info_t;

/*
 * All messages (e.g. for generic receive)
 */
typedef union kcom_msg_s {
    kcom_msg_hdr_t hdr;
    kcom_msg_version_t version;
    kcom_msg_string_t string;
    kcom_msg_hw_reset_t hw_reset;
    kcom_msg_hw_init_t hw_init;
    kcom_msg_eth_hw_config_t eth_hw_config;
    kcom_msg_hw_info_t hw_info;
    kcom_msg_detach_t detach;
    kcom_msg_reprobe_t reprobe;
    kcom_msg_netif_create_t netif_create;
    kcom_msg_netif_destroy_t netif_destroy;
    kcom_msg_netif_list_t netif_list;
    kcom_msg_netif_get_t netif_get;
    kcom_msg_filter_create_t filter_create;
    kcom_msg_filter_destroy_t filter_destroy;
    kcom_msg_filter_list_t filter_list;
    kcom_msg_filter_get_t filter_get;
    kcom_msg_dma_info_t dma_info;
    kcom_msg_dbg_pkt_set_t dbg_pkt_set;
    kcom_msg_dbg_pkt_get_t dbg_pkt_get;
    kcom_msg_wb_cleanup_t wb_cleanup;
    kcom_msg_clock_cmd_t clock_cmd;
    kcom_msg_pcie_link_status_t pcie_link_status;
} kcom_msg_t;

/*
 * KCOM communication channel vectors
 *
 * open
 * Open KCOM channel.
 *
 * close
 * Close KCOM channel.
 *
 * send
 * Send KCOM message. If bufsz is non-zero, a synchronous send will be
 * performed (if supported) and the function will return the number of
 * bytes in the response.
 *
 * recv
 * Receive KCOM message. This function is used t oreceive unsolicited
 * messages from the kernel. If synchronous send is not supported, this
 * function is also used to retrieve responses to command messages.
 */

typedef struct kcom_chan_s {
    void *(*open)(char *name);
    int (*close)(void *handle);
    int (*send)(void *handle, void *msg, unsigned int len, unsigned int bufsz);
    int (*recv)(void *handle, void *msg, unsigned int bufsz);
} kcom_chan_t;

#endif /* _KCOM_H */
