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

#ifndef __BCM_GENL_NETIF_H__
#define __BCM_GENL_NETIF_H__

#include <linux/netdevice.h>

/* generic netlink data per interface */
typedef struct {
    struct net_device *dev;
    unsigned short id;
    unsigned short  port;
    unsigned short vlan;
    unsigned short qnum;
    unsigned int sample_rate;
    unsigned int sample_size;
} bcmgenl_netif_t;

typedef int
(*bcmgenl_netif_search_f)(void *cb_data, bcmgenl_netif_t *bcmgenl_netif);

extern int
bcmgenl_netif_search(char *dev_name, bcmgenl_netif_search_f cb, void *cb_data);

extern int
bcmgenl_netif_num_get(void);

extern int
bcmgenl_netif_get_by_ifindex(int ifindex, bcmgenl_netif_t *bcmgenl_netif);

extern int
bcmgenl_netif_get_by_port(int port, bcmgenl_netif_t *bcmgenl_netif);

extern int
bcmgenl_netif_default_sample_set(int sample_rate, int sample_size);

extern int
bcmgenl_netif_init(void);

extern int
bcmgenl_netif_cleanup(void);

#endif /* __BCM_GENL_NETIF_H__ */
