#ifndef _SYSEEPROM_SYSFS_H_
#define _SYSEEPROM_SYSFS_H_

struct s3ip_sysfs_syseeprom_drivers_s {
    int (*get_syseeprom_size)(void);
    ssize_t (*read_syseeprom_data)(char *buf, loff_t offset, size_t count);
    ssize_t (*write_syseeprom_data)(char *buf, loff_t offset, size_t count);
};

extern int s3ip_sysfs_syseeprom_drivers_register(struct s3ip_sysfs_syseeprom_drivers_s *drv);
extern void s3ip_sysfs_syseeprom_drivers_unregister(void);
#endif /*_SYSEEPROM_SYSFS_H_ */
