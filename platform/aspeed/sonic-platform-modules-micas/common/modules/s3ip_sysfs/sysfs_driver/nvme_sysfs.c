/*
 * nvme_sysfs.c
 *
 * This module create nvme kobjects and attributes in /sys/s3ip/nvme
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "nvme_sysfs.h"

static int g_nvme_loglevel = 0;

#define NVME_INFO(fmt, args...) do { \
    if (g_nvme_loglevel & INFO) { \
        printk(KERN_INFO "[NVME_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define NVME_ERR(fmt, args...) do { \
    if (g_nvme_loglevel & ERR) { \
        printk(KERN_ERR "[NVME_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define NVME_DBG(fmt, args...) do { \
    if (g_nvme_loglevel & DBG) { \
        printk(KERN_DEBUG "[NVME_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct sensor_obj_s {
    struct switch_obj *obj;
};

struct nvme_obj_s {
    unsigned int temp_number;
    unsigned int power_number;
    struct sensor_obj_s *temp;
    struct sensor_obj_s *power;
    struct switch_obj *obj;
};

struct nvme_s {
    unsigned int nvme_number;
    struct nvme_obj_s *nvme;
};

static struct nvme_s g_nvme;
static struct switch_obj *g_nvme_obj = NULL;
static struct s3ip_sysfs_nvme_drivers_s *g_nvme_drv = NULL;

static ssize_t nvme_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_nvme.nvme_number);
}

static ssize_t sys_nvme_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *nvme_attr;

    check_p(g_nvme_drv);
    check_p(g_nvme_drv->get_nvme_attr);

    index = obj->index;
    NVME_DBG("nvme index: %u\n", index);

    nvme_attr = to_switch_device_attr(attr);
    check_p(nvme_attr);

    return g_nvme_drv->get_nvme_attr(index, nvme_attr->type, buf, PAGE_SIZE);
}

static ssize_t nvme_temp_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int nvme_index, temp_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *nvme_attr;

    check_p(g_nvme_drv);
    check_p(g_nvme_drv->get_nvme_temp_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    nvme_index = g_obj->index;
    temp_index = obj->index;

    nvme_attr = to_switch_device_attr(attr);
    check_p(nvme_attr);

    NVME_DBG("nvme_index: %u, temp_index: %u, nvme_attr->type: %d\n", nvme_index, temp_index, nvme_attr->type);

    return g_nvme_drv->get_nvme_temp_attr(nvme_index, temp_index, nvme_attr->type, buf, PAGE_SIZE);
}

static ssize_t nvme_power_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int nvme_index, power_index;
    struct switch_obj *g_obj;
    struct switch_device_attribute *nvme_attr;

    check_p(g_nvme_drv);
    check_p(g_nvme_drv->get_nvme_power_attr);

    g_obj = to_switch_obj(obj->kobj.parent);
    nvme_index = g_obj->index;
    power_index = obj->index;

    nvme_attr = to_switch_device_attr(attr);
    check_p(nvme_attr);

    NVME_DBG("nvme index: %u, power index: %u, nvme_attr->type: %d\n", nvme_index, power_index, nvme_attr->type);

    return g_nvme_drv->get_nvme_power_attr(nvme_index, power_index, nvme_attr->type, buf, PAGE_SIZE);
}

/************************************nvme dir and attr*******************************************/
static struct switch_attribute nvme_number_attr = __ATTR(number, S_IRUGO, nvme_number_show, NULL);

static struct attribute *nvme_root_attrs[] = {
    &nvme_number_attr.attr,
    NULL,
};

static struct attribute_group nvme_root_attr_group = {
    .attrs = nvme_root_attrs,
};

/************************************nvme[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_ALIAS_E);
static SWITCH_DEVICE_ATTR(status, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_STATUS_E);
static SWITCH_DEVICE_ATTR(health_status, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_HEALTH_STATUS_E);
static SWITCH_DEVICE_ATTR(pdlu, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PDLU_E);
static SWITCH_DEVICE_ATTR(vendor_id, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PCIE_VENDOR_ID_E);
static SWITCH_DEVICE_ATTR(serial_number, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_SERIAL_NUMBER_E);
static SWITCH_DEVICE_ATTR(firmware_version, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_FIRMWARE_VERSION_E);
static SWITCH_DEVICE_ATTR(bootloader_version, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_BOOTLOADER_VERSION_E);
static SWITCH_DEVICE_ATTR(device_id, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PCIE_DEVICE_ID_E);
static SWITCH_DEVICE_ATTR(sub_vendor_id, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PCIE_SUB_VENDOR_ID_E);
static SWITCH_DEVICE_ATTR(sub_device_id, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PCIE_SUB_DEVICE_ID_E);
static SWITCH_DEVICE_ATTR(mfr_name, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_MFR_NAME_E);
static SWITCH_DEVICE_ATTR(part_number, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PART_NUMBER_E);
static SWITCH_DEVICE_ATTR(psn, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PSN_E);
static SWITCH_DEVICE_ATTR(product_name, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_PRODUCT_NAME_E);
static SWITCH_DEVICE_ATTR(capacity, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_CAPACITY_E);
static SWITCH_DEVICE_ATTR(interface_type, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_INTERFACE_TYPE_E);
static SWITCH_DEVICE_ATTR(media_type, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_MEDIA_TYPE_E);
static SWITCH_DEVICE_ATTR(monitor_flag, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_MONITOR_FLAG_E);
static SWITCH_DEVICE_ATTR(num_temp_sensors, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_TEMP_NUM_E);
static SWITCH_DEVICE_ATTR(num_power_sensors, S_IRUGO, sys_nvme_attr_show, NULL, DFD_NVME_POWER_NUM_E);

static struct attribute *nvme_attrs[] = {
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_status.switch_attr.attr,
    &switch_dev_attr_health_status.switch_attr.attr,
    &switch_dev_attr_pdlu.switch_attr.attr,
    &switch_dev_attr_vendor_id.switch_attr.attr,
    &switch_dev_attr_serial_number.switch_attr.attr,
    &switch_dev_attr_firmware_version.switch_attr.attr,
    &switch_dev_attr_bootloader_version.switch_attr.attr,
    &switch_dev_attr_device_id.switch_attr.attr,
    &switch_dev_attr_sub_vendor_id.switch_attr.attr,
    &switch_dev_attr_sub_device_id.switch_attr.attr,
    &switch_dev_attr_mfr_name.switch_attr.attr,
    &switch_dev_attr_part_number.switch_attr.attr,
    &switch_dev_attr_psn.switch_attr.attr,
    &switch_dev_attr_product_name.switch_attr.attr,
    &switch_dev_attr_capacity.switch_attr.attr,
    &switch_dev_attr_interface_type.switch_attr.attr,
    &switch_dev_attr_media_type.switch_attr.attr,
    &switch_dev_attr_monitor_flag.switch_attr.attr,
    &switch_dev_attr_num_temp_sensors.switch_attr.attr,
    &switch_dev_attr_num_power_sensors.switch_attr.attr,
    NULL,
};

static struct attribute_group nvme_attr_group = {
    .attrs = nvme_attrs,
};

/*******************************nvme temp[1-n] dir and attrs begin ****************************************/
static SWITCH_DEVICE_ATTR_PRE(temp, value, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_INPUT_E);
static SWITCH_DEVICE_ATTR_PRE(temp, alias, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(temp, max, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MAX_E);
static SWITCH_DEVICE_ATTR_PRE(temp, min, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MIN_E);
static SWITCH_DEVICE_ATTR_PRE(temp, high, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(temp, low, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_LOW_E);
static SWITCH_DEVICE_ATTR_PRE(temp, notice_high, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E);
static SWITCH_DEVICE_ATTR_PRE(temp, notice_low, S_IRUGO, nvme_temp_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E);

static struct attribute *nvme_temp_attrs[] = {
    &switch_dev_attr_temp_alias.switch_attr.attr,
    &switch_dev_attr_temp_value.switch_attr.attr,
    &switch_dev_attr_temp_high.switch_attr.attr,
    &switch_dev_attr_temp_low.switch_attr.attr,
    &switch_dev_attr_temp_notice_high.switch_attr.attr,
    &switch_dev_attr_temp_notice_low.switch_attr.attr,
    &switch_dev_attr_temp_max.switch_attr.attr,
    &switch_dev_attr_temp_min.switch_attr.attr,
    NULL,
};

static struct attribute_group nvme_temp_attr_group = {
    .attrs = nvme_temp_attrs,
};
/*******************************nvme temp[1-n] dir and attrs end ******************************************/

/*******************************nvme power[1-n] dir and attrs begin ***************************************/
static SWITCH_DEVICE_ATTR_PRE(power, value, S_IRUGO, nvme_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_INPUT_E);
static SWITCH_DEVICE_ATTR_PRE(power, alias, S_IRUGO, nvme_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_ALIAS_E);
static SWITCH_DEVICE_ATTR_PRE(power, max, S_IRUGO, nvme_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MAX_E);
static SWITCH_DEVICE_ATTR_PRE(power, min, S_IRUGO, nvme_power_attr_show, NULL, DFD_BMC_MANAGED_SENSOR_MIN_E);

static struct attribute *nvme_power_attrs[] = {
    &switch_dev_attr_power_alias.switch_attr.attr,
    &switch_dev_attr_power_value.switch_attr.attr,
    &switch_dev_attr_monitor_flag.switch_attr.attr,
    &switch_dev_attr_power_min.switch_attr.attr,
    &switch_dev_attr_power_max.switch_attr.attr,
    NULL,
};

static struct attribute_group nvme_power_attr_group = {
    .attrs = nvme_power_attrs,
};
/********************************nvme power[1-n] dir and attrs end ****************************************/

static int index_single_sensor_create_kobj_and_attrs(struct nvme_obj_s *curr_nvme, 
                                                            struct sensor_obj_s *curr_sensor,
                                                            unsigned int sensor_index, 
                                                            unsigned char *sensor_prefix,
                                                            struct attribute_group *sensor_attr_group)
{
    char name[DIR_NAME_MAX_LEN];
    struct switch_obj *sensor_kobj;
    int ret;
    
    if (!curr_nvme || !curr_nvme->obj || !sensor_attr_group || !sensor_prefix || sensor_index == 0 ) {
        NVME_ERR("Invalid parameters: curr_nvme=%p, sensor_index=%u, sensor_prefix=%s, sensor_attr_group=%p\n", 
                curr_nvme, sensor_index, sensor_prefix, sensor_attr_group);
        return -EINVAL;
    }
    
    mem_clear(name, sizeof(name));

    ret = snprintf(name, sizeof(name), "%s%u", sensor_prefix, sensor_index);
    if (ret >= sizeof(name) || ret < 0) {
        NVME_ERR("Name generation failed for sensor_index=%u\n", sensor_index);
        return -EINVAL;
    }
    
    sensor_kobj = switch_kobject_create(name, &curr_nvme->obj->kobj);
    if (!sensor_kobj) {
        NVME_ERR("Failed to create %s kobject for NVMe%u\n", 
                name, curr_nvme->obj->index);
        return -ENOMEM;
    }
    
    curr_sensor->obj = sensor_kobj;
    curr_sensor->obj->index = sensor_index;
    
    ret = sysfs_create_group(&curr_sensor->obj->kobj, sensor_attr_group);
    if (ret != 0) {
        NVME_ERR("Failed to create attributes for NVMe%u/%s: %d\n", 
                curr_nvme->obj->index, name, ret);
                
        switch_kobject_delete(&curr_sensor->obj);
        curr_sensor->obj = NULL;
        
        return (ret == -ENOMEM) ? -ENOMEM : -EBADRQC;
    }
    
    NVME_DBG("Successfully created NVMe%u/%s\n", 
            curr_nvme->obj->index, name);
    
    return 0;
}

static void index_sensor_cleanup(struct nvme_obj_s *curr_nvme, 
                                        struct sensor_obj_s *curr_sensor, 
                                        struct attribute_group *sensor_attr_group,
                                        unsigned int sensor_index)
{
    if (!curr_nvme || !curr_sensor || !sensor_attr_group || sensor_index == 0) {
        NVME_ERR("cleanup: invalid param. nvme=%p, sensor=%p, group=%p, index=%u\n",
                curr_nvme, curr_sensor, sensor_attr_group, sensor_index);
        return;
    }

    if (curr_sensor->obj) {
        sysfs_remove_group(&curr_sensor->obj->kobj, sensor_attr_group);
        switch_kobject_delete(&curr_sensor->obj);
        curr_sensor->obj = NULL;
        NVME_DBG("Cleanup sensor index %u: removed group and kobject\n", sensor_index);
    }

    return;
}

static int index_sensor_create_kobj_and_attrs(struct nvme_obj_s *curr_nvme)
{
    unsigned int i, sensor_num;
    unsigned int temp_index, power_index;

    sensor_num = curr_nvme->temp_number;
    curr_nvme->temp = kzalloc(sizeof(struct sensor_obj_s) * sensor_num, GFP_KERNEL);
    if (!curr_nvme->temp) {
        NVME_ERR("kzalloc temp error, nvme index: %u, temp number: %u.\n",
            curr_nvme->obj->index, sensor_num);
        return -ENOMEM;
    }

    for(temp_index = 1; temp_index <= sensor_num; temp_index++) {
        if(index_single_sensor_create_kobj_and_attrs(curr_nvme, 
                &curr_nvme->temp[temp_index -1], 
                temp_index, 
                "temp", 
                &nvme_temp_attr_group) != 0 ) {
            goto temp_error;
        }
    }

    sensor_num = curr_nvme->power_number;
    curr_nvme->power = kzalloc(sizeof(struct sensor_obj_s) * sensor_num, GFP_KERNEL);
    if (!curr_nvme->power) {
        NVME_ERR("kzalloc power error, nvme index: %u, power number: %u.\n",
            curr_nvme->obj->index, sensor_num);
        goto temp_error;
    }

    for(power_index = 1; power_index <= sensor_num; power_index++) {
        if(index_single_sensor_create_kobj_and_attrs(curr_nvme, 
                &curr_nvme->power[power_index -1], 
                power_index, 
                "power", 
                &nvme_power_attr_group) != 0 ) {
            goto power_error;
        }
    }

    return 0;

power_error:
    for(i = power_index - 1; i > 0; i--) {
        index_sensor_cleanup(curr_nvme, &curr_nvme->power[i - 1], &nvme_power_attr_group, i);
    }

    kfree(curr_nvme->power);
    curr_nvme->power = NULL;

temp_error:
    for(i = temp_index - 1; i > 0; i--) {
        index_sensor_cleanup(curr_nvme, &curr_nvme->temp[i - 1], &nvme_temp_attr_group, i);
    }

    kfree(curr_nvme->temp);
    curr_nvme->temp = NULL;

    return -ENOMEM;
}


static void index_sensor_remove_kobj_and_attrs(struct nvme_obj_s *curr_nvme)
{
    unsigned int temp_index, power_index;
    unsigned int temp_num, power_num;

    if (curr_nvme->temp) {
        temp_num = curr_nvme->temp_number;
        for (temp_index = temp_num; temp_index > 0; temp_index--) {
            index_sensor_cleanup(curr_nvme, &curr_nvme->temp[temp_index - 1], &nvme_temp_attr_group, temp_index); 
        }

        kfree(curr_nvme->temp);
        curr_nvme->temp = NULL;
    }

    if (curr_nvme->power) {
        power_num = curr_nvme->power_number;
        for (power_index = power_num; power_index > 0; power_index--) {
            index_sensor_cleanup(curr_nvme, &curr_nvme->power[power_index - 1], &nvme_power_attr_group, power_index);
        }

        kfree(curr_nvme->power);
        curr_nvme->power = NULL;
    }

    return;
}


static int nvme_root_create(void)
{
    g_nvme_obj = switch_kobject_create("nvme", NULL);
    if (!g_nvme_obj) {
        NVME_ERR("create NVMe sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_nvme_obj->kobj, &nvme_root_attr_group) != 0) {
        switch_kobject_delete(&g_nvme_obj);
        NVME_ERR("create NVMe root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int nvme_device_create(int index)
{
    char name[32];
    struct nvme_obj_s *nvme;
    int ret;

    nvme = &g_nvme.nvme[index - 1];

    snprintf(name, sizeof(name), "nvme%u", index);
    nvme->obj = switch_kobject_create(name, &g_nvme_obj->kobj);
    if (!nvme->obj) {
        NVME_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    nvme->obj->index = index;

    ret = sysfs_create_group(&nvme->obj->kobj, &nvme_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&nvme->obj);
        NVME_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    NVME_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int nvme_devices_create(void)
{
    int i, ret;
    struct nvme_obj_s *nvme;

	if (g_nvme.nvme_number == 0) {
		NVME_INFO("nvme is 0, do nothing\n");
	    return 0;
	}

    g_nvme.nvme = kzalloc(sizeof(struct nvme_obj_s) * g_nvme.nvme_number, GFP_KERNEL);
    if (!g_nvme.nvme) {
        NVME_ERR("allocate nvme objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_nvme.nvme_number; i++) {
        ret = nvme_device_create(i);
        if (ret) {
            while (--i >= 1) {
                nvme = &g_nvme.nvme[i - 1];
                if (nvme->obj) {
                    sysfs_remove_group(&nvme->obj->kobj, &nvme_attr_group);
                    switch_kobject_delete(&nvme->obj);
                }
            }
            kfree(g_nvme.nvme);
            g_nvme.nvme = NULL;
            return ret;
        }
    }

    return 0;
}

static int nvme_sensor_create(void)
{
    int nvme_num, temp_number, power_number;
    unsigned int nvme_index, i;
    struct nvme_obj_s *curr_nvme;

    nvme_num = g_nvme.nvme_number;
    if (nvme_num == 0) {
        NVME_DBG("nvme number is 0, skip to create temp* dirs and attrs.\n");
        return 0;
    }

    check_p(g_nvme_drv->get_nvme_temp_number);
    check_p(g_nvme_drv->get_nvme_power_number);

    temp_number = g_nvme_drv->get_nvme_temp_number();
    if (temp_number <= 0) {
        NVME_DBG("temp number: %d, don't need to create temp* dirs and attrs.\n",
            temp_number);
    }

    power_number = g_nvme_drv->get_nvme_power_number();
    if (power_number <= 0) {
        NVME_DBG("power number: %d, don't need to create temp* dirs and attrs.\n",
            power_number);
    }

    for(nvme_index = 1; nvme_index <= nvme_num; nvme_index++) {
        curr_nvme = &g_nvme.nvme[nvme_index - 1];
        curr_nvme->temp_number = temp_number;
        curr_nvme->power_number = power_number;

        NVME_DBG("NVMe[%d]: temp_number=%d, power_number=%d\n",
            nvme_index, curr_nvme->temp_number, curr_nvme->power_number);

        if(index_sensor_create_kobj_and_attrs(curr_nvme) != 0) {
            goto error;
        }
    }

 
    return 0;

error:
    for(i = nvme_index - 1; i > 0; i--) {
        curr_nvme = &g_nvme.nvme[i - 1];
        index_sensor_remove_kobj_and_attrs(curr_nvme);
    }
    return -EBADRQC;
}

static void nvme_device_remove(int index)
{
    struct nvme_obj_s *nvme = &g_nvme.nvme[index - 1];
    if (nvme->obj) {
        sysfs_remove_group(&nvme->obj->kobj, &nvme_attr_group);
        switch_kobject_delete(&nvme->obj);
        NVME_DBG("removed nvme%d\n", index);
    }
}

static void nvme_devices_remove(void)
{
    int i;
    struct nvme_obj_s *nvme;

    if (g_nvme.nvme) {
        for (i = g_nvme.nvme_number; i > 0; i--) {
            nvme = &g_nvme.nvme[i - 1];
            index_sensor_remove_kobj_and_attrs(nvme);
            nvme_device_remove(i);
        }

        kfree(g_nvme.nvme);
        g_nvme.nvme = NULL;
    }

    g_nvme.nvme_number = 0;
}

static void nvme_sysfs_remove(void)
{
    nvme_devices_remove();

    if (g_nvme_obj) {
        sysfs_remove_group(&g_nvme_obj->kobj, &nvme_root_attr_group);
        switch_kobject_delete(&g_nvme_obj);
        g_nvme_obj = NULL;
    }
}

static int nvme_sysfs_create(void)
{
    int ret;

    ret = nvme_root_create();
    if (ret) {
        return ret;
    }

    ret = nvme_devices_create();
    if (ret) {
        nvme_sysfs_remove();
        NVME_ERR("create nvme dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    ret = nvme_sensor_create();
    if (ret) {
        NVME_ERR("create nvme temp dir and attrs failed, ret: %d\n", ret);
        nvme_sysfs_remove();
        nvme_devices_remove();
        g_nvme_drv = NULL;

        return ret;
    }

    return 0;
}

int s3ip_sysfs_nvme_drivers_register(struct s3ip_sysfs_nvme_drivers_s *drv)
{
    int nvme_number;
    int ret;

    NVME_INFO("s3ip_sysfs_nvme_drivers_register...\n");
    if (g_nvme_drv) {
        NVME_ERR("g_nvme_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_nvme_number);
    g_nvme_drv = drv;

    nvme_number = g_nvme_drv->get_nvme_number();
    if (nvme_number < 0) {
        g_nvme_drv = NULL;
        NVME_ERR("Invalid nvme number: %d\n", nvme_number);
        return -EINVAL;
    }

    mem_clear(&g_nvme, sizeof(struct nvme_s));

    g_nvme.nvme_number = nvme_number;

    ret = nvme_sysfs_create();
    if (ret) {
        g_nvme_drv = NULL;
        return ret;
    }

    NVME_INFO("Registered NVMe driver with %d devices\n", nvme_number);

    return 0;
}

void s3ip_sysfs_nvme_drivers_unregister(void)
{
    if (g_nvme_drv) {
        nvme_sysfs_remove();
        g_nvme_drv = NULL;
        NVME_INFO("NVMe driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_nvme_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_nvme_drivers_unregister);
module_param(g_nvme_loglevel, int, 0644);
MODULE_PARM_DESC(g_nvme_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");