#ifndef _TEMP_SENSOR_SYSFS_H_
#define _TEMP_SENSOR_SYSFS_H_

struct s3ip_sysfs_temp_sensor_drivers_s {
    int (*get_main_board_temp_number)(void);
    ssize_t (*get_temp_attr)(unsigned int temp_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_main_board_temp_monitor_flag)(unsigned int temp_index, char *buf, size_t count);
};

extern int s3ip_sysfs_temp_sensor_drivers_register(struct s3ip_sysfs_temp_sensor_drivers_s *drv);
extern void s3ip_sysfs_temp_sensor_drivers_unregister(void);
#endif /*_TEMP_SENSOR_SYSFS_H_ */
