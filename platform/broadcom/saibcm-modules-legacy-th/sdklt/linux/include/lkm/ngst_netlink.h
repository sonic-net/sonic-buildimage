/*! \file ngst_netlink.h
 *
 * NGST device Netlink message definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 * IMPORTANT!
 * All shared structures must be properly 64-bit aligned.
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

#ifndef NGST_NETLINK_H
#define NGST_NETLINK_H

#include <linux/types.h>

#define NGST_NETLINK_PROTOCOL 17

#define NGST_NL_MSG_TYPE_ST_DATA_REQ        1
#define NGST_NL_MSG_TYPE_ST_DATA_NOT_READY  2
#define NGST_NL_MSG_TYPE_ST_DATA_RSP        3

struct ngst_nl_msg_hdr_s {
    __u32 unit;
    __u32 msg_type;
};

#endif /* NGST_NETLINK_H */
