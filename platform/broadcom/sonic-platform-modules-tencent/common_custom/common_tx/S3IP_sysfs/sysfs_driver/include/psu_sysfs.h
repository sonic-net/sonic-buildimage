#ifndef _PSU_SYSFS_H_
#define _PSU_SYSFS_H_

struct s3ip_sysfs_psu_drivers_s {
    int (*get_psu_number)(void);
    int (*get_psu_temp_number)(unsigned int psu_index);
    ssize_t (*get_psu_model_name)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_serial_number)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_part_number)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hardware_version)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_type)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_curr)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_vol)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_curr)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_vol)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_max_power)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_present_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_alias)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_type)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_value)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
};

extern int s3ip_sysfs_psu_drivers_register(struct s3ip_sysfs_psu_drivers_s *drv);
extern void s3ip_sysfs_psu_drivers_unregister(void);
#endif /*_PSU_SYSFS_H_ */
