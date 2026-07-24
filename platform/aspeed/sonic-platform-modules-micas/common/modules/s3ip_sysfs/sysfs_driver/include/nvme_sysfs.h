#ifndef __NVME_SYSFS_H__
#define __NVME_SYSFS_H__

struct s3ip_sysfs_nvme_drivers_s {
    int (*get_nvme_number)(void);
    int (*set_nvme_attr)(unsigned int index, unsigned int type, unsigned int value);
    ssize_t (*get_nvme_attr)(unsigned int index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_nvme_temp_attr)(unsigned int nvme_index, unsigned int temp_index, unsigned int type, char *buf, size_t count);
    ssize_t (*get_nvme_power_attr)(unsigned int nvme_index, unsigned int power_index, unsigned int type, char *buf, size_t count);
    int (*get_nvme_temp_number)(void);
    int (*get_nvme_power_number)(void);
};

extern int s3ip_sysfs_nvme_drivers_register(struct s3ip_sysfs_nvme_drivers_s *drv);
extern void s3ip_sysfs_nvme_drivers_unregister(void);

#endif /* __NVME_SYSFS_H__ */