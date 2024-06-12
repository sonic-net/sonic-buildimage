#ifndef _SYSFS_COMMON_H_
#define _SYSFS_COMMON_H_

#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define DIR_NAME_MAX_LEN        (64)

#define WB_SYSFS_DEV_ERROR         "NA"
/* sysfs directory name */
#define FAN_SYSFS_NAME          "fan"
#define PSU_SYSFS_NAME          "psu"
#define SLOT_SYSFS_NAME         "slot"
#define VOLTAGE_SYSFS_NAME      "in"
#define TEMP_SYSFS_NAME         "temp"
#define SFF_SYSFS_NAME          "sff"

typedef enum wb_main_dev_type_e {
    WB_MAIN_DEV_MAINBOARD = 0,
    WB_MAIN_DEV_FAN       = 1,
    WB_MAIN_DEV_PSU       = 2,
    WB_MAIN_DEV_SFF       = 3,
    WB_MAIN_DEV_CPLD      = 4,
    WB_MAIN_DEV_SLOT      = 5,
} wb_main_dev_type_t;

typedef enum wb_minor_dev_type_e {
    WB_MINOR_DEV_NONE  = 0,    /* None */
    WB_MINOR_DEV_TEMP  = 1,
    WB_MINOR_DEV_IN    = 2,
    WB_MINOR_DEV_CURR  = 3,
    WB_MINOR_DEV_POWER = 4,
    WB_MINOR_DEV_MOTOR = 5,
    WB_MINOR_DEV_PSU   = 6,
} wb_minor_dev_type_t;

typedef enum wb_sensor_type_e {
    WB_SENSOR_INPUT    = 0,
    WB_SENSOR_ALIAS    = 1,
    WB_SENSOR_TYPE     = 2,
    WB_SENSOR_MAX      = 3,
    WB_SENSOR_MAX_HYST = 4,
    WB_SENSOR_MIN      = 5,
    WB_SENSOR_CRIT     = 6,
} wb_sensor_type_t;

typedef enum wb_sff_cpld_attr_e {
    WB_SFF_POWER_ON      = 0x01,
    WB_SFF_TX_FAULT,
    WB_SFF_TX_DIS,
    WB_SFF_PRE_N,
    WB_SFF_RX_LOS,
    WB_SFF_RESET,
    WB_SFF_LPMODE,
    WB_SFF_MODULE_PRESENT,
    WB_SFF_INTERRUPT,
} wb_sff_cpld_attr_t;

struct switch_drivers_t{
    /* device */
    int (*get_dev_number) (unsigned int main_dev_id, unsigned int minor_dev_id);
    /* fan */
    int (*get_fan_number) (void);
    ssize_t (*get_fan_speed) (unsigned int fan_index, unsigned int motor_index, unsigned int *speed);
    int (*get_fan_pwm) (unsigned int fan_index, unsigned int motor_index, int *pwm);
    int (*set_fan_pwm) (unsigned int fan_index, unsigned int motor_index, int pwm);
    int (*get_fan_present_status)(unsigned int fan_index);
    int (*get_fan_roll_status)(unsigned int fan_index, unsigned int motor_index);
    int (*get_fan_speed_level)(unsigned int fan_index, unsigned int motor_index, int *level);
    int (*set_fan_speed_level)(unsigned int fan_index, unsigned int motor_index, int level);
    /* slot */
    int (*get_slot_present_status) (unsigned int slot_index);
    /* sensors */
    ssize_t (*get_temp_info)( uint8_t main_dev_id, uint8_t dev_index,
            uint8_t temp_index, uint8_t temp_attr, char *buf);
    ssize_t (*get_voltage_info)( uint8_t main_dev_id, uint8_t dev_index,
            uint8_t in_index, uint8_t in_attr, char *buf);
    /* psu */
    int (*get_psu_present_status)(unsigned int psu_index);
    int (*get_psu_output_status)(unsigned int psu_index);
    int (*get_psu_alert_status)(unsigned int psu_index);
    /* sff */
    ssize_t (*get_sff_cpld_info)( unsigned int sff_index, int cpld_reg_type, char *buf, int len);
    ssize_t (*get_sff_dir_name)(unsigned int sff_index, char *buf, int buf_len);
};

extern struct switch_drivers_t * dfd_plat_driver_get(void);

#endif /*_SYSFS_COMMON_H_ */
