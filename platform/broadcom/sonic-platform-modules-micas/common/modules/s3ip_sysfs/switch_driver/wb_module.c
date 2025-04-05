/*
 * Copyright(C) 2001-2012 whitebox Network. All rights reserved.
 */
/*
 * wb_module.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * dfd kernel code, DM-independent code extracted
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0    sonic_rd@whitebox         2020-02-17          Initial version
 *   *  v1.1    sonic_rd@whitebox         2021-08-26          S3IP sysfs
 */
#include <linux/module.h>

#include "./include/wb_module.h"
#include "./include/dfd_cfg.h"

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
    int key,dev_num;
    int *p_dev_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, main_dev_id, minor_dev_id);
    p_dev_num = dfd_ko_cfg_get_item(key);
    if (p_dev_num == NULL) {
        DBG_DEBUG(DBG_ERROR, "get device number failed, key:0x%x\n",key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    dev_num = *p_dev_num;
    DBG_DEBUG(DBG_VERBOSE, "get device number ok, number:%d\n",dev_num);
    return dev_num;
}
