/*
 * plat_slot.c
 * Original Author: support 2020-02-17
 *
 * This module create sff kobjects and attributes in /sys/wb_plat/slot
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0         support                  2020-02-17         Initial version
 */

#include <linux/slab.h>

#include "./include/plat_switch.h"
#include "./include/sysfs_common.h"

#define SLOT_INFO(fmt, args...) LOG_INFO("slot: ", fmt, ##args)
#define SLOT_ERR(fmt, args...)  LOG_ERR("slot: ", fmt, ##args)
#define SLOT_DBG(fmt, args...)  LOG_DBG("slot: ", fmt, ##args)

struct slot_temp_obj_t{
    struct switch_obj *obj;
};

struct slot_in_obj_t{
    struct switch_obj *obj;
};

struct slot_obj_t{
    unsigned int temp_number;
    unsigned int in_number;
    struct slot_temp_obj_t *temp;
    struct slot_in_obj_t *in;
    struct switch_obj *obj;
};

struct slot_t{
    unsigned int slot_number;
    struct slot_obj_t *slot;
};

static int g_loglevel = 0;
static struct slot_t g_slot;
static struct switch_obj *g_slot_obj = NULL;
static struct switch_drivers_t *g_drv = NULL;

static ssize_t slot_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_slot.slot_number);
}

static ssize_t slot_temp_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;

    index = obj->index;
    SLOT_DBG("slot_temp_number_show,slot index:%d\n",index);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_slot.slot[index-1].temp_number);
}

static ssize_t slot_in_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;

    index = obj->index;
    SLOT_DBG("slot_in_number_show,slot index:%d\n",index);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_slot.slot[index-1].in_number);
}

static ssize_t slot_present_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index;
    int ret;

    slot_index = obj->index;
    SLOT_DBG("slot_present_status_show, slot index:%d\n",slot_index);
    check_p(g_drv);
    check_p(g_drv->get_slot_present_status);

    ret = g_drv->get_slot_present_status(slot_index);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t slot_voltage_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, in_index;
    int ret;
    struct switch_obj *p_obj;
    struct switch_device_attribute *in_attr;

    check_p(g_drv);
    check_p(g_drv->get_voltage_info);

    p_obj = to_switch_obj(obj->kobj.parent);
    check_p(p_obj);

    slot_index = p_obj->index;
    in_index = obj->index;

    in_attr = to_switch_device_attr(attr);
    check_p(in_attr);
    SLOT_DBG("slot_voltage_show, slot index:0x%x, temp index:0x%x, temp type:0x%x\n",slot_index, in_index, in_attr->type);
    ret = g_drv->get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, in_index, in_attr->type, buf);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t slot_temp_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int slot_index, temp_index;
    int ret;
    struct switch_obj *p_obj;
    struct switch_device_attribute *temp_attr;

    check_p(g_drv);
    check_p(g_drv->get_temp_info);

    p_obj = to_switch_obj(obj->kobj.parent);
    check_p(p_obj);

    slot_index = p_obj->index;
    temp_index = obj->index;

    temp_attr = to_switch_device_attr(attr);
    check_p(temp_attr);
    SLOT_DBG("slot_temp_show, slot index:0x%x, temp index:0x%x, temp type:0x%x\n",slot_index, temp_index, temp_attr->type);
    ret = g_drv->get_temp_info(WB_MAIN_DEV_SLOT, slot_index, temp_index, temp_attr->type, buf);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }
    return ret;
}

/************************************slot dir and attrs*******************************************/
static struct switch_attribute slot_number_att = __ATTR(num_slots, S_IRUGO, slot_number_show, NULL);

static struct attribute *slot_dir_attrs[] = {
    &slot_number_att.attr,
    NULL,
};

static struct attribute_group slot_root_attr_group = {
    .attrs = slot_dir_attrs,
};

/*******************************slot1 slot2 dir and attrs*******************************************/
static struct switch_attribute num_temp_sensors_att = __ATTR(num_temp_sensors, S_IRUGO, slot_temp_number_show, NULL);
static struct switch_attribute num_in_sensors_att = __ATTR(num_in_sensors, S_IRUGO, slot_in_number_show, NULL);
static struct switch_attribute slot_present_status_att = __ATTR(present, S_IRUGO, slot_present_status_show, NULL);

static struct attribute *slot_attrs[] = {
    &num_temp_sensors_att.attr,
    &num_in_sensors_att.attr,
    &slot_present_status_att.attr,
    NULL,
};

static struct attribute_group slot_attr_group = {
    .attrs = slot_attrs,
};

/*******************************temp dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(temp_alias, S_IRUGO, slot_temp_show, NULL, WB_SENSOR_ALIAS);
static SWITCH_DEVICE_ATTR(temp_type, S_IRUGO, slot_temp_show, NULL, WB_SENSOR_TYPE);
static SWITCH_DEVICE_ATTR(temp_max, S_IRUGO, slot_temp_show, NULL, WB_SENSOR_MAX);
static SWITCH_DEVICE_ATTR(temp_max_hyst, S_IRUGO, slot_temp_show, NULL, WB_SENSOR_MAX_HYST);
static SWITCH_DEVICE_ATTR(temp_min, S_IRUGO, slot_temp_show, NULL, WB_SENSOR_MIN);
static SWITCH_DEVICE_ATTR(temp_input, S_IRUGO, slot_temp_show, NULL, WB_SENSOR_INPUT);

static struct attribute *temp_attrs[] = {
    &switch_dev_attr_temp_alias.switch_attr.attr,
    &switch_dev_attr_temp_type.switch_attr.attr,
    &switch_dev_attr_temp_max.switch_attr.attr,
    &switch_dev_attr_temp_max_hyst.switch_attr.attr,
    &switch_dev_attr_temp_min.switch_attr.attr,
    &switch_dev_attr_temp_input.switch_attr.attr,
    NULL,
};

static struct attribute_group temp_attr_group = {
    .attrs = temp_attrs,
};

/*******************************Voltage dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(in_alias, S_IRUGO, slot_voltage_show, NULL, WB_SENSOR_ALIAS);
static SWITCH_DEVICE_ATTR(in_type, S_IRUGO, slot_voltage_show, NULL, WB_SENSOR_TYPE);
static SWITCH_DEVICE_ATTR(in_max, S_IRUGO, slot_voltage_show, NULL, WB_SENSOR_MAX);
static SWITCH_DEVICE_ATTR(in_crit, S_IRUGO, slot_voltage_show, NULL, WB_SENSOR_CRIT);
static SWITCH_DEVICE_ATTR(in_min, S_IRUGO, slot_voltage_show, NULL, WB_SENSOR_MIN);
static SWITCH_DEVICE_ATTR(in_input, S_IRUGO, slot_voltage_show, NULL, WB_SENSOR_INPUT);

static struct attribute *in_attrs[] = {
    &switch_dev_attr_in_alias.switch_attr.attr,
    &switch_dev_attr_in_type.switch_attr.attr,
    &switch_dev_attr_in_max.switch_attr.attr,
    &switch_dev_attr_in_crit.switch_attr.attr,
    &switch_dev_attr_in_min.switch_attr.attr,
    &switch_dev_attr_in_input.switch_attr.attr,
    NULL,
};

static struct attribute_group in_attr_group = {
    .attrs = in_attrs,
};

static void slotindex_single_temp_remove_kobj_and_attrs(struct slot_obj_t * curr_slot, unsigned int temp_index)
{

    struct slot_temp_obj_t *curr_temp; /* point to temp1 temp2...*/

    curr_temp = &curr_slot->temp[temp_index - 1];
    if (curr_temp->obj) {
        sysfs_remove_group(&curr_temp->obj->kobj, &temp_attr_group);
        wb_plat_kobject_delete(&curr_temp->obj);
        SLOT_DBG("delete slot:%d temp%d.\n", curr_slot->obj->index, temp_index);
    }
    return;
}

static int slotindex_single_temp_create_kobj_and_attrs(struct slot_obj_t * curr_slot, unsigned int temp_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_temp_obj_t *curr_temp; /* point to temp1 temp2...*/

    curr_temp = &curr_slot->temp[temp_index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "%s%d", TEMP_SYSFS_NAME, temp_index);

    curr_temp->obj = wb_plat_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr_temp->obj) {
        SLOT_ERR("create slot_index:%d, temp%d object error!\n", curr_slot->obj->index, temp_index);
        return -EBADRQC;
    }
    curr_temp->obj->index = temp_index;
    if (sysfs_create_group(&curr_temp->obj->kobj, &temp_attr_group) != 0) {
        SLOT_ERR("create slot_index:%d, temp%d attrs error.\n", curr_slot->obj->index, temp_index);
        wb_plat_kobject_delete(&curr_temp->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot_index:%d, temp%d ok.\n", curr_slot->obj->index, temp_index);
    return 0;
}

static void slotindex_temp_remove_kobj_and_attrs(struct slot_obj_t * curr_slot)
{
    int temp_index;

    for(temp_index = curr_slot->temp_number; temp_index > 0; temp_index--) {
        slotindex_single_temp_remove_kobj_and_attrs(curr_slot, temp_index);
    }

    if(curr_slot->temp) {
        kfree(curr_slot->temp);
        curr_slot->temp = NULL;
        curr_slot->temp_number = 0;
    }
    return;
}

static int slotindex_temp_create_kobj_and_attrs(struct slot_obj_t * curr_slot, int temp_num)
{
    int temp_index, i;

    curr_slot->temp_number = temp_num;
    curr_slot->temp = kzalloc(sizeof(struct slot_temp_obj_t) * temp_num, GFP_KERNEL);
    if (!curr_slot->temp) {
        SLOT_ERR("kzalloc slot temp error, slot index = %d, temp number = %d.\n", curr_slot->obj->index, temp_num);
        return -ENOMEM;
    }

    for (temp_index = 1; temp_index <= temp_num; temp_index++) {
        if (slotindex_single_temp_create_kobj_and_attrs(curr_slot, temp_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = temp_index - 1; i > 0; i--) {
        slotindex_single_temp_remove_kobj_and_attrs(curr_slot, i);
    }

    if (curr_slot->temp) {
        kfree(curr_slot->temp);
        curr_slot->temp = NULL;
        curr_slot->temp_number = 0;
    }
    return -EBADRQC;
}

static void slotindex_single_in_remove_kobj_and_attrs(struct slot_obj_t * curr_slot, unsigned int in_index)
{

    struct slot_in_obj_t *curr_in; /* point to in1 in2...*/

    curr_in = &curr_slot->in[in_index - 1];
    if (curr_in->obj) {
        sysfs_remove_group(&curr_in->obj->kobj, &in_attr_group);
        wb_plat_kobject_delete(&curr_in->obj);
        SLOT_DBG("delete slot:%d in%d.\n", curr_slot->obj->index, in_index);
    }
    return;
}

static int slotindex_single_in_create_kobj_and_attrs(struct slot_obj_t * curr_slot, unsigned int in_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_in_obj_t *curr_in; /* point to in1 in2...*/

    curr_in = &curr_slot->in[in_index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "%s%d", VOLTAGE_SYSFS_NAME, in_index);
    curr_in->obj = wb_plat_kobject_create(name, &curr_slot->obj->kobj);
    if (!curr_in->obj) {
        SLOT_ERR("create slot_index:%d, in%d object error!\n", curr_slot->obj->index, in_index);
        return -EBADRQC;
    }
    curr_in->obj->index = in_index;
    if (sysfs_create_group(&curr_in->obj->kobj, &in_attr_group) != 0) {
        SLOT_ERR("create slot_index:%d, in%d attrs error.\n", curr_slot->obj->index, in_index);
        wb_plat_kobject_delete(&curr_in->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot_index:%d, in%d ok.\n", curr_slot->obj->index, in_index);
    return 0;
}

static void slotindex_in_remove_kobj_and_attrs(struct slot_obj_t * curr_slot)
{
    int in_index;

    for(in_index = curr_slot->in_number; in_index > 0; in_index--) {
        slotindex_single_in_remove_kobj_and_attrs(curr_slot, in_index);
    }

    if(curr_slot->in) {
        kfree(curr_slot->in);
        curr_slot->in = NULL;
        curr_slot->in_number = 0;
    }
    return;
}

static int slotindex_in_create_kobj_and_attrs(struct slot_obj_t * curr_slot, int in_num)
{
    int in_index, i;

    curr_slot->in_number = in_num;
    curr_slot->in = kzalloc(sizeof(struct slot_in_obj_t) * in_num, GFP_KERNEL);
    if (!curr_slot->in) {
        SLOT_ERR("kzalloc slot Voltage error, slot index = %d, Voltage number = %d.\n", curr_slot->obj->index, in_num);
        return -ENOMEM;
    }

    for (in_index = 1; in_index <= in_num; in_index++) {
        if (slotindex_single_in_create_kobj_and_attrs(curr_slot, in_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for (i = in_index - 1; i > 0; i++) {
        slotindex_single_in_remove_kobj_and_attrs(curr_slot, i);
    }

    if (curr_slot->in) {
        kfree(curr_slot->in);
        curr_slot->in = NULL;
        curr_slot->in_number = 0;
    }
    return -EBADRQC;
}

static void slotindex_obj_remove_kobj_and_attrs(struct slot_obj_t * curr_slot, unsigned int obj_id)
{
    switch (obj_id) {
    case WB_MINOR_DEV_TEMP:
        slotindex_temp_remove_kobj_and_attrs(curr_slot);
        break;
    case WB_MINOR_DEV_IN:
        slotindex_in_remove_kobj_and_attrs(curr_slot);
        break;
    default:
        SLOT_ERR("Unknow obj id:%d\n", obj_id);
    }
    return ;
}

static int slotindex_obj_create_kobj_and_attrs(struct slot_obj_t * curr_slot, unsigned int obj_id, int obj_num)
{
    int ret;

    switch (obj_id) {
    case WB_MINOR_DEV_TEMP:
        ret = slotindex_temp_create_kobj_and_attrs(curr_slot, obj_num);
        break;
    case WB_MINOR_DEV_IN:
        ret = slotindex_in_create_kobj_and_attrs(curr_slot, obj_num);
        break;
    default:
        SLOT_ERR("Unknow obj id:%d\n", obj_id);
        ret = -EINVAL;
    }
    return ret;
}

static void slot_child_obj_remove_by_id(unsigned int obj_id)
{
    int slot_num;
    unsigned int slot_index;
    struct slot_obj_t *curr_slot;     /* point to slot1 slot2...*/

    slot_num = g_slot.slot_number;
    if (slot_num <= 0 || !g_slot.slot) {
        SLOT_DBG("Warning:slot number = %d\n", slot_num);
        return;
    }

    for(slot_index = slot_num; slot_index > 0; slot_index--) {
        curr_slot = &g_slot.slot[slot_index - 1];
        slotindex_obj_remove_kobj_and_attrs(curr_slot, obj_id);
    }
    return;
}

static int slot_child_obj_create_by_id(unsigned int obj_id)
{
    int slot_num, obj_num;
    unsigned int slot_index, i;
    struct slot_obj_t *curr_slot;     /* point to slot1 slot2...*/

    check_p(g_drv->get_dev_number);
    obj_num = g_drv->get_dev_number(WB_MAIN_DEV_SLOT,obj_id);
    slot_num = g_slot.slot_number;
    if (obj_num <= 0 || slot_num <= 0 || !g_slot.slot) {
        SLOT_DBG("Warning:slot number = %d, object number:%d.obj_id:%d\n", slot_num, obj_num, obj_id);
        return 0;
    }

    for (slot_index = 1; slot_index <= slot_num; slot_index++) {
        curr_slot = &g_slot.slot[slot_index - 1];
        if (slotindex_obj_create_kobj_and_attrs(curr_slot, obj_id, obj_num) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for(i = slot_index - 1; i > 0; i++) {
        curr_slot = &g_slot.slot[i - 1];
        slotindex_obj_remove_kobj_and_attrs(curr_slot, obj_id);
    }
    return -EBADRQC;
}

static void slot_child_obj_remove(void)
{
    /* temp remove */
    slot_child_obj_remove_by_id(WB_MINOR_DEV_TEMP);

    /* in creat */
    slot_child_obj_remove_by_id(WB_MINOR_DEV_IN);
    return;
}

static int slot_child_obj_create(void)
{
    int ret;

    /* temp creat */
    ret = slot_child_obj_create_by_id(WB_MINOR_DEV_TEMP);
    if (ret < 0) {
        goto temp_err;
    }
    /* Voltage creat */
    ret = slot_child_obj_create_by_id(WB_MINOR_DEV_IN);
    if(ret < 0) {
        goto in_err;
    }
    return 0;
in_err:
    slot_child_obj_remove_by_id(WB_MINOR_DEV_TEMP);
temp_err:
    return ret;
}

static void slot_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct slot_obj_t *curr_slot;

    curr_slot = &g_slot.slot[index - 1];
    if (curr_slot->obj) {
        sysfs_remove_group(&curr_slot->obj->kobj, &slot_attr_group);
        wb_plat_kobject_delete(&curr_slot->obj);
        SLOT_DBG("delete slot%d.\n", index);
    }
    return;
}

static int slot_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct slot_obj_t *curr_slot;

    curr_slot = &g_slot.slot[index - 1];
    SLOT_DBG("create %s%d ...\n", SLOT_SYSFS_NAME, index);
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "%s%d", SLOT_SYSFS_NAME, index);
    curr_slot->obj = wb_plat_kobject_create(name, parent);
    if (!curr_slot->obj) {
        SLOT_ERR("create slot%d object error!\n", index);
        return -EBADRQC;
    }
    curr_slot->obj->index = index;
    if (sysfs_create_group(&curr_slot->obj->kobj, &slot_attr_group) != 0) {
        SLOT_ERR("create slot%d attrs error.\n", index);
        wb_plat_kobject_delete(&curr_slot->obj);
        return -EBADRQC;
    }
    SLOT_DBG("create slot%d ok.\n", index);
    return 0;
}

static int slot_sub_create_kobj_and_attrs(struct kobject *parent, int slot_num)
{
    unsigned int slot_index, i;

    g_slot.slot = kzalloc(sizeof(struct slot_obj_t) * slot_num, GFP_KERNEL);
    if (!g_slot.slot) {
        SLOT_ERR("kzalloc slot.slot error, slot number = %d.\n", slot_num);
        return -ENOMEM;
    }

    for (slot_index = 1; slot_index <= slot_num; slot_index++) {
        if (slot_sub_single_create_kobj_and_attrs(parent, slot_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for (i = slot_index - 1; i > 0; i--) {
        slot_sub_single_remove_kobj_and_attrs(i);
    }
    if (g_slot.slot) {
        kfree(g_slot.slot);
        g_slot.slot = NULL;
    }
    return -EBADRQC;
}

/* create slot1 slot2...dir and attrs */
static int slot_sub_create(void)
{
    int ret, slot_num;

    check_p(g_drv->get_dev_number);
    slot_num = g_drv->get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_NONE);
    g_slot.slot_number = slot_num;
    if (slot_num <= 0) {
        SLOT_DBG("Warning:slot number = %d \n", slot_num);
        return 0;
    }
    ret = slot_sub_create_kobj_and_attrs(&g_slot_obj->kobj, slot_num);
    return ret;
}

/**
 * slot_sub_remove - delete slot1 slot2...dir and attrs
 */
static void slot_sub_remove(void)
{
    unsigned int slot_index;

    if (g_slot.slot) {
       for (slot_index = g_slot.slot_number; slot_index > 0; slot_index--) {
           slot_sub_single_remove_kobj_and_attrs(slot_index);
       }
       kfree(g_slot.slot);
    }
    mem_clear(&g_slot, sizeof(struct slot_t));
    return ;
}

/* create slot dir  and num_slots attr */
static int slot_root_create(void)
{
    g_slot_obj = wb_plat_kobject_create(SLOT_SYSFS_NAME, NULL);
    if (!g_slot_obj) {
        SLOT_ERR("wb_plat_kobject_create slot error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_slot_obj->kobj, &slot_root_attr_group) != 0) {
        wb_plat_kobject_delete(&g_slot_obj);
        SLOT_ERR("create slot dir attrs error!\n");
        return -EBADRQC;
    }
    SLOT_DBG("wb_plat_kobject_create slot directory and attribute success.\n");
    return 0;
}

static void slot_root_remove(void)
{
    if (g_slot_obj) {
        sysfs_remove_group(&g_slot_obj->kobj, &slot_root_attr_group);
        wb_plat_kobject_delete(&g_slot_obj);
        SLOT_DBG("delete slot root success\n");
    }

    return;
}

static int wb_slot_init(void)
{
    int ret;

    SLOT_INFO("wb_slot_init...\n");
    g_drv = dfd_plat_driver_get();
    check_p(g_drv);

    ret = slot_root_create();
    if (ret < 0) {
        goto slot_root_error;
    }

    ret = slot_sub_create();
    if (ret < 0) {
        goto slot_sub_error;
    }

    ret = slot_child_obj_create();
    if (ret < 0) {
        goto slot_child_obj_error;
    }

    SLOT_INFO("wb_slot_init ok.\n");
    return 0;
slot_child_obj_error:
    slot_sub_remove();
slot_sub_error:
    slot_root_remove();
slot_root_error:
    return ret;
}

static void wb_slot_exit(void)
{
    slot_child_obj_remove();
    slot_sub_remove();
    slot_root_remove();
    SLOT_INFO("wb_slot_exit ok.\n");
    return ;
}

module_init(wb_slot_init);
module_exit(wb_slot_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("slot sysfs driver");
