
#ifndef __NETIF_KNL_H__
#define __NETIF_KNL_H__

#include "netif_types.h"
#include "netif_pub.h"
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <net/genetlink.h>

#define GT_NETIF_MAX_NUM         (512)
#define GT_NETIF_PORT_DI_MAX_NUM (512)
#define GT_NETIF_WAIT_RX_TIMEOUT (3000)
#define GT_NETIF_PKT_SEND_RETYR_NUM (100)
#define GT_NETIF_DFLT_VLAN       (1)

struct net_device_priv {
    struct net_device *ptr_net_dev;
    struct net_device_stats stats;
    uint32_t unit;
    uint32_t id;
    uint32_t port_id;
    uint16_t vlan;
    uint16_t duplex;
    uint32_t speed;
    uint32_t tx_channel;
    uint32_t max_mtu;
    uint8_t vlan_tag_type;
    uint8_t skip_port_state_event;
    /* psample */
    uint32_t igr_sample_rate;
    uint32_t egr_sample_rate;
    uint32_t tc;
};

struct gt_netif_cnt {
    uint32_t packets;
    uint32_t bytes;
    uint32_t enqueue_ok;
    uint32_t enqueue_fail;
    uint32_t dequeue_ok;
    uint32_t dequeue_fail;
    uint32_t interrupts;
};

struct gt_netif_ioctl_intf {
    uint32_t id; /* unique key */
    char name[GT_NETIF_NAME_LEN];
    uint32_t port_id;
    gt_mac_t mac;
    gt_ioctl_error_no_t rc;
};

struct netif_port {
    struct gt_netif_ioctl_intf intf;
    struct net_device *ptr_net_dev;
};

struct gt_netif_netdev_cnt {
    uint32_t intf_id;
    uint64_t rx_pkt;
    uint64_t tx_pkt;
    uint64_t tx_err;
    uint64_t tx_queue_full;
    gt_ioctl_error_no_t rc;
};

typedef struct {
    uint16_t pvid;  // [kg] igress pph igr_vlan is invalid;
} port_db_t;

struct profile_list {
    struct list_head list;
    spinlock_t lock;
    DECLARE_BITMAP(profile_id_bitmap, GT_PROFILE_MAX_NUM);
};

/* Netlink structure */
typedef enum {
    NETIF_NL_PKT_PSAMPLE_OTHER = 0,
    NETIF_NL_PKT_PSAMPLE_INGRESS,
    NETIF_NL_PKT_PSAMPLE_EGRESS,
    NETIF_NL_PKT_PSAMPLE_MAX,
} netlink_psample_dir_e;

typedef enum {
    NETLINK_RX_TYPE_OTHER,
    NETLINK_RX_TYPE_SFLOW,
    NETLINK_RX_TYPE_MOD,
} netlink_rx_type_e;

struct netlink_rx_pkt_extra {
    uint16_t iifindex;
    uint16_t eifindex;
    netlink_psample_dir_e psample_dir;
    uint32_t sample_rate;
    uint32_t igr_port_si;
};

struct netlink_rx_cookie {
    struct gt_netif_rx_dst_netlink *nl;
    struct netlink_rx_pkt_extra pkt;
    netlink_rx_type_e netlink_type;
};

struct netlink_list {
    struct list_head list;
    spinlock_t lock;
    DECLARE_BITMAP(netlink_id_bitmap, GT_NETLINK_MAX_NUM);
};

/* NETIF DRV */
typedef int (*gt_pkt_get_dst)(uint32_t unit,
                               struct dma_rx_packet *rx_packet,
                               uint32_t *rx_port,
                               uint32_t *cpu_reason);
typedef int (*gt_pkt_parse_pkt_info)(uint32_t unit,
                                      struct dma_rx_packet *rx_packet,
                                      void *ptr_cookie);
typedef int (*gt_pkt_tx_packet)(uint32_t unit, uint32_t port_id, struct sk_buff *ptr_skb);

typedef struct gt_netif_drv_cb {
    uint32_t mtu;
    uint32_t cpu_port;
    gt_pkt_get_dst get_pkt_dst;
    gt_pkt_parse_pkt_info parse_netlink_info;
    gt_pkt_tx_packet tx_packet;
    uint32_t netif_di2id_map[GT_NETIF_PORT_DI_MAX_NUM];
    DECLARE_BITMAP(netif_id_bitmap, GT_NETIF_MAX_NUM);
    struct netif_port netif_db[GT_NETIF_MAX_NUM];
    struct profile_list profile;
    struct netlink_list netlink;

    /* cnt */
    struct gt_netif_cnt cnt;

    /* for mod dmac modify */
    bool enable_mod_dmac;
    gt_mac_t mod_dmac;

    /* port_id map */
    uint32_t ports_num_unit;
    uint32_t slices_per_unit;
    uint32_t unit_num;
    uint32_t dies_per_unit;
    uint32_t slices_per_die;
    uint32_t ports_per_slice;
    uint32_t *ptr_port_map_db;
    port_db_t port_db[GT_NETIF_PORT_DI_MAX_NUM];
} gt_netif_drv_cb_t;

/* netdevice operation */
int
gt_netif_net_dev_create(uint32_t unit, unsigned long arg);
int
gt_netif_net_dev_destroy(uint32_t unit, unsigned long arg);
int
gt_netif_net_dev_set(uint32_t unit, unsigned long arg);
int
gt_netif_get_netdev(uint32_t unit, unsigned long arg);
int
gt_netif_set_intf_attr(uint32_t unit, unsigned long arg);
int
gt_netif_get_intf_attr(uint32_t unit, unsigned long arg);
int
gt_netif_get_netdev_cnt(uint32_t unit, unsigned long arg);
int
gt_netif_clear_netdev_cnt(uint32_t unit, unsigned long arg);

/* receive/send packet from/to sdk */
int
gt_netif_receive_to_sdk(uint32_t unit, unsigned long arg);
int
gt_netif_receive_fd_to_sdk(uint32_t unit, unsigned long arg);
int
gt_netif_send_from_sdk(uint32_t unit, unsigned long arg);

/* packet to kernel */
struct sk_buff *
netif_construct_skb_from_rx_packet(uint32_t unit,
                                   uint32_t port_id,
                                   struct dma_rx_packet *rx_packet);
struct sk_buff *
netif_construct_fast_skb_from_rx_packet(uint32_t unit, struct dma_rx_packet *rx_packet);

int
gt_netif_netdev_receive_skb(uint32_t unit, uint32_t port_id, struct sk_buff *ptr_skb);

/* profile operation */
int
gt_netif_rx_profile_create(uint32_t unit, unsigned long arg);
int
gt_netif_rx_profile_destroy(uint32_t unit, unsigned long arg);
int
gt_netif_rx_profile_get(uint32_t unit, unsigned long arg);
struct profile_rule *
gt_netif_match_profile(uint32_t unit,
                        uint32_t port_id,
                        uint32_t reason,
                        struct dma_rx_packet *rx_packet);

/* Netlink operation */
int
gt_netif_create_netlink(uint32_t unit, unsigned long arg);
int
gt_netif_destroy_netlink(uint32_t unit, unsigned long arg);
int
gt_netif_get_netlink(uint32_t unit, unsigned long arg);
int
gt_netif_set_pkt_mod(uint32_t unit, unsigned long arg);
int
gt_netif_get_pkt_mod(uint32_t unit, unsigned long arg);
int
netif_netlink_reveive_skb(uint32_t unit,
                          struct dma_rx_packet *rx_packet,
                          uint32_t port_id,
                          void *ptr_cookie);
int
gt_netlink_init(void);
void
gt_netlink_deinit(void);

int
gt_netif_set_port_map(uint32_t unit, unsigned long arg);
int
gt_netif_get_port_map(uint32_t unit, unsigned long arg);

int
gt_netif_set_port_attr(uint32_t unit, unsigned long arg);

int
gt_netif_get_port_attr(uint32_t unit, unsigned long arg);

int
gt_netif_init(void);
void
gt_netif_deinit(void);

int __init shm_net_init(void);
void shm_net_exit(void);

void
print_packet(uint32_t loglvl, const unsigned char *data, size_t len);

uint32_t
gt_netif_di2id_lookup(uint32_t unit, uint32_t di);

void
gt_netif_performance_test_rx(uint32_t unit, struct dma_rx_packet *rx_packet);
int
gt_netif_netdev_receive_send_ifa(uint32_t unit, struct dma_rx_packet *rx_packet, uint32_t port_id);

int
gt_netif_set_ifa_cfg(uint32_t unit, unsigned long arg);

int
gt_netif_get_ifa_cfg(uint32_t unit, unsigned long arg);

netdev_tx_t
gt_netif_fast_tx(uint32_t unit, struct sk_buff *ptr_skb, uint16_t di, uint8_t tc);

void
gt_netif_wake_tx_queues(uint32_t unit, uint32_t channel);

void
gt_netif_performance_test_tx(uint32_t unit, struct sk_buff *ptr_skb);

void
netif_process_skb_from_rx_packet(uint32_t unit,
                                 uint32_t port_id,
                                 struct dma_rx_packet *rx_packet,
                                 struct sk_buff *ptr_merge_skb);

struct net_device *
gt_netif_get_netdev_from_sys_port(uint32_t unit, uint16_t sys_port);
#endif // __NETIF_KNL_H__
