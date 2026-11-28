/* FILE NAME:  netif_knl.c
 * PURPOSE:
 *      To provide operations registered to kernel callback and
 *      dispatch the operations to different chip drivers.
 *
 * NOTES:
 *
 */

/*****************************************************************************
 * INCLUDE FILE DECLARATIONS
 *****************************************************************************
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/bitmap.h>
#include <linux/delay.h>
#include <linux/ethtool.h>

#include <netif_osal.h>
#include "netif_dev.h"
#include "netif_knl.h"

#define NETIF_KNL_SUPPORT_CHIP          "wl"

#define NETIF_KNL_MODULE_DESC           "NETIF Kernel Module (" NETIF_KNL_SUPPORT_CHIP ")"

#define NETIF_KNL_DRIVER_MINOR_NUM      (252) /* DO NOT use MISC_DYNAMIC_MINOR */
#define NETIF_KNL_DRIVER_NAME           "gt_netif"

#define NETIF_KNL_IO_ERROR_RC           (-1)

typedef ssize_t
(*NETIF_KNL_DEV_TX_FUNC_T)(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos);

typedef long
(*NETIF_KNL_DEV_IOCTL_FUNC_T)(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg);

typedef GT_ERROR_NO_T
(*NETIF_KNL_DEV_INIT_T)(
    const UI32_T            chip);

typedef GT_ERROR_NO_T
(*NETIF_KNL_DEV_EXIT_T)(
    const UI32_T            chip);

typedef struct
{
    NETIF_KNL_DEV_IOCTL_FUNC_T          ioctl;
    NETIF_KNL_DEV_TX_FUNC_T             tx;
    NETIF_KNL_DEV_INIT_T                init;
    NETIF_KNL_DEV_EXIT_T                exit;
} NETIF_KNL_DEV_OPS_T;

typedef struct
{
    UI16_T                              dev_id;
    NETIF_KNL_DEV_OPS_T                 ops;

} NETIF_KNL_CB_T;

extern struct pci_dev                   *_ptr_ext_pci_dev;
static NETIF_KNL_CB_T                   _netif_knl_cb;
UI32_T                                  ext_dbg_flag = 0;
UI32_T                                  gt_dev_tc = 15;
UI32_T                                  vlan_push_flag = 1;
UI32_T                                  frame_vid = 0;
#if (defined(CONFIG_INTEL_IOMMU_DEFAULT_ON) || defined(CONFIG_INTEL_IOMMU_DEFAULT_ON_INTGPU_OFF))&& defined(CONFIG_INTEL_IOMMU)
UI32_T                                  intel_iommu_flag = 1;
#else
UI32_T                                  intel_iommu_flag = 0;
#endif


#define NETIF_KNL_DBG_FLAG_COMMON                   (0x1UL << 5)

#define NETIF_KNL_DBG(__flag__, ...)      do        \
{                                                   \
    if (0 != ((__flag__) & (ext_dbg_flag)))         \
    {                                               \
        osal_printf(__VA_ARGS__);                   \
    }                                               \
}while (0)


gt_device_t *gt_netif_obj;

static GT_ERROR_NO_T
_netif_knl_initDevOps(
    const UI16_T                        dev_id,
          NETIF_KNL_DEV_OPS_T           *ptr_ops)
{
    GT_ERROR_NO_T      rc = GT_E_OK;
    /* identify chip */

    return (rc);
}

static GT_ERROR_NO_T
_netif_knl_getDevId(
    UI16_T              *ptr_dev_id)
{
    if (_ptr_ext_pci_dev)
        pci_read_config_word(_ptr_ext_pci_dev, PCI_DEVICE_ID, ptr_dev_id);
    else
        *ptr_dev_id = 0;
    return (GT_E_OK);
}

static int
_netif_knl_dev_open(
    struct inode            *inode,
    struct file             *file)
{
    UI32_T              chip = 0;
    GT_ERROR_NO_T      rc;

    _netif_knl_getDevId(&_netif_knl_cb.dev_id);

    rc = _netif_knl_initDevOps(_netif_knl_cb.dev_id,
                               &_netif_knl_cb.ops);
    if (GT_E_OK == rc)
    {
        if (_netif_knl_cb.ops.init != NULL)
        {
            _netif_knl_cb.ops.init(chip);
        }
    }

    return (0);
}

static int
_netif_knl_dev_close(
    struct inode            *inode,
    struct file             *file)
{
    return (0);
}

static ssize_t
_netif_knl_dev_tx(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos)
{
    long ret = NETIF_KNL_IO_ERROR_RC;

    if (_netif_knl_cb.ops.tx != NULL)
    {
        ret = _netif_knl_cb.ops.tx(file, buf, count, pos);
    }

    return (ret);
}

static ssize_t
_netif_knl_dev_rx(
    struct file             *file,
    char __user             *buf,
    size_t                  count,
    loff_t                  *pos)
{
    return (0);
}

static int
gt_netif_di2id_bind(uint32_t unit, uint32_t port_id, uint32_t netif_id)
{
    gt_netif_obj(unit)->netif_di2id_map[port_id] = netif_id;
    return 0;
}

uint32_t
gt_netif_di2id_lookup(uint32_t unit, uint32_t port_id)
{
    return gt_netif_obj(unit)->netif_di2id_map[port_id];
}

static void
gt_netif_di2id_unbind(uint32_t unit, uint32_t port_id)
{
    gt_netif_obj(unit)->netif_di2id_map[port_id] = -1;
}

static void gt_netif_setup(struct net_device *ptr_net_dev);

int
gt_netif_net_dev_create(uint32_t chip, unsigned long arg)
{
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    struct gt_netif_ioctl_intf netif_cookie;
    uint32_t netif_id = 0;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct gt_netif_ioctl_intf)))
        return -EFAULT;

    if (netif_cookie.port_id >=GT_NETIF_PORT_DI_MAX_NUM) {
        dbg_print(DBG_INTF, "Invalid port_id%d. chip=%u\n", netif_cookie.port_id, chip);
        return -EINVAL;
    }

    netif_id = gt_netif_di2id_lookup(chip, netif_cookie.port_id);
    if (netif_id != -1) {
        dbg_print(DBG_INTF, "port_id %d already exists. chip=%u\n", netif_cookie.port_id, chip);
        return -EEXIST;
    }

    ptr_net_dev = dev_get_by_name(&init_net, netif_cookie.name);
    if (ptr_net_dev) {
        dev_put(ptr_net_dev);
        return -EEXIST;
    }

    // alloc netif id
    netif_cookie.id = find_first_zero_bit(gt_netif_obj(chip)->netif_id_bitmap,GT_NETIF_MAX_NUM);
    if (netif_cookie.id ==GT_NETIF_MAX_NUM) {
        dbg_print(DBG_PROFILE, "No available netif id. chip=%u\n", chip);
        return -ENOSPC;
    }
    set_bit(netif_cookie.id, gt_netif_obj(chip)->netif_id_bitmap);
    gt_netif_di2id_bind(chip, netif_cookie.port_id, netif_cookie.id);

    ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv), netif_cookie.name, NET_NAME_USER,
                               gt_netif_setup);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    eth_hw_addr_set(ptr_net_dev, netif_cookie.mac);
#else
    memcpy((void *)ptr_net_dev->dev_addr, netif_cookie.mac, ptr_net_dev->addr_len);
#endif
    ptr_priv = netdev_priv(ptr_net_dev);
    ptr_net_dev->mtu = gt_netif_obj(chip)->mtu;

    /* Port info will be used when packet sent from this netdev */
    ptr_priv->port_id = netif_cookie.port_id;
    ptr_priv->id = netif_cookie.id;
    ptr_priv->unit = chip;
    ptr_priv->max_mtu = gt_netif_obj(chip)->mtu;
    ptr_priv->speed = SPEED_1000; // default speed.
    ptr_priv->duplex = DUPLEX_FULL; // default duplex.

    register_netdev(ptr_net_dev);
    netif_carrier_off(ptr_net_dev);

    if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct gt_netif_ioctl_intf))) {
        return -EFAULT;
    }

    gt_netif_obj(chip)->netif_db[netif_cookie.id].intf.id = netif_cookie.id;
    gt_netif_obj(chip)->netif_db[netif_cookie.id].ptr_net_dev = ptr_net_dev;
    dbg_print(DBG_INTF, "Network device %s created, port_id:%d, ifindex:%d. chip=%u\n",
              ptr_net_dev->name, netif_cookie.port_id, ptr_net_dev->ifindex, chip);

    return 0;
}

int
gt_netif_net_dev_destroy(uint32_t chip, unsigned long arg)
{
    struct net_device *ptr_net_dev = NULL;
    struct net_device_priv *ptr_priv = NULL;
    uint32_t intf_id;
    int netif_id = 0;
    uint32_t port_id;

    if (copy_from_user(&intf_id, (void __user *)arg, sizeof(uint32_t)))
        return -EFAULT;

    for (netif_id = 0; netif_id <GT_NETIF_MAX_NUM; netif_id++) {
        if ((gt_netif_obj(chip)->netif_db[netif_id].intf.id == intf_id) &&
            (gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev != NULL)) {
            dbg_print(DBG_INTF, "Delete network device %s. chip=%u\n",
                      gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev->name, chip);
            ptr_net_dev = gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            port_id = ptr_priv->port_id;

            /* stop traffic and tx quque */
            netif_device_detach(ptr_net_dev);
            netif_tx_disable(ptr_net_dev);
            netif_carrier_off(ptr_net_dev);

            /* ungregister netdev */
            unregister_netdev(ptr_net_dev);
            gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev = NULL;
            gt_netif_di2id_unbind(chip, port_id);
            clear_bit(netif_id, gt_netif_obj(chip)->netif_id_bitmap);
            break;
        }
    }

    return 0;
}

int
gt_netif_net_dev_set(uint32_t chip, unsigned long arg)
{
    struct gt_netif_ioctl_intf netif_cookie;
    int netif_id = 0;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct gt_netif_ioctl_intf)))
        return -EFAULT;

    for (netif_id = 0; netif_id <GT_NETIF_MAX_NUM; netif_id++) {
        if ((gt_netif_obj(chip)->netif_db[netif_id].intf.id == netif_cookie.id) &&
            (gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);

            ether_addr_copy((u8 *)ptr_net_dev->dev_addr, netif_cookie.mac);
            if (netif_cookie.port_id != ptr_priv->port_id) {
                if (netif_cookie.port_id >= GT_NETIF_PORT_DI_MAX_NUM ||
                    ptr_priv->port_id >= GT_NETIF_PORT_DI_MAX_NUM) {
                    dbg_print(DBG_ERR, "Invalid port_id%d or %d. chip=%u\n", netif_cookie.port_id,
                              ptr_priv->port_id, chip);
                    return -EINVAL;
                }
                gt_netif_di2id_unbind(chip, ptr_priv->port_id);
                gt_netif_di2id_bind(chip, netif_cookie.port_id, netif_cookie.id);
            }
            ptr_priv->port_id = netif_cookie.port_id;
        }
    }

    if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct gt_netif_ioctl_intf)))
        return -EFAULT;
    return 0;
}

int
gt_netif_get_netdev(uint32_t chip, unsigned long arg)
{
    struct gt_netif_ioctl_intf netif_cookie;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;
    bool found = false;
    int netif_id = 0;

    if (copy_from_user(&netif_cookie, (void __user *)arg, sizeof(struct gt_netif_ioctl_intf)))
        return -EFAULT;

    for (netif_id = 0; netif_id <GT_NETIF_MAX_NUM; netif_id++) {
        if ((gt_netif_obj(chip)->netif_db[netif_id].intf.id == netif_cookie.id) &&
            (gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            memcpy(netif_cookie.mac, ptr_net_dev->dev_addr, ptr_net_dev->addr_len);
            memcpy(netif_cookie.name, ptr_net_dev->name,GT_NETIF_NAME_LEN);
            netif_cookie.port_id = ptr_priv->port_id;
            found = true;
            break;
        }
    }
    if (found) {
        if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct gt_netif_ioctl_intf)))
            return -EFAULT;
        dbg_print(DBG_INTF, "Found net device %s success. port_id:%u. chip=%u\n", ptr_net_dev->name,
                  ptr_priv->port_id, chip);
    } else {
        dbg_print(DBG_INTF, "Netdevice ID %u not found. chip=%u\n", netif_cookie.id, chip);
        netif_cookie.rc = gt_IOCTL_E_ENTRY_NOT_FOUND;
        if (copy_to_user((void __user *)arg, &netif_cookie, sizeof(struct gt_netif_ioctl_intf)))
            return -EFAULT;
        return 0;
    }

    return 0;
}

int
gt_netif_get_netdev_cnt(uint32_t chip, unsigned long arg)
{
    struct gt_netif_netdev_cnt cnt_cookie;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;
    bool found = false;
    int netif_id = 0;

    if (copy_from_user(&cnt_cookie, (void __user *)arg, sizeof(struct gt_netif_netdev_cnt)))
        return -EFAULT;

    for (netif_id = 0; netif_id <GT_NETIF_MAX_NUM; netif_id++) {
        if ((gt_netif_obj(chip)->netif_db[netif_id].intf.id == cnt_cookie.intf_id) &&
            (gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            cnt_cookie.rx_pkt = ptr_priv->stats.rx_packets;
            cnt_cookie.tx_pkt = ptr_priv->stats.tx_packets;
            cnt_cookie.tx_err = ptr_priv->stats.tx_errors;
            cnt_cookie.tx_queue_full = ptr_priv->stats.rx_fifo_errors;
            found = true;
            break;
        }
    }

    if (found) {
        if (copy_to_user((void __user *)arg, &cnt_cookie, sizeof(struct gt_netif_netdev_cnt))) {
            dbg_print(DBG_ERR, "Failed to copy netdev cnt to user space. chip=%u\n", chip);
            return -EFAULT;
        }
    } else {
        dbg_print(DBG_INTF, "Netdevice ID %u not found. chip=%u\n", cnt_cookie.intf_id, chip);
        cnt_cookie.rc = gt_IOCTL_E_ENTRY_NOT_FOUND;
        if (copy_to_user((void __user *)arg, &cnt_cookie, sizeof(struct gt_netif_netdev_cnt))) {
            dbg_print(DBG_ERR, "Failed to copy netdev cnt to user space. chip=%u\n", chip);
            return -EFAULT;
        }
        return 0;
    }

    return 0;
}

int
gt_netif_clear_netdev_cnt(uint32_t chip, unsigned long arg)
{
    uint32_t intf_id;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv = NULL;
    int netif_id = 0;

    if (copy_from_user(&intf_id, (void __user *)arg, sizeof(uint32_t)))
        return -EFAULT;

    for (netif_id = 0; netif_id <GT_NETIF_MAX_NUM; netif_id++) {
        if ((gt_netif_obj(chip)->netif_db[netif_id].intf.id == intf_id) &&
            (gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev != NULL)) {
            ptr_net_dev = gt_netif_obj(chip)->netif_db[netif_id].ptr_net_dev;
            ptr_priv = netdev_priv(ptr_net_dev);
            ptr_priv->stats.rx_packets = 0;
            ptr_priv->stats.tx_packets = 0;
            ptr_priv->stats.tx_errors = 0;
            ptr_priv->stats.rx_fifo_errors = 0;
            break;
        }
    }

    return 0;
}

/* to be updated*/
int
gt_netif_netdev_receive_skb(uint32_t unit, uint32_t port_id, struct sk_buff *ptr_skb)
{
    struct net_device_priv *ptr_priv;
    uint32_t netif_id;
    uint32_t pkt_len;

    ptr_skb->dev = NULL;
    netif_id = gt_netif_di2id_lookup(unit, port_id);
    if (netif_id == (uint32_t)-1) {
        dbg_print(DBG_RX, "invalid port_id,port_id=%d. unit=%u\n", port_id, unit);
        kfree_skb(ptr_skb);
        return -1;
    }
    ptr_skb->dev = gt_netif_obj(unit)->netif_db[netif_id].ptr_net_dev;
    if (!ptr_skb->dev) {
        kfree_skb(ptr_skb);
        dbg_print(DBG_RX, "ptr_net_dev is null pointer, netif_id=%d port_id=%d. unit=%u\n", netif_id, port_id, unit);
        return -1;
    }

    pkt_len = ptr_skb->len;
    ptr_priv = netdev_priv(ptr_skb->dev);
    ptr_skb->protocol = eth_type_trans(ptr_skb, ptr_skb->dev);
    ptr_skb->ip_summed = CHECKSUM_UNNECESSARY;

    if (netif_receive_skb(ptr_skb) == NET_RX_SUCCESS) {
        ptr_priv->stats.rx_packets++;
        ptr_priv->stats.rx_bytes += pkt_len;
    } else {
        ptr_priv->stats.rx_dropped++;
    }

    return 0;
}
EXPORT_SYMBOL(gt_netif_netdev_receive_skb);

static long
_netif_knl_dev_ioctl(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg)
{
    long ret = NETIF_KNL_IO_ERROR_RC;
    gt_ioctl_cmd_t         *ptr_cmd = (gt_ioctl_cmd_t *)&cmd;
    unsigned int            chip = ptr_cmd->field.chip;
    gt_ioctl_type_t         type = ptr_cmd->field.type;

    switch(type) {
        case NETIF_IOCTL_TYPE_CREATE_INTF:
            ret = gt_netif_net_dev_create(chip, arg);
            break;
        case NETIF_IOCTL_TYPE_DESTROY_INTF:
            ret = gt_netif_net_dev_destroy(chip, arg);
            break;
        case NETIF_IOCTL_TYPE_GET_INTF:
            ret = gt_netif_get_netdev(chip, arg);
            break;
        case NETIF_IOCTL_TYPE_SET_INTF:
            ret = gt_netif_net_dev_set(chip, arg);
            break;
        case NETIF_IOCTL_TYPE_CREATE_PROFILE:
            /* it is create in SDK user space scope*/
            break;
        case NETIF_IOCTL_TYPE_DESTROY_PROFILE:
            /* it is create in SDK user space scope*/
            break;
        case NETIF_IOCTL_TYPE_GET_PROFILE:
            /* it is create in SDK user space scope*/
            break;
        case NETIF_IOCTL_TYPE_GET_INTF_CNT:
            ret = gt_netif_get_netdev_cnt(chip, arg);
            break;
        case NETIF_IOCTL_TYPE_CLEAR_INTF_CNT:
            ret = gt_netif_clear_netdev_cnt(chip, arg);
            break;
        default:
            break;
    }
    dbg_print(DBG_DEBUG, "_netif_knl_dev_ioctl, chip: %u type: %u ret: %ld.\n", chip, type, ret);
    return (ret);
}


static int
gt_netif_net_dev_init(struct net_device *ptr_net_dev)
{
    return 0;
}

static int
gt_netif_net_dev_open(struct net_device *ptr_net_dev)
{
    netif_carrier_on(ptr_net_dev);
    netif_start_queue(ptr_net_dev);
    return 0;
}

static int
gt_netif_net_dev_stop(struct net_device *ptr_net_dev)
{
    netif_stop_queue(ptr_net_dev);
    return 0;
}

static int
gt_netif_net_dev_ioctl(struct net_device *ptr_net_dev, struct ifreq *ptr_ifreq, int cmd)
{
    return 0;
}

void
gt_netif_wake_tx_queues(uint32_t unit, uint32_t channel)
{
    int netif_id;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv;

    for (netif_id = 0; netif_id <GT_NETIF_MAX_NUM; netif_id++) {
        ptr_net_dev = gt_netif_obj(unit)->netif_db[netif_id].ptr_net_dev;
        if (ptr_net_dev == NULL)
            continue;
        ptr_priv = netdev_priv(ptr_net_dev);
        if (ptr_priv->tx_channel == channel && netif_queue_stopped(ptr_net_dev))
            netif_wake_queue(ptr_net_dev);
    }
}

static netdev_tx_t
gt_netif_net_dev_tx(struct sk_buff *ptr_skb, struct net_device *ptr_net_dev)
{
    struct net_device_priv *ptr_priv = NULL;
    uint32_t unit, port_id;
    uint32_t pkt_len;

    /* check skb */
    if (NULL == ptr_skb) {
        dbg_print(DBG_ERR, "skb is NULL\n");
        return NETDEV_TX_OK;
    }

    if (ptr_net_dev == NULL) {
        dbg_print(DBG_ERR, "net_device is NULL\n");
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_OK;
    }

    ptr_priv = netdev_priv(ptr_net_dev);
    if (ptr_priv == NULL) {
        dbg_print(DBG_ERR, "get netdev_priv failed\n");
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_OK;
    }

    if (!netif_running(ptr_net_dev)) {
        dbg_print(DBG_ERR, "net_device is not running\n");
        dev_kfree_skb_any(ptr_skb);
        return NETDEV_TX_OK;
    }

    unit = ptr_priv->unit;
    port_id = ptr_priv->port_id;
    pkt_len = ptr_skb->len;
    dbg_print(
        DBG_TX,
        "netdev:%s bind port_id:%d, pkt_len:%d, unit=%u\n",
        ptr_priv->ptr_net_dev->name, ptr_priv->port_id, pkt_len, unit);

    print_packet(DBG_TX_PAYLOAD, ptr_skb->data, ptr_skb->len);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 6, 7)
    ptr_net_dev->trans_start = jiffies;
#else
    netdev_get_tx_queue(ptr_net_dev, 0)->trans_start = jiffies;
#endif

    if (gt_netif_obj(unit)->tx_packet &&
        0 == gt_netif_obj(unit)->tx_packet(unit, port_id, ptr_skb)) {
        ptr_priv->stats.tx_packets += 1;
        ptr_priv->stats.tx_bytes += pkt_len;
    } else {
        ptr_priv->stats.tx_fifo_errors++;
        ptr_priv->stats.tx_dropped++;
    }

    dev_kfree_skb_any(ptr_skb);
    return NETDEV_TX_OK;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static void
gt_netif_net_dev_tx_timeout(struct net_device *ptr_net_dev, unsigned int txqueue)
#else
static void
gt_netif_net_dev_tx_timeout(struct net_device *ptr_net_dev)
#endif
{
    netif_stop_queue(ptr_net_dev);
    usleep_range(100, 1000);
    netif_wake_queue(ptr_net_dev);
}

static struct net_device_stats *
gt_netif_net_dev_get_stats(struct net_device *ptr_net_dev)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);

    return (&ptr_priv->stats);
}

static int
gt_netif_net_dev_set_mtu(struct net_device *ptr_net_dev, int new_mtu)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);
    if (new_mtu < 64 || new_mtu > ptr_priv->max_mtu) {
        return -EINVAL;
    }
    ptr_net_dev->mtu = new_mtu;
    return 0;
}
static int
gt_netif_net_dev_set_mac(struct net_device *ptr_net_dev, void *ptr_mac_addr)
{
    struct sockaddr *ptr_addr = ptr_mac_addr;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
    eth_hw_addr_set(ptr_net_dev, ptr_addr->sa_data);
#else
    memcpy((void *)ptr_net_dev->dev_addr, ptr_addr->sa_data, ptr_net_dev->addr_len);
#endif
    return 0;
}

static void
gt_netif_net_dev_set_rx_mode(struct net_device *ptr_dev)
{}

static int gt_netif_net_dev_get_link_ksettings(struct net_device *ptr_net_dev,
    struct ethtool_link_ksettings *cmd)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);

    //ethtool_link_ksettings_add_link_mode(cmd, advertising, 400000baseDR4_Full);
    cmd->base.speed = ptr_priv->speed;

    cmd->base.duplex = ptr_priv->duplex;
    // cmd->base.autoneg = AUTONEG_ENABLE;
    //cmd->base.port = PORT_OTHER;

    return 0;
}

static struct net_device_ops gt_netif_net_dev_ops = {
    .ndo_init = gt_netif_net_dev_init,
    .ndo_open = gt_netif_net_dev_open,
    .ndo_stop = gt_netif_net_dev_stop,
    .ndo_do_ioctl = gt_netif_net_dev_ioctl,
    .ndo_start_xmit = gt_netif_net_dev_tx,
    .ndo_tx_timeout = gt_netif_net_dev_tx_timeout,
    .ndo_get_stats = gt_netif_net_dev_get_stats,
    .ndo_change_mtu = gt_netif_net_dev_set_mtu,
    .ndo_set_mac_address = gt_netif_net_dev_set_mac,
    .ndo_set_rx_mode = gt_netif_net_dev_set_rx_mode,
};

static const struct ethtool_ops gt_netif_net_dev_ethtool_ops = {
    .get_link_ksettings = gt_netif_net_dev_get_link_ksettings,
};

static void
gt_netif_setup(struct net_device *ptr_net_dev)
{
    struct net_device_priv *ptr_priv = netdev_priv(ptr_net_dev);

    /* setup net device */
    ether_setup(ptr_net_dev);
    ptr_net_dev->netdev_ops = &gt_netif_net_dev_ops;
    ptr_net_dev->ethtool_ops = &gt_netif_net_dev_ethtool_ops;
    ptr_net_dev->watchdog_timeo = 30 * HZ;
    ptr_net_dev->mtu = 1500;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
    ptr_net_dev->min_mtu = 64;
    ptr_net_dev->max_mtu = 65535;
#endif
    eth_hw_addr_random(ptr_net_dev);
    // random_ether_addr(ptr_net_dev->dev_addr); /* Please use the mac-addr of interface. */

    /* setup private data */
    ptr_priv->ptr_net_dev = ptr_net_dev;
    memset(&ptr_priv->stats, 0, sizeof(struct net_device_stats));
}
#ifdef CONFIG_COMPAT
static long
_netif_knl_dev_compat_ioctl(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg)
{
    return _netif_knl_dev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static struct file_operations _netif_knl_dev_ops =
{
    .owner          = THIS_MODULE,
    .open           = _netif_knl_dev_open,
    .release        = _netif_knl_dev_close,
    .write          = _netif_knl_dev_tx,
    .read           = _netif_knl_dev_rx,
    .unlocked_ioctl = _netif_knl_dev_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = _netif_knl_dev_compat_ioctl,
#endif
};

static struct miscdevice _netif_knl_dev =
{
    .minor    = NETIF_KNL_DRIVER_MINOR_NUM,
    .name     = NETIF_KNL_DRIVER_NAME,
    .fops     = &_netif_knl_dev_ops,
};

static int __init
netif_knl_init(void)
{
    uint32_t unit = 0;
    int rc;

    gt_netif_obj = kzalloc(sizeof(gt_device_t), GFP_ATOMIC);
    if (!gt_netif_obj)
        return -ENOMEM;

    rc = misc_register(&_netif_knl_dev);
    if (rc) {
        dbg_print(DBG_ERR, "Failed to register misc device, rc:%d\n", rc);
        kfree(gt_netif_obj);
        return rc;
    }

    osal_memset(&_netif_knl_cb, 0x0, sizeof(NETIF_KNL_CB_T));

    /* SHM transport init */
    rc = shm_net_init();
    if (rc) {
        dbg_print(DBG_ERR, "Failed to init shm_net, rc:%d\n", rc);
        misc_deregister(&_netif_knl_dev);
        kfree(gt_netif_obj);
        return rc;
    }

    /* packet DMA driver init */
    rc = netif_obj_init(unit, NULL);
    if (rc) {
        dbg_print(DBG_ERR, "Failed to init netif obj, rc:%d\n", rc);
        shm_net_exit();
        misc_deregister(&_netif_knl_dev);
        kfree(gt_netif_obj);
        return rc;
    }

    return (0);
}

void
print_packet(uint32_t loglvl, const unsigned char *data, size_t len)
{
    /* stub: real implementation from SDK */
}

static void __exit
netif_knl_exit(void)
{
    UI32_T      chip = 0;
    int         netif_id;
    struct net_device *ptr_net_dev;
    struct net_device_priv *ptr_priv;
    gt_netif_drv_cb_t *pkt_drv;

    if (_netif_knl_cb.ops.exit != NULL)
        _netif_knl_cb.ops.exit(chip);

    pkt_drv = gt_netif_obj ? gt_netif_obj(chip) : NULL;

    /* Destroy all remaining netdevs before module memory is freed */
    if (pkt_drv) {
        for (netif_id = 0; netif_id < GT_NETIF_MAX_NUM; netif_id++) {
            ptr_net_dev = pkt_drv->netif_db[netif_id].ptr_net_dev;
            if (ptr_net_dev) {
                ptr_priv = netdev_priv(ptr_net_dev);

                unregister_netdev(ptr_net_dev);
                pkt_drv->netif_db[netif_id].ptr_net_dev = NULL;
                pkt_drv->netif_di2id_map[ptr_priv->port_id] = (uint32_t)-1;
                clear_bit(netif_id, pkt_drv->netif_id_bitmap);
            }
        }
    }

    /* Free pkt_drv resources */
    if (gt_netif_obj && pkt_drv)
        gt_pkt_drv_deinit(chip);

    /* Free virt dev driver allocations */
    if (gt_netif_obj) {
        int i;
        for (i = 0; i < gt_netif_obj->virt_dev_num; i++) {
            struct gt_virt_dev_s *vd = gt_netif_obj->gt_virt_dev[i];
            if (vd && vd->gt_drv) {
                if (vd->gt_drv->pkt_drv)
                    kfree(vd->gt_drv->pkt_drv);
                kfree(vd->gt_drv);
                vd->gt_drv = NULL;
            }
        }
    }

    shm_net_exit();
    misc_deregister(&_netif_knl_dev);

    kfree(gt_netif_obj);
    gt_netif_obj = NULL;

    pr_info("gt_netif: unloaded\n");
}

module_init(netif_knl_init);
module_exit(netif_knl_exit);

module_param(ext_dbg_flag, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ext_dbg_flag, "bit0:error, bit1:tx, bit2:rx, bit3:intf, bit4:profile, "   \
                               "bit5:common, bit6:netlink");
module_param(gt_dev_tc, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gt_dev_tc, "set tc from 0-15");
module_param(vlan_push_flag, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(vlan_push_flag, "0:flase 1:true");
module_param(frame_vid, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(frame_vid, "VLAN ID (VID) indicates the VLAN to which a frame belongs (default 0)");
module_param(intel_iommu_flag, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(intel_iommu_flag, "intel iommu on:1, intel iommu off:0");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("grandtrans");
MODULE_DESCRIPTION(NETIF_KNL_MODULE_DESC);
