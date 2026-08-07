/*
 * pll_sysfs.c
 *
 * This module create pll kobjects and attributes in /sys/s3ip/pll
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2025-09-18                 S3IP sysfs
 */

#include "switch.h"
#include "pll_sysfs.h"

static int g_pll_loglevel = 0;

#define PLL_INFO(fmt, args...) do { \
    if (g_pll_loglevel & INFO) { \
        printk(KERN_INFO "[PLL_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PLL_ERR(fmt, args...) do { \
    if (g_pll_loglevel & ERR) { \
        printk(KERN_ERR "[PLL_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define PLL_DBG(fmt, args...) do { \
    if (g_pll_loglevel & DBG) { \
        printk(KERN_DEBUG "[PLL_SYSFS][func:%s line:%d]" fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct pll_obj_s {
    struct switch_obj *obj;
};

struct pll_s {
    unsigned int pll_number;
    struct pll_obj_s *pll;
};

static struct pll_s g_pll;
static struct switch_obj *g_pll_obj = NULL;
static struct s3ip_sysfs_pll_drivers_s *g_pll_drv = NULL;

static ssize_t pll_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_pll.pll_number);
}

static ssize_t sys_pll_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *pll_attr;

    check_p(g_pll_drv);
    check_p(g_pll_drv->get_pll_attr);

    index = obj->index;
    PLL_DBG("pll index: %u\n", index);

    pll_attr = to_switch_device_attr(attr);
    check_p(pll_attr);

    return g_pll_drv->get_pll_attr(index, pll_attr->type, buf, PAGE_SIZE);
}

/************************************pll dir and attr*******************************************/
static struct switch_attribute pll_number_attr = __ATTR(number, S_IRUGO, pll_number_show, NULL);

static struct attribute *pll_root_attrs[] = {
    &pll_number_attr.attr,
    NULL,
};

static struct attribute_group pll_root_attr_group = {
    .attrs = pll_root_attrs,
};

/************************************pll[1-n] attr*******************************************/
static SWITCH_DEVICE_ATTR(bus_status, S_IRUGO, sys_pll_attr_show, NULL, DFD_PLL_BUS_STATUS_E);
static SWITCH_DEVICE_ATTR(alias, S_IRUGO, sys_pll_attr_show, NULL, DFD_PLL_ALIAS_E);

static struct attribute *pll_attrs[] = {
    &switch_dev_attr_bus_status.switch_attr.attr,
    &switch_dev_attr_alias.switch_attr.attr,
    NULL,
};

static struct attribute_group pll_attr_group = {
    .attrs = pll_attrs,
};

static int pll_root_create(void)
{
    g_pll_obj = switch_kobject_create("pll", NULL);
    if (!g_pll_obj) {
        PLL_ERR("create PLL sysfs object failed\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_pll_obj->kobj, &pll_root_attr_group) != 0) {
        switch_kobject_delete(&g_pll_obj);
        PLL_ERR("create PLL root attributes failed\n");
        return -EBADRQC;
    }

    return 0;
}

static int pll_device_create(int index)
{
    char name[32];
    struct pll_obj_s *pll;
    int ret;

    pll = &g_pll.pll[index - 1];

    snprintf(name, sizeof(name), "pll%u", index);
    pll->obj = switch_kobject_create(name, &g_pll_obj->kobj);
    if (!pll->obj) {
        PLL_ERR("create %s object failed\n", name);
        return -ENOMEM;
    }

    pll->obj->index = index;

    ret = sysfs_create_group(&pll->obj->kobj, &pll_attr_group);
    if (ret != 0) {
        switch_kobject_delete(&pll->obj);
        PLL_ERR("create %s attributes failed: %d\n", name, ret);
        return ret;
    }

    PLL_DBG("created %s directory and attributes\n", name);
    return 0;
}

static int pll_devices_create(void)
{
    int i, ret;
    struct pll_obj_s *pll;

    g_pll.pll = kzalloc(sizeof(struct pll_obj_s) * g_pll.pll_number, GFP_KERNEL);
    if (!g_pll.pll) {
        PLL_ERR("allocate pll objects failed\n");
        return -ENOMEM;
    }

    for (i = 1; i <= g_pll.pll_number; i++) {
        ret = pll_device_create(i);
        if (ret) {
            while (--i >= 1) {
                pll = &g_pll.pll[i - 1];
                if (pll->obj) {
                    sysfs_remove_group(&pll->obj->kobj, &pll_attr_group);
                    switch_kobject_delete(&pll->obj);
                }
            }
            kfree(g_pll.pll);
            g_pll.pll = NULL;
            return ret;
        }
    }

    return 0;
}

static void pll_device_remove(int index)
{
    struct pll_obj_s *pll = &g_pll.pll[index - 1];
    if (pll->obj) {
        sysfs_remove_group(&pll->obj->kobj, &pll_attr_group);
        switch_kobject_delete(&pll->obj);
        PLL_DBG("removed pll%d\n", index);
    }
}

static void pll_devices_remove(void)
{
    int i;
    struct pll_obj_s *pll;

    if (g_pll.pll) {
        for (i = g_pll.pll_number; i > 0; i--) {
            pll = &g_pll.pll[i - 1];
            pll_device_remove(i);
        }

        kfree(g_pll.pll);
        g_pll.pll = NULL;
    }

    g_pll.pll_number = 0;
}

static void pll_sysfs_remove(void)
{
    pll_devices_remove();

    if (g_pll_obj) {
        sysfs_remove_group(&g_pll_obj->kobj, &pll_root_attr_group);
        switch_kobject_delete(&g_pll_obj);
        g_pll_obj = NULL;
    }
}

static int pll_sysfs_create(void)
{
    int ret;

    ret = pll_root_create();
    if (ret) {
        return ret;
    }

    ret = pll_devices_create();
    if (ret) {
        pll_sysfs_remove();
        PLL_ERR("create pll dir and attrs failed, ret: %d\n", ret);

        return ret;
    }

    return 0;
}

int s3ip_sysfs_pll_drivers_register(struct s3ip_sysfs_pll_drivers_s *drv)
{
    int pll_number;
    int ret;

    PLL_INFO("s3ip_sysfs_pll_drivers_register...\n");
    if (g_pll_drv) {
        PLL_ERR("g_pll_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_pll_number);
    g_pll_drv = drv;

    pll_number = g_pll_drv->get_pll_number();
    if (pll_number <= 0) {
        g_pll_drv = NULL;
        PLL_ERR("Invalid pll number: %d\n", pll_number);
        return -EINVAL;
    }

    mem_clear(&g_pll, sizeof(struct pll_s));

    g_pll.pll_number = pll_number;

    ret = pll_sysfs_create();
    if (ret) {
        g_pll_drv = NULL;
        return ret;
    }

    PLL_INFO("Registered PLL driver with %d devices\n", pll_number);

    return 0;
}

void s3ip_sysfs_pll_drivers_unregister(void)
{
    if (g_pll_drv) {
        pll_sysfs_remove();
        g_pll_drv = NULL;
        PLL_INFO("PLL driver unregistered\n");
    }
}

EXPORT_SYMBOL(s3ip_sysfs_pll_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_pll_drivers_unregister);
module_param(g_pll_loglevel, int, 0644);
MODULE_PARM_DESC(g_pll_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");