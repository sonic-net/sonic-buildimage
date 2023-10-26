/*
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
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
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa
 */
/*
 * $Id: bcm-knet.h,v 1.4 Broadcom SDK $
 * $Copyright: (c) 2005 Broadcom Corp.
 * All Rights Reserved.$
 */
#ifndef __LINUX_BCM_KNET_H__
#define __LINUX_BCM_KNET_H__

#ifndef __KERNEL__
#include <stdint.h>
#endif

typedef struct  {
    int rc;
    int len;
    int bufsz;
    int reserved;
    uint64_t buf;
} bkn_ioctl_t;

#ifdef __KERNEL__

/*
 * Call-back interfaces for other Linux kernel drivers.
 */
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <kcom.h>

typedef struct {
    uint32 netif_user_data;
    uint32 filter_user_data;
    uint16 dcb_type;
    int port;
    uint64_t ts;
    uint32 hwts;
} knet_skb_cb_t;

#define KNET_SKB_CB(_skb) ((knet_skb_cb_t *)_skb->cb)

typedef struct sk_buff *
(*knet_skb_cb_f)(struct sk_buff *skb, int dev_no, void *meta);

typedef int
(*knet_filter_cb_f)(uint8_t *pkt, int size, int dev_no, void *meta,
                    int chan, kcom_filter_t *filter);

typedef int
(*knet_hw_tstamp_enable_cb_f)(int dev_no, int phys_port, int tx_type);

typedef int
(*knet_hw_tstamp_tx_time_get_cb_f)(int dev_no, int phys_port, uint8_t *pkt, uint64_t *ts, int tx_type);

typedef int
(*knet_hw_tstamp_tx_meta_get_cb_f)(int dev_no, int hwts, int hdrlen, struct sk_buff *skb, uint64_t *ts, uint32_t **md);

typedef int
(*knet_hw_tstamp_ptp_clock_index_cb_f)(int dev_no);

typedef int
(*knet_hw_tstamp_rx_pre_process_cb_f)(int dev_no, uint8_t *pkt, uint32_t sspa, int *pkt_offset);

typedef int
(*knet_hw_tstamp_rx_time_upscale_cb_f)(int dev_no, int phys_port, struct sk_buff *skb, uint32_t *meta, uint64_t *ts);

typedef int
(*knet_hw_tstamp_ioctl_cmd_cb_f)(kcom_msg_clock_cmd_t *kmsg, int len, int dcb_type);

extern int
bkn_rx_skb_cb_register(knet_skb_cb_f rx_cb);

extern int
bkn_rx_skb_cb_unregister(knet_skb_cb_f rx_cb);

extern int
bkn_tx_skb_cb_register(knet_skb_cb_f tx_cb);

extern int
bkn_tx_skb_cb_unregister(knet_skb_cb_f tx_cb);

extern int
bkn_filter_cb_register(knet_filter_cb_f filter_cb);

extern int
bkn_filter_cb_unregister(knet_filter_cb_f filter_cb);

extern int
bkn_hw_tstamp_enable_cb_register(knet_hw_tstamp_enable_cb_f hw_tstamp_enable_cb);

extern int
bkn_hw_tstamp_enable_cb_unregister(knet_hw_tstamp_enable_cb_f hw_tstamp_enable_cb);

extern int
bkn_hw_tstamp_disable_cb_register(knet_hw_tstamp_enable_cb_f hw_tstamp_disable_cb);

extern int
bkn_hw_tstamp_disable_cb_unregister(knet_hw_tstamp_enable_cb_f hw_tstamp_disable_cb);

extern int
bkn_hw_tstamp_tx_time_get_cb_register(knet_hw_tstamp_tx_time_get_cb_f hw_tstamp_tx_time_get_cb);

extern int
bkn_hw_tstamp_tx_time_get_cb_unregister(knet_hw_tstamp_tx_time_get_cb_f hw_tstamp_tx_time_get_cb);

extern int
bkn_hw_tstamp_tx_meta_get_cb_register(knet_hw_tstamp_tx_meta_get_cb_f hw_tstamp_tx_meta_get_cb);

extern int
bkn_hw_tstamp_tx_meta_get_cb_unregister(knet_hw_tstamp_tx_meta_get_cb_f hw_tstamp_tx_meta_get_cb);

extern int
bkn_hw_tstamp_ptp_clock_index_cb_register(knet_hw_tstamp_ptp_clock_index_cb_f hw_tstamp_ptp_clock_index_cb);

extern int
bkn_hw_tstamp_ptp_clock_index_cb_unregister(knet_hw_tstamp_ptp_clock_index_cb_f hw_tstamp_ptp_clock_index_cb);

extern int
bkn_hw_tstamp_rx_pre_process_cb_register(knet_hw_tstamp_rx_pre_process_cb_f hw_tstamp_rx_pre_process_cb);

extern int
bkn_hw_tstamp_rx_pre_process_cb_unregister(knet_hw_tstamp_rx_pre_process_cb_f hw_tstamp_rx_pre_process_cb);

extern int
bkn_hw_tstamp_rx_time_upscale_cb_register(knet_hw_tstamp_rx_time_upscale_cb_f hw_tstamp_rx_time_upscale_cb);

extern int
bkn_hw_tstamp_rx_time_upscale_cb_unregister(knet_hw_tstamp_rx_time_upscale_cb_f hw_tstamp_rx_time_upscale_cb);

extern int
bkn_hw_tstamp_ioctl_cmd_cb_register(knet_hw_tstamp_ioctl_cmd_cb_f hw_tstamp_ioctl_cmd_cb);

extern int
bkn_hw_tstamp_ioctl_cmd_cb_unregister(knet_hw_tstamp_ioctl_cmd_cb_f hw_tstamp_ioctl_cmd_cb);
typedef struct {
    uint8 cmic_type;
    uint8 dcb_type;
    uint8 dcb_size;
    uint8 pkt_hdr_size;
    uint32 cdma_channels;
} knet_hw_info_t;

extern int
bkn_hw_info_get(int unit, knet_hw_info_t *hw_info);

typedef int
(*knet_netif_cb_f)(int unit, kcom_netif_t *netif, struct net_device *dev);

extern int
bkn_netif_create_cb_register(knet_netif_cb_f netif_cb);

extern int
bkn_netif_create_cb_unregister(knet_netif_cb_f netif_cb);

extern int
bkn_netif_destroy_cb_register(knet_netif_cb_f netif_cb);

extern int
bkn_netif_destroy_cb_unregister(knet_netif_cb_f netif_cb);
#endif

#endif /* __LINUX_BCM_KNET_H__ */
