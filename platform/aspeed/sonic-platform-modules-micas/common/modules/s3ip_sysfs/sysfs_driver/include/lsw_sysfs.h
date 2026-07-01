#ifndef __LSW_SYSFS_H__
#define __LSW_SYSFS_H__

struct s3ip_sysfs_lsw_drivers_s {
    int (*get_lsw_number)(void);
    int (*set_lsw_attr)(unsigned int index, unsigned int type, unsigned int value);
    ssize_t (*get_lsw_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_lsw_drivers_register(struct s3ip_sysfs_lsw_drivers_s *drv);
extern void s3ip_sysfs_lsw_drivers_unregister(void);

#endif /* __LSW_SYSFS_H__ */