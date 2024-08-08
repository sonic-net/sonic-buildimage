/*
 * $Copyright: 2007-2023 Broadcom Inc. All rights reserved.
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
 */

/*
 * Middle-driver for communication between Linux KNET driver and
 * drivers support Generic Netlink channel.
 *
 * This code is used to provide the device information.
 * This driver is built with the DCB library as the helper for parsing
 * the RX packet meta data from the Linux KNET driver filter call-back function.
 * The environment DCBDIR must be set to indicate the directroy of the DCB
 * library.
 */

#include <gmodule.h> /* Must be included first */
#include <linux-bde.h>
#include <kcom.h>
#include <bcm-knet.h>

#include <appl/dcb/dcb_handler.h>

#include "bcm-genl-dev.h"

/* Module header Op-Codes */
#define SOC_HIGIG_OP_CPU        0x00 /* CPU Frame */
#define SOC_HIGIG_OP_UC         0x01 /* Unicast Frame */
#define SOC_HIGIG_OP_BC         0x02 /* Broadcast or DLF frame */
#define SOC_HIGIG_OP_MC         0x03 /* Multicast Frame */
#define SOC_HIGIG_OP_IPMC       0x04 /* IP Multicast Frame */

static dcb_handle_t g_dcb_hdl[LINUX_BDE_MAX_DEVICES];

static dcb_handle_t *
dcb_handle_get(int dev_no)
{
    uint16_t dev_id;
    uint8_t rev_id;
    dcb_handle_t *dcbh;

    if (dev_no >= LINUX_BDE_MAX_DEVICES || dev_no < 0) {
        return NULL;
    }

    dcbh = &g_dcb_hdl[dev_no];
    if (DCB_OP(dcbh) == NULL) {
        if (bkn_hw_device_get(dev_no, &dev_id, &rev_id) < 0) {
            return NULL;
        }
        if (dcb_handle_init(dcbh, dev_id, rev_id) != DCB_OK) {
            gprintk("%s: dev id 0x%04x rev id 0x%02x is not supported\n",
                    __func__, dev_id, rev_id);
            return NULL;
        }
    }

    return DCB_OP(dcbh) ? dcbh : NULL;
}

int
bcmgenl_dev_pktmeta_rx_srcport_get(int dev_no, void *pkt_meta,
                                   uint32_t *srcport)
{
    dcb_handle_t *dcbh = dcb_handle_get(dev_no);

    if (dcbh == NULL || srcport == NULL) {
        return -1;
    }

    *srcport = DCB_RX_SRCPORT_GET(dcbh, pkt_meta);

    return 0;
}

int
bcmgenl_dev_pktmeta_rx_dstport_get(int dev_no, void *pkt_meta,
                                   bool *mcast, uint32_t *dstport)
{
    dcb_handle_t *dcbh = dcb_handle_get(dev_no);
    uint32_t opc;

    if (dcbh == NULL || mcast == NULL || dstport == NULL) {
        return -1;
    }

    opc = DCB_RX_OPCODE_GET(dcbh, pkt_meta);
    *mcast = (opc == SOC_HIGIG_OP_CPU || opc == SOC_HIGIG_OP_UC) ? 0 : 1;
    if (*mcast) {
        *dstport = 0;
    } else {
        *dstport = DCB_RX_DESTPORT_GET(dcbh, pkt_meta);
    }

    return 0;
}

int
bcmgenl_dev_pktmeta_rx_reason_get(int dev_no, void *pkt_meta, uint64_t *reason)
{
    dcb_handle_t *dcbh = dcb_handle_get(dev_no);

    if (dcbh == NULL || reason == NULL) {
        return -1;
    }

    *reason = (uint64_t)DCB_RX_REASON_HI_GET(dcbh, pkt_meta) << 32;
    *reason |= DCB_RX_REASON_GET(dcbh, pkt_meta);

    return 0;
}

int
bcmgenl_dev_rx_reason_sample_source_get(int dev_no, uint64_t *val)
{
    dcb_handle_t *dcbh = dcb_handle_get(dev_no);
    soc_rx_reason_t *map;
    int idx = 0;

    if (dcbh == NULL || val == NULL) {
        return -1;
    }
    *val = 0;

    if (DCB_TYPE(dcbh) == 38) {
        /* Rx reason for SampleSouce is an encoded value (3) in bits [17:16] */
        *val = 0x30000;
    } else {
        map = DCB_OP(dcbh)->rx_reason_maps[0];
        do {
            if (map[idx] == socRxReasonSampleSource) {
                *val = (uint64_t)1 << idx;
                break;
            }
        } while (map[++idx] != socRxReasonInvalid);
    }

    if (*val == 0) {
        gprintk("%s: No rx reasone sample source for dcb type %d\n",
                __func__, DCB_TYPE(dcbh));
        return -1;
    }
    return 0;
}

int
bcmgenl_dev_dcb_info_get(int dev_no, int *dcb_type, int *dcb_size)
{
    dcb_handle_t *dcbh = dcb_handle_get(dev_no);

    if (dcbh == NULL) {
        return -1;
    }

    if (dcb_type) {
        *dcb_type = DCB_TYPE(dcbh);
    }
    if (dcb_size) {
        *dcb_size = DCB_SIZE(dcbh);
    }

    return 0;
}

int
bcmgenl_dev_init(void)
{
    memset(g_dcb_hdl, 0, sizeof(g_dcb_hdl));
    return 0;
}

int
bcmgenl_dev_cleanup(void)
{
    return 0;
}
