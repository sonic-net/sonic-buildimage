#ifndef _SYSLED_SYSFS_H_
#define _SYSLED_SYSFS_H_

struct s3ip_sysfs_sysled_drivers_s {
    ssize_t (*get_sys_led_status)(char *buf, size_t count);
    int (*set_sys_led_status)(int status);
    ssize_t (*get_bmc_led_status)(char *buf, size_t count);
    int (*set_bmc_led_status)(int status);
    ssize_t (*get_sys_fan_led_status)(char *buf, size_t count);
    int (*set_sys_fan_led_status)(int status);
    ssize_t (*get_sys_psu_led_status)(char *buf, size_t count);
    int (*set_sys_psu_led_status)(int status);
    ssize_t (*get_id_led_status)(char *buf, size_t count);
    int (*set_id_led_status)(int status);
};

extern int s3ip_sysfs_sysled_drivers_register(struct s3ip_sysfs_sysled_drivers_s *drv);
extern void s3ip_sysfs_sysled_drivers_unregister(void);
#endif /*_SYSLED_SYSFS_H_ */
