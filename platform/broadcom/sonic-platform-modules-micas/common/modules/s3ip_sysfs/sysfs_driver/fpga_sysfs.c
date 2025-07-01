/*
 * An fpga_sysfs driver for fpga sysfs devcie function
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
#include "fpga_sysfs.h"

static int g_fpga_loglevel = 0;

#define FPGA_INFO(fmt, args...) do {                                        \
    if (g_fpga_loglevel & INFO) { \
        printk(KERN_INFO "[FPGA_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_ERR(fmt, args...) do {                                        \
    if (g_fpga_loglevel & ERR) { \
        printk(KERN_ERR "[FPGA_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_DBG(fmt, args...) do {                                        \
    if (g_fpga_loglevel & DBG) { \
        printk(KERN_DEBUG "[FPGA_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct fpga_obj_s {
    struct switch_obj *obj;
};

struct fpga_s {
    unsigned int fpga_number;
    struct fpga_obj_s *fpga;
};

static struct fpga_s g_fpga;
static struct switch_obj *g_fpga_obj = NULL;
static struct s3ip_sysfs_fpga_drivers_s *g_fpga_drv = NULL;

static ssize_t fpga_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_fpga.fpga_number);
}

static ssize_t fpga_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_alias);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    return g_fpga_drv->get_main_board_fpga_alias(fpga_index, buf, PAGE_SIZE);
}

static ssize_t fpga_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_type);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    return g_fpga_drv->get_main_board_fpga_type(fpga_index, buf, PAGE_SIZE);
}

static ssize_t fpga_fw_version_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_firmware_version);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    return g_fpga_drv->get_main_board_fpga_firmware_version(fpga_index, buf, PAGE_SIZE);
}

static ssize_t fpga_board_version_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_board_version);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    return g_fpga_drv->get_main_board_fpga_board_version(fpga_index, buf, PAGE_SIZE);
}

static ssize_t fpga_test_reg_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_test_reg);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    return g_fpga_drv->get_main_board_fpga_test_reg(fpga_index, buf, PAGE_SIZE);
}

static ssize_t fpga_test_reg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int fpga_index, value;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->set_main_board_fpga_test_reg);

    fpga_index = obj->index;
    sscanf(buf, "0x%x", &value);
    ret = g_fpga_drv->set_main_board_fpga_test_reg(fpga_index, value);
    if (ret < 0) {
        FPGA_ERR("set fpga%u test reg failed, value:0x%x, ret: %d.\n", fpga_index, value, ret);
        return ret;
    }
    FPGA_DBG("set fpga%u test reg success, value: 0x%x.\n", fpga_index, value);
    return count;
}

/************************************fpga dir and attrs*******************************************/
static struct switch_attribute fpga_number_att = __ATTR(number, S_IRUGO, fpga_number_show, NULL);

static struct attribute *fpga_dir_attrs[] = {
    &fpga_number_att.attr,
    NULL,
};

static struct attribute_group fpga_root_attr_group = {
    .attrs = fpga_dir_attrs,
};

/*******************************fpga[1-n] dir and attrs*******************************************/
static struct switch_attribute fpga_alias_attr = __ATTR(alias, S_IRUGO, fpga_alias_show, NULL);
static struct switch_attribute fpga_type_attr = __ATTR(type, S_IRUGO, fpga_type_show, NULL);
static struct switch_attribute fpga_fw_version_attr = __ATTR(firmware_version, S_IRUGO, fpga_fw_version_show, NULL);
static struct switch_attribute fpga_board_version_attr = __ATTR(board_version, S_IRUGO, fpga_board_version_show, NULL);
static struct switch_attribute fpga_test_reg_attr = __ATTR(reg_test, S_IRUGO | S_IWUSR, fpga_test_reg_show, fpga_test_reg_store);

static struct attribute *fpga_attrs[] = {
    &fpga_alias_attr.attr,
    &fpga_type_attr.attr,
    &fpga_fw_version_attr.attr,
    &fpga_board_version_attr.attr,
    &fpga_test_reg_attr.attr,
    NULL,
};

static struct attribute_group fpga_attr_group = {
    .attrs = fpga_attrs,
};

static int fpga_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct fpga_obj_s *curr_fpga;

    curr_fpga = &g_fpga.fpga[index - 1];
    if (curr_fpga->obj) {
        sysfs_remove_group(&curr_fpga->obj->kobj, &fpga_attr_group);
        switch_kobject_delete(&curr_fpga->obj);
        FPGA_DBG("delete fpga%u dir and attrs success.\n", index);
    }
    return 0;
}

static int fpga_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[8];
    struct fpga_obj_s *curr_fpga;

    curr_fpga = &g_fpga.fpga[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "fpga%u", index);
    curr_fpga->obj = switch_kobject_create(name, parent);
    if (!curr_fpga->obj) {
        FPGA_ERR("create %s object error!\n", name);
        return -EBADRQC;
    }
    curr_fpga->obj->index = index;
    if (sysfs_create_group(&curr_fpga->obj->kobj, &fpga_attr_group) != 0) {
        FPGA_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_fpga->obj);
        return -EBADRQC;
    }
    FPGA_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int fpga_sub_create_kobj_and_attrs(struct kobject *parent, int fpga_num)
{
    unsigned int fpga_index, i;

    g_fpga.fpga = kzalloc(sizeof(struct fpga_obj_s) * fpga_num, GFP_KERNEL);
    if (!g_fpga.fpga) {
        FPGA_ERR("kzalloc g_fpga.fpga error, fpga number = %d.\n", fpga_num);
        return -ENOMEM;
    }

    for (fpga_index = 1; fpga_index <= fpga_num; fpga_index++) {
        if (fpga_sub_single_create_kobj_and_attrs(parent, fpga_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = fpga_index; i > 0; i--) {
        fpga_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_fpga.fpga);
    g_fpga.fpga = NULL;
    return -EBADRQC;
}

/* create fpga[1-n] directory and attributes*/
static int fpga_sub_create(void)
{
    int ret;

    ret = fpga_sub_create_kobj_and_attrs(&g_fpga_obj->kobj, g_fpga.fpga_number);
    return ret;
}

/* delete fpga[1-n] directory and attributes*/
static void fpga_sub_remove(void)
{
    unsigned int fpga_index;

    if (g_fpga.fpga) {
        for (fpga_index = g_fpga.fpga_number; fpga_index > 0; fpga_index--) {
            fpga_sub_single_remove_kobj_and_attrs(fpga_index);
        }
        kfree(g_fpga.fpga);
        g_fpga.fpga = NULL;
    }
    g_fpga.fpga_number = 0;
    return;
}

/* create fpga directory and number attributes */
static int fpga_root_create(void)
{
    g_fpga_obj = switch_kobject_create("fpga", NULL);
    if (!g_fpga_obj) {
        FPGA_ERR("switch_kobject_create fpga error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_fpga_obj->kobj, &fpga_root_attr_group) != 0) {
        switch_kobject_delete(&g_fpga_obj);
        FPGA_ERR("create fpga dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete fpga directory and number attributes */
static void fpga_root_remove(void)
{
    if (g_fpga_obj) {
        sysfs_remove_group(&g_fpga_obj->kobj, &fpga_root_attr_group);
        switch_kobject_delete(&g_fpga_obj);
    }

    return;
}

int s3ip_sysfs_fpga_drivers_register(struct s3ip_sysfs_fpga_drivers_s *drv)
{
    int ret, fpga_num;

    FPGA_INFO("s3ip_sysfs_fpga_drivers_register...\n");
    if (g_fpga_drv) {
        FPGA_ERR("g_fpga_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_fpga_number);
    g_fpga_drv = drv;

    fpga_num = g_fpga_drv->get_main_board_fpga_number();
    if (fpga_num <= 0) {
        FPGA_ERR("fpga number: %d, don't need to create fpga dirs and attrs.\n", fpga_num);
        g_fpga_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_fpga, sizeof(struct fpga_s));
    g_fpga.fpga_number = fpga_num;
    ret = fpga_root_create();
    if (ret < 0) {
        FPGA_ERR("create fpga root dir and attrs failed, ret: %d\n", ret);
        g_fpga_drv = NULL;
        return ret;
    }
    ret = fpga_sub_create();
    if (ret < 0) {
        FPGA_ERR("create fpga sub dir and attrs failed, ret: %d\n", ret);
        fpga_root_remove();
        g_fpga_drv = NULL;
        return ret;
    }
    FPGA_INFO("s3ip_sysfs_fpga_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_fpga_drivers_unregister(void)
{
    if (g_fpga_drv) {
        fpga_sub_remove();
        fpga_root_remove();
        g_fpga_drv = NULL;
        FPGA_DBG("s3ip_sysfs_fpga_drivers_unregister success.\n");
    }

    return;
}

EXPORT_SYMBOL(s3ip_sysfs_fpga_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_fpga_drivers_unregister);
module_param(g_fpga_loglevel, int, 0644);
MODULE_PARM_DESC(g_fpga_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
