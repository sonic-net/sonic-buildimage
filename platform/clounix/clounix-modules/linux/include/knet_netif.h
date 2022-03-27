#ifndef __CLX_NETIF_H__
#define __CLX_NETIF_H__

#include "knet_types.h"
#include "knet_common.h"
#include "knet_dma.h"
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <net/genetlink.h>

#define CLX_NETIF_MAX_NUM         (288)
#define CLX_PROFILE_MAX_NUM       (288)
#define CLX_NETLINK_MAX_NUM       (288)
#define CLX_NETIF_WAIT_RX_TIMEOUT (3000)
#define CLX_NETIF_REASON_MAX      (512)

#define CLX_NETIF_PORT_MAP_INDEX(unit, slice, port)                                              \
    (unit * clx_netif_drv(unit)->ports_num_unit + slice * clx_netif_drv(unit)->ports_per_slice + \
     port)
#define CLX_NETIF_GET_PORT_DI(unit, slice, slice_port) \
    clx_netif_drv(unit)->ptr_port_map_db[CLX_NETIF_PORT_MAP_INDEX(unit, slice, slice_port)]
#define CLX_NETIF_SET_PORT_DI(unit, slice, slice_port, di) \
    clx_netif_drv(unit)->ptr_port_map_db[CLX_NETIF_PORT_MAP_INDEX(unit, slice, slice_port)] = di

struct net_device_priv {
    struct net_device *ptr_net_dev;
    struct net_device_stats stats;
    uint32_t unit;
    uint32_t id;
    uint32_t port;
    uint16_t vlan;
    uint32_t speed;
    uint32_t tx_channel;
    uint32_t max_mtu;
    uint8_t vlan_tag_type;
    uint8_t skip_port_state_event;
    /* psample */
    uint32_t igr_sample_rate;
    uint32_t egr_sample_rate;
    uint32_t tc;

    /* cpu reason cnt*/
    struct clx_netif_ioctl_rx_reason_cnt rx_reason_cnt[CLX_NETIF_REASON_MAX];
};

struct clx_netif_cnt {
    uint32_t packets;
    uint32_t bytes;
    uint32_t enqueue_ok;
    uint32_t enqueue_fail;
    uint32_t dequeue_ok;
    uint32_t dequeue_fail;
    uint32_t interrupts;
};

struct netif_port {
    struct clx_netif_ioctl_intf intf;
    struct net_device *ptr_net_dev;
};

struct profile_list {
    struct list_head list;
    spinlock_t lock;
    DECLARE_BITMAP(profile_id_bitmap, CLX_PROFILE_MAX_NUM);
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
    uint16_t igr_port_si;
};

struct netlink_rx_cookie {
    struct clx_netif_rx_dst_netlink *nl;
    struct netlink_rx_pkt_extra pkt;
    netlink_rx_type_e netlink_type;
};

struct netlink_list {
    struct list_head list;
    spinlock_t lock;
    DECLARE_BITMAP(netlink_id_bitmap, CLX_NETLINK_MAX_NUM);
};

/* NETIF DRV */
typedef int (*clx_pkt_get_dst)(uint32_t unit,
                               struct dma_rx_packet *rx_packet,
                               uint32_t *rx_port,
                               uint32_t *cpu_reason);
typedef int (*clx_pkt_parse_pkt_info)(uint32_t unit,
                                      struct dma_rx_packet *rx_packet,
                                      void *ptr_cookie);

typedef struct {
    uint32_t mtu;
    uint32_t cpu_port;
    clx_pkt_get_dst get_pkt_dst;
    clx_pkt_parse_pkt_info parse_netlink_info;

    struct netif_port port_db[CLX_NETIF_MAX_NUM];
    struct profile_list profile;
    struct netlink_list netlink;
    /* cnt */
    struct clx_netif_cnt cnt;
    struct clx_pkt_rx_reason_cnt cpu_reason_cnt[CLX_NETIF_REASON_MAX];

    /* for mod dmac modify */
    bool enable_mod_dmac;
    clx_mac_t mod_dmac;

    /* port map */
    uint32_t unit_num;
    uint32_t ports_num_unit;
    uint32_t slices_per_unit;
    uint32_t ports_per_slice;
    uint32_t *ptr_port_map_db; /* [unit_num][slices_per_unit * ports_per_slice] */
} clx_netif_drv_cb_t;

/* netdevice operation */
int
clx_netif_net_dev_create(uint32_t unit, unsigned long arg);
int
clx_netif_net_dev_destroy(uint32_t unit, unsigned long arg);
int
clx_netif_net_dev_set(uint32_t unit, unsigned long arg);
int
clx_netif_get_netdev(uint32_t unit, unsigned long arg);
int
clx_netif_set_intf_attr(uint32_t unit, unsigned long arg);
int
clx_netif_get_intf_attr(uint32_t unit, unsigned long arg);
int
clx_netif_get_netdev_cnt(uint32_t unit, unsigned long arg);
int
clx_netif_clear_netdev_cnt(uint32_t unit, unsigned long arg);
int
clx_netif_get_rx_reason_cnt(uint32_t unit, unsigned long arg);
int
clx_netif_clear_rx_reason_cnt(uint32_t unit, unsigned long arg);

/* receive/send packet from/to sdk */
int
clx_netif_receive_to_sdk(uint32_t unit, unsigned long arg);
int
clx_netif_send_from_sdk(uint32_t unit, unsigned long arg);

/* packet to kernel */
struct sk_buff *
netif_construct_skb_from_rx_packet(uint32_t unit, uint32_t port, struct dma_rx_packet *rx_packet);
int
clx_netif_netdev_receive_skb(uint32_t unit, struct dma_rx_packet *rx_packet, uint32_t port);

/* profile operation */
int
clx_netif_rx_profile_create(uint32_t unit, unsigned long arg);
int
clx_netif_rx_profile_destroy(uint32_t unit, unsigned long arg);
int
clx_netif_rx_profile_get(uint32_t unit, unsigned long arg);
struct profile_rule *
clx_netif_match_profile(uint32_t unit,
                        uint32_t port,
                        uint32_t reason,
                        struct dma_rx_packet *rx_packet);

/* Netlink operation */
int
clx_netif_create_netlink(uint32_t unit, unsigned long arg);
int
clx_netif_destroy_netlink(uint32_t unit, unsigned long arg);
int
clx_netif_get_netlink(uint32_t unit, unsigned long arg);
int
clx_netif_set_pkt_mod(uint32_t unit, unsigned long arg);
int
clx_netif_get_pkt_mod(uint32_t unit, unsigned long arg);
int
netif_netlink_reveive_skb(uint32_t unit,
                          struct dma_rx_packet *rx_packet,
                          uint32_t port,
                          void *ptr_cookie);
int
clx_netlink_init(void);
void
clx_netlink_deinit(void);

int
clx_netif_set_port_map(uint32_t unit, unsigned long arg);

int
clx_netif_init(void);
void
clx_netif_deinit(void);

void
print_packet(uint32_t loglvl, const unsigned char *data, size_t len);

int
clx_pkt_get_rx_reason_cnt(uint32_t unit, unsigned long arg);

int
clx_pkt_clear_rx_reason_cnt(uint32_t unit, unsigned long arg);

#endif // __CLX_NETIF_H__