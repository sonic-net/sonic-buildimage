/*
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
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
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

/*
 * Middle-driver for communication between Linux KNET driver and
 * drivers support Generic Netlink channel.
 *
 * This code is used to integrate packet from KNET Rx filter
 * call-back function to the genl_packet driver from Google
 * for sending packets to userspace applications using Generic Netlink
 * interfaces.
 *
 * This driver is also built with the DCB library as the helper for parsing
 * the RX packet meta data from the Linux KNET driver filter call-back function.
 * The environment DCBDIR must be set to indicate the directroy of the DCB
 * library.
 */
#ifdef BUILD_GENL_PACKET
#include <gmodule.h> /* Must be included first */
#include <kcom.h>
#include <bcm-knet.h>
#include <linux/if_vlan.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <linux/namei.h>
#include <linux/time.h>
#ifndef LINUX_HAS_MONOTONIC_TIME
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)
#include <linux/timekeeping.h>
#define LINUX_HAS_MONOTONIC_TIME
#endif
#endif
#include <net/net_namespace.h>
#include <net/genl-packet.h>
#include <uapi/linux/genl-packet.h>
#include "bcm-genl-packet.h"
#include "bcm-genl-dev.h"
#include "bcm-genl-netif.h"

#define GENL_CB_DBG
#ifdef GENL_CB_DBG
static int debug;

#define DBG_LVL_PRINT   0x1
#define DBG_LVL_PDMP    0x2
#define GENL_CB_DBG_PRINT(...) \
    if (debug & DBG_LVL_PRINT) {         \
        gprintk(__VA_ARGS__);  \
    }
#else
#define GENL_CB_DBG_PRINT(...)
#endif

/* last should be static or global */
#ifdef LINUX_HAS_MONOTONIC_TIME
#define genl_limited_gprintk(last, ...) { \
  struct timespec64 tv; \
  ktime_get_ts64(&tv); \
  if (tv.tv_sec != last) { \
    gprintk(__VA_ARGS__); \
    last = tv.tv_sec; \
  } \
}
#else
#define genl_limited_gprintk(last, ...) { \
  struct timeval tv; \
  do_gettimeofday(&tv); \
  if (tv.tv_sec != last) { \
    gprintk(__VA_ARGS__); \
    last = tv.tv_sec; \
  } \
}
#endif

#define FCS_SZ 4

#define GENL_QLEN_DFLT 1024
static int genl_qlen = GENL_QLEN_DFLT;
LKM_MOD_PARAM(genl_qlen, "i", int, 0);
MODULE_PARM_DESC(genl_qlen,
"generic cb queue length (default 1024 buffers)");

/* driver proc entry root */
static struct proc_dir_entry *genl_proc_root = NULL;
static char genl_procfs_path[80];

/* generic general info */
typedef struct {
    struct net *netns;
} genl_info_t;
static genl_info_t g_genl_info = {0};

/* Maintain sampled pkt statistics */
typedef struct genl_stats_s {
    unsigned long pkts_f_genl_cb;
    unsigned long pkts_f_genl_mod;
    unsigned long pkts_f_handled;
    unsigned long pkts_f_pass_through;
    unsigned long pkts_f_tag_checked;
    unsigned long pkts_f_tag_stripped;
    unsigned long pkts_f_dst_mc;
    unsigned long pkts_f_src_cpu;
    unsigned long pkts_f_dst_cpu;
    unsigned long pkts_c_qlen_cur;
    unsigned long pkts_c_qlen_hi;
    unsigned long pkts_d_qlen_max;
    unsigned long pkts_d_no_mem;
    unsigned long pkts_d_not_ready;
    unsigned long pkts_d_metadata;
    unsigned long pkts_d_meta_srcport;
    unsigned long pkts_d_meta_dstport;
    unsigned long pkts_d_invalid_size;
} genl_stats_t;
static genl_stats_t g_genl_stats = {0};

typedef struct genl_meta_s {
    int src_ifindex;
    int dst_ifindex;
    uint32 user_data;
} genl_meta_t;

typedef struct genl_pkt_s {
    struct list_head list;
    struct net *netns;
    genl_meta_t meta;
    struct sk_buff *skb;
} genl_pkt_t;

typedef struct genl_work_s {
    struct list_head pkt_list;
    struct work_struct wq;
    spinlock_t lock;
} genl_work_t;
static genl_work_t g_genl_work;

static int
genl_meta_srcport_get(int dev_no, void *pkt_meta)
{
    uint32_t p;

    if (bcmgenl_dev_pktmeta_rx_srcport_get(dev_no, pkt_meta, &p) < 0) {
        return -1;
    }

    return p;
}

static int
genl_meta_dstport_get(int dev_no, void *pkt_meta, bool *is_mcast)
{
    bool mcast;
    uint32_t p;

    if (bcmgenl_dev_pktmeta_rx_dstport_get(dev_no, pkt_meta, &mcast, &p) < 0) {
        return -1;
    }
    if (is_mcast) {
        *is_mcast = mcast;
    }

    return p;
}

static int
genl_meta_get(int dev_no, kcom_filter_t *kf, void *pkt_meta,
              genl_meta_t *genl_meta)
{
    bool mcast = false;
    int srcport, dstport;
    int src_ifindex = 0;
    int dst_ifindex = 0;
    bcmgenl_netif_t bcmgenl_netif;

#ifdef GENL_CB_DBG
    if (debug & 0x1) {
        int i=0;
        uint8_t *meta = (uint8_t*)pkt_meta;
        GENL_CB_DBG_PRINT("%s: generic pkt metadata\n", __func__);
        for (i=0; i<64; i+=16) {
            GENL_CB_DBG_PRINT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    meta[i+0], meta[i+1], meta[i+2], meta[i+3], meta[i+4], meta[i+5], meta[i+6], meta[i+7],
                    meta[i+8], meta[i+9], meta[i+10], meta[i+11], meta[i+12], meta[i+13], meta[i+14], meta[i+15]);
        }
    }
#endif

    /* parse pkt metadata for src and dst ports */
    srcport = genl_meta_srcport_get(dev_no, pkt_meta);
    dstport = genl_meta_dstport_get(dev_no, pkt_meta, &mcast);
    if (srcport == -1 || dstport == -1) {
        gprintk("%s: invalid srcport %d or dstport %d\n", __func__, srcport, dstport);
        return -1;
    }

    /* find src port netif (no need to lookup CPU port) */
    if (srcport != 0) {
        if (bcmgenl_netif_get_by_port(srcport, &bcmgenl_netif) == 0) {
            src_ifindex = bcmgenl_netif.dev->ifindex;
        } else {
            src_ifindex = -1;
            g_genl_stats.pkts_d_meta_srcport++;
            GENL_CB_DBG_PRINT("%s: could not find srcport(%d)\n", __func__, srcport);
        }
    } else {
        g_genl_stats.pkts_f_src_cpu++;
    }

    /* set generic dst type for MC pkts */
    if (mcast) {
        g_genl_stats.pkts_f_dst_mc++;
    /* find dst port netif for UC pkts (no need to lookup CPU port) */
    } else if (dstport != 0) {
        if (bcmgenl_netif_get_by_port(dstport, &bcmgenl_netif) == 0) {
            dst_ifindex = bcmgenl_netif.dev->ifindex;
        } else {
            g_genl_stats.pkts_d_meta_dstport++;
            GENL_CB_DBG_PRINT("%s: could not find dstport(%d)\n", __func__, dstport);
        }
    } else if (dstport == 0) {
        dst_ifindex = 0;
        g_genl_stats.pkts_f_dst_cpu++;
    }

    GENL_CB_DBG_PRINT("%s: dstport %d, src_ifindex 0x%x, dst_ifindex 0x%x\n",
                      __func__, dstport, src_ifindex, dst_ifindex);

    genl_meta->src_ifindex = src_ifindex;
    genl_meta->dst_ifindex = dst_ifindex;
    return 0;
}

static void dump_pkt(struct sk_buff *skb)
{
    int idx;
    char str[128];
    uint8_t *data = skb->data;

    for (idx = 0; idx < skb->len; idx++) {
        if ((idx & 0xf) == 0) {
            sprintf(str, "%04x: ", idx);
        }
        if ((idx & 0xf) == 8) {
            sprintf(&str[strlen(str)], "- ");
        }
        sprintf(&str[strlen(str)], "%02x ", data[idx]);
        if ((idx & 0xf) == 0xf) {
            sprintf(&str[strlen(str)], "\n");
            gprintk(str);
        }
    }
    if ((idx & 0xf) != 0) {
        sprintf(&str[strlen(str)], "\n");
        gprintk(str);
    }
}

static void
genl_task(struct work_struct *work)
{
    genl_work_t *genl_work = container_of(work, genl_work_t, wq);
    unsigned long flags;
    struct list_head *list_ptr, *list_next;
    genl_pkt_t *pkt;

    spin_lock_irqsave(&genl_work->lock, flags);
    list_for_each_safe(list_ptr, list_next, &genl_work->pkt_list) {
        /* dequeue pkt from list */
        pkt = list_entry(list_ptr, genl_pkt_t, list);
        list_del(list_ptr);
        g_genl_stats.pkts_c_qlen_cur--;
        spin_unlock_irqrestore(&genl_work->lock, flags);

        /* send to genl */
        if (pkt) {
            GENL_CB_DBG_PRINT("%s: netns 0x%p, src_ifdx 0x%x, dst_ifdx 0x%x\n",
                    __func__, pkt->netns, pkt->meta.src_ifindex,
                    pkt->meta.dst_ifindex);

            if (debug & DBG_LVL_PDMP) {
                dump_pkt(pkt->skb);
            }

            genl_packet_send_packet(pkt->netns,
                                    pkt->skb,
                                    pkt->meta.src_ifindex,
                                    pkt->meta.dst_ifindex,
                                    pkt->meta.user_data);
            g_genl_stats.pkts_f_genl_mod++;

            dev_kfree_skb_any(pkt->skb);
            kfree(pkt);
        }
        spin_lock_irqsave(&genl_work->lock, flags);
    }
    spin_unlock_irqrestore(&genl_work->lock, flags);
}

static int
genl_filter_cb(uint8_t *pkt, int size, int dev_no, void *pkt_meta,
               int chan, kcom_filter_t *kf)
{
    genl_meta_t meta;
    int rv = 0;
    unsigned long flags;
    genl_pkt_t *genl_pkt;
    struct sk_buff *skb;
    bool strip_tag = false;
    static uint32_t last_drop = 0;
    static uint32_t last_alloc = 0;
    static uint32_t last_skb_fail = 0;

    GENL_CB_DBG_PRINT("%s: pkt size %d, kf->dest_id %d, kf->cb_user_data %d\n",
            __func__, size, kf->dest_id, kf->cb_user_data);

    g_genl_stats.pkts_f_genl_cb++;

    /* get generic metadata */
    rv = genl_meta_get(dev_no, kf, pkt_meta, &meta);
    if (rv < 0) {
        gprintk("%s: Could not parse pkt metadata\n", __func__);
        g_genl_stats.pkts_d_metadata++;
        goto GENL_FILTER_CB_PKT_HANDLED;
    }

    meta.user_data = kf->cb_user_data;

    /* Adjust original pkt size to remove 4B FCS */
    if (size < FCS_SZ) {
        g_genl_stats.pkts_d_invalid_size++;
        goto GENL_FILTER_CB_PKT_HANDLED;
    } else {
       size -= FCS_SZ;
    }

    GENL_CB_DBG_PRINT("%s: netns 0x%p, src_ifdx 0x%x, dst_ifdx 0x%x, user_data %d\n",
                      __func__, g_genl_info.netns, meta.src_ifindex, meta.dst_ifindex, meta.user_data);

    if (g_genl_stats.pkts_c_qlen_cur >= genl_qlen) {
        g_genl_stats.pkts_d_qlen_max++;
        genl_limited_gprintk(last_drop,
                             "%s: tail drop due to max qlen %d reached: %lu\n",
                             __func__, genl_qlen, g_genl_stats.pkts_d_qlen_max);
        goto GENL_FILTER_CB_PKT_HANDLED;
    }

    if ((genl_pkt = kmalloc(sizeof(genl_pkt_t), GFP_ATOMIC)) == NULL)
    {
        g_genl_stats.pkts_d_no_mem++;
        genl_limited_gprintk(last_alloc, "%s: failed to alloc generic mem for pkt: %lu\n", __func__, g_genl_stats.pkts_d_no_mem);
        goto GENL_FILTER_CB_PKT_HANDLED;
    }
    memcpy(&genl_pkt->meta, &meta, sizeof(genl_meta_t));
    genl_pkt->netns = g_genl_info.netns;

    if (size >= 16) {
        uint16_t vlan_proto = (uint16_t) ((pkt[12] << 8) | pkt[13]);
        uint16_t vlan = (uint16_t) ((pkt[14] << 8) | pkt[15]);
        strip_tag =  ((vlan_proto == 0x8100) || (vlan_proto == 0x88a8) || (vlan_proto == 0x9100))
                     && (vlan == 0xFFF);
        if (strip_tag) {
            size -= 4;
        }
        g_genl_stats.pkts_f_tag_checked++;
    }

    if ((skb = dev_alloc_skb(size)) == NULL)
    {
        g_genl_stats.pkts_d_no_mem++;
        genl_limited_gprintk(last_skb_fail, "%s: failed to alloc generic mem for pkt skb: %lu\n", __func__, g_genl_stats.pkts_d_no_mem);
        kfree(genl_pkt);
        goto GENL_FILTER_CB_PKT_HANDLED;
    }

    /* setup skb by copying packet content */
    if(strip_tag) {
        memcpy(skb->data, pkt, 12);
        memcpy(skb->data + 12, pkt + 16, size - 12);
        g_genl_stats.pkts_f_tag_stripped++;
    } else {
        memcpy(skb->data, pkt, size);
    }

    skb_put(skb, size);
    skb->len = size;
    genl_pkt->skb = skb;

    spin_lock_irqsave(&g_genl_work.lock, flags);
    list_add_tail(&genl_pkt->list, &g_genl_work.pkt_list);

    g_genl_stats.pkts_c_qlen_cur++;
    if (g_genl_stats.pkts_c_qlen_cur > g_genl_stats.pkts_c_qlen_hi)
    {
        g_genl_stats.pkts_c_qlen_hi = g_genl_stats.pkts_c_qlen_cur;
    }

    schedule_work(&g_genl_work.wq);
    spin_unlock_irqrestore(&g_genl_work.lock, flags);

    /* Set rv to packet handled */
    rv = 1;

GENL_FILTER_CB_PKT_HANDLED:
    if (rv == 1) {
        g_genl_stats.pkts_f_handled++;
        return 1;
    }
    g_genl_stats.pkts_f_pass_through++;
    return 0;
}

/*
 * generic debug Proc Read Entry
 */
static int
genl_proc_debug_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Config\n", GENL_PACKET_NAME);
    seq_printf(m, "  debug:           0x%x\n", debug);
    seq_printf(m, "  netif_count:     %d\n",   bcmgenl_netif_num_get());
    seq_printf(m, "  queue length:    %d\n",   genl_qlen);

    return 0;
}

static int
genl_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, genl_proc_debug_show, NULL);
}

/*
 * generic debug Proc Write Entry
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
genl_proc_debug_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    char debug_str[40];
    char *ptr;

    if (count >= sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }
    debug_str[count] = '\0';

    if ((ptr = strstr(debug_str, "debug=")) != NULL) {
        ptr += 6;
        debug = simple_strtol(ptr, NULL, 0);
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    return count;
}

struct proc_ops genl_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       genl_proc_debug_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      genl_proc_debug_write,
    .proc_release =    single_release,
};

static int
genl_proc_stats_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Stats\n", GENL_PACKET_NAME);
    seq_printf(m, "  pkts filter generic cb         %10lu\n", g_genl_stats.pkts_f_genl_cb);
    seq_printf(m, "  pkts sent to generic module    %10lu\n", g_genl_stats.pkts_f_genl_mod);
    seq_printf(m, "  pkts handled by generic cb     %10lu\n", g_genl_stats.pkts_f_handled);
    seq_printf(m, "  pkts pass through              %10lu\n", g_genl_stats.pkts_f_pass_through);
    seq_printf(m, "  pkts with vlan tag checked     %10lu\n", g_genl_stats.pkts_f_tag_checked);
    seq_printf(m, "  pkts with vlan tag stripped    %10lu\n", g_genl_stats.pkts_f_tag_stripped);
    seq_printf(m, "  pkts with mc destination       %10lu\n", g_genl_stats.pkts_f_dst_mc);
    seq_printf(m, "  pkts with cpu source           %10lu\n", g_genl_stats.pkts_f_src_cpu);
    seq_printf(m, "  pkts with cpu destination      %10lu\n", g_genl_stats.pkts_f_dst_cpu);
    seq_printf(m, "  pkts current queue length      %10lu\n", g_genl_stats.pkts_c_qlen_cur);
    seq_printf(m, "  pkts high queue length         %10lu\n", g_genl_stats.pkts_c_qlen_hi);
    seq_printf(m, "  pkts drop max queue length     %10lu\n", g_genl_stats.pkts_d_qlen_max);
    seq_printf(m, "  pkts drop no memory            %10lu\n", g_genl_stats.pkts_d_no_mem);
    seq_printf(m, "  pkts drop generic not ready    %10lu\n", g_genl_stats.pkts_d_not_ready);
    seq_printf(m, "  pkts drop metadata parse error %10lu\n", g_genl_stats.pkts_d_metadata);
    seq_printf(m, "  pkts with invalid src port     %10lu\n", g_genl_stats.pkts_d_meta_srcport);
    seq_printf(m, "  pkts with invalid dst port     %10lu\n", g_genl_stats.pkts_d_meta_dstport);
    seq_printf(m, "  pkts with invalid orig pkt sz  %10lu\n", g_genl_stats.pkts_d_invalid_size);
    return 0;
}

static int
genl_proc_stats_open(struct inode * inode, struct file * file)
{
    return single_open(file, genl_proc_stats_show, NULL);
}

/*
 * generic stats Proc Write Entry
 *
 *   Syntax:
 *   write any value to clear stats
 */
static ssize_t
genl_proc_stats_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    int qlen_cur = 0;
    unsigned long flags;

    spin_lock_irqsave(&g_genl_work.lock, flags);
    qlen_cur = g_genl_stats.pkts_c_qlen_cur;
    memset(&g_genl_stats, 0, sizeof(genl_stats_t));
    g_genl_stats.pkts_c_qlen_cur = qlen_cur;
    spin_unlock_irqrestore(&g_genl_work.lock, flags);

    return count;
}
struct proc_ops genl_proc_stats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       genl_proc_stats_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      genl_proc_stats_write,
    .proc_release =    single_release,
};

static int
genl_cb_proc_init(char *procfs_path)
{
    struct proc_dir_entry *entry;

    if (procfs_path == NULL || procfs_path[0] == '\0') {
        return 0;
    }

    /* create procfs for generic */
    snprintf(genl_procfs_path, sizeof(genl_procfs_path) - 1,
             "%s/%s", procfs_path, GENL_PACKET_NAME);
    genl_proc_root = proc_mkdir(genl_procfs_path, NULL);

    /* create procfs for generic stats */
    PROC_CREATE(entry, "stats", 0666, genl_proc_root,
                &genl_proc_stats_file_ops);
    if (entry == NULL) {
        return -1;
    }

    /* create procfs for debug log */
    PROC_CREATE(entry, "debug", 0666, genl_proc_root, &genl_proc_debug_file_ops);
    if (entry == NULL) {
        return -1;
    }

    return 0;
}

static int
genl_cb_proc_cleanup(void)
{
    if (genl_proc_root) {
        remove_proc_entry("stats", genl_proc_root);
        remove_proc_entry("debug", genl_proc_root);
        remove_proc_entry(genl_procfs_path, NULL);
        genl_proc_root = NULL;
    }
    return 0;
}

static int
genl_cb_cleanup(void)
{
    genl_pkt_t *pkt;

    cancel_work_sync(&g_genl_work.wq);

    while (!list_empty(&g_genl_work.pkt_list)) {
        pkt = list_entry(g_genl_work.pkt_list.next, genl_pkt_t, list);
        list_del(&pkt->list);
        dev_kfree_skb_any(pkt->skb);
        kfree(pkt);
    }

    return 0;
}

static int
genl_cb_init(void)
{
    /* clear data structs */
    memset(&g_genl_stats, 0, sizeof(genl_stats_t));
    memset(&g_genl_work, 0, sizeof(genl_work_t));

    /* setup generic work queue */
    spin_lock_init(&g_genl_work.lock);
    INIT_LIST_HEAD(&g_genl_work.pkt_list);
    INIT_WORK(&g_genl_work.wq, genl_task);

    /* get net namespace */
    g_genl_info.netns = get_net_ns_by_pid(current->pid);
    if (!g_genl_info.netns) {
        gprintk("%s: Could not get network namespace for pid %d\n",
                __func__, current->pid);
        return -1;
    }
    GENL_CB_DBG_PRINT("%s: current->pid %d, netns 0x%p\n", __func__,
                         current->pid, g_genl_info.netns);

    return 0;
}

int bcmgenl_packet_cleanup(void)
{
    genl_cb_cleanup();
    genl_cb_proc_cleanup();
    bkn_filter_cb_unregister(genl_filter_cb);
    return 0;
}

int
bcmgenl_packet_init(char *procfs_path)
{
    bkn_filter_cb_attr_t fcb_attr;

    memset(&fcb_attr, 0, sizeof(fcb_attr));
    fcb_attr.name = GENL_PACKET_NAME;

    bkn_filter_cb_attr_register(genl_filter_cb, &fcb_attr);
    genl_cb_proc_init(procfs_path);
    return genl_cb_init();
}
#endif
