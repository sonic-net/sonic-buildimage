/*
 * switch_driver.c
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */
#include <linux/module.h>

#include "dfd_sysfs_common.h"
#include "switch_driver.h"
#include "wb_module.h"
#include "wb_fan_driver.h"
#include "wb_cabletray_driver.h"
#include "wb_eeprom_driver.h"
#include "wb_cpld_driver.h"
#include "wb_fpga_driver.h"
#include "wb_led_driver.h"
#include "wb_slot_driver.h"
#include "wb_sensors_driver.h"
#include "wb_psu_driver.h"
#include "wb_sff_driver.h"
#include "wb_watchdog_driver.h"
#include "wb_system_driver.h"
#include "wb_leak_detector_driver.h"
#include "wb_sol_driver.h"
#include "wb_misc_fw_driver.h"
#include "wb_gcu_driver.h"
#include "wb_nvme_driver.h"
#include "wb_debug_driver.h"
#include "wb_lsw_driver.h"
#include "wb_pcie_driver.h"
#include "wb_clock_driver.h"
#include "wb_pll_driver.h"
#include "wb_avs_driver.h"
#include "wb_dpu_driver.h"
#include "dfd_cfg.h"
#include "wb_disk_driver.h"
#include "wb_md_driver.h"

int g_switch_dbg_level = 0;

/***************************************main board temp*****************************************/
/*
 * dfd_get_main_board_temp_number - Used to get main board temperature sensors number,
 *
 * This function returns main board temperature sensors by your switch,
 * If there is no main board temperature sensors, returns 0,
 * otherwise it returns a negative value on failed.
 */
static int dfd_get_main_board_temp_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_TEMP);
    return ret;
}

/*
 * dfd_get_temp_attr - Used to get the attr of temperature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @temp_index: start with 1
 * @type: threshold type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_temp_attr(unsigned int temp_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, type,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_main_board_debug_temp_attr - Used to set main board temp debug data
 * @temp_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_main_board_debug_temp_attr(unsigned int type, unsigned int temp_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, DFD_DEBUG_TEMP, type, temp_index, value);
}

/*
 * dfd_get_main_board_temp_monitor_flag - Used to get monitor flag of temperature sensor
 * filled the value to buf, the value is integer
 * @index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_monitor_flag(unsigned int index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_main_board_monitor_flag(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, WB_MINOR_DEV_TEMP, index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_temp_status - Used to get the status of sensor
 * filled the value to buf, the value is hex with 0x0/0x1/0x2/0xff
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_status(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_status(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}
/***********************************end of main board temp*************************************/

/*************************************main board voltage***************************************/
static int dfd_get_main_board_vol_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_IN);
    return ret;
}

/*
 * dfd_get_main_board_vol_alias - Used to identify the location of the voltage sensor,
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_alias(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_ALIAS,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_vol_type - Used to get the model of voltage sensor,
 * such as udc90160, tps53622 and so on
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_type(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_TYPE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_vol_threshold - Used to get the threshold of volerature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @vol_index: start with 1
 * @type: threshold type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_threshold(unsigned int vol_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, type,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_vol_range - Used to get the output error value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_range(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index,
              WB_SENSOR_RANGE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_vol_nominal_value - Used to get the nominal value of voltage sensor
 * filled the value to buf
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_nominal_value(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index,
              WB_SENSOR_NOMINAL_VAL, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_vol_value - Used to get the input value of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_value(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_INPUT,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_vol_monitor_flag - Used to get monitor flag of voltage sensor
 * filled the value to buf, the value is integer
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_monitor_flag(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_main_board_monitor_flag(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, WB_MINOR_DEV_IN, temp_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_main_board_debug_vol_attr - Used to set main board vol debug data
 * @vol_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_main_board_debug_vol_attr(unsigned int type, unsigned int vol_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, DFD_DEBUG_VOL, type, vol_index, value);
}



/*
 * dfd_get_main_board_vol_status - Used to get the status of sensor
 * filled the value to buf, the value is integer with 0 | 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_status(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_vol_status(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*********************************end of main board voltage************************************/
/*************************************main board current***************************************/
static int dfd_get_main_board_curr_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_CURR);
    return ret;
}

/*
 * dfd_get_main_board_curr_alias - Used to identify the location of the current sensor,
 * @curr_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_main_board_debug_curr_attr(unsigned int type, unsigned int curr_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, DFD_DEBUG_CURR, type, curr_index, value);
}

/*
 * dfd_get_main_board_curr_alias - Used to identify the location of the current sensor,
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_alias(unsigned int curr_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, curr_index, WB_SENSOR_ALIAS,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_curr_type - Used to get the model of current sensor,
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_type(unsigned int curr_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, curr_index, WB_SENSOR_TYPE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_curr_threshold - Used to get the threshold of currerature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @curr_index: start with 1
 * @type: threshold type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_threshold(unsigned int curr_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, curr_index, type,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_curr_value - Used to get the input value of current sensor
 * filled the value to buf, the value is integer with mA
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_value(unsigned int curr_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, curr_index, WB_SENSOR_INPUT,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_curr_monitor_flag - Used to get monitor flag of current sensor
 * filled the value to buf, the value is integer
 * @index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_monitor_flag(unsigned int index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_main_board_monitor_flag(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, WB_MINOR_DEV_CURR, index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}
/*********************************end of main board current************************************/

/*************************************main board power***************************************/
static int dfd_get_main_board_power_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_POWER);
    return ret;
}

/*
 * dfd_get_main_board_power_alias - Used to identify the location of the power sensor,
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_power_alias(unsigned int power_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_power_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, power_index, WB_SENSOR_ALIAS,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_power_type - Used to get the model of power sensor,
 * such as udc90160, tps53622 and so on
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_power_type(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_power_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_TYPE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_power_max - Used to get the maximum threshold of power sensor
 * filled the value to buf, the value is integer with uW
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_power_max(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_power_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_MAX,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_power_min - Used to get the minimum threshold of power sensor
 * filled the value to buf, the value is integer with uW
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_power_min(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_power_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_MIN,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_power_value - Used to get the input value of power sensor
 * filled the value to buf, the value is integer with uW
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_power_value(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_power_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_INPUT,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_main_board_power_monitor_flag - Used to get monitor flag of power sensor
 * filled the value to buf, the value is integer
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_power_monitor_flag(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_main_board_monitor_flag(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, WB_MINOR_DEV_POWER, temp_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}
/*********************************end of main board voltage************************************/


/*****************************************syseeprom*******************************************/
/*
 * dfd_get_syseeprom_size - Used to get syseeprom size
 *
 * This function returns the size of syseeprom by your switch,
 * otherwise it returns a negative value on failed.
 */
static int dfd_get_syseeprom_size(void)
{
    int ret;

    ret = dfd_get_eeprom_size(WB_MAIN_DEV_MAINBOARD, 0);
    return ret;
}

/*
 * dfd_read_syseeprom_data - Used to read syseeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read syseeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_read_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_MAINBOARD, 0, buf, offset, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            /* No such device or address */
            return -ENXIO;
        }
        /* I/O error */
        return -EIO;
    }
    return ret;
}

/*
 * dfd_write_syseeprom_data - Used to write syseeprom data
 * @buf: Data write buffer
 * @offset: offset address to write syseeprom data
 * @count: length of buf
 *
 * This function returns the written length of syseeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_write_syseeprom_data(char *buf, loff_t offset, size_t count)
{
    ssize_t ret;

    ret = dfd_write_eeprom_data(WB_MAIN_DEV_MAINBOARD, 0, buf, offset, count);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}
/*************************************end of syseeprom****************************************/

/*****************************************s3ip_debug*******************************************/
/*
 * dfd_reload_s3ip_config - Used to reload s3ip config
 *
 */
static int dfd_reload_s3ip_config(void)
{
    int ret;

    ret = dfd_dev_cfg_init(false);
    return ret;
}


/*************************************end of s3ip_debug****************************************/


/*****************************************debug*******************************************/
/*
 * dfd_set_debug_and_reset - Used to reset debug threshold
 *
 */
static int dfd_set_debug_and_reset(unsigned int mode)
{
    int ret;

    ret = dfd_dev_set_debug_and_reset(mode);
    return ret;
}

/*************************************end of debug****************************************/

/********************************************fan**********************************************/
static int dfd_get_fan_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_FAN, WB_MINOR_DEV_NONE);
    return ret;
}

/*
 * dfd_get_fan_status - Used to get fan status,
 * filled the value to buf, fan status define see enum status_e
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_status(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fan_status_str(fan_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_present - Used to get fan present status,
 * filled the value to buf, fan status define see enum status_e
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_present(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fan_present_str(fan_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static int dfd_get_fan_motor_number(unsigned int fan_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_FAN, WB_MINOR_DEV_MOTOR);
    return ret;
}

/*
 * dfd_get_fan_model_name - Used to get fan model name,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_model_name(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_NAME, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_vendor - Used to get fan vendor,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_vendor(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_VENDOR, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_serial_number - Used to get fan serial number,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_serial_number(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_SN, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_part_number - Used to get fan part number,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_part_number(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_hardware_version - Used to get fan hardware version,
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_hardware_version(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_HW_INFO, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_led_status - Used to get fan led status
 * filled the value to buf, led status value define see enum fan_status_e
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_led_status(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_led_status(WB_FAN_LED_MODULE, fan_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_fan_led_status - Used to set fan led status
 * @fan_index: start with 1
 * @status: led status, led status value define see enum led_status_e
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_fan_led_status(unsigned int fan_index, int status)
{
    int ret;

    ret = dfd_set_led_status(WB_FAN_LED_MODULE, fan_index, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_get_fan_direction - Used to get fan air flow direction,
 * filled the value to buf, air flow direction define see enum air_flow_direction_e
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_direction(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf,  count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_direction_str(fan_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_motor_status - Used to get fan motor status
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_motor_status(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fan_motor_status_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_motor_speed - Used to get fan motor speed
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_motor_speed(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fan_speed_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_motor_speed_tolerance - Used to get fan motor speed tolerance
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_motor_speed_tolerance(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_motor_speed_tolerance_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_motor_speed_target - Used to get fan motor speed target
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_motor_speed_target(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_motor_speed_target_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_motor_speed_max - Used to get the maximum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_motor_speed_max(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_motor_speed_max_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_fan_motor_speed_min - Used to get the minimum threshold of fan motor
 * filled the value to buf
 * @fan_index: start with 1
 * @motor_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_motor_speed_min(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_fan_motor_speed_min_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_fan_motor_speed_threshold_cnt(unsigned int fan_index, unsigned int motor_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fan_motor_speed_threshold_cnt_str(fan_index, motor_index, buf, count);
    if (ret < 0) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%d\n", SENSOR_THRESHOLD_CNT_DEFALUT);
    }
    return ret;
}

/*
 * dfd_get_fan_ratio - Used to get the ratio of fan
 * filled the value to buf
 * @fan_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fan_ratio(unsigned int fan_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fan_pwm_str(fan_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_fan_ratio - Used to set the ratio of fan
 * @fan_index: start with 1
 * @ratio: motor speed ratio, from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_fan_ratio(unsigned int fan_index, int ratio)
{
    int ret;

    /* add vendor codes here */
    ret = dfd_set_fan_pwm(fan_index, ratio);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

static ssize_t dfd_get_fan_attr(unsigned int fan_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func fan_get_func;

    fan_get_func = dfd_get_sysfs_value_func(fan_func_table, type, ARRAY_SIZE(fan_func_table));
    if (fan_get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = fan_get_func(WB_MAIN_DEV_MAINBOARD, fan_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_fan_attr - Used to set fan register
 * @fan_index: start with 1
 * @type: write fan attr
 * @value: value write to fan
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_fan_attr(unsigned int fan_index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func fan_set_func;

    fan_set_func = dfd_set_sysfs_value_func(fan_func_table, type, ARRAY_SIZE(fan_func_table));
    if (fan_set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = fan_set_func(WB_MAIN_DEV_MAINBOARD, fan_index, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/*
 * dfd_get_psu_eeprom_size - Used to get fan eeprom size
 *
 * This function returns the size of fan eeprom,
 * otherwise it returns a negative value on failed.
 */
static int dfd_get_fan_eeprom_size(unsigned int index)
{
    int ret;

    ret = dfd_get_eeprom_size(WB_MAIN_DEV_FAN, index);
    return ret;
}

/*
 * dfd_read_fan_eeprom_data - Used to read eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_read_fan_eeprom_data(unsigned int fan_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_FAN, fan_index, buf, offset, count);
    return ret;
}

/*
 * dfd_write_fan_eeprom_data - Used to write eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write eeprom data
 * @count: length of buf
 *
 * This function returns the written length of eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_write_fan_eeprom_data(unsigned int fan_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_fan_present_status(fan_index);
    if (status == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    ret = dfd_write_eeprom_data(WB_MAIN_DEV_FAN, fan_index, buf, offset, count);
    return ret;
}

/*
 * dfd_set_fan_dev_debug_fan_attr - Used to set main board fan debug data
* @type: present,alias
 * @fan_index: start with 1
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_fan_dev_debug_fan_attr(unsigned int type, unsigned int fan_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_FAN, WB_MINOR_DEV_NONE, DFD_DEBUG_FAN, type, fan_index, value);
}

/*
 * dfd_get_fanctrl_duration_max
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_fanctrl_duration_max(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fanctrl_duration_max_str(buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}
/****************************************end of fan*******************************************/
/********************************************psu**********************************************/
static int dfd_get_psu_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_PSU, WB_MINOR_DEV_NONE);
    return ret;
}

/*
 * dfd_get_psu_present - Used to get psu present
 * filled the value to buf, psu present status define see enum psu_status_e
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_present(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_present_status_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static int dfd_get_psu_temp_number(unsigned int psu_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_PSU, WB_MINOR_DEV_TEMP);
    return ret;
}

static ssize_t dfd_get_psu_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf,  count);
        status = 0x01;
        return (ssize_t)snprintf(buf, count, "0x%x\n", status);
    }

    ret = dfd_get_psu_status_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_alarm(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_alarm_status(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_psu_type - Used to get the input type of psu
 * filled the value to buf, input type value define see enum psu_input_type_e
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_type(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_input_type(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_psu_sensor_attr - Used to get the sensor attr of psu
 * filled the value to buf, the value is integer with mA
 * @psu_index: start with 1
 * @type: attr type
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_sensor_attr(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_psu_in_status - Used to get psu input status
 * filled the value to buf, psu input status define see enum psu_io_status_e
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_in_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_in_status_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_psu_out_status - Used to get psu output status
 * filled the value to buf, psu output status define see enum psu_io_status_e
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_out_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf,  count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_out_status_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_hw_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_hw_status_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_hw_detail_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_hw_detail_status_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_attr_threshold(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_threshold_str(psu_index, type, buf, count);
    if (ret < 0) {
        if (type >= PSU_IN_VOL_THRESHOLD_CNT && type <= PSU_FAN_SPEED_THRESHOLD_CNT) {
            return (ssize_t)snprintf(buf, count, "%d\n", SENSOR_THRESHOLD_CNT_DEFALUT);
        }

        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_status_pmbus(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_status_pmbus_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_psu_fan_ratio - Used to get the ratio of psu fan
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_fan_ratio(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_fan_ratio_str(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_psu_fan_ratio - Used to set the ratio of psu fan
 * @psu_index: start with 1
 * @ratio: from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_psu_fan_ratio(unsigned int psu_index, int ratio)
{
    /* add vendor codes here */
    return -WB_SYSFS_RV_UNSUPPORT;
}

/* Similar to dfd_get_fan_led_status */
static ssize_t dfd_get_psu_led_status(unsigned int psu_index, char *buf, size_t count)
{
#if 0
    ssize_t ret;
    int status_word;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        status = LED_STATUS_DARK; /* led off */
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
    status = LED_STATUS_GREEN;

    status_word = 0;
    ret = dfd_get_psu_pmbus_val(psu_index, &status_word, PMBUS_STATUS_WORD_SYSFS);
    if (ret < 0) {
        SWITCH_DEBUG(DBG_ERROR, "get psu pmbus status error, ret: %zd, psu_index: %u\n", ret, psu_index);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    mem_clear(buf, count);
    if (status_word > 0) {
        status = LED_STATUS_YELLOW; /* led amber */
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
    return (ssize_t)snprintf(buf, count, "%d\n", status); /* led green */
#else
    return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
#endif
}

static ssize_t dfd_get_psu_attr(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    if (type != DFD_DEV_INFO_I2C_BUS && type != DFD_DEV_INFO_I2C_ADDR &&
        type != DFD_DEV_INFO_I2C_PMBUS_BUS && type != DFD_DEV_INFO_I2C_PMBUS_ADDR) {
        status = dfd_get_psu_present_status(psu_index);
        if (status == DEV_ABSENT) {
            mem_clear(buf, count);
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        }
    }

    ret = dfd_get_psu_info(psu_index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_max */
static ssize_t dfd_get_psu_temp_attr(unsigned int psu_index, unsigned int temp_index,
                    unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    if (type == WB_SENSOR_THRESHOLD_CNT) {
        ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, type, buf, count);
        if (ret < 0) {
            return (ssize_t)snprintf(buf, count, "%d\n", SENSOR_THRESHOLD_CNT_DEFALUT);
        }
        return ret;
    }

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, type,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_psu_eeprom_size - Used to get psu eeprom size
 *
 * This function returns the size of psu eeprom,
 * otherwise it returns a negative value on failed.
 */
static int dfd_get_psu_eeprom_size(unsigned int psu_index)
{
    int ret;

    ret = dfd_get_eeprom_size(WB_MAIN_DEV_PSU, psu_index);
    return ret;
}

/*
 * dfd_read_psu_eeprom_data - Used to read psu eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read psu eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_read_psu_eeprom_data(unsigned int psu_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_PSU, psu_index, buf, offset, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            /* No such device or address */
            return -ENXIO;
        }
        /* I/O error */
        return -EIO;
    }
    return ret;
}

static ssize_t dfd_get_psu_blackbox_path(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_blackbox(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_pmbus_info(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf,  count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_pmbus(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_clear_psu_blackbox_info - Used to clear psu blackbox information
 * @psu_index: start with 1
 * @value: 1
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_clear_psu_blackbox_info(unsigned int psu_index, uint8_t value)
{
    int ret;

    ret = dfd_clear_psu_blackbox(psu_index, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;

}

static ssize_t dfd_get_psu_support_upgrade(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_support_upgrade_func(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_upgrade_active_type(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_upgrade_active_type_func(psu_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_clear_psu_blackbox_info - Used to clear psu blackbox information
 * @psu_index: start with 1
 * @value: 1
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_psu_reset(unsigned int psu_index, uint8_t value)
{
    int ret;

    ret = dfd_set_psu_reset_func(psu_index, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/*
 * dfd_set_psu_off - Used to set psu off
 * @psu_index: start with 1
 * @value: 1
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_psu_off(unsigned int psu_index, uint8_t value)
{
    int ret;

    ret = dfd_set_psu_off_func(psu_index, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/*
 * dfd_write_psu_eeprom_data - Used to write eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write eeprom data
 * @count: length of buf
 *
 * This function returns the written length of eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_write_psu_eeprom_data(unsigned int psu_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    ret = dfd_write_eeprom_data(WB_MAIN_DEV_PSU, psu_index, buf, offset, count);
    return ret;
}

/*
 * dfd_set_main_board_debug_psu_sensor_value_attr - Used to set main board psu threshold debug data
 * @psu_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_main_board_debug_psu_sensor_value_attr(unsigned int type, unsigned int psu_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, DFD_DEBUG_PSU_SENSOR, type, psu_index, value);
}

/*
 * dfd_set_main_board_debug_psu_status_attr - Used to set main board psu debug data
 * @psu_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_main_board_debug_psu_status_attr(unsigned int type, unsigned int psu_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, DFD_DEBUG_PSU, type, psu_index, value);
}

/****************************************end of psu*******************************************/
/****************************************transceiver******************************************/
static int dfd_get_eth_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SFF, WB_MINOR_DEV_NONE);
    return ret;
}

/*
 * dfd_get_transceiver_power_on_status - Used to get the whole machine port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_transceiver_power_on_status(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_transceiver_power_on_status_str(buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_transceiver_power_on_status - Used to set the whole machine port power on status,
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_transceiver_power_on_status(int status)
{
    int ret;

    ret = dfd_set_sff_cpld_info(0, WB_SFF_POWER_ON, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_get_transceiver_present_status - Used to get the whole machine port present status,
 * filled the value to buf, 0: absent, 1: present
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_transceiver_present_status(char *buf, size_t count)
{
    ssize_t ret;
    unsigned int eth_index, eth_num;
    int len, left_len;
    int rv;

    rv = dfd_get_dev_number(WB_MAIN_DEV_SFF, WB_MINOR_DEV_NONE);
    if (rv <= 0) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }
    eth_num = rv;

    mem_clear(buf, count);
    len = 0;
    left_len = count - 1;

    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        SWITCH_DEBUG(DBG_VERBOSE, "eth index: %u\n", eth_index);
        if (left_len > 0) {
            ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_MODULE_PRESENT, buf + len, left_len);
            if (ret < 0) {
                SWITCH_DEBUG(DBG_ERROR, "get eth%u present status failed, ret: %zd\n", eth_index, ret);
                break;
            }
        } else {
            SWITCH_DEBUG(DBG_ERROR, "error: get_transceiver_present_status are not enough buffers.\n");
            ret = -DFD_RV_NO_MEMORY;
            break;
        }
        dfd_ko_cfg_del_lf_cr(buf);  /* del '\n' */
        len = strlen(buf);
        left_len = count - len - 2; /* Reserve end to add '\n' and '\0' */
    }

    if (ret < 0) {
        mem_clear(buf, count);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    len = strlen(buf);
    if (len >= count) {
        SWITCH_DEBUG(DBG_ERROR, "error: get_transceiver_present_status buffers too long, need: %zu, act: %d.\n", count, len);
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }
    buf[len] = '\n';
    ret = strlen(buf);
    SWITCH_DEBUG(DBG_VERBOSE, "get_transceiver_present_status ok. sff num:%d, len:%zd\n", eth_num, ret);

    return ret;
}

/*
 * dfd_get_eth_i2c_bus - Used to get eth i2c bus,
 * filled the value to buf
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_i2c_bus(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_single_eth_i2c_bus(eth_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_eth_cage_type - Used to get eth cage_type,
 * filled the value to buf
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_cage_type(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_single_eth_cage_type(eth_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_eth_power_on_status - Used to get single port power on status,
 * filled the value to buf, 0: power off, 1: power on
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_power_on_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;
    int power_group;

    ret = dfd_get_single_eth_power_group(eth_index, &power_group);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        /* If the port does not support power group, get the individual port status */
        ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_POWER_ON, buf, count);
        if (ret < 0) {
            if (ret == -DFD_RV_DEV_NOTSUPPORT) {
                return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
            } else {
                return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
            }
        }
        return ret;
    }

    ret = dfd_get_sff_power_group_state(power_group, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_eth_e2_low_page - Used to get eth e2 low page info
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_e2_low_page(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_e2_page_info(eth_index, WB_SFF_E2_PAGE_LOW, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/*
 * dfd_get_eth_temp - Used to get eth temp
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_temp(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_sff_present_status(eth_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_sff_temp(eth_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}
/*
 * dfd_set_eth_power_on_status - Used to set single port power on status,
 * @eth_index: start with 1
 * @status: power on status, 0: power off, 1: power on
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_eth_power_on_status(unsigned int eth_index, int status)
{
    int ret, power_group;

    ret = dfd_get_single_eth_power_group(eth_index, &power_group);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        /* If the port does not support power group, set the individual port status */
        SWITCH_DEBUG(DBG_VERBOSE, "set the individual port status. eth:%d, status:%d\n", eth_index, status);
        ret = dfd_set_sff_cpld_info(eth_index, WB_SFF_POWER_ON, status);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return -WB_SYSFS_RV_UNSUPPORT;
        }
        return ret;
    }

    SWITCH_DEBUG(DBG_VERBOSE, "set the group status. power group:%d, status:%d\n", power_group, status);
    ret = dfd_set_sff_power_group_state(power_group, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/*
 * dfd_get_eth_present_status - Used to get port present status,
 * filled the value to buf, 1: present, 0: absent
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_present_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_MODULE_PRESENT, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_eth_attr - Used to get port attr,
 * filled the value to buf
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_attr(unsigned int eth_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int status;
    dfd_sysfs_get_data_func get_func;

    status = dfd_get_sff_present_status(eth_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(sff_func_table, type, ARRAY_SIZE(sff_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(eth_index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_eth_attr - Used to set port attr,
 * @eth_index: start with 1
 * @status: write status
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_eth_attr(unsigned int eth_index, unsigned int type, int status)
{
    int ret;
    dfd_sysfs_set_data_func set_func;

    set_func = dfd_set_sysfs_value_func(sff_func_table, type, ARRAY_SIZE(sff_func_table));
    if (set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = set_func(eth_index, type, &status, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/**
 * dfd_get_eth_optoe_type - get sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
static ssize_t dfd_get_eth_optoe_type(unsigned int eth_index, int *optoe_type, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_single_eth_optoe_type(eth_index, optoe_type);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return (ssize_t)snprintf(buf, count, "%d\n", *optoe_type);
}

/**
 * dfd_set_eth_optoe_type - set sff optoe type
 * @sff_index: Optical module number, starting from 1
 * @optoe_type: Optical module type
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
static int dfd_set_eth_optoe_type(unsigned int eth_index, int optoe_type)
{
    int ret;

    ret = dfd_set_single_eth_optoe_type(eth_index, optoe_type);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/**
 * dfd_get_eth_power_group - get sff power group
 * @sff_index: Optical module number, starting from 1
 * @power_group: power group
 * return: Success: Returns the length of fill buf
 *       : Failed: A negative value is returned
 */
static ssize_t dfd_get_eth_power_group(unsigned int eth_index, int *power_group)
{
    ssize_t ret;

    ret = dfd_get_single_eth_power_group(eth_index, power_group);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_set_eth_port_led_status - Used to set the port led status,
 * @status: power on status
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_eth_port_led_status(unsigned int eth_index, int status)
{
    int ret;

    ret = dfd_set_led_status(WB_PORT_LED, eth_index, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}


/*
 * dfd_get_eth_port_led_status - Used to get the port led status,
 * filled the value to buf,
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_port_led_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_led_status(WB_PORT_LED, eth_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_eth_port_bus_status - Used to get the port bus status,
 * filled the value to buf,
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_port_bus_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_single_eth_port_bus_status(eth_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_eth_eeprom_size - Used to get port eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int dfd_get_eth_eeprom_size(unsigned int eth_index)
{
    int ret;

    ret = dfd_get_eeprom_size(WB_MAIN_DEV_SFF, eth_index);
    return ret;
}

/*
 * dfd_read_eth_eeprom_data - Used to read port eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read port eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_read_eth_eeprom_data(unsigned int eth_index, char *buf, loff_t offset,
                   size_t count, int upgrade_flag)
{
    ssize_t ret;
    int status;

    status = dfd_get_sff_present_status(eth_index);
    if (status == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    if (!upgrade_flag) {
        ret = dfd_read_eeprom_data(WB_MAIN_DEV_SFF, eth_index, buf, offset, count);
    } else {
        ret = dfd_read_eeprom_upg_data(WB_MAIN_DEV_SFF, eth_index, buf, offset, count);
    }
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            /* No such device or address */
            return -ENXIO;
        }
        /* I/O error */
        return -EIO;
    }
    return ret;
}

/*
 * dfd_write_eth_eeprom_data - Used to write port eeprom data
 * @buf: Data write buffer
 * @offset: offset address to write port eeprom data
 * @count: length of buf
 *
 * This function returns the written length of port eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_write_eth_eeprom_data(unsigned int eth_index, char *buf, loff_t offset,
                   size_t count, int upgrade_flag)
{
    ssize_t ret;
    int status;

    status = dfd_get_sff_present_status(eth_index);
    if (status == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    if (!upgrade_flag) {
        ret = dfd_write_eeprom_data(WB_MAIN_DEV_SFF, eth_index, buf, offset, count);
    } else {
        ret = dfd_write_eeprom_upg_data(WB_MAIN_DEV_SFF, eth_index, buf, offset, count);
    }

    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return -WB_SYSFS_RV_UNSUPPORT;
        } else {
            return -EIO;
        }
    }

    return ret;
}

static ssize_t dfd_get_eth_dev_available(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_sff_present_status(eth_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_read_eeprom_dev_available(WB_MAIN_DEV_SFF, eth_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static int dfd_set_eth_dev_available(unsigned int eth_index, int status)
{
    int ret;

    ret = dfd_get_sff_present_status(eth_index);
    if (ret == DEV_ABSENT) {
        return -WB_SYSFS_RV_ABSENT;
    }

    ret = dfd_write_eeprom_dev_available(WB_MAIN_DEV_SFF, eth_index, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_set_main_board_debug_sff_attr - Used to set main board sff debug data
 * @sff_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_main_board_debug_sff_attr(unsigned int type, unsigned int sff_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, DFD_DEBUG_SFF, type, sff_index, value);
}

/************************************end of transceiver***************************************/
/*****************************************sysled**********************************************/
/*
 * dfd_get_sys_led_status - Used to get sys led status
 * filled the value to buf, led status value define see enum fan_status_e
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_sys_led_status(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_led_status(WB_SYS_LED_FRONT, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_sys_led_status - Used to set sys led status
 * @status: led status, led status value define see enum led_status_e
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_sys_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_SYS_LED_FRONT, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_sys_led_by_bmc(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_BMC_SYS_LED, WB_MINOR_DEV_NONE, buf, count);
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_bmc_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_BMC_LED, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_bmc_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_BMC_LED, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_sys_fan_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_FAN_LED_FRONT, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_sys_fan_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_FAN_LED_FRONT, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_sys_psu_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_PSU_LED_FRONT, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_sys_psu_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_PSU_LED_FRONT, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_id_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_ID_LED_FRONT, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_id_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_ID_LED_FRONT, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_mem_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_MEM_LED, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_mem_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_MEM_LED, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_sys_hot_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_SYS_HOT_LED, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_sys_hot_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_SYS_HOT_LED, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_lan_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_OCP_LAN_LED, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_lan_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(WB_OCP_LAN_LED, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/*
 * dfd_get_bmc_host_sysled - Used to get BMC HOST system LED status
 * @buf: Data receiving buffer
 * @count: Length of the buffer
 *
 * This function returns the length of the filled buffer,
 * if not supported, it fills "NA" into the buffer,
 * otherwise, it returns a negative value on failure.
 */
static ssize_t dfd_get_bmc_host_sysled(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(BMC_HOST_SYS_LED, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_bmc_host_sysled_attr - Used to set BMC HOST system LED attributes
 * @status: LED status, defined in enum led_status_e
 *
 * This function returns 0 on success,
 * otherwise, it returns a negative value on failure.
 */
static int dfd_set_bmc_host_sysled_attr(int status)
{
    int ret;

    ret = dfd_set_led_status(BMC_HOST_SYS_LED, WB_MINOR_DEV_NONE, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/**************************************end of sysled******************************************/
/******************************************FPGA***********************************************/
static int dfd_get_main_board_fpga_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_FPGA);
    return ret;
}

static ssize_t dfd_get_fpga_attr(unsigned int fpga_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func fpga_get_func;

    fpga_get_func = dfd_get_sysfs_value_func(fpga_func_table, type, ARRAY_SIZE(fpga_func_table));
    if (fpga_get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = fpga_get_func(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_fpga_attr - Used to set fpga register
 * @cpld_index: start with 1
 * @type: write fpga attr
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_fpga_attr(unsigned int fpga_index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func fpga_set_func;

    fpga_set_func = dfd_set_sysfs_value_func(fpga_func_table, type, ARRAY_SIZE(fpga_func_table));
    if (fpga_set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = fpga_set_func(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/***************************************end of FPGA*******************************************/
/******************************************CPLD***********************************************/
static int dfd_get_main_board_cpld_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_CPLD);
    return ret;
}

static ssize_t dfd_get_cpld_attr(unsigned int cpld_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func cpld_get_func;

    cpld_get_func = dfd_get_sysfs_value_func(cpld_func_table, type, ARRAY_SIZE(cpld_func_table));
    if (cpld_get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = cpld_get_func(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_cpld_attr - Used to set cpld register
 * @cpld_index: start with 1
 * @type: write cpld attr
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_cpld_attr(unsigned int cpld_index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func cpld_set_func;

    cpld_set_func = dfd_set_sysfs_value_func(cpld_func_table, type, ARRAY_SIZE(cpld_func_table));
    if (cpld_set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = cpld_set_func(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}
/***************************************end of CPLD*******************************************/
/******************************************OTHER_FW***********************************************/
static int dfd_get_main_board_misc_fw_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_MISC_FW);
    return ret;
}

static ssize_t dfd_get_misc_fw_attr(unsigned int misc_fw_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func misc_fw_get_func;

    misc_fw_get_func = dfd_get_sysfs_value_func(misc_fw_func_table, type, ARRAY_SIZE(misc_fw_func_table));
    if (misc_fw_get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = misc_fw_get_func(WB_MAIN_DEV_MAINBOARD, misc_fw_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_misc_fw_attr - Used to set misc_fw register
 * @misc_fw_index: start with 1
 * @type: write misc_fw attr
 * @value: value write to misc_fw
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_misc_fw_attr(unsigned int misc_fw_index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func misc_fw_set_func;

    misc_fw_set_func = dfd_set_sysfs_value_func(misc_fw_func_table, type, ARRAY_SIZE(misc_fw_func_table));
    if (misc_fw_set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = misc_fw_set_func(WB_MAIN_DEV_MAINBOARD, misc_fw_index - 1, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/***************************************end of MISC_FW*******************************************/
/****************************************watchdog*********************************************/
/*
 * dfd_get_watchdog_identify - Used to get watchdog identify, such as iTCO_wdt
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_watchdog_identify(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_watchdog_info(WB_WDT_TYPE_NAME, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_watchdog_timeleft - Used to get watchdog timeleft,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_watchdog_timeleft(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_watchdog_info(WB_WDT_TYPE_TIMELEFT, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_watchdog_timeout - Used to get watchdog timeout,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_watchdog_timeout(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_watchdog_info(WB_WDT_TYPE_TIMEOUT, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_watchdog_timeout - Used to set watchdog timeout,
 * @value: timeout value
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_watchdog_timeout(int value)
{
    ssize_t ret;

    ret = dfd_watchdog_set_timeout(value);

    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_get_watchdog_enable_status - Used to get watchdog enable status,
 * filled the value to buf, 0: disable, 1: enable
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_watchdog_enable_status(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_watchdog_get_status(buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_watchdog_enable_status - Used to set watchdog enable status,
 * @value: enable status value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_watchdog_enable_status(int value)
{
    /* add vendor codes here */
    int ret;
    ret = dfd_watchdog_set_status(value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_set_watchdog_reset - Used to feed watchdog,
 * @value: any value to feed watchdog
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_watchdog_reset(int value)
{
    ssize_t ret;

    /* add vendor codes here */
    ret = dfd_watchdog_set_reset(value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*************************************end of watchdog*****************************************/
/******************************************slot***********************************************/
static int dfd_get_slot_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_NONE);
    return ret;
}

static int dfd_get_slot_temp_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_TEMP);
    return ret;
}

static int dfd_get_slot_vol_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_IN);
    return ret;
}

static int dfd_get_slot_curr_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_CURR);
    return ret;
}

static int dfd_get_slot_fpga_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_FPGA);
    return ret;
}

static int dfd_get_slot_cpld_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_SLOT, WB_MINOR_DEV_CPLD);
    return ret;
}

/* Similar to dfd_get_fan_model_name */
static ssize_t dfd_get_slot_model_name(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_NAME, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_slot_vendor(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_VENDOR, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to wb_get_fan_serial_number */
static ssize_t dfd_get_slot_serial_number(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_SN, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to wb_get_fan_part_number */
static ssize_t dfd_get_slot_part_number(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to wb_get_fan_hardware_version */
static ssize_t dfd_get_slot_hardware_version(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_HW_INFO, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_slot_card_type(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_DEV_TYPE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_slot_present(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_present_str(slot_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_fan_status */
static ssize_t dfd_get_slot_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_status_str(slot_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_fan_led_status */
static ssize_t dfd_get_slot_led_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_led_status(WB_SLOT_LED_MODULE, slot_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_fan_led_status */
static int dfd_set_slot_led_status(unsigned int slot_index, int status)
{
    int ret;

    ret = dfd_set_led_status(WB_SLOT_LED_MODULE, slot_index, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

static ssize_t dfd_get_slot_power_ctrl_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_power_ctrl_status_str(slot_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static int dfd_set_slot_power_ctrl_status(unsigned int slot_index, int status)
{
    int ret;

    ret = dfd_set_slot_power_ctrl_status_str(slot_index, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_alias */
static ssize_t dfd_get_slot_temp_alias(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_SLOT, slot_index, temp_index, WB_SENSOR_ALIAS,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_type */
static ssize_t dfd_get_slot_temp_type(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_SLOT, slot_index, temp_index, WB_SENSOR_TYPE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_max */
static ssize_t dfd_get_slot_temp_max(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_SLOT, slot_index, temp_index, WB_SENSOR_MAX,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_min */
static ssize_t dfd_get_slot_temp_min(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_SLOT, slot_index, temp_index, WB_SENSOR_MIN,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_value */
static ssize_t dfd_get_slot_temp_value(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_SLOT, slot_index, temp_index, WB_SENSOR_INPUT,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_alias */
static ssize_t dfd_get_slot_vol_alias(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_ALIAS,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_type */
static ssize_t dfd_get_slot_vol_type(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_TYPE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_max */
static ssize_t dfd_get_slot_vol_max(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_MAX,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_min */
static ssize_t dfd_get_slot_vol_min(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_MIN,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_range */
static ssize_t dfd_get_slot_vol_range(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_RANGE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_nominal_value */
static ssize_t dfd_get_slot_vol_nominal_value(unsigned int slot_index,
                   unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_NOMINAL_VAL,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_vol_value */
static ssize_t dfd_get_slot_vol_value(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_SLOT, slot_index, vol_index, WB_SENSOR_INPUT,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_curr_alias */
static ssize_t dfd_get_slot_curr_alias(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_SLOT, slot_index, curr_index, WB_SENSOR_ALIAS,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_curr_type */
static ssize_t dfd_get_slot_curr_type(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_SLOT, slot_index, curr_index, WB_SENSOR_TYPE,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_curr_max */
static ssize_t dfd_get_slot_curr_max(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_SLOT, slot_index, curr_index, WB_SENSOR_MAX,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_curr_min */
static ssize_t dfd_get_slot_curr_min(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_SLOT, slot_index, curr_index, WB_SENSOR_MIN,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_curr_value */
static ssize_t dfd_get_slot_curr_value(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_SLOT, slot_index, curr_index, WB_SENSOR_INPUT,
              buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_fpga_alias */
static ssize_t dfd_get_slot_fpga_alias(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_name(slot_index, fpga_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_fpga_type */
static ssize_t dfd_get_slot_fpga_type(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_type(slot_index, fpga_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_fpga_firmware_version */
static ssize_t dfd_get_slot_fpga_firmware_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_fw_version(slot_index, fpga_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_fpga_board_version */
static ssize_t dfd_get_slot_fpga_board_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_hw_version(slot_index, fpga_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_fpga_test_reg */
static ssize_t dfd_get_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_testreg_str(slot_index, fpga_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_main_board_fpga_test_reg */
static int dfd_set_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
           unsigned int value)
{
    int ret;

    ret = dfd_set_fpga_testreg(slot_index, fpga_index - 1, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/* Similar to dfd_get_main_board_cpld_alias */
static ssize_t dfd_get_slot_cpld_alias(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_name(slot_index, cpld_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_cpld_type */
static ssize_t dfd_get_slot_cpld_type(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_type(slot_index, cpld_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_cpld_firmware_version */
static ssize_t dfd_get_slot_cpld_firmware_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_fw_version(slot_index, cpld_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_cpld_board_version */
static ssize_t dfd_get_slot_cpld_board_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_hw_version(slot_index, cpld_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_cpld_test_reg */
static ssize_t dfd_get_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_testreg_str(slot_index, cpld_index - 1, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_set_main_board_cpld_test_reg */
static int dfd_set_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
           unsigned int value)
{
    int ret;

    ret = dfd_set_cpld_testreg(slot_index, cpld_index - 1, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}
/***************************************end of slot*******************************************/
/*****************************************system*********************************************/
static ssize_t dfd_get_system_my_slot_id(char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_my_slot_id(buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    buf = strstrip(buf);
    return ret;
}

static ssize_t dfd_get_system_bmc_status(int bmc_sw_status, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_bmc_status(bmc_sw_status, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static ssize_t dfd_get_system_value(unsigned int type, char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_system_value(type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_system_value_decode_string(unsigned int type, char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_system_value_decode_string(type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_set_system_value(unsigned int type, int value)
{
    int ret;

    ret = dfd_system_set_system_value(type, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

static ssize_t dfd_get_mem_isolation_value(unsigned int type, char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_mem_isolation_value(type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_set_mem_isolation_value(unsigned int type, int value)
{
    int ret;

    ret = dfd_system_set_mem_isolation_value(type, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

static ssize_t dfd_get_system_port_power_status(unsigned int type, char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_port_power_status(type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_system_bmc_view(char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_bmc_view(buf, count);
    if (ret < 0) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }

    return ret;
}

static ssize_t dfd_set_system_bmc_switch(const char* buf, size_t count)
{
    int ret;

    ret = dfd_system_set_bmc_switch(buf, count);
    return ret;
}

static ssize_t dfd_get_system_bmc_dualboot_wdt_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_bmc_dualboot_wdt_status(buf, count);
    if (ret < 0) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }

    return ret;
}

static ssize_t dfd_set_system_bmc_dualboot_wdt(const char* buf, size_t count)
{
    int ret;

    ret = dfd_system_set_bmc_dualboot_wdt(buf, count);
    return ret;
}

static ssize_t dfd_get_system_serial_number(char *buf, size_t count)
{
    int ret;

    /* Default to use tag name: syseeprom */
    ret = dfd_get_system_info(WB_MAIN_DEV_MAINBOARD, DFD_DEV_INFO_TYPE_SN, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/*************************************end of system*****************************************/
/*****************************************eeprom*********************************************/
static int dfd_get_eeprom_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_EEPROM);
    return ret;
}

/*
 * dfd_get_board_eeprom_size - Used to get board eeprom size, including slots eeprom
 *
 * This function returns the size of board eeprom, including slots eeprom
 * otherwise it returns a negative value on failed.
 */
static int dfd_get_board_eeprom_size(unsigned int e2_index)
{
    int ret;

    ret = dfd_get_eeprom_size(WB_MAIN_DEV_MAINBOARD, e2_index);
    return ret;
}

static ssize_t dfd_get_eeprom_attr(unsigned int eeprom_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func eeprom_get_func;

    eeprom_get_func = dfd_get_sysfs_value_func(eeprom_func_table, type, ARRAY_SIZE(eeprom_func_table));
    if (eeprom_get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = eeprom_get_func(WB_MAIN_DEV_MAINBOARD, eeprom_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_eeprom_attr - Used to set eeprom register
 * @cpld_index: start with 1
 * @type: write eeprom attr
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_eeprom_attr(unsigned int eeprom_index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func eeprom_set_func;

    eeprom_set_func = dfd_set_sysfs_value_func(eeprom_func_table, type, ARRAY_SIZE(eeprom_func_table));
    if (eeprom_set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = eeprom_set_func(WB_MAIN_DEV_MAINBOARD, eeprom_index, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}


/*
 * dfd_read_board_eeprom_data - Used to read board eeprom data, including slots eeprom
 * @buf: Data read buffer
 * @offset: offset address to read board eeprom data, including slots eeprom
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_read_board_eeprom_data(unsigned int e2_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_MAINBOARD, e2_index, buf, offset, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            /* No such device or address */
            return -ENXIO;
        }
        /* I/O error */
        return -EIO;
    }
    return ret;
}

/*
 * dfd_write_board_eeprom_data - Used to write board eeprom data, including slots eeprom
 * @buf: Data write buffer
 * @offset: offset address to write board eeprom data, including slots eeprom
 * @count: length of buf
 *
 * This function returns the written length of eeprom,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_write_board_eeprom_data(unsigned int e2_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    ret = dfd_write_eeprom_data(WB_MAIN_DEV_MAINBOARD, e2_index, buf, offset, count);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*************************************end of eeprom*****************************************/


/********************************************cabletray**********************************************/
static int dfd_get_cabletray_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_CABLETRAY, WB_MINOR_DEV_NONE);
    return ret;
}

/*
 * dfd_get_cabletray_attr - Used to get cabletray attr,
 * @cabletray_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_cabletray_attr(unsigned int cabletray_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func cabletray_get_func;

    cabletray_get_func = dfd_get_sysfs_value_func(cabletray_func_table, type, ARRAY_SIZE(cabletray_func_table));
    if (cabletray_get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = cabletray_get_func(WB_MAIN_DEV_CABLETRAY, cabletray_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_get_cabletray_eeprom_size - Used to get cabletray eeprom size
 *
 * This function returns the size of cabletray eeprom,
 * otherwise it returns a negative value on failed.
 */
 static int dfd_get_cabletray_eeprom_size(unsigned int cabletray_index)
 {
     int ret;

     ret = dfd_get_eeprom_size(WB_MAIN_DEV_CABLETRAY, cabletray_index);
     return ret;
 }

/*
 * dfd_read_cabletray_eeprom_data - Used to read cabletray eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read cabletray eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_read_cabletray_eeprom_data(unsigned int cabletray_index, char *buf, loff_t offset,
    size_t count)
{
    ssize_t ret;

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_CABLETRAY, cabletray_index, buf, offset, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            /* No such device or address */
            return -ENXIO;
        }
        /* I/O error */
        return -EIO;
    }
    return ret;
}
/*************************************end of cabletray*****************************************/

/******************************************SOL***********************************************/
static int dfd_get_sys_sol_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_SOL);
    return ret;
}

/*
 * dfd_get_sol_attr - Used to get sol register
 * @index: start with 1
 * @type: write sol attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_sol_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(sol_func_table, type, ARRAY_SIZE(sol_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, WB_MINOR_DEV_NONE, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_sol_attr - Used to set sol register
 * @index: start with 1
 * @type: write sol attr
 * @value: value write to sol
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_sol_attr(unsigned int index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func set_func;

    set_func = dfd_set_sysfs_value_func(sol_func_table, type, ARRAY_SIZE(sol_func_table));
    if (set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = set_func(index, WB_MINOR_DEV_NONE, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}
/***************************************end of SOL*******************************************/
/*****************************************leak_detector**********************************************/
static int dfd_get_sys_leak_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_LEAK_DETECTOR);
    return ret;
}

/*
 * dfd_get_sol_attr - Used to get sol register
 * @index: start with 1
 * @type: write sol attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_leak_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(leak_func_table, type, ARRAY_SIZE(leak_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(WB_MAIN_DEV_MAINBOARD, index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_leak_attr - Used to set leak register
 * @index: start with 1
 * @type: write leak attr
 * @value: value write to leak
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_leak_attr(unsigned int index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func set_func;

    set_func = dfd_set_sysfs_value_func(leak_func_table, type, ARRAY_SIZE(leak_func_table));
    if (set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = set_func(WB_MAIN_DEV_MAINBOARD, index, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/**************************************end of leak_detector******************************************/
/***********************************************gcu**************************************************/
static int dfd_get_sys_gcu_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_GCU);
    if (ret < 0) {
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return ret;
}

static ssize_t dfd_get_gcu_attr_funcs(unsigned int index, unsigned int type, dfd_sysfs_func_map_t **func_table, int gcu_type)
{
    if ((index < 1) || (index >= GCU_MAX_NUMBER)) {
        *func_table = NULL;
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (type == DFD_GCU_ID_MONITOR_FLAG_E) {
        if (DFD_GCU_TYPE_IS_VALID(gcu_type)) {
            *func_table = gcu_list[gcu_type].gcu_attr_funcs;
            return gcu_list[gcu_type].gcu_attr_funcs_size;
        }

        *func_table = NULL;
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (gcu_id_info_table[index - 1].gcu_type < 0 || gcu_id_info_table[index - 1].gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        *func_table = NULL;
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    *func_table = gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_attr_funcs;

    return gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_attr_funcs_size;
}

static ssize_t dfd_gcu_get_monitor_flag(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    int i;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = -1;

    for (i = 0; i < WB_MAIN_GCU_DEV_MAX; i++) {
        ret = dfd_get_gcu_attr_funcs(index, type, &func_table, gcu_list[i].gcu_main_id);
        if (func_table == NULL) {
            continue;
        }

        get_func = dfd_get_sysfs_value_func(func_table, type, ret);
        if (get_func == NULL) {
            continue;
        }

        ret = get_func(index, type, buf, count);
        if (ret < 0) {
            continue;
        }

        return ret;
    }

    return -DFD_RV_DEV_NOTSUPPORT;
}

/*
 * dfd_get_gcu_attr - Used to get gcu register
 * @index: start with 1
 * @type: write gcu attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_gcu_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    if (type == DFD_GCU_ID_MONITOR_FLAG_E) {
        ret = dfd_gcu_get_monitor_flag(index, type, buf, count);
        if (ret < 0) {
            if (ret == -DFD_RV_DEV_NOTSUPPORT) {
                return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
            } else {
                return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
            }
        }

        return ret;
    }

    ret = dfd_get_gcu_attr_funcs(index, type, &func_table, 0);
    if (func_table == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table, type, ret);
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static ssize_t dfd_get_gcu_temp_funcs(unsigned int index, unsigned int type, dfd_sysfs_func_map_t **func_table)
{
    if ((index < 1) || (index >= GCU_MAX_NUMBER)) {
        *func_table = NULL;
        return 0;
    }

    if (gcu_id_info_table[index - 1].gcu_type < 0 || gcu_id_info_table[index - 1].gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        *func_table = NULL;
        return 0;
    }

    *func_table = gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_temp_funcs;

    return gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_temp_funcs_size;
}

/*
 * dfd_get_gcu_temp_attr - Used to get gcu register
 * @index: start with 1
 * @type: write gcu attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_gcu_temp_attr(unsigned int gcu_index, unsigned int temp_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_gcu_temp_funcs(gcu_index, type, &func_table);
    if (func_table == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table,
        type, ret);
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_GCU_SENSOR_MAIN_DEV_ID(gcu_index, temp_index), type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static ssize_t dfd_get_gcu_vol_funcs(unsigned int index, unsigned int type, dfd_sysfs_func_map_t **func_table)
{
    if ((index < 1) || (index >= GCU_MAX_NUMBER)) {
        *func_table = NULL;
        return 0;
    }

    if (gcu_id_info_table[index - 1].gcu_type < 0 || gcu_id_info_table[index - 1].gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        *func_table = NULL;
        return 0;
    }

    *func_table = gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_vol_funcs;

    return gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_vol_funcs_size;
}

/*
 * dfd_get_gcu_vol_attr - Used to get gcu register
 * @index: start with 1
 * @type: write gcu attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_gcu_vol_attr(unsigned int gcu_index, unsigned int vol_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_gcu_vol_funcs(gcu_index, type, &func_table);
    if (func_table == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table,
        type, ret);
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_GCU_SENSOR_MAIN_DEV_ID(gcu_index, vol_index), type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static ssize_t dfd_get_gcu_power_funcs(unsigned int index, unsigned int type, dfd_sysfs_func_map_t **func_table)
{
    if ((index < 1) || (index >= GCU_MAX_NUMBER)) {
        *func_table = NULL;
        return 0;
    }

    if (gcu_id_info_table[index - 1].gcu_type < 0 || gcu_id_info_table[index - 1].gcu_type >= WB_MAIN_GCU_DEV_MAX) {
        *func_table = NULL;
        return 0;
    }

    *func_table = gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_power_funcs;

    return gcu_list[gcu_id_info_table[index - 1].gcu_type].gcu_power_funcs_size;
}

/*
 * dfd_get_gcu_power_attr - Used to get gcu register
 * @index: start with 1
 * @type: write gcu attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_gcu_power_attr(unsigned int gcu_index, unsigned int power_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;
    dfd_sysfs_func_map_t *func_table;

    ret = dfd_get_gcu_power_funcs(gcu_index, type, &func_table);
    if (func_table == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    get_func = dfd_get_sysfs_value_func(func_table,
        type, ret);
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_GCU_SENSOR_MAIN_DEV_ID(gcu_index, power_index), type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static int dfd_get_gcu_temp_number(void)
{
    int max_temp_number;
    int i;

    max_temp_number = 0;

    for (i = 0; i < WB_MAIN_GCU_DEV_MAX ; i++) {
        if (gcu_list[i].temp_number > max_temp_number) {
            max_temp_number = gcu_list[i].temp_number;
        }
    }

    return max_temp_number;
}

static int dfd_get_gcu_vol_number(void)
{
    int max_vol_number;
    int i;

    max_vol_number = 0;

    for (i = 0; i < WB_MAIN_GCU_DEV_MAX ; i++) {
        if (gcu_list[i].vol_number > max_vol_number) {
            max_vol_number = gcu_list[i].vol_number;
        }
    }

    return max_vol_number;
}

static int dfd_get_gcu_power_number(void)
{
    int max_power_number;
    int i;

    max_power_number = 0;

    for (i = 0; i < WB_MAIN_GCU_DEV_MAX ; i++) {
        if (gcu_list[i].power_number > max_power_number) {
            max_power_number = gcu_list[i].power_number;
        }
    }

    return max_power_number;
}

/**************************************end of gcu******************************************/
/***********************************************nvme**************************************************/
static int dfd_get_sys_nvme_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_NVME);

    return ret;
}

/*
 * dfd_nvme_nvme_attr - Used to get nvme register
 * @index: start with 1
 * @type: write nvme attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_nvme_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(nvme_basic_func_table, type,
        ARRAY_SIZE(nvme_basic_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/*
 * dfd_set_nvme_attr - Used to set nvme register
 * @index: start with 1
 * @type: write nvme attr
 * @value: value write to nvme
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_nvme_attr(unsigned int index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func set_func;

    set_func = dfd_set_sysfs_value_func(nvme_basic_func_table, type, ARRAY_SIZE(nvme_basic_func_table));
    if (set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = set_func(index, type, &value, 1);

    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}

/*
 * dfd_get_nvme_temp_attr - Used to get nvme register
 * @index: start with 1
 * @type: write nvme attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_nvme_temp_attr(unsigned int nvme_index, unsigned int temp_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(nvme_temp_func_table,
        type, ARRAY_SIZE(nvme_temp_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_NVME_SENSOR_MAIN_DEV_ID(nvme_index, temp_index), type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

/*
 * dfd_get_nvme_power_attr - Used to get nvme register
 * @index: start with 1
 * @type: write nvme attr
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_nvme_power_attr(unsigned int nvme_index, unsigned int power_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(nvme_power_func_table,
        type, ARRAY_SIZE(nvme_power_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(DFD_COM_NVME_SENSOR_MAIN_DEV_ID(nvme_index, power_index), type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    return ret;
}

static int dfd_get_nvme_temp_number(void)
{
    unsigned int max_temp_number;
    int i;

    max_temp_number = 0;
    for (i = 0; i < WB_MIAN_NVME_TYPE_MAX; i++) {
        if (nvme_list[i].temp_number > max_temp_number) {
            max_temp_number = nvme_list[i].temp_number;
        }
    }

    return max_temp_number;
}

static int dfd_get_nvme_power_number(void)
{
    unsigned int max_power_number;
    int i;

    max_power_number = 0;

    for (i = 0; i < WB_MIAN_NVME_TYPE_MAX; i++) {
        if (nvme_list[i].power_number > max_power_number) {
            max_power_number = nvme_list[i].power_number;
        }
    }

    return max_power_number;
}

/**************************************end of nvme******************************************/

/******************************************LSW***********************************************/
static int dfd_get_lsw_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_LSW);
    return ret;
}

static ssize_t dfd_get_lsw_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(lsw_func_table, type, ARRAY_SIZE(lsw_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_lsw_attr - Used to set lsw file
 * @index: start with 1
 * @type: write lsw attr
 * @value: value write to lsw
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_lsw_attr(unsigned int index, unsigned int type, unsigned int value)
{
    int ret;
    dfd_sysfs_set_data_func set_func;

    set_func = dfd_set_sysfs_value_func(lsw_func_table, type, ARRAY_SIZE(lsw_func_table));
    if (set_func == NULL) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    ret = set_func(index, type, &value, 1);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }

    return ret;
}
/***************************************end of LSW*******************************************/

/******************************************PCIe***********************************************/
static int dfd_get_pcie_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_PCIE);
    return ret;
}

static ssize_t dfd_get_pcie_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(pcie_func_table, type, ARRAY_SIZE(pcie_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_debug_pcie_attr - Used to set PCIe debug data
 * @pcie_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_debug_pcie_attr(unsigned int type, unsigned int pcie_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_PCIE, DFD_DEBUG_PCIE, type, pcie_index, value);
}

/***************************************end of PCIe*******************************************/

/******************************************CLOCK***********************************************/
static int dfd_get_clock_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_CLOCK);
    return ret;
}

static ssize_t dfd_get_clock_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(clock_func_table, type, ARRAY_SIZE(clock_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/***************************************end of CLOCK*******************************************/

/******************************************PLL***********************************************/
static int dfd_get_pll_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_PLL);
    return ret;
}

static ssize_t dfd_get_pll_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(pll_func_table, type, ARRAY_SIZE(pll_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/***************************************end of PLL*******************************************/

/******************************************AVS***********************************************/
static int dfd_get_avs_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_AVS);
    return ret;
}

static ssize_t dfd_get_avs_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(avs_func_table, type, ARRAY_SIZE(avs_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_set_debug_avs_attr(unsigned int type, unsigned int index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MAINBOARD, WB_MAIN_DEV_AVS, DFD_DEBUG_AVS, type,
            index, value);
}
/***************************************end of AVS*******************************************/

/******************************************disk***********************************************/
static int dfd_get_disk_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_DISK, WB_MINOR_DEV_NONE);
    return ret;
}

static ssize_t dfd_get_disk_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(disk_func_table, type, ARRAY_SIZE(disk_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_debug_disk_attr - Used to set DISK debug data
 * @disk_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_debug_disk_attr(unsigned int type, unsigned int disk_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_DISK, WB_MINOR_DEV_NONE, DFD_DEBUG_DISK, type, disk_index, value);
}
/***************************************end of disk*******************************************/

/******************************************md***********************************************/
static int dfd_get_md_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MD, WB_MINOR_DEV_NONE);
    return ret;
}

static ssize_t dfd_get_md_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;
    dfd_sysfs_get_data_func get_func;

    get_func = dfd_get_sysfs_value_func(md_func_table, type, ARRAY_SIZE(md_func_table));
    if (get_func == NULL) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = get_func(index, type, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/*
 * dfd_set_debug_md_attr - Used to set MD debug data
 * @md_index: start with 1
 * @type: min,max,value,alias
 * @value: debug value
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_set_debug_md_attr(unsigned int type, unsigned int md_index, const char *value)
{
    return dfd_debug_data_common_attr(WB_MAIN_DEV_MD, WB_MINOR_DEV_NONE, DFD_DEBUG_MD, type, md_index, value);
}
/***************************************end of md*******************************************/
/******************************************dpu***********************************************/
static int dfd_get_dpu_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_DPU);
    return ret;
}

static int dfd_get_dpu_fw_number(void)
{
    return dfd_common_get_dpu_fw_number();
}

static int dfd_get_dpu_temp_number(void)
{
    return dfd_common_get_dpu_temp_number();
}

static ssize_t dfd_get_dpu_attr(unsigned int index, unsigned int type, char *buf, size_t count)
{
    return dfd_common_get_dpu_attr(index, type, buf, count);
}

static int dfd_set_dpu_attr(unsigned int index, unsigned int type, unsigned int value)
{
    return dfd_common_set_dpu_attr(index, type, value);
}

static ssize_t dfd_get_dpu_fw_attr(unsigned int index, unsigned int fw_index, unsigned int type,
    char *buf, size_t count)
{
    return dfd_common_get_dpu_fw_attr(index, fw_index, type, buf, count);
}

static ssize_t dfd_get_dpu_temp_attr(unsigned int dpu_index, unsigned int temp_index, unsigned int type,
    char *buf, size_t count)
{
    return dfd_common_get_dpu_temp_attr(dpu_index, temp_index, type, buf, count);
}

/***************************************end of dpu*******************************************/

static struct switch_drivers_s switch_drivers = {
    /*
     * set odm switch drivers,
     * if not support the function, set corresponding hook to NULL.
     */
    /* temperature sensors */
    .get_main_board_temp_number = dfd_get_main_board_temp_number,
    .get_temp_attr = dfd_get_temp_attr,
    .get_main_board_temp_monitor_flag = dfd_get_main_board_temp_monitor_flag,
    .get_main_board_temp_status = dfd_get_main_board_temp_status,
    .set_main_board_debug_temp_attr = dfd_set_main_board_debug_temp_attr,
    /* voltage sensors */
    .get_main_board_vol_number = dfd_get_main_board_vol_number,
    .get_main_board_vol_alias = dfd_get_main_board_vol_alias,
    .get_main_board_vol_type = dfd_get_main_board_vol_type,
    .get_main_board_vol_threshold = dfd_get_main_board_vol_threshold,
    .get_main_board_vol_range = dfd_get_main_board_vol_range,
    .get_main_board_vol_nominal_value = dfd_get_main_board_vol_nominal_value,
    .get_main_board_vol_value = dfd_get_main_board_vol_value,
    .get_main_board_vol_monitor_flag = dfd_get_main_board_vol_monitor_flag,
    .set_main_board_debug_vol_attr = dfd_set_main_board_debug_vol_attr,
    .get_main_board_vol_status = dfd_get_main_board_vol_status,
    /* current sensors */
    .get_main_board_curr_number = dfd_get_main_board_curr_number,
    .get_main_board_curr_alias = dfd_get_main_board_curr_alias,
    .get_main_board_curr_type = dfd_get_main_board_curr_type,
    .get_main_board_curr_threshold = dfd_get_main_board_curr_threshold,
    .get_main_board_curr_value = dfd_get_main_board_curr_value,
    .get_main_board_curr_monitor_flag = dfd_get_main_board_curr_monitor_flag,
    .set_main_board_debug_curr_attr = dfd_set_main_board_debug_curr_attr,
    /* power sensors */
    .get_main_board_power_number = dfd_get_main_board_power_number,
    .get_main_board_power_alias = dfd_get_main_board_power_alias,
    .get_main_board_power_type = dfd_get_main_board_power_type,
    .get_main_board_power_max = dfd_get_main_board_power_max,
    .get_main_board_power_min = dfd_get_main_board_power_min,
    .get_main_board_power_value = dfd_get_main_board_power_value,
    .get_main_board_power_monitor_flag = dfd_get_main_board_power_monitor_flag,
    /* syseeprom */
    .get_syseeprom_size = dfd_get_syseeprom_size,
    .read_syseeprom_data = dfd_read_syseeprom_data,
    .write_syseeprom_data = dfd_write_syseeprom_data,
    .reload_s3ip_config = dfd_reload_s3ip_config,
    /* fan */
    .get_fan_number = dfd_get_fan_number,
    .get_fan_motor_number = dfd_get_fan_motor_number,
    .get_fan_model_name = dfd_get_fan_model_name,
    .get_fan_vendor = dfd_get_fan_vendor,
    .get_fan_serial_number = dfd_get_fan_serial_number,
    .get_fan_part_number = dfd_get_fan_part_number,
    .get_fan_hardware_version = dfd_get_fan_hardware_version,
    .get_fan_status = dfd_get_fan_status,
    .get_fan_present = dfd_get_fan_present,
    .get_fan_led_status = dfd_get_fan_led_status,
    .set_fan_led_status = dfd_set_fan_led_status,
    .get_fan_direction = dfd_get_fan_direction,
    .get_fan_motor_status = dfd_get_fan_motor_status,
    .get_fan_motor_speed = dfd_get_fan_motor_speed,
    .get_fan_motor_speed_tolerance = dfd_get_fan_motor_speed_tolerance,
    .get_fan_motor_speed_target = dfd_get_fan_motor_speed_target,
    .get_fan_motor_speed_max = dfd_get_fan_motor_speed_max,
    .get_fan_motor_speed_min = dfd_get_fan_motor_speed_min,
    .get_fan_motor_speed_threshold_cnt = dfd_get_fan_motor_speed_threshold_cnt,
    .get_fan_ratio = dfd_get_fan_ratio,
    .set_fan_ratio = dfd_set_fan_ratio,
    .get_fan_attr = dfd_get_fan_attr,
    .set_fan_attr = dfd_set_fan_attr,
    .get_fan_eeprom_size = dfd_get_fan_eeprom_size,
    .read_fan_eeprom_data = dfd_read_fan_eeprom_data,
    .write_fan_eeprom_data = dfd_write_fan_eeprom_data,
    .set_fan_dev_debug_fan_attr = dfd_set_fan_dev_debug_fan_attr,
    .get_fanctrl_duration_max = dfd_get_fanctrl_duration_max,
    /* psu */
    .get_psu_number = dfd_get_psu_number,
    .get_psu_temp_number = dfd_get_psu_temp_number,
    .get_psu_attr = dfd_get_psu_attr,
    .get_psu_status = dfd_get_psu_status,
    .get_psu_hw_status = dfd_get_psu_hw_status,
    .get_psu_hw_detail_status = dfd_get_psu_hw_detail_status,
    .get_psu_alarm = dfd_get_psu_alarm,
    .get_psu_type = dfd_get_psu_type,
    .get_psu_sensor_attr = dfd_get_psu_sensor_attr,
    .get_psu_present_status = dfd_get_psu_present,
    .get_psu_in_status = dfd_get_psu_in_status,
    .get_psu_out_status = dfd_get_psu_out_status,
    .get_psu_status_pmbus = dfd_get_psu_status_pmbus,
    .get_psu_fan_ratio = dfd_get_psu_fan_ratio,
    .set_psu_fan_ratio = dfd_set_psu_fan_ratio,
    .get_psu_led_status = dfd_get_psu_led_status,
    .get_psu_temp_attr = dfd_get_psu_temp_attr,
    .get_psu_attr_threshold = dfd_get_psu_attr_threshold,
    .get_psu_eeprom_size = dfd_get_psu_eeprom_size,
    .read_psu_eeprom_data = dfd_read_psu_eeprom_data,
    .get_psu_blackbox_path = dfd_get_psu_blackbox_path,
    .get_psu_pmbus_info = dfd_get_psu_pmbus_info,
    .clear_psu_blackbox = dfd_clear_psu_blackbox_info,
    .get_psu_support_upgrade = dfd_get_psu_support_upgrade,
    .get_psu_upgrade_active_type = dfd_get_psu_upgrade_active_type,
    .set_psu_reset = dfd_set_psu_reset,
    .set_psu_off = dfd_set_psu_off,
    .write_psu_eeprom_data = dfd_write_psu_eeprom_data,
    .set_main_board_debug_psu_status_attr = dfd_set_main_board_debug_psu_status_attr,
    .set_main_board_debug_psu_sensor_value_attr = dfd_set_main_board_debug_psu_sensor_value_attr,
    /* transceiver */
    .get_eth_number = dfd_get_eth_number,
    .get_transceiver_power_on_status = dfd_get_transceiver_power_on_status,
    .set_transceiver_power_on_status = dfd_set_transceiver_power_on_status,
    .get_sff_power_status_mix_default_value = dfd_get_sff_power_status_mix_default_value,
    .get_eth_power_on_status = dfd_get_eth_power_on_status,
    .set_eth_power_on_status = dfd_set_eth_power_on_status,
    .get_transceiver_present_status = dfd_get_transceiver_present_status,
    .get_eth_i2c_bus = dfd_get_eth_i2c_bus,
    .get_eth_cage_type = dfd_get_eth_cage_type,
    .get_eth_present_status = dfd_get_eth_present_status,
    .get_eth_attr = dfd_get_eth_attr,
    .set_eth_attr = dfd_set_eth_attr,
    .get_eth_eeprom_size = dfd_get_eth_eeprom_size,
    .read_eth_eeprom_data = dfd_read_eth_eeprom_data,
    .write_eth_eeprom_data = dfd_write_eth_eeprom_data,
    .get_eth_dev_available = dfd_get_eth_dev_available,
    .set_eth_dev_available = dfd_set_eth_dev_available,
    .get_eth_optoe_type = dfd_get_eth_optoe_type,
    .set_eth_optoe_type = dfd_set_eth_optoe_type,
    .get_eth_power_group = dfd_get_eth_power_group,
    .get_eth_e2_low_page = dfd_get_eth_e2_low_page,
    .get_eth_temp = dfd_get_eth_temp,
    .get_eth_port_led_status = dfd_get_eth_port_led_status,
    .set_eth_port_led_status = dfd_set_eth_port_led_status,
    .set_main_board_debug_sff_attr = dfd_set_main_board_debug_sff_attr,
    .get_eth_port_bus_status = dfd_get_eth_port_bus_status,
    /* sysled */
    .get_sys_led_status = dfd_get_sys_led_status,
    .set_sys_led_status = dfd_set_sys_led_status,
    .get_sys_led_by_bmc = dfd_get_sys_led_by_bmc,
    .get_bmc_led_status = dfd_get_bmc_led_status,
    .set_bmc_led_status = dfd_set_bmc_led_status,
    .get_sys_fan_led_status = dfd_get_sys_fan_led_status,
    .set_sys_fan_led_status = dfd_set_sys_fan_led_status,
    .get_sys_psu_led_status = dfd_get_sys_psu_led_status,
    .set_sys_psu_led_status = dfd_set_sys_psu_led_status,
    .get_id_led_status = dfd_get_id_led_status,
    .set_id_led_status = dfd_set_id_led_status,
    .get_mem_led_status = dfd_get_mem_led_status,
    .set_mem_led_status = dfd_set_mem_led_status,
    .get_sys_hot_led_status = dfd_get_sys_hot_led_status,
    .set_sys_hot_led_status = dfd_set_sys_hot_led_status,
    .get_lan_led_status = dfd_get_lan_led_status,
    .set_lan_led_status = dfd_set_lan_led_status,
    .get_bmc_host_sysled = dfd_get_bmc_host_sysled,
    .set_bmc_host_sysled_attr = dfd_set_bmc_host_sysled_attr,
    /* FPGA */
    .get_main_board_fpga_number = dfd_get_main_board_fpga_number,
    .get_main_board_fpga_attr = dfd_get_fpga_attr,
    .set_main_board_fpga_attr = dfd_set_fpga_attr,
    /* CPLD */
    .get_main_board_cpld_number = dfd_get_main_board_cpld_number,
    .get_main_board_cpld_attr = dfd_get_cpld_attr,
    .set_main_board_cpld_attr = dfd_set_cpld_attr,
    /* MISC_FW */
    .get_main_board_misc_fw_number = dfd_get_main_board_misc_fw_number,
    .get_main_board_misc_fw_attr = dfd_get_misc_fw_attr,
    .set_main_board_misc_fw_attr = dfd_set_misc_fw_attr,
    /* watchdog */
    .get_watchdog_identify = dfd_get_watchdog_identify,
    .get_watchdog_timeleft = dfd_get_watchdog_timeleft,
    .get_watchdog_timeout = dfd_get_watchdog_timeout,
    .set_watchdog_timeout = dfd_set_watchdog_timeout,
    .get_watchdog_enable_status = dfd_get_watchdog_enable_status,
    .set_watchdog_enable_status = dfd_set_watchdog_enable_status,
    .set_watchdog_reset = dfd_set_watchdog_reset,
    /* slot */
    .get_slot_number = dfd_get_slot_number,
    .get_slot_temp_number = dfd_get_slot_temp_number,
    .get_slot_vol_number = dfd_get_slot_vol_number,
    .get_slot_curr_number = dfd_get_slot_curr_number,
    .get_slot_cpld_number = dfd_get_slot_cpld_number,
    .get_slot_fpga_number = dfd_get_slot_fpga_number,
    .get_slot_model_name = dfd_get_slot_model_name,
    .get_slot_vendor = dfd_get_slot_vendor,
    .get_slot_serial_number = dfd_get_slot_serial_number,
    .get_slot_part_number = dfd_get_slot_part_number,
    .get_slot_hardware_version = dfd_get_slot_hardware_version,
    .get_slot_card_type = dfd_get_slot_card_type,
    .get_slot_present = dfd_get_slot_present,
    .get_slot_status = dfd_get_slot_status,
    .get_slot_led_status = dfd_get_slot_led_status,
    .set_slot_led_status = dfd_set_slot_led_status,
    .get_slot_power_ctrl_status = dfd_get_slot_power_ctrl_status,
    .set_slot_power_ctrl_status = dfd_set_slot_power_ctrl_status,
    .get_slot_temp_alias = dfd_get_slot_temp_alias,
    .get_slot_temp_type = dfd_get_slot_temp_type,
    .get_slot_temp_max = dfd_get_slot_temp_max,
    .get_slot_temp_min = dfd_get_slot_temp_min,
    .get_slot_temp_value = dfd_get_slot_temp_value,
    .get_slot_vol_alias = dfd_get_slot_vol_alias,
    .get_slot_vol_type = dfd_get_slot_vol_type,
    .get_slot_vol_max = dfd_get_slot_vol_max,
    .get_slot_vol_min = dfd_get_slot_vol_min,
    .get_slot_vol_range = dfd_get_slot_vol_range,
    .get_slot_vol_nominal_value = dfd_get_slot_vol_nominal_value,
    .get_slot_vol_value = dfd_get_slot_vol_value,
    .get_slot_curr_alias = dfd_get_slot_curr_alias,
    .get_slot_curr_type = dfd_get_slot_curr_type,
    .get_slot_curr_max = dfd_get_slot_curr_max,
    .get_slot_curr_min = dfd_get_slot_curr_min,
    .get_slot_curr_value = dfd_get_slot_curr_value,
    .get_slot_fpga_alias = dfd_get_slot_fpga_alias,
    .get_slot_fpga_type = dfd_get_slot_fpga_type,
    .get_slot_fpga_firmware_version = dfd_get_slot_fpga_firmware_version,
    .get_slot_fpga_board_version = dfd_get_slot_fpga_board_version,
    .get_slot_fpga_test_reg = dfd_get_slot_fpga_test_reg,
    .set_slot_fpga_test_reg = dfd_set_slot_fpga_test_reg,
    .get_slot_cpld_alias = dfd_get_slot_cpld_alias,
    .get_slot_cpld_type = dfd_get_slot_cpld_type,
    .get_slot_cpld_firmware_version = dfd_get_slot_cpld_firmware_version,
    .get_slot_cpld_board_version = dfd_get_slot_cpld_board_version,
    .get_slot_cpld_test_reg = dfd_get_slot_cpld_test_reg,
    .set_slot_cpld_test_reg = dfd_set_slot_cpld_test_reg,
    .get_system_value = dfd_get_system_value,
    .get_system_value_decode_string = dfd_get_system_value_decode_string,
    .get_my_slot_id = dfd_get_system_my_slot_id,
    .get_bmc_status = dfd_get_system_bmc_status,
    .get_system_port_power_status = dfd_get_system_port_power_status,
    .set_system_value = dfd_set_system_value,
    .set_bmc_switch = dfd_set_system_bmc_switch,
    .get_bmc_view = dfd_get_system_bmc_view,
    .get_bmc_dualboot_wdt_status = dfd_get_system_bmc_dualboot_wdt_status,
    .set_bmc_dualboot_wdt = dfd_set_system_bmc_dualboot_wdt,
    .get_system_serial_number = dfd_get_system_serial_number,
    .get_mem_isolation_value = dfd_get_mem_isolation_value,
    .set_mem_isolation_value = dfd_set_mem_isolation_value,
    /* eeprom */
    .get_eeprom_number = dfd_get_eeprom_number,
    .get_eeprom_size = dfd_get_board_eeprom_size,
    .get_eeprom_attr = dfd_get_eeprom_attr,
    .set_eeprom_attr = dfd_set_eeprom_attr,
    .read_eeprom_data = dfd_read_board_eeprom_data,
    .write_eeprom_data = dfd_write_board_eeprom_data,
    /* cabletray */
    .get_cabletray_number = dfd_get_cabletray_number,
    .get_cabletray_attr = dfd_get_cabletray_attr,
    .get_cabletray_eeprom_size = dfd_get_cabletray_eeprom_size,
    .read_cabletray_eeprom_data = dfd_read_cabletray_eeprom_data,
    /* SOL */
    .get_main_board_sol_number = dfd_get_sys_sol_number,
    .get_main_board_sol_attr = dfd_get_sol_attr,
    .set_main_board_sol_attr = dfd_set_sol_attr,
    /* leak */
    .get_main_board_leak_number = dfd_get_sys_leak_number,
    .get_main_board_leak_attr = dfd_get_leak_attr,
    .set_main_board_leak_attr = dfd_set_leak_attr,
    /* gcu */
    .get_gcu_number = dfd_get_sys_gcu_number,
    .get_main_board_gcu_attr = dfd_get_gcu_attr,
    .get_gcu_temp_attr = dfd_get_gcu_temp_attr,
    .get_gcu_vol_attr = dfd_get_gcu_vol_attr,
    .get_gcu_power_attr = dfd_get_gcu_power_attr,
    .get_gcu_temp_number = dfd_get_gcu_temp_number,
    .get_gcu_vol_number = dfd_get_gcu_vol_number,
    .get_gcu_power_number = dfd_get_gcu_power_number,
    /* nvme */
    .get_nvme_number = dfd_get_sys_nvme_number,
    .get_nvme_attr = dfd_get_nvme_attr,
    .set_nvme_attr = dfd_set_nvme_attr,
    .get_nvme_temp_attr = dfd_get_nvme_temp_attr,
    .get_nvme_power_attr = dfd_get_nvme_power_attr,
    .get_nvme_temp_number = dfd_get_nvme_temp_number,
    .get_nvme_power_number = dfd_get_nvme_power_number,
    /* debug */
    .set_debug_and_reset = dfd_set_debug_and_reset,

    /* lsw */
    .get_lsw_number = dfd_get_lsw_number,
    .get_lsw_attr = dfd_get_lsw_attr,
    .set_lsw_attr = dfd_set_lsw_attr,
    /* pcie */
    .get_pcie_number = dfd_get_pcie_number,
    .get_pcie_attr = dfd_get_pcie_attr,
    .set_debug_pcie_attr = dfd_set_debug_pcie_attr,
    /* clock */
    .get_clock_number = dfd_get_clock_number,
    .get_clock_attr = dfd_get_clock_attr,
    /* pll */
    .get_pll_number = dfd_get_pll_number,
    .get_pll_attr = dfd_get_pll_attr,
    /* avs */
    .get_avs_number = dfd_get_avs_number,
    .get_avs_attr = dfd_get_avs_attr,
    .set_debug_avs_attr = dfd_set_debug_avs_attr,
    /* disk */
    .get_disk_number = dfd_get_disk_number,
    .get_disk_attr = dfd_get_disk_attr,
    .set_debug_disk_attr = dfd_set_debug_disk_attr,
    /* md */
    .get_md_number = dfd_get_md_number,
    .get_md_attr = dfd_get_md_attr,
    .set_debug_md_attr = dfd_set_debug_md_attr,
    /* dpu */
    .get_dpu_number = dfd_get_dpu_number,
    .get_dpu_fw_number = dfd_get_dpu_fw_number,
    .get_dpu_temp_number = dfd_get_dpu_temp_number,
    .get_dpu_attr = dfd_get_dpu_attr,
    .set_dpu_attr = dfd_set_dpu_attr,
    .get_dpu_fw_attr = dfd_get_dpu_fw_attr,
    .get_dpu_temp_attr = dfd_get_dpu_temp_attr,
};

struct switch_drivers_s * s3ip_switch_driver_get(void)
{
    return &switch_drivers;
}

static int32_t __init switch_driver_init(void)
{
    int ret;

    SWITCH_DEBUG(DBG_VERBOSE, "Enter.\n");
    ret = wb_dev_cfg_init();
    if (ret < 0) {
        SWITCH_DEBUG(DBG_ERROR, "wb_dev_cfg_init failed ret %d.\n", ret);
        return ret;
    }
    SWITCH_DEBUG(DBG_VERBOSE, "success.\n");
    return 0;
}

static void __exit switch_driver_exit(void)
{
    SWITCH_DEBUG(DBG_VERBOSE, "switch_driver_exit.\n");
    wb_dev_cfg_exit();
    return;
}

module_init(switch_driver_init);
module_exit(switch_driver_exit);
EXPORT_SYMBOL(s3ip_switch_driver_get);
module_param(g_switch_dbg_level, int, S_IRUGO | S_IWUSR);
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_LICENSE("GPL");
