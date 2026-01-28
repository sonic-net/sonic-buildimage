/*! \file bcmgenl_packet.c
 *
 * BCMGENL packet callback module.
 *
 */
/*
 * Copyright 2018-2024 Broadcom. All rights reserved.
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
#include <lkm/ngknet_kapi.h>
#include <ngknet_linux.h>

#include <linux/if_vlan.h>
#include <linux/namei.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#if 0
/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("BCMGENL Module");
MODULE_LICENSE("GPL");
/*! \endcond */

#include <bcmgenl.h>
#include <bcmgenl_packet.h>
#include <net/genl-packet.h>

#define BCMGENL_PACKET_NAME GENL_PACKET_NAME

#ifdef GENL_DEBUG
static int debug;
#endif /* GENL_DEBUG */

#define BCMGENL_PACKET_QLEN_DFLT 1024
static int bcmgenl_packet_qlen = BCMGENL_PACKET_QLEN_DFLT;
MODULE_PARAM(bcmgenl_packet_qlen, int, 0);
MODULE_PARM_DESC(bcmgenl_packet_qlen, "generic cb queue length (default 1024 buffers)");

#define FCS_SZ 4

static bcmgenl_info_t g_bcmgenl_packet_info = {{0}};

/* Maintain sampled pkt statistics */
typedef struct bcmgenl_packet_stats_s {
    unsigned long pkts_f_packet_cb;
    unsigned long pkts_f_packet_mod;
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
    unsigned long pkts_d_skb;
    unsigned long pkts_d_skb_cbd;
    unsigned long pkts_d_meta_srcport;
    unsigned long pkts_d_meta_dstport;
    unsigned long pkts_d_invalid_size;
} bcmgenl_packet_stats_t;
static bcmgenl_packet_stats_t g_bcmgenl_packet_stats = {0};

typedef struct genl_packet_meta_s {
    int in_ifindex;
    int out_ifindex;
    unsigned int context;
} genl_packet_meta_t;

typedef struct genl_pkt_s {
    struct list_head list;
    struct net *netns;
    genl_packet_meta_t meta;
    struct sk_buff *skb;
} genl_pkt_t;

typedef struct bcmgenl_packet_work_s {
    struct list_head pkt_list;
    struct work_struct wq;
    spinlock_t lock;
} bcmgenl_packet_work_t;
static bcmgenl_packet_work_t g_bcmgenl_packet_work = {{0}};

/* driver proc entry root */
static struct proc_dir_entry *bcmgenl_packet_proc_root = NULL;

static bcmgenl_netif_t *
bcmgenl_packet_netif_lookup_by_ifindex(int ifindex)  __attribute__ ((unused));
static bcmgenl_netif_t *
bcmgenl_packet_netif_lookup_by_ifindex(int ifindex)
{
    struct list_head *list;
    bcmgenl_netif_t *bcmgenl_netif = NULL;
    unsigned long flags;

    /* look for ifindex from list of available net_devices */
    spin_lock_irqsave(&g_bcmgenl_packet_info.lock, flags);
    list_for_each(list, &g_bcmgenl_packet_info.netif_list) {
        bcmgenl_netif = (bcmgenl_netif_t*)list;
        if (bcmgenl_netif->dev->ifindex == ifindex) {
            spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);
            return bcmgenl_netif;
        }
    }
    spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);
    return (NULL);
}

static bcmgenl_netif_t *
bcmgenl_packet_netif_lookup_by_port(int port)
{
    struct list_head *list;
    bcmgenl_netif_t *bcmgenl_netif = NULL;
    unsigned long flags;

    /* look for port from list of available net_devices */
    spin_lock_irqsave(&g_bcmgenl_packet_info.lock, flags);
    list_for_each(list, &g_bcmgenl_packet_info.netif_list) {
        bcmgenl_netif = (bcmgenl_netif_t*)list;
        if (bcmgenl_netif->port == port) {
            spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);
            return bcmgenl_netif;
        }
    }
    spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);
    return (NULL);
}

static int
bcmgenl_packet_generic_meta_get(bcmgenl_pkt_t *bcmgenl_pkt, genl_packet_meta_t *genl_packet_meta)
{
    int srcport, dstport, dstport_type;
    int src_ifindex = 0, dst_ifindex = 0;
    bcmgenl_netif_t *bcmgenl_netif = NULL;

    if (!bcmgenl_pkt || !genl_packet_meta) {
        GENL_DBG_WARN("%s: bcmgenl_pkt or genl_packet_meta is NULL\n", __func__);
        return (-1);
    }

    /* get src and dst ports */
    srcport = bcmgenl_pkt->meta.src_port;
    dstport = bcmgenl_pkt->meta.dst_port;
    dstport_type = bcmgenl_pkt->meta.dst_port_type;
    if ((srcport == -1) || (dstport == -1)) {
        GENL_DBG_WARN("%s: invalid srcport %d or dstport %d\n", __func__, srcport, dstport);
        return (-1);
    }

    /* find src port netif (no need to lookup CPU port) */
    if (srcport != 0) {
        if ((bcmgenl_netif = bcmgenl_packet_netif_lookup_by_port(srcport))) {
            src_ifindex = bcmgenl_netif->dev->ifindex;
        } else {
            src_ifindex = -1;
            g_bcmgenl_packet_stats.pkts_d_meta_srcport++;
            GENL_DBG_VERB("%s: could not find srcport(%d)\n", __func__, srcport);
        }
    } else {
        g_bcmgenl_packet_stats.pkts_f_src_cpu++;
    }

    /* set generic dst type for MC pkts */
    if (dstport_type == DSTPORT_TYPE_MC) {
        g_bcmgenl_packet_stats.pkts_f_dst_mc++;
    } else if (dstport != 0) {
        /* find dst port netif for UC pkts (no need to lookup CPU port) */
        if ((bcmgenl_netif = bcmgenl_packet_netif_lookup_by_port(dstport))) {
            dst_ifindex = bcmgenl_netif->dev->ifindex;
        } else {
            dst_ifindex = -1;
            g_bcmgenl_packet_stats.pkts_d_meta_dstport++;
            GENL_DBG_VERB("%s: could not find dstport(%d)\n", __func__, dstport);
        }
    } else if (dstport == 0) {
        g_bcmgenl_packet_stats.pkts_f_dst_cpu++;
    }

    GENL_DBG_VERB
        ("%s: srcport %d, dstport %d, src_ifindex %d, dst_ifindex %d\n",
         __func__, srcport, dstport, src_ifindex, dst_ifindex);

    memset(genl_packet_meta, 0, sizeof(genl_packet_meta_t));
    genl_packet_meta->in_ifindex = src_ifindex;
    genl_packet_meta->out_ifindex = dst_ifindex;
    return (0);
}

static struct sk_buff *
bcmgenl_packet_filter_cb(struct sk_buff *skb, ngknet_filter_t **filt)
{
    int rv = 0, dev_no, pkt_len;
    const struct ngknet_callback_desc *cbd = NULL;
    ngknet_filter_t *match_filt = NULL;
    unsigned long flags;
    bcmgenl_pkt_t bcmgenl_pkt;
    genl_pkt_t *generic_pkt = NULL;
    bool strip_tag = false;
    struct sk_buff *skb_generic_pkt;
    static uint32_t last_drop, last_alloc, last_skb;
    uint8_t *pkt;

    if (!skb) {
        GENL_DBG_WARN("%s: skb is NULL\n", __func__);
        g_bcmgenl_packet_stats.pkts_d_skb++;
        return (NULL);
    }
    cbd = NGKNET_SKB_CB(skb);
    if (cbd) {
        match_filt = cbd->filt;
    }

    if (!cbd || !match_filt) {
        GENL_DBG_WARN("%s: cbd(0x%p) or match_filt(0x%p) is NULL\n",
                      __func__, cbd, match_filt);
        g_bcmgenl_packet_stats.pkts_d_skb_cbd++;
        return (skb);
    }

    /* check if this packet is from the same filter */
    if (match_filt->dest_type != NGKNET_FILTER_DEST_T_CB ||
        strncmp(match_filt->desc, BCMGENL_PACKET_NAME, NGKNET_FILTER_DESC_MAX) != 0) {
        return (skb);
    }
    dev_no = cbd->dinfo->dev_no;
    pkt = cbd->pmd + cbd->pmd_len;
    pkt_len = cbd->pkt_len;

    GENL_DBG_VERB
        ("pkt size %d, match_filt->dest_id %d\n",
         pkt_len, match_filt->dest_id);
    GENL_DBG_VERB
        ("filter user data: 0x%08x\n", *(uint32_t *)match_filt->user_data);
    GENL_DBG_VERB
        ("filter_cb for dev %d: %s\n", dev_no, cbd->dinfo->type_str);
    g_bcmgenl_packet_stats.pkts_f_packet_cb++;

    /* Adjust original pkt_len to remove 4B FCS */
    if (pkt_len < FCS_SZ) {
        g_bcmgenl_packet_stats.pkts_d_invalid_size++;
        goto FILTER_CB_PKT_HANDLED;
    } else {
        pkt_len -= FCS_SZ;
    }

    if (g_bcmgenl_packet_stats.pkts_c_qlen_cur >= bcmgenl_packet_qlen) {
        g_bcmgenl_packet_stats.pkts_d_qlen_max++;
        last_drop = 0;
        bcmgenl_limited_gprintk
            (last_drop, "%s: tail drop due to max qlen %d reached: %lu\n",
             __func__, bcmgenl_packet_qlen,
             g_bcmgenl_packet_stats.pkts_d_qlen_max);
        goto FILTER_CB_PKT_HANDLED;
    }

    if ((generic_pkt = kmalloc(sizeof(genl_pkt_t), GFP_ATOMIC)) == NULL) {
        g_bcmgenl_packet_stats.pkts_d_no_mem++;
        last_alloc = 0;
        bcmgenl_limited_gprintk
            (last_alloc, "%s: failed to alloc generic mem for pkt: %lu\n",
             __func__, g_bcmgenl_packet_stats.pkts_d_no_mem);
        goto FILTER_CB_PKT_HANDLED;
    }
    /* get packet metadata */
    rv = bcmgenl_pkt_package(dev_no, skb,
                             &g_bcmgenl_packet_info,
                             &bcmgenl_pkt);
    if (rv < 0) {
        GENL_DBG_WARN("%s: Could not parse pkt metadata\n", __func__);
        g_bcmgenl_packet_stats.pkts_d_metadata++;
        goto FILTER_CB_PKT_HANDLED;
    }

    GENL_DBG_VERB
        ("%s: netns 0x%p, src_port %d, dst_port %d, dst_port_type %x\n",
         __func__,
         bcmgenl_pkt.netns,
         bcmgenl_pkt.meta.src_port,
         bcmgenl_pkt.meta.dst_port,
         bcmgenl_pkt.meta.dst_port_type);

    /* generic_pkt start */
    generic_pkt->netns = bcmgenl_pkt.netns;

    /* get generic_pkt generic metadata */
    rv = bcmgenl_packet_generic_meta_get(&bcmgenl_pkt, &generic_pkt->meta);
    if (rv < 0) {
        GENL_DBG_WARN("%s: Could not parse pkt metadata\n", __func__);
        g_bcmgenl_packet_stats.pkts_d_metadata++;
        goto FILTER_CB_PKT_HANDLED;
    }
    generic_pkt->meta.context = *(uint32_t *)cbd->filt->user_data;

    if (pkt_len >= 16) {
        uint16_t proto = bcmgenl_pkt.meta.proto;
        uint16_t vlan = bcmgenl_pkt.meta.vlan;
        strip_tag = (vlan == 0xFFF) &&
                    ((proto == 0x8100) || (proto == 0x88a8) ||
                     (proto == 0x9100));
        if (strip_tag) {
            pkt_len -= 4;
        }
        g_bcmgenl_packet_stats.pkts_f_tag_checked++;
    }

    if ((skb_generic_pkt = dev_alloc_skb(pkt_len)) == NULL)
    {
        g_bcmgenl_packet_stats.pkts_d_no_mem++;
        last_skb = 0;
        bcmgenl_limited_gprintk
            (last_skb, "%s: failed to alloc generic mem for pkt skb: %lu\n",
             __func__, g_bcmgenl_packet_stats.pkts_d_no_mem);
        goto FILTER_CB_PKT_HANDLED;
    }

    /* setup skb by copying packet content */
    if (strip_tag) {
        memcpy(skb_generic_pkt->data, pkt, 12);
        memcpy(skb_generic_pkt->data + 12, pkt + 16, pkt_len - 12);
        g_bcmgenl_packet_stats.pkts_f_tag_stripped++;
    } else {
        memcpy(skb_generic_pkt->data, pkt, pkt_len);
    }
    skb_put(skb_generic_pkt, pkt_len);
    skb_generic_pkt->len = pkt_len;
    generic_pkt->skb = skb_generic_pkt;
    if (debug & GENL_DBG_LVL_PDMP) {
        dump_skb(skb_generic_pkt);
    }
    /* generic_pkt end */

    spin_lock_irqsave(&g_bcmgenl_packet_work.lock, flags);
    list_add_tail(&generic_pkt->list, &g_bcmgenl_packet_work.pkt_list);

    g_bcmgenl_packet_stats.pkts_c_qlen_cur++;
    if (g_bcmgenl_packet_stats.pkts_c_qlen_cur >
        g_bcmgenl_packet_stats.pkts_c_qlen_hi) {
        g_bcmgenl_packet_stats.pkts_c_qlen_hi =
            g_bcmgenl_packet_stats.pkts_c_qlen_cur;
    }

    schedule_work(&g_bcmgenl_packet_work.wq);
    spin_unlock_irqrestore(&g_bcmgenl_packet_work.lock, flags);

    /*
     * expected rv values:
     * -ve for error
     * 0 for passthrough
     * 1 for packet handled
     *
     */

    /* Set rv to packet handled */
    rv = 1;

FILTER_CB_PKT_HANDLED:
    if (rv == 1) {
        g_bcmgenl_packet_stats.pkts_f_handled++;
    } else {
        g_bcmgenl_packet_stats.pkts_f_pass_through++;
        if (generic_pkt) {
            kfree(generic_pkt);
        }
    }
    dev_kfree_skb_any(skb);
    return NULL;
}

static void
bcmgenl_packet_task(struct work_struct *work)
{
    bcmgenl_packet_work_t *packet_work =
        container_of(work, bcmgenl_packet_work_t, wq);
    unsigned long flags;
    struct list_head *list_ptr, *list_next;
    genl_pkt_t *pkt;

    spin_lock_irqsave(&packet_work->lock, flags);
    list_for_each_safe(list_ptr, list_next, &packet_work->pkt_list) {
        /* dequeue pkt from list */
        pkt = list_entry(list_ptr, genl_pkt_t, list);
        list_del(list_ptr);
        g_bcmgenl_packet_stats.pkts_c_qlen_cur--;
        spin_unlock_irqrestore(&packet_work->lock, flags);

        /* send generic_pkt to generic netlink */
        if (pkt) {
            GENL_DBG_VERB
                ("%s: netns 0x%p, in_ifindex %d, out_ifindex %d, context 0x%08x\n",
                 __func__,
                 pkt->netns,
                 pkt->meta.in_ifindex,
                 pkt->meta.out_ifindex,
                 pkt->meta.context);
            genl_packet_send_packet(pkt->netns,
                                    pkt->skb,
                                    pkt->meta.in_ifindex,
                                    pkt->meta.out_ifindex,
                                    pkt->meta.context);
            g_bcmgenl_packet_stats.pkts_f_packet_mod++;

            dev_kfree_skb_any(pkt->skb);
            kfree(pkt);
        }
        spin_lock_irqsave(&packet_work->lock, flags);
    }
    spin_unlock_irqrestore(&packet_work->lock, flags);
}

static int
bcmgenl_packet_netif_create_cb(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif)
{
    bool found;
    struct list_head *list;
    bcmgenl_netif_t *new_netif, *lbcmgenl_netif;
    unsigned long flags;

    if (!dinfo) {
        GENL_DBG_WARN("%s: dinfo is NULL\n", __func__);
        return (-1);
    }
    if (netif->id == 0) {
        GENL_DBG_WARN("%s: netif->id == 0 is not a valid interface ID\n", __func__);
        return (-1);
    }
    if ((new_netif = kmalloc(sizeof(bcmgenl_netif_t), GFP_ATOMIC)) == NULL) {
        GENL_DBG_WARN("%s: failed to alloc psample mem for netif '%s'\n",
                      __func__, netif->name);
        return (-1);
    }

    spin_lock_irqsave(&g_bcmgenl_packet_info.lock, flags);
    new_netif->dev = dinfo->vdev[netif->id];
    new_netif->id = netif->id;
    new_netif->port = netif->port;
    new_netif->vlan = netif->vlan;

    /* insert netif sorted by ID similar to ngknet_netif_create() */
    found = false;
    list_for_each(list, &g_bcmgenl_packet_info.netif_list) {
        lbcmgenl_netif = (bcmgenl_netif_t *)list;
        if (netif->id < lbcmgenl_netif->id) {
            found = true;
            break;
        }
    }

    if (found) {
        /* Replace previously removed interface */
        list_add_tail(&new_netif->list, &lbcmgenl_netif->list);
    } else {
        /* No holes - add to end of list */
        list_add_tail(&new_netif->list, &g_bcmgenl_packet_info.netif_list);
    }
    g_bcmgenl_packet_info.netif_count++;
    spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);

    GENL_DBG_VERB
        ("%s: added netlink packet netif '%s'\n", __func__, netif->name);
    return (0);
}

static int
bcmgenl_packet_netif_destroy_cb(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif)
{
    bool found = false;
    struct list_head *list;
    bcmgenl_netif_t *lbcmgenl_netif;
    unsigned long flags;

    if (!dinfo || !netif) {
        GENL_DBG_WARN("%s: dinfo or netif is NULL\n", __func__);
        return (-1);
    }

    if (g_bcmgenl_packet_info.netif_count == 0) {
        GENL_DBG_WARN("%s: no netif is created\n", __func__);
        return (0);
    }
    spin_lock_irqsave(&g_bcmgenl_packet_info.lock, flags);

    list_for_each(list, &g_bcmgenl_packet_info.netif_list) {
        lbcmgenl_netif = (bcmgenl_netif_t *)list;
        if (netif->id == lbcmgenl_netif->id) {
            found = true;
            list_del(&lbcmgenl_netif->list);
            GENL_DBG_VERB
                ("%s: removing generic netif '%s'\n", __func__, netif->name);
            kfree(lbcmgenl_netif);
            g_bcmgenl_packet_info.netif_count--;
            break;
        }
    }

    spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);

    if (!found) {
        GENL_DBG_WARN("%s: netif ID %d not found!\n", __func__, netif->id);
        return (-1);
    }
    return (0);
}

/*
 * map Proc Read Entry
 */
static int
bcmgenl_packet_proc_map_show(struct seq_file *m, void *v)
{
    struct list_head *list;
    bcmgenl_netif_t *bcmgenl_netif;
    unsigned long flags;

    seq_printf(m, "  Interface      logical port   ifindex\n");
    seq_printf(m, "-------------    ------------   -------\n");
    spin_lock_irqsave(&g_bcmgenl_packet_info.lock, flags);

    list_for_each(list, &g_bcmgenl_packet_info.netif_list) {
        bcmgenl_netif = (bcmgenl_netif_t*)list;
        seq_printf(m, "  %-14s %-14d %d\n",
                bcmgenl_netif->dev->name,
                bcmgenl_netif->port,
                bcmgenl_netif->dev->ifindex);
    }

    spin_unlock_irqrestore(&g_bcmgenl_packet_info.lock, flags);
    return 0;
}

static int
bcmgenl_packet_proc_map_open(struct inode * inode, struct file * file)
{
    return single_open(file, bcmgenl_packet_proc_map_show, NULL);
}

static struct proc_ops bcmgenl_packet_proc_map_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bcmgenl_packet_proc_map_open,
    .proc_read =        seq_read,
    .proc_write =       NULL,
    .proc_lseek =       seq_lseek,
    .proc_release =     single_release,
};

static int
bcmgenl_packet_proc_stats_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Stats\n", BCMGENL_PACKET_NAME);
    seq_printf(m, "  DCB type %d\n",                          g_bcmgenl_packet_info.hw.dcb_type);
    seq_printf(m, "  pkts filter generic cb         %10lu\n", g_bcmgenl_packet_stats.pkts_f_packet_cb);
    seq_printf(m, "  pkts sent to generic module    %10lu\n", g_bcmgenl_packet_stats.pkts_f_packet_mod);
    seq_printf(m, "  pkts handled by generic cb     %10lu\n", g_bcmgenl_packet_stats.pkts_f_handled);
    seq_printf(m, "  pkts pass through              %10lu\n", g_bcmgenl_packet_stats.pkts_f_pass_through);
    seq_printf(m, "  pkts with vlan tag checked     %10lu\n", g_bcmgenl_packet_stats.pkts_f_tag_checked);
    seq_printf(m, "  pkts with vlan tag stripped    %10lu\n", g_bcmgenl_packet_stats.pkts_f_tag_stripped);
    seq_printf(m, "  pkts with mc destination       %10lu\n", g_bcmgenl_packet_stats.pkts_f_dst_mc);
    seq_printf(m, "  pkts with cpu source           %10lu\n", g_bcmgenl_packet_stats.pkts_f_src_cpu);
    seq_printf(m, "  pkts with cpu destination      %10lu\n", g_bcmgenl_packet_stats.pkts_f_dst_cpu);
    seq_printf(m, "  pkts current queue length      %10lu\n", g_bcmgenl_packet_stats.pkts_c_qlen_cur);
    seq_printf(m, "  pkts high queue length         %10lu\n", g_bcmgenl_packet_stats.pkts_c_qlen_hi);
    seq_printf(m, "  pkts drop max queue length     %10lu\n", g_bcmgenl_packet_stats.pkts_d_qlen_max);
    seq_printf(m, "  pkts drop no memory            %10lu\n", g_bcmgenl_packet_stats.pkts_d_no_mem);
    seq_printf(m, "  pkts drop generic not ready    %10lu\n", g_bcmgenl_packet_stats.pkts_d_not_ready);
    seq_printf(m, "  pkts drop metadata parse error %10lu\n", g_bcmgenl_packet_stats.pkts_d_metadata);
    seq_printf(m, "  pkts drop skb error            %10lu\n", g_bcmgenl_packet_stats.pkts_d_skb);
    seq_printf(m, "  pkts drop skb cbd error        %10lu\n", g_bcmgenl_packet_stats.pkts_d_skb_cbd);
    seq_printf(m, "  pkts with invalid src port     %10lu\n", g_bcmgenl_packet_stats.pkts_d_meta_srcport);
    seq_printf(m, "  pkts with invalid dst port     %10lu\n", g_bcmgenl_packet_stats.pkts_d_meta_dstport);
    seq_printf(m, "  pkts with invalid orig pkt sz  %10lu\n", g_bcmgenl_packet_stats.pkts_d_invalid_size);
    return 0;
}

static int
bcmgenl_packet_proc_stats_open(struct inode * inode, struct file * file)
{
    return single_open(file, bcmgenl_packet_proc_stats_show, NULL);
}

/*
 * generic stats Proc Write Entry
 *
 *   Syntax:
 *   write any value to clear stats
 */
static ssize_t
bcmgenl_packet_proc_stats_write(
    struct file *file, const char *buf,
    size_t count, loff_t *loff)
{
    int qlen_cur = 0;
    unsigned long flags;

    spin_lock_irqsave(&g_bcmgenl_packet_work.lock, flags);
    qlen_cur = g_bcmgenl_packet_stats.pkts_c_qlen_cur;
    memset(&g_bcmgenl_packet_stats, 0, sizeof(bcmgenl_packet_stats_t));
    g_bcmgenl_packet_stats.pkts_c_qlen_cur = qlen_cur;
    spin_unlock_irqrestore(&g_bcmgenl_packet_work.lock, flags);

    return count;
}

static struct proc_ops bcmgenl_packet_proc_stats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bcmgenl_packet_proc_stats_open,
    .proc_read =        seq_read,
    .proc_write =       bcmgenl_packet_proc_stats_write,
    .proc_lseek =       seq_lseek,
    .proc_release =     single_release,
};

/*
 * generic debug Proc Read Entry
 */
static int
bcmgenl_packet_proc_debug_show(struct seq_file *m, void *v)
{
    seq_printf(m, "BCM KNET %s Callback Config\n", BCMGENL_PACKET_NAME);
    seq_printf(m, "  debug:           0x%x\n", debug);
    seq_printf(m, "  cmic_type:       %d\n",   g_bcmgenl_packet_info.hw.cmic_type);
    seq_printf(m, "  dcb_type:        %d\n",   g_bcmgenl_packet_info.hw.dcb_type);
    seq_printf(m, "  dcb_size:        %d\n",   g_bcmgenl_packet_info.hw.dcb_size);
    seq_printf(m, "  pkt_hdr_size:    %d\n",   g_bcmgenl_packet_info.hw.pkt_hdr_size);
    seq_printf(m, "  cdma_channels:   %d\n",   g_bcmgenl_packet_info.hw.cdma_channels);
    seq_printf(m, "  netif_count:     %d\n",   g_bcmgenl_packet_info.netif_count);
    seq_printf(m, "  queue length:    %d\n",   bcmgenl_packet_qlen);

    return 0;
}

static int
bcmgenl_packet_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, bcmgenl_packet_proc_debug_show, NULL);
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
bcmgenl_packet_proc_debug_write(
    struct file *file, const char *buf,
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
        GENL_DBG_WARN("Warning: unknown configuration setting\n");
    }

    return count;
}

static struct proc_ops bcmgenl_packet_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bcmgenl_packet_proc_debug_open,
    .proc_read =        seq_read,
    .proc_write =       bcmgenl_packet_proc_debug_write,
    .proc_lseek =       seq_lseek,
    .proc_release =     single_release,
};

static int
genl_cb_proc_cleanup(void)
{
    remove_proc_entry("stats", bcmgenl_packet_proc_root);
    remove_proc_entry("debug", bcmgenl_packet_proc_root);
    remove_proc_entry("map"  , bcmgenl_packet_proc_root);

    proc_remove(bcmgenl_packet_proc_root);
    return 0;
}

static int
genl_cb_proc_init(void)
{
    char packet_procfs_path[PROCFS_MAX_PATH];
    struct proc_dir_entry *entry;

    /* create procfs for generic */
    snprintf(packet_procfs_path, PROCFS_MAX_PATH, "%s/%s",
             BCMGENL_MODULE_NAME, BCMGENL_PACKET_NAME);
    bcmgenl_packet_proc_root = proc_mkdir(packet_procfs_path, NULL);

    /* create procfs for generic stats */
    PROC_CREATE(entry, "stats", 0666, bcmgenl_packet_proc_root,
                &bcmgenl_packet_proc_stats_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/stats'\n",
               __func__, packet_procfs_path);
        return -1;
    }

    /* create procfs for getting netdev mapping */
    PROC_CREATE(entry, "map", 0666, bcmgenl_packet_proc_root,
                &bcmgenl_packet_proc_map_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/map'\n",
               __func__, packet_procfs_path);
        return -1;
    }

    /* create procfs for debug log */
    PROC_CREATE(entry, "debug", 0666, bcmgenl_packet_proc_root,
                &bcmgenl_packet_proc_debug_file_ops);
    if (entry == NULL) {
        printk("%s: Unable to create procfs entry '/procfs/%s/debug'\n",
               __func__, packet_procfs_path);
        return -1;
    }
    return 0;
}

static int
genl_cb_cleanup(void)
{
    genl_pkt_t *pkt;

    cancel_work_sync(&g_bcmgenl_packet_work.wq);

    while (!list_empty(&g_bcmgenl_packet_work.pkt_list)) {
        pkt = list_entry(g_bcmgenl_packet_work.pkt_list.next,
                         genl_pkt_t, list);
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
    memset(&g_bcmgenl_packet_stats, 0, sizeof(bcmgenl_packet_stats_t));
    memset(&g_bcmgenl_packet_info, 0, sizeof(bcmgenl_info_t));
    memset(&g_bcmgenl_packet_work, 0, sizeof(bcmgenl_packet_work_t));

    /* setup bcmgenl_packet_info struct */
    INIT_LIST_HEAD(&g_bcmgenl_packet_info.netif_list);
    spin_lock_init(&g_bcmgenl_packet_info.lock);

    /* setup generic work queue */
    spin_lock_init(&g_bcmgenl_packet_work.lock);
    INIT_LIST_HEAD(&g_bcmgenl_packet_work.pkt_list);
    INIT_WORK(&g_bcmgenl_packet_work.wq, bcmgenl_packet_task);

    /* get net namespace */
    g_bcmgenl_packet_info.netns = get_net_ns_by_pid(current->pid);
    if (!g_bcmgenl_packet_info.netns) {
        GENL_DBG_WARN("%s: Could not get network namespace for pid %d\n",
                      __func__, current->pid);
        return (-1);
    }
    GENL_DBG_VERB
        ("%s: current->pid %d, netns 0x%p\n",
         __func__, current->pid, g_bcmgenl_packet_info.netns);
    return 0;
}

int bcmgenl_packet_cleanup(void)
{
    ngknet_netif_create_cb_unregister(bcmgenl_packet_netif_create_cb);
    ngknet_netif_destroy_cb_unregister(bcmgenl_packet_netif_destroy_cb);
    ngknet_filter_cb_unregister(bcmgenl_packet_filter_cb);
    genl_cb_cleanup();
    genl_cb_proc_cleanup();
    return 0;
}

int bcmgenl_packet_init(void)
{
    ngknet_netif_create_cb_register(bcmgenl_packet_netif_create_cb);
    ngknet_netif_destroy_cb_register(bcmgenl_packet_netif_destroy_cb);
    ngknet_filter_cb_register_by_name
        (bcmgenl_packet_filter_cb, BCMGENL_PACKET_NAME);

    genl_cb_proc_init();
    return genl_cb_init();
}

EXPORT_SYMBOL(bcmgenl_packet_cleanup);
EXPORT_SYMBOL(bcmgenl_packet_init);
#endif
