#ifndef _WATCHDOG_SYSFS_H_
#define _WATCHDOG_SYSFS_H_

struct s3ip_sysfs_watchdog_drivers_s {
    ssize_t (*get_watchdog_identify)(char *buf, size_t count);
    ssize_t (*get_watchdog_timeleft)(char *buf, size_t count);
    ssize_t (*get_watchdog_timeout)(char *buf, size_t count);
    int (*set_watchdog_timeout)(int value);
    ssize_t (*get_watchdog_enable_status)(char *buf, size_t count);
    int (*set_watchdog_enable_status)(int value);
    int (*set_watchdog_reset)(int value);
};

extern int s3ip_sysfs_watchdog_drivers_register(struct s3ip_sysfs_watchdog_drivers_s *drv);
extern void s3ip_sysfs_watchdog_drivers_unregister(void);
#endif /*_WATCHDOG_SYSFS_H_ */
