/*
 * slot_device_driver.c
 *
 * This module realize /sys/s3ip/slot attributes read and write functions
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "device_driver_common.h"
#include "slot_sysfs.h"

#define SLOT_INFO(fmt, args...) LOG_INFO("slot: ", fmt, ##args)
#define SLOT_ERR(fmt, args...)  LOG_ERR("slot: ", fmt, ##args)
#define SLOT_DBG(fmt, args...)  LOG_DBG("slot: ", fmt, ##args)

static int g_loglevel = 0;

/******************************************slot***********************************************/
static int demo_get_slot_number(void)
{
    /* add vendor codes here */
    return 1;
}

static int demo_get_slot_temp_number(unsigned int slot_index)
{
    /* add vendor codes here */
    return 1;
}

static int demo_get_slot_vol_number(unsigned int slot_index)
{
    /* add vendor codes here */
    return 1;
}

static int demo_get_slot_curr_number(unsigned int slot_index)
{
    /* add vendor codes here */
    return 1;
}

static int demo_get_slot_fpga_number(unsigned int slot_index)
{
    /* add vendor codes here */
    return 1;
}

static int demo_get_slot_cpld_number(unsigned int slot_index)
{
    /* add vendor codes here */
    return 1;
}

/*
 * demo_get_slot_model_name - Used to get slot model name,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_model_name(unsigned int slot_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_serial_number - Used to get slot serial number,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_serial_number(unsigned int slot_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_part_number - Used to get slot part number,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_part_number(unsigned int slot_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_hardware_version - Used to get slot hardware version,
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_hardware_version(unsigned int slot_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_status - Used to get slot status,
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
static ssize_t demo_get_slot_status(unsigned int slot_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_led_status - Used to get slot led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * @slot_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_led_status(unsigned int slot_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_led_status - Used to set slot led status
 * @slot_index: start with 1
 * @status: led status, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_led_status(unsigned int slot_index, int status)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_temp_alias - Used to identify the location of the temperature sensor of slot,
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_temp_alias(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_temp_type - Used to get the model of temperature sensor of slot,
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_temp_type(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;

}

/*
 * demo_get_slot_temp_max - Used to get the maximum threshold of temperature sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_temp_max(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_temp_max - Used to set the maximum threshold of temperature sensor of slot,
 * get value from buf and set it to maximum threshold of slot temperature sensor
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '80.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_temp_max(unsigned int slot_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_temp_min - Used to get the minimum threshold of temperature sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_temp_min(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_temp_min - Used to set the minimum threshold of temperature sensor of slot,
 * get value from buf and set it to minimum threshold of slot temperature sensor
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_temp_min(unsigned int slot_index, unsigned int temp_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_temp_value - Used to get the input value of temperature sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_temp_value(unsigned int slot_index, unsigned int temp_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_vol_alias - Used to identify the location of the voltage sensor of slot,
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_vol_alias(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_vol_type - Used to get the model of voltage sensor of slot,
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
static ssize_t demo_get_slot_vol_type(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;

}

/*
 * demo_get_slot_vol_max - Used to get the maximum threshold of voltage sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_vol_max(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_vol_max - Used to set the maximum threshold of volatge sensor of slot,
 * get value from buf and set it to maximum threshold of volatge sensor
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: the buf store the data to be set, eg '3.567'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_vol_max(unsigned int slot_index, unsigned int vol_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_vol_min - Used to get the minimum threshold of voltage sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_vol_min(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_vol_min - Used to set the minimum threshold of voltage sensor of slot,
 * get value from buf and set it to minimum threshold of voltage sensor
 * @slot_index: start with 1
 * @temp_index: start with 1
 * @buf: the buf store the data to be set, eg '3.123'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_vol_min(unsigned int slot_index, unsigned int vol_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_vol_range - Used to get the output error value of voltage sensor of slot,
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
static ssize_t demo_get_slot_vol_range(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_vol_nominal_value - Used to get the nominal value of voltage sensor of slot,
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
static ssize_t demo_get_slot_vol_nominal_value(unsigned int slot_index,
                   unsigned int vol_index, char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_vol_value - Used to get the input value of voltage sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @vol_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_vol_value(unsigned int slot_index, unsigned int vol_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_curr_alias - Used to identify the location of the current sensor of slot,
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_curr_alias(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_curr_type - Used to get the model of current sensor of slot,
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_curr_type(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_curr_max - Used to get the maximum threshold of current sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_curr_max(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_curr_max - Used to set the maximum threshold of current sensor of slot,
 * get value from buf and set it to maximum threshold of current sensor
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: the buf store the data to be set
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_curr_max(unsigned int slot_index, unsigned int curr_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_curr_min - Used to get the minimum threshold of current sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_curr_min(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_curr_min - Used to set the minimum threshold of current sensor of slot,
 * get value from buf and set it to minimum threshold of current sensor
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: the buf store the data to be set, eg '50.000'
 * @count: length of buf
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_curr_min(unsigned int slot_index, unsigned int curr_index,
               const char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_curr_value - Used to get the input value of current sensor of slot,
 * filled the value to buf, and the value keep three decimal places
 * @slot_index: start with 1
 * @curr_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_curr_value(unsigned int slot_index, unsigned int curr_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_fpga_alias - Used to identify the location of slot fpga,
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_fpga_alias(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_fpga_type - Used to get slot fpga model name
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_fpga_type(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_fpga_firmware_version - Used to get slot fpga firmware version,
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_fpga_firmware_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_fpga_board_version - Used to get slot fpga board version,
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_fpga_board_version(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_fpga_test_reg - Used to test slot fpga register read
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
static ssize_t demo_get_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_fpga_test_reg - Used to test slot fpga register write
 * @slot_index: start with 1
 * @fpga_index: start with 1
 * @value: value write to slot fpga
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_fpga_test_reg(unsigned int slot_index, unsigned int fpga_index,
           unsigned int value)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_cpld_alias - Used to identify the location of slot cpld,
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_cpld_alias(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_cpld_type - Used to get slot cpld model name
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_cpld_type(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_cpld_firmware_version - Used to get slot cpld firmware version,
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_cpld_firmware_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_cpld_board_version - Used to get slot cpld board version,
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t demo_get_slot_cpld_board_version(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_get_slot_cpld_test_reg - Used to test slot cpld register read
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
static ssize_t demo_get_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
                   char *buf, size_t count)
{
    /* add vendor codes here */
    return -ENOSYS;
}

/*
 * demo_set_slot_cpld_test_reg - Used to test slot cpld register write
 * @slot_index: start with 1
 * @cpld_index: start with 1
 * @value: value write to slot cpld
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int demo_set_slot_cpld_test_reg(unsigned int slot_index, unsigned int cpld_index,
           unsigned int value)
{
    /* add vendor codes here */
    return -ENOSYS;
}
/***************************************end of slot*******************************************/

static struct s3ip_sysfs_slot_drivers_s drivers = {
    /*
     * set ODM slot drivers to /sys/s3ip/slot,
     * if not support the function, set corresponding hook to NULL.
     */
    .get_slot_number = demo_get_slot_number,
    .get_slot_temp_number = demo_get_slot_temp_number,
    .get_slot_vol_number = demo_get_slot_vol_number,
    .get_slot_curr_number = demo_get_slot_curr_number,
    .get_slot_cpld_number = demo_get_slot_cpld_number,
    .get_slot_fpga_number = demo_get_slot_fpga_number,
    .get_slot_model_name = demo_get_slot_model_name,
    .get_slot_serial_number = demo_get_slot_serial_number,
    .get_slot_part_number = demo_get_slot_part_number,
    .get_slot_hardware_version = demo_get_slot_hardware_version,
    .get_slot_status = demo_get_slot_status,
    .get_slot_led_status = demo_get_slot_led_status,
    .set_slot_led_status = demo_set_slot_led_status,
    .get_slot_temp_alias = demo_get_slot_temp_alias,
    .get_slot_temp_type = demo_get_slot_temp_type,
    .get_slot_temp_max = demo_get_slot_temp_max,
    .set_slot_temp_max = demo_set_slot_temp_max,
    .get_slot_temp_min = demo_get_slot_temp_min,
    .set_slot_temp_min = demo_set_slot_temp_min,
    .get_slot_temp_value = demo_get_slot_temp_value,
    .get_slot_vol_alias = demo_get_slot_vol_alias,
    .get_slot_vol_type = demo_get_slot_vol_type,
    .get_slot_vol_max = demo_get_slot_vol_max,
    .set_slot_vol_max = demo_set_slot_vol_max,
    .get_slot_vol_min = demo_get_slot_vol_min,
    .set_slot_vol_min = demo_set_slot_vol_min,
    .get_slot_vol_range = demo_get_slot_vol_range,
    .get_slot_vol_nominal_value = demo_get_slot_vol_nominal_value,
    .get_slot_vol_value = demo_get_slot_vol_value,
    .get_slot_curr_alias = demo_get_slot_curr_alias,
    .get_slot_curr_type = demo_get_slot_curr_type,
    .get_slot_curr_max = demo_get_slot_curr_max,
    .set_slot_curr_max = demo_set_slot_curr_max,
    .get_slot_curr_min = demo_get_slot_curr_min,
    .set_slot_curr_min = demo_set_slot_curr_min,
    .get_slot_curr_value = demo_get_slot_curr_value,
    .get_slot_fpga_alias = demo_get_slot_fpga_alias,
    .get_slot_fpga_alias = demo_get_slot_fpga_alias,
    .get_slot_fpga_type = demo_get_slot_fpga_type,
    .get_slot_fpga_firmware_version = demo_get_slot_fpga_firmware_version,
    .get_slot_fpga_board_version = demo_get_slot_fpga_board_version,
    .get_slot_fpga_test_reg = demo_get_slot_fpga_test_reg,
    .set_slot_fpga_test_reg = demo_set_slot_fpga_test_reg,
    .get_slot_cpld_alias = demo_get_slot_cpld_alias,
    .get_slot_cpld_type = demo_get_slot_cpld_type,
    .get_slot_cpld_firmware_version = demo_get_slot_cpld_firmware_version,
    .get_slot_cpld_board_version = demo_get_slot_cpld_board_version,
    .get_slot_cpld_test_reg = demo_get_slot_cpld_test_reg,
    .set_slot_cpld_test_reg = demo_set_slot_cpld_test_reg,
};

static int __init slot_dev_drv_init(void)
{
    int ret;

    SLOT_INFO("slot_init...\n");

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
