/*
 * An dfd_slot_driver driver for dfd slot function
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

#include <linux/module.h>
#include <linux/slab.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_cfg_info.h"
#include "../dev_sysfs/include/sysfs_common.h"

#define SLOT_SIZE                         (256)

int g_dfd_slot_dbg_level = 0;
module_param(g_dfd_slot_dbg_level, int, S_IRUGO | S_IWUSR);

int dfd_get_slot_present_status(unsigned int slot_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, WB_MAIN_DEV_SLOT, slot_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_SLOT_DEBUG(DBG_ERROR, "get slot status error, key:0x%x\n",key);
        return ret;
    }
    return status;
}
