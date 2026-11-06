#ifndef __CLOCK_SYSFS_H__
#define __CLOCK_SYSFS_H__

struct s3ip_sysfs_clock_drivers_s {
    int (*get_clock_number)(void);
    ssize_t (*get_clock_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_clock_drivers_register(struct s3ip_sysfs_clock_drivers_s *drv);
extern void s3ip_sysfs_clock_drivers_unregister(void);

#endif /* __CLOCK_SYSFS_H__ */