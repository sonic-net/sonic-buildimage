/*! \file ngknetcb_main.c
 *
 * NGKNET Callback module entry.
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
#include <lkm/ngknet_kapi.h>
#include <ngknet_linux.h>
#include <ngknet_callback.h>
#include "bcmcnet/bcmcnet_core.h"

#ifdef KPMD
#include <bcmpkt/bcmpkt_flexhdr_internal.h>
#include <bcmpkt/bcmpkt_flexhdr_field.h>
#include <bcmpkt/bcmpkt_higig_defs.h>
#include <bcmpkt/bcmpkt_lbhdr_field.h>
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_rxpmd_defs.h>
#include <bcmpkt/bcmpkt_rxpmd_fid.h>
#include <bcmpkt/bcmpkt_rxpmd_field.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id.h>
#include <bcmpkt/bcmpkt_txpmd_field.h>
#endif /* KPMD */

/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("NGKNET Callback Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! \cond */
int debug = 0;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug,
"Debug level (default 0)");
/*! \endcond */

/*! Module information */
#define NGKNETCB_MODULE_NAME    "linux_ngknetcb"
#define NGKNETCB_MODULE_MAJOR   122

/* set KNET_CB_DEBUG for debug info */
#define KNET_CB_DEBUG

/* These below need to match incoming enum values */
#define FILTER_TAG_STRIP 0
#define FILTER_TAG_KEEP  1
#define FILTER_TAG_ORIGINAL 2

#define NGKNET_CB_DBG_LVL_VERB       0x0001
#define NGKNET_CB_DBG_LVL_PDMP       0x0002
#define NGKNET_CB_DBG_LVL_WARN       0x0004

#ifndef KPMD
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    BCMDRD_DEV_T_##_bd,
/*! Enumeration for all base device types. */
typedef enum {
    BCMDRD_DEV_T_NONE = 0,
/*! \cond */
#include <bcmdrd/bcmdrd_devlist.h>
/*! \endcond */
    BCMDRD_DEV_T_COUNT
} bcmdrd_dev_type_t;

/*! Create enumeration values from list of supported variants. */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
    BCMLRD_VARIANT_T_##_bd##_##_ve,

/*! Enumeration for all device variants. */
typedef enum bcmlrd_variant_e {
    BCMLRD_VARIANT_T_NONE = 0,
/*! \cond */
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
/*! \endcond */
     BCMLRD_VARIANT_T_COUNT
} bcmlrd_variant_t;
#endif /* !KPMD */

typedef struct ngknetcb_dev_s {
    bool initialized;
    bcmdrd_dev_type_t dev_type;
    bcmlrd_variant_t var_type;
} ngknetcb_dev_t;

static ngknetcb_dev_t cb_dev[NUM_PDMA_DEV_MAX];

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    {#_bd, BCMDRD_DEV_T_##_bd},
static const struct {
    char *name;
    bcmdrd_dev_type_t dev;
} device_types[] = {
    {"device_none", BCMDRD_DEV_T_NONE},
#include <bcmdrd/bcmdrd_devlist.h>
    {"device_count", BCMDRD_DEV_T_COUNT}
};

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
    {#_bd, #_ve, BCMLRD_VARIANT_T_##_bd##_##_ve},
static const struct {
    char *dev_name;
    char *var_name;
    bcmlrd_variant_t var;
} variant_types[] = {
    {"device_none", "variant_none", BCMLRD_VARIANT_T_NONE},
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    {"device_count", "variant_count", BCMLRD_VARIANT_T_COUNT}
};

#ifdef KPMD
/*
  Change this structure to reflect the match_ids of interest.
  This is an example of how it can be used.
 */
typedef struct cb_match_id_s {
    int egress_pkt_fwd_l2_hdr_etag;
    int egress_pkt_fwd_l2_hdr_l2;
    int ingress_pkt_inner_l2_hdr_l2;
    int ingress_pkt_fwd_l2_hdr_etag;
    int ingress_pkt_outer_l2_hdr_itag;
    int ingress_pkt_outer_l2_hdr_otag;
} cb_match_id_t;

static cb_match_id_t match_id;

struct name_value_pair_s {
    char *name;
    int value;
};

static struct name_value_pair_s rxpmd_info[] = {
    BCMPKT_RXPMD_FIELD_NAME_MAP_INIT
};

static const shr_enum_map_t reason_names[] =
{
    BCMPKT_REASON_NAME_MAP_INIT
};

static void
print_all_rxpmd_fields(
    bcmdrd_dev_type_t dev_type,
    const uint8_t *rxpmd)
{
    int rv, fid;
    bcmpkt_rxpmd_fid_support_t support;
    uint32_t val;

    printk("\n[RX metadata information]:\n");
    bcmpkt_rxpmd_fid_support_get(dev_type, &support);

    BCMPKT_RXPMD_FID_SUPPORT_ITER(support, fid) {
        rv = bcmpkt_rxpmd_field_get
            (dev_type, (uint32_t *)rxpmd, fid, &val);
        if (rv == 0) {
            printk("  %-26s = %10d [0x%X]\n", rxpmd_info[fid].name, val, val);
        }
    }
}


static void
print_all_rx_reason(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd)
{
    int reason, rv;
    bcmpkt_rx_reasons_t reasons;

    if (rxpmd) {
        BCMPKT_RX_REASON_CLEAR_ALL(reasons);
        rv = bcmpkt_rxpmd_reasons_get(dev_type, rxpmd, &reasons);
        if (rv == 0) {
            BCMPKT_RX_REASON_ITER(reasons, reason) {
                printk("  %s\n", reason_names[reason].name);
            }
        }
    }
}
#endif /* KPMD */


/* Maintain tag strip statistics */
struct strip_stats_s {
    unsigned long stripped;     /* Number of packets that have been stripped */
    unsigned long checked;
    unsigned long skipped;
};

static struct strip_stats_s strip_stats;
static unsigned int rx_count = 0;

/* Local function prototypes */
static void strip_vlan_tag(struct sk_buff *skb);

/* Remove VLAN tag for select TPIDs */
static void
strip_vlan_tag(struct sk_buff *skb)
{
    uint16_t    vlan_proto;
    uint8_t     *pkt = skb->data;

    vlan_proto = (uint16_t) ((pkt[12] << 8) | pkt[13]);
    if ((vlan_proto == 0x8100) || (vlan_proto == 0x88a8) || (vlan_proto == 0x9100)) {
        /* Move first 12 bytes of packet back by 4 */
        memmove(&skb->data[4], skb->data, 12);
        skb_pull(skb, 4);       /* Remove 4 bytes from start of buffer */
    }
}

/*
 * The function get_tag_status() returns the tag status.
 * 0  = Untagged
 * 1  = Single inner-tag
 * 2  = Single outer-tag
 * 3  = Double tagged.
 * -1 = Unsupported type
 */
static int
get_tag_status(uint32_t dev_type, uint32_t variant, void *rxpmd)
{
    int rv;
    const char *tag_type[4] = {
        "Untagged",
        "Inner Tagged",
        "Outer Tagged",
        "Double Tagged"
    };
    int tag_status = -1;
    bcmpkt_rxpmd_fid_support_t support;
    uint32_t val = 0;

    bcmpkt_rxpmd_fid_support_get(dev_type, &support);

    if (BCMPKT_RXPMD_FID_SUPPORT_GET(support, BCMPKT_RXPMD_ING_TAG_TYPE)) {
        rv = bcmpkt_rxpmd_field_get(dev_type, (uint32_t *)rxpmd,
                                    BCMPKT_RXPMD_ING_TAG_TYPE, &val);
        /* Tomahawk4 family */

        /*
         * Indicates the incoming tag status (INCOMING_TAG_STATUS):
         * For single tag device:
         *   0: untagged, 1: tagged
         * For double tag device:
         *   0: untagged, 1: single inner-tag, 2: single outer-tag, 3: double tagged
         */
        if (SHR_SUCCESS(rv)) {
            if (1 == val) {
#ifdef KNET_CB_DEBUG
                if (debug & 0x1){
                    printk("  Incoming frame tagged\n");
                }
#endif
                tag_status = 2;
            } else if (0 == val) {
#ifdef KNET_CB_DEBUG
                if (debug & 0x1){
                    printk("  Incoming frame untagged\n");
                }
#endif
                tag_status = 0;
            }
        }
    } else if (BCMPKT_RXPMD_FID_SUPPORT_GET(support, BCMPKT_RXPMD_MATCH_ID_LO) &&
               BCMPKT_RXPMD_FID_SUPPORT_GET(support, BCMPKT_RXPMD_MATCH_ID_HI)) {
        /* Trident4 family. */
        uint32_t match_id_data[2];
        bool itag = false, otag = false;

        bcmpkt_rxpmd_field_get(dev_type, rxpmd, BCMPKT_RXPMD_MATCH_ID_LO,
                               &match_id_data[0]);
        bcmpkt_rxpmd_field_get(dev_type, rxpmd, BCMPKT_RXPMD_MATCH_ID_HI,
                               &match_id_data[1]);
        rv = bcmpkt_rxpmd_match_id_present(variant, match_id_data, 2,
                                           match_id.ingress_pkt_outer_l2_hdr_itag);
        if (SHR_SUCCESS(rv)) {
            itag = true;
        }
        rv = bcmpkt_rxpmd_match_id_present(variant, match_id_data, 2,
                                           match_id.ingress_pkt_outer_l2_hdr_otag);
        if (SHR_SUCCESS(rv)) {
            otag = true;
        }
        if (itag && otag) {
            tag_status = 3;
        } else if (itag) {
            tag_status = 1;
        } else if (otag) {
            tag_status = 2;
        } else {
            tag_status = 0;
        }
    } else if (BCMPKT_RXPMD_FID_SUPPORT_GET(support, BCMPKT_RXPMD_ARC_ID_LO) &&
               BCMPKT_RXPMD_FID_SUPPORT_GET(support, BCMPKT_RXPMD_ARC_ID_HI)){
        /* Trident5 Family*/
        uint32_t match_id_data[2];
        bool itag = false, otag = false;

        bcmpkt_rxpmd_field_get(dev_type, rxpmd, BCMPKT_RXPMD_ARC_ID_LO,
                               &match_id_data[0]);
        bcmpkt_rxpmd_field_get(dev_type, rxpmd, BCMPKT_RXPMD_ARC_ID_HI,
                               &match_id_data[1]);
        rv = bcmpkt_rxpmd_match_id_from_arc_id_present(variant, match_id_data, 2,
                                           match_id.ingress_pkt_outer_l2_hdr_itag);
        if (SHR_SUCCESS(rv)) {
            itag = true;
        }
        rv = bcmpkt_rxpmd_match_id_from_arc_id_present(variant, match_id_data, 2,
                                           match_id.ingress_pkt_outer_l2_hdr_otag);
        if (SHR_SUCCESS(rv)) {
            otag = true;
        }
        if (itag && otag) {
            tag_status = 3;
        } else if (itag) {
            tag_status = 1;
        } else if (otag) {
            tag_status = 2;
        } else {
            tag_status = 0;
        }
    }
#ifdef KNET_CB_DEBUG
    if (debug & NGKNET_CB_DBG_LVL_VERB) {
        if (tag_status != -1) {
            if (tag_status == 0) {
                printk("  Incoming frame untagged\n");
            } else {
                printk("  Incoming frame tagged: %s\n", tag_type[tag_status]);
            }
        } else {
            printk("  Unsupported tag type\n");
        }
    }
#endif /* KNET_CB_DEBUG */
    return tag_status;
}

#ifdef KNET_CB_DEBUG
static void
dump_buffer(uint8_t * data, int size)
{
    const char         *const to_hex = "0123456789ABCDEF";
    int                 i;
    char                buffer[128];
    char               *buffer_ptr;
    int                 addr = 0;

    buffer_ptr = buffer;
    for (i = 0; i < size; i++) {
        *buffer_ptr++ = ' ';
        *buffer_ptr++ = to_hex[(data[i] >> 4) & 0xF];
        *buffer_ptr++ = to_hex[data[i] & 0xF];
        if (((i % 16) == 15) || (i == size - 1)) {
            *buffer_ptr = '\0';
            buffer_ptr = buffer;
            printk(KERN_INFO "%04X  %s\n", addr, buffer);
            addr = i + 1;
        }
    }
}

static void
show_pmd(uint8_t *pmd, int len)
{
    if (debug & 0x1) {
        printk("PMD (%d bytes):\n", len);
        dump_buffer(pmd, len);
    }
}

static void
show_mac(uint8_t *pkt)
{
    if (debug & 0x1) {
    printk("DMAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
           pkt[0], pkt[1], pkt[2], pkt[3], pkt[4], pkt[5]);
}
}
#endif

static struct sk_buff *
strip_tag_rx_cb(struct sk_buff *skb)
{
    const struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    int rcpu_mode = 0;
    int tag_status, unit;
    uint32_t dev_type = 0;
    bcmlrd_variant_t var_type;
    uint32_t *rxpmd = NULL;

    unit = cbd->dinfo->dev_no;
    rxpmd = (uint32_t *)cbd->pmd;
    rcpu_mode = (cbd->netif->flags & NGKNET_NETIF_F_RCPU_ENCAP)? 1 : 0;
#ifdef KNET_CB_DEBUG
    if (debug & 0x1)
    {
        printk(KERN_INFO
                "\n%4u --------------------------------------------------------------------------------\n",
                rx_count);
        printk(KERN_INFO
                "RX KNET callback: dev_no=%1d; dev_id=:%6s; type_str=%4s; RCPU: %3s \n",
                cbd->dinfo->dev_no, cbd->dinfo->var_str, cbd->dinfo->type_str, rcpu_mode ? "yes" : "no");
        printk(KERN_INFO "                  pkt_len=%4d; pmd_len=%2d; SKB len: %4d\n",
                cbd->pkt_len, cbd->pmd_len, skb->len);
        if (cbd->filt) {
            printk(KERN_INFO "Filter user data: 0x%08x\n",
                    *(uint32_t *) cbd->filt->user_data);
        }
        printk(KERN_INFO "Before SKB (%d bytes):\n", skb->len);
        dump_buffer(skb->data, skb->len);
        printk("rx_cb for dev %d: id %s, %s\n", cbd->dinfo->dev_no, cbd->dinfo->var_str, cbd->dinfo->type_str);
        printk("netif user data: 0x%08x\n", *(uint32_t *)cbd->netif->user_data);
        show_pmd(cbd->pmd, cbd->pmd_len);
        if (rcpu_mode) {
            const int           RCPU_header_len = PKT_HDR_SIZE + cbd->pmd_len;
            const int           payload_len = skb->len - RCPU_header_len;
            unsigned char      *payload_start = skb->data + payload_len;

            printk(KERN_INFO "Packet Payload (%d bytes):\n", payload_len);
            dump_buffer(payload_start, payload_len);
        } else {
            printk(KERN_INFO "Packet (%d bytes):\n", cbd->pkt_len);
            dump_buffer(skb->data, cbd->pkt_len);
        }
    }
#endif

    if ((!rcpu_mode) && (cbd->filt)) {
        if (cb_dev[unit].initialized) {
            dev_type = cb_dev[unit].dev_type;
            var_type = cb_dev[unit].var_type;
            if (FILTER_TAG_ORIGINAL == cbd->filt->user_data[0]) {
                tag_status = get_tag_status(dev_type, var_type,
                        (void *)rxpmd);
                if (tag_status < 0) {
                    strip_stats.skipped++;
                    goto _strip_tag_rx_cb_exit;
                }
                strip_stats.checked++;
                if (tag_status < 2) {
                    strip_stats.stripped++;
                    strip_vlan_tag(skb);
                }
            }
            if (FILTER_TAG_STRIP == cbd->filt->user_data[0]) {
                strip_stats.stripped++;
                strip_vlan_tag(skb);
            }
#ifdef KNET_CB_DEBUG
            if (debug & 0x1) {
                printk("ngknetcb_main for dev %d:", cbd->dinfo->dev_no);
                printk("type_str:%s dev_id: 0x%x variant: %s\n",
                        cbd->dinfo->type_str, cbd->dinfo->dev_id, variant_types[var_type].var_name);
                printk("dev_type: %d\n", dev_type);
                printk("variant: %d\n", var_type);

                print_all_rxpmd_fields(dev_type, (void *)rxpmd);
                printk("\n[RX reasons]:\n");
                print_all_rx_reason(dev_type, (void *)rxpmd);
            }
#endif
        }
    }
_strip_tag_rx_cb_exit:
#ifdef KNET_CB_DEBUG
    if (debug & 0x1) {
        printk(KERN_INFO "After SKB (%d bytes):\n", skb->len);
        dump_buffer(skb->data, skb->len);
        printk(KERN_INFO
               "\n%4u --------------------------------------------------------------------------------\n",
               rx_count++);
    }
#endif
    return skb;
}

static struct sk_buff *
strip_tag_tx_cb(struct sk_buff *skb)
{
#ifdef KNET_CB_DEBUG
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);

    if (debug & 0x1) {
        printk("tx_cb for dev %d: %s\n", cbd->dinfo->dev_no, cbd->dinfo->type_str);
    }
    show_pmd(cbd->pmd, cbd->pmd_len);
    show_mac(cbd->pmd + cbd->pmd_len);
#endif
    return skb;
}

static struct sk_buff *
ngknet_rx_cb(struct sk_buff *skb)
{
    skb = strip_tag_rx_cb(skb);
    return skb;
}

static struct sk_buff *
ngknet_tx_cb(struct sk_buff *skb)
{
    skb = strip_tag_tx_cb(skb);
    return skb;
}

/*!
 * Generic module functions
 */
static int
ngknetcb_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Broadcom Linux NGKNET Callback: Untagged VLAN Stripper\n");
    seq_printf(m, "    %lu stripped packets\n", strip_stats.stripped);
    seq_printf(m, "    %lu packets checked\n", strip_stats.checked);
    seq_printf(m, "    %lu packets skipped\n", strip_stats.skipped);
    return 0;
}

static int
ngknetcb_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, ngknetcb_show, NULL);
}

static int
ngknetcb_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t
ngknetcb_write(struct file *file, const char *buf,
               size_t count, loff_t *loff)
{
    memset(&strip_stats, 0, sizeof(strip_stats));
    printk("Cleared NGKNET callback stats\n");
    return count;
}

static long
ngknetcb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int
ngknetcb_mmap(struct file *filp, struct vm_area_struct *vma)
{
    return 0;
}

static struct file_operations ngknetcb_fops = {
    PROC_OWNER(THIS_MODULE)
    .open = ngknetcb_open,
    .read = seq_read,
    .write = ngknetcb_write,
    .llseek = seq_lseek,
    .release = ngknetcb_release,
    .unlocked_ioctl = ngknetcb_ioctl,
    .compat_ioctl = ngknetcb_ioctl,
    .mmap = ngknetcb_mmap,
};

/* Added this for PROC_CREATE */
static struct proc_ops ngknetcb_proc_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open = ngknetcb_open,
    .proc_read = seq_read,
    .proc_write = ngknetcb_write,
    .proc_lseek = seq_lseek,
    .proc_release = ngknetcb_release,
    .proc_ioctl = ngknetcb_ioctl,
    .proc_compat_ioctl = ngknetcb_ioctl,
    .proc_mmap = ngknetcb_mmap,
};

#ifdef KPMD
/*
  Initialize the desired match_ids for use later in the code.
 */
static void
init_match_ids(int unit)
{
    uint32_t val;

    match_id.egress_pkt_fwd_l2_hdr_etag  = -1;
    match_id.egress_pkt_fwd_l2_hdr_l2    = -1;
    match_id.ingress_pkt_inner_l2_hdr_l2 = -1;
    match_id.ingress_pkt_fwd_l2_hdr_etag = -1;
    match_id.ingress_pkt_outer_l2_hdr_itag = -1;
    match_id.ingress_pkt_outer_l2_hdr_otag = -1;
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "EGRESS_PKT_FWD_L2_HDR_ETAG", &val) == 0) {
        match_id.egress_pkt_fwd_l2_hdr_etag = val;
        printk("EGRESS_PKT_FWD_L2_HDR_ETAG: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "EGRESS_PKT_FWD_L2_HDR_L2", &val) == 0) {
        match_id.egress_pkt_fwd_l2_hdr_l2 = val;
        printk("EGRESS_PKT_FWD_L2_HDR_L2: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_INNER_L2_HDR_L2", &val) == 0) {
        match_id.ingress_pkt_inner_l2_hdr_l2 = val;
        printk("INGRESS_PKT_INNER_L2_HDR_L2: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_FWD_L2_HDR_ETAG", &val) == 0) {
        match_id.ingress_pkt_fwd_l2_hdr_etag = val;
        printk("INGRESS_PKT_FWD_L2_HDR_ETAG: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_OUTER_L2_HDR_ITAG", &val) == 0) {
        match_id.ingress_pkt_outer_l2_hdr_itag = val;
        printk("INGRESS_PKT_OUTER_L2_HDR_ITAG: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_OUTER_L2_HDR_OTAG", &val) == 0) {
        match_id.ingress_pkt_outer_l2_hdr_otag = val;
        printk("INGRESS_PKT_OUTER_L2_HDR_OTAG: %d\n", val);
    }
}
#endif
/*!
 * \brief Device Initialization Callback.
 *
 * The device initialization callback allows an external module to
 * perform device-specific initialization in preparation for Tx and Rx
 * packet processing.
 *
 * \param [in] dinfo Device information.
 *
 */
static void
init_cb(ngknet_dev_info_t *dinfo)
{
    int unit;
    bcmdrd_dev_type_t dt;
    bcmlrd_variant_t var;
    unit = dinfo->dev_no;

    if ((unsigned int)unit >= NUM_PDMA_DEV_MAX) {
        return;
    }
    /* Update dev_type and variant type*/
    for (dt = 0; dt < BCMDRD_DEV_T_COUNT; dt++) {
        if (!strcasecmp(dinfo->type_str, device_types[dt].name)) {
            cb_dev[unit].dev_type = dt;
            break;
        }
    }

    for (var = 0; var < BCMLRD_VARIANT_T_COUNT; var++) {
        if ((!strcasecmp(dinfo->type_str, variant_types[var].dev_name)) &&
            (!strcasecmp(dinfo->var_str, variant_types[var].var_name))) {
            cb_dev[unit].var_type = var;
            break;
        }
    }
#ifdef KNET_CB_DEBUG
    if (debug & 1) {
        printk("init_cb unit %d, dev %s dev_id: 0x%x variant %s\n",
               dinfo->dev_no, dinfo->type_str, dinfo->dev_id, dinfo->var_str);
        printk("dev_type: %d\n", cb_dev[unit].dev_type);
        printk("variant: %d\n", cb_dev[unit].var_type);
    }
#endif /* KNET_CB_DEBUG */
    cb_dev[unit].initialized = true;
#ifdef KPMD
    init_match_ids(unit);
#endif /* KPMD */
}

static int __init
ngknetcb_init_module(void)
{
    int rv;
    struct proc_dir_entry *entry = NULL; 

    rv = register_chrdev(NGKNETCB_MODULE_MAJOR, NGKNETCB_MODULE_NAME, &ngknetcb_fops);
    if (rv < 0) {
        printk(KERN_WARNING "%s: can't get major %d\n",
               NGKNETCB_MODULE_NAME, NGKNETCB_MODULE_MAJOR);
        return rv;
    }

    PROC_CREATE(entry, NGKNETCB_MODULE_NAME, 0666, NULL, &ngknetcb_proc_ops);
    if (entry == NULL) {
        printk(KERN_ERR "%s: proc_mkdir failed\n",
                NGKNETCB_MODULE_NAME);
    }
    ngknet_dev_init_cb_register(init_cb);
    ngknet_rx_cb_register(ngknet_rx_cb);
    ngknet_tx_cb_register(ngknet_tx_cb);

    return 0;
}

static void __exit
ngknetcb_exit_module(void)
{
    ngknet_dev_init_cb_unregister(init_cb);
    ngknet_rx_cb_unregister(ngknet_rx_cb);
    ngknet_tx_cb_unregister(ngknet_tx_cb);

    remove_proc_entry(NGKNETCB_MODULE_NAME, NULL);

    unregister_chrdev(NGKNETCB_MODULE_MAJOR, NGKNETCB_MODULE_NAME);
}

module_init(ngknetcb_init_module);
module_exit(ngknetcb_exit_module);
