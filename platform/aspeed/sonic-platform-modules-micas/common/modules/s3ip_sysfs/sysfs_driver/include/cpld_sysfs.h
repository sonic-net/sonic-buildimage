#ifndef _CPLD_SYSFS_H_
#define _CPLD_SYSFS_H_

struct s3ip_sysfs_cpld_drivers_s {
    int (*get_main_board_cpld_number)(void);
    int (*set_main_board_cpld_attr)(unsigned int cpld_index, unsigned int type, unsigned int value);
    ssize_t (*get_main_board_cpld_attr)(unsigned int cpld_index, unsigned int type, char *buf, size_t count);
};

extern int s3ip_sysfs_cpld_drivers_register(struct s3ip_sysfs_cpld_drivers_s *drv);
extern void s3ip_sysfs_cpld_drivers_unregister(void);
#endif /*_CPLD_SYSFS_H_ */
