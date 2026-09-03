/*
 * system_sysfs.c
 *
 * This module create temp sensor kobjects and attributes in /sys/s3ip/system
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "system_sysfs.h"
#include "switch_driver.h"

typedef enum system_overtemp_reboot_flag_e {
    SYSTEM_OVERTEMP_REBOOT_DEFAULT = 0,
    SYSTEM_OVERTEMP_REBOOT_ENABLE = 1,
    SYSTEM_OVERTEMP_REBOOT_DISABLE = 2,
} system_overtemp_reboot_flag_t;

static int g_system_loglevel = 0;
static int system_status_value = 0;
static DEFINE_MUTEX(system_status_mutex);
static int overtemp_reboot_value = SYSTEM_OVERTEMP_REBOOT_DEFAULT;
static int g_bmc_sw_status = BMC_SW_UNKNOWN;
static DEFINE_MUTEX(system_mutex);
typedef enum system_status_value_e {
    SYSTEM_STATUS_OK = 0,
    SYSTEM_STATUS_NOTICE = 1,
    SYSTEM_STATUS_WARNING = 2,
    SYSTEM_STATUS_CRITICAL = 3,
    SYSTEM_STATUS_MAX,
} system_status_value_t;

#define SYSTEM_INFO(fmt, args...) do {                                        \
    if (g_system_loglevel & INFO) { \
        printk(KERN_INFO "[system][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SYSTEM_ERR(fmt, args...) do {                                        \
    if (g_system_loglevel & ERR) { \
        printk(KERN_ERR "[system][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SYSTEM_DBG(fmt, args...) do {                                        \
    if (g_system_loglevel & DBG) { \
        printk(KERN_DEBUG "[system][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct system_obj_s {
    struct switch_obj *obj;
};

struct system_s {
    unsigned int api_number;
    struct system_obj_s *temp;
};

static struct s3ip_sysfs_system_drivers_s *g_system_drv = NULL;
static struct switch_obj *g_system_obj = NULL;

static ssize_t bmc_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_system_drv);
    check_p(g_system_drv->get_bmc_status);

    return g_system_drv->get_bmc_status(g_bmc_sw_status, buf, PAGE_SIZE);
}

static ssize_t bmc_sw_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", g_bmc_sw_status);
}

static ssize_t bmc_sw_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret, bmc_sw_status;

    bmc_sw_status = 0;
    ret = kstrtoint(buf, 0, &bmc_sw_status);
    if (ret != 0) {
        SYSTEM_ERR("invaild bmc_sw_status ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (bmc_sw_status < BMC_SW_UNKNOWN || bmc_sw_status > BMC_SW_OTHER_STATUS) {
        SYSTEM_ERR("param invalid, can not set bmc_sw_status: %d.\n", bmc_sw_status);
        return -EINVAL;
    }

    SYSTEM_DBG("bmc_sw_status: %d\n", bmc_sw_status);
    mutex_lock(&system_mutex);
    g_bmc_sw_status = bmc_sw_status;
    SYSTEM_DBG("set bmc_sw_status success\n");
    mutex_unlock(&system_mutex);
    return count;
}

static ssize_t my_slot_id_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_system_drv);
    check_p(g_system_drv->get_my_slot_id);

    return g_system_drv->get_my_slot_id(buf, PAGE_SIZE);
}

static ssize_t system_value_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    struct switch_device_attribute *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->get_system_value);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);
    SYSTEM_DBG("system_value_show type 0x%x \n", system_attr->type);
    return g_system_drv->get_system_value(system_attr->type, buf, PAGE_SIZE);
}

static ssize_t system_value_decode_string_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    struct switch_device_attribute *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->get_system_value_decode_string);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);
    SYSTEM_DBG("system_value_show type 0x%x \n", system_attr->type);
    return g_system_drv->get_system_value_decode_string(system_attr->type, buf, PAGE_SIZE);
}

static ssize_t system_value_store(struct switch_obj *obj, struct switch_attribute *attr,
                                  const char* buf, size_t count)
{
    int ret, value;
    struct switch_device_attribute *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->set_system_value);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        SYSTEM_ERR("system_value_store, input parameter: %s error. ret:%d\n", buf, ret);
        return ret;
    }

    if(value > 0xff) {
        SYSTEM_ERR("system_value_store, input parameter bigger than 0xff: %d\n", value);
        return -EINVAL;
    }
    SYSTEM_DBG("system_value_store, type: 0x%x. value=%d\n", system_attr->type, value);
    ret = g_system_drv->set_system_value(system_attr->type, value);
    if (ret < 0) {
        /* ret=-999 if not support */
        SYSTEM_ERR("set system reg value: %d failed. ret:%d\n", value, ret);
        return ret;
    }
    return count;
}

static ssize_t system_port_port_status_value(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    struct switch_device_attribute *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->get_system_port_power_status);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);
    SYSTEM_DBG("type 0x%x \n", system_attr->type);
    return g_system_drv->get_system_port_power_status(system_attr->type, buf, PAGE_SIZE);
}

static ssize_t system_bmc_view(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_system_drv);
    check_p(g_system_drv->get_bmc_view);

    return g_system_drv->get_bmc_view(buf, PAGE_SIZE);
}

static ssize_t system_bmc_switch(struct switch_obj *obj, struct switch_attribute *attr,
                                const char* buf, size_t count)
{
    int ret, value;

    check_p(g_system_drv);
    check_p(g_system_drv->set_bmc_switch);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        SYSTEM_ERR("system_bmc_switch, input parameter: %s error. ret:%d\n", buf, ret);
        return ret;
    }

    if(value > 0xff) {
        SYSTEM_ERR("system_bmc_switch, input parameter bigger than 0xff: %d\n", value);
        return -EINVAL;
    }
    SYSTEM_DBG("system_bmc_switch, value=%d\n", value);

    ret = g_system_drv->set_bmc_switch(buf, count);
    if (ret < 0) {
        /* ret=-999 if not support */
        SYSTEM_ERR("set system reg value: %d failed. ret:%d\n", value, ret);
        return ret;
    }
    return count;
}

static ssize_t system_bmc_dualboot_wdt_en_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_system_drv);
    check_p(g_system_drv->get_bmc_dualboot_wdt_status);

    return g_system_drv->get_bmc_dualboot_wdt_status(buf, PAGE_SIZE);
}

static ssize_t system_bmc_dualboot_wdt_en_store(struct switch_obj *obj, struct switch_attribute *attr,
                                const char* buf, size_t count)
{
    int ret, value;

    check_p(g_system_drv);
    check_p(g_system_drv->set_bmc_dualboot_wdt);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        SYSTEM_ERR("system_bmc_dualboot_wdt_en_store, input parameter: %s error. ret:%d\n", buf, ret);
        return ret;
    }

    if (value != 0) {
        SYSTEM_ERR("system_bmc_dualboot_wdt_en_store, invalid value: %d, write value must 0\n", value);
        return -EINVAL;
    }
    SYSTEM_DBG("system_bmc_dualboot_wdt_en_store, value=%d\n", value);

    ret = g_system_drv->set_bmc_dualboot_wdt(buf, count);
    if (ret < 0) {
        /* ret=-999 if not support */
        SYSTEM_ERR("system_bmc_dualboot_wdt_en_store failed. ret: %d\n", ret);
        return ret;
    }
    return count;
}

static ssize_t system_get_serial_number(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    check_p(g_system_drv);
    check_p(g_system_drv->get_system_serial_number);

    return g_system_drv->get_system_serial_number(buf, PAGE_SIZE);
}

static ssize_t system_status_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int val;

    mutex_lock(&system_status_mutex);
    val = system_status_value;
    mutex_unlock(&system_status_mutex);

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t system_status_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    int val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret != 0) {
        SYSTEM_ERR("invaild value ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (val < SYSTEM_STATUS_OK || val >= SYSTEM_STATUS_MAX) {
        SYSTEM_ERR("param invalid, can not set ratio: %d.\n", val);
        return -EINVAL;
    }

    mutex_lock(&system_status_mutex);
    system_status_value = val;
    SYSTEM_DBG("set system_status success. system_status_value = %d\n", system_status_value);
    mutex_unlock(&system_status_mutex);

    return count;
}

static ssize_t overtemp_reboot_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", overtemp_reboot_value);
}

static ssize_t overtemp_reboot_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;
    int val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret != 0) {
        SYSTEM_ERR("invaild overtemp_reboot ret: %d, buf: %s.\n", ret, buf);
        return -EINVAL;
    }

    if (val != SYSTEM_OVERTEMP_REBOOT_DEFAULT && val != SYSTEM_OVERTEMP_REBOOT_ENABLE && val != SYSTEM_OVERTEMP_REBOOT_DISABLE) {
        SYSTEM_ERR("param invalid, can not set overtemp_reboot: %d.\n", val);
        return -EINVAL;
    }

    overtemp_reboot_value = val;
    SYSTEM_DBG("set overtemp_reboot success. overtemp_reboot_value = %d\n", overtemp_reboot_value);

    return count;
}

static ssize_t mem_isolation_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    struct switch_device_attribute *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->get_mem_isolation_value);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);
    SYSTEM_DBG("mem_isolation_show type 0x%x \n", system_attr->type);
    return g_system_drv->get_mem_isolation_value(system_attr->type, buf, PAGE_SIZE);
}

static ssize_t mem_isolation_store(struct switch_obj *obj, struct switch_attribute *attr,
                                  const char* buf, size_t count)
{
    int ret, value;
    struct switch_device_attribute *system_attr;

    check_p(g_system_drv);
    check_p(g_system_drv->set_mem_isolation_value);

    system_attr = to_switch_device_attr(attr);
    check_p(system_attr);

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        SYSTEM_ERR("mem_isolation_store, input parameter: %s error. ret:%d\n", buf, ret);
        return ret;
    }

    if(value != MEM_ISOLATION_ENABLE && value != MEM_ISOLATION_DISABLE) {
        SYSTEM_ERR("mem_isolation_store, input parameter invalid: %d\n", value);
        return -EINVAL;
    }
    SYSTEM_DBG("mem_isolation_store, type: 0x%x. value=%d\n", system_attr->type, value);
    ret = g_system_drv->set_mem_isolation_value(system_attr->type, value);
    if (ret < 0) {
        /* ret=-999 if not support */
        SYSTEM_ERR("set system reg value: %d failed. ret:%d\n", value, ret);
        return ret;
    }
    return count;
}

/************************************system dir and attrs*******************************************/
static SWITCH_DEVICE_ATTR(bmc_ready, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_BMC_READY);
static SWITCH_DEVICE_ATTR(sol_active, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_SOL_ACTIVE);
static SWITCH_DEVICE_ATTR(psu_reset, S_IWUSR, NULL, system_value_store, WB_SYSTEM_PSU_RESET);
static SWITCH_DEVICE_ATTR(cpu_board_ctrl, S_IWUSR, NULL, system_value_store, WB_SYSTEM_CPU_BOARD_CTRL);
static SWITCH_DEVICE_ATTR(cpu_board_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_CPU_BOARD_STATUS);
static SWITCH_DEVICE_ATTR(ipmi_powercycle_mode, S_IRUGO , system_value_show, NULL, WB_SYSTEM_POWER_CYCLE_MODE);
static SWITCH_DEVICE_ATTR(bios_switch, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_BIOS_SWITCH);
static SWITCH_DEVICE_ATTR(bios_flash_switch, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_BIOS_FLASH_SWITCH);
static SWITCH_DEVICE_ATTR(bios_view, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BIOS_VIEW);
static SWITCH_DEVICE_ATTR(bios_boot_ok, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BIOS_BOOT_OK);
static SWITCH_DEVICE_ATTR(bios_fail_record, S_IRUGO, system_value_show, NULL, WB_SYSTEM_BIOS_FAIL_RECORD);
static SWITCH_DEVICE_ATTR(bios_debug_mode, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_BIOS_DEBUG_MODE);
static SWITCH_DEVICE_ATTR(bmc_reset, S_IWUSR, NULL, system_value_store, WB_SYSTEM_BMC_RESET);
static SWITCH_DEVICE_ATTR(mac_board_reset, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_MAC_BOARD_RESET);
static SWITCH_DEVICE_ATTR(mac_pwr_ctrl, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_MAC_PWR_CTRL);
static SWITCH_DEVICE_ATTR(emmc_pwr_ctrl, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_EMMC_PWR_CTRL);
static SWITCH_DEVICE_ATTR(port_pwr_ctl, S_IRUGO | S_IWUSR, system_port_port_status_value, system_value_store, WB_SYSTEM_PORT_PWR_CTL);
static SWITCH_DEVICE_ATTR(port_pwr_ctrl, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_PORT_PWR_CTL);
static SWITCH_DEVICE_ATTR(bmc_view, S_IRUGO, system_bmc_view, NULL, WB_SYSTEM_BMC_VIEW);
static SWITCH_DEVICE_ATTR(bmc_switch, S_IWUSR, NULL, system_bmc_switch, WB_SYSTEM_BMC_SWITCH);
static SWITCH_DEVICE_ATTR(bmc_dualboot_wdt_en, S_IRUGO |S_IWUSR, system_bmc_dualboot_wdt_en_show, system_bmc_dualboot_wdt_en_store, 0);
static SWITCH_DEVICE_ATTR(is_main_mgmt_board, S_IRUGO , system_value_show, NULL, WB_SYSTEM_IS_MAIN_MGMT_BOARD);
static SWITCH_DEVICE_ATTR(product_serial_number, S_IRUGO, system_get_serial_number, NULL, 0);
static SWITCH_DEVICE_ATTR(ssd_pwr_ctrl, S_IRUGO | S_IWUSR , system_value_show, system_value_store, WB_SYSTEM_SSD_PWR_CTRL);
static SWITCH_DEVICE_ATTR(ssd_pwr_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_SSD_PWR_STATUS);
static SWITCH_DEVICE_ATTR(mac_pwr_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_MAC_PWR_STATUS);
static SWITCH_DEVICE_ATTR(port_pwr_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_PORT_PWR_STATUS);
static SWITCH_DEVICE_ATTR(bmc_reset_cnt, S_IRUGO , system_value_show, NULL, WB_SYSTEM_BMC_RESET_CNT);
static SWITCH_DEVICE_ATTR(cpu_reset_cnt, S_IRUGO , system_value_show, NULL, WB_SYSTEM_CPU_RESET_CNT);
static SWITCH_DEVICE_ATTR(cpu_reset_type, S_IRUGO , system_value_show, NULL, WB_SYSTEM_CPU_RESET_TYPE);
static SWITCH_DEVICE_ATTR(panel_com_owner, S_IRUGO , system_value_show, NULL, WB_SYSTEM_PANEL_COM_OWNER);
static SWITCH_DEVICE_ATTR(panel_com_switch, S_IRUGO | S_IWUSR , system_value_show, system_value_store, WB_SYSTEM_PANEL_COM_SWITCH);
static SWITCH_DEVICE_ATTR(cpu_rtc_clear, S_IWUSR, NULL, system_value_store, WB_SYSTEM_CPU_RTC_CLEAR);
static SWITCH_DEVICE_ATTR(cpu_rtc_clear_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_CPU_RTC_CLEAR_STATUS);
static SWITCH_DEVICE_ATTR(bmc_reset_event_done, S_IWUSR, NULL, system_value_store, WB_SYSTEM_BMC_RESET_EVENT_DONE);
static SWITCH_DEVICE_ATTR(cpu_pwrup_status, S_IRUGO, system_value_show, NULL, WB_SYSTEM_CPU_PWRUP_STATUS);
static SWITCH_DEVICE_ATTR(cpu_pwrup_detail_status, S_IRUGO, system_value_decode_string_show, NULL, WB_SYSTEM_CPU_PWRUP_STATUS);
static SWITCH_DEVICE_ATTR(bmc_access_perm, S_IRUGO | S_IWUSR, system_value_show, system_value_store, WB_SYSTEM_BMC_ACCESS_PERM);
static SWITCH_DEVICE_ATTR(rmc_role, S_IRUGO , system_value_show, NULL, WB_SYSTEM_RMC_ROLE);
static SWITCH_DEVICE_ATTR(cpu_type, S_IRUGO , system_value_show, NULL, WB_SYSTEM_CPU_TYPE);
static SWITCH_DEVICE_ATTR(psu_off, S_IWUSR, NULL, system_value_store, WB_SYSTEM_PSU_OFF);
static SWITCH_DEVICE_ATTR(dev_rated_power, S_IRUGO , system_value_show, NULL, WB_SYSTEM_DEV_RATED_POWER);
static SWITCH_DEVICE_ATTR(mgmt_link_status, S_IRUGO , system_value_show, NULL, WB_SYSTEM_MGMT_LINK_STATUS);
static SWITCH_DEVICE_ATTR(mem_isolation, S_IRUGO | S_IWUSR, mem_isolation_show, mem_isolation_store, WB_SYSTEM_MEM_ISOLATION);

static struct switch_attribute slot_id_attr = __ATTR(slot_id, S_IRUGO, my_slot_id_show, NULL);
static struct switch_attribute bmc_status_attr = __ATTR(bmc_status, S_IRUGO, bmc_status_show, NULL);
static struct switch_attribute bmc_sw_status_attr = __ATTR(bmc_sw_status, S_IRUGO | S_IWUSR, bmc_sw_status_show, bmc_sw_status_store);
static struct switch_attribute system_status_attr = __ATTR(system_status, S_IRUGO | S_IWUSR, system_status_show, system_status_store);
static struct switch_attribute overtemp_reboot_attr = __ATTR(overtemp_reboot, S_IRUGO | S_IWUSR, overtemp_reboot_show, overtemp_reboot_store);

static struct attribute *system_dir_attrs[] = {
    &switch_dev_attr_bmc_ready.switch_attr.attr,
    &switch_dev_attr_sol_active.switch_attr.attr,
    &switch_dev_attr_psu_reset.switch_attr.attr,
    &switch_dev_attr_cpu_board_ctrl.switch_attr.attr,
    &switch_dev_attr_cpu_board_status.switch_attr.attr,
    &switch_dev_attr_ipmi_powercycle_mode.switch_attr.attr,
    &switch_dev_attr_bios_switch.switch_attr.attr,
    &switch_dev_attr_bios_flash_switch.switch_attr.attr,
    &switch_dev_attr_bios_view.switch_attr.attr,
    &switch_dev_attr_bios_boot_ok.switch_attr.attr,
    &switch_dev_attr_bios_fail_record.switch_attr.attr,
    &switch_dev_attr_bios_debug_mode.switch_attr.attr,
    &switch_dev_attr_bmc_reset.switch_attr.attr,
    &switch_dev_attr_mac_board_reset.switch_attr.attr,
    &switch_dev_attr_mac_pwr_ctrl.switch_attr.attr,
    &switch_dev_attr_emmc_pwr_ctrl.switch_attr.attr,
    &switch_dev_attr_port_pwr_ctl.switch_attr.attr,
    &switch_dev_attr_port_pwr_ctrl.switch_attr.attr,
    &switch_dev_attr_bmc_view.switch_attr.attr,
    &switch_dev_attr_bmc_switch.switch_attr.attr,
    &switch_dev_attr_bmc_dualboot_wdt_en.switch_attr.attr,
    &switch_dev_attr_is_main_mgmt_board.switch_attr.attr,
    &switch_dev_attr_product_serial_number.switch_attr.attr,
    &switch_dev_attr_ssd_pwr_ctrl.switch_attr.attr,
    &switch_dev_attr_ssd_pwr_status.switch_attr.attr,
    &switch_dev_attr_mac_pwr_status.switch_attr.attr,
    &switch_dev_attr_port_pwr_status.switch_attr.attr,
    &switch_dev_attr_bmc_reset_cnt.switch_attr.attr,
    &switch_dev_attr_cpu_reset_cnt.switch_attr.attr,
    &switch_dev_attr_cpu_reset_type.switch_attr.attr,
    &switch_dev_attr_panel_com_owner.switch_attr.attr,
    &switch_dev_attr_panel_com_switch.switch_attr.attr,
    &switch_dev_attr_cpu_rtc_clear.switch_attr.attr,
    &switch_dev_attr_cpu_rtc_clear_status.switch_attr.attr,
    &switch_dev_attr_bmc_reset_event_done.switch_attr.attr,
    &switch_dev_attr_cpu_pwrup_status.switch_attr.attr,
    &switch_dev_attr_cpu_pwrup_detail_status.switch_attr.attr,
    &switch_dev_attr_bmc_access_perm.switch_attr.attr,
    &switch_dev_attr_rmc_role.switch_attr.attr,
    &switch_dev_attr_cpu_type.switch_attr.attr,
    &switch_dev_attr_psu_off.switch_attr.attr,
    &switch_dev_attr_dev_rated_power.switch_attr.attr,  
    &switch_dev_attr_mgmt_link_status.switch_attr.attr,
    &switch_dev_attr_mem_isolation.switch_attr.attr,
    &slot_id_attr.attr,
    &system_status_attr.attr,
    &overtemp_reboot_attr.attr,
    &bmc_status_attr.attr,
    &bmc_sw_status_attr.attr,
    NULL,
};

static struct attribute_group system_root_attr_group = {
    .attrs = system_dir_attrs,
};

/* create system directory and number attributes */
static int system_root_create(void)
{
    g_system_obj = switch_kobject_create("system", NULL);
    if (!g_system_obj) {
        SYSTEM_ERR("switch_kobject_create system error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_system_obj->kobj, &system_root_attr_group) != 0) {
        switch_kobject_delete(&g_system_obj);
        SYSTEM_ERR("create system dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete system directory and number attributes */
static void system_root_remove(void)
{
    if (g_system_obj) {
        sysfs_remove_group(&g_system_obj->kobj, &system_root_attr_group);
        switch_kobject_delete(&g_system_obj);
    }

    return;
}

int s3ip_sysfs_system_drivers_register(struct s3ip_sysfs_system_drivers_s *drv)
{
    int ret;

    SYSTEM_INFO("s3ip_sysfs_system_drivers_register...\n");
    check_p(drv);
    g_system_drv = drv;
    ret = system_root_create();
    if (ret < 0) {
        SYSTEM_ERR("create system root dir and attrs failed, ret: %d\n", ret);
        return ret;
    }

    SYSTEM_INFO("s3ip_sysfs_system_drivers_register success\n");
    return ret;
}

void s3ip_sysfs_system_drivers_unregister(void)
{
    if (g_system_drv) {
        system_root_remove();
        g_system_drv = NULL;
        SYSTEM_DBG("s3ip_sysfs_system_drivers_unregister success.\n");
    }
    return;
}

EXPORT_SYMBOL(s3ip_sysfs_system_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_system_drivers_unregister);
module_param(g_system_loglevel, int, 0644);
MODULE_PARM_DESC(g_system_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");
