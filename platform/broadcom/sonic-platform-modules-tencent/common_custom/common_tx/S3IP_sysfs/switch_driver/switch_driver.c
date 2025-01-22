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
#include "rg_module.h"
#include "rg_fan_driver.h"
#include "rg_eeprom_driver.h"
#include "rg_cpld_driver.h"
#include "rg_fpga_driver.h"
#include "rg_led_driver.h"
#include "rg_slot_driver.h"
#include "rg_sensors_driver.h"
#include "rg_psu_driver.h"
#include "rg_sff_driver.h"
#include "rg_watchdog_driver.h"

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

    ret = dfd_get_dev_number(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_TEMP);
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

    ret = dfd_get_temp_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, temp_index, RG_SENSOR_ALIAS,
              buf, count);
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

    ret = dfd_get_temp_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, temp_index, RG_SENSOR_TYPE,
              buf, count);
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

    ret = dfd_get_temp_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, temp_index, RG_SENSOR_MAX,
              buf, count);
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

    ret = dfd_get_temp_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, temp_index, RG_SENSOR_MIN,
              buf, count);
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

    ret = dfd_get_temp_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, temp_index, RG_SENSOR_INPUT,
              buf, count);
    return ret;
}
/***********************************end of main board temp*************************************/

/*************************************main board voltage***************************************/
static int dfd_get_main_board_vol_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_IN);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index, RG_SENSOR_ALIAS,
              buf, count);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index, RG_SENSOR_TYPE,
              buf, count);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index, RG_SENSOR_MAX,
              buf, count);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index, RG_SENSOR_MIN,
              buf, count);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index,
              RG_SENSOR_RANGE, buf, count);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index,
              RG_SENSOR_NOMINAL_VAL, buf, count);
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

    ret = dfd_get_voltage_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, vol_index, RG_SENSOR_INPUT,
              buf, count);
    return ret;
}
/*********************************end of main board voltage************************************/
/*************************************main board current***************************************/
static int dfd_get_main_board_curr_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_CURR);
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

    ret = dfd_get_current_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, curr_index, RG_SENSOR_ALIAS,
              buf, count);
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

    ret = dfd_get_current_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, curr_index, RG_SENSOR_TYPE,
              buf, count);
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

    ret = dfd_get_current_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, curr_index, RG_SENSOR_MAX,
              buf, count);
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

    ret = dfd_get_current_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, curr_index, RG_SENSOR_MIN,
              buf, count);
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

    ret = dfd_get_current_info(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_NONE, curr_index, RG_SENSOR_INPUT,
              buf, count);
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

    ret = dfd_get_eeprom_size(RG_MAIN_DEV_MAINBOARD, 0);
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

    ret = dfd_read_eeprom_data(RG_MAIN_DEV_MAINBOARD, 0, buf, offset, count);
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

    ret = dfd_write_eeprom_data(RG_MAIN_DEV_MAINBOARD, 0, buf, offset, count);
    return ret;
}
/*************************************end of syseeprom****************************************/

/********************************************fan**********************************************/
static int dfd_get_fan_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_FAN, RG_MINOR_DEV_NONE);
    return ret;
}

static int dfd_get_fan_motor_number(unsigned int fan_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_FAN, RG_MINOR_DEV_MOTOR);
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

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_NAME, buf, count);
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

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_SN, buf, count);
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

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count);
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

    ret = dfd_get_fan_info(fan_index, DFD_DEV_INFO_TYPE_HW_INFO, buf, count);
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

    ret = dfd_get_led_status(RG_FAN_LED_MODULE, fan_index, buf, count);
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

    ret = dfd_set_led_status(RG_FAN_LED_MODULE, fan_index, status);
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

    ret = dfd_get_fan_direction_str(fan_index, buf, count);
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

    ret = dfd_get_fan_motor_speed_tolerance_str(fan_index, motor_index, buf, count);
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

    ret = dfd_get_fan_motor_speed_target_str(fan_index, motor_index, buf, count);
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

    ret = dfd_get_fan_motor_speed_max_str(fan_index, motor_index, buf, count);
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

    ret = dfd_get_fan_motor_speed_min_str(fan_index, motor_index, buf, count);
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
    return ret;
}
/****************************************end of fan*******************************************/
/********************************************psu**********************************************/
static int dfd_get_psu_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_PSU, RG_MINOR_DEV_NONE);
    return ret;
}

static int dfd_get_psu_temp_number(unsigned int psu_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_PSU, RG_MINOR_DEV_TEMP);
    return ret;
}

/* Similar to dfd_get_psu_model_name */
static ssize_t dfd_get_psu_model_name(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_PART_NAME, buf, count);
    return ret;
}

/* Similar to rg_get_fan_serial_number */
static ssize_t dfd_get_psu_serial_number(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_SN, buf, count);
    return ret;
}

/* Similar to rg_get_fan_part_number */
static ssize_t dfd_get_psu_part_number(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count);
    return ret;
}

/* Similar to rg_get_fan_hardware_version */
static ssize_t dfd_get_psu_hardware_version(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_HW_INFO, buf, count);
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

    ret = dfd_get_psu_input_type(psu_index, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_IN_CURR, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_IN_VOL, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_IN_POWER, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_OUT_CURR, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_OUT_VOL, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_OUT_POWER, buf, count);
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

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_MAX_OUTPUT_POWRER, buf, count);
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
static ssize_t dfd_get_psu_present_status(unsigned int psu_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_psu_present_status_str(psu_index, buf, count);
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

    ret = dfd_get_psu_in_status_str(psu_index, buf, count);
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

    ret = dfd_get_psu_out_status_str(psu_index, buf, count);
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

    ret = dfd_get_psu_sensor_info(psu_index, PSU_FAN_SPEED, buf, count);
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
    return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
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
    return -ENOSYS;
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

    ret = dfd_get_psu_info(psu_index, DFD_DEV_INFO_TYPE_FAN_DIRECTION, buf, count);
    return ret;
}

/* Similar to dfd_get_fan_led_status */
static ssize_t dfd_get_psu_led_status(unsigned int psu_index, char *buf, size_t count)
{
    return (ssize_t)snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
}

/* Similar to dfd_get_main_board_temp_alias */
static ssize_t dfd_get_psu_temp_alias(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_PSU, psu_index, temp_index, RG_SENSOR_ALIAS,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_temp_type */
static ssize_t dfd_get_psu_temp_type(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_PSU, psu_index, temp_index, RG_SENSOR_TYPE,
              buf, count);
    return ret;

}

/* Similar to dfd_get_main_board_temp_max */
static ssize_t dfd_get_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_PSU, psu_index, temp_index, RG_SENSOR_MAX,
              buf, count);
    return ret;
}

/* Similar to dfd_set_main_board_temp_max */
static int dfd_set_psu_temp_max(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/* Similar to dfd_get_main_board_temp_min */
static ssize_t dfd_get_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_PSU, psu_index, temp_index, RG_SENSOR_MIN,
              buf, count);
    return ret;
}

/* Similar to dfd_set_main_board_temp_min */
static int dfd_set_psu_temp_min(unsigned int psu_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/* Similar to dfd_get_main_board_temp_value */
static ssize_t dfd_get_psu_temp_value(unsigned int psu_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_PSU, psu_index, temp_index, RG_SENSOR_INPUT,
              buf, count);
    return ret;
}
/****************************************end of psu*******************************************/
/****************************************transceiver******************************************/
static int dfd_get_eth_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SFF, RG_MINOR_DEV_NONE);
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

    ret = dfd_get_sff_cpld_info(0, RG_SFF_POWER_ON, buf, count);
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

    ret = dfd_set_sff_cpld_info(0, RG_SFF_POWER_ON, status);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_POWER_ON, buf, count);
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

    ret = dfd_set_sff_cpld_info(eth_index, RG_SFF_POWER_ON, status);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_TX_FAULT, buf, count);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_TX_DIS, buf, count);
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

    ret = dfd_set_sff_cpld_info(eth_index, RG_SFF_TX_DIS, status);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_MODULE_PRESENT, buf, count);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_RX_LOS, buf, count);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_RESET, buf, count);
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

    ret = dfd_set_sff_cpld_info(eth_index, RG_SFF_RESET, status);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_LPMODE, buf, count);
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

    ret = dfd_get_sff_cpld_info(eth_index, RG_SFF_INTERRUPT, buf, count);
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

    ret = dfd_get_eeprom_size(RG_MAIN_DEV_SFF, eth_index);
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

    ret = dfd_read_eeprom_data(RG_MAIN_DEV_SFF, eth_index, buf, offset, count);
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

    ret = dfd_write_eeprom_data(RG_MAIN_DEV_SFF, eth_index, buf, offset, count);
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

    ret = dfd_get_led_status(RG_SYS_LED_FRONT, RG_MINOR_DEV_NONE, buf, count);
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

    ret = dfd_set_led_status(RG_SYS_LED_FRONT, RG_MINOR_DEV_NONE, status);
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_bmc_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(RG_BMC_LED_FRONT, RG_MINOR_DEV_NONE, buf, count);
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_bmc_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(RG_BMC_LED_FRONT, RG_MINOR_DEV_NONE, status);
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_sys_fan_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(RG_FAN_LED_FRONT, RG_MINOR_DEV_NONE, buf, count);
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_sys_fan_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(RG_FAN_LED_FRONT, RG_MINOR_DEV_NONE, status);
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_sys_psu_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(RG_PSU_LED_FRONT, RG_MINOR_DEV_NONE, buf, count);
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_sys_psu_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(RG_PSU_LED_FRONT, RG_MINOR_DEV_NONE, status);
    return ret;
}

/* Similar to dfd_get_sys_led_status */
static ssize_t dfd_get_id_led_status(char *buf, size_t count)
{
    int ret;

    ret = dfd_get_led_status(RG_ID_LED_FRONT, RG_MINOR_DEV_NONE, buf, count);
    return ret;
}

/* Similar to dfd_set_sys_led_status */
static int dfd_set_id_led_status(int status)
{
    int ret;

    ret = dfd_set_led_status(RG_ID_LED_FRONT, RG_MINOR_DEV_NONE, status);
    return ret;
}

/**************************************end of sysled******************************************/
/******************************************FPGA***********************************************/
static int dfd_get_main_board_fpga_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_FPGA);
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

    ret = dfd_get_fpga_name(RG_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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

    ret = dfd_get_fpga_type(RG_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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

    ret = dfd_get_fpga_fw_version(RG_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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

    ret = dfd_get_fpga_hw_version(RG_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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

    ret = dfd_get_fpga_testreg_str(RG_MAIN_DEV_MAINBOARD, fpga_index - 1, buf, count);
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

    ret = dfd_set_fpga_testreg(RG_MAIN_DEV_MAINBOARD, fpga_index - 1, value);
    return ret;
}
/***************************************end of FPGA*******************************************/
/******************************************CPLD***********************************************/
static int dfd_get_main_board_cpld_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_MAINBOARD, RG_MINOR_DEV_CPLD);
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

    ret = dfd_get_cpld_name(RG_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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

    ret = dfd_get_cpld_type(RG_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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

    ret = dfd_get_cpld_fw_version(RG_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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

    ret = dfd_get_cpld_hw_version(RG_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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

    ret = dfd_get_cpld_testreg_str(RG_MAIN_DEV_MAINBOARD, cpld_index - 1, buf, count);
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

    ret = dfd_set_cpld_testreg(RG_MAIN_DEV_MAINBOARD, cpld_index - 1, value);
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

    ret = dfd_get_watchdog_info(RG_WDT_TYPE_NAME, buf, count);
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

    ret = dfd_get_watchdog_info(RG_WDT_TYPE_TIMELEFT, buf, count);
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

    ret = dfd_get_watchdog_info(RG_WDT_TYPE_TIMEOUT, buf, count);
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
    return -ENOSYS;
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

    ret = dfd_watchdog_get_status_str(buf, count);
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
    return -ENOSYS;
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
    return -ENOSYS;
}
/*************************************end of watchdog*****************************************/
/******************************************slot***********************************************/
static int dfd_get_slot_number(void)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SLOT, RG_MINOR_DEV_NONE);
    return ret;
}

static int dfd_get_slot_temp_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SLOT, RG_MINOR_DEV_TEMP);
    return ret;
}

static int dfd_get_slot_vol_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SLOT, RG_MINOR_DEV_IN);
    return ret;
}

static int dfd_get_slot_curr_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SLOT, RG_MINOR_DEV_CURR);
    return ret;
}

static int dfd_get_slot_fpga_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SLOT, RG_MINOR_DEV_FPGA);
    return ret;
}

static int dfd_get_slot_cpld_number(unsigned int slot_index)
{
    int ret;

    ret = dfd_get_dev_number(RG_MAIN_DEV_SLOT, RG_MINOR_DEV_CPLD);
    return ret;
}

/* Similar to dfd_get_fan_model_name */
static ssize_t dfd_get_slot_model_name(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_NAME, buf, count);
    return ret;
}

/* Similar to rg_get_fan_serial_number */
static ssize_t dfd_get_slot_serial_number(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_SN, buf, count);
    return ret;
}

/* Similar to rg_get_fan_part_number */
static ssize_t dfd_get_slot_part_number(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_PART_NUMBER, buf, count);
    return ret;
}

/* Similar to rg_get_fan_hardware_version */
static ssize_t dfd_get_slot_hardware_version(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_info(slot_index, DFD_DEV_INFO_TYPE_HW_INFO, buf, count);
    return ret;
}

/* Similar to dfd_get_fan_status */
static ssize_t dfd_get_slot_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_slot_status_str(slot_index, buf, count);
    return ret;
}

/* Similar to dfd_get_fan_led_status */
static ssize_t dfd_get_slot_led_status(unsigned int slot_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_led_status(RG_SLOT_LED_MODULE, slot_index, buf, count);
    return ret;
}

/* Similar to dfd_set_fan_led_status */
static int dfd_set_slot_led_status(unsigned int slot_index, int status)
{
    int ret;

    ret = dfd_set_led_status(RG_SLOT_LED_MODULE, slot_index, status);
    return ret;
}

/* Similar to dfd_get_main_board_temp_alias */
static ssize_t dfd_get_slot_temp_alias(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_SLOT, slot_index, temp_index, RG_SENSOR_ALIAS,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_temp_type */
static ssize_t dfd_get_slot_temp_type(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_SLOT, slot_index, temp_index, RG_SENSOR_TYPE,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_temp_max */
static ssize_t dfd_get_slot_temp_max(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_SLOT, slot_index, temp_index, RG_SENSOR_MAX,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_temp_min */
static ssize_t dfd_get_slot_temp_min(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_SLOT, slot_index, temp_index, RG_SENSOR_MIN,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_temp_value */
static ssize_t dfd_get_slot_temp_value(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_temp_info(RG_MAIN_DEV_SLOT, slot_index, temp_index, RG_SENSOR_INPUT,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_alias */
static ssize_t dfd_get_slot_vol_alias(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_ALIAS,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_type */
static ssize_t dfd_get_slot_vol_type(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_TYPE,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_max */
static ssize_t dfd_get_slot_vol_max(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_MAX,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_min */
static ssize_t dfd_get_slot_vol_min(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_MIN,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_range */
static ssize_t dfd_get_slot_vol_range(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_RANGE,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_nominal_value */
static ssize_t dfd_get_slot_vol_nominal_value(unsigned int slot_index,
                   unsigned int vol_index, char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_NOMINAL_VAL,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_vol_value */
static ssize_t dfd_get_slot_vol_value(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_voltage_info(RG_MAIN_DEV_SLOT, slot_index, vol_index, RG_SENSOR_INPUT,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_curr_alias */
static ssize_t dfd_get_slot_curr_alias(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(RG_MAIN_DEV_SLOT, slot_index, curr_index, RG_SENSOR_ALIAS,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_curr_type */
static ssize_t dfd_get_slot_curr_type(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(RG_MAIN_DEV_SLOT, slot_index, curr_index, RG_SENSOR_TYPE,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_curr_max */
static ssize_t dfd_get_slot_curr_max(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(RG_MAIN_DEV_SLOT, slot_index, curr_index, RG_SENSOR_MAX,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_curr_min */
static ssize_t dfd_get_slot_curr_min(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(RG_MAIN_DEV_SLOT, slot_index, curr_index, RG_SENSOR_MIN,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_curr_value */
static ssize_t dfd_get_slot_curr_value(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_current_info(RG_MAIN_DEV_SLOT, slot_index, curr_index, RG_SENSOR_INPUT,
              buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_fpga_alias */
static ssize_t dfd_get_slot_fpga_alias(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_name(slot_index, fpga_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_fpga_type */
static ssize_t dfd_get_slot_fpga_type(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_type(slot_index, fpga_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_fpga_firmware_version */
static ssize_t dfd_get_slot_fpga_firmware_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_fw_version(slot_index, fpga_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_fpga_board_version */
static ssize_t dfd_get_slot_fpga_board_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_hw_version(slot_index, fpga_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_fpga_test_reg */
static ssize_t dfd_get_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_fpga_testreg_str(slot_index, fpga_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_set_main_board_fpga_test_reg */
static int dfd_set_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
           unsigned int value)
{
    int ret;

    ret = dfd_set_fpga_testreg(slot_index, fpga_index - 1, value);
    return ret;
}

/* Similar to dfd_get_main_board_cpld_alias */
static ssize_t dfd_get_slot_cpld_alias(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_name(slot_index, cpld_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_cpld_type */
static ssize_t dfd_get_slot_cpld_type(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_type(slot_index, cpld_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_cpld_firmware_version */
static ssize_t dfd_get_slot_cpld_firmware_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_fw_version(slot_index, cpld_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_cpld_board_version */
static ssize_t dfd_get_slot_cpld_board_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_hw_version(slot_index, cpld_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_get_main_board_cpld_test_reg */
static ssize_t dfd_get_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    ssize_t ret;

    ret = dfd_get_cpld_testreg_str(slot_index, cpld_index - 1, buf, count);
    return ret;
}

/* Similar to dfd_set_main_board_cpld_test_reg */
static int dfd_set_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
           unsigned int value)
{
    int ret;

    ret = dfd_set_cpld_testreg(slot_index, cpld_index - 1, value);
    return ret;
}
/***************************************end of slot*******************************************/
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
    /* voltage sensors */
    .get_main_board_vol_number = dfd_get_main_board_vol_number,
    .get_main_board_vol_alias = dfd_get_main_board_vol_alias,
    .get_main_board_vol_type = dfd_get_main_board_vol_type,
    .get_main_board_vol_max = dfd_get_main_board_vol_max,
    .get_main_board_vol_min = dfd_get_main_board_vol_min,
    .get_main_board_vol_range = dfd_get_main_board_vol_range,
    .get_main_board_vol_nominal_value = dfd_get_main_board_vol_nominal_value,
    .get_main_board_vol_value = dfd_get_main_board_vol_value,
    /* current sensors */
    .get_main_board_curr_number = dfd_get_main_board_curr_number,
    .get_main_board_curr_alias = dfd_get_main_board_curr_alias,
    .get_main_board_curr_type = dfd_get_main_board_curr_type,
    .get_main_board_curr_max = dfd_get_main_board_curr_max,
    .get_main_board_curr_min = dfd_get_main_board_curr_min,
    .get_main_board_curr_value = dfd_get_main_board_curr_value,
    /* syseeprom */
    .get_syseeprom_size = dfd_get_syseeprom_size,
    .read_syseeprom_data = dfd_read_syseeprom_data,
    .write_syseeprom_data = dfd_write_syseeprom_data,
    /* fan */
    .get_fan_number = dfd_get_fan_number,
    .get_fan_motor_number = dfd_get_fan_motor_number,
    .get_fan_model_name = dfd_get_fan_model_name,
    .get_fan_serial_number = dfd_get_fan_serial_number,
    .get_fan_part_number = dfd_get_fan_part_number,
    .get_fan_hardware_version = dfd_get_fan_hardware_version,
    .get_fan_status = dfd_get_fan_status,
    .get_fan_led_status = dfd_get_fan_led_status,
    .set_fan_led_status = dfd_set_fan_led_status,
    .get_fan_direction = dfd_get_fan_direction,
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
    .get_psu_present_status = dfd_get_psu_present_status,
    .get_psu_in_status = dfd_get_psu_in_status,
    .get_psu_out_status = dfd_get_psu_out_status,
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
    /* transceiver */
    .get_eth_number = dfd_get_eth_number,
    .get_transceiver_power_on_status = dfd_get_transceiver_power_on_status,
    .set_transceiver_power_on_status = dfd_set_transceiver_power_on_status,
    .get_eth_power_on_status = dfd_get_eth_power_on_status,
    .set_eth_power_on_status = dfd_set_eth_power_on_status,
    .get_eth_tx_fault_status = dfd_get_eth_tx_fault_status,
    .get_eth_tx_disable_status = dfd_get_eth_tx_disable_status,
    .set_eth_tx_disable_status = dfd_set_eth_tx_disable_status,
    .get_eth_present_status = dfd_get_eth_present_status,
    .get_eth_rx_los_status = dfd_get_eth_rx_los_status,
    .get_eth_reset_status = dfd_get_eth_reset_status,
    .set_eth_reset_status = dfd_set_eth_reset_status,
    .get_eth_low_power_mode_status = dfd_get_eth_low_power_mode_status,
    .get_eth_interrupt_status = dfd_get_eth_interrupt_status,
    .get_eth_eeprom_size = dfd_get_eth_eeprom_size,
    .read_eth_eeprom_data = dfd_read_eth_eeprom_data,
    .write_eth_eeprom_data = dfd_write_eth_eeprom_data,
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
    .get_slot_serial_number = dfd_get_slot_serial_number,
    .get_slot_part_number = dfd_get_slot_part_number,
    .get_slot_hardware_version = dfd_get_slot_hardware_version,
    .get_slot_status = dfd_get_slot_status,
    .get_slot_led_status = dfd_get_slot_led_status,
    .set_slot_led_status = dfd_set_slot_led_status,
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
};

struct switch_drivers_s * switch_driver_get(void) {
    return &switch_drivers;
}

static int32_t __init switch_driver_init(void)
{
    int ret;

    SWITCH_DEBUG(DBG_VERBOSE, "Enter.\n");
    ret = rg_dev_cfg_init();
    if (ret < 0) {
        SWITCH_DEBUG(DBG_ERROR, "rg_dev_cfg_init failed ret %d.\n", ret);
        return ret;
    }
    SWITCH_DEBUG(DBG_VERBOSE, "success.\n");
    return 0;
}

static void __exit switch_driver_exit(void)
{
    SWITCH_DEBUG(DBG_VERBOSE, "switch_driver_exit.\n");
    rg_dev_cfg_exit();
    return;
}

module_init(switch_driver_init);
module_exit(switch_driver_exit);
EXPORT_SYMBOL(switch_driver_get);
module_param(g_switch_dbg_level, int, S_IRUGO | S_IWUSR);
MODULE_AUTHOR("sonic S3IP sysfs");
MODULE_LICENSE("GPL");
