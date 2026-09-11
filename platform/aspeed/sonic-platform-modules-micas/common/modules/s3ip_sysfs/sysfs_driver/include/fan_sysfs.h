#ifndef _FAN_SYSFS_H_
#define _FAN_SYSFS_H_

#include <wb_platform_common.h>

struct s3ip_sysfs_fan_drivers_s {
    int (*get_fan_number)(void);
    int (*get_fan_motor_number)(unsigned int fan_index);
    ssize_t (*get_fan_model_name)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_vendor)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_serial_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_part_number)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_hardware_version)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_status)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_present)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_led_status)(unsigned int fan_index, char *buf, size_t count);
    int (*set_fan_led_status)(unsigned int fan_index, int status);
    ssize_t (*get_fan_direction)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_status)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_tolerance)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_target)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_max)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_min)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_motor_speed_threshold_cnt)(unsigned int fan_index, unsigned int motor_index, char *buf, size_t count);
    ssize_t (*get_fan_ratio)(unsigned int fan_index, char *buf, size_t count);
    ssize_t (*get_fanctrl_duration_max)(char *buf, size_t count);
    int (*set_fan_ratio)(unsigned int fan_index, int ratio);
    int (*set_fan_attr)(unsigned int fan_index, unsigned int type, unsigned int value);
    ssize_t (*get_fan_attr)(unsigned int fan_index, unsigned int type, char *buf, size_t count);
    int (*get_fan_eeprom_size)(unsigned int fan_index);
    ssize_t (*read_fan_eeprom_data)(unsigned int fan_index, char *buf, loff_t offset, size_t count);
    ssize_t (*write_fan_eeprom_data)(unsigned int fan_index, char *buf, loff_t offset, size_t count);
    ssize_t (*set_fan_dev_debug_fan_attr)(unsigned int type, unsigned int fan_index, const char *value);
};

extern int s3ip_sysfs_fan_drivers_register(struct s3ip_sysfs_fan_drivers_s *drv);
extern void s3ip_sysfs_fan_drivers_unregister(void);
extern ssize_t fan_debug_present_get(char *buf);
extern ssize_t fan_debug_present_set(const char* buf, size_t count);
#define SINGLE_FAN_STATUS_DEBUG_FILE       WB_HOST_MISC_DIR ".status_fan_%d"
#define FAN_ABSENT_STR              "0\n"
#define FAN_OK_STR                  "1\n"
#define FAN_NOTOK_STR               "2\n"
#endif /*_FAN_SYSFS_H_ */
