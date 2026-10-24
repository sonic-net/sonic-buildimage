/*
 * cabletray_sysfs.c
 *
 * This module create cabletray kobjects and attributes in /sys/s3ip/cabletray
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "cabletray_sysfs.h"

static int g_cabletray_loglevel = 0;

#define CABLETRAY_INFO(fmt, args...) do {                                        \
    if (g_cabletray_loglevel & INFO) { \
        printk(KERN_INFO "[CABLETRAY_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CABLETRAY_ERR(fmt, args...) do {                                        \
    if (g_cabletray_loglevel & ERR) { \
        printk(KERN_ERR "[CABLETRAY_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define CABLETRAY_DBG(fmt, args...) do {                                        \
    if (g_cabletray_loglevel & DBG) { \
        printk(KERN_DEBUG "[CABLETRAY_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct cabletray_obj_s {
    struct bin_attribute bin;
    int cabletray_creat_bin_flag;
    struct switch_obj *obj;
};

struct cabletray_s {
    unsigned int cabletray_number;
    struct cabletray_obj_s *cabletray;
};

static struct cabletray_s g_cabletray;
static struct switch_obj *g_cabletray_obj = NULL;
static struct s3ip_sysfs_cabletray_drivers_s *g_cabletray_drv = NULL;

static ssize_t cabletray_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_cabletray.cabletray_number);
}

static ssize_t cabletray_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int cabletray_index;
    struct switch_device_attribute *cabletray_attr;

    check_p(g_cabletray_drv);
    check_p(g_cabletray_drv->get_cabletray_attr);

    cabletray_index = obj->index;
    CABLETRAY_DBG("cabletray index: %u\n", cabletray_index);
    cabletray_attr = to_switch_device_attr(attr);
    check_p(cabletray_attr);

    return g_cabletray_drv->get_cabletray_attr(cabletray_index, cabletray_attr->type, buf, PAGE_SIZE);
}

/************************************cabletray dir and attrs*******************************************/
static struct switch_attribute cabletray_number_att = __ATTR(number, S_IRUGO, cabletray_number_show, NULL);

static struct attribute *cabletray_dir_attrs[] = {
    &cabletray_number_att.attr,
    NULL,
};

static struct attribute_group cabletray_root_attr_group = {
    .attrs = cabletray_dir_attrs,
};

/*******************************cabletray dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_ALIAS_E);
static SWITCH_DEVICE_ATTR(name, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_NAME_E);
static SWITCH_DEVICE_ATTR(manufacturer, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_VENDOR_E);
static SWITCH_DEVICE_ATTR(sn, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_SN_E);
static SWITCH_DEVICE_ATTR(pn, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_PN_E);
static SWITCH_DEVICE_ATTR(version, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_VERSION_E);
static SWITCH_DEVICE_ATTR(slotid, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_SLOTID_E);
static SWITCH_DEVICE_ATTR(rack_sn, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_RACK_SN_E);
static SWITCH_DEVICE_ATTR(uid, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_UID_E);
static SWITCH_DEVICE_ATTR(h_location, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_H_LOCATION_E);
static SWITCH_DEVICE_ATTR(v_location, S_IRUGO, cabletray_attr_show, NULL, DFD_CABLETRAY_V_LOCATION_E);

static struct attribute *cabletray_attrs[] = {
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_name.switch_attr.attr,
    &switch_dev_attr_manufacturer.switch_attr.attr,
    &switch_dev_attr_sn.switch_attr.attr,
    &switch_dev_attr_pn.switch_attr.attr,
    &switch_dev_attr_slotid.switch_attr.attr,
    &switch_dev_attr_version.switch_attr.attr,
    &switch_dev_attr_rack_sn.switch_attr.attr,
    &switch_dev_attr_uid.switch_attr.attr,
    &switch_dev_attr_h_location.switch_attr.attr,
    &switch_dev_attr_v_location.switch_attr.attr,
    NULL,
};

static struct attribute_group cabletray_attr_group = {
    .attrs = cabletray_attrs,
};

static ssize_t cabletray_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
    char *buf, loff_t offset, size_t count)
{
    struct switch_obj *cabletray_obj;
    ssize_t rd_len;
    unsigned int cabletray_index;

    check_p(g_cabletray_drv);
    check_p(g_cabletray_drv->read_cabletray_eeprom_data);

    cabletray_obj = to_switch_obj(kobj);
    cabletray_index = cabletray_obj->index;
    mem_clear(buf,  count);
    rd_len = g_cabletray_drv->read_cabletray_eeprom_data(cabletray_index, buf, offset, count);
    if (rd_len < 0) {
        CABLETRAY_ERR("read cabletray%u eeprom data error, offset: 0x%llx, read len: %zu, ret: %zd.\n",
        cabletray_index, offset, count, rd_len);
        return rd_len;
    }

    CABLETRAY_DBG("read cabletray%u eeprom data success, offset:0x%llx, read len:%zu, really read len:%zd.\n",
    cabletray_index, offset, count, rd_len);

    return rd_len;
}

/* create cabletray* eeprom attributes */
static int cabletray_sub_single_create_eeprom_attrs(unsigned int index)
{
    int ret, eeprom_size;
    struct cabletray_obj_s *curr_cabletray;

    check_p(g_cabletray_drv->get_cabletray_eeprom_size);
    eeprom_size = g_cabletray_drv->get_cabletray_eeprom_size(index);
    if (eeprom_size <= 0) {
        CABLETRAY_INFO("cabletray%u, eeprom_size: %d, don't need to creat eeprom attr.\n",
            index, eeprom_size);
        return 0;
    }

    curr_cabletray = &g_cabletray.cabletray[index - 1];
    sysfs_bin_attr_init(&curr_cabletray->bin);
    curr_cabletray->bin.attr.name = "eeprom";
    curr_cabletray->bin.attr.mode = 0444;
    curr_cabletray->bin.read = cabletray_eeprom_read;
    curr_cabletray->bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&curr_cabletray->obj->kobj, &curr_cabletray->bin);
    if (ret) {
        CABLETRAY_ERR("cabletray%u, create eeprom bin error, ret: %d. \n", index, ret);
        return -EBADRQC;
    }

    CABLETRAY_DBG("cabletray%u, create bin file success, eeprom size:%d.\n", index, eeprom_size);
    curr_cabletray->cabletray_creat_bin_flag = 1;
    return 0;
}

static int cabletray_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct cabletray_obj_s *curr_cabletray;

    curr_cabletray = &g_cabletray.cabletray[index - 1];
    if (curr_cabletray->obj) {
        if (curr_cabletray->cabletray_creat_bin_flag) {
            sysfs_remove_bin_file(&curr_cabletray->obj->kobj, &curr_cabletray->bin);
            curr_cabletray->cabletray_creat_bin_flag = 0;
        }
        sysfs_remove_group(&curr_cabletray->obj->kobj, &cabletray_attr_group);
        switch_kobject_delete(&curr_cabletray->obj);
        CABLETRAY_DBG("delete cabletray%u dir and attrs success.\n", index);
    }
    return 0;
}

static int cabletray_sub_single_create_kobj(struct kobject *parent, unsigned int index)
{

    char name[CABLETRAY_NAME_LEN];
    struct cabletray_obj_s *curr_cabletray;

    curr_cabletray = &g_cabletray.cabletray[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "cabletray%u", index);
    curr_cabletray->obj = switch_kobject_create(name, parent);
    if (!curr_cabletray->obj) {
        CABLETRAY_ERR("create %s object error!\n", name);
        return -ENOMEM;
    }

    curr_cabletray->obj->index = index;
    if (sysfs_create_group(&curr_cabletray->obj->kobj, &cabletray_attr_group) != 0) {
        CABLETRAY_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_cabletray->obj);
        return -EBADRQC;
    }

    CABLETRAY_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int cabletray_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    int ret;

    ret = cabletray_sub_single_create_kobj(parent, index);
    if (ret < 0) {
        CABLETRAY_ERR("create cabletray%d dir error.\n", index);
        return ret;
    }

    cabletray_sub_single_create_eeprom_attrs(index);
    return 0;
}

/* create cabletray[1-n] directory and attributes */
static int cabletray_sub_create_kobj_and_attrs(struct kobject *parent, int cabletray_num)
{
    unsigned int cabletray_index, i;

    g_cabletray.cabletray = kzalloc(sizeof(struct cabletray_obj_s) * cabletray_num, GFP_KERNEL);
    if (!g_cabletray.cabletray) {
        CABLETRAY_ERR("kzalloc cabletray.cabletray error, cabletray number: %d.\n", cabletray_num);
        return -ENOMEM;
    }

    for (cabletray_index = 1; cabletray_index <= cabletray_num; cabletray_index++) {
        if (cabletray_sub_single_create_kobj_and_attrs(parent, cabletray_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = cabletray_index - 1; i > 0; i--) {
        cabletray_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_cabletray.cabletray);
    g_cabletray.cabletray = NULL;
    return -EBADRQC;
}

static int cabletray_sub_create(void)
{
    int ret;

    ret = cabletray_sub_create_kobj_and_attrs(&g_cabletray_obj->kobj, g_cabletray.cabletray_number);
    return ret;
}

/* delete cabletray[1-n] directory and attributes */
static void cabletray_sub_remove(void)
{
    unsigned int cabletray_index;

    if (g_cabletray.cabletray) {
        for (cabletray_index = g_cabletray.cabletray_number; cabletray_index > 0; cabletray_index--) {
            cabletray_sub_single_remove_kobj_and_attrs(cabletray_index);
        }
        kfree(g_cabletray.cabletray);
        g_cabletray.cabletray = NULL;
    }
    g_cabletray.cabletray_number = 0;

    return;
}

/* create cabletray directory and number attributes */
static int cabletray_root_create(void)
{
    g_cabletray_obj = switch_kobject_create("cabletray", NULL);
    if (!g_cabletray_obj) {
        CABLETRAY_ERR("switch_kobject_create cabletray error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_cabletray_obj->kobj, &cabletray_root_attr_group) != 0) {
        switch_kobject_delete(&g_cabletray_obj);
        CABLETRAY_ERR("create cabletray dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete cabletray directory and number attributes */
static void cabletray_root_remove(void)
{
    if (g_cabletray_obj) {
        sysfs_remove_group(&g_cabletray_obj->kobj, &cabletray_root_attr_group);
        switch_kobject_delete(&g_cabletray_obj);
        CABLETRAY_DBG("delete cabletray dir and attrs success.\n");
    }
    return;
}

int s3ip_sysfs_cabletray_drivers_register(struct s3ip_sysfs_cabletray_drivers_s *drv)
{
    int ret, cabletray_num;

    CABLETRAY_INFO("s3ip_sysfs_cabletray_drivers_register...\n");
    if (g_cabletray_drv) {
        CABLETRAY_ERR("g_cabletray_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_cabletray_number);
    g_cabletray_drv = drv;

    cabletray_num = g_cabletray_drv->get_cabletray_number();
    if (cabletray_num <= 0) {
        CABLETRAY_ERR("cabletray number: %d, don't need to create cabletray dirs and attrs.\n", cabletray_num);
        g_cabletray_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_cabletray, sizeof(struct cabletray_s));
    g_cabletray.cabletray_number = cabletray_num;
    ret = cabletray_root_create();
    if (ret < 0) {
        CABLETRAY_ERR("create cabletray root dir and attrs failed, ret: %d\n", ret);
        g_cabletray_drv = NULL;
        return ret;
    }

    ret = cabletray_sub_create();
    if (ret < 0) {
        CABLETRAY_ERR("create cabletray sub dir and attrs failed, ret: %d\n", ret);
        cabletray_root_remove();
        g_cabletray_drv = NULL;
        return ret;
    }

    CABLETRAY_INFO("s3ip_sysfs_cabletray_drivers_register success.\n");
    return 0;
}

void s3ip_sysfs_cabletray_drivers_unregister(void)
{
    if (g_cabletray_drv) {
        cabletray_sub_remove();
        cabletray_root_remove();
        g_cabletray_drv = NULL;
        CABLETRAY_DBG("s3ip_sysfs_cabletray_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_cabletray_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_cabletray_drivers_unregister);
module_param(g_cabletray_loglevel, int, 0644);
MODULE_PARM_DESC(g_cabletray_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
