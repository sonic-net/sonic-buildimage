/*
 * Copyright 2022 Broadcom
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
 * $Id: psample_cb.h $
 * $Copyright: (c) 2019 Broadcom Corp.
 * All Rights Reserved.$
 */
#ifndef __PSAMPLE_CB_H__
#define __PSAMPLE_CB_H__

#include <lkm/lkm.h>
#include <linux/netdevice.h>

//#define PSAMPLE_SUPPORT 1  // TODO: MLI@BRCM - Add this as part of conditional in Makefile
#define PSAMPLE_CB_NAME "psample"

extern int
psample_init(void);

extern int
psample_cleanup(void);

extern struct sk_buff*
psample_rx_cb(struct net_device *dev, struct sk_buff *skb);

/* psample data per interface */
typedef struct {
    struct list_head list;
    struct net_device *dev;
    uint16_t id;
    uint16_t port;
    uint16_t vlan;
    uint16_t qnum;
    uint32_t sample_rate;
    uint32_t sample_size;
} psample_netif_t;

extern int
psample_netif_create_cb(struct net_device *dev);

extern int
psample_netif_destroy_cb(struct net_device *dev);

#endif /* __PSAMPLE_CB_H__ */
