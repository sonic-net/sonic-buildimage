/*
 * An system_sysfs driver for system sysfs devcie function
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

#include <linux/slab.h>

#include "switch.h"
#include "system_sysfs.h"
#include "switch_driver.h"

static int g_system_loglevel = 0;

#define SYSTEM_INFO(fmt, args...) do {                                        \
    if (g_system_loglevel & INFO) { \
        printk(KERN_INFO "[system][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SYSTEM_ERR(fmt, args...) do {                                        \
    if (g_system_loglevel & ERR) { \
        printk(KERN_ERR "[system][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SYSTEM_DBG(fmt, args...) do {                                        \
    if (g_system_loglevel & DBG) { \
        printk(KERN_DEBUG "[system][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct system_obj_s {
    struct switch_obj *obj;
};

struct system_s {
    unsigned int api_number;
    struct system_obj_s *temp;
};

static struct s3ip_sysfs_system_drivers_s *g_system_drv = NULL;
static struct switch_obj *g_system_obj = NULL;

static ssize_t system_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int value;
    struct switch_device_attribute  *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->get_system_value);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);
    SYSTEM_DBG("system_value_show type 0x%x \n", system_attr->type);
    return g_system_drv->get_system_value(system_attr->type, &value, buf, PAGE_SIZE);
}

static ssize_t system_value_store(struct switch_obj *obj, struct switch_attribute *attr,
                                  const char* buf, size_t count)
{
    int ret, value;
    struct switch_device_attribute  *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->set_system_value);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);

    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        SYSTEM_ERR("system_value_store, input parameter: %s error. ret:%d\n", buf, ret);
        return ret;
    }

    if(value > 0xff) {
        SYSTEM_ERR("system_value_store, input parameter bigger than 0xff: %d\n", value);
        return -EINVAL;
    }
    SYSTEM_DBG("system_value_store, type: 0x%x. value=%d\n", system_attr->type, value);
    ret = g_system_drv->set_system_value(system_attr->type, value);
    if (ret < 0) {
        /* ret=-999 if not support */
        SYSTEM_ERR("set system reg value: %d failed. ret:%d\n", value, ret);
        return ret;
    }
    return count;
}

static ssize_t system_port_port_status_value(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    struct switch_device_attribute  *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->get_system_port_power_status);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);
    SYSTEM_DBG("type 0x%x \n", system_attr->type);
    return g_system_drv->get_system_port_power_status(system_attr->type, buf, PAGE_SIZE);
}

/************************************system dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(bmc_ready, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_BMC_READY);
static SWITCH_DEVICE_ATTR(sol_active, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_SOL_ACTIVE);
static SWITCH_DEVICE_ATTR(psu_reset, S_IWUSR, NULL, system_value_store, WB_SYSTEM_PSU_RESET);
static SWITCH_DEVICE_ATTR(cpu_board_ctrl, S_IWUSR, NULL, system_value_store, WB_SYSTEM_CPU_BOARD_CTRL);
static SWITCH_DEVICE_ATTR(cpu_board_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_CPU_BOARD_STATUS);
static SWITCH_DEVICE_ATTR(bios_switch, S_IWUSR, NULL, system_value_store, WB_SYSTEM_BIOS_SWITCH);
static SWITCH_DEVICE_ATTR(bios_view, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BIOS_VIEW);
static SWITCH_DEVICE_ATTR(bios_boot_ok, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BIOS_BOOT_OK);
static SWITCH_DEVICE_ATTR(bios_fail_record, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BIOS_FAIL_RECORD);
static SWITCH_DEVICE_ATTR(bmc_reset, S_IWUSR, NULL, system_value_store, WB_SYSTEM_BMC_RESET);
static SWITCH_DEVICE_ATTR(mac_board_reset, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_MAC_BOARD_RESET);
static SWITCH_DEVICE_ATTR(mac_pwr_ctrl, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_MAC_PWR_CTRL);
static SWITCH_DEVICE_ATTR(emmc_pwr_ctrl, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_EMMC_PWR_CTRL);
static SWITCH_DEVICE_ATTR(port_pwr_ctl, S_IRUGO | S_IWUSR, system_port_port_status_value, system_value_store, WB_SYSTEM_PORT_PWR_CTL);
static SWITCH_DEVICE_ATTR(bmc_view, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BMC_VIEW);
static SWITCH_DEVICE_ATTR(bmc_switch, S_IWUSR, NULL, system_value_store, WB_SYSTEM_BMC_SWITCH);

static struct attribute *system_dir_attrs[] = {
    &switch_dev_attr_bmc_ready.switch_attr.attr,
    &switch_dev_attr_sol_active.switch_attr.attr,
    &switch_dev_attr_psu_reset.switch_attr.attr,
    &switch_dev_attr_cpu_board_ctrl.switch_attr.attr,
    &switch_dev_attr_cpu_board_status.switch_attr.attr,
    &switch_dev_attr_bios_switch.switch_attr.attr,
    &switch_dev_attr_bios_view.switch_attr.attr,
    &switch_dev_attr_bios_boot_ok.switch_attr.attr,
    &switch_dev_attr_bios_fail_record.switch_attr.attr,
    &switch_dev_attr_bmc_reset.switch_attr.attr,
    &switch_dev_attr_mac_board_reset.switch_attr.attr,
    &switch_dev_attr_mac_pwr_ctrl.switch_attr.attr,
    &switch_dev_attr_emmc_pwr_ctrl.switch_attr.attr,
    &switch_dev_attr_port_pwr_ctl.switch_attr.attr,
    &switch_dev_attr_bmc_view.switch_attr.attr,
    &switch_dev_attr_bmc_switch.switch_attr.attr,
    NULL,
};

static struct attribute_group system_root_attr_group = {
    .attrs = system_dir_attrs,
};

/* create system directory and number attributes */
static int system_root_create(void)
{
    g_system_obj = switch_kobject_create("system", NULL);
    if (!g_system_obj) {
        SYSTEM_ERR("switch_kobject_create system error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_system_obj->kobj, &system_root_attr_group) != 0) {
        switch_kobject_delete(&g_system_obj);
        SYSTEM_ERR("create system dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete system directory and number attributes */
static void system_root_remove(void)
{
    if (g_system_obj) {
        sysfs_remove_group(&g_system_obj->kobj, &system_root_attr_group);
        switch_kobject_delete(&g_system_obj);
    }

    return;
}

int s3ip_sysfs_system_drivers_register(struct s3ip_sysfs_system_drivers_s *drv)
{
    int ret;

    SYSTEM_INFO("s3ip_sysfs_system_drivers_register...\n");
    check_p(drv);
    g_system_drv = drv;
    ret = system_root_create();
    if (ret < 0) {
        SYSTEM_ERR("create system root dir and attrs failed, ret: %d\n", ret);
        return ret;
    }

    SYSTEM_INFO("s3ip_sysfs_system_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_system_drivers_unregister(void)
{
    if (g_system_drv) {
        system_root_remove();
        g_system_drv = NULL;
        SYSTEM_DBG("s3ip_sysfs_system_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_system_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_system_drivers_unregister);
module_param(g_system_loglevel, int, 0644);
MODULE_PARM_DESC(g_system_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
