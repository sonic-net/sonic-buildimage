#ifndef _PSU_SYSFS_H_
#define _PSU_SYSFS_H_

struct s3ip_sysfs_psu_drivers_s {
    int (*get_psu_number)(void);
    int (*get_psu_temp_number)(unsigned int psu_index);
    ssize_t (*get_psu_attr)(unsigned int psu_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_psu_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hw_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_hw_detail_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_alarm)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_type)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_sensor_attr)(unsigned int psu_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_psu_present_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_status_pmbus)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_in_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_out_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_fan_ratio)(unsigned int psu_index, char *buf, size_t count);
    int (*set_psu_fan_ratio)(unsigned int psu_index, int ratio);
    ssize_t (*get_psu_fan_direction)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_led_status)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_temp_attr)(unsigned int psu_index, unsigned int temp_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_psu_attr_threshold)(unsigned int psu_index, unsigned int type,  char *buf, size_t count);
    int (*get_psu_eeprom_size)(unsigned int psu_index);
    ssize_t (*read_psu_eeprom_data)(unsigned int psu_index, char *buf, loff_t offset, size_t count);
    ssize_t (*get_psu_blackbox_path)(unsigned int psu_index, char *buf, size_t count);
    ssize_t (*get_psu_pmbus_info)(unsigned int psu_index, char *buf, size_t count);
    int (*clear_psu_blackbox)(unsigned int psu_index, uint8_t value);
    ssize_t (*get_psu_support_upgrade)(unsigned int cpld_index, char *buf, size_t count);
    ssize_t (*get_psu_upgrade_active_type)(unsigned int cpld_index, char *buf, size_t count);
    int (*set_psu_reset)(unsigned int psu_index, uint8_t value);
};

extern int s3ip_sysfs_psu_drivers_register(struct s3ip_sysfs_psu_drivers_s *drv);
extern void s3ip_sysfs_psu_drivers_unregister(void);
#define SINGLE_PSU_PRESENT_DEBUG_FILE       "/etc/sonic/.present_psu_%d"

#endif /*_PSU_SYSFS_H_ */
