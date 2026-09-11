#ifndef _DEBUG_SYSFS_H_
#define _DEBUG_SYSFS_H_

struct s3ip_sysfs_debug_drivers_s {
    int (*set_debug_and_reset)(unsigned int reset);
};

extern int s3ip_sysfs_debug_drivers_register(struct s3ip_sysfs_debug_drivers_s *drv);
extern void s3ip_sysfs_debug_drivers_unregister(void);

#endif /*_DEBUG_SYSFS_H_ */
