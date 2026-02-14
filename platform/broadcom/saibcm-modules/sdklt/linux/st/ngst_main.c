/*! \file ngst_main.c
 *
 * Streaming Telemetry support module entry.
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

#include <lkm/lkm.h>
#include <lkm/ngbde_kapi.h>
#include <lkm/ngst_ioctl.h>
#include <lkm/ngst_netlink.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <net/netlink.h>
#include <net/sock.h>
#include <net/genetlink.h>

/*! \cond */
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Streaming Telemetry Support Module");
MODULE_LICENSE("GPL");
/*! \endcond */

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

static st_dev_t stdev;

static struct task_struct *ngst_kthread;

static const struct genl_multicast_group ngst_genl_mcgrps[] = {
    { .name = NGST_GENL_MCGRP_NAME },
};

static struct genl_family ngst_genl_family = {
    .name = NGST_GENL_FAMILY_NAME,
    .version = NGST_GENL_VERSION,
    .module = THIS_MODULE,
    .mcgrps = ngst_genl_mcgrps,
    .n_mcgrps = ARRAY_SIZE(ngst_genl_mcgrps),
};

/*! Send netlink message to user-space. */
static inline void
ngst_nl_msg_send(int cmd, const char *payload, uint32_t payload_size)
{
    void *gnlh;
    struct sk_buff *skb_out;

    skb_out = genlmsg_new(payload_size, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_INFO "Failed genlmsg_new()\n");
        return;
    }
    gnlh = genlmsg_put(skb_out, 0, 0, &ngst_genl_family, 0, cmd);
    memcpy(gnlh, payload, payload_size);

    genlmsg_multicast(&ngst_genl_family, skb_out, 0, 0, GFP_KERNEL);
    return;
}


static int
ngst_send_msgs_to_user(void *data)
{
    void *cur_dma_vaddr = NULL;

    while (!kthread_should_stop()) {
        if (!stdev.dma_vaddr) {
            msleep(10);
            continue;
        }
        stdev.buff_wr_ptr = *((uint32_t *)(stdev.dma_vaddr));

        if (stdev.buff_wr_ptr != stdev.buff_rd_ptr) {
            cur_dma_vaddr = stdev.dma_buff_addr_va +
                            (stdev.buff_rd_ptr * stdev.buff_chunk_size);
            ngst_nl_msg_send(NGST_CMD_DATA_RSP, cur_dma_vaddr, stdev.buff_chunk_size);
            if (++stdev.buff_rd_ptr == stdev.buff_chunk_cnt) {
                stdev.buff_rd_ptr = 0;
            }
            memset(cur_dma_vaddr, 0, stdev.buff_chunk_size);
            continue;
        }
        usleep_range(NGST_IDLE_USLEEP_MIN, NGST_IDLE_USLEEP_MAX);
    }
    return 0;
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

    switch (cmd) {
        case NGST_IOC_DMA_INFO:
            if (copy_from_user(&ioc,
                               (struct ngst_ioc_dma_info_s __user *)arg,
                               sizeof(ioc)))
                return -EFAULT;

            if (ioc.chunk_cnt == 0 || ioc.size == 0) {
                return 0;
            }
            stdev.dma_dev = ngbde_kapi_dma_dev_get(ioc.unit);
            if (!stdev.dma_dev) {
                printk(KERN_INFO "Not Found ST dev %d\n", ioc.unit);
                return -EFAULT;
            }

            if (!stdev.dma_vaddr) {
                stdev.buff_wr_ptr = 0;
                stdev.buff_rd_ptr = 0;
                /* Including write pointer size */
                stdev.dma_size = ioc.size + sizeof(uint32_t);
                stdev.dma_vaddr = dma_alloc_coherent(stdev.dma_dev,
                                                      stdev.dma_size,
                                                      &stdev.dma_handle,
                                                      GFP_KERNEL);
                if (!stdev.dma_vaddr) {
                    printk(KERN_ERR "Error allocating DMA buffer\n");
                    return -ENOMEM;
                } else {
                    printk(KERN_INFO "DMA buffer allocated successfully\n");
                }
                memset(stdev.dma_vaddr, 0, stdev.dma_size);
                stdev.dma_buff_addr_va = stdev.dma_vaddr + sizeof(uint32_t);

                stdev.buff_chunk_cnt = ioc.chunk_cnt;
                stdev.buff_chunk_size = ioc.size / ioc.chunk_cnt;
                ngst_genl_family.hdrsize = stdev.buff_chunk_size;
            } else {
                if ((stdev.buff_chunk_cnt != ioc.chunk_cnt) ||
                    (stdev.buff_chunk_size != ioc.size / ioc.chunk_cnt)) {
                    printk(KERN_ERR "DMA buffer is already allocated\n");
                    return -EFAULT;
                }
            }

            ioc.paddr = (uint64_t)stdev.dma_handle;
            if (copy_to_user((struct ngst_ioc_dma_info_s __user *)arg,
                             &ioc, sizeof(ioc)))
                return -EFAULT;
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
    if (ngst_kthread) {
        kthread_stop(ngst_kthread);
        ngst_kthread = NULL;
    }

    unregister_chrdev(NGST_MODULE_MAJOR, NGST_MODULE_NAME);

    genl_unregister_family(&ngst_genl_family);

    if (stdev.dma_vaddr) {
        dma_free_coherent(stdev.dma_dev, stdev.dma_size,
                          stdev.dma_vaddr, stdev.dma_handle);
    }
    printk(KERN_INFO "Broadcom NGST unloaded successfully\n");
}

static int __init
ngst_init_module(void)
{
    int rv;

    rv = register_chrdev(NGST_MODULE_MAJOR, NGST_MODULE_NAME, &ngst_fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               NGST_MODULE_NAME, NGST_MODULE_MAJOR);
        return rv;
    }

    rv = genl_register_family(&ngst_genl_family);
    if (rv) {
        printk(KERN_WARNING "%s: Unable to create netlink socket\n",
               NGST_MODULE_NAME);
        return -EFAULT;
    }

    ngst_kthread = kthread_run(ngst_send_msgs_to_user, NULL, "ngst_send_msgs_to_user");
    if (IS_ERR(ngst_kthread)) {
        return PTR_ERR(ngst_kthread);
    }

    printk(KERN_INFO "Broadcom NGST loaded successfully\n");
    return 0;
}

module_exit(ngst_exit_module);
module_init(ngst_init_module);
