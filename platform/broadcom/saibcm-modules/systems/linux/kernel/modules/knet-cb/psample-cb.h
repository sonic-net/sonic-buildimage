/*
 * Copyright 2017 Broadcom
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

#include <gmodule.h>
#include <kcom.h>
#include <linux/netdevice.h>

#define PSAMPLE_CB_NAME "psample"

extern int
psample_init(void);

extern int
psample_cleanup(void);

extern int
psample_filter_cb(uint8_t * pkt, int size, int dev_no, void *pkt_meta,
                  int chan, kcom_filter_t *kf);

/* psample data per interface */
typedef struct {
    struct list_head list;
    struct net_device *dev;
    uint16 id;
    uint16 port;
    uint16 vlan;
    uint16 qnum;
    uint32 sample_rate;
    uint32 sample_size;
} psample_netif_t;

extern int
psample_netif_create_cb(int unit, kcom_netif_t *netif, struct net_device *dev);

extern int
psample_netif_destroy_cb(int unit, kcom_netif_t *netif, struct net_device *dev);

#endif /* __PSAMPLE_CB_H__ */
