#ifndef _POWER_SENSOR_SYSFS_H_
#define _POWER_SENSOR_SYSFS_H_

struct s3ip_sysfs_power_sensor_drivers_s {
    int (*get_main_board_power_number)(void);
    ssize_t (*get_main_board_power_alias)(unsigned int power_index, char *buf, size_t count);
    ssize_t (*get_main_board_power_type)(unsigned int power_index, char *buf, size_t count);
    ssize_t (*get_main_board_power_max)(unsigned int power_index, char *buf, size_t count);
    int (*set_main_board_power_max)(unsigned int power_index, const char *buf, size_t count);
    ssize_t (*get_main_board_power_min)(unsigned int power_index, char *buf, size_t count);
    int (*set_main_board_power_min)(unsigned int power_index, const char *buf, size_t count);
    ssize_t (*get_main_board_power_value)(unsigned int power_index, char *buf, size_t count);
    ssize_t (*get_main_board_power_monitor_flag)(unsigned int power_index, char *buf, size_t count);
};

extern int s3ip_sysfs_power_sensor_drivers_register(struct s3ip_sysfs_power_sensor_drivers_s *drv);
extern void s3ip_sysfs_power_sensor_drivers_unregister(void);
#endif /*_POWER_SENSOR_SYSFS_H_ */
