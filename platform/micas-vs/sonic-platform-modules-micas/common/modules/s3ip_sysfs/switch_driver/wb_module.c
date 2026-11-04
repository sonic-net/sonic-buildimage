/*
 * An wb_module driver for module devcie function
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

#include "wb_module.h"
#include "dfd_cfg.h"

int g_dfd_dbg_level = 0;   /* Debug level */
module_param(g_dfd_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * wb_dev_cfg_init - dfd module initialization
 *
 * @returns:<0 Failed, otherwise succeeded
 */
int32_t wb_dev_cfg_init(void)
{
    return dfd_dev_cfg_init();
}

/**
 * wb_dev_cfg_exit - dfd module exit
 *
 * @returns: void
 */

void wb_dev_cfg_exit(void)
{
    dfd_dev_cfg_exit();
    return;
}

/**
 * dfd_get_dev_number - Get the number of devices
 * @main_dev_id:Master device number
 * @minor_dev_id:Secondary device number
 * @returns: <0 failed, otherwise number of devices is returned
 */
int dfd_get_dev_number(unsigned int main_dev_id, unsigned int minor_dev_id)
{
    uint64_t key;
    int dev_num;
    int *p_dev_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, main_dev_id, minor_dev_id);
    p_dev_num = dfd_ko_cfg_get_item(key);
    if (p_dev_num == NULL) {
        DBG_DEBUG(DBG_ERROR, "get device number failed, key_name:%s\n",
            key_to_name(DFD_CFG_ITEM_DEV_NUM));
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    dev_num = *p_dev_num;
    DBG_DEBUG(DBG_VERBOSE, "get device number ok, number:%d\n",dev_num);
    return dev_num;
}
