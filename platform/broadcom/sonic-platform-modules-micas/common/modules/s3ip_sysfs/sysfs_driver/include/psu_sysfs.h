#ifndef _PSU_SYSFS_H_
#define _PSU_SYSFS_H_

struct s3ip_sysfs_psu_drivers_s {
    int (*get_psu_number)(void);
    int (*get_psu_temp_number)(unsigned int psu_index);
    ssize_t (*get_psu_model_name)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_vendor)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_date)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm)(unsigned int psu_index, char *buf, size_t count);
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
    ssize_t (*get_psu_status_pmbus)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_speed_cal)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_alias)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_type)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_max)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    int (*set_psu_temp_min)(unsigned int psu_index, unsigned int temp_index, const char *buf, size_t count);
    ssize_t (*get_psu_temp_value)(unsigned int psu_index, unsigned int temp_index, char *buf, size_t count);
    ssize_t (*get_psu_attr_threshold)(unsigned int psu_index, unsigned int type,  char *buf, size_t count);
};

extern int s3ip_sysfs_psu_drivers_register(struct s3ip_sysfs_psu_drivers_s *drv);
extern void s3ip_sysfs_psu_drivers_unregister(void);

typedef enum psu_threshold_type_e {
    PSU_IN_VOL_MAX     = 13,
    PSU_IN_CURR_MAX    = 14,
    PSU_IN_VOL_MIN     = 15,
    PSU_IN_CURR_MIN    = 16,
    PSU_OUT_VOL_MAX     = 17,
    PSU_OUT_CURR_MAX    = 18,
    PSU_OUT_VOL_MIN     = 19,
    PSU_OUT_CURR_MIN    = 20,
    PSU_FAN_SPEED_MAX   = 21,
    PSU_FAN_SPEED_MIN   = 22,
    PSU_IN_POWER_MAX    = 23,
    PSU_IN_POWER_MIN    = 24,
    PSU_OUT_POWER_MAX   = 25,
    PSU_OUT_POWER_MIN   = 26,
} psu_threshold_type_t;

#endif /*_PSU_SYSFS_H_ */
