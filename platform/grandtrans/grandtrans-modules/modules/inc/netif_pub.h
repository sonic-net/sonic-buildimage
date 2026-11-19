#ifndef _NETIF_PUB_H_
#define _NETIF_PUB_H_

#include "netif_types.h"

#define GT_NETIF_NAME_LEN     16
#define GT_NETIF_NAME_LEN    16
#define NETIF_NAME_LEN        16

#define GT_PROFILE_MAX_NUM    256
#define GT_NETLINK_MAX_NUM    64
#define gt_MAX_CHIP_NUM       8
#define gt_MAX_VIRT_DEV_NUM   8

#define NB_MAX_UNIT_NUM       8

/* Debug flags */
#define DBG_CRIT       (0x1UL << 0)
#define DBG_ERR        (0x1UL << 1)
#define DBG_WARN       (0x1UL << 2)
#define DBG_INFO       (0x1UL << 3)
#define DBG_TX         (0x1UL << 4)
#define DBG_RX         (0x1UL << 5)
#define DBG_TX_PAYLOAD (0x1UL << 6)
#define DBG_RX_PAYLOAD (0x1UL << 7)
#define DBG_INTF       (0x1UL << 8)
#define DBG_PROFILE    (0x1UL << 9)
#define DBG_NETLINK    (0x1UL << 10)
#define DBG_INTR       (0x1UL << 11)
#define DBG_DEBUG      (0x1UL << 12)

extern UI32_T ext_dbg_flag;

/* ---- interrupt types ---- */
typedef struct {
    uint32_t irq_num;
} gt_intr_info_t;

typedef struct {
    uint32_t num_irqs;
} gt_intr_drv_cb_t;

/* ---- DMA types ---- */
struct dma_rx_packet {
    uint8_t *data;
    uint32_t len;
    uint32_t port_id;
    uint32_t reason;
};

/* ---- forward declarations for types fully defined in netif_knl.h ---- */
struct gt_netif_drv_cb;
typedef struct gt_netif_drv_cb gt_netif_drv_cb_t;

struct profile_rule;
struct gt_netif_rx_dst_netlink;

#endif
