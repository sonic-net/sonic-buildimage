/*
 * An psu_device_driver driver for psu devcie function
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


#include <linux/slab.h>

#include "device_driver_common.h"
#include "psu_sysfs.h"
#include "dfd_sysfs_common.h"

#define PSU_INFO(fmt, args...) LOG_INFO("psu: ", fmt, ##args)
#define PSU_ERR(fmt, args...)  LOG_ERR("psu: ", fmt, ##args)
#define PSU_DBG(fmt, args...)  LOG_DBG("psu: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/********************************************psu**********************************************/
static int wb_get_psu_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_number);

    ret = g_drv->get_psu_number();
    return ret;
}

static int wb_get_psu_temp_number(unsigned int psu_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_number);

    ret = g_drv->get_psu_temp_number(psu_index);
    return ret;
}

/*
 * wb_get_psu_model_name - Used to get psu model name,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_model_name(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_model_name);

    ret = g_drv->get_psu_model_name(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_vendor - Used to get psu model name,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_vendor(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_vendor);

    ret = g_drv->get_psu_vendor(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_date - Used to get psu date,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_date(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_date);

    ret = g_drv->get_psu_date(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_status - Used to get psu status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_status);

    ret = g_drv->get_psu_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_hw_status - Used to get psu status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_hw_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_hw_status);

    ret = g_drv->get_psu_hw_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_alarm - Used to get psu alarm status,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_alarm(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_alarm);

    ret = g_drv->get_psu_alarm(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_serial_number - Used to get psu serial number,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_serial_number(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_serial_number);

    ret = g_drv->get_psu_serial_number(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_part_number - Used to get psu part number,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_part_number(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_part_number);

    ret = g_drv->get_psu_part_number(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_hardware_version - Used to get psu hardware version,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_hardware_version(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_hardware_version);

    ret = g_drv->get_psu_hardware_version(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_type - Used to get the input type of psu
 * filled the value to buf, input type value define as below:
 * 0: DC
 * 1: AC
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_type(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_type);

    ret = g_drv->get_psu_type(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_in_curr - Used to get the input current of psu
 * filled the value to buf, the value is integer with mA
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_in_curr(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_in_curr);

    ret = g_drv->get_psu_in_curr(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_in_vol - Used to get the input voltage of psu
 * filled the value to bu, the value is integer with mV
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_in_vol(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_in_vol);

    ret = g_drv->get_psu_in_vol(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_in_power - Used to get the input power of psu
 * filled the value to buf, the value is integer with uW
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_in_power(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_in_power);

    ret = g_drv->get_psu_in_power(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_out_curr - Used to get the output current of psu
 * filled the value to buf, the value is integer with mA
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_out_curr(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_out_curr);

    ret = g_drv->get_psu_out_curr(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_out_vol - Used to get the output voltage of psu
 * filled the value to buf, the value is integer with mV
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_out_vol(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_out_vol);

    ret = g_drv->get_psu_out_vol(psu_index, buf, count);
    return ret;
}

static ssize_t wb_get_psu_attr_threshold(unsigned int psu_index, unsigned int type, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_attr_threshold);

    ret = g_drv->get_psu_attr_threshold(psu_index, type, buf, count);
    return ret;
}

/*
 * wb_get_psu_out_power - Used to get the output power of psu
 * filled the value to buf, the value is integer with uW
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_out_power(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_out_power);

    ret = g_drv->get_psu_out_power(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_out_max_power - Used to get the output max power of psu
 * filled the value to buf, the value is integer with uW
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_out_max_power(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_out_max_power);

    ret = g_drv->get_psu_out_max_power(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_present_status - Used to get psu present status
 * filled the value to buf, psu present status define as below:
 * 0: ABSENT
 * 1: PRESENT
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_present_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_present_status);

    ret = g_drv->get_psu_present_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_in_status - Used to get psu input status
 * filled the value to buf, psu input status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_in_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_in_status);

    ret = g_drv->get_psu_in_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_status_pmbus - Used to get psu status from pmbus
 * filled the value to buf, psu output status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_status_pmbus(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_status_pmbus);

    ret = g_drv->get_psu_status_pmbus(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_out_status - Used to get psu output status
 * filled the value to buf, psu output status define as below:
 * 0: NOT OK
 * 1: OK
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_out_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_out_status);

    ret = g_drv->get_psu_out_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_fan_speed_cal - Used to get psu fan speed cal
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_fan_speed_cal(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_fan_speed_cal);

    ret = g_drv->get_psu_fan_speed_cal(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_fan_speed - Used to get psu fan speed
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_fan_speed(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_fan_speed);

    ret = g_drv->get_psu_fan_speed(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_fan_ratio - Used to get the ratio of psu fan
 * filled the value to buf
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_fan_ratio(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_fan_ratio);

    ret = g_drv->get_psu_fan_ratio(psu_index, buf, count);
    return ret;
}

/*
 * wb_set_psu_fan_ratio - Used to set the ratio of psu fan
 * @psu_index: start with 1
 * @ratio: from 0 to 100
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_psu_fan_ratio(unsigned int psu_index, int ratio)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_psu_fan_ratio);

    ret = g_drv->set_psu_fan_ratio(psu_index, ratio);
    return ret;
}

/*
 * wb_get_psu_fan_direction - Used to get psu air flow direction,
 * filled the value to buf, air flow direction define as below:
 * 0: F2B
 * 1: B2F
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_fan_direction(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_fan_direction);

    ret = g_drv->get_psu_fan_direction(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_led_status - Used to get psu led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4: blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8: blue light flashing
 *
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_led_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_led_status);

    ret = g_drv->get_psu_led_status(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_temp_alias - Used to identify the location of the temperature sensor of psu,
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_temp_alias(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_alias);

    ret = g_drv->get_psu_temp_alias(psu_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_temp_type - Used to get the model of temperature sensor of psu,
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_temp_type(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_type);

    ret = g_drv->get_psu_temp_type(psu_index, temp_index, buf, count);
    return ret;

}

/*
 * wb_get_psu_temp_max - Used to get the maximum threshold of temperature sensor of psu,
 * filled the value to buf, the value is integer with millidegree Celsius
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_max);

    ret = g_drv->get_psu_temp_max(psu_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_set_psu_temp_max - Used to set the maximum threshold of temperature sensor of psu,
 * get value from buf and set it to maximum threshold of psu temperature sensor
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_psu_temp_max);

    ret = g_drv->set_psu_temp_max(psu_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_temp_min - Used to get the minimum threshold of temperature sensor of psu,
 * filled the value to buf, the value is integer with millidegree Celsius
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_min);

    ret = g_drv->get_psu_temp_min(psu_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_set_psu_temp_min - Used to set the minimum threshold of temperature sensor of psu,
 * get value from buf and set it to minimum threshold of psu temperature sensor
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->set_psu_temp_min);

    ret = g_drv->set_psu_temp_min(psu_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_temp_value - Used to get the input value of temperature sensor of psu
 * filled the value to buf, the value is integer with millidegree Celsius
 * @psu_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_temp_value(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_temp_value);

    ret = g_drv->get_psu_temp_value(psu_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_eeprom_size - Used to get psu eeprom size
 *
 * This function returns the size of port eeprom,
 * otherwise it returns a negative value on failed.
 */
static int wb_get_psu_eeprom_size(unsigned int psu_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_eeprom_size);

    ret = g_drv->get_psu_eeprom_size(psu_index);
    return ret;
}

/*
 * wb_read_psu_eeprom_data - Used to read psu eeprom data,
 * @buf: Data read buffer
 * @offset: offset address to read psu eeprom data
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * returns 0 means EOF,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_read_psu_eeprom_data(unsigned int psu_index, char *buf, loff_t offset,
                   size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->read_psu_eeprom_data);

    ret = g_drv->read_psu_eeprom_data(psu_index, buf, offset, count);
    return ret;
}

/*
 * wb_get_psu_blackbox_info - Used to get psu blackbox information,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_blackbox_info(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_blackbox_info);

    ret = g_drv->get_psu_blackbox_info(psu_index, buf, count);
    return ret;
}

/*
 * wb_get_psu_pmbus_info - Used to get psu pmbus information,
 * @psu_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_psu_pmbus_info(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_psu_pmbus_info);

    ret = g_drv->get_psu_pmbus_info(psu_index, buf, count);
    return ret;
}

/*
 * wb_clear_psu_blackbox - Used to clear psu blackbox information
 * @psu_index: start with 1
 * @value: 1
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_clear_psu_blackbox(unsigned int psu_index, uint8_t value)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->clear_psu_blackbox);

    ret = g_drv->clear_psu_blackbox(psu_index, value);
    return ret;
}

/****************************************end of psu*******************************************/

static struct s3ip_sysfs_psu_drivers_s drivers = {
    /*
     * set ODM psu drivers to /sys/s3ip/psu,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_psu_number = wb_get_psu_number,
    .get_psu_temp_number = wb_get_psu_temp_number,
    .get_psu_model_name = wb_get_psu_model_name,
    .get_psu_vendor = wb_get_psu_vendor,
    .get_psu_date = wb_get_psu_date,
    .get_psu_status = wb_get_psu_status,
    .get_psu_hw_status = wb_get_psu_hw_status,
    .get_psu_alarm = wb_get_psu_alarm,
    .get_psu_serial_number = wb_get_psu_serial_number,
    .get_psu_part_number = wb_get_psu_part_number,
    .get_psu_hardware_version = wb_get_psu_hardware_version,
    .get_psu_type = wb_get_psu_type,
    .get_psu_in_curr = wb_get_psu_in_curr,
    .get_psu_in_vol = wb_get_psu_in_vol,
    .get_psu_in_power = wb_get_psu_in_power,
    .get_psu_out_curr = wb_get_psu_out_curr,
    .get_psu_out_vol = wb_get_psu_out_vol,
    .get_psu_out_power = wb_get_psu_out_power,
    .get_psu_out_max_power = wb_get_psu_out_max_power,
    .get_psu_present_status = wb_get_psu_present_status,
    .get_psu_status_pmbus = wb_get_psu_status_pmbus,
    .get_psu_in_status = wb_get_psu_in_status,
    .get_psu_out_status = wb_get_psu_out_status,
    .get_psu_fan_speed = wb_get_psu_fan_speed,
    .get_psu_fan_ratio = wb_get_psu_fan_ratio,
    .set_psu_fan_ratio = wb_set_psu_fan_ratio,
    .get_psu_fan_direction = wb_get_psu_fan_direction,
    .get_psu_led_status = wb_get_psu_led_status,
    .get_psu_temp_alias = wb_get_psu_temp_alias,
    .get_psu_temp_type = wb_get_psu_temp_type,
    .get_psu_temp_max = wb_get_psu_temp_max,
    .set_psu_temp_max = wb_set_psu_temp_max,
    .get_psu_temp_min = wb_get_psu_temp_min,
    .set_psu_temp_min = wb_set_psu_temp_min,
    .get_psu_temp_value = wb_get_psu_temp_value,
    .get_psu_fan_speed_cal = wb_get_psu_fan_speed_cal,
    .get_psu_attr_threshold = wb_get_psu_attr_threshold,
    .get_psu_eeprom_size = wb_get_psu_eeprom_size,
    .read_psu_eeprom_data = wb_read_psu_eeprom_data,
    .get_psu_blackbox_info = wb_get_psu_blackbox_info,
    .get_psu_pmbus_info = wb_get_psu_pmbus_info,
    .clear_psu_blackbox = wb_clear_psu_blackbox,
};

static int __init psu_dev_drv_init(void)
{
    int ret;

    PSU_INFO("psu_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_psu_drivers_register(&drivers);
    if (ret < 0) {
        PSU_ERR("psu drivers register err, ret %d.\n", ret);
        return ret;
    }
    PSU_INFO("psu_init success.\n");
    return 0;
}

static void __exit psu_dev_drv_exit(void)
{
    s3ip_sysfs_psu_drivers_unregister();
    PSU_INFO("psu_exit ok.\n");

    return;
}

module_init(psu_dev_drv_init);
module_exit(psu_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("psu device driver");
