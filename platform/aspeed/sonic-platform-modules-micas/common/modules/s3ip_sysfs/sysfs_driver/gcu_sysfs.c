/*
 * gcu_sysfs.c
 *
 * This module create gcu kobjects and attributes in /sys/s3ip/gcu
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "gcu_sysfs.h"

static int g_gcu_loglevel = 0x0;

#define GCU_INFO(fmt, args...) do { \
    if (g_gcu_loglevel & INFO) { \
        printk(KERN_INFO "[GCU_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define GCU_ERR(fmt, args...) do { \
    if (g_gcu_loglevel & ERR) { \
        printk(KERN_ERR "[GCU_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define GCU_DBG(fmt, args...) do { \
    if (g_gcu_loglevel & DBG) { \
        printk(KERN_DEBUG "[GCU_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct sensor_obj_s {
    struct switch_obj *obj;
};

struct gcu_obj_s {
    unsigned int temp_number;
    unsigned int vol_number;
    unsigned int power_number;
    unsigned int monitor_flag;
    struct sensor_obj_s *temp;
    struct sensor_obj_s *vol;
    struct sensor_obj_s *power;
    struct switch_obj *obj;
};

struct gcu_s {
    unsigned int gcu_number;
    unsigned int gcu_status;
    struct gcu_obj_s *gcu;
};

static struct gcu_s g_gcu;
static struct switch_obj *g_gcu_obj = NULL;
static struct s3ip_sysfs_gcu_drivers_s *g_gcu_drv = NULL;

static ssize_t gcu_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_gcu.gcu_number);
}

static ssize_t sys_gcu_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *gcu_attr;

    check_p(g_gcu_drv);
    check_p(g_gcu_drv->get_main_board_gcu_attr);

    index = obj->index;
    GCU_DBG("gcu index: %u\n", index);

    gcu_attr = to_switch_device_attr(attr);
    check_p(gcu_attr);

    return g_gcu_drv->get_main_board_gcu_attr(index, gcu_attr->type, buf, PAGE_SIZE);
}

static ssize_t gcu_subset_monitor_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int gcu_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *gcu_attr;

    check_p(g_gcu_drv);

    g_obj = to_switch_obj(obj->kobj.parent);
    gcu_index = g_obj->index;

    GCU_DBG("gcu_index: %u\n", gcu_index);

    gcu_attr = to_switch_device_attr(attr);
    check_p(gcu_attr);

    return g_gcu_drv->get_main_board_gcu_attr(gcu_index, gcu_attr->type, buf, PAGE_SIZE);
}

static ssize_t gcu_temp_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int gcu_index, temp_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *gcu_attr;

    check_p(g_gcu_drv);
    check_p(g_gcu_drv->get_gcu_temp_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    gcu_index = g_obj->index;
    temp_index = obj->index;

    GCU_DBG("gcu_index: %u, temp_index: %u\n", gcu_index, temp_index);

    gcu_attr = to_switch_device_attr(attr);
    check_p(gcu_attr);

    return g_gcu_drv->get_gcu_temp_attr(gcu_index, temp_index, gcu_attr->type, buf, PAGE_SIZE);
}

static ssize_t gcu_vol_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int gcu_index, vol_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *gcu_attr;

    check_p(g_gcu_drv);
    check_p(g_gcu_drv->get_gcu_vol_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    gcu_index = g_obj->index;
    vol_index = obj->index;

    GCU_DBG("gcu index: %u, vol index: %u\n", gcu_index, vol_index);

    gcu_attr = to_switch_device_attr(attr);
    check_p(gcu_attr);

    return g_gcu_drv->get_gcu_vol_attr(gcu_index, vol_index, gcu_attr->type, buf, PAGE_SIZE);
}

static ssize_t gcu_power_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int gcu_index, power_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *gcu_attr;

    check_p(g_gcu_drv);
    check_p(g_gcu_drv->get_gcu_power_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    gcu_index = g_obj->index;
    power_index = obj->index;


    gcu_attr = to_switch_device_attr(attr);
    check_p(gcu_attr);
    GCU_DBG("gcu index: %u, power index: %u type: %d.\n", gcu_index, power_index, gcu_attr->type);
    GCU_DBG("begin to call get_gcu_power_attr.\n");
    return g_gcu_drv->get_gcu_power_attr(gcu_index, power_index, gcu_attr->type, buf, PAGE_SIZE);
}

/************************************gcu dir and attr*******************************************/
static struct switch_attribute gcu_number_attr = __ATTR(number, S_IRUGO, gcu_number_show, NULL);

static struct attribute *gcu_root_attrs[] = {
    &gcu_number_attr.attr,
    NULL,
};

static struct attribute_group gcu_root_attr_group = {
    .attrs = gcu_root_attrs,
};

/************************************gcu[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_ALIAS_E);
static SWITCH_DEVICE_ATTR(driver_version, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_DRIVER_VERSION_E);
static SWITCH_DEVICE_ATTR(firmware_version, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_FIRMWARE_VERSION_E);
static SWITCH_DEVICE_ATTR(pcie_vendorid, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_VENDOR_ID_E);
static SWITCH_DEVICE_ATTR(pcie_deviceid, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_DEVICE_ID_E);
static SWITCH_DEVICE_ATTR(pcie_subvendorid, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_SUB_VENDOR_ID_E);
static SWITCH_DEVICE_ATTR(pcie_subdeviceid, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_SUB_DEVICE_ID_E);
static SWITCH_DEVICE_ATTR(product_type, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PRODUCT_TYPE_E);
static SWITCH_DEVICE_ATTR(part_number, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PART_NUMBER_E);
static SWITCH_DEVICE_ATTR(manufacturer_date, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_MANUFACTURER_DATE_E);
static SWITCH_DEVICE_ATTR(serial_number, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_SERIAL_NUMBER_E);
static SWITCH_DEVICE_ATTR(hbm_capacity, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_HBM_CAPACITY_E);
static SWITCH_DEVICE_ATTR(mfr_name, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_MFR_NAME_E);
static SWITCH_DEVICE_ATTR(gcu_frequent_max, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_FREQUENT_MAX_E);
static SWITCH_DEVICE_ATTR(ecc_status, S_IRUGO | S_IWUSR, sys_gcu_attr_show, NULL, DFD_GCU_ECC_STATUS_E);
static SWITCH_DEVICE_ATTR(board_type, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_BOARD_TYPE_E);
static SWITCH_DEVICE_ATTR(pcie_rate_max, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_RATE_MAX_E);
static SWITCH_DEVICE_ATTR(pcie_rate_current, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_RATE_CURRENT_E);
static SWITCH_DEVICE_ATTR(pcie_width_max, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_LINK_WIDTH_MAX_E);
static SWITCH_DEVICE_ATTR(pcie_width_current, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_PCIE_LINK_WIDTH_CURRENT_E);
static SWITCH_DEVICE_ATTR(health_status, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_HEALTH_STATUS_E);
static SWITCH_DEVICE_ATTR(id_detect, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_ID_DETECT_E);
static SWITCH_DEVICE_ATTR(monitor_flag, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_ID_MONITOR_FLAG_E);
static SWITCH_DEVICE_ATTR(num_temp_sensors, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_TEMP_NUM_E);
static SWITCH_DEVICE_ATTR(num_vol_sensors, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_VOL_NUM_E);
static SWITCH_DEVICE_ATTR(num_power_sensors, S_IRUGO, sys_gcu_attr_show, NULL, DFD_GCU_POWER_NUM_E);

static struct attribute *gcu_attrs[] = {
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_driver_version.switch_attr.attr,
    &switch_dev_attr_firmware_version.switch_attr.attr,
    &switch_dev_attr_pcie_vendorid.switch_attr.attr,
    &switch_dev_attr_pcie_deviceid.switch_attr.attr,
    &switch_dev_attr_pcie_subvendorid.switch_attr.attr,
    &switch_dev_attr_pcie_subdeviceid.switch_attr.attr,
    &switch_dev_attr_product_type.switch_attr.attr,
    &switch_dev_attr_part_number.switch_attr.attr,
    &switch_dev_attr_manufacturer_date.switch_attr.attr,
    &switch_dev_attr_serial_number.switch_attr.attr,
    &switch_dev_attr_hbm_capacity.switch_attr.attr,
    &switch_dev_attr_mfr_name.switch_attr.attr,
    &switch_dev_attr_gcu_frequent_max.switch_attr.attr,
    &switch_dev_attr_ecc_status.switch_attr.attr,
    &switch_dev_attr_board_type.switch_attr.attr,
    &switch_dev_attr_pcie_rate_max.switch_attr.attr,
    &switch_dev_attr_pcie_rate_current.switch_attr.attr,
    &switch_dev_attr_pcie_width_max.switch_attr.attr,
    &switch_dev_attr_pcie_width_current.switch_attr.attr,
    &switch_dev_attr_health_status.switch_attr.attr,
    &switch_dev_attr_id_detect.switch_attr.attr,
    &switch_dev_attr_monitor_flag.switch_attr.attr,
    &switch_dev_attr_num_temp_sensors.switch_attr.attr,
    &switch_dev_attr_num_vol_sensors.switch_attr.attr,
    &switch_dev_attr_num_power_sensors.switch_attr.attr,
    NULL,
};

static struct attribute_group gcu_attr_group = {
    .attrs = gcu_attrs,
};

/*******************************gcu temp[1-n] dir and attrs begin ****************************************/
static SWITCH_DEVICE_ATTR_PRE(temp, alias, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(temp, value, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_INPUT_E);
static SWITCH_DEVICE_ATTR_PRE(temp, max, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MAX_E);
static SWITCH_DEVICE_ATTR_PRE(temp, min, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MIN_E);
static SWITCH_DEVICE_ATTR_PRE(temp, high, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(temp, low, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(temp, notice_high, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(temp, notice_low, S_IRUGO, gcu_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(temp, monitor_flag, S_IRUGO, gcu_subset_monitor_show, NULL, DFD_GCU_ID_MONITOR_FLAG_E);

static struct attribute *gcu_temp_attrs[] = {
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

static struct attribute_group gcu_temp_attr_group = {
    .attrs = gcu_temp_attrs,
};
/*******************************gcu temp[1-n] dir and attrs end ******************************************/

/*******************************gcu vol[1-n] dir and attrs begin *****************************************/
static SWITCH_DEVICE_ATTR_PRE(vol, alias, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(vol, value, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_INPUT_E);
static SWITCH_DEVICE_ATTR_PRE(vol, max, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MAX_E);
static SWITCH_DEVICE_ATTR_PRE(vol, min, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MIN_E);
static SWITCH_DEVICE_ATTR_PRE(vol, high, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(vol, low, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(vol, notice_high, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(vol, notice_low, S_IRUGO, gcu_vol_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(vol, monitor_flag, S_IRUGO, gcu_subset_monitor_show, NULL, DFD_GCU_ID_MONITOR_FLAG_E);

static struct attribute *gcu_vol_attrs[] = {
    &switch_dev_attr_vol_alias.switch_attr.attr,
    &switch_dev_attr_vol_value.switch_attr.attr,
    &switch_dev_attr_vol_max.switch_attr.attr,
    &switch_dev_attr_vol_min.switch_attr.attr,
    &switch_dev_attr_vol_high.switch_attr.attr,
    &switch_dev_attr_vol_low.switch_attr.attr,
    &switch_dev_attr_vol_notice_high.switch_attr.attr,
    &switch_dev_attr_vol_notice_low.switch_attr.attr,
    &switch_dev_attr_vol_monitor_flag.switch_attr.attr,
    NULL,
};

static struct attribute_group gcu_vol_attr_group = {
    .attrs = gcu_vol_attrs,
};
/*******************************gcu vol[1-n] dir and attrs end *******************************************/

/*******************************gcu power[1-n] dir and attrs begin ***************************************/
static SWITCH_DEVICE_ATTR_PRE(power, alias, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(power, value, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_INPUT_E);
static SWITCH_DEVICE_ATTR_PRE(power, max, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MAX_E);
static SWITCH_DEVICE_ATTR_PRE(power, min, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MIN_E);
static SWITCH_DEVICE_ATTR_PRE(power, high, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(power, low, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(power, notice_high, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(power, notice_low, S_IRUGO, gcu_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(power, monitor_flag, S_IRUGO, gcu_subset_monitor_show, NULL, DFD_GCU_ID_MONITOR_FLAG_E);

static struct attribute *gcu_power_attrs[] = {
    &switch_dev_attr_power_alias.switch_attr.attr,
    &switch_dev_attr_power_value.switch_attr.attr,
    &switch_dev_attr_power_max.switch_attr.attr,
    &switch_dev_attr_power_min.switch_attr.attr,
    &switch_dev_attr_power_high.switch_attr.attr,
    &switch_dev_attr_power_low.switch_attr.attr,
    &switch_dev_attr_power_notice_high.switch_attr.attr,
    &switch_dev_attr_power_notice_low.switch_attr.attr,
    &switch_dev_attr_power_monitor_flag.switch_attr.attr,
    NULL,
};

static struct attribute_group gcu_power_attr_group = {
    .attrs = gcu_power_attrs,
};
/********************************gcu power[1-n] dir and attrs end ****************************************/

static int gcuindex_single_sensor_create_kobj_and_attrs(struct gcu_obj_s *curr_gcu,
                                                            struct sensor_obj_s *curr_sensor,
                                                            unsigned int sensor_index,
                                                            unsigned char *sensor_prefix,
                                                            struct attribute_group *sensor_attr_group)
{
    char name[DIR_NAME_MAX_LEN];
    struct switch_obj *sensor_kobj;
    int ret;

    if (!curr_gcu || !curr_gcu->obj || !sensor_attr_group || !sensor_prefix || sensor_index == 0 ) {
        GCU_ERR("Invalid parameters: curr_gcu=%p, sensor_index=%u, sensor_prefix=%p, sensor_attr_group=%p\n",
                curr_gcu, sensor_index, sensor_prefix, sensor_attr_group);
        return -EINVAL;
    }

    mem_clear(name, sizeof(name));

    ret = snprintf(name, sizeof(name), "%s%u", sensor_prefix, sensor_index);
    if (ret >= sizeof(name) || ret < 0) {
        GCU_ERR("Name generation failed for sensor_index=%u\n", sensor_index);
        return -EINVAL;
    }

    sensor_kobj = switch_kobject_create(name, &curr_gcu->obj->kobj);
    if (!sensor_kobj) {
        GCU_ERR("Failed to create %s kobject for GCU%u\n",
                name, curr_gcu->obj->index);
        return -ENOMEM;
    }

    curr_sensor->obj = sensor_kobj;
    curr_sensor->obj->index = sensor_index;

    ret = sysfs_create_group(&curr_sensor->obj->kobj, sensor_attr_group);
    if (ret != 0) {
        GCU_ERR("Failed to create attributes for GCU%u/%s: %d\n",
                curr_gcu->obj->index, name, ret);

        switch_kobject_delete(&curr_sensor->obj);
        curr_sensor->obj = NULL;

        return (ret == -ENOMEM) ? -ENOMEM : -EBADRQC;
    }

    GCU_DBG("Successfully created GCU%u/%s\n",
            curr_gcu->obj->index, name);

    return 0;
}

static void gcuindex_sensor_cleanup(struct gcu_obj_s *curr_gcu,
                                        struct sensor_obj_s *curr_sensor,
                                        struct attribute_group *sensor_attr_group,
                                        unsigned int sensor_index)
{
    if (!curr_gcu || !curr_sensor || !sensor_attr_group || sensor_index == 0) {
        GCU_ERR("cleanup: invalid param. gcu=%p, sensor=%p, group=%p, index=%u\n",
                curr_gcu, curr_sensor, sensor_attr_group, sensor_index);
        return;
    }

    if (curr_sensor->obj) {
        sysfs_remove_group(&curr_sensor->obj->kobj, sensor_attr_group);
        switch_kobject_delete(&curr_sensor->obj);
        curr_sensor->obj = NULL;
        GCU_DBG("Cleanup sensor index %u: removed group and kobject\n", sensor_index);
    }

    return;
}

static int gcuindex_sensor_create_kobj_and_attrs(struct gcu_obj_s *curr_gcu)
{
    unsigned int i, sensor_num;
    unsigned int temp_index, vol_index, power_index;

    sensor_num = curr_gcu->temp_number;
    curr_gcu->temp = kzalloc(sizeof(struct sensor_obj_s) * sensor_num, GFP_KERNEL);
    if (!curr_gcu->temp) {
        GCU_ERR("kzalloc temp error, gcu index: %u, temp number: %u.\n",
            curr_gcu->obj->index, sensor_num);
        return -ENOMEM;
    }

    for(temp_index = 1; temp_index <= sensor_num; temp_index++) {
        if(gcuindex_single_sensor_create_kobj_and_attrs(curr_gcu,
                &curr_gcu->temp[temp_index -1],
                temp_index,
                "temp",
                &gcu_temp_attr_group) != 0 ) {
            goto temp_error;
        }
    }

    sensor_num = curr_gcu->vol_number;
    curr_gcu->vol = kzalloc(sizeof(struct sensor_obj_s) * sensor_num, GFP_KERNEL);
    if (!curr_gcu->vol) {
        GCU_ERR("kzalloc vol error, gcu index: %u, vol number: %u.\n",
            curr_gcu->obj->index, sensor_num);
        goto temp_error;
    }

    for(vol_index = 1; vol_index <= sensor_num; vol_index++) {
        if(gcuindex_single_sensor_create_kobj_and_attrs(curr_gcu,
                &curr_gcu->vol[vol_index -1],
                vol_index,
                "vol",
                &gcu_vol_attr_group) != 0 ) {
            goto vol_error;
        }
    }

    sensor_num = curr_gcu->power_number;
    curr_gcu->power = kzalloc(sizeof(struct sensor_obj_s) * sensor_num, GFP_KERNEL);
    if (!curr_gcu->power) {
        GCU_ERR("kzalloc power error, gcu index: %u, power number: %u.\n",
            curr_gcu->obj->index, sensor_num);
        goto vol_error;
    }

    for(power_index = 1; power_index <= sensor_num; power_index++) {
        if(gcuindex_single_sensor_create_kobj_and_attrs(curr_gcu,
                &curr_gcu->power[power_index -1],
                power_index,
                "power",
                &gcu_power_attr_group) != 0 ) {
            goto power_error;
        }
    }

    return 0;

power_error:
    for(i = power_index - 1; i > 0; i--) {
        gcuindex_sensor_cleanup(curr_gcu, &curr_gcu->power[i - 1], &gcu_power_attr_group, i);
    }

    kfree(curr_gcu->power);
    curr_gcu->power = NULL;

vol_error:
    for(i = vol_index - 1; i > 0; i--) {
        gcuindex_sensor_cleanup(curr_gcu, &curr_gcu->vol[i - 1], &gcu_vol_attr_group, i);
    }

    kfree(curr_gcu->vol);
    curr_gcu->vol = NULL;

temp_error:
    for(i = temp_index - 1; i > 0; i--) {
        gcuindex_sensor_cleanup(curr_gcu, &curr_gcu->temp[i - 1], &gcu_temp_attr_group, i);
    }

    kfree(curr_gcu->temp);
    curr_gcu->temp = NULL;

    return -ENOMEM;
}


static void gcuindex_sensor_remove_kobj_and_attrs(struct gcu_obj_s *curr_gcu)
{
    unsigned int temp_index, vol_index, power_index;
    unsigned int temp_num, vol_num, power_num;

    if (curr_gcu->temp) {
        temp_num = curr_gcu->temp_number;
        for (temp_index = temp_num; temp_index > 0; temp_index--) {
            gcuindex_sensor_cleanup(curr_gcu, &curr_gcu->temp[temp_index - 1], &gcu_temp_attr_group, temp_index);
        }

        kfree(curr_gcu->temp);
        curr_gcu->temp = NULL;
    }

    if (curr_gcu->vol) {
        vol_num = curr_gcu->vol_number;
        for (vol_index = vol_num; vol_index > 0; vol_index--) {
            gcuindex_sensor_cleanup(curr_gcu, &curr_gcu->vol[vol_index - 1], &gcu_vol_attr_group, vol_index);
        }

        kfree(curr_gcu->vol);
        curr_gcu->vol = NULL;
    }

    if (curr_gcu->power) {
        power_num = curr_gcu->power_number;
        for (power_index = power_num; power_index > 0; power_index--) {
            gcuindex_sensor_cleanup(curr_gcu, &curr_gcu->power[power_index - 1], &gcu_power_attr_group, power_index);
        }

        kfree(curr_gcu->power);
        curr_gcu->power = NULL;
    }

    return;
}


static int gcu_root_create(void)
{
    g_gcu_obj = switch_kobject_create("gcu", NULL);
    if (!g_gcu_obj) {
        GCU_ERR("create GCU sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_gcu_obj->kobj, &gcu_root_attr_group) != 0) {
        switch_kobject_delete(&g_gcu_obj);
        GCU_ERR("create GCU root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int gcu_device_create(int index)
{
    char name[32];
    struct gcu_obj_s *gcu;
    int ret;

    gcu = &g_gcu.gcu[index - 1];

    snprintf(name, sizeof(name), "gcu%u", index);
    gcu->obj = switch_kobject_create(name, &g_gcu_obj->kobj);
    if (!gcu->obj) {
        GCU_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    gcu->obj->index = index;

    ret = sysfs_create_group(&gcu->obj->kobj, &gcu_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&gcu->obj);
        GCU_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    GCU_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int gcu_devices_create(void)
{
    int i, ret;
    struct gcu_obj_s *gcu;

    g_gcu.gcu = kzalloc(sizeof(struct gcu_obj_s) * g_gcu.gcu_number, GFP_KERNEL);
    if (!g_gcu.gcu) {
        GCU_ERR("allocate gcu objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_gcu.gcu_number; i++) {
        ret = gcu_device_create(i);
        if (ret) {
            while (--i >= 1) {
                gcu = &g_gcu.gcu[i - 1];
                if (gcu->obj) {
                    sysfs_remove_group(&gcu->obj->kobj, &gcu_attr_group);
                    switch_kobject_delete(&gcu->obj);
                }
            }

            kfree(g_gcu.gcu);
            g_gcu.gcu = NULL;

            return ret;
        }
    }

    return 0;
}

static int gcu_sensor_create(void)
{
    int gcu_num, temp_number, vol_number, power_number;
    unsigned int gcu_index, i;
    struct gcu_obj_s *curr_gcu;

    gcu_num = g_gcu.gcu_number;
    if (gcu_num <= 0) {
        GCU_DBG("gcu number: %d, skip to create temp* dirs and attrs.\n", gcu_num);
        return 0;
    }

    check_p(g_gcu_drv->get_gcu_temp_number);
    check_p(g_gcu_drv->get_gcu_vol_number);
    check_p(g_gcu_drv->get_gcu_power_number);

    temp_number = g_gcu_drv->get_gcu_temp_number();
    if (temp_number <= 0) {
        GCU_DBG("temp number: %d, don't need to create temp* dirs and attrs.\n",
            temp_number);
    }

    vol_number = g_gcu_drv->get_gcu_vol_number();
    if (vol_number <= 0) {
        GCU_DBG("vol number: %d, don't need to create temp* dirs and attrs.\n",
            vol_number);
    }

    power_number = g_gcu_drv->get_gcu_power_number();
    if (power_number <= 0) {
        GCU_DBG("power number: %d, don't need to create temp* dirs and attrs.\n",
            power_number);
    }

    for(gcu_index = 1; gcu_index <= gcu_num; gcu_index++) {
        curr_gcu = &g_gcu.gcu[gcu_index - 1];
        curr_gcu->temp_number = temp_number;
        curr_gcu->vol_number = vol_number;
        curr_gcu->power_number = power_number;

        GCU_DBG("GCU[%d]: temp_number=%d, vol_number=%d, power_number=%d\n",
            gcu_index, curr_gcu->temp_number, curr_gcu->vol_number, curr_gcu->power_number);

        if(gcuindex_sensor_create_kobj_and_attrs(curr_gcu) != 0) {
            goto error;
        }
    }


    return 0;

error:
    for(i = gcu_index - 1; i > 0; i--) {
        curr_gcu = &g_gcu.gcu[i - 1];
        gcuindex_sensor_remove_kobj_and_attrs(curr_gcu);
    }
    return -EBADRQC;
}

static void gcu_device_remove(int index)
{
    struct gcu_obj_s *gcu = &g_gcu.gcu[index - 1];
    if (gcu->obj) {
        sysfs_remove_group(&gcu->obj->kobj, &gcu_attr_group);
        switch_kobject_delete(&gcu->obj);
        GCU_DBG("removed gcu%d\n", index);
    }
}

static void gcu_devices_remove(void)
{
    int i;
    struct gcu_obj_s *gcu;

    if (g_gcu.gcu) {
        for (i = g_gcu.gcu_number; i > 0; i--) {
            gcu = &g_gcu.gcu[i - 1];

            if (gcu->temp) {
                gcuindex_sensor_cleanup(gcu, &gcu->temp[i - 1], &gcu_temp_attr_group, i);
                kfree(gcu->temp);
                gcu->temp = NULL;
            }

            if (gcu->vol) {
                gcuindex_sensor_cleanup(gcu, &gcu->vol[i - 1], &gcu_vol_attr_group, i);
                kfree(gcu->vol);
                gcu->vol = NULL;
            }

            if (gcu->power) {
                gcuindex_sensor_cleanup(gcu, &gcu->power[i - 1], &gcu_power_attr_group, i);
                kfree(gcu->power);
                gcu->power = NULL;
            }

            gcu_device_remove(i);
        }

        kfree(g_gcu.gcu);
        g_gcu.gcu = NULL;
    }

    g_gcu.gcu_number = 0;
}

static void gcu_sysfs_remove(void)
{
    int i;

    i = 0;
    if (g_gcu.gcu) {
        for (i = g_gcu.gcu_number; i > 0; i--) {
            gcuindex_sensor_remove_kobj_and_attrs(&g_gcu.gcu[i-1]);
        }
    }

    gcu_devices_remove();

    if (g_gcu_obj) {
        sysfs_remove_group(&g_gcu_obj->kobj, &gcu_root_attr_group);
        sysfs_remove_group(&g_gcu_obj->kobj, &gcu_attr_group);
        switch_kobject_delete(&g_gcu_obj);
        g_gcu_obj = NULL;
    }
}

static int gcu_sysfs_create(void)
{
    int ret;

    ret = gcu_root_create();
    if (ret) {
        return ret;
    }

    ret = gcu_devices_create();
    if (ret) {
        gcu_sysfs_remove();
        GCU_ERR("create gcu dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    ret = gcu_sensor_create();
    if (ret) {
        GCU_ERR("create gcu temp dir and attrs failed, ret: %d\n", ret);
        gcu_sysfs_remove();
        g_gcu_drv = NULL;

        return ret;
    }

    return 0;
}

int s3ip_sysfs_gcu_drivers_register(struct s3ip_sysfs_gcu_drivers_s *drv)
{
    int gcu_number;
    int ret;

    GCU_INFO("s3ip_sysfs_gcu_drivers_register...\n");
    if (g_gcu_drv) {
        GCU_ERR("g_gcu_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_gcu_number);
    g_gcu_drv = drv;

    gcu_number = drv->get_gcu_number();
    if (gcu_number <= 0) {
        g_gcu_drv = NULL;
        GCU_ERR("Invalid gcu number: %d\n", gcu_number);
        return -EINVAL;
    }

    mem_clear(&g_gcu, sizeof(struct gcu_s));

    g_gcu.gcu_number = gcu_number;

    ret = gcu_sysfs_create();
    if (ret) {
        g_gcu_drv = NULL;
        return ret;
    }

    GCU_INFO("Registered GCU driver with %d devices\n", gcu_number);

    return 0;
}

void s3ip_sysfs_gcu_drivers_unregister(void)
{
    if (g_gcu_drv) {
        gcu_sysfs_remove();
        g_gcu_drv = NULL;
        GCU_INFO("GCU driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_gcu_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_gcu_drivers_unregister);
module_param(g_gcu_loglevel, int, 0644);
MODULE_PARM_DESC(g_gcu_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");