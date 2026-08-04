/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>

#include "../rg_dev_sysfs/include/rg_sysfs_common.h"
#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_fan_driver.h"
#include "./include/dfd_syseeprom_driver.h"
#include "./include/dfd_cpld_driver.h"
#include "./include/dfd_led_driver.h"
#include "./include/dfd_slot_driver.h"
#include "./include/dfd_sensors_driver.h"
#include "./include/dfd_psu_driver.h"
#include "./include/dfd_sff_driver.h"

typedef enum dfd_dev_init_fail_s {
    DFD_KO_INIT_CPLD_FAIL       = 1,
    DFD_KO_INIT_FPGA_FAIL       = 2,
    DFD_KO_INIT_IRQ_FAIL        = 3,
    DFD_KO_INIT_CFG_FAIL        = 4,
    DFD_KO_INIT_DATA_FAIL       = 5,
} dfd_dev_init_fail_t;

int g_dfd_dbg_level = 0;

char *g_status_mem_str[STATUS_MEM_END] = {
    "ABSENT",
    "OK",
    "NOT OK",
};

int dfd_get_dev_number(unsigned int main_dev_id, unsigned int minor_dev_id)
{
    int key,dev_num;
    int *p_dev_num;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_NUM, main_dev_id, minor_dev_id);
    p_dev_num = dfd_ko_cfg_get_item(key);
    if (p_dev_num == NULL) {
        DBG_DEBUG(DBG_ERROR, "get device number failed, key:0x%x\n",key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }
    dev_num = *p_dev_num;
    DBG_DEBUG(DBG_VERBOSE, "get device number ok, number:%d\n",dev_num);
    return dev_num;
}

static struct switch_drivers_t switch_drivers= {
    .get_dev_number = dfd_get_dev_number,
    /* fan */
    .get_fan_status = dfd_get_fan_status,
    .get_fan_info = dfd_get_fan_info,
    .get_fan_speed = dfd_get_fan_speed,
    .get_fan_pwm = dfd_get_fan_pwm,
    .set_fan_pwm = dfd_set_fan_pwm,
    .get_fan_speed_tolerance = dfd_get_fan_speed_tolerance,
    .get_fan_speed_target = dfd_get_fan_speed_target,
    .get_fan_direction = dfd_get_fan_direction,
    .get_fan_status_str = dfd_get_fan_status_str,
    .get_fan_direction_str = dfd_get_fan_direction_str,
    .get_fan_present_status = dfd_get_fan_present_status,
    .get_fan_roll_status = dfd_get_fan_roll_status,
    .get_fan_speed_level = dfd_get_fan_speed_level,
    .set_fan_speed_level = dfd_set_fan_speed_level,
    /* syseeprom */
    .get_syseeprom_info = dfd_get_syseeprom_info,
    /* cpld */
    .get_cpld_name = dfd_get_cpld_name,
    .get_cpld_type = dfd_get_cpld_type,
    .get_cpld_version = dfd_get_cpld_version,
    .get_cpld_testreg = dfd_get_cpld_testreg,
    .set_cpld_testreg = dfd_set_cpld_testreg,
    /* led */
    .get_led_status = dfd_get_led_status,
    /* slot */
    .get_slot_status_str = dfd_get_slot_status_str,
    .get_slot_info = dfd_get_slot_info,
    /* sensors */
    .get_temp_info = dfd_get_temp_info,
    .get_voltage_info = dfd_get_voltage_info,
    /* psu */
    .get_psu_info = dfd_get_psu_info,
    .get_psu_status_str = dfd_get_psu_status_str,
    .get_psu_sensor_info = dfd_get_psu_sensor_info,
    .get_psu_present_status = dfd_get_psu_present_status,
    .get_psu_output_status = dfd_get_psu_output_status,
    .get_psu_alert_status = dfd_get_psu_alert_status,
    /* sff */
    .get_sff_id = dfd_get_sff_id,
    .set_sff_cpld_info = dfd_set_sff_cpld_info,
    .get_sff_cpld_info = dfd_get_sff_cpld_info,
    .get_sff_eeprom_info = dfd_get_sff_eeprom_info,
    .get_sff_dir_name = dfd_get_sff_dir_name,
    .get_sff_polling_size = dfd_get_sff_polling_size,
    .get_sff_polling_data = dfd_get_sff_polling_data,
};

struct switch_drivers_t * dfd_plat_driver_get(void) {
    return &switch_drivers;
}

static int32_t __init dfd_dev_init(void)
{
    int ret;

    DBG_DEBUG(DBG_VERBOSE, "Enter.\n");

    ret = dfd_dev_cfg_init();
    if (ret != 0) {
        DBG_DEBUG(DBG_ERROR, "dfd_dev_cfg_init failed ret %d.\n", ret);
        ret = -DFD_KO_INIT_CFG_FAIL;
        return ret;
    }

    DBG_DEBUG(DBG_VERBOSE, "success.\n");
    return 0;
}

static void __exit dfd_dev_exit(void)
{
    DBG_DEBUG(DBG_VERBOSE, "dfd_dev_exit.\n");
    dfd_dev_cfg_exit();
    return ;
}

module_init(dfd_dev_init);
module_exit(dfd_dev_exit);
module_param(g_dfd_dbg_level, int, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL(dfd_plat_driver_get);
MODULE_AUTHOR("sonic_rd <sonic_rd@ruijie.com.cn>");
MODULE_LICENSE("GPL");
