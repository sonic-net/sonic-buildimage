/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */
#ifndef __VIRTIO_SPEC_H__
#define __VIRTIO_SPEC_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#ifndef BIT
#define BIT(nr)                 (1UL << (nr))
#endif
#ifndef BIT_ULL
#define BIT_ULL(nr)             (1ULL << (nr))
#endif
#ifndef BIT_MASK
#define BIT_MASK(nr)            (1UL << (nr))
#endif
#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE           (8)
#endif

/* 4.1.4.3 - common configuration structure layout */

/* upper bounds, non-inclusive, for indirect access via select registers */
enum {
    VIRTIO_PCI_FEATURE_SELECT_COUNT = 2,
    VIRTIO_PCI_QUEUE_SELECT_COUNT = 128,
};

/* feature config, for indirect access via select register */
typedef uint32_t virtio_pci_feature_cfg_t;

/* virtqueue config, for indirect access via select register */
typedef struct virtio_pci_queue_cfg {
    uint16_t                    queue_size;
    uint16_t                    queue_msix_vector;
    uint16_t                    queue_enable;
    uint16_t                    queue_notify_off;
    uint32_t                    queue_desc_lo;
    uint32_t                    queue_desc_hi;
    uint32_t                    queue_avail_lo;
    uint32_t                    queue_avail_hi;
    uint32_t                    queue_used_lo;
    uint32_t                    queue_used_hi;
} __attribute__((packed)) virtio_pci_queue_cfg_t;

/* common config, with dummy fields in place of indirect access */
typedef struct virtio_pci_common_cfg {
    uint32_t                    device_feature_select;
    virtio_pci_feature_cfg_t    device_feature; // indirect
    uint32_t                    driver_feature_select;
    virtio_pci_feature_cfg_t    driver_feature; // indirect
    uint16_t                    config_msix_vector;
    uint16_t                    num_queues;
    uint8_t                     device_status;
    uint8_t                     config_generation;
    uint16_t                    queue_select;
    union {
        virtio_pci_queue_cfg_t  queue_cfg; // indirect
        /* indirect features are hidden behind unused queue_cfg */
        struct {
            virtio_pci_feature_cfg_t device_feature_cfg[VIRTIO_PCI_FEATURE_SELECT_COUNT];
            virtio_pci_feature_cfg_t driver_feature_cfg[VIRTIO_PCI_FEATURE_SELECT_COUNT];
            /* pciemgr observed device status nonzero -> zero */
            uint8_t need_reset;
        };
    };
} __attribute__((packed)) virtio_pci_common_cfg_t;

typedef struct virtio_net_config {
    /* The config defining mac address (if VIRTIO_NET_F_MAC) */
    uint8_t mac[6];

    /* See VIRTIO_NET_F_STATUS and VIRTIO_NET_S_* above */
    uint16_t status;

    /* Maximum number of each of transmit and receive queues;
     * see VIRTIO_NET_F_MQ and VIRTIO_NET_CTRL_MQ.
     * Legal values are between 1 and 0x8000
     */
    uint16_t max_virtqueue_pairs;

    /* Default maximum transmit unit advice */
    uint16_t mtu;

    /* Speed, in units of 1Mb. All values 0 to INT_MAX are legal.
     * Any other value stands for unknown.
     */
    uint32_t speed;

    /* 0x00 - half duplex
     * 0x01 - full duplex
     * Any other value stands for unknown.
     */
    uint8_t duplex;

    /* maximum size of RSS key */
    uint8_t rss_max_key_size;

    /* maximum number of indirection table entries */
    uint16_t rss_max_indirection_table_length;

    /* bitmask of supported VIRTIO_NET_RSS_HASH_ types */
    uint32_t supported_hash_types;
} __attribute__((packed)) virtio_net_config_t;

/* 2.1 - device status field */
enum {
    VIRTIO_S_ACKNOWLEDGE                = (1u << 0),
    VIRTIO_S_DRIVER                     = (1u << 1),
    VIRTIO_S_DRIVER_OK                  = (1u << 2),
    VIRTIO_S_FEATURES_OK                = (1u << 3),
    VIRTIO_S_NEEDS_RESET                = (1u << 6),
    VIRTIO_S_FAILED                     = (1u << 7),
};

/* 5.1.3 - network device - feature bits */
enum {
    VIRTIO_NET_F_CSUM                   = (1ull << 0),
    VIRTIO_NET_F_GUEST_CSUM             = (1ull << 1),
    VIRTIO_NET_F_CTRL_GUEST_OFFLOADS    = (1ull << 2),
    VIRTIO_NET_F_MTU                    = (1ull << 3),
    VIRTIO_NET_F_MAC                    = (1ull << 5),
    VIRTIO_NET_F_GSO                    = (1ull << 6),
    VIRTIO_NET_F_GUEST_TSO4             = (1ull << 7),
    VIRTIO_NET_F_GUEST_TSO6             = (1ull << 8),
    VIRTIO_NET_F_GUEST_ECN              = (1ull << 9),
    VIRTIO_NET_F_GUEST_UFO              = (1ull << 10),
    VIRTIO_NET_F_HOST_TSO4              = (1ull << 11),
    VIRTIO_NET_F_HOST_TSO6              = (1ull << 12),
    VIRTIO_NET_F_HOST_ECN               = (1ull << 13),
    VIRTIO_NET_F_HOST_UFO               = (1ull << 14),
    VIRTIO_NET_F_MRG_RXBUF              = (1ull << 15),
    VIRTIO_NET_F_STATUS                 = (1ull << 16),
    VIRTIO_NET_F_CTRL_VQ                = (1ull << 17),
    VIRTIO_NET_F_CTRL_RX                = (1ull << 18),
    VIRTIO_NET_F_CTRL_VLAN              = (1ull << 19),
    VIRTIO_NET_F_CTRL_RX_EXTRA          = (1ull << 20),
    VIRTIO_NET_F_GUEST_ANNOUNCE         = (1ull << 21),
    VIRTIO_NET_F_MQ                     = (1ull << 22),
    VIRTIO_NET_F_CTRL_MAC_ADDR          = (1ull << 23),
    VIRTIO_NET_F_NOTF_COAL              = (1ull << 53),
    VIRTIO_NET_F_HASH_REPORT            = (1ull << 57),
    VIRTIO_NET_F_RSS                    = (1ull << 60),
    VIRTIO_NET_F_RSS_EXT                = (1ull << 61),
    VIRTIO_NET_F_STANDBY                = (1ull << 62),
    VIRTIO_NET_F_SPEED_DUPLEX           = (1ull << 63),
};

/* 6 - reserved feature bits */
enum {
    VIRTIO_F_NOTIFY_ON_EMPTY            = (1ull << 24),
    VIRTIO_F_ANY_LAYOUT                 = (1ull << 27),
    VIRTIO_F_RING_INDIRECT_DESC         = (1ull << 28),
    VIRTIO_F_RING_EVENT_IDX             = (1ull << 29),
    VIRTIO_F_UNUSED                     = (1ull << 30),
    VIRTIO_F_VERSION_1                  = (1ull << 32),
    VIRTIO_F_ACCESS_PLATFORM            = (1ull << 33),
    VIRTIO_F_RING_PACKED                = (1ull << 34),
    VIRTIO_F_IN_ORDER                   = (1ull << 35),
    VIRTIO_F_ORDER_PLATFORM             = (1ull << 36),
    VIRTIO_F_SR_IOV                     = (1ull << 37),
    VIRTIO_F_NOTIFICATION_DATA          = (1ull << 38),
};

/* supported/enabled hash types */
enum {
    VIRTIO_NET_RSS_HASH_TYPE_IPv4       = (1u << 0),
    VIRTIO_NET_RSS_HASH_TYPE_TCPv4      = (1u << 1),
    VIRTIO_NET_RSS_HASH_TYPE_UDPv4      = (1u << 2),
    VIRTIO_NET_RSS_HASH_TYPE_IPv6       = (1u << 3),
    VIRTIO_NET_RSS_HASH_TYPE_TCPv6      = (1u << 4),
    VIRTIO_NET_RSS_HASH_TYPE_UDPv6      = (1u << 5),
    VIRTIO_NET_RSS_HASH_TYPE_IP_EX      = (1u << 6),
    VIRTIO_NET_RSS_HASH_TYPE_TCP_EX     = (1u << 7),
    VIRTIO_NET_RSS_HASH_TYPE_UDP_EX     = (1u << 8),
};

struct virtio_pci_notify_reg {
    uint8_t inc_pi_dbell[512];
    uint8_t set_pi_dbell[512];
};

#define VIRTIO_NOTIFY_MULTIPLIER 4

struct virtio_ident_reg {
    uint64_t hw_features;
    uint16_t max_vqs;
    uint16_t max_qlen;
    uint16_t min_qlen;
};

struct virtio_dev_regs {
    union {
        struct virtio_pci_common_cfg cmn_cfg;
        uint8_t part0[256];
    };
    union {
        struct virtio_net_config net_cfg;
        uint8_t dev_cfg[256];
        uint8_t part1[256];
    };
    union {
        struct virtio_ident_reg ident;
        uint8_t part2[512];
    };
    union {
        struct virtio_pci_notify_reg notify_reg;
        uint8_t part3[1024];
    };
    union {
        uint8_t isr_cfg[2048];
        uint8_t part4[2048];
    };
    /* indirect queue configs */
    struct virtio_pci_queue_cfg queue_cfg[VIRTIO_PCI_QUEUE_SELECT_COUNT];
} __attribute__((packed));

#define VIRTIO_DEV_REG_OFF(fld) offsetof(struct virtio_dev_regs, fld)
#define VIRTIO_DEV_REG_SZ(fld) sizeof(((struct virtio_dev_regs *)0)->fld)
#define VIRTIO_DEV_REG_ADDR(base, fld) ((base) + VIRTIO_DEV_REG_OFF(fld))

struct pvirtq_desc {
    uint64_t addr;      /* Buffer Address. */
    uint32_t len;       /* Buffer Length. */
    uint16_t id;        /* Buffer ID. */
    uint16_t flags;     /* The flags depending on descriptor type. */
};

#ifdef __cplusplus
}
#endif

#endif /* __VIRTIO_SPEC_H__ */
