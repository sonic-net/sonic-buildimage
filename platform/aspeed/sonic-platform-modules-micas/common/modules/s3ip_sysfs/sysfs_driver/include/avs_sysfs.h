#ifndef __AVS_SYSFS_H__
#define __AVS_SYSFS_H__

struct s3ip_sysfs_avs_drivers_s {
    int (*get_avs_number)(void);
    ssize_t (*get_avs_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
    ssize_t (*set_debug_avs_attr)(unsigned int type, unsigned int index, const char *value);
};

extern int s3ip_sysfs_avs_drivers_register(struct s3ip_sysfs_avs_drivers_s *drv);
extern void s3ip_sysfs_avs_drivers_unregister(void);

#endif /* __AVS_SYSFS_H__ */