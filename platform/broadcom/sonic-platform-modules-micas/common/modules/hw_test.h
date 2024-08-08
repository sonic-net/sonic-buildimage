/*
 * A header definition for hw_test driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LINUX_DRAM_DRIVER_H
#define _LINUX_DRAM_DRIVER_H

#include <linux/types.h>
#include <linux/compiler.h>

#define mem_clear(data, val, size) memset((data), val, (size))

struct phydev_user_info {
    int phy_index;
    u32 regnum;
    u32 regval;
};

#define CMD_PHY_LIST                        _IOR('P', 0, struct phydev_user_info)
#define CMD_PHY_READ                        _IOR('P', 1, struct phydev_user_info)
#define CMD_PHY_WRITE                       _IOR('P', 2, struct phydev_user_info)

struct mdio_dev_user_info {
    int mdio_index;
    int phyaddr;
    u32 regnum;
    u32 regval;
};

#define CMD_MDIO_LIST                        _IOR('M', 0, struct mdio_dev_user_info)
#define CMD_MDIO_READ                        _IOR('M', 1, struct mdio_dev_user_info)
#define CMD_MDIO_WRITE                       _IOR('M', 2, struct mdio_dev_user_info)

#endif
