/*! \file bcmgenl.c
 *
 * BCMGENL module entry.
 *
 */
/*
 * $Copyright: Copyright 2018-2023 Broadcom. All rights reserved.
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
 * be found in the LICENSES folder.$
 */

#include <lkm/lkm.h>
#include <lkm/ngknet_kapi.h>
#include <ngknet_linux.h>
#include <bcmgenl.h>

#include <bcmgenl_packet.h>
#include <bcmgenl_psample.h>

#ifdef KPMD
#include <bcmpkt/bcmpkt_flexhdr_internal.h>
#include <bcmpkt/bcmpkt_flexhdr_field.h>
#include <bcmpkt/bcmpkt_higig_defs.h>
#include <bcmpkt/bcmpkt_lbhdr_field.h>
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_rxpmd_fid.h>
#include <bcmpkt/bcmpkt_rxpmd_field.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id.h>
#include <bcmpkt/bcmpkt_txpmd_field.h>
#endif /* KPMD */

#include <shr/shr_error.h>

/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("BCMGENL Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! driver proc entry root */
static struct proc_dir_entry *bcmgenl_proc_root = NULL;

/*! set BCMGENL_DEBUG for debug info */
#define BCMGENL_DEBUG

#ifdef BCMGENL_DEBUG
#define DBG_LVL_VERB    0x1
#define DBG_LVL_PDMP    0x2

/*! \cond */
static int debug = 0;
MODULE_PARAM(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug level (default 0)");
/*! \endcond */
#endif /* BCMGENL_DEBUG */

/*! These below need to match incoming enum values */
#define FILTER_TAG_STRIP 0
#define FILTER_TAG_KEEP  1
#define FILTER_TAG_ORIGINAL 2

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

void dump_skb(struct sk_buff *skb)
{
    int idx;
    char str[128];
    uint8_t *data = skb->data;

    printk(KERN_INFO " SKB len: %4d\n", skb->len);
    for (idx = 0; idx < skb->len; idx++) {
        if ((idx & 0xf) == 0) {
            printk(str, "%04x: ", idx);
        }
        if ((idx & 0xf) == 8) {
            printk(&str[strlen(str)], "- ");
        }
        sprintf(&str[strlen(str)], "%02x ", data[idx]);
        if ((idx & 0xf) == 0xf) {
            sprintf(&str[strlen(str)], "\n");
            printk("%s", str);
        }
    }
    if ((idx & 0xf) != 0) {
        sprintf(&str[strlen(str)], "\n");
        printk("%s", str);
    }
}

#ifdef BCMGENL_DEBUG
static void
dump_buffer(uint8_t * data, int size)
{
    const char         *const to_hex = "0123456789ABCDEF";
    int i;
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
dump_pmd(uint8_t *pmd, int len)
{
    if (debug & DBG_LVL_PDMP) {
        printk("PMD (%d bytes):\n", len);
        dump_buffer(pmd, len);
    }
}

void dump_bcmgenl_pkt(bcmgenl_pkt_t *bcmgenl_pkt)
{
    printk(KERN_INFO "Network namespace 0x%p\n",
           bcmgenl_pkt->netns);
    printk(KERN_INFO "ing_pp_port %d src_port %d, dst_port %d, dst_port_type %x\n",
           bcmgenl_pkt->meta.ing_pp_port,
           bcmgenl_pkt->meta.src_port,
           bcmgenl_pkt->meta.dst_port,
           bcmgenl_pkt->meta.dst_port_type);
    printk(KERN_INFO "tag status %d",
           bcmgenl_pkt->meta.tag_status);
    printk(KERN_INFO "proto 0x%x, vlan 0x%x\n",
           bcmgenl_pkt->meta.proto,
           bcmgenl_pkt->meta.vlan);
    printk(KERN_INFO "sample_rate %d, sample_size %d\n",
           bcmgenl_pkt->psamp_meta.sample_rate,
           bcmgenl_pkt->psamp_meta.sample_size);
}
#endif /* BCMGENL_DEBUG */

/*
 * The function get_tag_status() returns the tag status.
 * 0  = Untagged
 * 1  = Single inner-tag
 * 2  = Single outer-tag
 * 3  = Double tagged.
 * -1 = Unsupported type
 */
static int
get_tag_status(uint32_t dev_type, uint32_t variant, void *meta)
{
    uint32_t  *valptr;
    uint32_t  fd_index;
    uint32_t  outer_l2_hdr = 0;
    int tag_status = -1;
    uint32_t  match_id_minbit = 1;
    uint32_t  outer_tag_match = 0x10;

    if ((dev_type == 0xb880) || (dev_type == 0xb780)) {
        /* Field BCM_PKTIO_RXPMD_MATCH_ID_LO has tag status in RX PMD */
        fd_index = 2;
        valptr = (uint32_t *)meta;
        match_id_minbit = (dev_type == 0xb780) ? 2 : 1;
        outer_l2_hdr = (valptr[fd_index] >> match_id_minbit & 0xFF);
        outer_tag_match = ((dev_type == 0xb780 && variant == 1) ? 0x8 : 0x10);
        if (outer_l2_hdr & 0x1) {
            tag_status = 0;
            if (outer_l2_hdr & 0x4) {
                tag_status = 0;
            }
            if (outer_l2_hdr & outer_tag_match) {
                tag_status = 2;
                if (outer_l2_hdr & 0x20) {
                    tag_status = 3;
                }
            }
            else if (outer_l2_hdr & 0x20) {
                tag_status = 1;
            }
        }
    }
    else if ((dev_type == 0xb990)|| (dev_type == 0xb996)) {
        fd_index = 9;
        valptr = (uint32_t *)meta;
        /* On TH4, outer_l2_header means INCOMING_TAG_STATUS.
         * TH4 only supports single tagging, so if TAG_STATUS
         * says there's a tag, then we don't want to strip.
         * Otherwise, we do.
         */
        outer_l2_hdr = (valptr[fd_index] >> 13) & 3;

        if (outer_l2_hdr) {
            tag_status = 2;
        } else {
            tag_status = 0;
        }
    }
#ifdef BCMGENL_DEBUG
    if (debug & DBG_LVL_VERB) {
        if (outer_l2_hdr) {
            printk("  L2 Header Present\n");
            if (tag_status == 0) {
                printk("  Incoming frame untagged\n");
            } else {
                printk("  Incoming frame tagged\n");
            }
            switch (tag_status) {
            case 0:
                printk("  SNAP/LLC\n");
                break;
            case 1:
                printk("  Inner Tagged\n");
                break;
            case 2:
                printk("  Outer Tagged\n");
                break;
            case 3:
                printk("  Double Tagged\n");
                break;
            default:
                break;
            }
        }
        printk("%s; Device Type: %d; tag status: %d\n", __func__, dev_type, tag_status);
    }
#endif /* BCMGENL_DEBUG */
    return tag_status;
}

static int
dstport_get(void *pkt_meta)
{
    int dstport = 0;
    HIGIG_t *hg = (HIGIG_t *)pkt_meta;
    HIGIG2_t *hg2 = (HIGIG2_t *)pkt_meta;

    if (HIGIG2_STARTf_GET(*hg2) == BCMPKT_HIGIG2_SOF)
    {
        if (HIGIG2_MCSTf_GET(*hg2))
        {
            dstport = 0;
        }
        else
        {
            dstport = HIGIG2_DST_PORT_MGIDLf_GET(*hg2);
        }
    }
    else if (HIGIG_STARTf_GET(*hg) == BCMPKT_HIGIG_SOF)
    {
        dstport = HIGIG_DST_PORTf_GET(*hg);
    }
#ifdef BCMGENL_DEBUG
    else
    {
        /* SDKLT-43751: Failed to parse dstport on TD4/TH4 */
        if (debug & DBG_LVL_VERB) {
            printk("%s: Could not detect metadata sop type: 0x%02x\n",
                   __func__, HIGIG_STARTf_GET(*hg));
            return (-1);
        }
    }
#endif /* BCMGENL_DEBUG */
    return dstport;
}

static bool
dstport_type_get(void *pkt_meta)
{
    HIGIG2_t *hg2 = (HIGIG2_t *)pkt_meta;

    if (HIGIG2_STARTf_GET(*hg2) == BCMPKT_HIGIG2_SOF)
    {
        if (HIGIG2_MCSTf_GET(*hg2))
        {
            return DSTPORT_TYPE_MC;
        }
    }
    return DSTPORT_TYPE_NONE;
}

int
bcmgenl_pkt_package(
    int dev,
    struct sk_buff *skb,
    bcmgenl_info_t *bcmgenl_info,
    bcmgenl_pkt_t *bcmgenl_pkt)
{
    int unit, rv, rv2;
    struct ngknet_callback_desc *cbd;
    uint8_t *pkt;
    uint32_t rxpmd[BCMPKT_RXPMD_SIZE_WORDS];
    uint32_t dev_type = 0;
    bcmlrd_variant_t var_type;
    uint32_t *rxpmd_flex = NULL;
    uint32_t rxpmd_flex_len = 0;
    uint32_t hid, val = 0;
    int fid;

    if (!skb || !bcmgenl_info || !bcmgenl_pkt) {
        return SHR_E_PARAM;
    }
    cbd = NGKNET_SKB_CB(skb);
    unit = cbd->dinfo->dev_no;
    pkt = cbd->pmd + cbd->pmd_len;

    bcmgenl_pkt->meta.proto = (uint16_t) ((pkt[12] << 8) | pkt[13]);
    bcmgenl_pkt->meta.vlan = (uint16_t) ((pkt[14] << 8) | pkt[15]);

    bcmgenl_pkt->netns = bcmgenl_info->netns;

    if (cb_dev[unit].initialized) {
#ifdef KPMD
        dev_type = cb_dev[unit].dev_type;
        var_type = cb_dev[unit].var_type;
        /* Get tag status */
        bcmgenl_pkt->meta.tag_status = get_tag_status(dev_type, var_type,(void *)cbd->pmd);

        /* Get dst port */
        bcmgenl_pkt->meta.dst_port = dstport_get((void *)cbd->pmd);
        bcmgenl_pkt->meta.dst_port_type = dstport_type_get((void *)cbd->pmd);

        /* Get src port */
        rv = bcmpkt_rxpmd_field_get(dev_type,
            (uint32_t *)cbd->pmd, BCMPKT_RXPMD_SRC_PORT_NUM, &val);
        if (SHR_SUCCESS(rv)) {
            bcmgenl_pkt->meta.src_port = val;
        }
        rv = bcmpkt_rxpmd_flexdata_get
            (dev_type, rxpmd, &rxpmd_flex, &rxpmd_flex_len);
        if (SHR_FAILURE(rv) && (rv != SHR_E_UNAVAIL)) {
            printk("Failed to detect RXPMD_FLEX.\n");
        } else {
            if (rxpmd_flex_len) {
                /* Get hid of RXPMD_FLEX_T */
                if (bcmpkt_flexhdr_header_id_get(var_type,
                                                 "RXPMD_FLEX_T", &hid)) {
                    rv = SHR_E_UNAVAIL;
                }

                /* Get fid of INGRESS_PP_PORT_7_0 */
                if (SHR_FAILURE(rv) ||
                    bcmpkt_flexhdr_field_id_get(var_type, hid,
                                                "INGRESS_PP_PORT_7_0",
                                                &fid) ||
                    bcmpkt_flexhdr_field_get(var_type, hid,
                                             rxpmd_flex,
                                             BCMPKT_FLEXHDR_PROFILE_NONE,
                                             fid, &val)) {
                    rv2 = SHR_E_UNAVAIL;
                }
                if (SHR_SUCCESS(rv) || SHR_SUCCESS(rv2)) {
                    bcmgenl_pkt->meta.ing_pp_port = val;
                }

                /* Get fid of ING_TIMESTAMP_31_0 */
                if (SHR_FAILURE(rv) ||
                    bcmpkt_flexhdr_field_id_get(var_type, hid,
                                                "ING_TIMESTAMP_31_0",
                                                &fid) ||
                    bcmpkt_flexhdr_field_get(var_type, hid,
                                             rxpmd_flex,
                                             BCMPKT_FLEXHDR_PROFILE_NONE,
                                             fid, &val)) {
                    rv2 = SHR_E_UNAVAIL;
                }
                if (SHR_SUCCESS(rv) || SHR_SUCCESS(rv2)) {
                    bcmgenl_pkt->meta.timestamp = val;
                }
            }
        }
#endif /* KPMD */
    }
#ifdef BCMGENL_DEBUG
    if (debug & DBG_LVL_PDMP) {
        if (cb_dev[unit].initialized) {
            printk("bcmgenl_pkt_package for dev %d: %s variant %s\n",
                   cbd->dinfo->dev_no, cbd->dinfo->type_str,
                   variant_types[cb_dev[unit].var_type].var_name);
            printk("dev_type: %d\n", cb_dev[unit].dev_type);
            printk("variant: %d\n\n", cb_dev[unit].var_type);

            if (cbd->pmd_len != 0) {
                dump_pmd(cbd->pmd, cbd->pmd_len);
            }
            printk("Packet raw data (%d):", cbd->pkt_len);
            dump_buffer(pkt, cbd->pkt_len);
        }
        dump_bcmgenl_pkt(bcmgenl_pkt);
    }
#endif /* BCMGENL_DEBUG */
    return SHR_E_NONE;
}

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
} cb_match_id_t;

static cb_match_id_t match_id;

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
}
#endif /* KPMD */

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

    printk("init_cb unit %d, dev %s variant %s\n",
           dinfo->dev_no, dinfo->type_str, dinfo->var_str);
    printk("dev_type: %d\n", cb_dev[unit].dev_type);
    printk("variant: %d\n", cb_dev[unit].var_type);

    cb_dev[unit].initialized = true;

#ifdef KPMD
    init_match_ids(unit);
#endif /* KPMD */
}

static int
bcmgenl_proc_cleanup(void)
{
    remove_proc_entry(BCMGENL_PROCFS_PATH, NULL);
    remove_proc_entry(BCM_PROCFS_NAME, NULL);
    return 0;
}

static int
bcmgenl_proc_init(void)
{
    /* initialize proc files (for bcmgenl) */
    proc_mkdir(BCM_PROCFS_NAME, NULL);
    bcmgenl_proc_root = proc_mkdir(BCMGENL_PROCFS_PATH, NULL);
    return 0;
}

static int __init
bcmgenl_init_module(void)
{
    ngknet_dev_init_cb_register(init_cb);

    bcmgenl_proc_init();
#ifdef PACKET_SUPPORT
    bcmgenl_packet_init();
#endif
#ifdef PSAMPLE_SUPPORT
    bcmgenl_psample_init();
#endif
    return 0;
}

static void __exit
bcmgenl_exit_module(void)
{
    ngknet_dev_init_cb_unregister(init_cb);

#ifdef PACKET_SUPPORT
    bcmgenl_packet_cleanup();
#endif
#ifdef PSAMPLE_SUPPORT
    bcmgenl_psample_cleanup();
#endif
    bcmgenl_proc_cleanup();
}

module_init(bcmgenl_init_module);
module_exit(bcmgenl_exit_module);

