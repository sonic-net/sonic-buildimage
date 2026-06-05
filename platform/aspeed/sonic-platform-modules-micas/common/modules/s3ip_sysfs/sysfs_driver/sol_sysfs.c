/*
 * sol_sysfs.c
 *
 * This module create sol kobjects and attributes in /sys/s3ip/sol
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "sol_sysfs.h"

static int g_sol_loglevel = 0;

#define SOL_INFO(fmt, args...) do {                                        \
    if (g_sol_loglevel & INFO) { \
        printk(KERN_INFO "[SYSSOL_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SOL_ERR(fmt, args...) do {                                        \
    if (g_sol_loglevel & ERR) { \
        printk(KERN_ERR "[SYSSOL_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SOL_DBG(fmt, args...) do {                                        \
    if (g_sol_loglevel & DBG) { \
        printk(KERN_DEBUG "[SYSSOL_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct sol_obj_s {
    struct switch_obj *obj;
};

struct sol_s {
    unsigned int sol_number;
    struct sol_obj_s *sol;
};

static struct sol_s g_sol;
static struct switch_obj *g_sol_obj = NULL;

static struct s3ip_sysfs_sol_drivers_s *g_sol_drv = NULL;

static ssize_t sol_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_sol.sol_number);
}

static ssize_t sol_attr_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int index;
    struct switch_device_attribute *sol_attr;

    check_p(g_sol_drv);
    check_p(g_sol_drv->get_main_board_sol_attr);

    index = obj->index;
    SOL_DBG("sol index: %u\n", index);
    sol_attr = to_switch_device_attr(attr);
    check_p(sol_attr);

    return g_sol_drv->get_main_board_sol_attr(index, sol_attr->type, buf, PAGE_SIZE);
}

static ssize_t sol_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;
    unsigned int sol_index;
    struct switch_device_attribute *sol_attr;

    check_p(g_sol_drv);
    check_p(g_sol_drv->set_main_board_sol_attr);

    sol_index = obj->index;
    SOL_DBG("sol index: %u\n", sol_index);

    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SOL_ERR("invaild sol status ret: %d, can't set sol active.\n", ret);
        SOL_ERR("invaild sol status buf: %s\n", buf);
        return -EINVAL;
    }

    sol_attr = to_switch_device_attr(attr);
    check_p(sol_attr);

    ret = g_sol_drv->set_main_board_sol_attr(sol_index, sol_attr->type, value);
    if (ret < 0) {
        SOL_ERR("set sol active %d failed, ret: %d\n", value, ret);
        return ret;
    }

    SOL_DBG("set sol active %d success\n", value);
    return count;
}

/************************************sol dir and attrs*******************************************/
static struct switch_attribute sol_number_attr = __ATTR(number, S_IRUGO, sol_number_show, NULL);

static struct attribute *sol_dir_attrs[] = {
    &sol_number_attr.attr,
    NULL,
};

static struct attribute_group sol_root_attr_group = {
    .attrs = sol_dir_attrs,
};

/************************************sol[1-n] and attrs*******************************************/
static SWITCH_DEVICE_ATTR(name, S_IRUGO, sol_attr_show, NULL, DFD_SOL_NAME_E);
static SWITCH_DEVICE_ATTR(device, S_IRUGO, sol_attr_show, NULL, DFD_SOL_DEVICE_E);
static SWITCH_DEVICE_ATTR(active, S_IRUGO | S_IWUSR, sol_attr_show, sol_attr_store, DFD_SOL_ACTIVE_E);

static struct attribute *sol_attrs[] = {
    &switch_dev_attr_name.switch_attr.attr,
    &switch_dev_attr_device.switch_attr.attr,
    &switch_dev_attr_active.switch_attr.attr,
    NULL,
};

static struct attribute_group sol_attr_group = {
    .attrs = sol_attrs,
};

/* create sysesol directory and attributes*/
static int sol_root_create(void)
{
    g_sol_obj = switch_kobject_create("sol", NULL);
    if (!g_sol_obj) {
        SOL_ERR("switch_kobject_create sol error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_sol_obj->kobj, &sol_root_attr_group) != 0) {
        switch_kobject_delete(&g_sol_obj);
        SOL_ERR("create sol dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete sysesol directory and attributes*/
static void sol_root_remove(void)
{
    if (g_sol_obj) {
        sysfs_remove_group(&g_sol_obj->kobj, &sol_root_attr_group);
        switch_kobject_delete(&g_sol_obj);
    }

    return;
}

static int sol_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct sol_obj_s *sol;

    sol = &g_sol.sol[index - 1];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "sol%u", index);
    sol->obj = switch_kobject_create(name, parent);

    if (!sol->obj) {
        SOL_ERR("create %s object error.\n", name);
        return -ENOMEM;
    }

    sol->obj->index = index;
    if (sysfs_create_group(&sol->obj->kobj, &sol_attr_group) != 0) {
        SOL_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&sol->obj);
        return -EBADRQC;
    }

    SOL_ERR("create %s dir and attrs success.\n", name);
    return 0;
}

static void sol_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct sol_obj_s *sol;

    sol = &g_sol.sol[index - 1];
    if (sol->obj) {
        sysfs_remove_group(&sol->obj->kobj, &sol_attr_group);
        switch_kobject_delete(&sol->obj);
        SOL_DBG("delete sol%u dir and attrs success.\n", index);
    }

    return;
}

static int sol_sub_create_kobj_and_attrs(struct kobject *parent, int sol_number)
{
    unsigned int sol_index, i;

    g_sol.sol = kzalloc(sizeof(struct sol_obj_s) * sol_number, GFP_KERNEL);
    if (!g_sol.sol) {
        SOL_ERR("kzalloc g_sol.sol error, sol number: %d.\n", sol_number);
        return -ENOMEM;
    }

    for (sol_index = 1; sol_index <= sol_number; sol_index++) {
        if (sol_sub_single_create_kobj_and_attrs(parent, sol_index) != 0) {
            goto error;
        }
    }

    return 0;

error:
    for (i = sol_index - 1; i > 0; i--) {
        sol_sub_single_remove_kobj_and_attrs(i);
    }

    kfree(g_sol.sol);
    g_sol.sol = NULL;

    return -EBADRQC;
}

/* create leak_detetor[1-n] directory and attributes*/
static int sol_sub_create(void)
{
    int ret;

    ret = sol_sub_create_kobj_and_attrs(&g_sol_obj->kobj,
              g_sol.sol_number);
    return ret;
}

int s3ip_sysfs_sol_drivers_register(struct s3ip_sysfs_sol_drivers_s *drv)
{
    int ret, sol_number;

    SOL_INFO("s3ip_sysfs_sol_drivers_register...\n");
    if (g_sol_drv) {
        SOL_ERR("g_sol_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_sol_number);
    g_sol_drv = drv;

    sol_number = g_sol_drv->get_main_board_sol_number();
    if (sol_number <= 0) {
        SOL_ERR("sol_number is %d, can't register\n", sol_number);
        g_sol_drv = NULL;
        return -EINVAL;
    }

    mem_clear(&g_sol, sizeof(struct sol_s));
    g_sol.sol_number = sol_number;

    ret = sol_root_create();
    if (ret < 0) {
        SOL_ERR("sol create error.\n");
        g_sol_drv = NULL;
        return ret;
    }

    ret = sol_sub_create();
    if (ret < 0) {
        SOL_ERR("create sol sub dir and attrs failed, ret: %d\n", ret);
        sol_root_remove();
        g_sol_drv = NULL;
        return ret;
    }

    SOL_INFO("s3ip_sysfs_sol_drivers_register success\n");
    return 0;
}

/* delete sol[1-n] directory and attributes*/
static void sol_sub_remove(void)
{
    unsigned int sol_index;

    if (g_sol.sol) {
        for (sol_index = g_sol.sol_number; sol_index > 0; sol_index--) {
            sol_sub_single_remove_kobj_and_attrs(sol_index);
        }

        kfree(g_sol.sol);
        g_sol.sol = NULL;
    }

    g_sol.sol_number = 0;
    return;
}

void s3ip_sysfs_sol_drivers_unregister(void)
{
    if (g_sol_drv) {
        sol_sub_remove();
        sol_root_remove();
        g_sol_drv = NULL;
        SOL_DBG("s3ip_sysfs_sol_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_sol_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_sol_drivers_unregister);
module_param(g_sol_loglevel, int, 0644);
MODULE_PARM_DESC(g_sol_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");