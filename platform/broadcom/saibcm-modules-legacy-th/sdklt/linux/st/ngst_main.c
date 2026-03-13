/*! \file ngst_main.c
 *
 * Streaming Telemetry support module entry.
 *
 */
/*
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

#include <lkm/lkm.h>
#include <lkm/ngbde_kapi.h>
#include <lkm/ngst_ioctl.h>
#include <lkm/ngst_netlink.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/netlink.h>
#include <net/sock.h>
#include <net/net_namespace.h>

/*! \cond */
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Streaming Telemetry Support Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! Maximum number of switch devices supported. */
#ifndef NGST_NUM_SWDEV_MAX
#define NGST_NUM_SWDEV_MAX     NGBDE_NUM_SWDEV_MAX
#endif

/*! Switch device descriptor. */
typedef struct st_dev_s {

    /*! Logical address of DMA pool. */
    void *dma_vaddr;

    /*! Logical address of buffer pool. */
    void *dma_buff_addr_va;

    /*! Physical address of DMA pool. */
    dma_addr_t dma_handle;

    /*! Size of DMA memory (in bytes). */
    size_t dma_size;

    /*! Buffer chunk size (in bytes). */
    uint32_t buff_chunk_size;

    /*! Buffer chunk count. */
    uint32_t buff_chunk_cnt;

    /*! Buffer read pointer. */
    uint32_t buff_rd_ptr;

    /*! Buffer write pointer. */
    uint32_t buff_wr_ptr;

    /*! Linux DMA device associated with DMA pool. */
    struct device *dma_dev;

} st_dev_t;

static st_dev_t stdevs[NGST_NUM_SWDEV_MAX];

/*! Netlink socket. */
static struct sock *nl_sk;

/*! Send netlink message to user-space. */
static void
ngst_nl_msg_send(int unit, int pid, int msg_type, const char *payload)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    int res;
    int msg_size;
    uint32_t payload_size;
    st_dev_t *stdev;

    struct ngst_nl_msg_hdr_s rsp = {
        .unit = unit,
        .msg_type = msg_type,
    };

    stdev = &stdevs[unit];
    switch (msg_type) {
        case NGST_NL_MSG_TYPE_ST_DATA_NOT_READY:
            msg_size = sizeof(struct ngst_nl_msg_hdr_s);
            skb_out = nlmsg_new(msg_size, 0);
            if (!skb_out) {
                printk(KERN_ERR "Failed to allocate new skb for reply\n");
                return;
            }

            nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
            NETLINK_CB(skb_out).dst_group = 0;
            memcpy(nlmsg_data(nlh), &rsp, msg_size);

            break;
        case NGST_NL_MSG_TYPE_ST_DATA_RSP:
            payload_size = stdev->buff_chunk_size;
            msg_size = sizeof(struct ngst_nl_msg_hdr_s) + payload_size;
            skb_out = nlmsg_new(msg_size, 0);
            if (!skb_out) {
                printk(KERN_ERR "Failed to allocate new skb for reply\n");
                return;
            }

            nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
            NETLINK_CB(skb_out).dst_group = 0;
            memcpy(nlmsg_data(nlh), &rsp, sizeof(struct ngst_nl_msg_hdr_s));
            memcpy(nlmsg_data(nlh) + sizeof(struct ngst_nl_msg_hdr_s),
                   payload, payload_size);
            break;
        default:
            return;
    }

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0) {
        printk(KERN_INFO "Error while sending back to user: %d\n", res);
    }

    return;
}


/*! Receive netlink message from user-space. */
static void
ngst_nl_msg_recv(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct ngst_nl_msg_hdr_s *rcv_nlmsg;
    struct ngst_nl_msg_hdr_s *st_data_req_msg;
    void *cur_dma_vaddr = NULL;
    int user_pid, unit;
    st_dev_t *stdev;

    nlh = (struct nlmsghdr *)skb->data;
    user_pid = nlh->nlmsg_pid;
    rcv_nlmsg = (struct ngst_nl_msg_hdr_s *)nlmsg_data(nlh);

    unit = rcv_nlmsg->unit;
    if (unit < 0 || unit > NGST_NUM_SWDEV_MAX) {
        return;
    }
    stdev = &stdevs[rcv_nlmsg->unit];

    if (rcv_nlmsg->msg_type == NGST_NL_MSG_TYPE_ST_DATA_REQ) {
        if (!stdev->dma_vaddr) {
            ngst_nl_msg_send(unit, user_pid,
                             NGST_NL_MSG_TYPE_ST_DATA_NOT_READY, NULL);
            return;
        }
        stdev->buff_wr_ptr = *((uint32_t *)(stdev->dma_vaddr));
        st_data_req_msg = (struct ngst_nl_msg_hdr_s *)nlmsg_data(nlh);

        stdev->buff_rd_ptr =
            stdev->buff_rd_ptr == stdev->buff_chunk_cnt ? 0 : stdev->buff_rd_ptr;
        if (stdev->buff_wr_ptr != stdev->buff_rd_ptr) {
            cur_dma_vaddr = stdev->dma_buff_addr_va +
                            (stdev->buff_rd_ptr * stdev->buff_chunk_size);
            ngst_nl_msg_send(unit, user_pid,
                             NGST_NL_MSG_TYPE_ST_DATA_RSP, cur_dma_vaddr);
            stdev->buff_rd_ptr = stdev->buff_wr_ptr;
        } else {
            ngst_nl_msg_send(unit, user_pid,
                             NGST_NL_MSG_TYPE_ST_DATA_NOT_READY, NULL);
        }
    }

    return;
}

/*!
 * Generic module functions
 */

static int
ngst_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int
ngst_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long
ngst_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ngst_ioc_dma_info_s ioc;
    st_dev_t *stdev;

    switch (cmd) {
        case NGST_IOC_DMA_INFO:
            if (copy_from_user(&ioc,
                               (struct ngst_ioc_dma_info_s __user *)arg,
                               sizeof(ioc)))
                return ~EFAULT;

            if (ioc.chunk_cnt == 0 || ioc.size == 0) {
                return 0;
            }
            stdev = &stdevs[ioc.unit];
            stdev->dma_dev = ngbde_kapi_dma_dev_get(ioc.unit);
            if (!stdev->dma_dev) {
                printk(KERN_INFO "Not Found ST dev %d\n", ioc.unit);
                return ~EFAULT;
            }

            if (!stdev->dma_vaddr) {
                /* Including write pointer size */
                stdev->dma_size = ioc.size + sizeof(uint32_t);
                stdev->dma_vaddr = dma_alloc_coherent(stdev->dma_dev,
                                                      stdev->dma_size,
                                                      &stdev->dma_handle,
                                                      GFP_KERNEL);
                if (!stdev->dma_vaddr) {
                    printk(KERN_ERR "Error allocating DMA buffer\n");
                    return ~ENOMEM;
                } else {
                    printk(KERN_INFO "DMA buffer allocated successfully\n");
                }
                memset((void *)stdev->dma_vaddr, 0, stdev->dma_size);
                stdev->dma_buff_addr_va = stdev->dma_vaddr + sizeof(uint32_t);

                stdev->buff_chunk_cnt = ioc.chunk_cnt;
                stdev->buff_chunk_size = ioc.size / ioc.chunk_cnt;
            } else {
                if ((stdev->buff_chunk_cnt != ioc.chunk_cnt) ||
                    (stdev->buff_chunk_size != ioc.size / ioc.chunk_cnt)) {
                    printk(KERN_ERR "DMA buffer is already allocated.n");
                    return ~EFAULT;
                }
            }

            ioc.paddr = (uint64_t)stdev->dma_handle;
            if (copy_to_user((struct ngst_ioc_dma_info_s __user *)arg,
                             &ioc, sizeof(ioc)))
                return ~EFAULT;
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations ngst_fops = {
    .open = ngst_open,
    .release = ngst_release,
    .unlocked_ioctl = ngst_ioctl,
    .compat_ioctl = ngst_ioctl,
};

static void __exit
ngst_exit_module(void)
{
    int unit;
    st_dev_t *stdev;

    unregister_chrdev(NGST_MODULE_MAJOR, NGST_MODULE_NAME);

    if (nl_sk) {
        netlink_kernel_release(nl_sk);
    }

    for (unit = 0; unit < NGST_NUM_SWDEV_MAX; unit++) {
        stdev = &stdevs[unit];

        if (stdev->dma_vaddr) {
            dma_free_coherent(stdev->dma_dev, stdev->dma_size,
                              stdev->dma_vaddr, stdev->dma_handle);
        }
    }
    printk(KERN_INFO "Broadcom NGST unloaded successfully.\n");
}

static int __init
ngst_init_module(void)
{
    int rv;
    struct netlink_kernel_cfg cfg = {
        .input = ngst_nl_msg_recv,
    };

    rv = register_chrdev(NGST_MODULE_MAJOR, NGST_MODULE_NAME, &ngst_fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               NGST_MODULE_NAME, NGST_MODULE_MAJOR);
        return rv;
    }

    nl_sk = netlink_kernel_create(&init_net, NGST_NETLINK_PROTOCOL, &cfg);
    if (!nl_sk) {
        printk(KERN_WARNING "%s: Unable to create netlink socket\n",
               NGST_MODULE_NAME);
        return -EFAULT;
    }
    printk(KERN_INFO "Broadcom NGST loaded successfully\n");
    return 0;
}

module_exit(ngst_exit_module);
module_init(ngst_init_module);
