/* Copyright (C) 2020  MediaTek, Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

 /* FILE NAME:  netif_nl.h
 * PURPOSE:
 *      It provide xxx API.
 * NOTES:
 */

#ifndef NETIF_NL_H
#define NETIF_NL_H

#include <linux/skbuff.h>

#define NETIF_NL_NETLINK_MC_GROUP_NUM           (32)
#define NETIF_NL_NETLINK_NAME_LEN               (16)

typedef enum
{
    NETIF_NL_INTF_PROPERTY_IGR_SAMPLING_RATE,
    NETIF_NL_INTF_PROPERTY_EGR_SAMPLING_RATE,
    NETIF_NL_INTF_PROPERTY_LAST
} NETIF_NL_INTF_PROPERTY_T;

/* must be the same with NPS_NETIF_RX_DST_NETLINK_T */
typedef struct
{
    C8_T                                name[NETIF_NL_NETLINK_NAME_LEN];
    C8_T                                mc_group_name[NETIF_NL_NETLINK_NAME_LEN];
} NETIF_NL_RX_DST_NETLINK_T;

/* must be the same with NPS_NETIF_NETLINK_MC_GROUP_T */
typedef struct
{
    C8_T                                name[NETIF_NL_NETLINK_NAME_LEN];

} NETIF_NL_NETLINK_MC_GROUP_T;

/* must be the same with NPS_NETIF_NETLINK_T */
typedef struct
{
    UI32_T                              id;
    C8_T                                name[NETIF_NL_NETLINK_NAME_LEN];
    NETIF_NL_NETLINK_MC_GROUP_T         mc_group[NETIF_NL_NETLINK_MC_GROUP_NUM];
    UI32_T                              mc_group_num;

} NETIF_NL_NETLINK_T;

NPS_ERROR_NO_T
netif_nl_rxSkb(
    const UI32_T                        unit,
    struct sk_buff                      *ptr_skb,
    void                                *ptr_cookie);

NPS_ERROR_NO_T
netif_nl_setIntfProperty(
    const UI32_T                        unit,
    const UI32_T                        id,
    const NETIF_NL_INTF_PROPERTY_T      property,
    const UI32_T                        param0,
    const UI32_T                        param1);

NPS_ERROR_NO_T
netif_nl_getIntfProperty(
    const UI32_T                        unit,
    const UI32_T                        port,
    const NETIF_NL_INTF_PROPERTY_T      property,
    UI32_T                              *ptr_param0,
    UI32_T                              *ptr_param1);

NPS_ERROR_NO_T
netif_nl_createNetlink(
    const UI32_T                        unit,
    NETIF_NL_NETLINK_T                  *ptr_netlink,
    UI32_T                              *ptr_netlink_id);

NPS_ERROR_NO_T
netif_nl_destroyNetlink(
    const UI32_T                        unit,
    const UI32_T                        group_id);

NPS_ERROR_NO_T
netif_nl_getNetlink(
    const UI32_T                        unit,
    const UI32_T                        netlink_id,
    NETIF_NL_NETLINK_T                  *ptr_netlink);


NPS_ERROR_NO_T
netif_nl_init(void);

#endif /* end of NETIF_NL_H */
