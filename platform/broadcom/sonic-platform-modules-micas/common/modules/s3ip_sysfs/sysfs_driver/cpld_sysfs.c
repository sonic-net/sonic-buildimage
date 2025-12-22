/*
 * An cpld_sysfs driver for cpld sysfs devcie function
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
#include <linux/fs.h>

#include "switch.h"
#include "cpld_sysfs.h"

static int g_cpld_loglevel = 0;

#define CPLD_REBOOT_CAUSE_FILE      "/etc/.reboot/.previous-reboot-cause.txt"
#define REBOOT_CAUSE_NAME_LEN       (64)

/* Reboot cause type */
typedef enum wb_reboot_cause_type_e {
    REBOOT_CAUSE_NON_HARDWARE = 0,
    REBOOT_CAUSE_POWER_LOSS,
    REBOOT_CAUSE_THERMAL_OVERLOAD_CPU,
    REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
    REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER,
    REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED,
    REBOOT_CAUSE_WATCHDOG,
    REBOOT_CAUSE_HARDWARE_OTHER,
    REBOOT_CAUSE_CPU_COLD_RESET,
    REBOOT_CAUSE_CPU_WARM_RESET,
    REBOOT_CAUSE_BIOS_RESET,
    REBOOT_CAUSE_PSU_SHUTDOWN,
    REBOOT_CAUSE_BMC_SHUTDOWN,
    REBOOT_CAUSE_RESET_BUTTON_SHUTDOWN,
    REBOOT_CAUSE_RESET_BUTTON_COLD_SHUTDOWN,
} wb_reboot_cause_type_t;

struct reboot_cause_file_info_s {
    wb_reboot_cause_type_t reboot_cause_type;
    char reboot_cause_name[REBOOT_CAUSE_NAME_LEN];
};

struct reboot_cause_file_info_s reboot_cause_file_info_match[] = {
    {REBOOT_CAUSE_POWER_LOSS,                   "Power Loss"},
    {REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,        "Watchdog reboot"},
    {REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER,       "BMC reboot"},
    {REBOOT_CAUSE_BMC_SHUTDOWN,                 "BMC powerdown"},
    {REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,        "Thermal Overload: ASIC"},
    {REBOOT_CAUSE_CPU_WARM_RESET,               "Warm reboot"},
};

#define CPLD_INFO(fmt, args...) do {                                        \
    if (g_cpld_loglevel & INFO) { \
        printk(KERN_INFO "[CPLD_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CPLD_ERR(fmt, args...) do {                                        \
    if (g_cpld_loglevel & ERR) { \
        printk(KERN_ERR "[CPLD_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CPLD_DBG(fmt, args...) do {                                        \
    if (g_cpld_loglevel & DBG) { \
        printk(KERN_DEBUG "[CPLD_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct cpld_obj_s {
    struct switch_obj *obj;
};

struct cpld_s {
    unsigned int cpld_number;
    struct cpld_obj_s *cpld;
};

static struct cpld_s g_cpld;
static struct switch_obj *g_cpld_obj = NULL;
static struct s3ip_sysfs_cpld_drivers_s *g_cpld_drv = NULL;

static int cpld_file_read(char *fpath, char *buf, int size)
{
    int ret;
    struct file *filp;
    loff_t pos;

    filp = filp_open(fpath, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        CPLD_ERR("can't open %s", fpath);
        filp = NULL;
        ret = -ENOENT;
        goto fail;
    }
    mem_clear(buf, size);
    pos = 0;
    ret = kernel_read(filp, buf, size - 1, &pos);
    if (ret < 0) {
        CPLD_ERR("read file %s error, ret:%d\n", fpath, ret);
    }
fail:
    if (filp != NULL) {
        filp_close(filp, NULL);
        filp = NULL;
    }

    return ret;
}

static ssize_t cpld_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_cpld.cpld_number);
}

static ssize_t cpld_reboot_cause_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    char reboot_cause_buf[REBOOT_CAUSE_NAME_LEN];
    int ret, i;
    char *point;
    int reboot_cause_type;

    mem_clear(reboot_cause_buf, sizeof(reboot_cause_buf));
    ret = cpld_file_read(CPLD_REBOOT_CAUSE_FILE, reboot_cause_buf, REBOOT_CAUSE_NAME_LEN - 1);
    if (ret < 0) {
        CPLD_ERR("read file %s error, ret:%d\n", CPLD_REBOOT_CAUSE_FILE, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
    }

    point = strchr(reboot_cause_buf, ',');
    if (point != NULL) {
        *point = 0;
    }
    CPLD_DBG("read reboot cause:%s\n", reboot_cause_buf);

    reboot_cause_type = 0;
    for (i = 0; i < ARRAY_SIZE(reboot_cause_file_info_match); i++) {
        if (strncmp(reboot_cause_file_info_match[i].reboot_cause_name, reboot_cause_buf, \
                    strlen(reboot_cause_file_info_match[i].reboot_cause_name)) == 0) {
            reboot_cause_type = reboot_cause_file_info_match[i].reboot_cause_type;
            CPLD_DBG("reboot cause %s match type[%d].\n", reboot_cause_file_info_match[i].reboot_cause_name, reboot_cause_type);
            break;
        }
    }
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", reboot_cause_type);
}

static ssize_t cpld_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int cpld_index;

    check_p(g_cpld_drv);
    check_p(g_cpld_drv->get_main_board_cpld_alias);

    cpld_index = obj->index;
    CPLD_DBG("cpld index: %u\n", cpld_index);
    return g_cpld_drv->get_main_board_cpld_alias(cpld_index, buf, PAGE_SIZE);
}

static ssize_t cpld_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int cpld_index;

    check_p(g_cpld_drv);
    check_p(g_cpld_drv->get_main_board_cpld_type);

    cpld_index = obj->index;
    CPLD_DBG("cpld index: %u\n", cpld_index);
    return g_cpld_drv->get_main_board_cpld_type(cpld_index, buf, PAGE_SIZE);
}

static ssize_t cpld_fw_version_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int cpld_index;

    check_p(g_cpld_drv);
    check_p(g_cpld_drv->get_main_board_cpld_firmware_version);

    cpld_index = obj->index;
    CPLD_DBG("cpld index: %u\n", cpld_index);
    return g_cpld_drv->get_main_board_cpld_firmware_version(cpld_index, buf, PAGE_SIZE);
}

static ssize_t cpld_board_version_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int cpld_index;

    check_p(g_cpld_drv);
    check_p(g_cpld_drv->get_main_board_cpld_board_version);

    cpld_index = obj->index;
    CPLD_DBG("cpld index: %u\n", cpld_index);
    return g_cpld_drv->get_main_board_cpld_board_version(cpld_index, buf, PAGE_SIZE);
}

static ssize_t cpld_test_reg_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int cpld_index;

    check_p(g_cpld_drv);
    check_p(g_cpld_drv->get_main_board_cpld_test_reg);

    cpld_index = obj->index;
    CPLD_DBG("cpld index: %u\n", cpld_index);
    return g_cpld_drv->get_main_board_cpld_test_reg(cpld_index, buf, PAGE_SIZE);
}

static ssize_t cpld_test_reg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int cpld_index, value;
    int ret;

    check_p(g_cpld_drv);
    check_p(g_cpld_drv->set_main_board_cpld_test_reg);

    cpld_index = obj->index;
    sscanf(buf, "0x%x", &value);
    ret = g_cpld_drv->set_main_board_cpld_test_reg(cpld_index, value);
    if (ret < 0) {
        CPLD_ERR("set cpld%u test reg failed, value:0x%x, ret: %d.\n", cpld_index, value, ret);
        return ret;
    }
    CPLD_DBG("set cpld%u test reg success, value: 0x%x.\n", cpld_index, value);
    return count;
}

/************************************cpld dir and attrs*******************************************/
static struct switch_attribute cpld_number_att = __ATTR(number, S_IRUGO, cpld_number_show, NULL);
static struct switch_attribute cpld_reboot_cause_att = __ATTR(reboot_cause, S_IRUGO, cpld_reboot_cause_show, NULL);

static struct attribute *cpld_dir_attrs[] = {
    &cpld_number_att.attr,
    &cpld_reboot_cause_att.attr,
    NULL,
};

static struct attribute_group cpld_root_attr_group = {
    .attrs = cpld_dir_attrs,
};

/*******************************cpld[1-n] dir and attrs*******************************************/
static struct switch_attribute cpld_alias_attr = __ATTR(alias, S_IRUGO, cpld_alias_show, NULL);
static struct switch_attribute cpld_type_attr = __ATTR(type, S_IRUGO, cpld_type_show, NULL);
static struct switch_attribute cpld_fw_version_attr = __ATTR(firmware_version, S_IRUGO, cpld_fw_version_show, NULL);
static struct switch_attribute cpld_board_version_attr = __ATTR(board_version, S_IRUGO, cpld_board_version_show, NULL);
static struct switch_attribute cpld_test_reg_attr = __ATTR(reg_test, S_IRUGO | S_IWUSR, cpld_test_reg_show, cpld_test_reg_store);

static struct attribute *cpld_attrs[] = {
    &cpld_alias_attr.attr,
    &cpld_type_attr.attr,
    &cpld_fw_version_attr.attr,
    &cpld_board_version_attr.attr,
    &cpld_test_reg_attr.attr,
    NULL,
};

static struct attribute_group cpld_attr_group = {
    .attrs = cpld_attrs,
};

static int cpld_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct cpld_obj_s *curr_cpld;

    curr_cpld = &g_cpld.cpld[index - 1];
    if (curr_cpld->obj) {
        sysfs_remove_group(&curr_cpld->obj->kobj, &cpld_attr_group);
        switch_kobject_delete(&curr_cpld->obj);
        CPLD_DBG("delete cpld%u dir and attrs success.\n", index);
    }

    return 0;
}

static int cpld_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[8];
    struct cpld_obj_s *curr_cpld;

    curr_cpld = &g_cpld.cpld[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "cpld%u", index);
    curr_cpld->obj = switch_kobject_create(name, parent);
    if (!curr_cpld->obj) {
        CPLD_ERR("create %s object error!\n", name);
        return -EBADRQC;
    }
    curr_cpld->obj->index = index;
    if (sysfs_create_group(&curr_cpld->obj->kobj, &cpld_attr_group) != 0) {
        CPLD_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_cpld->obj);
        return -EBADRQC;
    }
    CPLD_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int cpld_sub_create_kobj_and_attrs(struct kobject *parent, int cpld_num)
{
    unsigned int cpld_index, i;

    g_cpld.cpld = kzalloc(sizeof(struct cpld_obj_s) * cpld_num, GFP_KERNEL);
    if (!g_cpld.cpld) {
        CPLD_ERR("kzalloc g_cpld.cpld error, cpld number = %d.\n", cpld_num);
        return -ENOMEM;
    }

    for (cpld_index = 1; cpld_index <= cpld_num; cpld_index++) {
        if (cpld_sub_single_create_kobj_and_attrs(parent, cpld_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = cpld_index; i > 0; i--) {
        cpld_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_cpld.cpld);
    g_cpld.cpld = NULL;
    return -EBADRQC;
}

/* create cpld[1-n] directory and attributes*/
static int cpld_sub_create(void)
{
    int ret;

    ret = cpld_sub_create_kobj_and_attrs(&g_cpld_obj->kobj, g_cpld.cpld_number);
    return ret;
}

/* delete cpld[1-n] directory and attributes*/
static void cpld_sub_remove(void)
{
    unsigned int cpld_index;

    if (g_cpld.cpld) {
        for (cpld_index = g_cpld.cpld_number; cpld_index > 0; cpld_index--) {
            cpld_sub_single_remove_kobj_and_attrs(cpld_index);
        }
        kfree(g_cpld.cpld);
        g_cpld.cpld = NULL;
    }
    g_cpld.cpld_number = 0;
    return;
}

/* create cpld directory and number attributes */
static int cpld_root_create(void)
{
    g_cpld_obj = switch_kobject_create("cpld", NULL);
    if (!g_cpld_obj) {
        CPLD_ERR("switch_kobject_create cpld error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_cpld_obj->kobj, &cpld_root_attr_group) != 0) {
        switch_kobject_delete(&g_cpld_obj);
        CPLD_ERR("create cpld dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete cpld directory and number attributes */
static void cpld_root_remove(void)
{
    if (g_cpld_obj) {
        sysfs_remove_group(&g_cpld_obj->kobj, &cpld_root_attr_group);
        switch_kobject_delete(&g_cpld_obj);
    }

    return;
}

int s3ip_sysfs_cpld_drivers_register(struct s3ip_sysfs_cpld_drivers_s *drv)
{
    int ret, cpld_num;

    CPLD_INFO("s3ip_sysfs_cpld_drivers_register...\n");
    if (g_cpld_drv) {
        CPLD_ERR("g_cpld_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_cpld_number);
    g_cpld_drv = drv;

    cpld_num = g_cpld_drv->get_main_board_cpld_number();
    if (cpld_num <= 0) {
        CPLD_ERR("cpld number: %d, don't need to create cpld dirs and attrs.\n", cpld_num);
        g_cpld_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_cpld, sizeof(struct cpld_s));
    g_cpld.cpld_number = cpld_num;
    ret = cpld_root_create();
    if (ret < 0) {
        CPLD_ERR("create cpld root dir and attrs failed, ret: %d\n", ret);
        g_cpld_drv = NULL;
        return ret;
    }
    ret = cpld_sub_create();
    if (ret < 0) {
        CPLD_ERR("create cpld sub dir and attrs failed, ret: %d\n", ret);
        cpld_root_remove();
        g_cpld_drv = NULL;
        return ret;
    }
    CPLD_INFO("s3ip_sysfs_cpld_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_cpld_drivers_unregister(void)
{
    if (g_cpld_drv) {
        cpld_sub_remove();
        cpld_root_remove();
        g_cpld_drv = NULL;
        CPLD_DBG("s3ip_sysfs_cpld_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_cpld_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_cpld_drivers_unregister);
module_param(g_cpld_loglevel, int, 0644);
MODULE_PARM_DESC(g_cpld_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
