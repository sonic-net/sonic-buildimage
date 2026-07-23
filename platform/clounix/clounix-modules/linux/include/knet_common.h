/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

#ifndef __KNET_COMMON_H__
#define __KNET_COMMON_H__

#include "knet_types.h"

#define CLX_DRIVER_NAME "clx_dev"

#define CLX_MISC_MAJOR_NUM (10)
#define CLX_MISC_MINOR_NUM (250)

#define CLX_MAX_CHIP_NUM  (16)
#define CLX_PCI_BUS_WIDTH (4)

#define CLX_PKT_DMA_RING_SIZE          (1024)
#define CLX_PKT_DMA_FRAG_SIZE          (4096)
#define CLX_DMA_FRAG_NUM(_packet_len_) (((_packet_len_ - 1) / CLX_PKT_DMA_FRAG_SIZE) + 1)

#define CLX_NETIF_NAME_LEN               (16)
#define CLX_NETLINK_NAME_LEN             (16)
#define CLX_NETIF_NETLINK_MC_GRP_NUM_MAX (32)
#define CLX_RX_RSN_BMP_SIZE              (16)

/* interrupt */
#define INTR_MODE_INTX 0
#define INTR_MODE_MSI  1
#define INTR_MODE_MSIX 2

#define CLX_INTR_VALID_CODE  (0xABCD900D)
#define CLX_INTR_INVALID_MSI (0xDEADDEAD)

/* ioctl structure */
struct clx_ioctl_dma_buffer {
    clx_addr_t phy_addr;
    clx_addr_t bus_addr;
    clx_addr_t size;
#ifdef __KERNEL__
    void *virt_addr;
    struct device *alloc_dev;
    struct list_head list;
#endif
};

typedef struct interrupt_info {
    uint32_t unit;
    uint32_t irq;
    uint32_t valid;
} clx_intr_info_t;

struct clx_pci_info_s {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t revision;
    uint64_t pci_mmio_start;
    uint32_t pci_mmio_size;
};

struct clx_dev_info_s {
    uint32_t pci_dev_num;
    struct clx_pci_info_s pci_info[CLX_MAX_CHIP_NUM];
};

/* intf attr */
typedef enum {
    NETIF_PORT_SPEED_1G = 1000,
    NETIF_PORT_SPEED_10G = 10000,
    NETIF_PORT_SPEED_25G = 25000,
    NETIF_PORT_SPEED_40G = 40000,
    NETIF_PORT_SPEED_50G = 50000,
    NETIF_PORT_SPEED_100G = 100000,
    NETIF_PORT_SPEED_200G = 200000,
    NETIF_PORT_SPEED_400G = 400000,
    NETIF_PORT_SPEED_800G = 800000,
    NETIF_PORT_SPEED_LAST
} netif_port_speed_e;

typedef enum {
    NETIF_OPER_STATE_DOWN,
    NETIF_OPER_STATE_UP,
} clx_netif_oper_state_e;

typedef enum {
    NETIF_VLAN_TAG_TYPE_STRIP = 0,
    NETIF_VLAN_TAG_TYPE_KEEP,
    NETIF_VLAN_TAG_TYPE_ORIGINAL,
} clx_netif_vlan_tag_type_e;

/* DMA counter */
struct clx_dma_channel_cnt {
    uint32_t packets;
    uint32_t bytes;
    uint32_t no_memory;
    uint32_t err_recover;

    /* interrupt */
    uint32_t interrupts;
    uint32_t error_interrupts;

    /* execpt */
    uint32_t netdev_miss;
};

/* ----------------------------------------------------------------------------------- ioctl
 * structure */
struct clx_netif_ioctl_rx_cnt {
    /* rx enqueue cnt */
    uint32_t enqueue_ok;
    uint32_t enqueue_fail;
    uint32_t deque_ok;
    uint32_t deque_fail;
    uint32_t trig_event;
    /* dma cnt */
    uint32_t channel;
    struct clx_dma_channel_cnt dma_cnt;
};

struct clx_netif_ioctl_tx_cnt {
    /* dma cnt */
    uint32_t channel;
    struct clx_dma_channel_cnt dma_cnt;
};

struct clx_netif_netdev_cnt {
    uint32_t intf_id;
    uint32_t tx_pkt;
    uint32_t tx_queue_full;
    uint32_t tx_err;
    uint32_t rx_pkt;
    clx_ioctl_error_no_t rc;
};

struct clx_netif_ioctl_rx_reason_cnt {
    uint32_t intf_id;
    uint32_t cpu_reason;
    uint64_t pkt_cnts;
    uint64_t byte_cnts;
};

struct clx_netif_ioctl_port_attr {
    uint32_t port_di;
    netif_port_speed_e speed;
    clx_netif_oper_state_e oper_state;
    clx_netif_vlan_tag_type_e vlan_tag_type;
    uint32_t igr_sample_rate;
    uint32_t egr_sample_rate;
    uint8_t skip_port_state_event;
    uint32_t tc;
};

struct clx_netif_ioctl_intf {
    uint32_t id; /* unique key */
    char name[CLX_NETIF_NAME_LEN];
    uint32_t port_di;
    clx_mac_t mac;
    clx_ioctl_error_no_t rc;
};

struct clx_netif_ioctl_rx_fragment {
    uint32_t fragment_size;
    clx_addr_t fragment_dma_addr; /* Pointer to DMA buffer allocated by the user (virtual) */
};

struct clx_netif_ioctl_rx_packet {
    uint32_t unit;
    uint32_t num_fragments;
    uint32_t packet_len;
    uint32_t channel;
    struct clx_netif_ioctl_rx_fragment fragments[];
};

struct clx_ioctl_intr_cookie {
    uint32_t intr_mode; /* out */
};

struct clx_ioctl_port_map_cookie {
    uint32_t unit;
    uint32_t port_di; /* only support unit port and local port */
    uint32_t slice;
    uint32_t slice_port;
};

/* netlink */
typedef char netlink_family_t[CLX_NETLINK_NAME_LEN]; // family name
typedef char netlink_mc_grp_t[CLX_NETLINK_NAME_LEN]; // mc group name

struct clx_netif_ioctl_netlink {
    uint32_t id;
    netlink_family_t family_name;
    netlink_mc_grp_t mc_grp_name[CLX_NETIF_NETLINK_MC_GRP_NUM_MAX];
    uint32_t mc_grp_num;
    clx_ioctl_error_no_t rc;
};

struct clx_netif_rx_dst_netlink {
    netlink_family_t family_name;
    netlink_mc_grp_t mc_grp_name;
};

typedef enum {
    ACTION_NETDEV = 1,
    ACTION_NETLINK,
    ACTION_SDK,
    ACTION_FAST_FWD,
    ACTION_DROP
} clx_rx_action_e;

struct match_port {
    uint8_t match_type; // 0=don't care port,1=pattern port
    uint32_t port_di;
};

typedef uint32_t clx_rx_rsn_bmp_t[CLX_RX_RSN_BMP_SIZE];
#define CLX_REASON_BITMAP_CHK(reason_bitmap, reason) \
    (((uint32_t *)(reason_bitmap))[(reason) / 32] & (1UL << ((reason) % 32)))

#define CLX_REASON_BITMAP_FOREACH(reason_bitmap, reason)            \
    for (reason = 0; reason < (CLX_RX_RSN_BMP_SIZE * 32); reason++) \
        if (CLX_REASON_BITMAP_CHK(reason_bitmap, (reason)))

struct match_reason {
    uint8_t match_type; // 0=don't care reason,1=pattern reason
    clx_rx_rsn_bmp_t reason_bitmap;
};

#define MAX_PATTERN_NUM    (4)
#define MAX_PATTERN_LENGTH (8)
struct match_pattern {
    uint8_t match_type; // 0=don't care pattern,1=match pattern
    uint8_t offset;
    uint8_t pattern[MAX_PATTERN_LENGTH];
    uint8_t mask[MAX_PATTERN_LENGTH];
};

struct profile_rule {
    uint32_t id;
    char name[CLX_NETIF_NAME_LEN];
    uint32_t priority;
    struct match_reason match_reason;
    struct match_port match_port;
    struct match_pattern match_pattern[MAX_PATTERN_NUM];
    uint32_t action;
    struct clx_netif_rx_dst_netlink netlink;
    clx_ioctl_error_no_t rc;
#ifdef __KERNEL__
    void *virt_addr;
    struct device *alloc_dev;
    struct list_head list;
#endif
};

struct clx_netif_ioctl_mod {
    uint32_t unit;
    clx_mac_t mod_dmac; /* mod mac  */
};

struct clx_pkt_rx_reason_cnt {
    uint32_t cpu_reason;
    uint64_t pkt_cnts;
    uint64_t byte_cnts;
};

typedef enum {
    CLX_IOCTL_INIT_DEV = 0,
    CLX_IOCTL_INIT_RSRV_DMA_MEM,
    CLX_IOCTL_DEINIT_RSRV_DMA_MEM,
    CLX_IOCTL_ALLOC_SYS_DMA_MEM,
    CLX_IOCTL_FREE_SYS_DMA_MEM,
    CLX_IOCTL_CONNECT_ISR,
    CLX_IOCTL_DISCONNECT_ISR,
    /* network interface */
    CLX_IOCTL_TYPE_NETIF_CREATE_INTF = 100,
    CLX_IOCTL_TYPE_NETIF_DESTROY_INTF,
    CLX_IOCTL_TYPE_NETIF_GET_INTF,
    CLX_IOCTL_TYPE_NETIF_SET_INTF,
    CLX_IOCTL_TYPE_NETIF_CREATE_PROFILE,
    CLX_IOCTL_TYPE_NETIF_DESTROY_PROFILE,
    CLX_IOCTL_TYPE_NETIF_GET_PROFILE,
    CLX_IOCTL_TYPE_NETIF_GET_INTF_CNT,
    CLX_IOCTL_TYPE_NETIF_CLEAR_INTF_CNT,
    /* packet */
    CLX_IOCTL_TYPE_NETIF_WAIT_RX_FREE = 200,
    CLX_IOCTL_TYPE_NETIF_RX_START,
    CLX_IOCTL_TYPE_NETIF_RX_STOP,
    CLX_IOCTL_TYPE_NETIF_DEV_TX,
    /* counter */
    CLX_IOCTL_TYPE_NETIF_GET_TX_CNT = 300,
    CLX_IOCTL_TYPE_NETIF_GET_RX_CNT,
    CLX_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
    CLX_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
    /* port attribute */
    CLX_IOCTL_TYPE_NETIF_SET_PORT_ATTR = 400,
    CLX_IOCTL_TYPE_NETIF_GET_PORT_ATTR,
    /* netlink */
    CLX_IOCTL_TYPE_NETIF_NL_CREATE_NETLINK = 500,
    CLX_IOCTL_TYPE_NETIF_NL_DESTROY_NETLINK,
    CLX_IOCTL_TYPE_NETIF_NL_GET_NETLINK,
    CLX_IOCTL_TYPE_NETIF_NL_SET_PKT_MOD,
    CLX_IOCTL_TYPE_NETIF_NL_GET_PKT_MOD,

    CLX_IOCTL_TYPE_SET_PORT_MAP,
    CLX_IOCTL_TYPE_CLEAR_PORT_MAP,

    /* cpu reason cnt */
    CLX_IOCTL_TYPE_PKT_GET_REASON_CNT,
    CLX_IOCTL_TYPE_PKT_CLEAR_REASON_CNT,
    CLX_IOCTL_TYPE_NETIF_GET_REASON_CNT,
    CLX_IOCTL_TYPE_NETIF_CLEAR_REASON_CNT,

    CLX_IOCTL_TYPE_SET_IFA_CFG,
    CLX_IOCTL_TYPE_GET_IFA_CFG,
    CLX_IOCTL_TYPE_LAST

} clx_ioctl_type_t;

typedef union {
    uint32_t value;
#if defined(CLX_EN_BIG_ENDIAN)
    struct {
        uint32_t rsvd : 16;
        uint32_t type : 10; /* Maximum 1024 IOCTL types         */
        uint32_t unit : 6;  /* Maximum unit number is 64.       */
    } field;
#elif defined(CLX_EN_LITTLE_ENDIAN)
    struct {
        uint32_t unit : 6;  /* Maximum unit number is 64.       */
        uint32_t type : 10; /* Maximum 1024 IOCTL types         */
        uint32_t rsvd : 16;
    } field;
#else
#error "Host endian is not defined!!\n"
#endif
} clx_ioctl_cmd_t;

#endif // __KNET_COMMON_H__
