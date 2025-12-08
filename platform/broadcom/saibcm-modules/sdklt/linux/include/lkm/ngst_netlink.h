/*! \file ngst_netlink.h
 *
 * NGST device Netlink message definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 */
/*
 *
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

#define NGST_GENL_VERSION 1
#define NGST_GENL_FAMILY_NAME "brcm_stel"
#define NGST_GENL_MCGRP_NAME "ipfix"

enum ngst_genl_cmds {
    NGST_CMD_UNSPEC,
    NGST_CMD_DATA_RSP,
    NGST_CMD_MAX,
};

#define NGST_IDLE_USLEEP_MIN 20
#define NGST_IDLE_USLEEP_MAX 100

#endif /* NGST_NETLINK_H */
