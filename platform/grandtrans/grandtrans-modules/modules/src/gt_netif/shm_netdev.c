/*
 * shm_netdev.c -- SHM Ring Netdev Transport
 *
 * Provides shared-memory ring buffer transport for virtual netdevs.
 * Netdev create/destroy/ops are handled by netif_knl.c via the unified
 * net_device_ops.  This module registers a tx_packet callback.
 *
 * /dev/shm_net : mmap / poll / write(wake rx thread)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include "shm_ring.h"
#include "netif_dev.h"
#include "netif_knl.h"

#define SHM_DEV_NAME    "shm_net"

struct shm_dev {
    struct miscdevice   mdev;
    struct shm_region   *region;
    size_t              region_size;
    wait_queue_head_t   tx_wq;
    atomic_t            tx_avail;
    spinlock_t          tx_lock;        /* protects tx ring enqueue (MPSC) */
    wait_queue_head_t   rx_wq;          /* wake rx kthread */
    struct task_struct  *rx_thread;     /* drains rx ring -> netif_rx */
};

static struct shm_dev *g_shm;

static int shm_tx_packet(uint32_t unit, uint32_t port_id, struct sk_buff *skb);

/* ---- tx ring: locked enqueue (MPSC) ---- */
static int shm_tx_enqueue_locked(struct shm_dev *sd,
                                  const struct shm_metadata *meta,
                                  const uint8_t *data, uint32_t len)
{
    int rc;

    spin_lock(&sd->tx_lock);
    rc = shm_ring_enqueue(&sd->region->tx, meta, data, len);
    spin_unlock(&sd->tx_lock);
    return rc;
}

/*
 * shm_tx_packet — registered as gt_netif_obj(unit)->tx_packet.
 * Called from gt_netif_net_dev_tx (unified net_device_ops in netif_knl.c).
 */
static int shm_tx_packet(uint32_t unit, uint32_t port_id, struct sk_buff *skb)
{
    struct shm_dev *sd = g_shm;
    struct shm_metadata meta;
    int rc;

    if (!sd || !sd->region)
        return -1;

    meta.netif_id = 0;
    meta.port_id  = (uint16_t)port_id;
    memset(meta.rsvd, 0, sizeof(meta.rsvd));

    rc = shm_tx_enqueue_locked(sd, &meta, skb->data, skb->len);
    if (rc < 0)
        return -1;

    atomic_set(&sd->tx_avail, 1);
    wake_up_interruptible(&sd->tx_wq);
    return 0;
}

/* ---- Rx kernel thread: drains rx ring -> netif_rx ---- */
static int shm_rx_thread(void *data)
{
    struct shm_dev *sd = data;
    struct sk_buff *skb;
    struct shm_metadata meta;
    uint8_t *pkt;
    uint32_t total_len, data_len;
    uint32_t unit = 0;

    pkt = kmalloc(SHM_PKT_SIZE, GFP_KERNEL);
    if (!pkt)
        return -ENOMEM;

    while (!kthread_should_stop()) {
        wait_event_interruptible(sd->rx_wq,
            !shm_ring_empty(&sd->region->rx) || kthread_should_stop());

        if (kthread_should_stop())
            break;

        /* drain all available packets */
        while (shm_ring_dequeue(&sd->region->rx, &meta,
                                pkt, &total_len, &data_len) == 0) {
            if (data_len < ETH_HLEN)
                continue;

            skb = dev_alloc_skb(data_len);
            if (!skb)
                break;

            skb_put_data(skb, pkt, data_len);

            gt_netif_netdev_receive_skb(unit, meta.port_id, skb);
        }
    }

    kfree(pkt);
    return 0;
}

/* ---- Char device ops ---- */
static int shm_fop_open(struct inode *inode, struct file *filp)
{
    filp->private_data = g_shm;
    return 0;
}

static int shm_fop_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static int shm_fop_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct shm_dev *sd = filp->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (size > PAGE_ALIGN(sd->region_size))
        return -EINVAL;

    vm_flags_set(vma, VM_DONTEXPAND);
    return remap_vmalloc_range(vma, sd->region, 0);
}

static ssize_t shm_fop_write(struct file *filp, const char __user *buf,
                             size_t count, loff_t *pos)
{
    struct shm_dev *sd = filp->private_data;

    if (!sd->region)
        return -ENODEV;

    /* just wake the rx kthread; it drains the ring */
    wake_up_interruptible(&sd->rx_wq);
    return count;
}

static __poll_t shm_fop_poll(struct file *filp, poll_table *wait)
{
    struct shm_dev *sd = filp->private_data;
    __poll_t mask = 0;

    poll_wait(filp, &sd->tx_wq, wait);

    if (!shm_ring_empty(&sd->region->tx))
        mask |= POLLIN | POLLRDNORM;

    if (atomic_read(&sd->tx_avail)) {
        atomic_set(&sd->tx_avail, 0);
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

gt_netif_drv_cb_t virt_dev_pkt_driver = {
    .mtu                = 9600,
    .cpu_port           = 256,
    .unit_num           = NB_MAX_UNIT_NUM,
    .get_pkt_dst        = NULL,
    .parse_netlink_info = NULL,
    .ptr_port_map_db    = NULL,
    .tx_packet          = shm_tx_packet,
};

int
netif_virt_init(uint32_t unit, gt_drv_cb_t *ptr_gt_drv)
{
    int ret = 0;

    memset(ptr_gt_drv, 0, sizeof(gt_drv_cb_t));
    ptr_gt_drv->pkt_drv = (gt_netif_drv_cb_t *)kmalloc(sizeof(gt_netif_drv_cb_t), GFP_KERNEL);
    if (!ptr_gt_drv->pkt_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for pkt_drv. unit=%d\n", unit);
        return -ENOMEM;
    }
    dbg_print(DBG_DEBUG, "Allocate pkt_drv. unit=%d ptr_gt_drv->pkt_drv=%p\n", unit,
              ptr_gt_drv->pkt_drv);
    memcpy(ptr_gt_drv->pkt_drv, &virt_dev_pkt_driver, sizeof(gt_netif_drv_cb_t));

    return ret;
}

static const struct file_operations shm_fops = {
    .owner          = THIS_MODULE,
    .open           = shm_fop_open,
    .release        = shm_fop_release,
    .mmap           = shm_fop_mmap,
    .write          = shm_fop_write,
    .poll           = shm_fop_poll,
};

/* ---- Init / Exit ---- */
int __init shm_net_init(void)
{
    struct shm_dev *sd;
    int ret;

    sd = kzalloc(sizeof(*sd), GFP_KERNEL);
    if (!sd)
        return -ENOMEM;

    sd->region_size = sizeof(struct shm_region);
    sd->region = vmalloc_user(sd->region_size);
    if (!sd->region) {
        kfree(sd);
        return -ENOMEM;
    }
    memset(sd->region, 0, sd->region_size);

    init_waitqueue_head(&sd->tx_wq);
    init_waitqueue_head(&sd->rx_wq);
    atomic_set(&sd->tx_avail, 0);
    spin_lock_init(&sd->tx_lock);

    /* start rx kernel thread */
    sd->rx_thread = kthread_run(shm_rx_thread, sd, "shm_rx");
    if (IS_ERR(sd->rx_thread)) {
        vfree(sd->region);
        kfree(sd);
        return PTR_ERR(sd->rx_thread);
    }

    /* Register misc char device */
    sd->mdev.minor = MISC_DYNAMIC_MINOR;
    sd->mdev.name  = SHM_DEV_NAME;
    sd->mdev.fops  = &shm_fops;

    ret = misc_register(&sd->mdev);
    if (ret) {
        kthread_stop(sd->rx_thread);
        vfree(sd->region);
        kfree(sd);
        return ret;
    }

    g_shm = sd;

    pr_info("shm_netdev: loaded, chardev=/dev/%s, ring=%lu bytes\n",
            SHM_DEV_NAME, sd->region_size);
    return 0;
}
void shm_net_exit(void)
{
    struct shm_dev *sd = g_shm;

    if (!sd)
        return;

    /* stop rx kthread first */
    kthread_stop(sd->rx_thread);

    misc_deregister(&sd->mdev);

    vfree(sd->region);
    kfree(sd);
    g_shm = NULL;

    pr_info("shm_netdev: unloaded\n");
}

/* shm_net_init / shm_net_exit are called from netif_knl_init / netif_knl_exit */
