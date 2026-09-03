/*
 * dpu_sysfs.c
 *
 * This module create dpu kobjects and attributes in /sys/s3ip/dpu
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>
#include <linux/fs.h>
#include "switch.h"
#include "dpu_sysfs.h"

static int g_dpu_loglevel = 0x0;

#define DPU_SYSFS_DEV_NUM_MAX          (256)

#define DPU_INFO(fmt, args...) do {                                        \
    if (g_dpu_loglevel & INFO) { \
        printk(KERN_INFO "[DPU_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DPU_ERR(fmt, args...) do {                                        \
    if (g_dpu_loglevel & ERR) { \
        printk(KERN_ERR "[DPU_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DPU_DBG(fmt, args...) do {                                        \
    if (g_dpu_loglevel & DBG) { \
        printk(KERN_DEBUG "[DPU_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct dpu_info_obj_s {
    struct switch_obj *obj;
};

struct dpu_obj_s {
    unsigned int dpu_fw_num;
    unsigned int dpu_temp_num;
    struct dpu_info_obj_s *dpu_fw;
    struct dpu_info_obj_s *dpu_temp;
    struct switch_obj *obj;
};

struct dpu_s {
    unsigned int dpu_number;
    struct dpu_obj_s *dpu;
};

static struct dpu_s g_dpu;
static struct switch_obj *g_dpu_obj = NULL;
static struct s3ip_sysfs_dpu_drivers_s *g_dpu_drv = NULL;

static ssize_t dpu_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_dpu.dpu_number);
}

static ssize_t dpu_fw_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    if (obj && obj->index >= 1 && obj->index <= g_dpu.dpu_number) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_dpu.dpu[obj->index - 1].dpu_fw_num);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t dpu_temp_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    if (obj && obj->index >= 1 && obj->index <= g_dpu.dpu_number) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_dpu.dpu[obj->index - 1].dpu_temp_num);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t dpu_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int dpu_index;
    struct switch_device_attribute *dpu_attr;

    check_p(g_dpu_drv);
    check_p(g_dpu_drv->get_dpu_attr);

    dpu_index = obj->index;
    DPU_DBG("dpu index: %u\n", dpu_index);
    dpu_attr = to_switch_device_attr(attr);
    check_p(dpu_attr);

    return g_dpu_drv->get_dpu_attr(dpu_index, dpu_attr->type, buf, PAGE_SIZE);
}

static ssize_t dpu_fw_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int dpu_index, fw_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *dpu_attr;

    check_p(g_dpu_drv);
    check_p(g_dpu_drv->get_dpu_fw_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    dpu_index = g_obj->index;
    fw_index = obj->index;

    DPU_DBG("dpu_index: %u, fw_index: %u\n", dpu_index, fw_index);

    dpu_attr = to_switch_device_attr(attr);
    check_p(dpu_attr);

    return g_dpu_drv->get_dpu_fw_attr(dpu_index, fw_index, dpu_attr->type, buf, PAGE_SIZE);
}

static ssize_t dpu_subset_monitor_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int dpu_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *dpu_attr;

    check_p(g_dpu_drv);

    g_obj = to_switch_obj(obj->kobj.parent);
    dpu_index = g_obj->index;

    DPU_DBG("dpu_index: %u\n", dpu_index);

    dpu_attr = to_switch_device_attr(attr);
    check_p(dpu_attr);

    return g_dpu_drv->get_dpu_attr(dpu_index, dpu_attr->type, buf, PAGE_SIZE);
}

static ssize_t dpu_temp_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int dpu_index, temp_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *dpu_attr;

    check_p(g_dpu_drv);
    check_p(g_dpu_drv->get_dpu_temp_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    dpu_index = g_obj->index;
    temp_index = obj->index;

    DPU_DBG("dpu_index: %u, temp_index: %u\n", dpu_index, temp_index);

    dpu_attr = to_switch_device_attr(attr);
    check_p(dpu_attr);

    return g_dpu_drv->get_dpu_temp_attr(dpu_index, temp_index, dpu_attr->type, buf, PAGE_SIZE);
}



/************************************dpu dir and attrs*******************************************/
static struct switch_attribute dpu_number_att = __ATTR(number, S_IRUGO, dpu_number_show, NULL);

static struct attribute *dpu_dir_attrs[] = {
    &dpu_number_att.attr,
    NULL,
};

static struct attribute_group dpu_root_attr_group = {
    .attrs = dpu_dir_attrs,
};

/*******************************dpu[1-n] dir and attrs*******************************************/
static struct switch_attribute dpu_fw_num_attr = __ATTR(num_misc_fw, S_IRUGO, dpu_fw_number_show, NULL);
static struct switch_attribute dpu_temp_num_attr = __ATTR(num_temp_sensors, S_IRUGO, dpu_temp_number_show, NULL);
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_NAME_E);
static SWITCH_DEVICE_ATTR(pcie_vendorid, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_VENDOR_E);
static SWITCH_DEVICE_ATTR(pcie_deviceid, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_DEVICE_E);
static SWITCH_DEVICE_ATTR(serial_number, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_SN_E);
static SWITCH_DEVICE_ATTR(part_number, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_PN_E);
static SWITCH_DEVICE_ATTR(mac, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_MAC_E);
static SWITCH_DEVICE_ATTR(monitor_flag, S_IRUGO, dpu_attr_show, NULL, DFD_DPU_ID_MONITOR_FLAG_E);

static struct attribute *dpu_attrs[] = {
    &dpu_fw_num_attr.attr,
    &dpu_temp_num_attr.attr,
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_pcie_vendorid.switch_attr.attr,
    &switch_dev_attr_pcie_deviceid.switch_attr.attr,
    &switch_dev_attr_serial_number.switch_attr.attr,
    &switch_dev_attr_part_number.switch_attr.attr,
    &switch_dev_attr_mac.switch_attr.attr,
    &switch_dev_attr_monitor_flag.switch_attr.attr,
    NULL,
};

static struct attribute_group dpu_attr_group = {
    .attrs = dpu_attrs,
};


/*******************************dpu fw[1-n] dir and attrs begin ****************************************/
static SWITCH_DEVICE_ATTR_PRE(misc_fw, alias, S_IRUGO, dpu_fw_attr_show, NULL, DFD_DPU_FW_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(misc_fw, firmware_version, S_IRUGO, dpu_fw_attr_show, NULL, DFD_DPU_FW_VERSION_E);
static SWITCH_DEVICE_ATTR_PRE(misc_fw, support_upgrade, S_IRUGO, dpu_fw_attr_show, NULL, DFD_DPU_FW_SUP_UP_E);
static SWITCH_DEVICE_ATTR_PRE(misc_fw, upgrade_active_type, S_IRUGO, dpu_fw_attr_show, NULL, DFD_DPU_FW_UP_TYPE_E);
static SWITCH_DEVICE_ATTR_PRE(misc_fw, monitor_flag, S_IRUGO, dpu_subset_monitor_show, NULL, DFD_DPU_ID_MONITOR_FLAG_E);


static struct attribute *dpu_fw_attrs[] = {
    &switch_dev_attr_misc_fw_alias.switch_attr.attr,
    &switch_dev_attr_misc_fw_firmware_version.switch_attr.attr,
    &switch_dev_attr_misc_fw_support_upgrade.switch_attr.attr,
    &switch_dev_attr_misc_fw_upgrade_active_type.switch_attr.attr,
    &switch_dev_attr_misc_fw_monitor_flag.switch_attr.attr,
    NULL,
};

static struct attribute_group dpu_fw_attr_group = {
    .attrs = dpu_fw_attrs,
};
/*******************************dpu fw[1-n] dir and attrs end ******************************************/

/*******************************dpu temp[1-n] dir and attrs begin ****************************************/


static SWITCH_DEVICE_ATTR_PRE(temp, alias, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(temp, value, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_INPUT_E);
static SWITCH_DEVICE_ATTR_PRE(temp, max, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MAX_E);
static SWITCH_DEVICE_ATTR_PRE(temp, min, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MIN_E);
static SWITCH_DEVICE_ATTR_PRE(temp, high, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(temp, low, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(temp, notice_high, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(temp, notice_low, S_IRUGO, dpu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(temp, monitor_flag, S_IRUGO, dpu_subset_monitor_show, NULL, DFD_DPU_ID_MONITOR_FLAG_E);

static struct attribute *dpu_temp_attrs[] = {
    &switch_dev_attr_temp_alias.switch_attr.attr,
    &switch_dev_attr_temp_value.switch_attr.attr,
    &switch_dev_attr_temp_max.switch_attr.attr,
    &switch_dev_attr_temp_min.switch_attr.attr,
    &switch_dev_attr_temp_high.switch_attr.attr,
    &switch_dev_attr_temp_low.switch_attr.attr,
    &switch_dev_attr_temp_notice_high.switch_attr.attr,
    &switch_dev_attr_temp_notice_low.switch_attr.attr,
    &switch_dev_attr_temp_monitor_flag.switch_attr.attr,
    NULL,
};

static struct attribute_group dpu_temp_attr_group = {
    .attrs = dpu_temp_attrs,
};
/*******************************dpu temp[1-n] dir and attrs end ******************************************/


static int dpu_single_info_create_kobj_and_attrs(struct dpu_obj_s *curr_dpu,
                                                            struct dpu_info_obj_s *curr_dpu_info,
                                                            unsigned int fw_index,
                                                            unsigned char *fw_prefix,
                                                            struct attribute_group *fw_attr_group)
{
    char name[DIR_NAME_MAX_LEN];
    struct switch_obj *sensor_kobj;
    int ret;

    if (!curr_dpu || !curr_dpu->obj || !fw_attr_group || !fw_prefix || fw_index == 0 ) {
        DPU_ERR("Invalid parameters: curr_dpu=%p, fw_index=%u, fw_prefix=%p, fw_attr_group=%p\n",
                curr_dpu, fw_index, fw_prefix, fw_attr_group);
        return -EINVAL;
    }

    mem_clear(name, sizeof(name));

    ret = snprintf(name, sizeof(name), "%s%u", fw_prefix, fw_index);
    if (ret >= sizeof(name) || ret < 0) {
        DPU_ERR("Name generation failed for fw_index=%u\n", fw_index);
        return -EINVAL;
    }

    sensor_kobj = switch_kobject_create(name, &curr_dpu->obj->kobj);
    if (!sensor_kobj) {
        DPU_ERR("Failed to create %s kobject for DPU%u\n",
                name, curr_dpu->obj->index);
        return -ENOMEM;
    }

    curr_dpu_info->obj = sensor_kobj;
    curr_dpu_info->obj->index = fw_index;

    ret = sysfs_create_group(&curr_dpu_info->obj->kobj, fw_attr_group);
    if (ret != 0) {
        DPU_ERR("Failed to create attributes for DPU%u/%s: %d\n",
                curr_dpu->obj->index, name, ret);

        switch_kobject_delete(&curr_dpu_info->obj);
        curr_dpu_info->obj = NULL;

        return (ret == -ENOMEM) ? -ENOMEM : -EBADRQC;
    }

    DPU_DBG("Successfully created DPU%u/%s\n",
            curr_dpu->obj->index, name);

    return 0;
}

static void dpu_info_cleanup(struct dpu_obj_s *curr_dpu,
                                        struct dpu_info_obj_s *curr_dpu_info,
                                        struct attribute_group *fw_attr_group,
                                        unsigned int fw_index)
{
    if (!curr_dpu || !curr_dpu_info || !fw_attr_group || fw_index == 0) {
        DPU_ERR("cleanup: invalid param. dpu=%p, sensor=%p, group=%p, index=%u\n",
                curr_dpu, curr_dpu_info, fw_attr_group, fw_index);
        return;
    }

    if (curr_dpu_info->obj) {
        sysfs_remove_group(&curr_dpu_info->obj->kobj, fw_attr_group);
        switch_kobject_delete(&curr_dpu_info->obj);
        curr_dpu_info->obj = NULL;
        DPU_DBG("Cleanup sensor index %u: removed group and kobject\n", fw_index);
    }

    return;
}

static int dpu_info_create_kobj_and_attrs(struct dpu_obj_s *curr_dpu)
{
    int ret;
    unsigned int i, dpu_info_num;
    unsigned int fw_index, temp_index;

    dpu_info_num = curr_dpu->dpu_fw_num;
    curr_dpu->dpu_fw = kcalloc(dpu_info_num, sizeof(struct dpu_info_obj_s), GFP_KERNEL);
    if (!curr_dpu->dpu_fw) {
        DPU_ERR("allocate dpu fw objects failed, dpu index: %u, dpu_fw number: %u.\n",
            curr_dpu->obj->index, dpu_info_num);
        return -ENOMEM;
    }

    for (fw_index = 1; fw_index <= dpu_info_num; fw_index++) {
        ret = dpu_single_info_create_kobj_and_attrs(curr_dpu,
                &curr_dpu->dpu_fw[fw_index -1],
                fw_index,
                "misc_fw",
                &dpu_fw_attr_group);
        if (ret) {
            goto fw_error;
        }
    }

    dpu_info_num = curr_dpu->dpu_temp_num;
    curr_dpu->dpu_temp = kcalloc(dpu_info_num, sizeof(struct dpu_info_obj_s), GFP_KERNEL);
    if (!curr_dpu->dpu_temp) {
        DPU_ERR("allocate dpu temp objects failed, dpu index: %u, dpu_temp number: %u.\n",
            curr_dpu->obj->index, dpu_info_num);
        ret = -ENOMEM;
        goto fw_error;
    }

    for (temp_index = 1; temp_index <= dpu_info_num; temp_index++) {
        ret = dpu_single_info_create_kobj_and_attrs(curr_dpu,
                &curr_dpu->dpu_temp[temp_index -1],
                temp_index,
                "temp",
                &dpu_temp_attr_group);
        if (ret) {
            goto temp_error;
        }
    }

    return 0;

temp_error:
    for (i = temp_index - 1; i > 0; i--) {
        dpu_info_cleanup(curr_dpu, &curr_dpu->dpu_temp[i - 1], &dpu_temp_attr_group, i);
    }

    kfree(curr_dpu->dpu_temp);
    curr_dpu->dpu_temp = NULL;

fw_error:
    for (i = fw_index - 1; i > 0; i--) {
        dpu_info_cleanup(curr_dpu, &curr_dpu->dpu_fw[i - 1], &dpu_fw_attr_group, i);
    }

    kfree(curr_dpu->dpu_fw);
    curr_dpu->dpu_fw = NULL;

    return ret;
}


static void dpu_info_remove_kobj_and_attrs(struct dpu_obj_s *curr_dpu)
{
    unsigned int fw_index, temp_index;
    unsigned int fw_num, temp_num;

    if (curr_dpu->dpu_fw) {
        fw_num = curr_dpu->dpu_fw_num;
        for (fw_index = fw_num; fw_index > 0; fw_index--) {
            dpu_info_cleanup(curr_dpu, &curr_dpu->dpu_fw[fw_index - 1], &dpu_fw_attr_group, fw_index);
        }

        kfree(curr_dpu->dpu_fw);
        curr_dpu->dpu_fw = NULL;
    }

    if (curr_dpu->dpu_temp) {
        temp_num = curr_dpu->dpu_temp_num;
        for (temp_index = temp_num; temp_index > 0; temp_index--) {
            dpu_info_cleanup(curr_dpu, &curr_dpu->dpu_temp[temp_index - 1], &dpu_temp_attr_group, temp_index);
        }

        kfree(curr_dpu->dpu_temp);
        curr_dpu->dpu_temp = NULL;
    }

    return;
}


static int dpu_root_create(void)
{
    g_dpu_obj = switch_kobject_create("dpu", NULL);
    if (!g_dpu_obj) {
        DPU_ERR("create DPU sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_dpu_obj->kobj, &dpu_root_attr_group) != 0) {
        switch_kobject_delete(&g_dpu_obj);
        DPU_ERR("create DPU root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int dpu_device_create(int index)
{
    char name[32];
    struct dpu_obj_s *dpu;
    int ret;

    dpu = &g_dpu.dpu[index - 1];

    snprintf(name, sizeof(name), "dpu%u", index);
    dpu->obj = switch_kobject_create(name, &g_dpu_obj->kobj);
    if (!dpu->obj) {
        DPU_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    dpu->obj->index = index;

    ret = sysfs_create_group(&dpu->obj->kobj, &dpu_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&dpu->obj);
        DPU_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    DPU_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int dpu_devices_create(void)
{
    int i, ret;
    struct dpu_obj_s *dpu;

    g_dpu.dpu = kzalloc(sizeof(struct dpu_obj_s) * g_dpu.dpu_number, GFP_KERNEL);
    if (!g_dpu.dpu) {
        DPU_ERR("allocate dpu objects failed.\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_dpu.dpu_number; i++) {
        ret = dpu_device_create(i);
        if (ret) {
            DPU_ERR("create dpu%d dir and attrs failed, ret: %d\n", i, ret);
            goto err_remove_devices;
        }
    }

    return 0;

err_remove_devices:
    while (--i >= 1) {
        dpu = &g_dpu.dpu[i - 1];
        if (dpu->obj) {
            sysfs_remove_group(&dpu->obj->kobj, &dpu_attr_group);
            switch_kobject_delete(&dpu->obj);
        }
    }

    kfree(g_dpu.dpu);
    g_dpu.dpu = NULL;

    return ret;
}

static int dpu_info_create(void)
{
    int dpu_num, dpu_fw_num, dpu_temp_num;
    unsigned int dpu_index, i;
    struct dpu_obj_s *curr_dpu;

    dpu_num = g_dpu.dpu_number;
    if (dpu_num <= 0) {
        DPU_DBG("dpu number: %d, skip to create dpu_fw dirs and attrs.\n", dpu_num);
        return 0;
    }

    dpu_fw_num = g_dpu_drv->get_dpu_fw_number ? g_dpu_drv->get_dpu_fw_number() : 0;
    if (dpu_fw_num <= 0) {
        DPU_DBG("dpu_fw_num: %d, don't need to create dpu_fw dirs and attrs.\n", dpu_fw_num);
        dpu_fw_num = 0;
    }

    dpu_temp_num = g_dpu_drv->get_dpu_temp_number ? g_dpu_drv->get_dpu_temp_number() : 0;
    if (dpu_temp_num <= 0) {
        DPU_DBG("dpu_temp_num: %d, don't need to create dpu_temp dirs and attrs.\n", dpu_temp_num);
        dpu_temp_num = 0;
    }

    for(dpu_index = 1; dpu_index <= dpu_num; dpu_index++) {
        curr_dpu = &g_dpu.dpu[dpu_index - 1];
        curr_dpu->dpu_fw_num = dpu_fw_num;
        curr_dpu->dpu_temp_num = dpu_temp_num;

        DPU_DBG("DPU[%d]: dpu_fw_num=%d, dpu_temp_num=%d\n", dpu_index, curr_dpu->dpu_fw_num, curr_dpu->dpu_temp_num);

        if(dpu_info_create_kobj_and_attrs(curr_dpu) != 0) {
            goto error;
        }
    }

    return 0;

error:
    for(i = dpu_index - 1; i > 0; i--) {
        curr_dpu = &g_dpu.dpu[i - 1];
        dpu_info_remove_kobj_and_attrs(curr_dpu);
    }
    return -EBADRQC;
}

static void dpu_device_remove(int index)
{
    struct dpu_obj_s *dpu = &g_dpu.dpu[index - 1];
    if (dpu->obj) {
        sysfs_remove_group(&dpu->obj->kobj, &dpu_attr_group);
        switch_kobject_delete(&dpu->obj);
        DPU_DBG("removed dpu%d\n", index);
    }
}

static void dpu_devices_remove(void)
{
    int i;

    if (g_dpu.dpu) {
        for (i = g_dpu.dpu_number; i > 0; i--) {
            dpu_device_remove(i);
        }

        kfree(g_dpu.dpu);
        g_dpu.dpu = NULL;
    }

    g_dpu.dpu_number = 0;
}

static void dpu_sysfs_remove(void)
{
    dpu_devices_remove();

    if (g_dpu_obj) {
        sysfs_remove_group(&g_dpu_obj->kobj, &dpu_root_attr_group);
        switch_kobject_delete(&g_dpu_obj);
        g_dpu_obj = NULL;
    }
}

static int dpu_sysfs_create(void)
{
    int ret;

    ret = dpu_root_create();
    if (ret) {
        return ret;
    }

    ret = dpu_devices_create();
    if (ret) {
        dpu_sysfs_remove();
        DPU_ERR("create dpu dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    ret = dpu_info_create();
    if (ret) {
        DPU_ERR("create dpu info dir and attrs failed, ret: %d\n", ret);
        dpu_sysfs_remove();
        g_dpu_drv = NULL;

        return ret;
    }

    return 0;
}

int s3ip_sysfs_dpu_drivers_register(struct s3ip_sysfs_dpu_drivers_s *drv)
{
    int dpu_number;
    int ret;

    DPU_INFO("s3ip_sysfs_dpu_drivers_register...\n");
    if (g_dpu_drv) {
        DPU_ERR("g_dpu_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_dpu_number);
    g_dpu_drv = drv;

    dpu_number = drv->get_dpu_number();
    if (dpu_number <= 0 || dpu_number > DPU_SYSFS_DEV_NUM_MAX) {
        g_dpu_drv = NULL;
        DPU_ERR("Invalid dpu number: %d, max: %u\n", dpu_number, DPU_SYSFS_DEV_NUM_MAX);
        return -EINVAL;
    }

    mem_clear(&g_dpu, sizeof(struct dpu_s));

    g_dpu.dpu_number = dpu_number;

    ret = dpu_sysfs_create();
    if (ret) {
        g_dpu_drv = NULL;
        return ret;
    }

    DPU_INFO("Registered DPU driver with %d devices\n", dpu_number);

    return 0;
}

void s3ip_sysfs_dpu_drivers_unregister(void)
{
    if (g_dpu_drv) {
        dpu_sysfs_remove();
        g_dpu_drv = NULL;
        DPU_INFO("DPU driver unregistered\n");
    }
}


EXPORT_SYMBOL(s3ip_sysfs_dpu_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_dpu_drivers_unregister);
module_param(g_dpu_loglevel, int, 0644);
MODULE_PARM_DESC(g_dpu_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
