/*
 * plat_psu.c
 * Original Author: support 2020-02-17
 *
 * This module create psu kobjects and attributes in /sys/wb_plat/psu
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0          support                 2020-02-17          Initial version
 */

#include <linux/slab.h>

#include "./include/plat_switch.h"
#include "./include/sysfs_common.h"

#define PSU_INFO(fmt, args...) LOG_INFO("psu: ", fmt, ##args)
#define PSU_ERR(fmt, args...)  LOG_ERR("psu: ", fmt, ##args)
#define PSU_DBG(fmt, args...)  LOG_DBG("psu: ", fmt, ##args)

struct temp_obj_t{
    struct switch_obj *obj;
};

struct psu_obj_t{
    unsigned int temp_number;
    struct temp_obj_t *temp;
    struct switch_obj *obj;
};

struct psu_t{
    unsigned int psu_number;
    struct psu_obj_t *psu;
};

static int g_loglevel = 0;
static struct psu_t g_psu;
static struct switch_obj *g_psu_obj = NULL;
static struct switch_drivers_t *g_drv = NULL;

static ssize_t psu_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_psu.psu_number);
}

static ssize_t psu_present_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    psu_index = obj->index;
    PSU_DBG("psu_present_status_show, psu index:%d\n",psu_index);
    check_p(g_drv);
    check_p(g_drv->get_psu_present_status);

    ret = g_drv->get_psu_present_status(psu_index);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t psu_output_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    psu_index = obj->index;
    PSU_DBG("psu_output_status_show, psu index:%d\n",psu_index);
    check_p(g_drv);
    check_p(g_drv->get_psu_output_status);

    ret = g_drv->get_psu_output_status(psu_index);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t psu_alert_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int psu_index;
    int ret;

    psu_index = obj->index;
    PSU_DBG("psu_alert_status_show, psu index:%d\n",psu_index);
    check_p(g_drv);
    check_p(g_drv->get_psu_alert_status);

    ret = g_drv->get_psu_alert_status(psu_index);
    if(ret < 0) {
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", WB_SYSFS_DEV_ERROR);
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

/************************************psu dir and attrs*******************************************/
static struct switch_attribute psu_number_att = __ATTR(num_psus, S_IRUGO, psu_number_show, NULL);

static struct attribute *psu_dir_attrs[] = {
    &psu_number_att.attr,
    NULL,
};

static struct attribute_group psu_root_attr_group = {
    .attrs = psu_dir_attrs,
};

/*******************************psu1 psu2 dir and attrs*******************************************/
static struct switch_attribute psu_present_status_att = __ATTR(present, S_IRUGO, psu_present_status_show, NULL);
static struct switch_attribute psu_output_status_att = __ATTR(output, S_IRUGO, psu_output_status_show, NULL);
static struct switch_attribute psu_alert_status_att = __ATTR(alert, S_IRUGO, psu_alert_status_show, NULL);

static struct attribute *psu_attrs[] = {
    &psu_present_status_att.attr,
    &psu_output_status_att.attr,
    &psu_alert_status_att.attr,
    NULL,
};

static struct attribute_group psu_attr_group = {
    .attrs = psu_attrs,
};

/*******************************psu temp0 temp1 dir and attrs*******************************************/
static struct attribute *psu_temp_attrs[] = {
    NULL,
};

static struct attribute_group psu_temp_attr_group = {
    .attrs = psu_temp_attrs,
};

static void psuindex_single_temp_remove_kobj_and_attrs(struct psu_obj_t * curr_psu, unsigned int temp_index)
{

    struct temp_obj_t *curr_temp; /* point to temp0 temp1...*/

    curr_temp = &curr_psu->temp[temp_index];
    if (curr_temp->obj) {
        sysfs_remove_group(&curr_temp->obj->kobj, &psu_temp_attr_group);
        wb_plat_kobject_delete(&curr_temp->obj);
        PSU_DBG("delete psu:%d temp%d.\n", curr_psu->obj->index, temp_index);
    }
    return;
}

static int psuindex_single_temp_create_kobj_and_attrs(struct psu_obj_t * curr_psu, unsigned int temp_index)
{
    char name[DIR_NAME_MAX_LEN];
    struct temp_obj_t *curr_temp; /* point to temp0 temp1...*/

    PSU_DBG("create psu_index:%d, temp%d ...\n", curr_psu->obj->index, temp_index);

    curr_temp = &curr_psu->temp[temp_index];
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "temp%d", temp_index);
    curr_temp->obj = wb_plat_kobject_create(name, &curr_psu->obj->kobj);
    if (!curr_temp->obj) {
        PSU_ERR("create psu_index:%d, temp%d object error!\n", curr_psu->obj->index, temp_index);
        return -EBADRQC;
    }
    curr_temp->obj->index = temp_index;
    if (sysfs_create_group(&curr_temp->obj->kobj, &psu_temp_attr_group) != 0) {
        PSU_ERR("create psu_index:%d, temp%d attrs error.\n", curr_psu->obj->index, temp_index);
        wb_plat_kobject_delete(&curr_temp->obj);
        return -EBADRQC;
    }
    PSU_DBG("create psu_index:%d, temp%d ok.\n", curr_psu->obj->index, temp_index);
    return 0;
}

static int psuindex_temp_create_kobj_and_attrs(struct psu_obj_t * curr_psu, int temp_num)
{
    int temp_index, i;

    curr_psu->temp = kzalloc(sizeof(struct temp_obj_t) * temp_num, GFP_KERNEL);
    if (!curr_psu->temp) {
        PSU_ERR("kzalloc temp error, psu index = %d, temp number = %d.\n", curr_psu->obj->index, temp_num);
        return -ENOMEM;
    }
    curr_psu->temp_number = temp_num;
    for (temp_index = 0; temp_index < temp_num; temp_index++) {
        if (psuindex_single_temp_create_kobj_and_attrs(curr_psu, temp_index) != 0) {
            goto temp_error;
        }
    }
    return 0;
temp_error:
    for (i = temp_index - 1; i >= 0; i--) {
        psuindex_single_temp_remove_kobj_and_attrs(curr_psu, i);
    }
    if (curr_psu->temp) {
        kfree(curr_psu->temp);
        curr_psu->temp = NULL;
    }
    return -EBADRQC;
}

static void psuindex_temp_remove_kobj_and_attrs(struct psu_obj_t * curr_psu, int temp_num)
{
    unsigned int temp_index;

    for (temp_index = temp_num - 1; temp_index >= 0; temp_index--) {
        psuindex_single_temp_remove_kobj_and_attrs(curr_psu, temp_index);
    }
    return;
}

static int psu_temp_create(void)
{
    int psu_num, temp_num;
    unsigned int psu_index, i;
    struct psu_obj_t *curr_psu;     /* point to psu1 psu2...*/

    check_p(g_drv->get_dev_number);
    temp_num = g_drv->get_dev_number(WB_MAIN_DEV_PSU, WB_MINOR_DEV_TEMP);
    if (temp_num <= 0) {
        PSU_INFO("psu temp_num:%d, don't need creat temp directory.\n", temp_num);
        return 0;
    }

    psu_num = g_psu.psu_number;
    for(psu_index = 1; psu_index <= psu_num; psu_index++) {
        curr_psu = &g_psu.psu[psu_index - 1];
        if(psuindex_temp_create_kobj_and_attrs(curr_psu, temp_num) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for(i = psu_index - 1; i > 0; i--) {
        curr_psu = &g_psu.psu[i - 1];
        temp_num = curr_psu->temp_number;
        psuindex_temp_remove_kobj_and_attrs(curr_psu, temp_num);
    }
    return -EBADRQC;
}

static void psu_temp_remove(void)
{
    unsigned int psu_index;
    struct psu_obj_t *curr_psu;

    if (g_psu.psu) {
       for (psu_index = g_psu.psu_number; psu_index > 0; psu_index--) {
           curr_psu = &g_psu.psu[psu_index - 1];
           if (curr_psu->temp) {
               psuindex_temp_remove_kobj_and_attrs(curr_psu,curr_psu->temp_number);
               kfree(curr_psu->temp);
               curr_psu->temp = NULL;
               curr_psu->temp_number = 0;
           }
       }
    }
    return;
}

static void psu_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct psu_obj_t *curr_psu;

    curr_psu = &g_psu.psu[index - 1];
    if (curr_psu->obj) {
        sysfs_remove_group(&curr_psu->obj->kobj, &psu_attr_group);
        wb_plat_kobject_delete(&curr_psu->obj);
        PSU_DBG("delete psu%d.\n", index);
    }
    return;
}

static int psu_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[DIR_NAME_MAX_LEN];
    struct psu_obj_t *curr_psu;

    curr_psu = &g_psu.psu[index-1];
    PSU_DBG("create psu%d ...\n", index);
    mem_clear(name, sizeof(name));
    snprintf(name, sizeof(name), "%s%d",PSU_SYSFS_NAME, index);
    curr_psu->obj = wb_plat_kobject_create(name, parent);
    if (!curr_psu->obj) {
        PSU_ERR("create psu%d object error!\n", index);
        return -EBADRQC;
    }
    curr_psu->obj->index = index;
    if (sysfs_create_group(&curr_psu->obj->kobj, &psu_attr_group) != 0) {
        PSU_ERR("create psu%d attrs error.\n", index);
        wb_plat_kobject_delete(&curr_psu->obj);
        return -EBADRQC;
    }
    PSU_DBG("create psu%d ok.\n", index);
    return 0;
}

static int psu_sub_create_kobj_and_attrs(struct kobject *parent, int psu_num)
{
    unsigned int psu_index, i;

    g_psu.psu = kzalloc(sizeof(struct psu_obj_t) * psu_num, GFP_KERNEL);
    if (!g_psu.psu) {
        PSU_ERR("kzalloc psu.psu error, psu number = %d.\n", psu_num);
        return -ENOMEM;
    }

    for (psu_index = 1; psu_index <= psu_num; psu_index++) {
        if (psu_sub_single_create_kobj_and_attrs(parent, psu_index) != 0) {
            goto error;
        }
    }
    return 0;
error:
    for(i = psu_index - 1; i > 0; i--) {
        psu_sub_single_remove_kobj_and_attrs(i);
    }
    if(g_psu.psu) {
        kfree(g_psu.psu);
        g_psu.psu = NULL;
    }
    return -EBADRQC;
}

static int psu_sub_create(void)
{
    int ret, psu_num;

    check_p(g_drv->get_dev_number);
    psu_num = g_drv->get_dev_number(WB_MAIN_DEV_PSU, WB_MINOR_DEV_NONE);
    if (psu_num < 0) {
        PSU_ERR("psu number = %d error.\n", psu_num);
        return -EINVAL;
    }
    g_psu.psu_number = psu_num;
    ret = psu_sub_create_kobj_and_attrs(&g_psu_obj->kobj, psu_num);
    return ret;
}

static void psu_sub_remove(void)
{
    unsigned int psu_index;

    if (g_psu.psu) {
       for (psu_index = g_psu.psu_number; psu_index > 0; psu_index--) {
           psu_sub_single_remove_kobj_and_attrs(psu_index);
       }
       kfree(g_psu.psu);
    }
    mem_clear(&g_psu, sizeof(struct psu_t));
    return ;
}

static int psu_root_create(void)
{
    g_psu_obj = wb_plat_kobject_create(PSU_SYSFS_NAME, NULL);
    if (!g_psu_obj) {
        PSU_ERR("wb_plat_kobject_create psu error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_psu_obj->kobj, &psu_root_attr_group) != 0) {
        wb_plat_kobject_delete(&g_psu_obj);
        PSU_ERR("create psu dir attrs error!\n");
        return -EBADRQC;
    }
    PSU_DBG("wb_plat_kobject_create psu directory and attribute success.\n");
    return 0;
}

static void psu_root_remove(void)
{
    if (g_psu_obj) {
        sysfs_remove_group(&g_psu_obj->kobj, &psu_root_attr_group);
        wb_plat_kobject_delete(&g_psu_obj);
        PSU_DBG("delete psu root success\n");
    }
    return;
}

static int wb_psu_init(void)
{
    int ret;

    PSU_INFO("wb_psu_init...\n");
    g_drv = dfd_plat_driver_get();
    check_p(g_drv);

    ret = psu_root_create();
    if (ret < 0) {
        goto psu_root_error;
    }

    ret = psu_sub_create();
    if (ret < 0) {
        goto psu_sub_error;
    }

    ret = psu_temp_create();
    if (ret < 0) {
        goto psu_temp_error;
    }

    PSU_INFO("wb_psu_init ok.\n");
    return 0;
psu_temp_error:
    psu_sub_remove();
psu_sub_error:
    psu_root_remove();
psu_root_error:
    return ret;
}

static void wb_psu_exit(void)
{
    psu_temp_remove();
    psu_sub_remove();
    psu_root_remove();
    PSU_INFO("wb_psu_exit ok.\n");
    return ;
}

module_init(wb_psu_init);
module_exit(wb_psu_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("psu sysfs driver");
