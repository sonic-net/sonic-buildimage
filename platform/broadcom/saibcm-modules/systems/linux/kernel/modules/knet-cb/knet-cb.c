/*
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
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
 * Driver for call-back functions for Linux KNET driver.
 *
 * This is sample code that demonstrates how to selectively strip VLAN tags
 * from an incoming packet based on tag information in the DMA control block
 * (DCB). The switch will automatically add a VLAN tag to packets that ingress
 * without an outer VLAN tag. Outer tagged and double tagged packets are
 * not modified. The call back defined here determines which packets have
 * had tags added by those and strips only those tags from the packet.
 *
 * This is sample code, the customer is responsible for maintaining and
 * modifying this code as necessary.
 *
 * The module can be built from the standard Linux user mode target
 * directories using the following command (assuming bash), e.g.
 *
 *   cd $SDK/systems/linux/user/<target>
 *   make BUILD_KNET_CB=1
 *
 */

#include <gmodule.h> /* Must be included first */
#include <kcom.h>
#include <bcm-knet.h>
#include <linux/if_vlan.h>

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Broadcom Linux KNET Call-Back Driver");
MODULE_LICENSE("GPL");

int debug;
LKM_MOD_PARAM(debug, "i", int, 0);
MODULE_PARM_DESC(debug,
"Debug level (default 0)");

static int tpid=0x8100;
LKM_MOD_PARAM(tpid, "i", int, 0);
MODULE_PARM_DESC(debug,
"Tag Protocol Identifier (TPID) indicates the frame type (default 0x8100)");

static int pri=0;
LKM_MOD_PARAM(pri, "i", int, 0);
MODULE_PARM_DESC(pri,
"Priority (PRI) indicates the frame priority (default 0)");

static int cfi=0;
LKM_MOD_PARAM(cfi, "i", int, 0);
MODULE_PARM_DESC(cfi,
"Canonical Format Indicator (CFI) indicates whether a MAC address is encapsulated in canonical format over different transmission media (default 0)");

static int vid=0;
LKM_MOD_PARAM(vid, "i", int, 0);
MODULE_PARM_DESC(vid,
"VLAN ID (VID) indicates the VLAN to which a frame belongs (default 0)");

/* Module Information */
#define MODULE_MAJOR 121
#define MODULE_NAME "linux-knet-cb"

/* set KNET_CB_DEBUG for debug info */
#define KNET_CB_DEBUG

/* These below need to match incoming enum values */
#define FILTER_TAG_STRIP 0
#define FILTER_TAG_KEEP  1
#define FILTER_TAG_ORIGINAL 2

/* Maintain tag strip statistics */
struct strip_stats_s {
    unsigned long stripped;     /* Number of packets that have been stripped */
    unsigned long checked;
    unsigned long skipped;
};

static struct strip_stats_s strip_stats;

/* Local function prototypes */
static void strip_vlan_tag(struct sk_buff *skb);
static int  get_tag_status(int dcb_type, void *meta);
static struct sk_buff *strip_tag_rx_cb(struct sk_buff *skb, int dev_no, void *meta);
static struct sk_buff *strip_tag_tx_cb(struct sk_buff *skb, int dev_no, void *meta);
int strip_tag_filter_cb(uint8_t * pkt, int size, int dev_no, void *meta,
                                int chan, kcom_filter_t * kf);
static int  _pprint(struct seq_file *m);
static int  _cleanup(void);
static int  _init(void);

/* Remove VLAN tag for select TPIDs */
static void
strip_vlan_tag(struct sk_buff *skb)
{
    uint16_t    vlan_proto = (uint16_t) ((skb->data[12] << 8) | skb->data[13]);
    if ((vlan_proto == 0x8100) || (vlan_proto == 0x88a8) || (vlan_proto == 0x9100)) {
        /* Move first 12 bytes of packet back by 4 */
        ((u32 *) skb->data)[3] = ((u32 *) skb->data)[2];
        ((u32 *) skb->data)[2] = ((u32 *) skb->data)[1];
        ((u32 *) skb->data)[1] = ((u32 *) skb->data)[0];
        skb_pull(skb, 4);       /* Remove 4 bytes from start of buffer */
    }
}

/* Add VLAN tag to untagged packet */
static void
add_vlan_tag(struct sk_buff *skb, u32 forward_domain)
{
    u32 vlan = 0;
    uint16_t    vlan_proto = (uint16_t) ((skb->data[12] << 8) | skb->data[13]);

    if ((vlan_proto != 0x8100) && (vlan_proto != 0x88a8) && (vlan_proto != 0x9100)) {
        /* If vid is specified, use configued vid as VLAN ID, or, use forward_domain as vid */
        vlan = vid ? vid: forward_domain;

        skb_push(skb, 4);       /* Add 4 bytes from start of buffer */
        /* Move first 12 bytes of packet forward by 4 */
        ((u32 *) skb->data)[0] = ((u32 *) skb->data)[1];
        ((u32 *) skb->data)[1] = ((u32 *) skb->data)[2];
        ((u32 *) skb->data)[2] = ((u32 *) skb->data)[3];

        /* Set VLAN tag */
        skb->data[12] = (tpid >> 8) & 0xff;
        skb->data[13] = tpid & 0xff;
        skb->data[14] = (((pri & 0x7) << 5) | ((cfi & 0x1) << 4) | ((vlan >> 8) & 0xf)) & 0xff;
        skb->data[15] = vlan & 0xff;
    }
}

/*
 * Location of tagging status in select DCB types found below:
 *
 * DCB type 14: word 12, bits 10.11
 * DCB type 19, 20, 21, 22, 30: word 12, bits 10..11
 * DCB type 23, 29: word 13, bits 0..1
 * DCB type 31, 34, 37: word 13, bits 0..1
 * DCB type 26, 32, 33, 35: word 13, bits 0..1
 *
 * The function get_tag_status() returns the tag status for known DCB types.
 * 0 = Untagged
 * 1 = Single inner-tag
 * 2 = Single outer-tag
 * 3 = Double tagged.
 * 4 = Dedicated for Dune device, packets are received with original tag status.
 * -1 = Unsupported DCB type
 */
static int
get_tag_status(int dcb_type, void *meta)
{
    uint32     *dcb = (uint32 *) meta;
    int         tag_status;
    switch (dcb_type) {
        case 14:
        case 19:
        case 20:
        case 21:
        case 22:
        case 30:
            tag_status = (dcb[12] > 10) & 0x3;
            break;
        case 23:
        case 29:
        case 31:
        case 34:
        case 37:
        case 26:
        case 32:
        case 33:
        case 35:
            tag_status = dcb[13] & 0x3;
            break;
        case 36:
            /* TD3 */
            tag_status = ((dcb[13] >> 9) & 0x3);
            break;
        break;
        case 38:
        {
            /* untested */
            /* TH3 only parses outer tag. */
            const int   tag_map[4] = { 0, 2, -1, -1 };
            tag_status = tag_map[(dcb[9] >> 13) & 0x3];
        }
        break;
        case 28:
        case 39:
            tag_status = 4;
            break;
        break;
        default:
            tag_status = -1;
            break;
    }
#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        gprintk("%s; DCB Type: %d; tag status: %d\n", __func__, dcb_type, tag_status);
    }
#endif
    return tag_status;
}

/* Rx packet callback function */
static struct sk_buff *
strip_tag_rx_cb(struct sk_buff *skb, int dev_no, void *meta)
{
    unsigned    netif_flags = KNET_SKB_CB(skb)->netif_user_data;
    unsigned    filter_flags =  KNET_SKB_CB(skb)->filter_user_data;
    unsigned    dcb_type;
    int         tag_status;
    unsigned    int strip_tag = 0;
    /* Currently not using filter flags:
     * unsigned    filter_flags = KNET_SKB_CB(skb)->filter_user_data;
     */

#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        gprintk("%s Enter; netif Flags: %08X filter_flags %08X \n",
                __func__, netif_flags, filter_flags);
    }
#endif
    /* Get DCB type for this packet, passed by KNET driver */
    dcb_type = KNET_SKB_CB(skb)->dcb_type;

    /* KNET implements this already */
    if (filter_flags == FILTER_TAG_KEEP)
    {
        if (dcb_type ==28 || dcb_type == 39)
        {
            uint32 *meta_buffer = (uint32 *)meta;
            uint32 forward_domain = meta_buffer[1] & 0xffff;
            add_vlan_tag(skb, forward_domain);
        }
        strip_stats.skipped++;
        return skb;
    }

    /* SAI strip implies always strip. If the packet is untagged or
       inner taged, SDK adds a .1q tag, so we need to strip tag
       anyway */
    if (filter_flags == FILTER_TAG_STRIP)
    {
        strip_tag = 1;
    }



    /* Get tag status from DCB */
    tag_status = get_tag_status(dcb_type, meta);
#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        gprintk("%s; DCB Type: %d; tag status: %d\n", __func__, dcb_type, tag_status);
    }
#endif
    if (tag_status < 0) {
        /* Unsupported DCB type */
        return skb;
    }

    if (filter_flags == FILTER_TAG_ORIGINAL)
    {
        /* If untagged or single inner, strip the extra tag that knet
           keep tag will add. */
        if (tag_status  <  2)
        {
            strip_tag = 1;
        }
    }

    strip_stats.checked++;

    if (strip_tag) {
#ifdef KNET_CB_DEBUG
        if (debug & 0x1) {
            gprintk("%s; Stripping VLAN tag\n", __func__);
        }
#endif
        strip_stats.stripped++;
        strip_vlan_tag(skb);
    }
#ifdef KNET_CB_DEBUG
    else {
        if (debug & 0x1) {
            gprintk("%s; Keeping VLAN tag\n", __func__);
        }
    }
#endif

    return skb;
}

/* Tx callback not used */
static struct sk_buff *
strip_tag_tx_cb(struct sk_buff *skb, int dev_no, void *meta)
{
    /* Pass through for now */
    return skb;
}

/* Filter callback not used */
int
strip_tag_filter_cb(uint8_t * pkt, int size, int dev_no, void *meta,
                    int chan, kcom_filter_t *kf)
{
    /* Pass through for now */
    return 0;
}

#ifdef BCM_DNX_SUPPORT
static int
knet_filter_cb(uint8_t * pkt, int size, int dev_no, void *meta,
                     int chan, kcom_filter_t *kf)
{
    /* check for filter callback handler */
#ifdef PSAMPLE_SUPPORT
    if (strncmp(kf->desc, PSAMPLE_CB_NAME, strlen(PSAMPLE_CB_NAME)) == 0) {
        return psample_filter_cb (pkt, size, dev_no, meta, chan, kf);
    }
#endif
    return strip_tag_filter_cb (pkt, size, dev_no, meta, chan, kf);
}

static int
/*knet_netif_create_cb(int unit, kcom_netif_t *netif, struct net_device *dev)*/
knet_netif_create_cb(struct net_device *dev, int unit, kcom_netif_t *netif)
{
    int retv = 0;
#ifdef PSAMPLE_SUPPORT
    retv = psample_netif_create_cb(unit, netif, dev);
#endif
    return retv;
}

static int
/*knet_netif_destroy_cb(int unit, kcom_netif_t *netif, struct net_device *dev)*/
knet_netif_destroy_cb(struct net_device *dev, int unit, kcom_netif_t *netif)
{
    int retv = 0;
#ifdef PSAMPLE_SUPPORT
    retv = psample_netif_destroy_cb(unit, netif, dev);
#endif
    return retv;
}
#endif

/*
 * Get statistics.
 * % cat /proc/linux-knet-cb
 */
static int
_pprint(struct seq_file *m)
{
    pprintf(m, "Broadcom Linux KNET Call-Back: Untagged VLAN Stripper\n");
    pprintf(m, "    %lu stripped packets\n", strip_stats.stripped);
    pprintf(m ,"    %lu packets checked\n", strip_stats.checked);
    pprintf(m, "    %lu packets skipped\n", strip_stats.skipped);

    return 0;
}

static int
_cleanup(void)
{
    bkn_rx_skb_cb_unregister(strip_tag_rx_cb);
    /* strip_tag_tx_cb is currently a noop, so
     * no need to unregister.
     */
    if (0)
    {
        bkn_tx_skb_cb_unregister(strip_tag_tx_cb);
    }

    return 0;
}

static int
_init(void)
{
    bkn_rx_skb_cb_register(strip_tag_rx_cb);
    /* strip_tag_tx_cb is currently a noop, so
     * no need to register.
     */
    if (0)
    {
        bkn_tx_skb_cb_register(strip_tag_tx_cb);
    }

    return 0;
}

static gmodule_t _gmodule = {
    name: MODULE_NAME,
    major: MODULE_MAJOR,
    init: _init,
    cleanup: _cleanup,
    pprint: _pprint,
    ioctl: NULL,
    open: NULL,
    close: NULL,
};

gmodule_t*
gmodule_get(void)
{
    EXPORT_NO_SYMBOLS;
    return &_gmodule;
}
