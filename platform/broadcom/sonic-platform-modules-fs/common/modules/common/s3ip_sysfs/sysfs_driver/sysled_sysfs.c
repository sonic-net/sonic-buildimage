/*
 * sysled_sysfs.c
 *
 * This module create sysled kobjects and attributes in /sys/s3ip/sysled
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "sysled_sysfs.h"

static int g_sysled_loglevel = 0;
static int g_bmc_sys_led_count[BMC_SYS_LED_STATUS_MAX];
#define SYSLED_INFO(fmt, args...) do {                                        \
    if (g_sysled_loglevel & INFO) { \
        printk(KERN_INFO "[SYSLED_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SYSLED_ERR(fmt, args...) do {                                        \
    if (g_sysled_loglevel & ERR) { \
        printk(KERN_ERR "[SYSLED_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SYSLED_DBG(fmt, args...) do {                                        \
    if (g_sysled_loglevel & DBG) { \
        printk(KERN_DEBUG "[SYSLED_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct switch_obj *g_sysled_obj = NULL;
static struct s3ip_sysfs_sysled_drivers_s *g_sysled_drv = NULL;

static ssize_t sys_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->get_sys_led_status);

    return g_sysled_drv->get_sys_led_status(buf, PAGE_SIZE);
}

static ssize_t sys_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_sys_led_status);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SYSLED_ERR("invaild led status ret: %d, can't set sys led status\n", ret);
        SYSLED_ERR("invaild led status buf: %s\n", buf);
        return -EINVAL;
    }
    ret = g_sysled_drv->set_sys_led_status(value);
    if (ret < 0) {
        SYSLED_ERR("set sys led status %d faield, ret: %d\n", value, ret);
        return ret;
    }
    SYSLED_DBG("set sys led status %d success\n", value);
    return count;
}

static ssize_t bmc_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->get_bmc_led_status);

    return g_sysled_drv->get_bmc_led_status(buf, PAGE_SIZE);
}

static ssize_t bmc_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_bmc_led_status);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SYSLED_ERR("invaild led status ret: %d, can't set bmc led status\n", ret);
        SYSLED_ERR("invaild led status buf: %s\n", buf);
        return -EINVAL;
    }
    ret = g_sysled_drv->set_bmc_led_status(value);
    if (ret < 0) {
        SYSLED_ERR("set bmc led status %d faield, ret: %d\n", value, ret);
        return ret;
    }
    SYSLED_DBG("set bmc led status %d success\n", value);
    return count;
}

static ssize_t sys_fan_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->get_sys_fan_led_status);

    return g_sysled_drv->get_sys_fan_led_status(buf, PAGE_SIZE);
}

static ssize_t sys_fan_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_sys_fan_led_status);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SYSLED_ERR("invaild led status ret: %d, can't set sys fan led status\n", ret);
        SYSLED_ERR("invaild led status buf: %s\n", buf);
        return -EINVAL;
    }
    ret = g_sysled_drv->set_sys_fan_led_status(value);
    if (ret < 0) {
        SYSLED_ERR("set sys fan led status %d faield, ret: %d\n", value, ret);
        return ret;
    }
    SYSLED_DBG("set sys fan led status %d success\n", value);
    return count;
}

static ssize_t sys_psu_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->get_sys_psu_led_status);

    return g_sysled_drv->get_sys_psu_led_status(buf, PAGE_SIZE);
}

static ssize_t sys_psu_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_sys_psu_led_status);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SYSLED_ERR("invaild led status ret: %d, can't set sys psu led status\n", ret);
        SYSLED_ERR("invaild led status buf: %s\n", buf);
        return -EINVAL;
    }
    ret = g_sysled_drv->set_sys_psu_led_status(value);
    if (ret < 0) {
        SYSLED_ERR("set sys psu led status %d faield, ret: %d\n", value, ret);
        return ret;
    }
    SYSLED_DBG("set sys psu led status %d success\n", value);
    return count;
}

static ssize_t id_led_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->get_id_led_status);

    return g_sysled_drv->get_id_led_status(buf, PAGE_SIZE);
}

static ssize_t id_led_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char *buf, size_t count)
{
    int ret, value;

    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_id_led_status);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret != 0) {
        SYSLED_ERR("invaild led status ret: %d, can't set id led status\n", ret);
        SYSLED_ERR("invaild led status buf: %s\n", buf);
        return -EINVAL;
    }
    ret = g_sysled_drv->set_id_led_status(value);
    if (ret < 0) {
        SYSLED_ERR("set id led status %d faield, ret: %d\n", value, ret);
        return ret;
    }
    SYSLED_DBG("set id led status %d success\n", value);
    return count;
}

static ssize_t bmc_host_sysled_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int ret;
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->get_bmc_host_sysled);

    ret = g_sysled_drv->get_bmc_host_sysled(buf, PAGE_SIZE);
    if (ret < 0) {
        SYSLED_ERR("Failed to get bmc_host_sysled status. Error code: %d\n", ret);
        return ret;
    }

    return ret;
}

static int get_led_info_by_state(s3ip_led_status_t status, sys_led_info_t *info)
{
    switch (status) {
    case S3IP_LED_STATUS_DARK:
        info->sys_led_value = S3IP_LED_STATUS_DARK;
        info->sys_led_priority= SYS_LED_PRIORITY_DARK;
        break;
    case S3IP_LED_STATUS_GREEN:
        info->sys_led_value = S3IP_LED_STATUS_GREEN;
        info->sys_led_priority= SYS_LED_PRIORITY_GREEN;
        break;
    case S3IP_LED_STATUS_RED:
        info->sys_led_value = S3IP_LED_STATUS_RED;
        info->sys_led_priority= SYS_LED_PRIORITY_RED;
        break;
    case S3IP_LED_STATUS_YELLOW:
        info->sys_led_value = S3IP_LED_STATUS_YELLOW;
        info->sys_led_priority= SYS_LED_PRIORITY_YELLOW;
        break;
    case S3IP_LED_STATUS_GREEN_FLASH:
        info->sys_led_value = S3IP_LED_STATUS_GREEN_FLASH;
        info->sys_led_priority= SYS_LED_PRIORITY_GREEN_4HZ_FLASH;
        break;
    case S3IP_LED_STATUS_YELLOW_FLASH:
        info->sys_led_value = S3IP_LED_STATUS_YELLOW_FLASH;
        info->sys_led_priority= SYS_LED_PRIORITY_GREEN;
        break;
    case S3IP_LED_STATUS_RED_FLASH:
        info->sys_led_value = S3IP_LED_STATUS_RED_FLASH;
        info->sys_led_priority= SYS_LED_PRIORITY_GREEN;
        break;
    default:
        return -1;
    }

    return 0;
}

static int monitor_host_sys_led_check(s3ip_led_status_t led_status, int get_host_sys_led_value)
{
    int rv;
    sys_led_info_t bmc_sys_led_info;
    sys_led_info_t host_sys_led_info;

    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_bmc_host_sysled_attr);
    check_p(g_sysled_drv->set_sys_led_status);

    rv = get_led_info_by_state(led_status, &bmc_sys_led_info);
    if (rv < 0) {
        SYSLED_ERR("get bmc sys led[state=%d] info fail, rv=%d\n", led_status, rv);
        return -1;
    }

    SYSLED_DBG("led_status = 0x%x.\n", led_status);

    rv = get_led_info_by_state(get_host_sys_led_value, &host_sys_led_info);
    if (rv < 0) {
        SYSLED_ERR("get host sys led[value=%d] info fail, rv=%d\n", get_host_sys_led_value, rv);
        return -1;
    }

    SYSLED_DBG("get_host_sys_led_value = 0x%x.\n", get_host_sys_led_value);

    if ((bmc_sys_led_info.sys_led_priority <= SYS_LED_PRIORITY_MIN)
        || (bmc_sys_led_info.sys_led_priority >= SYS_LED_PRIORITY_MAX)
        || (host_sys_led_info.sys_led_priority <= SYS_LED_PRIORITY_MIN)
        || (host_sys_led_info.sys_led_priority >= SYS_LED_PRIORITY_MAX)) {
        SYSLED_ERR("sys_led_priority invalid\n");
        return -1;
    }

    /* if sys led priority lower than bmc sys led, than write value to sys led */
    if (host_sys_led_info.sys_led_priority > bmc_sys_led_info.sys_led_priority) {
        rv = g_sysled_drv->set_sys_led_status(bmc_sys_led_info.sys_led_value);
        if (rv < 0) {
            /* In dfd, when the function is not supported, ret = -999 */
            SYSLED_ERR("set host led reg value failed. type: %d, value: %d, rv:%d\n", WB_SYS_LED_FRONT, bmc_sys_led_info.sys_led_value, rv);
            return rv;
        }
        SYSLED_DBG("host sys_led_priority success, set host sys led 0x%x\n", bmc_sys_led_info.sys_led_value);
    }

    return 0;
}

static ssize_t bmc_host_sysled_attr_store(struct switch_obj *obj, struct switch_attribute *attr,
                    const char *buf, size_t count)
{
    int ret, bmc_set_host_sys_led, retry_count;
    int get_host_sys_led_value;
    static int pre_bmc_sys_led = S3IP_LED_STATUS_GREEN;
    char host_buf[WB_MAX_S3IP_NODE_STR_LEN];
    check_p(g_sysled_drv);
    check_p(g_sysled_drv->set_bmc_host_sysled_attr);
    check_p(g_sysled_drv->get_sys_led_status);

    bmc_set_host_sys_led = 0;
    ret = kstrtoint(buf, 0, &bmc_set_host_sys_led);
    if (ret != 0) {
        SYSLED_ERR("input parameter: %s error. ret:%d\n", buf, ret);
        return -EINVAL;
    }

    if ((bmc_set_host_sys_led != S3IP_LED_STATUS_GREEN) && (bmc_set_host_sys_led != S3IP_LED_STATUS_RED)) {
        SYSLED_ERR("input parameter: %d error.\n", bmc_set_host_sys_led);
        return -EINVAL;
    }

    /* Check array index range to prevent out-of-bounds access */
    if (bmc_set_host_sys_led < 0 || bmc_set_host_sys_led >= ARRAY_SIZE(g_bmc_sys_led_count)) {
        SYSLED_ERR("Invalid bmc_set_host_sys_led: %d\n", bmc_set_host_sys_led);
        return -EINVAL;
    }

    ret = g_sysled_drv->set_bmc_host_sysled_attr(bmc_set_host_sys_led);
    if (ret < 0) {
        /* In dfd, when the function is not supported, ret = -999 */
        SYSLED_ERR("set bmc host led reg value failed. type: %d, value: %d, ret:%d\n", BMC_HOST_SYS_LED, bmc_set_host_sys_led, ret);
        return ret;
    }

    SYSLED_DBG("type: 0x%x. value=%d\n", BMC_HOST_SYS_LED, bmc_set_host_sys_led);
    memset(host_buf, 0, WB_MAX_S3IP_NODE_STR_LEN);
    /* check sys led priority */
    if (bmc_set_host_sys_led == pre_bmc_sys_led) {
        retry_count = g_bmc_sys_led_count[bmc_set_host_sys_led] + 1;
        memset(g_bmc_sys_led_count, 0, sizeof(g_bmc_sys_led_count));
        g_bmc_sys_led_count[bmc_set_host_sys_led] = retry_count;

        /* shake check */
        if (g_bmc_sys_led_count[bmc_set_host_sys_led] >= HOST_SYS_LED_COUNT_CHECK) {
            ret = g_sysled_drv->get_sys_led_status(host_buf, WB_MAX_S3IP_NODE_STR_LEN);
            if (ret < 0) {
                /* In dfd, when the function is not supported, ret = -999 */
                SYSLED_ERR("get host led reg value failed. type: %d, ret:%d\n", WB_SYS_LED_FRONT, ret);
                return ret;
            }
            ret = kstrtoint(host_buf, 0, &get_host_sys_led_value);
            if (ret != 0) {
               SYSLED_ERR("input parameter: %s error. ret:%d\n", buf, ret);
               return -EINVAL;
            }
            ret = monitor_host_sys_led_check(bmc_set_host_sys_led, get_host_sys_led_value);
            if (ret < 0) {
                SYSLED_ERR("check host sys led fail, rv=%d\n", ret);
                return ret;
            }
            g_bmc_sys_led_count[bmc_set_host_sys_led] = 0;
        }
    } else {
        memset(g_bmc_sys_led_count, 0, sizeof(g_bmc_sys_led_count));
    }

    pre_bmc_sys_led = bmc_set_host_sys_led;

    SYSLED_DBG("set system reg value success. type: %d, value: %d.\n", BMC_HOST_SYS_LED, bmc_set_host_sys_led);
    return count;
}

/************************************syseeprom dir and attrs*******************************************/
static struct switch_attribute sys_led_attr = __ATTR(sys_led_status, S_IRUGO | S_IWUSR, sys_led_status_show, sys_led_status_store);
static struct switch_attribute bmc_led_attr = __ATTR(bmc_led_status, S_IRUGO | S_IWUSR, bmc_led_status_show, bmc_led_status_store);
static struct switch_attribute fan_led_attr = __ATTR(fan_led_status, S_IRUGO | S_IWUSR, sys_fan_led_status_show, sys_fan_led_status_store);
static struct switch_attribute psu_led_attr = __ATTR(psu_led_status, S_IRUGO | S_IWUSR, sys_psu_led_status_show, sys_psu_led_status_store);
static struct switch_attribute id_led_attr = __ATTR(id_led_status, S_IRUGO | S_IWUSR, id_led_status_show, id_led_status_store);
static struct switch_attribute bmc_host_sysle_attr = __ATTR(bmc_host_sysled, S_IRUGO | S_IWUSR, bmc_host_sysled_show, bmc_host_sysled_attr_store);

static struct attribute *sysled_dir_attrs[] = {
    &sys_led_attr.attr,
    &bmc_led_attr.attr,
    &fan_led_attr.attr,
    &psu_led_attr.attr,
    &id_led_attr.attr,
    &bmc_host_sysle_attr.attr,
    NULL,
};

static struct attribute_group sysled_attr_group = {
    .attrs = sysled_dir_attrs,
};

/* create syseled directory and attributes*/
static int sysled_root_create(void)
{
    g_sysled_obj = switch_kobject_create("sysled", NULL);
    if (!g_sysled_obj) {
        SYSLED_ERR("switch_kobject_create sysled error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_sysled_obj->kobj, &sysled_attr_group) != 0) {
        switch_kobject_delete(&g_sysled_obj);
        SYSLED_ERR("create sysled dir attrs error!\n");
        return -EBADRQC;
    }

    return 0;
}

/* delete syseled directory and attributes*/
static void sysled_root_remove(void)
{
    if (g_sysled_obj) {
        sysfs_remove_group(&g_sysled_obj->kobj, &sysled_attr_group);
        switch_kobject_delete(&g_sysled_obj);
    }

    return;
}

int s3ip_sysfs_sysled_drivers_register(struct s3ip_sysfs_sysled_drivers_s *drv)
{
    int ret;

    SYSLED_INFO("s3ip_sysfs_sysled_drivers_register...\n");
    if (g_sysled_drv) {
        SYSLED_ERR("g_sysled_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    g_sysled_drv = drv;

    ret = sysled_root_create();
    if (ret < 0) {
        SYSLED_ERR("sysled create error.\n");
        g_sysled_drv = NULL;
        return ret;
    }
    SYSLED_INFO("s3ip_sysfs_sysled_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_sysled_drivers_unregister(void)
{
    if (g_sysled_drv) {
        sysled_root_remove();
        g_sysled_drv = NULL;
        SYSLED_DBG("s3ip_sysfs_sysled_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_sysled_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_sysled_drivers_unregister);
module_param(g_sysled_loglevel, int, 0644);
MODULE_PARM_DESC(g_sysled_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
