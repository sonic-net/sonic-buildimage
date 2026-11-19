/*
 * An switch_driver driver for switch devcie function
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>

#include "dfd_sysfs_common.h"
#include "switch_driver.h"
#include "wb_module.h"
#include "wb_fan_driver.h"
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
#include "dfd_cfg.h"

int g_switch_dbg_level = 0;

/* change the following parameter by your switch. */
#define MAIN_BOARD_TEMP_SENSOR_NUMBER    (10)
#define MAIN_BOARD_VOL_SENSOR_NUMBER     (10)
#define MAIN_BOARD_CURR_SENSOR_NUMBER    (0)
#define SYSEEPROM_SIZE                   (256)
#define FAN_NUMBER                       (6)
#define FAN_MOTOR_NUMBER                 (2)
#define PSU_NUMBER                       (2)
#define PSU_TEMP_SENSOR_NUMBER           (3)
#define ETH_NUMBER                       (32)
#define ETH_EEPROM_SIZE                  (0x8180)
#define MAIN_BOARD_FPGA_NUMBER           (1)
#define MAIN_BOARD_CPLD_NUMBER           (5)
#define SLOT_NUMBER                      (0)
#define SLOT_TEMP_NUMBER                 (0)
#define SLOT_VOL_NUMBER                  (0)
#define SLOT_CURR_NUMBER                 (0)
#define SLOT_FPGA_NUMBER                 (0)
#define SLOT_CPLD_NUMBER                 (0)

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
 * dfd_get_main_board_temp_alias - Used to identify the location of the temperature sensor,
 * such as air_inlet, air_outlet and so on.
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_alias(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_ALIAS,
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
 * dfd_get_main_board_temp_type - Used to get the model of temperature sensor,
 * such as lm75, tmp411 and so on
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_type(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_TYPE,
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
 * dfd_get_main_board_temp_max - Used to get the maximum threshold of temperature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_max(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_MAX,
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
 * dfd_get_main_board_temp_min - Used to get the minimum threshold of temperature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_min(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_MIN,
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
 * dfd_get_main_board_temp_high - Used to get the high threshold of temperature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_high(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_HIGH,
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
 * dfd_get_main_board_temp_low - Used to get the low threshold of temperature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_low(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_LOW,
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
 * dfd_get_main_board_temp_value - Used to get the input value of temperature sensor
 * filled the value to buf, the value is integer with millidegree Celsius
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_temp_value(unsigned int temp_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, temp_index, WB_SENSOR_INPUT,
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
 * dfd_get_main_board_vol_max - Used to get the maximum threshold of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_max(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_MAX,
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
 * dfd_get_main_board_vol_min - Used to get the minimum threshold of voltage sensor
 * filled the value to buf, the value is integer with mV
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_vol_min(unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, vol_index, WB_SENSOR_MIN,
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
 * dfd_get_main_board_curr_max - Used to get the maximum threshold of current sensor
 * filled the value to buf, the value is integer with mA
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_max(unsigned int curr_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, curr_index, WB_SENSOR_MAX,
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
 * dfd_get_main_board_curr_min - Used to get the minimum threshold of current sensor
 * filled the value to buf, the value is integer with mA
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_curr_min(unsigned int curr_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_NONE, curr_index, WB_SENSOR_MIN,
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
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
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
/****************************************end of fan*******************************************/
/********************************************psu**********************************************/
static int dfd_get_psu_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_PSU, WB_MINOR_DEV_NONE);
    return ret;
}

/*
 * dfd_get_psu_present_status - Used to get psu present status
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

/* Similar to dfd_get_psu_model_name */
static ssize_t dfd_get_psu_model_name(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_PART_NAME, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_vendor(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_VENDOR, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_date(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_ASSET_TAG, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static ssize_t dfd_get_psu_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status_word;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf,  count);
        status = status | 0x01;
        return (ssize_t)snprintf(buf, count, "0x%x\n", status);
    }

    ret = dfd_get_psu_pmbus_status(psu_index, buf, count);
    if (ret < 0) {
        SWITCH_DEBUG(DBG_ERROR, "get psu pmbus status error, ret: %ld, psu_index: %u\n", ret, psu_index);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    } else {
        ret = kstrtoint(buf, 0, &status_word);
        if (ret) {
            SWITCH_DEBUG(DBG_ERROR, "invalid value: %s \n", buf);
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }

    status = 0;
    mem_clear(buf,  count);
    if (status_word < 0) {
        return status_word;
    } else {
        status = (status_word & PSU_OFF_FAULT) ? (status | 0x02) : status;
        status = (status_word & PSU_FAN_FAULT) ? (status | 0x04) : status;
        status = (status_word & PSU_VOUT_FAULT) ? (status | 0x08) : status;
        status = (status_word & PSU_IOUT_FAULT) ? (status | 0x10) : status;
        status = (status_word & PSU_INPUT_FAULT) ? (status | 0x20) : status;
        status = (status_word & PSU_TEMP_FAULT) ? (status | 0x40) : status;
    }
    return (ssize_t)snprintf(buf, count, "0x%x\n", status);
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

/* Similar to wb_get_fan_serial_number */
static ssize_t dfd_get_psu_serial_number(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_SN, buf, count);
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
static ssize_t dfd_get_psu_part_number(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count);
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
static ssize_t dfd_get_psu_hardware_version(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_HW_INFO, buf, count);
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
 * dfd_get_psu_in_curr - Used to get the input current of psu
 * filled the value to buf, the value is integer with mA
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_in_curr(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_IN_CURR, buf, count);
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
 * dfd_get_psu_in_vol - Used to get the input voltage of psu
 * filled the value to buf, the value is integer with mV
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_in_vol(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_IN_VOL, buf, count);
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
 * dfd_get_psu_in_power - Used to get the input power of psu
 * filled the value to buf, the value is integer with uW
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_in_power(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_IN_POWER, buf, count);
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
 * dfd_get_psu_out_curr - Used to get the output current of psu
 * filled the value to buf, the value is integer with mA
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_out_curr(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_OUT_CURR, buf, count);
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
 * dfd_get_psu_out_vol - Used to get the output voltage of psu
 * filled the value to buf, the value is integer with mV
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_out_vol(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_OUT_VOL, buf, count);
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
 * dfd_get_psu_out_power - Used to get the output power of psu
 * filled the value to buf, the value is integer with uW
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_out_power(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_OUT_POWER, buf, count);
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
 * dfd_get_psu_out_max_power - Used to get the output max power of psu
 * filled the value to buf, the value is integer with uW
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_out_max_power(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_MAX_OUTPUT_POWRER, buf, count);
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

static ssize_t dfd_get_psu_attr_threshold(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_threshold_str(psu_index, type, buf, count);
    if (ret < 0) {
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
 * dfd_get_psu_fan_speed - Used to get psu fan speed
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_fan_speed(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_sensor_info(psu_index, PSU_FAN_SPEED, buf, count);
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

/*
 * dfd_get_psu_fan_direction - Used to get psu air flow direction,
 * filled the value to buf, air flow direction define enum air_flow_direction_e
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_psu_fan_direction(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_FAN_DIRECTION, buf, count);
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
static ssize_t dfd_get_psu_led_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status_word;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        status = LED_STATUS_DARK; /* led off */
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
    status = LED_STATUS_GREEN;

    ret = dfd_get_psu_pmbus_status(psu_index, buf, count);
    if (ret < 0) {
        SWITCH_DEBUG(DBG_ERROR, "get psu pmbus status error, ret: %ld, psu_index: %u\n", ret, psu_index);
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    } else {
        ret = kstrtoint(buf, 0, &status_word);
        if (ret) {
            SWITCH_DEBUG(DBG_ERROR, "invalid value: %s \n", buf);
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    mem_clear(buf,  count);
    if (status_word > 0) {
        status = LED_STATUS_YELLOW; /* led amber */
        return (ssize_t)snprintf(buf, count, "%d\n", status);
    }
    return (ssize_t)snprintf(buf, count, "%d\n", status); /* led green */
}

static ssize_t dfd_get_psu_fan_speed_cal(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_SPEED_CAL, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

/* Similar to dfd_get_main_board_temp_alias */
static ssize_t dfd_get_psu_temp_alias(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, WB_SENSOR_ALIAS,
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
static ssize_t dfd_get_psu_temp_type(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, WB_SENSOR_TYPE,
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
static ssize_t dfd_get_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, WB_SENSOR_MAX,
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

/* Similar to dfd_set_main_board_temp_max */
static int dfd_set_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -WB_SYSFS_RV_UNSUPPORT;
}

/* Similar to dfd_get_main_board_temp_min */
static ssize_t dfd_get_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, WB_SENSOR_MIN,
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

/* Similar to dfd_set_main_board_temp_min */
static int dfd_set_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -WB_SYSFS_RV_UNSUPPORT;
}

/* Similar to dfd_get_main_board_temp_value */
static ssize_t dfd_get_psu_temp_value(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;
    int status;

    status = dfd_get_psu_present_status(psu_index);
    if (status == DEV_ABSENT) {
        mem_clear(buf,  count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
    }

    ret = dfd_get_temp_info(WB_MAIN_DEV_PSU, psu_index, temp_index, WB_SENSOR_INPUT,
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

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_PSU, psu_index, buf, offset, count);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

static ssize_t dfd_get_psu_blackbox_info(unsigned int psu_index, char *buf, size_t count)
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
    unsigned int eth_index, eth_num;
    int len, left_len;
    eth_num = dfd_get_dev_number(WB_MAIN_DEV_SFF, WB_MINOR_DEV_NONE);
    if (eth_num <= 0) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }

    mem_clear(buf, count);
    len = 0;
    left_len = count - 1;

    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        SWITCH_DEBUG(DBG_VERBOSE, "eth index: %u\n", eth_index);
        if (left_len > 0) {
            ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_POWER_ON, buf, left_len);
            if (ret < 0) {
                SWITCH_DEBUG(DBG_ERROR, "get eth%u power status failed, ret: %ld\n", eth_index, ret);
                break;
            }
        } else {
            SWITCH_DEBUG(DBG_ERROR, "error: get_transceiver_power_on_status are not enough buffers.\n");
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
        SWITCH_DEBUG(DBG_ERROR, "error: get_transceiver_power_on_status buffers too long, need: %ld, act: %d.\n", count, len);
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }
    buf[len] = '\n';
    ret = strlen(buf);
    SWITCH_DEBUG(DBG_VERBOSE, "get_transceiver_power_on_status ok. sff num:%d, len:%ld\n", eth_num, ret);

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

    eth_num = dfd_get_dev_number(WB_MAIN_DEV_SFF, WB_MINOR_DEV_NONE);
    if (eth_num <= 0) {
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }

    mem_clear(buf, count);
    len = 0;
    left_len = count - 1;

    for (eth_index = 1; eth_index <= eth_num; eth_index++) {
        SWITCH_DEBUG(DBG_VERBOSE, "eth index: %u\n", eth_index);
        if (left_len > 0) {
            ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_MODULE_PRESENT, buf + len, left_len);
            if (ret < 0) {
                SWITCH_DEBUG(DBG_ERROR, "get eth%u present status failed, ret: %ld\n", eth_index, ret);
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
        SWITCH_DEBUG(DBG_ERROR, "error: get_transceiver_present_status buffers too long, need: %ld, act: %d.\n", count, len);
        mem_clear(buf, count);
        return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
    }
    buf[len] = '\n';
    ret = strlen(buf);
    SWITCH_DEBUG(DBG_VERBOSE, "get_transceiver_present_status ok. sff num:%d, len:%ld\n", eth_num, ret);

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
    int ret;

    ret = dfd_set_sff_cpld_info(eth_index, WB_SFF_POWER_ON, status);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}

/*
 * dfd_get_eth_tx_fault_status - Used to get port tx_fault status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_tx_fault_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_TX_FAULT, buf, count);
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
 * dfd_get_eth_tx_disable_status - Used to get port tx_disable status,
 * filled the value to buf, 0: tx_enable, 1: tx_disable
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_tx_disable_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_TX_DIS, buf, count);
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
 * dfd_set_eth_tx_disable_status - Used to set port tx_disable status,
 * @eth_index: start with 1
 * @status: tx_disable status, 0: tx_enable, 1: tx_disable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_eth_tx_disable_status(unsigned int eth_index, int status)
{
    int ret;

    ret = dfd_set_sff_cpld_info(eth_index, WB_SFF_TX_DIS, status);
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
 * dfd_get_eth_rx_los_status - Used to get port rx_los status,
 * filled the value to buf, 0: normal, 1: abnormal
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_rx_los_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_RX_LOS, buf, count);
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
 * dfd_get_eth_reset_status - Used to get port reset status,
 * filled the value to buf, 0: unreset, 1: reset
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_reset_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_RESET, buf, count);
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
 * dfd_set_eth_reset_status - Used to set port reset status,
 * @eth_index: start with 1
 * @status: reset status, 0: unreset, 1: reset
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_eth_reset_status(unsigned int eth_index, int status)
{
    int ret;

    ret = dfd_set_sff_cpld_info(eth_index, WB_SFF_RESET, status);
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


/*
 * dfd_get_eth_low_power_mode_status - Used to get port low power mode status,
 * filled the value to buf, 0: high power mode, 1: low power mode
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_low_power_mode_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_LPMODE, buf, count);
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
 * dfd_get_eth_interrupt_status - Used to get port interruption status,
 * filled the value to buf, 0: no interruption, 1: interruption
 * @eth_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_eth_interrupt_status(unsigned int eth_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_sff_cpld_info(eth_index, WB_SFF_INTERRUPT, buf, count);
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
                   size_t count)
{
    ssize_t ret;

    ret = dfd_read_eeprom_data(WB_MAIN_DEV_SFF, eth_index, buf, offset, count);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
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
                   size_t count)
{
    ssize_t ret;

    ret = dfd_write_eeprom_data(WB_MAIN_DEV_SFF, eth_index, buf, offset, count);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
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
static ssize_t dfd_get_bmc_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(WB_BMC_LED_FRONT, WB_MINOR_DEV_NONE, buf, count);
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

    ret = dfd_set_led_status(WB_BMC_LED_FRONT, WB_MINOR_DEV_NONE, status);
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

/**************************************end of sysled******************************************/
/******************************************FPGA***********************************************/
static int dfd_get_main_board_fpga_number(void)
{
    int ret;

    ret = dfd_get_dev_number(WB_MAIN_DEV_MAINBOARD, WB_MINOR_DEV_FPGA);
    return ret;
}

/*
 * dfd_get_main_board_fpga_alias - Used to identify the location of fpga,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_fpga_alias(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_name(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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
 * dfd_get_main_board_fpga_type - Used to get fpga model name
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_fpga_type(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_type(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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
 * dfd_get_main_board_fpga_firmware_version - Used to get fpga firmware version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_fpga_firmware_version(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_fw_version(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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
 * dfd_get_main_board_fpga_board_version - Used to get fpga board version,
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_fpga_board_version(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_hw_version(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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
 * dfd_get_main_board_fpga_test_reg - Used to test fpga register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_fpga_test_reg(unsigned int fpga_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_testreg_str(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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
 * dfd_set_main_board_fpga_test_reg - Used to test fpga register write
 * @fpga_index: start with 1
 * @value: value write to fpga
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_main_board_fpga_test_reg(unsigned int fpga_index, unsigned int value)
{
    int ret;

    ret = dfd_set_fpga_testreg(WB_MAIN_DEV_MAINBOARD, fpga_index - 1, value);
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

/*
 * dfd_get_main_board_cpld_alias - Used to identify the location of cpld,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_cpld_alias(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_name(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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
 * dfd_get_main_board_cpld_type - Used to get cpld model name
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_cpld_type(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_type(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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
 * dfd_get_main_board_cpld_firmware_version - Used to get cpld firmware version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_cpld_firmware_version(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_fw_version(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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
 * dfd_get_main_board_cpld_board_version - Used to get cpld board version,
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_cpld_board_version(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_hw_version(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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
 * dfd_get_main_board_cpld_test_reg - Used to test cpld register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_main_board_cpld_test_reg(unsigned int cpld_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_testreg_str(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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
 * dfd_set_main_board_cpld_test_reg - Used to test cpld register write
 * @cpld_index: start with 1
 * @value: value write to cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int dfd_set_main_board_cpld_test_reg(unsigned int cpld_index, unsigned int value)
{
    int ret;

    ret = dfd_set_cpld_testreg(WB_MAIN_DEV_MAINBOARD, cpld_index - 1, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}
/***************************************end of CPLD*******************************************/
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
    /* add vendor codes here */
    return -WB_SYSFS_RV_UNSUPPORT;
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
    /* add vendor codes here */
    return -WB_SYSFS_RV_UNSUPPORT;
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

static ssize_t dfd_get_slot_power_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_power_status_str(slot_index, buf, count);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return ret;
}

static int dfd_set_slot_power_status(unsigned int slot_index, int status)
{
    int ret;

    ret = dfd_set_slot_power_status_str(slot_index, status);
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

    ret = dfd_set_fpga_testreg(slot_index, fpga_index - 1, value);
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

    ret = dfd_set_cpld_testreg(slot_index, cpld_index - 1, value);
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
    }
    return ret;
}
/***************************************end of slot*******************************************/
/*****************************************system*********************************************/
static ssize_t dfd_get_system_value(unsigned int type, int *value, char *buf, size_t count)
{
    int ret;

    ret = dfd_system_get_system_value(type, value);
    if (ret < 0) {
        if (ret == -DFD_RV_DEV_NOTSUPPORT) {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
        } else {
            return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_ERROR);
        }
    }
    return (ssize_t)snprintf(buf, count, "%d\n", *value);
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

/*
 * dfd_get_board_eeprom_alias - Used to get board eeprom alias, including slots eeprom
 *
 * This function returns the alias of board eeprom, including slots eeprom
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_board_eeprom_alias(unsigned int e2_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_eeprom_alias(WB_MAIN_DEV_MAINBOARD, e2_index, buf, count);
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
 * dfd_get_board_eeprom_tag - Used to get board eeprom tag, including slots eeprom
 *
 * This function returns the alias of board eeprom, including slots eeprom
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_board_eeprom_tag(unsigned int e2_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_eeprom_tag(WB_MAIN_DEV_MAINBOARD, e2_index, buf, count);
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
 * dfd_get_board_eeprom_type - Used to get board eeprom type, including slots eeprom
 *
 * This function returns the type of board eeprom, including slots eeprom
 * otherwise it returns a negative value on failed.
 */
static ssize_t dfd_get_board_eeprom_type(unsigned int e2_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_eeprom_type(WB_MAIN_DEV_MAINBOARD, e2_index, buf, count);
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
    if (ret == -DFD_RV_DEV_NOTSUPPORT) {
        return -WB_SYSFS_RV_UNSUPPORT;
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


static struct switch_drivers_s switch_drivers = {
    /*
     * set odm switch drivers,
     * if not support the function, set corresponding hook to NULL.
     */
    /* temperature sensors */
    .get_main_board_temp_number = dfd_get_main_board_temp_number,
    .get_main_board_temp_alias = dfd_get_main_board_temp_alias,
    .get_main_board_temp_type = dfd_get_main_board_temp_type,
    .get_main_board_temp_max = dfd_get_main_board_temp_max,
    .get_main_board_temp_min = dfd_get_main_board_temp_min,
    .get_main_board_temp_value = dfd_get_main_board_temp_value,
    .get_main_board_temp_high = dfd_get_main_board_temp_high,
    .get_main_board_temp_low = dfd_get_main_board_temp_low,
    .get_main_board_temp_monitor_flag = dfd_get_main_board_temp_monitor_flag,
    /* voltage sensors */
    .get_main_board_vol_number = dfd_get_main_board_vol_number,
    .get_main_board_vol_alias = dfd_get_main_board_vol_alias,
    .get_main_board_vol_type = dfd_get_main_board_vol_type,
    .get_main_board_vol_max = dfd_get_main_board_vol_max,
    .get_main_board_vol_min = dfd_get_main_board_vol_min,
    .get_main_board_vol_range = dfd_get_main_board_vol_range,
    .get_main_board_vol_nominal_value = dfd_get_main_board_vol_nominal_value,
    .get_main_board_vol_value = dfd_get_main_board_vol_value,
    .get_main_board_vol_monitor_flag = dfd_get_main_board_vol_monitor_flag,
    /* current sensors */
    .get_main_board_curr_number = dfd_get_main_board_curr_number,
    .get_main_board_curr_alias = dfd_get_main_board_curr_alias,
    .get_main_board_curr_type = dfd_get_main_board_curr_type,
    .get_main_board_curr_max = dfd_get_main_board_curr_max,
    .get_main_board_curr_min = dfd_get_main_board_curr_min,
    .get_main_board_curr_value = dfd_get_main_board_curr_value,
    .get_main_board_curr_monitor_flag = dfd_get_main_board_curr_monitor_flag,
    /* syseeprom */
    .get_syseeprom_size = dfd_get_syseeprom_size,
    .read_syseeprom_data = dfd_read_syseeprom_data,
    .write_syseeprom_data = dfd_write_syseeprom_data,
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
    .get_fan_ratio = dfd_get_fan_ratio,
    .set_fan_ratio = dfd_set_fan_ratio,
    /* psu */
    .get_psu_number = dfd_get_psu_number,
    .get_psu_temp_number = dfd_get_psu_temp_number,
    .get_psu_model_name = dfd_get_psu_model_name,
    .get_psu_vendor = dfd_get_psu_vendor,
    .get_psu_date = dfd_get_psu_date,
    .get_psu_status = dfd_get_psu_status,
    .get_psu_hw_status = dfd_get_psu_hw_status,
    .get_psu_alarm = dfd_get_psu_alarm,
    .get_psu_serial_number = dfd_get_psu_serial_number,
    .get_psu_part_number = dfd_get_psu_part_number,
    .get_psu_hardware_version = dfd_get_psu_hardware_version,
    .get_psu_type = dfd_get_psu_type,
    .get_psu_in_curr = dfd_get_psu_in_curr,
    .get_psu_in_vol = dfd_get_psu_in_vol,
    .get_psu_in_power = dfd_get_psu_in_power,
    .get_psu_out_curr = dfd_get_psu_out_curr,
    .get_psu_out_vol = dfd_get_psu_out_vol,
    .get_psu_out_power = dfd_get_psu_out_power,
    .get_psu_out_max_power = dfd_get_psu_out_max_power,
    .get_psu_present_status = dfd_get_psu_present,
    .get_psu_in_status = dfd_get_psu_in_status,
    .get_psu_out_status = dfd_get_psu_out_status,
    .get_psu_status_pmbus = dfd_get_psu_status_pmbus,
    .get_psu_fan_speed = dfd_get_psu_fan_speed,
    .get_psu_fan_ratio = dfd_get_psu_fan_ratio,
    .set_psu_fan_ratio = dfd_set_psu_fan_ratio,
    .get_psu_fan_direction = dfd_get_psu_fan_direction,
    .get_psu_led_status = dfd_get_psu_led_status,
    .get_psu_temp_alias = dfd_get_psu_temp_alias,
    .get_psu_temp_type = dfd_get_psu_temp_type,
    .get_psu_temp_max = dfd_get_psu_temp_max,
    .set_psu_temp_max = dfd_set_psu_temp_max,
    .get_psu_temp_min = dfd_get_psu_temp_min,
    .set_psu_temp_min = dfd_set_psu_temp_min,
    .get_psu_temp_value = dfd_get_psu_temp_value,
    .get_psu_fan_speed_cal = dfd_get_psu_fan_speed_cal,
    .get_psu_attr_threshold = dfd_get_psu_attr_threshold,
    .get_psu_eeprom_size = dfd_get_psu_eeprom_size,
    .read_psu_eeprom_data = dfd_read_psu_eeprom_data,
    .get_psu_blackbox_info = dfd_get_psu_blackbox_info,
    .get_psu_pmbus_info = dfd_get_psu_pmbus_info,
    .clear_psu_blackbox = dfd_clear_psu_blackbox_info,
    /* transceiver */
    .get_eth_number = dfd_get_eth_number,
    .get_transceiver_power_on_status = dfd_get_transceiver_power_on_status,
    .set_transceiver_power_on_status = dfd_set_transceiver_power_on_status,
    .get_eth_power_on_status = dfd_get_eth_power_on_status,
    .set_eth_power_on_status = dfd_set_eth_power_on_status,
    .get_eth_tx_fault_status = dfd_get_eth_tx_fault_status,
    .get_eth_tx_disable_status = dfd_get_eth_tx_disable_status,
    .set_eth_tx_disable_status = dfd_set_eth_tx_disable_status,
    .get_transceiver_present_status = dfd_get_transceiver_present_status,
    .get_eth_present_status = dfd_get_eth_present_status,
    .get_eth_rx_los_status = dfd_get_eth_rx_los_status,
    .get_eth_reset_status = dfd_get_eth_reset_status,
    .set_eth_reset_status = dfd_set_eth_reset_status,
    .get_eth_low_power_mode_status = dfd_get_eth_low_power_mode_status,
    .get_eth_interrupt_status = dfd_get_eth_interrupt_status,
    .get_eth_eeprom_size = dfd_get_eth_eeprom_size,
    .read_eth_eeprom_data = dfd_read_eth_eeprom_data,
    .write_eth_eeprom_data = dfd_write_eth_eeprom_data,
    .get_eth_optoe_type = dfd_get_eth_optoe_type,
    .set_eth_optoe_type = dfd_set_eth_optoe_type,
    /* sysled */
    .get_sys_led_status = dfd_get_sys_led_status,
    .set_sys_led_status = dfd_set_sys_led_status,
    .get_bmc_led_status = dfd_get_bmc_led_status,
    .set_bmc_led_status = dfd_set_bmc_led_status,
    .get_sys_fan_led_status = dfd_get_sys_fan_led_status,
    .set_sys_fan_led_status = dfd_set_sys_fan_led_status,
    .get_sys_psu_led_status = dfd_get_sys_psu_led_status,
    .set_sys_psu_led_status = dfd_set_sys_psu_led_status,
    .get_id_led_status = dfd_get_id_led_status,
    .set_id_led_status = dfd_set_id_led_status,
    /* FPGA */
    .get_main_board_fpga_number = dfd_get_main_board_fpga_number,
    .get_main_board_fpga_alias = dfd_get_main_board_fpga_alias,
    .get_main_board_fpga_type = dfd_get_main_board_fpga_type,
    .get_main_board_fpga_firmware_version = dfd_get_main_board_fpga_firmware_version,
    .get_main_board_fpga_board_version = dfd_get_main_board_fpga_board_version,
    .get_main_board_fpga_test_reg = dfd_get_main_board_fpga_test_reg,
    .set_main_board_fpga_test_reg = dfd_set_main_board_fpga_test_reg,
    /* CPLD */
    .get_main_board_cpld_number = dfd_get_main_board_cpld_number,
    .get_main_board_cpld_alias = dfd_get_main_board_cpld_alias,
    .get_main_board_cpld_type = dfd_get_main_board_cpld_type,
    .get_main_board_cpld_firmware_version = dfd_get_main_board_cpld_firmware_version,
    .get_main_board_cpld_board_version = dfd_get_main_board_cpld_board_version,
    .get_main_board_cpld_test_reg = dfd_get_main_board_cpld_test_reg,
    .set_main_board_cpld_test_reg = dfd_set_main_board_cpld_test_reg,
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
    .get_slot_status = dfd_get_slot_status,
    .get_slot_led_status = dfd_get_slot_led_status,
    .set_slot_led_status = dfd_set_slot_led_status,
    .get_slot_power_status = dfd_get_slot_power_status,
    .set_slot_power_status = dfd_set_slot_power_status,
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
    .get_system_port_power_status = dfd_get_system_port_power_status,
    .set_system_value = dfd_set_system_value,
    /* eeprom */
    .get_eeprom_number = dfd_get_eeprom_number,
    .get_eeprom_size = dfd_get_board_eeprom_size,
    .get_eeprom_alias = dfd_get_board_eeprom_alias,
    .get_eeprom_tag = dfd_get_board_eeprom_tag,
    .get_eeprom_type = dfd_get_board_eeprom_type,
    .read_eeprom_data = dfd_read_board_eeprom_data,
    .write_eeprom_data = dfd_write_board_eeprom_data,
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
