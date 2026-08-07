/*
 * disk_sysfs.c
 *
 * This module create disk kobjects and attributes in /sys/s3ip/disk
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "disk_sysfs.h"

static int g_disk_loglevel = 0;

#define DISK_INFO(fmt, args...) do { \
    if (g_disk_loglevel & INFO) { \
        printk(KERN_INFO "[DISK_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DISK_ERR(fmt, args...) do { \
    if (g_disk_loglevel & ERR) { \
        printk(KERN_ERR "[DISK_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DISK_DBG(fmt, args...) do { \
    if (g_disk_loglevel & DBG) { \
        printk(KERN_DEBUG "[DISK_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct disk_obj_s {
    struct switch_obj *obj;
};

struct disk_s {
    unsigned int disk_number;
    struct disk_obj_s *disk;
};

static struct disk_s g_disk;
static struct switch_obj *g_disk_obj = NULL;
static struct s3ip_sysfs_disk_drivers_s *g_disk_drv = NULL;

static ssize_t disk_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_disk.disk_number);
}

static ssize_t disk_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *disk_attr;

    check_p(g_disk_drv);
    check_p(g_disk_drv->get_disk_attr);

    index = obj->index;
    DISK_DBG("disk index: %u\n", index);

    disk_attr = to_switch_device_attr(attr);
    check_p(disk_attr);

    return g_disk_drv->get_disk_attr(index, disk_attr->type, buf, PAGE_SIZE);
}

static ssize_t sys_disk_attr_dbg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    unsigned int disk_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_disk_drv);
    check_p(g_disk_drv->set_debug_disk_attr);
    
    disk_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_disk_drv->set_debug_disk_attr(tmp_attr->type, disk_index, buf);
    if (ret < 0) {
        DISK_ERR("disk index: %u , type: %d , debug_value : %s, set fail.\n", disk_index, tmp_attr->type, buf);
        return -EIO;
    }
    DISK_DBG("disk index: %u , type: %d , debug_value : %s, set success.\n", disk_index, tmp_attr->type, buf);
    return count;
}

/************************************disk dir and attr*******************************************/
static struct switch_attribute disk_number_attr = __ATTR(number, S_IRUGO, disk_number_show, NULL);

static struct attribute *disk_root_attrs[] = {
    &disk_number_attr.attr,
    NULL,
};

static struct attribute_group disk_root_attr_group = {
    .attrs = disk_root_attrs,
};

/************************************disk[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(ata_port, S_IRUGO, disk_attr_show, NULL, DFD_DISK_ATA_PORT_E);
static SWITCH_DEVICE_ATTR(type, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TYPE_E);
static SWITCH_DEVICE_ATTR(temp_max, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TEMP_MAX_E);
static SWITCH_DEVICE_ATTR(temp_min, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TEMP_MIN_E);
static SWITCH_DEVICE_ATTR(temp_high, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TEMP_HIGH_E);
static SWITCH_DEVICE_ATTR(temp_low, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TEMP_LOW_E);
static SWITCH_DEVICE_ATTR(target_link_speed, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TARGET_LINK_SPEED_E);
static SWITCH_DEVICE_ATTR(remaining_life_threshold, S_IRUGO, disk_attr_show, NULL, DFD_DISK_REMAINING_LIFE_THRESHOLD_E);
static SWITCH_DEVICE_ATTR(temp_status, S_IRUGO, disk_attr_show, NULL, DFD_DISK_TEMP_STATUS_E);
static SWITCH_DEVICE_ATTR(link_alarm, S_IRUGO | S_IWUSR, disk_attr_show, sys_disk_attr_dbg_store, DFD_DISK_LINK_ALARM_E);
static SWITCH_DEVICE_ATTR(wear_status, S_IRUGO | S_IWUSR, disk_attr_show, sys_disk_attr_dbg_store, DFD_DISK_WEAR_STATUS_E);
static SWITCH_DEVICE_ATTR(link_status, S_IRUGO, disk_attr_show, NULL, DFD_DISK_LINK_STATUS_E);
static SWITCH_DEVICE_ATTR(speed_status, S_IRUGO, disk_attr_show, NULL, DFD_DISK_SPEED_STATUS_E);
static SWITCH_DEVICE_ATTR(correctable_error, S_IRUGO, disk_attr_show, NULL, DFD_DISK_CE_E);
static SWITCH_DEVICE_ATTR(uncorrectable_error, S_IRUGO, disk_attr_show, NULL, DFD_DISK_UE_E);

static struct attribute *disk_attrs[] = {
    &switch_dev_attr_ata_port.switch_attr.attr,
    &switch_dev_attr_type.switch_attr.attr,
    &switch_dev_attr_temp_max.switch_attr.attr,
    &switch_dev_attr_temp_min.switch_attr.attr,
    &switch_dev_attr_temp_high.switch_attr.attr,
    &switch_dev_attr_temp_low.switch_attr.attr,
    &switch_dev_attr_target_link_speed.switch_attr.attr,
    &switch_dev_attr_remaining_life_threshold.switch_attr.attr,
    &switch_dev_attr_temp_status.switch_attr.attr,
    &switch_dev_attr_link_alarm.switch_attr.attr,
    &switch_dev_attr_wear_status.switch_attr.attr,
    &switch_dev_attr_link_status.switch_attr.attr,
    &switch_dev_attr_speed_status.switch_attr.attr,
    &switch_dev_attr_correctable_error.switch_attr.attr,
    &switch_dev_attr_uncorrectable_error.switch_attr.attr,
    NULL,
};

static struct attribute_group disk_attr_group = {
    .attrs = disk_attrs,
};


static int disk_root_create(void)
{
    int ret;

    g_disk_obj = switch_kobject_create("disk", NULL);
    if (!g_disk_obj) {
        DISK_ERR("create DISK sysfs object failed\n");
        return -ENOMEM;
    }

    ret = sysfs_create_group(&g_disk_obj->kobj, &disk_root_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&g_disk_obj);
        DISK_ERR("create DISK root attributes failed, ret: %d\n", ret);
        return ret;
    }

    return 0;
}

static int disk_device_create(int index)
{
    char name[32];
    struct disk_obj_s *disk;
    int ret;

    disk = &g_disk.disk[index - 1];

    snprintf(name, sizeof(name), "disk%u", index);
    disk->obj = switch_kobject_create(name, &g_disk_obj->kobj);
    if (!disk->obj) {
        DISK_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    disk->obj->index = index;

    ret = sysfs_create_group(&disk->obj->kobj, &disk_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&disk->obj);
        DISK_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    DISK_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int disk_devices_create(void)
{
    int i, ret;
    struct disk_obj_s *disk;

    g_disk.disk = kzalloc(sizeof(struct disk_obj_s) * g_disk.disk_number, GFP_KERNEL);
    if (!g_disk.disk) {
        DISK_ERR("allocate disk objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_disk.disk_number; i++) {
        ret = disk_device_create(i);
        if (ret) {
            while (--i >= 1) {
                disk = &g_disk.disk[i - 1];
                if (disk->obj) {
                    sysfs_remove_group(&disk->obj->kobj, &disk_attr_group);
                    switch_kobject_delete(&disk->obj);
                }
            }
            kfree(g_disk.disk);
            g_disk.disk = NULL;
            return ret;
        }
    }

    return 0;
}

static void disk_device_remove(int index)
{
    struct disk_obj_s *disk = &g_disk.disk[index - 1];
    if (disk->obj) {
        sysfs_remove_group(&disk->obj->kobj, &disk_attr_group);
        switch_kobject_delete(&disk->obj);
        DISK_DBG("removed disk%d\n", index);
    }
}

static void disk_devices_remove(void)
{
    int i;
    struct disk_obj_s *disk;

    if (g_disk.disk) {
        for (i = g_disk.disk_number; i > 0; i--) {
            disk = &g_disk.disk[i - 1];
            disk_device_remove(i);
        }

        kfree(g_disk.disk);
        g_disk.disk = NULL;
    }

    g_disk.disk_number = 0;
}

static void disk_sysfs_remove(void)
{
    disk_devices_remove();

    if (g_disk_obj) {
        sysfs_remove_group(&g_disk_obj->kobj, &disk_root_attr_group);
        switch_kobject_delete(&g_disk_obj);
        g_disk_obj = NULL;
    }
}

static int disk_sysfs_create(void)
{
    int ret;

    ret = disk_root_create();
    if (ret) {
        return ret;
    }

    ret = disk_devices_create();
    if (ret) {
        disk_sysfs_remove();
        DISK_ERR("create disk dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_disk_drivers_register(struct s3ip_sysfs_disk_drivers_s *drv)
{
    int disk_number;
    int ret;

    DISK_INFO("s3ip_sysfs_disk_drivers_register...\n");
    if (g_disk_drv) {
        DISK_ERR("g_disk_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_disk_number);
    g_disk_drv = drv;

    disk_number = g_disk_drv->get_disk_number();
    if (disk_number <= 0) {
        g_disk_drv = NULL;
        DISK_ERR("Invalid disk number: %d\n", disk_number);
        return -EINVAL;
    }

    mem_clear(&g_disk, sizeof(struct disk_s));

    g_disk.disk_number = disk_number;

    ret = disk_sysfs_create();
    if (ret) {
        g_disk_drv = NULL;
        return ret;
    }

    DISK_INFO("Registered DISK driver with %d devices\n", disk_number);

    return 0;
}

void s3ip_sysfs_disk_drivers_unregister(void)
{
    if (g_disk_drv) {
        disk_sysfs_remove();
        g_disk_drv = NULL;
        DISK_INFO("DISK driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_disk_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_disk_drivers_unregister);
module_param(g_disk_loglevel, int, 0644);
MODULE_PARM_DESC(g_disk_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");