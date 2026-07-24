/*
 * pcie_sysfs.c
 *
 * This module create pcie kobjects and attributes in /sys/s3ip/pcie
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "pcie_sysfs.h"

static int g_pcie_loglevel = 0;

#define PCIE_INFO(fmt, args...) do { \
    if (g_pcie_loglevel & INFO) { \
        printk(KERN_INFO "[PCIE_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PCIE_ERR(fmt, args...) do { \
    if (g_pcie_loglevel & ERR) { \
        printk(KERN_ERR "[PCIE_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PCIE_DBG(fmt, args...) do { \
    if (g_pcie_loglevel & DBG) { \
        printk(KERN_DEBUG "[PCIE_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct pcie_obj_s {
    struct switch_obj *obj;
};

struct pcie_s {
    unsigned int pcie_number;
    struct pcie_obj_s *pcie;
};

static struct pcie_s g_pcie;
static struct switch_obj *g_pcie_obj = NULL;
static struct s3ip_sysfs_pcie_drivers_s *g_pcie_drv = NULL;

static ssize_t pcie_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_pcie.pcie_number);
}

static ssize_t sys_pcie_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *pcie_attr;

    check_p(g_pcie_drv);
    check_p(g_pcie_drv->get_pcie_attr);

    index = obj->index;
    PCIE_DBG("pcie index: %u\n", index);

    pcie_attr = to_switch_device_attr(attr);
    check_p(pcie_attr);

    return g_pcie_drv->get_pcie_attr(index, pcie_attr->type, buf, PAGE_SIZE);
}

static ssize_t sys_pcie_attr_dbg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    unsigned int pcie_index;
    struct switch_device_attribute  *tmp_attr;

    check_p(g_pcie_drv);
    check_p(g_pcie_drv->set_debug_pcie_attr);
    
    pcie_index = obj->index;
    tmp_attr = to_switch_device_attr(attr);
    check_p(tmp_attr);

    ret = g_pcie_drv->set_debug_pcie_attr(tmp_attr->type, pcie_index, buf);
    if (ret < 0) {
        PCIE_ERR("pcie index: %u , type: %d , debug_value : %s, set fail.\n", pcie_index, tmp_attr->type, buf);
        return -EIO;
    }
    PCIE_DBG("pcie index: %u , type: %d , debug_value : %s, set success.\n", pcie_index, tmp_attr->type, buf);
    return count;
}

/************************************pcie dir and attr*******************************************/
static struct switch_attribute pcie_number_attr = __ATTR(number, S_IRUGO, pcie_number_show, NULL);

static struct attribute *pcie_root_attrs[] = {
    &pcie_number_attr.attr,
    NULL,
};

static struct attribute_group pcie_root_attr_group = {
    .attrs = pcie_root_attrs,
};

/************************************pcie[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(link_status, S_IRUGO | S_IWUSR, sys_pcie_attr_show, sys_pcie_attr_dbg_store, DFD_PCIE_LINK_STATUS_E);
static SWITCH_DEVICE_ATTR(speed_status, S_IRUGO | S_IWUSR, sys_pcie_attr_show, sys_pcie_attr_dbg_store, DFD_PCIE_SPEED_STATUS_E);
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, sys_pcie_attr_show, NULL, DFD_PCIE_ALIAS_E);
static SWITCH_DEVICE_ATTR(type, S_IRUGO, sys_pcie_attr_show, NULL, DFD_PCIE_TYPE_E);

static struct attribute *pcie_attrs[] = {
    &switch_dev_attr_link_status.switch_attr.attr,
    &switch_dev_attr_speed_status.switch_attr.attr,
    &switch_dev_attr_alias.switch_attr.attr,
    &switch_dev_attr_type.switch_attr.attr,
    NULL,
};

static struct attribute_group pcie_attr_group = {
    .attrs = pcie_attrs,
};

static int pcie_root_create(void)
{
    g_pcie_obj = switch_kobject_create("pcie", NULL);
    if (!g_pcie_obj) {
        PCIE_ERR("create PCIE sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_pcie_obj->kobj, &pcie_root_attr_group) != 0) {
        switch_kobject_delete(&g_pcie_obj);
        PCIE_ERR("create PCIE root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int pcie_device_create(int index)
{
    char name[32];
    struct pcie_obj_s *pcie;
    int ret;

    pcie = &g_pcie.pcie[index - 1];

    snprintf(name, sizeof(name), "pcie%u", index);
    pcie->obj = switch_kobject_create(name, &g_pcie_obj->kobj);
    if (!pcie->obj) {
        PCIE_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    pcie->obj->index = index;

    ret = sysfs_create_group(&pcie->obj->kobj, &pcie_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&pcie->obj);
        PCIE_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    PCIE_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int pcie_devices_create(void)
{
    int i, ret;
    struct pcie_obj_s *pcie;

    g_pcie.pcie = kzalloc(sizeof(struct pcie_obj_s) * g_pcie.pcie_number, GFP_KERNEL);
    if (!g_pcie.pcie) {
        PCIE_ERR("allocate pcie objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_pcie.pcie_number; i++) {
        ret = pcie_device_create(i);
        if (ret) {
            while (--i >= 1) {
                pcie = &g_pcie.pcie[i - 1];
                if (pcie->obj) {
                    sysfs_remove_group(&pcie->obj->kobj, &pcie_attr_group);
                    switch_kobject_delete(&pcie->obj);
                }
            }
            kfree(g_pcie.pcie);
            g_pcie.pcie = NULL;
            return ret;
        }
    }

    return 0;
}

static void pcie_device_remove(int index)
{
    struct pcie_obj_s *pcie = &g_pcie.pcie[index - 1];
    if (pcie->obj) {
        sysfs_remove_group(&pcie->obj->kobj, &pcie_attr_group);
        switch_kobject_delete(&pcie->obj);
        PCIE_DBG("removed pcie%d\n", index);
    }
}

static void pcie_devices_remove(void)
{
    int i;
    struct pcie_obj_s *pcie;

    if (g_pcie.pcie) {
        for (i = g_pcie.pcie_number; i > 0; i--) {
            pcie = &g_pcie.pcie[i - 1];
            pcie_device_remove(i);
        }

        kfree(g_pcie.pcie);
        g_pcie.pcie = NULL;
    }

    g_pcie.pcie_number = 0;
}

static void pcie_sysfs_remove(void)
{
    pcie_devices_remove();

    if (g_pcie_obj) {
        sysfs_remove_group(&g_pcie_obj->kobj, &pcie_root_attr_group);
        switch_kobject_delete(&g_pcie_obj);
        g_pcie_obj = NULL;
    }
}

static int pcie_sysfs_create(void)
{
    int ret;

    ret = pcie_root_create();
    if (ret) {
        return ret;
    }

    ret = pcie_devices_create();
    if (ret) {
        pcie_sysfs_remove();
        PCIE_ERR("create pcie dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_pcie_drivers_register(struct s3ip_sysfs_pcie_drivers_s *drv)
{
    int pcie_number;
    int ret;

    PCIE_INFO("s3ip_sysfs_pcie_drivers_register...\n");
    if (g_pcie_drv) {
        PCIE_ERR("g_pcie_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_pcie_number);
    g_pcie_drv = drv;

    pcie_number = g_pcie_drv->get_pcie_number();
    if (pcie_number <= 0) {
        g_pcie_drv = NULL;
        PCIE_ERR("Invalid pcie number: %d\n", pcie_number);
        return -EINVAL;
    }

    mem_clear(&g_pcie, sizeof(struct pcie_s));

    g_pcie.pcie_number = pcie_number;

    ret = pcie_sysfs_create();
    if (ret) {
        g_pcie_drv = NULL;
        return ret;
    }

    PCIE_INFO("Registered PCIE driver with %d devices\n", pcie_number);

    return 0;
}

void s3ip_sysfs_pcie_drivers_unregister(void)
{
    if (g_pcie_drv) {
        pcie_sysfs_remove();
        g_pcie_drv = NULL;
        PCIE_INFO("PCIE driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_pcie_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_pcie_drivers_unregister);
module_param(g_pcie_loglevel, int, 0644);
MODULE_PARM_DESC(g_pcie_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");