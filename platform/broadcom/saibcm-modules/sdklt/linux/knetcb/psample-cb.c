/*
 * Copyright 2017-2022 Broadcom
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
 */
/*
 * $Id: psample_cb.c $
 * $Copyright: (c) 2019 Broadcom Corp.
 * All Rights Reserved.$
 */

/*
 * Driver for call-back functions for Linux KNET driver.
 *
 * This code is used to integrate packet sampling KNET callback to 
 * the psample infra for sending sampled pkts to userspace sflow 
 * applications such as Host Sflow (https://github.com/sflow/host-sflow) 
 * using genetlink interfaces.  
 *
 * The module can be built from the standard Linux user mode target
 * directories using the following command (assuming bash), e.g.
 *
 *   cd $SDK/systems/linux/user/<target>
 *   make BUILD_KNET_CB=1
 *
 */

#include <lkm/lkm.h>
#include <ngknet_callback.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#include <net/psample.h>
#include "psample-cb.h"
#include "ngknet_main.h"

#define PSAMPLE_CB_DBG
#ifdef PSAMPLE_CB_DBG 
extern int debug;
#define PSAMPLE_CB_DBG_LVL_VERB (0x1)
#define PSAMPLE_CB_DBG_LVL_PMD  (0x2)
#define PSAMPLE_CB_DBG_PRINT(...) if (debug & PSAMPLE_CB_DBG_LVL_VERB) { printk(__VA_ARGS__); }
#define PSAMPLE_CB_PMD_PRINT(...) if (debug & PSAMPLE_CB_DBG_LVL_PMD)  { printk(__VA_ARGS__); }
#else
#define PSAMPLE_CB_DBG_PRINT(...)
#define PSAMPLE_CB_PMD_PRINT(...)
#endif

#define FCS_SZ 4
#define PSAMPLE_NLA_PADDING 4
#define PSAMPLE_PKT_HANDLED (1)

#define PSAMPLE_RATE_DFLT 1
#define PSAMPLE_SIZE_DFLT 128
static int psample_size = PSAMPLE_SIZE_DFLT;
module_param(psample_size, int, 0);
MODULE_PARM_DESC(psample_size,
"psample pkt size (default 128 bytes)");

#define PSAMPLE_QLEN_DFLT 1024
static int psample_qlen = PSAMPLE_QLEN_DFLT;
module_param(psample_qlen, int, 0);
MODULE_PARM_DESC(psample_qlen,
"psample queue length (default 1024 buffers)");

#if !IS_ENABLED(CONFIG_PSAMPLE)
inline struct 
psample_group *psample_group_get(struct net *net, u32 group_num)
{
    return NULL;
}
#endif

/* driver proc entry root */
static struct proc_dir_entry *psample_proc_root = NULL;
static struct proc_dir_entry *knet_cb_proc_root = NULL;

/* psample general info */
typedef struct {
    struct list_head netif_list;
    int netif_count;
    struct net *netns;
    spinlock_t lock;
    int dcb_type;
} psample_info_t;
static psample_info_t g_psample_info = {0};

/* Maintain sampled pkt statistics */
typedef struct psample_stats_s {
    unsigned long pkts_f_psample_cb;
    unsigned long pkts_f_psample_mod;
    unsigned long pkts_f_handled;
    unsigned long pkts_f_pass_through;
    unsigned long pkts_f_dst_mc;
    unsigned long pkts_c_qlen_cur;
    unsigned long pkts_c_qlen_hi;
    unsigned long pkts_d_qlen_max;
    unsigned long pkts_d_no_mem;
    unsigned long pkts_d_no_group;
    unsigned long pkts_d_sampling_disabled;
    unsigned long pkts_d_not_ready;
    unsigned long pkts_d_metadata;
    unsigned long pkts_d_skb;
    unsigned long pkts_d_skb_cbd;
    unsigned long pkts_d_meta_srcport;
    unsigned long pkts_d_meta_dstport;
    unsigned long pkts_d_invalid_size;
} psample_stats_t;
static psample_stats_t g_psample_stats = {0};

typedef struct psample_meta_s {
    int trunc_size;
    int src_ifindex;
    int dst_ifindex;
    int sample_rate;
} psample_meta_t;

typedef struct psample_pkt_s {
    struct list_head list;
    struct psample_group *group;
    psample_meta_t meta;
    struct sk_buff *skb;
} psample_pkt_t;

typedef struct psample_work_s {
    struct list_head pkt_list;
    struct work_struct wq;
    spinlock_t lock;
} psample_work_t;
static psample_work_t g_psample_work = {0};

static psample_netif_t*
psample_netif_lookup_by_ifindex(int ifindex)
{
    struct list_head *list;
    psample_netif_t *psample_netif = NULL;
    unsigned long flags;

    /* look for port from list of available net_devices */
    spin_lock_irqsave(&g_psample_info.lock, flags);
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        if (psample_netif->dev->ifindex == ifindex) {
            spin_unlock_irqrestore(&g_psample_info.lock, flags);
            return psample_netif;
        }
    }
    spin_unlock_irqrestore(&g_psample_info.lock, flags);
    return (NULL);
}
        
static psample_netif_t*
psample_netif_lookup_by_port(int port)  __attribute__ ((unused));
static psample_netif_t*
psample_netif_lookup_by_port(int port)
{
    struct list_head *list;
    psample_netif_t *psample_netif = NULL;
    unsigned long flags;

    /* look for port from list of available net_devices */
    spin_lock_irqsave(&g_psample_info.lock, flags);
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        if (psample_netif->port == port) {
            spin_unlock_irqrestore(&g_psample_info.lock, flags);
            return psample_netif;
        }
    }
    spin_unlock_irqrestore(&g_psample_info.lock, flags);
    return (NULL);
}

static int
psample_meta_get(struct sk_buff *skb, psample_meta_t *sflow_meta)
{
    int src_ifindex = 0;
    int sample_rate = 1;
    int sample_size = PSAMPLE_SIZE_DFLT;
    psample_netif_t *psample_netif = NULL;
    const struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    ngknet_netif_t *netif = cbd->netif;
    memset(sflow_meta, 0, sizeof(psample_meta_t));    

    /* find src port */
    if ((psample_netif = psample_netif_lookup_by_ifindex(netif->id))) {
        src_ifindex = psample_netif->dev->ifindex;
        sample_rate = psample_netif->sample_rate;
        sample_size = psample_netif->sample_size;
    } else {
        g_psample_stats.pkts_d_meta_srcport++;
        PSAMPLE_CB_DBG_PRINT("%s: could not find psample netif for src dev %s (ifidx %d)\n", 
                             __func__, netif->name, netif->id);
    }

    sflow_meta->src_ifindex = src_ifindex;
    sflow_meta->trunc_size  = sample_size;
    sflow_meta->sample_rate = sample_rate;
    return (0);
}

static void
psample_task(struct work_struct *work)
{
    psample_work_t *psample_work = container_of(work, psample_work_t, wq);
    unsigned long flags;
    struct list_head *list_ptr, *list_next;
    psample_pkt_t *pkt;

    spin_lock_irqsave(&psample_work->lock, flags);
    list_for_each_safe(list_ptr, list_next, &psample_work->pkt_list) {
        /* dequeue pkt from list */
        pkt = list_entry(list_ptr, psample_pkt_t, list);
        list_del(list_ptr);
        g_psample_stats.pkts_c_qlen_cur--;
        spin_unlock_irqrestore(&psample_work->lock, flags);
 
        /* send to psample */
        if (pkt) {
#if ((IS_ENABLED(CONFIG_PSAMPLE) && LINUX_VERSION_CODE >= KERNEL_VERSION(5,13,0)) || \
     (defined PSAMPLE_MD_EXTENDED_ATTR && PSAMPLE_MD_EXTENDED_ATTR))
            struct psample_metadata md = {0};
            md.trunc_size = pkt->meta.trunc_size;
            md.in_ifindex = pkt->meta.src_ifindex;
            md.out_ifindex = pkt->meta.dst_ifindex;
#endif
            PSAMPLE_CB_DBG_PRINT("%s: group 0x%x, trunc_size %d, src_ifdx 0x%x, dst_ifdx 0x%x, sample_rate %d\n",
                    __func__, pkt->group->group_num, 
                    pkt->meta.trunc_size, pkt->meta.src_ifindex, 
                    pkt->meta.dst_ifindex, pkt->meta.sample_rate);

#if ((IS_ENABLED(CONFIG_PSAMPLE) && LINUX_VERSION_CODE >= KERNEL_VERSION(5,13,0)) || \
     (defined PSAMPLE_MD_EXTENDED_ATTR && PSAMPLE_MD_EXTENDED_ATTR))
            psample_sample_packet(pkt->group, 
                                  pkt->skb,
                                  pkt->meta.sample_rate,
                                  &md);
#else
            psample_sample_packet(pkt->group, 
                                  pkt->skb, 
                                  pkt->meta.trunc_size,
                                  pkt->meta.src_ifindex,
                                  pkt->meta.dst_ifindex,
                                  pkt->meta.sample_rate);
#endif
            g_psample_stats.pkts_f_psample_mod++;
 
            dev_kfree_skb_any(pkt->skb);
            kfree(pkt);
        }
        spin_lock_irqsave(&psample_work->lock, flags);
    }
    spin_unlock_irqrestore(&psample_work->lock, flags);
}

struct sk_buff*
psample_rx_cb(struct net_device *dev, struct sk_buff *skb)
{
    struct psample_group *group;
    psample_meta_t meta;   
    int rv = 0, size;
    const struct ngknet_callback_desc *cbd = NULL;
    const struct ngknet_private *netif = NULL;
    const struct ngknet_filter_s *filt = NULL;
    const struct ngknet_filter_s *filt_src = NULL;

    if (!skb) {
        printk("%s: skb is NULL\n", __func__);
        g_psample_stats.pkts_d_skb++;
        return (NULL);
    }
    cbd = NGKNET_SKB_CB(skb);
    netif = netdev_priv(dev);
    filt_src = cbd->filt;
    filt = netif->filt_cb;

    if (!cbd || !netif || !filt_src) {
        printk("%s: cbd(0x%p) or priv(0x%p) or filter src(0x%p) is NULL\n", 
            __func__, cbd, netif, filt_src);
        g_psample_stats.pkts_d_skb_cbd++;
        return (skb);
    }

    /* Enable PMD output in dmesg: "echo "debug=0x2" > /proc/bcm/knet-cb/psample/debug"
     * Use bshell cmd "pmddecode rxpmd ..." to decode pkt metadata 
     */
    if (debug & PSAMPLE_CB_DBG_LVL_PMD) {
        char str[128];
        int i, len = cbd->pmd_len > 128? 128 : cbd->pmd_len;
        PSAMPLE_CB_PMD_PRINT("PMD (%d bytes): %s\n", 
                cbd->pmd_len, skb->dev->name);
        for (i=0; i<len; i+=4) {
            if ((i & 0x1f) == 0) {
                sprintf(str, "%04x: ", i);
            }
            sprintf(&str[strlen(str)], "0x%08x ", *((uint32_t*)(cbd->pmd+i)));
            if ((i & 0x1c) == 0x1c) {
                sprintf(&str[strlen(str)], "\n");
                printk(str);
                continue;
            }
        }
        if ((i & 0x1f) != 0) {
            sprintf(&str[strlen(str)], "\n");
            PSAMPLE_CB_PMD_PRINT(str);
        }
    }

    /* check if this packet is sampled packet (from sample filter) */
    if (!filt || 
        (NGKNET_FILTER_DEST_T_CB != filt->dest_type) ||
        (strncmp(filt->desc, PSAMPLE_CB_NAME, NGKNET_FILTER_DESC_MAX) != 0)) {
        return (skb);
    }

    PSAMPLE_CB_DBG_PRINT("%s: src dev %s, pkt size %d, filt->dest_id %d\n",
            __func__, skb->dev->name, cbd->pkt_len, filt->dest_id);
    g_psample_stats.pkts_f_psample_cb++;

    /* get psample group info. psample genetlink group ID passed in filt->dest_id */
    group = psample_group_get(g_psample_info.netns, filt->dest_id);
    if (!group) {
        printk("%s: Could not find psample genetlink group %d\n", __func__, filt->dest_id);
        g_psample_stats.pkts_d_no_group++;
        goto PSAMPLE_FILTER_CB_PKT_HANDLED;
    }

    /* get psample metadata */
    rv = psample_meta_get(skb, &meta);
    if (rv < 0) {
        printk("%s: Could not parse pkt metadata\n", __func__);
        g_psample_stats.pkts_d_metadata++;
        goto PSAMPLE_FILTER_CB_PKT_HANDLED;
    }

    /* Adjust original pkt size to remove 4B FCS */
    size = cbd->pkt_len;
    if (size < FCS_SZ) {
        g_psample_stats.pkts_d_invalid_size++;
        goto PSAMPLE_FILTER_CB_PKT_HANDLED;
    } else {
       size -= FCS_SZ; 
    }

    /* Account for padding in libnl used by psample */
    if (meta.trunc_size >= size) {
        meta.trunc_size = size - PSAMPLE_NLA_PADDING;
    }

    PSAMPLE_CB_DBG_PRINT("%s: group 0x%x, trunc_size %d, src_ifdx 0x%x, dst_ifdx 0x%x, sample_rate %d\n",
            __func__, group->group_num, meta.trunc_size, meta.src_ifindex, meta.dst_ifindex, meta.sample_rate);

    /* drop if configured sample rate is 0 */
    if (meta.sample_rate > 0) {
        unsigned long flags;
        psample_pkt_t *psample_pkt;
        struct sk_buff *skb_psample;

        if (g_psample_stats.pkts_c_qlen_cur >= psample_qlen) {
            printk("%s: tail drop due to max qlen %d reached\n", __func__, psample_qlen);
            g_psample_stats.pkts_d_qlen_max++;
            goto PSAMPLE_FILTER_CB_PKT_HANDLED;
        }

        if ((psample_pkt = kmalloc(sizeof(psample_pkt_t), GFP_ATOMIC)) == NULL) {
            printk("%s: failed to alloc psample mem for pkt\n", __func__);
            g_psample_stats.pkts_d_no_mem++;
            goto PSAMPLE_FILTER_CB_PKT_HANDLED;
        }
        memcpy(&psample_pkt->meta, &meta, sizeof(psample_meta_t));
        psample_pkt->group = group;

        if ((skb_psample = dev_alloc_skb(meta.trunc_size)) == NULL) {
            printk("%s: failed to alloc psample mem for pkt skb\n", __func__);
            g_psample_stats.pkts_d_no_mem++;
            goto PSAMPLE_FILTER_CB_PKT_HANDLED;
        }

        /* setup skb to point to pkt */
        memcpy(skb_psample->data, skb->data, meta.trunc_size);
        skb_put(skb_psample, meta.trunc_size);
        skb_psample->len = meta.trunc_size;
        psample_pkt->skb = skb_psample;

        spin_lock_irqsave(&g_psample_work.lock, flags);
        list_add_tail(&psample_pkt->list, &g_psample_work.pkt_list); 

        g_psample_stats.pkts_c_qlen_cur++;
        if (g_psample_stats.pkts_c_qlen_cur > g_psample_stats.pkts_c_qlen_hi) {
            g_psample_stats.pkts_c_qlen_hi = g_psample_stats.pkts_c_qlen_cur;
        }

        schedule_work(&g_psample_work.wq);
        spin_unlock_irqrestore(&g_psample_work.lock, flags);
    } else {
        g_psample_stats.pkts_d_sampling_disabled++;
    }

PSAMPLE_FILTER_CB_PKT_HANDLED:
    g_psample_stats.pkts_f_pass_through++;
    return skb;
}

int 
psample_netif_create_cb(struct net_device *dev)
{
    int found;
    struct list_head *list;
    psample_netif_t *psample_netif, *lpsample_netif;
    unsigned long flags;
    struct ngknet_private *netif = NULL;

    if (!dev) {
        printk("%s: net_device is NULL\n", __func__);
        return (-1);
    }
    netif = netdev_priv(dev);

    if ((psample_netif = kmalloc(sizeof(psample_netif_t), GFP_ATOMIC)) == NULL) {
        printk("%s: failed to alloc psample mem for netif '%s'\n", 
                __func__, dev->name);
        return (-1);
    }

    spin_lock_irqsave(&g_psample_info.lock, flags);

    psample_netif->dev = dev;
    psample_netif->id = netif->netif.id;
    /*Application has encoded the port in netif user data 0 & 1 */
    if (netif->netif.type == NGKNET_NETIF_T_PORT)
    {
        psample_netif->port = netif->netif.user_data[0];
        psample_netif->port |= netif->netif.user_data[1] << 8;
    }
    psample_netif->vlan = netif->netif.vlan;
    psample_netif->sample_rate = PSAMPLE_RATE_DFLT;
    psample_netif->sample_size = PSAMPLE_SIZE_DFLT;
    printk("\r\n Type %d vlan %d", netif->netif.type, psample_netif->vlan);
    /* insert netif sorted by ID similar to bkn_knet_netif_create() */
    found = 0;
    list_for_each(list, &g_psample_info.netif_list) {
        lpsample_netif = (psample_netif_t*)list;
        if (netif->netif.id < lpsample_netif->id) {
            found = 1;
            g_psample_info.netif_count++; 
            break;
        }
    }

    if (found) {
        /* Replace previously removed interface */
        list_add_tail(&psample_netif->list, &lpsample_netif->list);
    } else {
        /* No holes - add to end of list */
        list_add_tail(&psample_netif->list, &g_psample_info.netif_list);
    }
   
    spin_unlock_irqrestore(&g_psample_info.lock, flags);

    PSAMPLE_CB_DBG_PRINT("%s: added psample netif '%s'\n", __func__, dev->name);
    return (0);
}

int
psample_netif_destroy_cb(struct net_device *dev)
{
    int found;
    struct list_head *list;
    psample_netif_t *psample_netif;
    unsigned long flags; 
    struct ngknet_private *netif = NULL;

    if (!dev) {
        printk("%s: net_device is NULL\n", __func__);
        return (-1);
    }
    netif = netdev_priv(dev);

    spin_lock_irqsave(&g_psample_info.lock, flags);
    
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        if (netif->netif.id == psample_netif->id) {
            found = 1; 
            list_del(&psample_netif->list);
            PSAMPLE_CB_DBG_PRINT("%s: removing psample netif '%s'\n", __func__, dev->name);
            kfree(psample_netif);
            g_psample_info.netif_count--; 
            break;
        }
    }

    spin_unlock_irqrestore(&g_psample_info.lock, flags);
    
    if (!found) {    
        return (-1);
    }
    return (0);
}

/*
 * psample rate Proc Read Entry
 */
static int
psample_proc_rate_show(struct seq_file *m, void *v)
{
    struct list_head *list;
    psample_netif_t *psample_netif;
    unsigned long flags; 

    spin_lock_irqsave(&g_psample_info.lock, flags);
    
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        seq_printf(m, "  %-14s %d\n", psample_netif->dev->name, psample_netif->sample_rate);
    }

    spin_unlock_irqrestore(&g_psample_info.lock, flags);
 
    return 0;
}

static int
psample_proc_rate_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_rate_show, NULL);
}

/*
 * psample rate Proc Write Entry
 *
 *   Syntax:
 *   <netif>=<pkt sample rate>
 *
 *   Where <netif> is a virtual network interface name.
 *
 *   Examples:
 *   eth4=1000
 */
static ssize_t
psample_proc_rate_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    int found;
    struct list_head *list;
    psample_netif_t *psample_netif;
    char sample_str[40], *ptr, *newline;
    unsigned long flags;


    if (count > sizeof(sample_str)) {
        count = sizeof(sample_str) - 1;
        sample_str[count] = '\0';
    }
    if (copy_from_user(sample_str, buf, count)) {
        return -EFAULT;
    }
    sample_str[count] = 0;
    newline = strchr(sample_str, '\n');
    if (newline) {
        /* Chop off the trailing newline */
        *newline = '\0';
    }

    if ((ptr = strchr(sample_str, '=')) == NULL &&
        (ptr = strchr(sample_str, ':')) == NULL) {
        printk("Error: Pkt sample rate syntax not recognized: '%s'\n", sample_str);
        return count;
    }
    *ptr++ = 0;

    spin_lock_irqsave(&g_psample_info.lock, flags);
   
    found = 0; 
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        if (strcmp(psample_netif->dev->name, sample_str) == 0) {
            psample_netif->sample_rate = simple_strtol(ptr, NULL, 10);
            // TODO MLI@BRCM - check valid sample rate
            found = 1; 
            break;
        }
    }
    
    spin_unlock_irqrestore(&g_psample_info.lock, flags);

    if (!found) {
        printk("Warning: Failed setting psample rate on unknown network interface: '%s'\n", sample_str);
    }
    return count;
}

struct proc_ops psample_proc_rate_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       psample_proc_rate_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =       psample_proc_rate_write,
    .proc_release =    single_release,
};

/*
 * psample size Proc Read Entry
 */
static int
psample_proc_size_show(struct seq_file *m, void *v)
{
    struct list_head *list;
    psample_netif_t *psample_netif;
    unsigned long flags;

    spin_lock_irqsave(&g_psample_info.lock, flags);
    
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        seq_printf(m, "  %-14s %d\n", psample_netif->dev->name, psample_netif->sample_size);
    }

    spin_unlock_irqrestore(&g_psample_info.lock, flags);
    return 0;
}

static int
psample_proc_size_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_size_show, NULL);
}

/*
 * psample size Proc Write Entry
 *
 *   Syntax:
 *   <netif>=<pkt sample size in bytes>
 *
 *   Where <netif> is a virtual network interface name.
 *
 *   Examples:
 *   eth4=128
 */
static ssize_t
psample_proc_size_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    int found;
    struct list_head *list;
    psample_netif_t *psample_netif;
    char sample_str[40], *ptr, *newline;
    unsigned long flags;

    if (count > sizeof(sample_str)) {
        count = sizeof(sample_str) - 1;
        sample_str[count] = '\0';
    }
    if (copy_from_user(sample_str, buf, count)) {
        return -EFAULT;
    }
    sample_str[count] = 0;
    newline = strchr(sample_str, '\n');
    if (newline) {
        /* Chop off the trailing newline */
        *newline = '\0';
    }

    if ((ptr = strchr(sample_str, '=')) == NULL &&
        (ptr = strchr(sample_str, ':')) == NULL) {
        printk("Error: Pkt sample size syntax not recognized: '%s'\n", sample_str);
        return count;
    }
    *ptr++ = 0;

    spin_lock_irqsave(&g_psample_info.lock, flags);
   
    found = 0; 
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        if (strcmp(psample_netif->dev->name, sample_str) == 0) {
            psample_netif->sample_size = simple_strtol(ptr, NULL, 10);
            // TODO MLI@BRCM - check valid sample size
            found = 1;
            break; 
        }
    }
    
    spin_unlock_irqrestore(&g_psample_info.lock, flags);

    if (!found) {
        printk("Warning: Failed setting psample size on unknown network interface: '%s'\n", sample_str);
    }
    return count;
}

struct proc_ops psample_proc_size_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       psample_proc_size_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =       psample_proc_size_write,
    .proc_release =    single_release,
};

/*
 * psample map Proc Read Entry
 */
static int
psample_proc_map_show(struct seq_file *m, void *v)
{
    struct list_head *list;
    psample_netif_t *psample_netif;
    unsigned long flags;

    seq_printf(m, "  Interface      logical port   ifindex\n");
    seq_printf(m, "-------------    ------------   -------\n");
    spin_lock_irqsave(&g_psample_info.lock, flags);
    
    list_for_each(list, &g_psample_info.netif_list) {
        psample_netif = (psample_netif_t*)list;
        seq_printf(m, "  %-14s %-14d %d\n",
                psample_netif->dev->name,
                psample_netif->port,
                psample_netif->dev->ifindex);
    }

    spin_unlock_irqrestore(&g_psample_info.lock, flags);
    return 0;
}

static int
psample_proc_map_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_map_show, NULL);
}

struct proc_ops psample_proc_map_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       psample_proc_map_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =       NULL,
    .proc_release =    single_release,
};

/*
 * psample debug Proc Read Entry
 */
static int
psample_proc_debug_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Config\n", PSAMPLE_CB_NAME);
    seq_printf(m, "  debug:           0x%x\n", debug);
    seq_printf(m, "  dcb_type:        %d\n",   g_psample_info.dcb_type);
    seq_printf(m, "  netif_count:     %d\n",   g_psample_info.netif_count);
    seq_printf(m, "  queue length:    %d\n",   psample_qlen);

    return 0;
}

static int
psample_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_debug_show, NULL);
}

/*
 * psample debug Proc Write Entry
 *
 *   Syntax:
 *   debug=<mask>
 *
 *   Where <mask> corresponds to the debug module parameter.
 *
 *   Examples:
 *   debug=0x1
 */
static ssize_t
psample_proc_debug_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    char debug_str[40];
    char *ptr;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    if ((ptr = strstr(debug_str, "debug=")) != NULL) {
        ptr += 6;
        debug = simple_strtol(ptr, NULL, 0);
    } else {
        printk("Warning: unknown configuration setting\n");
    }

    return count;
}

struct proc_ops psample_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       psample_proc_debug_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =       psample_proc_debug_write,
    .proc_release =    single_release,
};

static int
psample_proc_stats_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Stats\n", PSAMPLE_CB_NAME);
    seq_printf(m, "  DCB type %d\n",                          g_psample_info.dcb_type);
    seq_printf(m, "  pkts filter psample cb         %10lu\n", g_psample_stats.pkts_f_psample_cb);
    seq_printf(m, "  pkts sent to psample module    %10lu\n", g_psample_stats.pkts_f_psample_mod);
    seq_printf(m, "  pkts handled by psample        %10lu\n", g_psample_stats.pkts_f_handled);
    seq_printf(m, "  pkts pass through              %10lu\n", g_psample_stats.pkts_f_pass_through);
    seq_printf(m, "  pkts with mc destination       %10lu\n", g_psample_stats.pkts_f_dst_mc);
    seq_printf(m, "  pkts current queue length      %10lu\n", g_psample_stats.pkts_c_qlen_cur);
    seq_printf(m, "  pkts high queue length         %10lu\n", g_psample_stats.pkts_c_qlen_hi);
    seq_printf(m, "  pkts drop max queue length     %10lu\n", g_psample_stats.pkts_d_qlen_max);
    seq_printf(m, "  pkts drop no memory            %10lu\n", g_psample_stats.pkts_d_no_mem);
    seq_printf(m, "  pkts drop no psample group     %10lu\n", g_psample_stats.pkts_d_no_group);
    seq_printf(m, "  pkts drop sampling disabled    %10lu\n", g_psample_stats.pkts_d_sampling_disabled);
    seq_printf(m, "  pkts drop psample not ready    %10lu\n", g_psample_stats.pkts_d_not_ready);
    seq_printf(m, "  pkts drop metadata parse error %10lu\n", g_psample_stats.pkts_d_metadata);
    seq_printf(m, "  pkts drop skb error            %10lu\n", g_psample_stats.pkts_d_skb);
    seq_printf(m, "  pkts drop skb cbd error        %10lu\n", g_psample_stats.pkts_d_skb_cbd);
    seq_printf(m, "  pkts with invalid src port     %10lu\n", g_psample_stats.pkts_d_meta_srcport);
    seq_printf(m, "  pkts with invalid dst port     %10lu\n", g_psample_stats.pkts_d_meta_dstport);
    seq_printf(m, "  pkts with invalid orig pkt sz  %10lu\n", g_psample_stats.pkts_d_invalid_size);
    return 0;
}

static int
psample_proc_stats_open(struct inode * inode, struct file * file)
{
    return single_open(file, psample_proc_stats_show, NULL);
}

/*
 * psample stats Proc Write Entry
 *
 *   Syntax:
 *   write any value to clear stats 
 */
static ssize_t
psample_proc_stats_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    int qlen_cur = 0;
    unsigned long flags;

    spin_lock_irqsave(&g_psample_work.lock, flags);
    qlen_cur = g_psample_stats.pkts_c_qlen_cur;
    memset(&g_psample_stats, 0, sizeof(psample_stats_t));
    g_psample_stats.pkts_c_qlen_cur = qlen_cur;
    spin_unlock_irqrestore(&g_psample_work.lock, flags);

    return count;
}
struct proc_ops psample_proc_stats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       psample_proc_stats_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      psample_proc_stats_write,
    .proc_release =    single_release,
};

int psample_cleanup(void)
{
    cancel_work_sync(&g_psample_work.wq);
    remove_proc_entry("stats", psample_proc_root);
    remove_proc_entry("rate",  psample_proc_root);
    remove_proc_entry("size",  psample_proc_root);
    remove_proc_entry("debug", psample_proc_root);
    remove_proc_entry("map"  , psample_proc_root);
    remove_proc_entry("psample", knet_cb_proc_root);
    remove_proc_entry("bcm/knet-cb", NULL);
    remove_proc_entry("bcm", NULL);
    return 0;
}

int psample_init(void)
{
    #define PROCFS_MAX_PATH 1024
    char psample_procfs_path[PROCFS_MAX_PATH];
    struct proc_dir_entry *entry;

    /* initialize proc files (for ngknet) */
    proc_mkdir("bcm", NULL);

    /* create procfs for psample */
    snprintf(psample_procfs_path, PROCFS_MAX_PATH, "bcm/knet-cb");
    knet_cb_proc_root = proc_mkdir(psample_procfs_path, NULL);
    snprintf(psample_procfs_path, PROCFS_MAX_PATH, "%s/%s", psample_procfs_path, PSAMPLE_CB_NAME);
    psample_proc_root = proc_mkdir(psample_procfs_path, NULL);

    /* create procfs for psample stats */
    PROC_CREATE(entry, "stats", 0666, psample_proc_root, &psample_proc_stats_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/stats'\n", __func__, psample_procfs_path);
        return -1;
    }

    /* create procfs for setting sample rates */
    PROC_CREATE(entry, "rate", 0666, psample_proc_root, &psample_proc_rate_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/rate'\n", __func__, psample_procfs_path);
        return -1;
    }

    /* create procfs for setting sample size */
    PROC_CREATE(entry, "size", 0666, psample_proc_root, &psample_proc_size_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/size'\n", __func__, psample_procfs_path);
        return -1;
    }
    
    /* create procfs for getting netdev mapping */
    PROC_CREATE(entry, "map", 0666, psample_proc_root, &psample_proc_map_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/map'\n", __func__, psample_procfs_path);
        return -1;
    }

    /* create procfs for debug log */
    PROC_CREATE(entry, "debug", 0666, psample_proc_root, &psample_proc_debug_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/debug'\n", __func__, psample_procfs_path);
        return -1;
    }

    /* clear data structs */
    memset(&g_psample_stats, 0, sizeof(psample_stats_t));
    memset(&g_psample_info, 0, sizeof(psample_info_t));
    memset(&g_psample_work, 0, sizeof(psample_work_t));

    /* FIXME: How to get DCB type from NGKNET? */
    //g_psample_info.dcb_type

    /* setup psample_info struct */
    INIT_LIST_HEAD(&g_psample_info.netif_list); 
    spin_lock_init(&g_psample_info.lock);

    /* setup psample work queue */
    spin_lock_init(&g_psample_work.lock); 
    INIT_LIST_HEAD(&g_psample_work.pkt_list); 
    INIT_WORK(&g_psample_work.wq, psample_task);

    /* get net namespace */ 
    g_psample_info.netns = get_net_ns_by_pid(current->pid);
    if (!g_psample_info.netns) {
        printk("%s: Could not get network namespace for pid %d\n", __func__, current->pid);
        return (-1);
    }
    PSAMPLE_CB_DBG_PRINT("%s: current->pid %d, netns 0x%p, sample_size %d\n", __func__, 
            current->pid, g_psample_info.netns, psample_size);


    return 0;
}
