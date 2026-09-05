/*! \file bcmgenl.c
 *
 * BCMGENL module entry.
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
#include <bcmgenl.h>
#include <bcmgenl_packet.h>
#include <bcmgenl_psample.h>

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

#include <shr/shr_error.h>

/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("BCMGENL Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! driver proc entry root */
static struct proc_dir_entry *bcmgenl_proc_root = NULL;

#ifdef GENL_DEBUG
/*! \cond */
static int debug = 0;
MODULE_PARAM(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug level (default 0)");
/*! \endcond */
#endif /* GENL_DEBUG */

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
print_all_rxpmd_flex_fields(
    bcmdrd_dev_type_t dev_type,
    bcmlrd_variant_t var_type,
    const uint8_t *rxpmd,
    uint32_t *rxpmd_flex)
{
    int rv, fid;
    int flex_profile = -1;
    bcmpkt_flex_field_info_t rxpmd_flex_info;
    uint32_t hid, val;

    rv = bcmpkt_rxpmd_field_get(dev_type, (uint32_t *)rxpmd,
                                BCMPKT_RXPMD_MPB_FLEX_DATA_TYPE, &val);
    if (rv < 0) {
        return;
    }
    flex_profile = (int)val;

    rv = bcmpkt_flexhdr_header_id_get(var_type, "RXPMD_FLEX_T", &hid);
    if (rv < 0) {
        return;
    }

    rv = bcmpkt_flexhdr_field_info_get(var_type, hid, &rxpmd_flex_info);
    if (rv < 0) {
        return;
    }

    printk("\n[RX metadata flex information]:\n");
    for (fid = BCMPKT_FID_INVALID + 1; fid < rxpmd_flex_info.num_fields; fid++) {
        rv = bcmpkt_flexhdr_field_get(var_type, hid, rxpmd_flex, flex_profile, fid, &val);
        if (rv == 0 && val != 0) {
            printk("  %-34s = %10d [0x%X]\n", rxpmd_flex_info.info[fid].name, val, val);
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

static void
print_all_rx_flex_reason(bcmlrd_variant_t variant, uint32_t *rxpmd_flex)
{
    int reason, reason_num = 0, rv;
    bcmpkt_bitmap_t reasons;
    char *name;
    uint32_t val;

    if (rxpmd_flex == NULL) {
        return;
    }

    rv = bcmpkt_rxpmd_flex_reason_max_get(variant, &val);
    if (rv < 0) {
        return;
    }
    reason_num = (int)val;

    rv = bcmpkt_rxpmd_flex_reasons_get(variant, rxpmd_flex, &reasons);
    if (rv == 0) {
        for (reason = 0; reason < reason_num; reason++) {
            if (BCMPKT_RXPMD_FLEX_REASON_GET(reasons, reason)) {
                rv = bcmpkt_rxpmd_flex_reason_name_get(variant, reason, &name);
                if (!rv) {
                    printk("  %s\n", name);
                }
            }
        }
    }
}

#endif /* KPMD */

#ifdef GENL_DEBUG
static void
dump_buffer(uint8_t *data, int size)
{
    const char         *const to_hex = "0123456789ABCDEF";
    int i;
    char                buffer[128];
    char               *buffer_ptr;
    int                 addr = 0;

    buffer_ptr = buffer;
    if (data && size != 0) {
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
}

static void
dump_pmd(uint8_t *pmd, int len)
{
    if (debug & GENL_DBG_LVL_PDMP) {
        printk(KERN_INFO "[PMD (%d bytes)]:\n", len);
        dump_buffer(pmd, len);
    }
}

void dump_skb(struct sk_buff *skb)
{
    if (skb && (skb->len != 0)) {
        printk(KERN_INFO "[SKB (%d bytes)]:\n", skb->len);
        dump_buffer(skb->data, skb->len);
    }
}

void dump_bcmgenl_pkt(bcmgenl_pkt_t *bcmgenl_pkt)
{
    printk(KERN_INFO"  %-20s = 0x%p\n", "Network namespace", bcmgenl_pkt->netns);
    printk(KERN_INFO"  %-20s = %d\n", "ing_pp_port", bcmgenl_pkt->meta.ing_pp_port);
    printk(KERN_INFO"  %-20s = %d\n", "src_port", bcmgenl_pkt->meta.src_port);
    printk(KERN_INFO"  %-20s = %d\n", "dst_port", bcmgenl_pkt->meta.dst_port);
    printk(KERN_INFO"  %-20s = %d\n", "dst_port_type", bcmgenl_pkt->meta.dst_port_type);
    printk(KERN_INFO"  %-20s = %d\n", "tag_status", bcmgenl_pkt->meta.tag_status);
    printk(KERN_INFO"  %-20s = 0x%x\n", "proto", bcmgenl_pkt->meta.proto);
    printk(KERN_INFO"  %-20s = %d\n", "vlan", bcmgenl_pkt->meta.vlan);
    printk(KERN_INFO"  %-20s = %s\n", "sample_type",
          (bcmgenl_pkt->meta.sample_type == SAMPLE_TYPE_NONE ? "Not sampled" :
           (bcmgenl_pkt->meta.sample_type == SAMPLE_TYPE_INGRESS ?
           "Ingress sampled" : "Egress sampled")));
}
#endif /* GENL_DEBUG */

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
            tag_status = val;
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
    }
#ifdef GENL_DEBUG
    if (debug & GENL_DBG_LVL_VERB) {
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
#endif /* GENL_DEBUG */
    return tag_status;
}

static int
dstport_get(void *raw_hg_hdr)
{
    /*
     * The bit positions of dest port field is fixed on TH4/TH5.
     * directly use HIGIG2_DST_MODID_MGIDH & HIGIG2_DST_PORT_MGIDL to
     * get dest port.
     */
    int dstport = 0;
    const HIGIG2_t *const higig2 = (HIGIG2_t *)raw_hg_hdr;

    if (HIGIG2_MCSTf_GET(*higig2)) {
        dstport = 0;
    } else {
        dstport = (HIGIG2_DST_MODID_MGIDHf_GET(*higig2) << 8) |
                  HIGIG2_DST_PORT_MGIDLf_GET(*higig2);
    }
    return dstport;
}

static int
dstport_type_get(void *raw_hg_hdr)
{
    /*
     * The bit positions of multicast field is fixed on TH4/TH5.
     * directly use HIGIG2_MCSTf_GET to get dest port.
     */
    const HIGIG2_t *const higig2 = (HIGIG2_t *)raw_hg_hdr;

    if (HIGIG2_MCSTf_GET(*higig2)) {
        return DSTPORT_TYPE_MC;
    }
    return DSTPORT_TYPE_NONE;
}

static bool
is_cpu_port(uint32_t dev_id, uint32_t port)
{
    if (((dev_id == 0xb880) && (port == 160)) ||
        ((dev_id == 0xb780) && (port == 80)) ||
        ((dev_id == 0xb690) && (port == 80)) ||
        ((dev_id == 0xb890) && (port == 272)) ||
        ((dev_id == 0xf800) && (port == 176))) {
        /*
         * SYSTEM_DESTINATION_15_0 = 0 is reserved and not used for CPU port on
         * Trident 4/5 families.
         * e.g TD4X11 map system port of CPU to {modid : 160}
         */
        return true;
    }
    return false;
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
    uint32_t dev_type = 0;
    bcmlrd_variant_t var_type;
    uint32_t *rxpmd = NULL;
    uint32_t *rxpmd_flex = NULL;
    uint32_t rxpmd_flex_len = 0;
    uint32_t hid, val = 0;
    int flex_profile = -1;
    int fid;
    uint32_t *mh = NULL;
    int reason, reason_num = 0;
    bcmpkt_bitmap_t reasons;
    bcmpkt_rx_reasons_t rx_reasons;
    char *name;

    if (!skb || !bcmgenl_info || !bcmgenl_pkt) {
        return SHR_E_PARAM;
    }
    cbd = NGKNET_SKB_CB(skb);
    unit = cbd->dinfo->dev_no;
    pkt = cbd->pmd + cbd->pmd_len;
    rxpmd = (uint32_t *)cbd->pmd;

    memset(&bcmgenl_pkt->meta, 0, sizeof(bcmgenl_packet_meta_t));

    bcmgenl_pkt->meta.proto = (uint16_t) ((pkt[12] << 8) | pkt[13]);
    bcmgenl_pkt->meta.vlan = (uint16_t) ((pkt[14] << 8) | pkt[15]);

    bcmgenl_pkt->netns = bcmgenl_info->netns;

    if (cb_dev[unit].initialized) {
#ifdef KPMD
        dev_type = cb_dev[unit].dev_type;
        var_type = cb_dev[unit].var_type;

        /* Get tag status */
        bcmgenl_pkt->meta.tag_status = get_tag_status(dev_type, var_type, (void *)rxpmd);

        /* Get sampling reason */
        BCMPKT_RX_REASON_CLEAR_ALL(reasons);
        rv = bcmpkt_rxpmd_reasons_get(dev_type, rxpmd, &rx_reasons);
        bcmgenl_pkt->meta.sample_type = SAMPLE_TYPE_NONE;
        if (SHR_SUCCESS(rv)) {
            if ((BCMPKT_RX_REASON_GET(rx_reasons, BCMPKT_RX_REASON_CPU_SFLOW_CPU_SFLOW_SRC)) ||
                (BCMPKT_RX_REASON_GET(rx_reasons, BCMPKT_RX_REASON_CPU_SFLOW_SRC))){
                bcmgenl_pkt->meta.sample_type = SAMPLE_TYPE_INGRESS;
            } else if ((BCMPKT_RX_REASON_GET(rx_reasons, BCMPKT_RX_REASON_CPU_SFLOW_CPU_SFLOW_DST)) ||
                       (BCMPKT_RX_REASON_GET(rx_reasons, BCMPKT_RX_REASON_CPU_SFLOW_DST))) {
                bcmgenl_pkt->meta.sample_type = SAMPLE_TYPE_EGRESS;
            }
        }

        /* Get Module header's pointer */
        rv = bcmpkt_rxpmd_mh_get(dev_type, rxpmd, &mh);
        if (SHR_SUCCESS(rv)) {
            /* Get dst_port and dst_port_type */
            bcmgenl_pkt->meta.dst_port = dstport_get((void *)mh);
            bcmgenl_pkt->meta.dst_port_type = dstport_type_get((void *)mh);
        }

        /* Get src port */
        rv = bcmpkt_rxpmd_field_get
            (dev_type, rxpmd, BCMPKT_RXPMD_SRC_PORT_NUM, &val);
        if (SHR_SUCCESS(rv)) {
            bcmgenl_pkt->meta.src_port = val;
        }
        rv = bcmpkt_rxpmd_flexdata_get
            (dev_type, rxpmd, &rxpmd_flex, &rxpmd_flex_len);
        if (SHR_FAILURE(rv) && (rv != SHR_E_UNAVAIL)) {
            GENL_DBG_VERB("Failed to detect RXPMD_FLEX.\n");
        } else {
            if (rxpmd_flex_len) {
                /* Get sampling reason from flex reasons */
                rv = bcmpkt_rxpmd_flex_reason_max_get(var_type, &val);
                rv2 = bcmpkt_rxpmd_flex_reasons_get(var_type, rxpmd_flex, &reasons);
                if (SHR_SUCCESS(rv) || SHR_SUCCESS(rv2)) {
                    bcmgenl_pkt->meta.sample_type = SAMPLE_TYPE_NONE;
                    reason_num = (int)val;
                    for (reason = 0; reason < reason_num; reason++) {
                        if (BCMPKT_RXPMD_FLEX_REASON_GET(reasons, reason)) {
                            rv = bcmpkt_rxpmd_flex_reason_name_get(var_type, reason, &name);
                            if (SHR_SUCCESS(rv)) {
                                if (strcmp(name, "MIRROR_SAMPLER_SAMPLED") == 0) {
                                    bcmgenl_pkt->meta.sample_type = SAMPLE_TYPE_INGRESS;
                                    break;
                                } else if (strcmp(name, "MIRROR_SAMPLER_EGR_SAMPLED") == 0) {
                                    bcmgenl_pkt->meta.sample_type = SAMPLE_TYPE_EGRESS;
                                    break;
                                }
                            }
                        }
                    }
                }

                /* Get hid of RXPMD_FLEX_T */
                if (bcmpkt_flexhdr_header_id_get(var_type,
                                                 "RXPMD_FLEX_T", &hid)) {
                    rv = SHR_E_UNAVAIL;
                }

                if (SHR_FAILURE(rv) ||
                    bcmpkt_rxpmd_field_get(dev_type, (uint32_t *)rxpmd,
                                           BCMPKT_RXPMD_MPB_FLEX_DATA_TYPE, &val)) {
                    rv = SHR_E_UNAVAIL;
                }
                flex_profile = (int)val;

                rv2 = SHR_E_NONE;
                /* Get fid of INGRESS_PP_PORT_7_0 */
                if (SHR_FAILURE(rv) ||
                    bcmpkt_flexhdr_field_id_get(var_type, hid,
                                                "INGRESS_PP_PORT_7_0",
                                                &fid) ||
                    bcmpkt_flexhdr_field_get(var_type, hid,
                                             rxpmd_flex,
                                             flex_profile,
                                             fid, &val)) {
                    rv2 = SHR_E_UNAVAIL;
                }
                if (SHR_SUCCESS(rv) || SHR_SUCCESS(rv2)) {
                    bcmgenl_pkt->meta.ing_pp_port = val;
                }

                /* Get dst_port and dst_port_type */
                rv2 = bcmpkt_rxpmd_field_get
                    (dev_type, rxpmd, BCMPKT_RXPMD_MULTICAST, &val);
                if (SHR_SUCCESS(rv2)) {
                    bcmgenl_pkt->meta.dst_port_type =
                        (val == 1 ? DSTPORT_TYPE_MC : DSTPORT_TYPE_NONE);
                }
                if (bcmgenl_pkt->meta.dst_port_type == DSTPORT_TYPE_MC) {
                    bcmgenl_pkt->meta.dst_port = 0;
                } else {
                    rv2 = SHR_E_NONE;
                    /* Get fid of SYSTEM_DESTINATION_15_0 */
                    if (SHR_FAILURE(rv) ||
                        bcmpkt_flexhdr_field_id_get(var_type, hid,
                                                    "SYSTEM_DESTINATION_15_0",
                                                    &fid) ||
                        bcmpkt_flexhdr_field_get(var_type, hid,
                                                 rxpmd_flex,
                                                 flex_profile,
                                                 fid, &val)) {
                        rv2 = SHR_E_UNAVAIL;
                    }
                    if (SHR_SUCCESS(rv) || SHR_SUCCESS(rv2)) {
                        if (is_cpu_port(cbd->dinfo->dev_id, val)) {
                            val = 0;
                        }
                        bcmgenl_pkt->meta.dst_port = val;
                    }
                }

                rv2 = SHR_E_NONE;
                /* Get fid of ING_TIMESTAMP_31_0 */
                if (SHR_FAILURE(rv) ||
                    bcmpkt_flexhdr_field_id_get(var_type, hid,
                                                "ING_TIMESTAMP_31_0",
                                                &fid) ||
                    bcmpkt_flexhdr_field_get(var_type, hid,
                                             rxpmd_flex,
                                             flex_profile,
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
#ifdef GENL_DEBUG
    if (debug & GENL_DBG_LVL_PDMP) {
        if (cb_dev[unit].initialized) {
            printk("bcmgenl_pkt_package for dev %d:", cbd->dinfo->dev_no);
            printk("type_str:%s dev_id: 0x%x variant: %s\n",
                   cbd->dinfo->type_str, cbd->dinfo->dev_id, variant_types[var_type].var_name);
            printk("dev_type: %d\n", dev_type);
            printk("variant: %d\n", var_type);

            print_all_rxpmd_fields(dev_type, (void *)rxpmd);
            if (rxpmd_flex_len) {
                print_all_rxpmd_flex_fields(dev_type, var_type, (void *)rxpmd, rxpmd_flex);
                printk("\n[RX flex reasons]:\n");
                print_all_rx_flex_reason(var_type, rxpmd_flex);
            } else {
                printk("\n[RX reasons]:\n");
                print_all_rx_reason(dev_type, (void *)rxpmd);
            }

            if (cbd->pmd_len != 0) {
                dump_pmd(cbd->pmd, cbd->pmd_len);
            }
            printk("\n[Packet raw data (%d)]:\n", cbd->pkt_len);
            dump_buffer(pkt, cbd->pkt_len);
        }
        dump_bcmgenl_pkt(bcmgenl_pkt);
    }
#endif /* GENL_DEBUG */
    return SHR_E_NONE;
}

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
        GENL_DBG_VERB("EGRESS_PKT_FWD_L2_HDR_ETAG: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "EGRESS_PKT_FWD_L2_HDR_L2", &val) == 0) {
        match_id.egress_pkt_fwd_l2_hdr_l2 = val;
        GENL_DBG_VERB("EGRESS_PKT_FWD_L2_HDR_L2: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_INNER_L2_HDR_L2", &val) == 0) {
        match_id.ingress_pkt_inner_l2_hdr_l2 = val;
        GENL_DBG_VERB("INGRESS_PKT_INNER_L2_HDR_L2: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_FWD_L2_HDR_ETAG", &val) == 0) {
        match_id.ingress_pkt_fwd_l2_hdr_etag = val;
        GENL_DBG_VERB("INGRESS_PKT_FWD_L2_HDR_ETAG: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_OUTER_L2_HDR_ITAG", &val) == 0) {
        match_id.ingress_pkt_outer_l2_hdr_itag = val;
        GENL_DBG_VERB("INGRESS_PKT_OUTER_L2_HDR_ITAG: %d\n", val);
    }
    if (bcmpkt_rxpmd_match_id_get(cb_dev[unit].var_type,
                                  "INGRESS_PKT_OUTER_L2_HDR_OTAG", &val) == 0) {
        match_id.ingress_pkt_outer_l2_hdr_otag = val;
        GENL_DBG_VERB("INGRESS_PKT_OUTER_L2_HDR_OTAG: %d\n", val);
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
#ifdef GENL_DEBUG
    if (debug & GENL_DBG_LVL_VERB) {
        printk("init_cb unit %d, dev %s dev_id: 0x%x variant %s\n",
               dinfo->dev_no, dinfo->type_str, dinfo->dev_id, dinfo->var_str);
        printk("dev_type: %d\n", cb_dev[unit].dev_type);
        printk("variant: %d\n", cb_dev[unit].var_type);
    }
#endif /* GENL_DEBUG */
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
#if 0
    bcmgenl_packet_init();
#endif
    bcmgenl_psample_init();

    return 0;
}

static void __exit
bcmgenl_exit_module(void)
{
    ngknet_dev_init_cb_unregister(init_cb);
#if 0
    bcmgenl_packet_cleanup();
#endif
    bcmgenl_psample_cleanup();
    bcmgenl_proc_cleanup();
}

module_init(bcmgenl_init_module);
module_exit(bcmgenl_exit_module);
