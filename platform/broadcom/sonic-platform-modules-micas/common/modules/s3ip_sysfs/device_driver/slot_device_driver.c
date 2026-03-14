/*
 * An slot_device_driver driver for slot devcie function
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
#include "slot_sysfs.h"
#include "dfd_sysfs_common.h"

#define SLOT_INFO(fmt, args...) LOG_INFO("slot: ", fmt, ##args)
#define SLOT_ERR(fmt, args...)  LOG_ERR("slot: ", fmt, ##args)
#define SLOT_DBG(fmt, args...)  LOG_DBG("slot: ", fmt, ##args)

static int g_loglevel = 0;
static struct switch_drivers_s *g_drv = NULL;

/******************************************slot***********************************************/
static int wb_get_slot_number(void)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_number);

    ret = g_drv->get_slot_number();
    return ret;
}

static int wb_get_slot_temp_number(unsigned int slot_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_temp_number);

    ret = g_drv->get_slot_temp_number(slot_index);
    return ret;
}

static int wb_get_slot_vol_number(unsigned int slot_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_number);

    ret = g_drv->get_slot_vol_number(slot_index);
    return ret;
}

static int wb_get_slot_curr_number(unsigned int slot_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_curr_number);

    ret = g_drv->get_slot_curr_number(slot_index);
    return ret;
}

static int wb_get_slot_fpga_number(unsigned int slot_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_fpga_number);

    ret = g_drv->get_slot_fpga_number(slot_index);
    return ret;
}

static int wb_get_slot_cpld_number(unsigned int slot_index)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_cpld_number);

    ret = g_drv->get_slot_cpld_number(slot_index);
    return ret;
}

/*
 * wb_get_slot_model_name - Used to get slot model name,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_model_name(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_model_name);

    ret = g_drv->get_slot_model_name(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vendor - Used to get slot model name,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vendor(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vendor);

    ret = g_drv->get_slot_vendor(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_serial_number - Used to get slot serial number,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_serial_number(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_serial_number);

    ret = g_drv->get_slot_serial_number(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_part_number - Used to get slot part number,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_part_number(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_part_number);

    ret = g_drv->get_slot_part_number(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_hardware_version - Used to get slot hardware version,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_hardware_version(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_hardware_version);

    ret = g_drv->get_slot_hardware_version(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_status - Used to get slot status,
 * filled the value to buf, slot status define as below:
 * 0: ABSENT
 * 1: OK
 * 2: NOT OK
 *
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_status);

    ret = g_drv->get_slot_status(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_led_status - Used to get slot led status
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
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_led_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_led_status);

    ret = g_drv->get_slot_led_status(slot_index, buf, count);
    return ret;
}

/*
 * wb_set_slot_led_status - Used to set slot led status
 * @slot_index: start with 1
 * @status: led status, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4:blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8:blue light flashing
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_slot_led_status(unsigned int slot_index, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_slot_led_status);

    ret = g_drv->set_slot_led_status(slot_index, status);
    return ret;
}

/*
 * wb_set_slot_power_status - Used to set slot power status,
 * filled the value to buf, slot status define as below:
 * 0: OFF
 * 1: ON
 *
 * @slot_index: start with 1
 * @status: power status
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_slot_power_status(unsigned int slot_index, int status)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_slot_power_status);

    ret = g_drv->set_slot_power_status(slot_index, status);
    return ret;
}

/*
 * wb_get_slot_power_status - Used to get slot power status,
 * filled the value to buf, slot status define as below:
 * 0: OFF
 * 1: ON
 *
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_power_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_power_status);

    ret = g_drv->get_slot_power_status(slot_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_temp_alias - Used to identify the location of the temperature sensor of slot,
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_temp_alias(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_temp_alias);

    ret = g_drv->get_slot_temp_alias(slot_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_temp_type - Used to get the model of temperature sensor of slot,
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_temp_type(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_temp_type);

    ret = g_drv->get_slot_temp_type(slot_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_temp_max - Used to get the maximum threshold of temperature sensor of slot,
 * filled the value to buf, the value is integer with millidegree Celsius
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_temp_max(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_temp_max);

    ret = g_drv->get_slot_temp_max(slot_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_temp_min - Used to get the minimum threshold of temperature sensor of slot,
 * filled the value to buf, the value is integer with millidegree Celsius
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_temp_min(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_temp_min);

    ret = g_drv->get_slot_temp_min(slot_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_temp_value - Used to get the input value of temperature sensor of slot,
 * filled the value to buf, the value is integer with millidegree Celsius
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_temp_value(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_temp_value);

    ret = g_drv->get_slot_temp_value(slot_index, temp_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_alias - Used to identify the location of the voltage sensor of slot,
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_alias(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_alias);

    ret = g_drv->get_slot_vol_alias(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_type - Used to get the model of voltage sensor of slot,
 * such as udc90160, tps53622 and so on
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_type(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_type);

    ret = g_drv->get_slot_vol_type(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_max - Used to get the maximum threshold of voltage sensor of slot,
 * filled the value to buf, the value is integer with mV
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_max(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_max);

    ret = g_drv->get_slot_vol_max(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_min - Used to get the minimum threshold of voltage sensor of slot,
 * filled the value to buf, the value is integer with mV
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_min(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_min);

    ret = g_drv->get_slot_vol_min(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_range - Used to get the output error value of voltage sensor of slot,
 * filled the value to buf
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_range(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_range);

    ret = g_drv->get_slot_vol_range(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_nominal_value - Used to get the nominal value of voltage sensor of slot,
 * filled the value to buf
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_nominal_value(unsigned int slot_index,
                   unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_nominal_value);

    ret = g_drv->get_slot_vol_nominal_value(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_vol_value - Used to get the input value of voltage sensor of slot,
 * filled the value to buf, the value is integer with mV
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_vol_value(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_vol_value);

    ret = g_drv->get_slot_vol_value(slot_index, vol_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_curr_alias - Used to identify the location of the current sensor of slot,
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_curr_alias(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_curr_alias);

    ret = g_drv->get_slot_curr_alias(slot_index, curr_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_curr_type - Used to get the model of current sensor of slot,
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_curr_type(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_curr_type);

    ret = g_drv->get_slot_curr_type(slot_index, curr_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_curr_max - Used to get the maximum threshold of current sensor of slot,
 * filled the value to buf, the value is integer with mA
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_curr_max(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_curr_max);

    ret = g_drv->get_slot_curr_max(slot_index, curr_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_curr_min - Used to get the minimum threshold of current sensor of slot,
 * filled the value to buf, the value is integer with mA
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_curr_min(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_curr_min);

    ret = g_drv->get_slot_curr_min(slot_index, curr_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_curr_value - Used to get the input value of current sensor of slot,
 * filled the value to buf, the value is integer with mA
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_curr_value(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_curr_value);

    ret = g_drv->get_slot_curr_value(slot_index, curr_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_fpga_alias - Used to identify the location of slot fpga,
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_fpga_alias(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_fpga_alias);

    ret = g_drv->get_slot_fpga_alias(slot_index, fpga_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_fpga_type - Used to get slot fpga model name
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_fpga_type(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_fpga_type);

    ret = g_drv->get_slot_fpga_type(slot_index, fpga_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_fpga_firmware_version - Used to get slot fpga firmware version,
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_fpga_firmware_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_fpga_firmware_version);

    ret = g_drv->get_slot_fpga_firmware_version(slot_index, fpga_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_fpga_board_version - Used to get slot fpga board version,
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_fpga_board_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_fpga_board_version);

    ret = g_drv->get_slot_fpga_board_version(slot_index, fpga_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_fpga_test_reg - Used to test slot fpga register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_fpga_test_reg);

    ret = g_drv->get_slot_fpga_test_reg(slot_index, fpga_index, buf, count);
    return ret;
}

/*
 * wb_set_slot_fpga_test_reg - Used to test slot fpga register write
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @value: value write to slot fpga
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
           unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_slot_fpga_test_reg);

    ret = g_drv->set_slot_fpga_test_reg(slot_index, fpga_index, value);
    return ret;
}

/*
 * wb_get_slot_cpld_alias - Used to identify the location of slot cpld,
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_cpld_alias(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_cpld_alias);

    ret = g_drv->get_slot_cpld_alias(slot_index, cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_cpld_type - Used to get slot cpld model name
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_cpld_type(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_cpld_type);

    ret = g_drv->get_slot_cpld_type(slot_index, cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_cpld_firmware_version - Used to get slot cpld firmware version,
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_cpld_firmware_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_cpld_firmware_version);

    ret = g_drv->get_slot_cpld_firmware_version(slot_index, cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_cpld_board_version - Used to get slot cpld board version,
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_cpld_board_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_cpld_board_version);

    ret = g_drv->get_slot_cpld_board_version(slot_index, cpld_index, buf, count);
    return ret;
}

/*
 * wb_get_slot_cpld_test_reg - Used to test slot cpld register read
 * filled the value to buf, value is hexadecimal, start with 0x
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t wb_get_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    check_p(g_drv);
    check_p(g_drv->get_slot_cpld_test_reg);

    ret = g_drv->get_slot_cpld_test_reg(slot_index, cpld_index, buf, count);
    return ret;
}

/*
 * wb_set_slot_cpld_test_reg - Used to test slot cpld register write
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @value: value write to slot cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int wb_set_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
           unsigned int value)
{
    int ret;

    check_p(g_drv);
    check_p(g_drv->set_slot_cpld_test_reg);

    ret = g_drv->set_slot_cpld_test_reg(slot_index, cpld_index, value);
    return ret;
}
/***************************************end of slot*******************************************/

static struct s3ip_sysfs_slot_drivers_s drivers = {
    /*
     * set ODM slot drivers to /sys/s3ip/slot,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_slot_number = wb_get_slot_number,
    .get_slot_temp_number = wb_get_slot_temp_number,
    .get_slot_vol_number = wb_get_slot_vol_number,
    .get_slot_curr_number = wb_get_slot_curr_number,
    .get_slot_cpld_number = wb_get_slot_cpld_number,
    .get_slot_fpga_number = wb_get_slot_fpga_number,
    .get_slot_model_name = wb_get_slot_model_name,
    .get_slot_vendor = wb_get_slot_vendor,
    .get_slot_serial_number = wb_get_slot_serial_number,
    .get_slot_part_number = wb_get_slot_part_number,
    .get_slot_hardware_version = wb_get_slot_hardware_version,
    .get_slot_status = wb_get_slot_status,
    .get_slot_led_status = wb_get_slot_led_status,
    .set_slot_led_status = wb_set_slot_led_status,
    .get_slot_power_status = wb_get_slot_power_status,
    .set_slot_power_status = wb_set_slot_power_status,
    .get_slot_temp_alias = wb_get_slot_temp_alias,
    .get_slot_temp_type = wb_get_slot_temp_type,
    .get_slot_temp_max = wb_get_slot_temp_max,
    .get_slot_temp_min = wb_get_slot_temp_min,
    .get_slot_temp_value = wb_get_slot_temp_value,
    .get_slot_vol_alias = wb_get_slot_vol_alias,
    .get_slot_vol_type = wb_get_slot_vol_type,
    .get_slot_vol_max = wb_get_slot_vol_max,
    .get_slot_vol_min = wb_get_slot_vol_min,
    .get_slot_vol_range = wb_get_slot_vol_range,
    .get_slot_vol_nominal_value = wb_get_slot_vol_nominal_value,
    .get_slot_vol_value = wb_get_slot_vol_value,
    .get_slot_curr_alias = wb_get_slot_curr_alias,
    .get_slot_curr_type = wb_get_slot_curr_type,
    .get_slot_curr_max = wb_get_slot_curr_max,
    .get_slot_curr_min = wb_get_slot_curr_min,
    .get_slot_curr_value = wb_get_slot_curr_value,
    .get_slot_fpga_alias = wb_get_slot_fpga_alias,
    .get_slot_fpga_alias = wb_get_slot_fpga_alias,
    .get_slot_fpga_type = wb_get_slot_fpga_type,
    .get_slot_fpga_firmware_version = wb_get_slot_fpga_firmware_version,
    .get_slot_fpga_board_version = wb_get_slot_fpga_board_version,
    .get_slot_fpga_test_reg = wb_get_slot_fpga_test_reg,
    .set_slot_fpga_test_reg = wb_set_slot_fpga_test_reg,
    .get_slot_cpld_alias = wb_get_slot_cpld_alias,
    .get_slot_cpld_type = wb_get_slot_cpld_type,
    .get_slot_cpld_firmware_version = wb_get_slot_cpld_firmware_version,
    .get_slot_cpld_board_version = wb_get_slot_cpld_board_version,
    .get_slot_cpld_test_reg = wb_get_slot_cpld_test_reg,
    .set_slot_cpld_test_reg = wb_set_slot_cpld_test_reg,
};

static int __init slot_dev_drv_init(void)
{
    int ret;

    SLOT_INFO("slot_init...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);

    ret = s3ip_sysfs_slot_drivers_register(&drivers);
    if (ret < 0) {
        SLOT_ERR("slot drivers register err, ret %d.\n", ret);
        return ret;
    }
    SLOT_INFO("slot_init success.\n");
    return 0;
}

static void __exit slot_dev_drv_exit(void)
{
    s3ip_sysfs_slot_drivers_unregister();
    SLOT_INFO("slot_exit success.\n");
    return;
}

module_init(slot_dev_drv_init);
module_exit(slot_dev_drv_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_DESCRIPTION("slot device driver");
